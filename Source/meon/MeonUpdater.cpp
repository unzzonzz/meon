#include "MeonUpdater.h"

#if JUCE_MAC
 #include <unistd.h>
#endif

#ifndef MEON_BUILD_ID
 #define MEON_BUILD_ID 0
#endif

namespace meon
{

namespace
{
   #if JUCE_MAC
    const char* const releaseBase = "https://github.com/unzzonzz/meon/releases/download/mac-latest/";
    const char* const packageName = "MEON-mac.zip";
   #else
    const char* const releaseBase = "https://github.com/unzzonzz/meon/releases/download/windows-latest/";
    const char* const packageName = "MEON-Installer.exe";
   #endif

    std::unique_ptr<juce::InputStream> open (const juce::String& name, int timeoutMs, int* status)
    {
        return juce::URL (juce::String (releaseBase) + name)
                   .createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                           .withConnectionTimeoutMs (timeoutMs)
                                           .withNumRedirectsToFollow (5)
                                           .withStatusCode (status));
    }

    juce::File downloadFolder()
    {
        return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MEON-Update");
    }

   #if JUCE_MAC
    // 지금 실행 중인 MEON.app. 이 자리를 새 앱으로 바꿔 넣는다.
    juce::File currentAppBundle()
    {
        return juce::File::getSpecialLocation (juce::File::currentApplicationFile);
    }

    bool runTool (const juce::StringArray& args, int timeoutMs)
    {
        juce::ChildProcess p;
        return p.start (args, 0) && p.waitForProcessToFinish (timeoutMs) && p.getExitCode() == 0;
    }

    // 앱이 완전히 꺼질 때까지 기다렸다가 MEON.app 을 새 것으로 바꾸고 다시 켠다.
    // 바꾸다 실패하면 원래 앱을 되돌려 놓고 그대로 켠다.
    const char* const swapScript = R"(
PID="$1"; NEW="$2"; APP="$3"
n=0
while kill -0 "$PID" 2>/dev/null; do
  n=$((n+1)); [ $n -gt 300 ] && exit 1
  sleep 0.2
done
OLD="$APP.old-$$"
if mv "$APP" "$OLD"; then
  if mv "$NEW" "$APP"; then rm -rf "$OLD"; else mv "$OLD" "$APP"; fi
fi
xattr -dr com.apple.quarantine "$APP"
open "$APP"
)";
   #endif
}

MeonUpdater::MeonUpdater() : juce::Thread ("MEON updater") {}

MeonUpdater::~MeonUpdater()
{
    cancelPendingUpdate();
    stopThread (5000);
}

bool MeonUpdater::isSupported()
{
   #if JUCE_WINDOWS
    return MEON_BUILD_ID > 0 && juce::JUCEApplicationBase::isStandaloneApp();
   #elif JUCE_MAC
    if (MEON_BUILD_ID <= 0 || ! juce::JUCEApplicationBase::isStandaloneApp())
        return false;
    // dmg 안에서 바로 켰거나 쓰기 권한이 없는 곳(다른 사용자 소유 등)이면 앱을 바꿔 넣을 수 없다
    const auto app = currentAppBundle();
    return app.getFileExtension() == ".app" && app.hasWriteAccess() && app.getParentDirectory().hasWriteAccess();
   #else
    return false;
   #endif
}

int MeonUpdater::currentBuild() { return MEON_BUILD_ID; }

MeonUpdater::State MeonUpdater::getState() const
{
    const juce::ScopedLock sl (lock);
    return state;
}

void MeonUpdater::setState (State s)
{
    {
        const juce::ScopedLock sl (lock);
        state = s;
    }
    sendChangeMessage();
}

void MeonUpdater::start (Job j, State s)
{
    if (! isSupported() || isThreadRunning())
        return;
    {
        const juce::ScopedLock sl (lock);
        job = j;
    }
    setState (s);
    startThread();
}

void MeonUpdater::check()
{
    const auto s = getState();
    if (s == State::Checking || s == State::Downloading || s == State::Ready)
        return;
    start (Job::Check, State::Checking);
}

void MeonUpdater::download()
{
    if (getState() != State::Available)
        return;
    progress = 0.0f;
    start (Job::Download, State::Downloading);
}

void MeonUpdater::run()
{
    Job j;
    {
        const juce::ScopedLock sl (lock);
        j = job;
    }
    failedOnDownload = (j == Job::Download);
    if (j == Job::Check)
    {
        if (! runCheck())
            setState (State::Failed);
    }
    else if (j == Job::Download)
    {
        if (runDownload())
        {
            setState (State::Ready);
            triggerAsyncUpdate();
        }
        else if (! threadShouldExit())
        {
            setState (State::Failed);
        }
    }
}

bool MeonUpdater::runCheck()
{
    int status = 0;
    auto in = open ("build.txt", 10000, &status);
    if (in == nullptr || status != 200)
        return false;
    const auto text = in->readEntireStreamAsString().trim();
    if (! text.containsOnly ("0123456789") || text.isEmpty())
        return false;
    latestBuild = text.getIntValue();
    setState (latestBuild.load() > currentBuild() ? State::Available : State::UpToDate);
    return true;
}

bool MeonUpdater::runDownload()
{
    auto folder = downloadFolder();
    folder.deleteRecursively();   // 지난번에 받은 설치 프로그램 정리
    if (! folder.createDirectory())
        return false;
   #if JUCE_MAC
    auto target = folder.getChildFile ("MEON-mac-" + juce::String (latestBuild.load()) + ".zip");
   #else
    auto target = folder.getChildFile ("MEON-Installer-" + juce::String (latestBuild.load()) + ".exe");
   #endif

    int status = 0;
    auto in = open (packageName, 15000, &status);
    if (in == nullptr || status != 200)
        return false;

    const auto total = in->getTotalLength();
    juce::int64 done = 0;
    {
        juce::FileOutputStream out (target);
        if (! out.openedOk())
            return false;
        juce::HeapBlock<char> buffer (64 * 1024);
        while (! in->isExhausted())
        {
            if (threadShouldExit())
                return false;
            const int n = in->read (buffer, 64 * 1024);
            if (n < 0)
                return false;
            if (n == 0)
                break;
            if (! out.write (buffer, (size_t) n))
                return false;
            done += n;
            const float p = total > 0 ? (float) ((double) done / (double) total) : -1.0f;
            if (std::abs (p - progress.load()) >= 0.01f || p < 0.0f)
            {
                progress = p;
                sendChangeMessage();
            }
        }
        out.flush();
    }

    // 잘린 파일이나 오류 페이지를 실행하지 않도록: 길이와 파일 머리(Windows MZ, macOS zip 의 PK)를 확인한다
    if ((total > 0 && done != total) || done < 1024 * 1024)
        return false;
    {
        juce::FileInputStream check (target);
       #if JUCE_MAC
        if (! check.openedOk() || check.readByte() != 'P' || check.readByte() != 'K')
            return false;
       #else
        if (! check.openedOk() || check.readByte() != 'M' || check.readByte() != 'Z')
            return false;
       #endif
    }

   #if JUCE_MAC
    // zip 을 풀고 (ditto 는 서명·심볼릭 링크를 그대로 살린다) 서명이 온전한지 확인한다
    auto unpacked = folder.getChildFile ("unpacked");
    if (threadShouldExit()
        || ! runTool ({ "/usr/bin/ditto", "-x", "-k", target.getFullPathName(), unpacked.getFullPathName() }, 120000))
        return false;
    auto app = unpacked.getChildFile ("MEON.app");
    if (! app.getChildFile ("Contents/MacOS/MEON").existsAsFile()
        || ! runTool ({ "/usr/bin/codesign", "--verify", "--deep", app.getFullPathName() }, 60000))
        return false;
    target.deleteFile();
    target = app;
   #endif

    const juce::ScopedLock sl (lock);
    installer = target;
    return true;
}

void MeonUpdater::handleAsyncUpdate()
{
    if (onDownloaded)
        onDownloaded();
}

bool MeonUpdater::launchInstaller()
{
    juce::File file;
    {
        const juce::ScopedLock sl (lock);
        if (state != State::Ready)
            return false;
        file = installer;
    }
   #if JUCE_MAC
    // 앱이 꺼진 뒤 셸 스크립트가 MEON.app 을 바꿔 넣고 다시 켠다 (ChildProcess 는 앱이 꺼져도 계속 돈다)
    auto script = downloadFolder().getChildFile ("swap.sh");
    if (script.replaceWithText (swapScript)
        && juce::ChildProcess().start ({ "/bin/sh", script.getFullPathName(),
                                         juce::String ((int) getpid()), file.getFullPathName(),
                                         currentAppBundle().getFullPathName() }, 0))
        return true;
    setState (State::Ready);
    return false;
   #else
    // /SILENT: 묻는 것 없이 진행 막대만 보여 준다. /UPDATE=1: 설치가 끝나면 앱을 다시 켠다 (wininstaller.iss).
    // ShellExecute 라서 관리자 권한 확인 창을 거친다. 거기서 취소하면 false.
    if (juce::Process::openDocument (file.getFullPathName(), "/SILENT /SUPPRESSMSGBOXES /NORESTART /UPDATE=1"))
        return true;
    setState (State::Ready);   // 다시 누를 수 있게 그대로 둔다
    return false;
   #endif
}

} // namespace meon
