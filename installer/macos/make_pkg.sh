#!/bin/sh
# Pack the built VST3 into a macOS installer package that puts it in the
# system-wide plug-in folder (/Library/Audio/Plug-Ins/VST3).
#
# Usage: make_pkg.sh <path/to/dirty630.vst3> <version> <out.pkg>
set -eu

vst3="$1"
ver="$2"
out="$3"

pkgbuild --component "$vst3" \
         --install-location /Library/Audio/Plug-Ins/VST3 \
         --identifier com.minus.dirty630.vst3 \
         --version "$ver" \
         "$out"
