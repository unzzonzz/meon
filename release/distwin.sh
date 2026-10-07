#!/bin/bash
# Windows 배포: build/ 의 Release 결과물을 MEON\ 에 모으고 Inno Setup 으로 설치 프로그램을 만든다.
# 사용법 (release/ 에서, Git Bash): ./distwin.sh <version>
# 결과: release/instoutput/MEON-<version>-Installer.exe
# 코드 서명은 여기서 하지 않는다. CI (.github/workflows/windows.yml) 가 SignPath 로 설치 파일을 서명한다.

set -e

if [ -z "$1" ] ; then
   echo "Usage: $0 <version>"
   exit 1
fi

VERSION=$1
BUILDDIR='../build/MEON_artefacts/Release'

rm -rf MEON
mkdir -p MEON/Plugins/VST3

cp -v ../doc/README_WINDOWS.txt MEON/README.txt
cp -v ${BUILDDIR}/Standalone/MEON.exe MEON/
cp -pRv ${BUILDDIR}/VST3/MEON.vst3 MEON/Plugins/VST3/

mkdir -p instoutput
rm -f instoutput/*

ISCC=${ISCC:-iscc}
"$ISCC" //O"instoutput" //DSBVERSION="${VERSION}" wininstaller.iss
