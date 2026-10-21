# Build profiles: production versus performance investigation

Profiling is **compile-time opt-in**. Normal builds do not contain the `FrameProfiler` member, GPU timer queries or the profiling command-line switch. Production assets and packages are not changed by the profiler.

## Production (default)

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DENABLE_PROFILING=OFF
cmake --build build/release --parallel 2
```

`ENABLE_PROFILING` defaults to `OFF` and is therefore optional. A production executable does not accept `--profile-output`.

## Development / profiling

```bash
cmake -S . -B build/profile -DCMAKE_BUILD_TYPE=Release -DENABLE_PROFILING=ON
cmake --build build/profile --parallel 2
python3 tools/export_mp4.py --executable build/profile/runtime/project \
  --resolution 320x180 --fps 12 --duration 3 --crf 35 --preset ultrafast \
  --output build/profile-smoke.mp4 --profile-json build/profile-results.json
```

Use a separate CMake build directory: CMake options are cached, and toggling a single build tree between profiles can invalidate measurements. If running headlessly on Linux, prepend `xvfb-run -a` to the Python command and set `LIBGL_ALWAYS_SOFTWARE=1`.

The report contains a frame count, simulation step count, CPU section wall times, GPU elapsed times for render stages, and sample counts. CPU timings around OpenGL calls include driver submission or waits; they are **not** pure CPU execution times. GPU elapsed times use deferred `GL_TIME_ELAPSED` queries; queries are resolved at the end of the video to avoid per-frame synchronization.

## GitHub Actions

The existing **Ubuntu 24.04 / GCC / Release** matrix entry is the **only** profiling build. It runs the same short 3-second MP4 smoke test with profiling enabled. The Ubuntu Clang, macOS, and Windows builds all compile with profiling disabled; release artifacts are uploaded only from non-profiled jobs (or Ubuntu ARM, which also has profiling disabled). There is no extra runner and no extra matrix entry.

These timings are diagnostic only: Mesa's software renderer does not represent discrete GPU timing or throughput. No performance regression threshold is enforced, as shared CI runner performance is variable.

## Limits

A three-second opening segment does not measure high-load meteor shower or Earth breakup scenes. Investigate those with a longer local profiling render before drawing optimization conclusions. Never compare measured GPU milliseconds from Mesa directly to physical GPUs.


## High-load benchmark (separate from packaged application)

Use `scripts/ci_heavy_profile.py` with an `ENABLE_PROFILING=ON` build to compare two **2-second** windows: opening (0–2s) and bombardment (48–50s). It simulates every 1/60-second step from the start to the late window before collecting timed frames, so it preserves meteor and particle history instead of simply jumping the animation clock. Pre-roll is excluded from section timings.

```bash
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PROFILING=ON
cmake --build build/cmake --parallel 2
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a python3 scripts/ci_heavy_profile.py
```

The script discards RGB frames rather than encoding video, avoiding FFmpeg and audio costs for these GPU/CPU stage comparisons. It reports end-to-end wall time **including** simulated pre-roll separately from the per-stage results, and adds an opening-versus-bombardment table to the GitHub Actions job summary. The short MP4 smoke test remains unchanged.

The `--profile-start` renderer argument exists **only** in profiling builds, requires `--profile-output`, and does not affect production builds or normal exported MP4s. Do not interpret a slow pre-roll as slow per-frame rendering. As before, Mesa measurements are diagnostic only.
