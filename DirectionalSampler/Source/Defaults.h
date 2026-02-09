//This is where all the default values are stored.
//Changing these values requires re-building the package!

#pragma once

namespace Defaults
{
//Values Channel Layout
constexpr int dfMaxCh = 64;
constexpr float dfOrderSetting = 4;
constexpr float dfLoopMin = 80.0f;
constexpr int dfVoices = 8;

//Values Parameters
constexpr float dfMasterGain = -3.0f;
constexpr float dfMasterGainBot = -30.0f;
constexpr float dfMasterGainCeil = 0.0f;

constexpr float dfPitchRange = 600.0f;
constexpr float dfPitchRangeBot = 100.0f;
constexpr float dfPitchRangeCeil = 2400.0f;

constexpr float dfRepFMin = 80.0f;
constexpr float dfRepFMinBot = 60.0f;
constexpr float dfRepFMinCeil = 250.0f;

constexpr float dfRepFMax = 2.0f;
constexpr float dfRepFMaxBot = 1.0f;
constexpr float dfRepFMaxCeil = 5.0f;

constexpr float dfGainMin = -40.0f;
constexpr float dfGainBot = -60.0f; //general Usage
constexpr float dfGainMinCeil = -10.0f;

constexpr float dfLpfMin = 500.0f;
constexpr float dfLpfMinCeil = 10000.0f;

constexpr float dfGainOffset = 10.0f;

constexpr float dfDuckingAtten = -10.0f;
constexpr float dfDuckingAttenBot = -20.0f;

constexpr float dfGainActive = 1.0f;
constexpr float dfPitchActive = 1.0f;
constexpr float dfLpfActive = 0.0f;
constexpr float dfDuckigActive = 0.0f;
constexpr float dfSeamless = 0.0f;

constexpr int dfOSCPort = 8888;

//Samples
inline juce::String dfSample = "ACV_TestSound";
inline juce::String dfGuideSample = "ACV_Guide_01";
}
