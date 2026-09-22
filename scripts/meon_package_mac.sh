#!/bin/bash
# MEON 테스터용 macOS 배포 zip 만들기 (서명·공증 없음)
# 사용: ./scripts/meon_package_mac.sh [build 폴더]   (기본: build)
set -e
BUILD=${1:-build}
ART="$BUILD/MEON_artefacts/Release"
VER=$(grep -E "^project\(MEON VERSION" CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
OUT="release/MEON-$VER-mac"
if [ ! -d "$ART/Standalone/MEON.app" ]; then
  echo "빌드 결과가 없습니다: $ART (먼저 cmake --build $BUILD --config Release)"; exit 1
fi
rm -rf "$OUT" "$OUT.zip"
mkdir -p "$OUT/AU" "$OUT/VST3"
cp -R "$ART/Standalone/MEON.app" "$OUT/"
cp -R "$ART/AU/MEON.component" "$OUT/AU/"
cp -R "$ART/VST3/MEON.vst3" "$OUT/VST3/"
cp doc/INSTALL.md "$OUT/설치 안내.md"
cp LICENSE "$OUT/LICENSE.txt"
cp LICENSE_EXCEPTION "$OUT/LICENSE_EXCEPTION.txt"
# ad-hoc 서명 (서명이 아예 없으면 최신 macOS 에서 실행이 막힌다)
codesign --force --deep --sign - "$OUT/MEON.app" 2>/dev/null || true
codesign --force --deep --sign - "$OUT/AU/MEON.component" 2>/dev/null || true
codesign --force --deep --sign - "$OUT/VST3/MEON.vst3" 2>/dev/null || true
(cd release && zip -qry "MEON-$VER-mac.zip" "MEON-$VER-mac")
echo "만들어짐: $OUT.zip"
ls -la "$OUT.zip"
