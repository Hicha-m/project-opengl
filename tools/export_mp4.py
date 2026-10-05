#!/usr/bin/env python3
"""Render the cinematic at a fixed frame rate and encode an MP4 with FFmpeg."""
import argparse
from array import array
import math
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import wave

ROOT = Path(__file__).resolve().parents[1]
RESOLUTIONS = {"720p": (1280, 720), "1080p": (1920, 1080), "1440p": (2560, 1440), "4k": (3840, 2160)}


def resolution(value):
    if value.lower() in RESOLUTIONS:
        return RESOLUTIONS[value.lower()]
    try:
        width, height = map(int, value.lower().split("x"))
        if width <= 0 or height <= 0 or width > 7680 or height > 4320 or width % 2 or height % 2:
            raise ValueError
        return width, height
    except ValueError:
        raise argparse.ArgumentTypeError("Use 720p, 1080p, 1440p, 4k or even dimensions such as 1920x1080")


def default_executable():
    for candidate in (ROOT / "build/cmake/runtime/project", ROOT / "build/cmake/runtime/project.exe",
                      ROOT / "build/cmake/runtime/project.app/Contents/MacOS/project", ROOT / "project", ROOT / "project.exe", ROOT / "project.app/Contents/MacOS/project"):
        if candidate.is_file():
            return candidate
    return ROOT / "build/cmake/runtime/project"


def resource_directory(executable):
    directory = executable.parent
    for candidate in (directory, directory.parent / "Resources", ROOT):
        if (candidate / "build/music/cinematic.wav").is_file():
            return candidate
    raise RuntimeError("Audio resources missing; build the project first")


def read_pcm(path):
    with wave.open(str(path), "rb") as source:
        if (source.getnchannels(), source.getsampwidth(), source.getframerate()) != (2, 2, 48000):
            raise RuntimeError(f"Expected stereo 48 kHz signed 16-bit PCM: {path}")
        pcm = array("h", source.readframes(source.getnframes()))
    if sys.byteorder != "little":
        pcm.byteswap()
    return pcm


def mix_impacts(log, source, destination, frames):
    effect = read_pcm(source)
    mixed = array("i", [0]) * (frames * 2)
    # Match the four bounded voices used in real-time playback.
    voices = [None] * 4
    next_voice = 0
    for line in log.read_text().splitlines():
        time, gain = map(float, line.split())
        start = round(time * 48000) * 2
        selected = next((i for i, voice in enumerate(voices)
                         if voice is None or voice[0] + len(effect) <= start), next_voice)
        previous = voices[selected]
        if previous is not None:
            old_start, old_gain = previous
            for i in range(max(start, old_start), min(old_start + len(effect), len(mixed))):
                mixed[i] -= round(effect[i - old_start] * old_gain)
        for i in range(min(len(effect), max(0, len(mixed) - start))):
            mixed[start + i] += round(effect[i] * gain)
        voices[selected] = start, gain
        next_voice = (selected + 1) % 4
    pcm = array("h", (max(-32768, min(32767, sample)) for sample in mixed))
    if sys.byteorder != "little":
        pcm.byteswap()
    with wave.open(str(destination), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(48000)
        output.writeframes(pcm.tobytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "exports/cinematic.mp4")
    parser.add_argument("--executable", type=Path, default=default_executable())
    parser.add_argument("--resolution", type=resolution, default=RESOLUTIONS["720p"],
                        help="720p (default), 1080p, 1440p, 4k or WIDTHxHEIGHT")
    parser.add_argument("--fps", type=int, default=30, help="Frames per second, 1–240 (default: 30)")
    parser.add_argument("--crf", type=int, default=18, help="H.264 quality, 0–51; lower is better (default: 18)")
    parser.add_argument("--preset", choices=["ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow"], default="medium",
                        help="Encoder speed; slower gives smaller files at comparable quality")
    parser.add_argument("--duration", type=float, help="Seconds to render; defaults to the complete music duration")
    parser.add_argument("--no-audio", action="store_true", help="Export silent video")
    parser.add_argument("--software-context", action="store_true", help="Use OSMesa (requires GLFW 3.4+ and OSMesa)")
    args = parser.parse_args()
    if not 1 <= args.fps <= 240 or not 0 <= args.crf <= 51:
        parser.error("fps must be 1–240 and crf 0–51")
    if args.duration is not None and (not math.isfinite(args.duration) or not 0 < args.duration <= 3600):
        parser.error("duration must be greater than zero and at most 3600 seconds")
    executable = args.executable.resolve()
    if not executable.is_file():
        parser.error(f"Executable missing: {executable}. Build the project first.")
    if not shutil.which("ffmpeg"):
        parser.error("FFmpeg is required on PATH")
    output = args.output.resolve()
    if output.suffix.lower() != ".mp4":
        parser.error("output must end in .mp4")
    if output.exists():
        parser.error(f"Output already exists: {output}; choose another filename")
    resources = resource_directory(executable)
    music = resources / "build/music/cinematic.wav"
    impact = resources / "build/music/impact.wav"
    with wave.open(str(music), "rb") as audio:
        duration = args.duration or audio.getnframes() / audio.getframerate()
    width, height = args.resolution
    count = math.ceil(duration * args.fps)
    output.parent.mkdir(parents=True, exist_ok=True)
    print(f"Export: {width} × {height}, {args.fps} fps, {duration:.2f} s, CRF {args.crf}, {args.preset}", flush=True)
    # Temporary files contain compressed video and an audio mix, never raw frame sequences.
    with tempfile.TemporaryDirectory(prefix="cinematic-export-", dir=output.parent) as temporary:
        temporary = Path(temporary)
        video, log = temporary / "video.mp4", temporary / "impacts.txt"
        command = [str(executable), "--export-frames", "--width", str(width), "--height", str(height),
                   "--fps", str(args.fps), "--duration", str(duration), "--impact-log", str(log)]
        if args.software_context:
            command.append("--software-context")
        renderer = subprocess.Popen(command, stdout=subprocess.PIPE)
        encoder = None
        try:
            encoder = subprocess.Popen(["ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin",
                "-f", "rawvideo", "-pixel_format", "rgb24", "-video_size", f"{width}x{height}",
                "-framerate", str(args.fps), "-i", "pipe:0", "-vf", "vflip", "-an",
                "-c:v", "libx264", "-crf", str(args.crf), "-preset", args.preset,
                "-pix_fmt", "yuv420p", str(video)], stdin=renderer.stdout)
            renderer.stdout.close()
            render_code = renderer.wait()
            encode_code = encoder.wait()
            if render_code or encode_code:
                raise RuntimeError(f"Export failed (renderer: {render_code}, FFmpeg: {encode_code})")
        finally:
            if renderer.stdout and not renderer.stdout.closed:
                renderer.stdout.close()
            for process in (renderer, encoder):
                if process is not None and process.poll() is None:
                    process.terminate()
                    process.wait()
        if args.no_audio:
            subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", str(video),
                            "-c", "copy", "-movflags", "+faststart", str(temporary / "final.mp4")], check=True)
        else:
            effects = temporary / "effects.wav"
            mix_impacts(log, impact, effects, math.ceil(count / args.fps * 48000))
            subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", str(video), "-i", str(music), "-i", str(effects),
                "-filter_complex", "[1:a]volume=0.7,apad[m];[m][2:a]amix=inputs=2:duration=longest:normalize=0[a]",
                "-map", "0:v:0", "-map", "[a]", "-c:v", "copy", "-c:a", "aac", "-b:a", "192k",
                "-t", str(count / args.fps), "-movflags", "+faststart", str(temporary / "final.mp4")], check=True)
        # Only publish a completed export; keep an existing destination intact.
        with output.open("xb") as destination, (temporary / "final.mp4").open("rb") as source:
            shutil.copyfileobj(source, destination)
    print(f"MP4 saved: {output}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError, wave.Error) as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("Export cancelled", file=sys.stderr)
        sys.exit(130)
