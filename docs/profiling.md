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
