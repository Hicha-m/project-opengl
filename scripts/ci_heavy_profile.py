#!/usr/bin/env python3
"""Compare opening versus bombardment stages using the profiling-only renderer.

No FFmpeg or MP4: raw RGB frames go to DEVNULL; simulation warm-up preserves
meteor emissions and impact history. Runs on the existing Linux GCC CI job.
"""
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = ROOT / "build/cmake/runtime/project"
FPS = 12
DURATION = 2
WIDTH, HEIGHT = 320, 180
STAGES = ("simulation", "texture_upload", "scene", "meteors",
          "breakup", "particles", "hdr", "readback", "output")


def profile(start, directory):
    report = directory / f"stage-{int(start)}.json"
    impacts = directory / f"impacts-{int(start)}.txt"
    args = [str(EXECUTABLE), "--export-frames", "--width", str(WIDTH),
            "--height", str(HEIGHT), "--fps", str(FPS),
            "--duration", str(DURATION), "--impact-log", str(impacts),
            "--profile-output", str(report), "--profile-start", str(start)]
    begin = time.perf_counter()
    with open(os.devnull, "wb") as sink:
        result = subprocess.run(args, cwd=ROOT / "build/cmake/runtime",
                                stdout=sink, stderr=subprocess.PIPE,
                                text=True, timeout=90)
    elapsed = time.perf_counter() - begin
    if result.returncode:
        raise RuntimeError(f"Profile {start}s failed: {result.stderr[-4000:]}")
    values = json.loads(report.read_text(encoding="utf-8"))
    if values["frames"] != FPS * DURATION or values["simulation_steps"] <= 0:
        raise RuntimeError(f"Bad profile counts for {start}s: {values}")
    for stage in STAGES:
        for key in ("cpu_ms", "gpu_ms", "samples"):
            amount = values[key][stage]
            if not math.isfinite(amount) or amount < 0:
                raise RuntimeError(f"Invalid {key}/{stage} for {start}s: {amount}")
    return values, elapsed


def main():
    if not EXECUTABLE.is_file():
        raise RuntimeError(f"Missing profiling executable: {EXECUTABLE}")
    with tempfile.TemporaryDirectory(prefix="cinematic-heavy-") as temporary:
        directory = Path(temporary)
        opening, opening_time = profile(0.0, directory)
        bombardment, bombardment_time = profile(48.0, directory)
        print(f"Opening {DURATION}s: {opening_time:.2f}s wall; "
              f"bombardment 48s+{DURATION}s: {bombardment_time:.2f}s wall (includes 48s simulation preroll)")
        lines = [
            "### Opening vs bombardment (profiling-only benchmark)",
            "",
            f"{FPS * DURATION} measured frames per case at {WIDTH}×{HEIGHT}, {FPS} fps.",
            "Bombardment pre-roll simulates 48 seconds without rendering, and is excluded from stage timings.",
            f"Wall: opening {opening_time:.2f}s, bombardment {bombardment_time:.2f}s (includes pre-roll).",
            "",
            "| Stage | Opening CPU ms | Bombardment CPU ms | Opening GPU ms | Bombardment GPU ms |",
            "|---|---:|---:|---:|---:|",
        ]
        for stage in STAGES:
            oc, hc = opening["cpu_ms"][stage], bombardment["cpu_ms"][stage]
            og, hg = opening["gpu_ms"][stage], bombardment["gpu_ms"][stage]
            lines.append(f"| {stage} | {oc:.3f} | {hc:.3f} | {og:.3f} | {hg:.3f} |")
        lines += ["", "GPU values are Mesa software GL timer queries; timing noise and "
                   "query overhead may dominate small stages. This is a diagnostic, not a performance gate."]
        print("\n".join(lines))
        if os.environ.get("GITHUB_STEP_SUMMARY"):
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as output:
                output.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
