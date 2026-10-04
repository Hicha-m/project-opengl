"""Extract and run the actual distributable from a Unicode path and unrelated CWD."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("packages", type=Path)
parser.add_argument("--mesa", type=Path, help="Windows CI software renderer; never added to the archive")
args = parser.parse_args()
archives = sorted(args.packages.glob("*.zip")) + sorted(args.packages.glob("*.tar.gz"))
if len(archives) != 1:
    raise SystemExit(f"Expected one package, found {archives}")

with tempfile.TemporaryDirectory(prefix="space-étoiles-") as directory:
    relocated = Path(directory) / "application avec espaces"
    if sys.platform == "darwin":
        subprocess.run(["ditto", "-x", "-k", str(archives[0].resolve()), str(relocated)], check=True)
    else:
        shutil.unpack_archive(str(archives[0]), str(relocated))
    names = ("project.exe",) if os.name == "nt" else ("project",)
    executables = [p for name in names for p in relocated.rglob(name) if p.is_file()]
    if len(executables) != 1:
        raise SystemExit(f"Expected one executable, found {executables}")
    executable = executables[0]
    if args.mesa:
        renderer = args.mesa / "opengl32.dll"
        if not renderer.is_file():
            raise SystemExit(f"Mesa renderer missing: {renderer}")
        for library in args.mesa.glob("*.dll"):
            shutil.copy2(library, executable.parent / library.name)
    environment = dict(os.environ, SDL_AUDIO_DRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    if os.name == "posix" and ".app" not in str(executable):
        libraries = subprocess.check_output(["ldd", "./project"], cwd=executable.parent, text=True)
        print(libraries, flush=True)
        for name in ("libSDL3", "libglfw", "libGLEW"):
            lines = [line for line in libraries.splitlines() if name in line]
            if not lines or any(str(relocated) not in line for line in lines):
                raise SystemExit(f"{name} must load from the package")
    if ".app/Contents/MacOS/" in executable.as_posix():
        bundle = executable.parent.parent.parent
        subprocess.run(["codesign", "--verify", "--deep", "--strict", str(bundle)], check=True)
    subprocess.run([str(executable), "--smoke-test"], cwd=directory,
                   env=environment, check=True, timeout=120)
    print(f"Verified relocated package: {archives[0].name}")
