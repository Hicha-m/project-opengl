# CI: fast feedback, full portability and measurable coverage

## Policy

This project has a **Linux-only fast PR gate** plus a full cross-platform verification workflow:

| Trigger | Matrix | OpenGL smoke | MP4 export | Packages and Docker |
|---|---|---|---|---|
| Pull request | Linux GCC Release + Linux Clang Debug | Linux GCC Release (Mesa + Xvfb) | Linux GCC Release (H.264 + AAC validated with FFmpeg) | Omitted for fast feedback |
| Push to `master`, version tag, or manual `workflow_dispatch` | All 9 OS/compiler/configuration combinations | All Linux builds | Linux GCC Release | Package relocation on each OS; Docker build/test; signed release package artifacts |

All selected matrix jobs run **real C++ unit tests** through CTest. Linux GCC Release additionally runs the hidden-window OpenGL integration test on PRs, which initializes rendering, music and impact audio and draws frames. The full release matrix runs this test on **all** Linux targets. Linux GCC Release verifies an actual exported MP4 (36 frames, audio/video streams, duration, full FFmpeg decode), rather than only compiling the project.

For now PRs deliberately run Linux **only** to establish a stable CI and renderer performance baseline. Windows/macOS are **not validated on PRs**, but remain configured and tested on the full matrix (push to `master`, release tags and manual dispatch). A Pull Request is **not** equivalent to a release validation: the complete 9-target matrix, packaged application relocation and Docker are deliberately retained for `master`, tags and manual runs. Running `workflow_dispatch` provides a full verification pass without merging a PR. A Draft PR is sufficient to trigger the fast checks.

Profiling is compile-time opt-in on the Linux GCC Release validation target. It does not enable profiling on distributable packages. Experimental high-load benchmarks remain on `perf/benchmarks`, not `master`.

## Job metrics

Every workflow run has a final **CI metrics** job using `if: always()`, including when another job fails. It requests read-only Actions access and uses the standard Python library to summarize:

- Passed / failed / cancelled jobs.
- Critical-path elapsed seconds from the earliest matrix start through the last completed prerequisite job (**excluding** the reporting job).
- Sum of job execution seconds across parallel runners (**not** the same thing as wall-clock time or GitHub-billed minutes).
- Duration and result per job; the ten slowest individual steps.
- Grouped setup, CMake, compilation, CTest, rendering/export and packaging durations.
- CTest case counts, failed/error cases and skipped cases from each job's JUnit XML.

The **CI metrics** job writes a Markdown table to the GitHub Actions run summary and uploads `ci-metrics.json` as a 30-day machine-readable artifact. Each build uploads its CTest XML as a `ctest-*` artifact retained for 14 days.

CTest counts are **executions across platforms**, not a count of distinct test names. A build failing before tests cannot produce a JUnit file; the report calls out missing reports rather than inventing successful tests. Timing on shared runners is diagnostic, and is not used as a performance regression gate. Mesa software GPU timer readings are not representative of physical GPU performance.

To exercise the Python metrics parser locally:

```bash
python3 -m unittest discover -s tests -p test_ci_metrics.py
python3 scripts/ci_metrics.py --jobs-json /path/to/jobs-api-response.json \
  --junit-dir /path/to/junit-folder --output /tmp/ci-metrics.json
```

## Why this is faster and more reliable

Previous runs on 22 October 2026 were dominated by APT and SDL setup, not by C++ compilation. The workflow now selects only missing Linux packages, bypasses the repeatedly slow Azure Ubuntu mirror, disables SDL's redundant installation of system packages, and reuses the built-in SDL cache. The Linux dependency fix was already validated in the experimental PR #13.

On PRs, this version runs **2 of 9** matrix jobs (Linux GCC Release and Linux Clang Debug), avoids Docker's second C++/OpenGL build and all Windows/macOS setup, packaging, or rendering tests. It also avoids a redundant OpenGL runtime test on PR Clang Debug while retaining the Clang Debug compile and C++ unit tests. The slower redundant runtime check remains enabled for the full release matrix. The primary C++ tests and actual Linux rendering smoke remain active. The reduction in wall-clock time must be measured from actual runs, not inferred from job count alone.

## Local verification

```bash
cmake -S . -B build/ci -DCMAKE_BUILD_TYPE=Release -DENABLE_RUNTIME_TESTS=ON
cmake --build build/ci --parallel 2
ctest --test-dir build/ci -L unit --output-on-failure --output-junit unit.xml
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a \
  ctest --test-dir build/ci -L runtime --output-on-failure --output-junit runtime.xml
```

For regression testing, compare similar event types and the same runner/OS. Do not compare the shortened PR matrix directly with historical full-matrix jobs to claim a per-job speedup. Use timestamps and slowest steps from the CI metrics summary.


## Prepared Windows path for full validation

Full-matrix Windows jobs remain available for manual verification, master and release tags. Their vcpkg dependencies now use `actions/cache@v4` to persist **binary package archives** in `${{ runner.temp }}/vcpkg-binary-cache` using the triplet and `vcpkg.json` hash, rather than caching the CMake-specific `build/cmake/vcpkg_installed` directory. The runner exports `VCPKG_DEFAULT_BINARY_CACHE` so vcpkg can reuse those archives on subsequent runs. This cache does **not** reuse compiled application outputs and is safe to regenerate.

The vcpkg git checkout is still pinned to the exact manifest baseline, and bootstrap/configure operations check their exit status. The initial Windows full-matrix run after this change may still be slow while populating its cache. **This Windows optimization is not covered by Linux-only PR CI** and must be verified by a manual full-matrix run before merging/releasing.

Do not interpret Linux-only PR success as validation of Windows/macOS packages.
