// MEON - 독립 앱 자동 업데이트 (Windows / macOS)
// CI 가 main 빌드마다 windows-latest / mac-latest 프리릴리스에 새 빌드와 build.txt(빌드 번호)를 올린다.
// 앱은 build.txt 를 내 빌드 번호(MEON_BUILD_ID)와 비교하고, 새 빌드면 받아서 설치한다.
// - Windows: 설치 프로그램을 조용한 설치(/SILENT)로 실행한 뒤 종료. 설치가 끝나면 설치 프로그램이 앱을 다시 켠다.
// - macOS: MEON-mac.zip 을 받아 풀어 두고, 설치 때 MEON.app 을 바꿔 넣은 뒤 종료. 떨어진 프로세스(setsid)가 앱을 다시 켠다.
//   기록: ~/Library/Logs/MEON/update.log
#pragma once

#include "MeonTheme.h"

namespace meon
{

class MeonUpdater : public juce::ChangeBroadcaster,
                    private juce::Thread,
                    private juce::AsyncUpdater
{
public:
    enum class State
    {
        Idle,          // 아직 확인 안 함
        Checking,
        UpToDate,
        Available,     // 새 빌드가 있음
        Downloading,
        Ready,         // 받기 끝, 설치 대기 (합주 중이라 바로 설치하지 않은 경우)
        Failed
    };

    MeonUpdater();
    ~MeonUpdater() override;

    /** Windows / macOS 독립 앱이고 CI 빌드(빌드 번호 있음)일 때만 true. 아니면 화면에 업데이트 항목을 두지 않는다. */
    static bool isSupported();
    static int currentBuild();

    void check();
    /** Available 일 때 새 빌드 받기 시작. 끝나면 onDownloaded 를 메시지 스레드에서 부른다. */
    void download();
    /** Ready 일 때 설치를 시작한다 (Windows: 설치 프로그램 실행, macOS: 앱 교체 후 다시 켜기 예약). 성공하면 true (호출한 쪽이 앱을 종료한다). */
    bool launchInstaller();

    State getState() const;
    int getLatestBuild() const      { return latestBuild.load(); }
    float getProgress() const       { return progress.load(); }   // 0~1, 길이를 모르면 음수
    bool lastFailureWasDownload() const { return failedOnDownload.load(); }
    /** macOS: 앱 자리에 쓸 수 없어서(dmg 안에서 실행 등) 받지 못했다. 다시 시도해도 소용없다. */
    bool lastFailureWasPermission() const { return failedOnPermission.load(); }

    std::function<void()> onDownloaded;

private:
    enum class Job { None, Check, Download };
    mutable juce::CriticalSection lock;
    State state = State::Idle;
    Job job = Job::None;
    std::atomic<int> latestBuild { 0 };
    std::atomic<float> progress { 0.0f };
    std::atomic<bool> failedOnDownload { false };
    std::atomic<bool> failedOnPermission { false };
    juce::File installer;

    void setState (State s);
    void start (Job j, State s);
    void run() override;
    bool runCheck();
    bool runDownload();
    void handleAsyncUpdate() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeonUpdater)
};

} // namespace meon
