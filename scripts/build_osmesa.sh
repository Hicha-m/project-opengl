#!/usr/bin/env bash
# CPU OpenGL for macOS CI runners, which do not expose a suitable native GPU.
set -euo pipefail
task_prefix="$1"
task_workspace="${RUNNER_TEMP:-/tmp}/space-osmesa-build"
mkdir -p "$task_workspace" "$task_prefix"
curl -fsSL --retry 3 https://archive.mesa3d.org/mesa-24.3.4.tar.xz -o "$task_workspace/mesa.tar.xz"
python3 - "$task_workspace/mesa.tar.xz" <<'PY'
import hashlib, pathlib, sys
expected = 'e641ae27191d387599219694560d221b7feaa91c900bcec46bf444218ed66025'
actual = hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest()
if actual != expected:
    raise SystemExit(f'Mesa checksum mismatch: {actual}')
PY
mkdir -p "$task_workspace/source"
tar -xf "$task_workspace/mesa.tar.xz" -C "$task_workspace/source" --strip-components=1
python3 -m venv "$task_workspace/python"
"$task_workspace/python/bin/pip" install 'meson==1.7.0' 'mako==1.3.8' 'packaging==24.2' 'PyYAML==6.0.2'
export PATH="$task_workspace/python/bin:$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
"$task_workspace/python/bin/meson" setup "$task_workspace/build" "$task_workspace/source" \
  --prefix "$task_prefix" --buildtype release \
  -Dgallium-drivers=softpipe -Dplatforms=[] -Dvulkan-drivers=[] \
  -Dosmesa=true -Dllvm=disabled -Dglx=disabled -Degl=disabled -Dgbm=disabled \
  -Dgles1=disabled -Dgles2=disabled -Dshared-glapi=disabled \
  -Dgallium-va=disabled -Dgallium-vdpau=disabled -Dgallium-xa=disabled \
  -Dbuild-tests=false -Dvideo-codecs=[]
ninja -C "$task_workspace/build" -j 3
ninja -C "$task_workspace/build" install
