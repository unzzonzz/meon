# MEON 진행 보고

SonoBus 1.7.2(upstream `sonosaurus/sonobus`, 커밋 35f1062d)를 포크한 MEON 합주 MVP 작업 기록.
각 단계마다 **한 것 / 막힌 것 / 판단이 필요한 것**을 남긴다.

---

## 1단계: 원본 빌드 (2026-09-22)

### 환경
- macOS 26.6.2 (Apple Silicon), Xcode 27.0 설치됨(단, 라이선스 미동의 상태), Command Line Tools clang 21, CMake 4.4.0
- Logic Pro 설치됨, 이 Mac에는 SonoBus가 설치되어 있지 않음

### 한 것
- `upstream` 리모트(sonosaurus/sonobus) 확인. `origin/main == upstream/main` (1.7.2 그대로).
- 원본 그대로 빌드 성공: **독립 앱, AU, VST3** (LV2도 함께 생성됨). arm64 단독 빌드와 기본 설정인 유니버설(x86_64+arm64) 빌드 둘 다 확인.
  ```bash
  export DEVELOPER_DIR=/Library/Developer/CommandLineTools   # Xcode 라이선스 미동의 상태일 때만 필요
  cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build --config Release -j 8
  # 결과: build/SonoBus_artefacts/Release/{Standalone,AU,VST3}
  ```
- **빌드 오류 1건을 빌드 설정으로 해결** (JUCE 소스는 손대지 않음):
  macOS 26/27 SDK에서 `CGWindowListCreateImage`가 obsoleted(15.0) 처리되어 JUCE의 헬퍼 도구 `juceaide` 컴파일이 실패했다.
  원인은 JUCE가 juceaide를 별도 cmake 프로세스로 빌드하면서 deployment target을 넘기지 않아 호스트 OS 버전(26.x)이 기본값이 되는 것.
  `CMakeLists.txt`에서 deployment target을 10.13으로 올리고 `MACOSX_DEPLOYMENT_TARGET` 환경변수를 하위 cmake에 전달하도록 했다.
- 독립 앱 실행 확인(창이 정상적으로 뜸, 한국어 UI 포함).
- AU 검증 통과: `auval -v aumf NBus Sono` → `AU VALIDATION SUCCEEDED` (Logic이 쓰는 검증과 동일한 도구).
- VST3: 번들을 직접 로드해 `GetPluginFactory`로 클래스 3개(Audio Module / Controller / Compatibility) 열거 확인.
- JUCE 버전 정정: 실제 빌드에 쓰이는 `deps/juce`는 **JUCE 7.0.8**(essej/JUCE `sono7good` 브랜치)이다. 최상위 `JUCE/` 폴더(6.0.8)는 빌드에 쓰이지 않는 잔재. JUCE 7도 GPLv3/상용 이중 라이선스라 조건은 같다. 올리지 않는다.
- **Windows ASIO 라이선스 조사 결과**:
  - 2025년 10월 Steinberg가 ASIO SDK **2.3.4**(2025-10-15)를 **proprietary / GPLv3 이중 라이선스**로 변경했다. (VST3 SDK 3.8은 MIT.)
  - SDK 동봉 README FAQ: "Can I switch to GPLv3 License? — Yes, if your product is released under GPLv3 License terms (including source code disclosure)."
  - 따라서 GPLv3로 전체 소스를 공개하는 MEON은 **ASIO를 포함한 Windows 빌드를 배포할 수 있다.** (Audacity가 과거에 ASIO 없이 배포한 문제는 해소됨.)
  - 다운로드: https://www.steinberg.net/asiosdk (→ `ASIO-SDK_2.3.4_2025-10-15.zip`). 원본 CMake는 `../asiosdk/common`을 참조하므로 Windows PC에서는 저장소 폴더 옆에 `asiosdk/`로 풀어둔다.
  - 근거: KVR/heise/Libre Arts 보도(2025-10-29), SDK 동봉 LICENSE.txt·README.md(미러 github.com/audiosdk/asio).

### 막힌 것
- **Xcode 라이선스 미동의**: `/usr/bin/git`, `xcodebuild`, `lipo` 등 Xcode 셔임이 모두 "You have not agreed to the Xcode license" 로 실패한다.
  Command Line Tools(`DEVELOPER_DIR=/Library/Developer/CommandLineTools`)로 우회해 빌드·git을 진행했다.
  Xcode 프로젝트 생성(`setupcmakexcode.sh`)이나 Xcode 빌드가 필요하면 터미널에서 한 번 실행해야 한다:
  ```bash
  sudo xcodebuild -license accept
  ```
- **Logic에서 실제 로드**: auval은 통과했지만 Logic 앱을 직접 조작해 확인하지는 못했다(화면 제어 권한 필요). Logic > 설정 > 플러그인 관리자에서 "Sonosaurus: SonoBus"가 성공으로 표시되는지 한 번 확인 필요.
- **Windows 빌드: 미확인** (Windows PC에서 확인 예정).

### 판단이 필요한 것
- "VST"는 **VST3**로 진행한다. VST2 SDK는 Steinberg가 배포를 중단했고 원본도 `VST2_SDK_PATH`가 없으면 VST2를 만들지 않는다.
- ASIO SDK를 저장소에 넣을지: GPLv3 이중 라이선스라 재배포는 가능해 보이지만, 동봉 LICENSE.txt에 "prior written agreement" 문구가 남아 있어 확답이 어렵다. 우선은 저장소에 넣지 않고 Windows 빌드 시 다운로드해 쓰는 방식으로 간다.
- 최소 macOS를 10.10 → **10.13**으로 올렸다 (최신 SDK가 10.13 미만을 지원하지 않음).
