# MEON 설치 안내 (테스터용)

이 빌드는 코드 서명과 공증을 하지 않았습니다. 처음 열 때 운영체제 경고가 뜨는 것이 정상이며, 아래 방법으로 넘길 수 있습니다.

## macOS

### 독립 앱 (MEON.app)
1. `MEON.app` 을 `응용 프로그램` 폴더로 옮깁니다.
2. 처음 실행하면 **"확인되지 않은 개발자" / "악성 소프트웨어인지 확인할 수 없음"** 경고가 뜹니다.
3. **시스템 설정 > 개인정보 보호 및 보안** 으로 가서 아래쪽 "MEON이(가) 차단되었습니다" 옆의 **[그대로 열기]** 를 누릅니다. (한 번만 하면 됩니다.)
   - 또는 터미널에서 격리 속성을 지워도 됩니다:
     ```bash
     xattr -dr com.apple.quarantine /Applications/MEON.app
     ```
4. 첫 실행 때 **마이크 접근 권한**을 허용해 주세요.

### 플러그인
| 형식 | 설치 위치 |
|---|---|
| AU (`MEON.component`) | `~/Library/Audio/Plug-Ins/Components/` |
| VST3 (`MEON.vst3`) | `~/Library/Audio/Plug-Ins/VST3/` |

- 파일을 복사한 뒤 DAW 를 다시 실행합니다.
- **Logic Pro**: MEON 은 *MIDI 제어 이펙트(Music Effect)* 로 등록됩니다. 소프트웨어 악기 트랙의 악기 슬롯 > **AU MIDI 제어 이펙트 > MEON** 에서 고르세요. 오디오 트랙에서 쓰려면 사이드체인으로 입력을 지정하거나, 마이크를 MEON 트랙의 입력으로 잡으면 됩니다.
- 플러그인이 목록에 없거나 "검증 실패"로 나오면:
  1. 플러그인 파일에도 격리 속성이 붙어 있을 수 있습니다:
     ```bash
     xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/MEON.component
     xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/MEON.vst3
     ```
  2. AU 캐시를 갱신합니다: 터미널에서 `killall -9 AudioComponentRegistrar` 를 실행한 뒤 DAW 를 다시 엽니다.
  3. Logic: **설정 > 플러그인 관리자** 에서 MEON 을 찾아 **[선택한 플러그인 재설정 및 재검사]**.
  4. 직접 검증해 보려면: `auval -v aumf Mjam Meon`

## Windows

### 독립 앱 (MEON.exe)
1. 처음 실행하면 **SmartScreen** 창("Windows의 PC 보호")이 뜹니다.
2. **[추가 정보]** 를 누른 뒤 **[실행]** 을 누릅니다.
3. Windows 방화벽이 네트워크 접근을 물으면 **개인 네트워크 / 공용 네트워크 모두 허용**합니다 (다른 멤버와 UDP 로 직접 연결됩니다).

### 플러그인 (VST3)
- `MEON.vst3` 폴더를 `C:\Program Files\Common Files\VST3\` 에 복사합니다.
- DAW 에서 플러그인 재검색을 합니다. 로드가 안 되면 파일 속성에서 **차단 해제**(파일 우클릭 > 속성 > "차단 해제" 체크)를 확인하세요.

### Windows ASIO
합주에 필요한 지연(왕복 18ms 안팎)을 내려면 **ASIO 드라이버가 있는 오디오 인터페이스**가 필요합니다.
MEON 은 ASIO 장치만 사용합니다. 첫 실행 화면에서 "오디오 인터페이스가 필요합니다" 가 뜨면:

1. 인터페이스 제조사 사이트에서 최신 ASIO 드라이버를 설치합니다 (Focusrite, MOTU, RME, Steinberg, Behringer, PreSonus 등).
2. 인터페이스를 USB 에 꽂고 MEON 에서 **[장치 다시 검색]** 을 누릅니다.
3. 인터페이스가 없다면 임시로 [ASIO4ALL](https://asio4all.org/) 같은 범용 드라이버로 시험할 수 있지만, 지연이 커서 합주용으로 권장하지 않습니다.

## 공통

- **헤드폰**을 쓰세요. 스피커로 들으면 다른 사람 소리가 내 마이크로 들어가 하울링이 납니다.
- 가능하면 **유선 랜**을 쓰세요. Wi-Fi 는 지터가 커서 끊김이 늘어납니다.
- 문제가 생기면 설정 > 문제 해결 > **[로그 폴더 열기]** 의 JSON 파일을 보내주세요.
