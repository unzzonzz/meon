#include "MeonUpdater.h"

#ifndef MEON_BUILD_ID
 #define MEON_BUILD_ID 0
#endif

namespace meon
{

namespace
{
    const char* const releaseBase = "https://github.com/unzzonzz/meon/releases/download/windows-latest/";

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
    auto target = folder.getChildFile ("MEON-Installer-" + juce::String (latestBuild.load()) + ".exe");

    int status = 0;
    auto in = open ("MEON-Installer.exe", 15000, &status);
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

    // 잘린 파일이나 오류 페이지를 실행하지 않도록: 길이와 실행 파일 머리(MZ)를 확인한다
    if ((total > 0 && done != total) || done < 1024 * 1024)
        return false;
    juce::FileInputStream check (target);
    if (! check.openedOk() || check.readByte() != 'M' || check.readByte() != 'Z')
        return false;

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
    // /SILENT: 묻는 것 없이 진행 막대만 보여 준다. /UPDATE=1: 설치가 끝나면 앱을 다시 켠다 (wininstaller.iss).
    // ShellExecute 라서 관리자 권한 확인 창을 거친다. 거기서 취소하면 false.
    if (juce::Process::openDocument (file.getFullPathName(), "/SILENT /SUPPRESSMSGBOXES /NORESTART /UPDATE=1"))
        return true;
    setState (State::Ready);   // 다시 누를 수 있게 그대로 둔다
    return false;
}

} // namespace meon
