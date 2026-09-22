#include "MeonSettings.h"

namespace meon
{

static juce::PropertiesFile::Options makeOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName     = "MEONUser";
    o.filenameSuffix      = ".settings";
    o.osxLibrarySubFolder = "Application Support/MEON";
#if JUCE_LINUX
    o.folderName          = "~/.config/meon";
#elif JUCE_WINDOWS
    o.folderName          = "MEON";
#else
    o.folderName          = "";
#endif
    o.storageFormat       = juce::PropertiesFile::storeAsXML;
    o.millisecondsBeforeSaving = 500;
    return o;
}

MeonSettings::MeonSettings()
{
    props = std::make_unique<juce::PropertiesFile> (makeOptions());
}

MeonSettings::~MeonSettings()
{
    save();
}

juce::String MeonSettings::getNickname() const
{
    return props->getValue ("nickname", "");
}

void MeonSettings::setNickname (const juce::String& name)
{
    props->setValue ("nickname", name.trim());
    save();
}

bool MeonSettings::isOnboardingDone (bool plugin) const
{
    return props->getBoolValue (plugin ? "onboardingDonePlugin" : "onboardingDoneApp", false);
}

void MeonSettings::setOnboardingDone (bool plugin, bool done)
{
    props->setValue (plugin ? "onboardingDonePlugin" : "onboardingDoneApp", done);
    save();
}

int MeonSettings::getInputChannelStart() const
{
    return props->getIntValue ("inputChannelStart", 0);
}

int MeonSettings::getInputChannelCount() const
{
    return juce::jlimit (1, 2, props->getIntValue ("inputChannelCount", 1));
}

void MeonSettings::setInputChannels (int start, int count)
{
    props->setValue ("inputChannelStart", juce::jmax (0, start));
    props->setValue ("inputChannelCount", juce::jlimit (1, 2, count));
    save();
}

bool MeonSettings::isChatOpen() const
{
    return props->getBoolValue ("chatOpen", true);
}

void MeonSettings::setChatOpen (bool open)
{
    props->setValue ("chatOpen", open);
}

void MeonSettings::save()
{
    props->saveIfNeeded();
}

juce::File MeonSettings::getSettingsFolder()
{
    return makeOptions().getDefaultFile().getParentDirectory();
}

juce::File MeonSettings::getLogFolder()
{
#if JUCE_MAC
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Logs/MEON");
#elif JUCE_WINDOWS
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("MEON").getChildFile ("logs");
#else
    return getSettingsFolder().getChildFile ("logs");
#endif
}

bool MeonSettings::isValidNickname (const juce::String& nameIn)
{
    auto name = nameIn.trim();
    const int len = name.length();
    if (len < 2 || len > 12)
        return false;

    for (int i = 0; i < len; ++i)
    {
        const juce::juce_wchar c = name[i];
        const bool hangul = (c >= 0xAC00 && c <= 0xD7A3) || (c >= 0x3131 && c <= 0x318E);
        const bool latin  = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool digit  = (c >= '0' && c <= '9');
        if (! (hangul || latin || digit || c == ' '))
            return false;
    }
    return true;
}

} // namespace meon
