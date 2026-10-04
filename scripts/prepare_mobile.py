#!/usr/bin/env python3
"""Fetch pinned mobile sources and prepare APK/bundle assets (host FFmpeg required)."""
import hashlib
import shutil
import subprocess
import tarfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPENDENCIES = [
    ("SDL", "https://github.com/libsdl-org/SDL/releases/download/release-3.2.10/SDL3-3.2.10.tar.gz", "f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0"),
    ("glm", "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz", "9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c"),
]

def main():
    deps = ROOT / "build/mobile-deps"
    deps.mkdir(parents=True, exist_ok=True)
    for name, url, checksum in DEPENDENCIES:
        destination = deps / name
        stamp = destination / ".source-sha256"
        if stamp.exists() and stamp.read_text().strip() == checksum:
            continue
        archive = deps / (name + ".tar.gz")
        if not archive.exists():
            request = urllib.request.Request(url, headers={"User-Agent": "SpaceCinematic-build"})
            with urllib.request.urlopen(request, timeout=120) as response, archive.open("wb") as output:
                shutil.copyfileobj(response, output)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != checksum:
            raise RuntimeError("Dependency checksum mismatch: " + name)
        staging = deps / (name + "-extract")
        shutil.rmtree(staging, ignore_errors=True)
        staging.mkdir()
        with tarfile.open(archive) as source:
            for member in source.getmembers():
                path = Path(member.name)
                if path.is_absolute() or ".." in path.parts or member.issym() or member.islnk():
                    raise RuntimeError("Unsafe archive entry")
                member.name = str(Path(*path.parts[1:]))
                if member.name != ".":
                    source.extract(member, staging)
        shutil.rmtree(destination, ignore_errors=True)
        staging.rename(destination)
        stamp.write_text(checksum + "\n")
    assets = ROOT / "build/mobile-assets"
    for name in ("shaders", "textures", "models"):
        shutil.copytree(ROOT / name, assets / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / "third_party/licenses", assets / "licenses", dirs_exist_ok=True)
    music = assets / "build/music"
    music.mkdir(parents=True, exist_ok=True)
    for source, target in (("Can You Hear The Music.mp3", "cinematic.wav"), ("asteroid-hitting-something.mp3", "impact.wav")):
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", str(ROOT / "audio" / source), "-ar", "48000", "-ac", "2", "-c:a", "pcm_s16le", str(music / target)], check=True)
    print("Mobile dependencies and assets ready:", assets)

if __name__ == "__main__":
    main()
