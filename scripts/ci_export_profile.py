#!/usr/bin/env python3
"""Fast end-to-end MP4 smoke check and wall-clock profiling for Linux CI.

Run inside xvfb-run using the already-built desktop executable.
This is throughput profiling, not a CPU/GPU flamegraph or FPS regression gate.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT, FPS, DURATION = 320, 180, 12, 3
EXPECTED_FRAMES = FPS * DURATION


def run(*argv, timeout=180):
    return subprocess.run(argv, cwd=ROOT, check=True, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)


def validate_mp4(path):
    probe = run("ffprobe", "-v", "error", "-count_frames",
                "-show_entries", "format=duration:stream=codec_type,codec_name,width,height,nb_read_frames",
                "-of", "json", str(path), timeout=30)
    metadata = json.loads(probe.stdout)
    streams = metadata.get("streams", [])
    video = next((item for item in streams if item.get("codec_type") == "video"), None)
    audio = next((item for item in streams if item.get("codec_type") == "audio"), None)
    if video is None or audio is None:
        raise RuntimeError("Expected both video and audio streams")
    if (video.get("codec_name"), video.get("width"), video.get("height")) != ("h264", WIDTH, HEIGHT):
        raise RuntimeError(f"Unexpected video stream: {video}")
    if audio.get("codec_name") != "aac":
        raise RuntimeError(f"Unexpected audio codec: {audio.get('codec_name')}")
    if int(video.get("nb_read_frames", -1)) != EXPECTED_FRAMES:
        raise RuntimeError(f"Expected {EXPECTED_FRAMES} decoded frames, got {video.get('nb_read_frames')}")
    seconds = float(metadata.get("format", {}).get("duration", 0))
    if not DURATION - 0.15 <= seconds <= DURATION + 0.15:
        raise RuntimeError(f"Unexpected container duration: {seconds}s")
    # ffprobe inspects streams, while actual decoding catches corrupt frame data.
    run("ffmpeg", "-v", "error", "-xerror", "-i", str(path), "-map", "0:v:0",
        "-map", "0:a:0", "-f", "null", "-", timeout=30)
    return seconds


def main():
    executable = ROOT / "build/cmake/runtime/project"
    if not executable.is_file():
        raise RuntimeError(f"Missing executable: {executable}")
    with tempfile.TemporaryDirectory(prefix="opengl-mp4-smoke-") as directory:
        output = Path(directory) / "smoke.mp4"
        args = [sys.executable, str(ROOT / "tools/export_mp4.py"),
                "--executable", str(executable), "--duration", str(DURATION),
                "--resolution", f"{WIDTH}x{HEIGHT}", "--fps", str(FPS),
                "--preset", "ultrafast", "--crf", "35", "--output", str(output)]
        begin = time.perf_counter()
        export = run(*args)
        elapsed = time.perf_counter() - begin
        if not output.is_file() or output.stat().st_size == 0:
            raise RuntimeError("No nonempty MP4 created")
        verify_begin = time.perf_counter()
        duration = validate_mp4(output)
        verify_elapsed = time.perf_counter() - verify_begin
        size = output.stat().st_size
        print(export.stdout)
        print(f"MP4 smoke: {EXPECTED_FRAMES} frames, {WIDTH}x{HEIGHT}, "
              f"{duration:.2f}s, {size} bytes")
        print(f"Export wall time: {elapsed:.2f}s; validation wall time: {verify_elapsed:.2f}s")
        print(f"Export throughput: {EXPECTED_FRAMES / max(elapsed, 1e-9):.2f} frames/s (includes startup, simulation, CPU OpenGL rendering, encoding and mixing)")
        summary = os.environ.get("GITHUB_STEP_SUMMARY")
        if summary:
            with open(summary, "a", encoding="utf-8") as output_summary:
                output_summary.write(
                    "### Fast MP4 export and profiling (Mesa software GL)\n\n"
                    "| Metric | Value |\n|---|---:|\n"
                    f"| Video | {WIDTH}×{HEIGHT}, {FPS} fps, {DURATION}s |\n"
                    f"| Decoded frames | {EXPECTED_FRAMES} |\n"
                    f"| File size | {size:,} bytes |\n"
                    f"| Export wall time | {elapsed:.2f}s |\n"
                    f"| Validation wall time | {verify_elapsed:.2f}s |\n"
                    f"| Export throughput | {EXPECTED_FRAMES / max(elapsed, 1e-9):.2f} frames/s |\n\n"
                    "Timing is informational, not a failure threshold; Mesa software rendering is not representative of a hardware GPU.\n")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired,
            ValueError, KeyError, json.JSONDecodeError) as exc:
        print(f"MP4 smoke failed: {exc}", file=sys.stderr)
        sys.exit(1)
