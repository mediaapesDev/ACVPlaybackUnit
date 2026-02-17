/*
 ==============================================================================
 This file is part of the software package of the playback unit developed
 in the "Acoustic Vision" ZiM Project.
 //
 Author of the Original Source Code: Daniel Rudrich
 Editor and Author: Alexander Leo
 //
 Original Source Code
 Copyright (c) 2017 - Institute of Electronic Music and Acoustics (IEM)
 https://iem.at
 
 Modified Code with Extended Functionality
 Copyright (c) 2026 - MediaApes GmbH
 //
 For any licensing detais of the original code, please refer to the
 data provided by the Institute of Electronic Music and Acoustics (IEM).
 
 This modified code is the IP of Alexander Leo & the MediaApes GmbH and
 may not be multiplied, distributed or modified without the permission
 of the according right holders.
 ==============================================================================
 */

#pragma once

#include "../../resources/AudioProcessorBase.h"
#include "../../resources/Conversions.h"
#include "../../resources/Quaternion.h"
#include "../../resources/ambisonicTools.h"
#include "../../resources/efficientSHvanilla.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Defaults.h"

#define CONFIGURATIONHELPER_ENABLE_LOUDSPEAKERLAYOUT_METHODS 1
#include "../../resources/ConfigurationHelper.h"

#define ProcessorClass DirectionalSamplerAudioProcessor

constexpr int maxNumberOfInputs = Defaults::dfMaxCh;
constexpr int startNnumberOfInputs = Defaults::dfVoices; // 0 orSets Auto Active

//==============================================================================
class DirectionalSamplerAudioProcessor
    : public AudioProcessorBase<IOTypes::AudioChannels<maxNumberOfInputs>, IOTypes::Ambisonics<>>
{
public:
    constexpr static int numberOfInputChannels = Defaults::dfMaxCh;
    constexpr static int numberOfOutputChannels = Defaults::dfMaxCh;
    //==============================================================================
    DirectionalSamplerAudioProcessor();
    ~DirectionalSamplerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //======= Parameters ===========================================================
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> createParameterLayout();

    //==============================================================================
    juce::Result loadConfiguration (const juce::File& configFile);
    juce::Result loadConfigFromString (juce::String configString);
    void loadSample(const juce::String& resourceName);
    void loadSampleManually();
    void setLastDir (juce::File newLastDir);
    juce::File getLastDir() { return lastDir; };
    

    bool yprInput;
    bool updateColours = false;
    bool updateSphere = true;
    bool soloMuteChanged = true;
    bool manualSample = false;
    
    int samplerSize = 1;
    
    float loopMin = Defaults::dfLoopMin;
    float timeConstant;
    
    double phi, theta;
    double sampleFileSampleRate = 48000.0; // will be overwritten
    
    constexpr static double kMaxLoopLengthMs = 5000.0;
    
    float xyzGrab[3];
    float xyz[maxNumberOfInputs][3];

    std::atomic<float>* azimuth[maxNumberOfInputs];
    std::atomic<float>* elevation[maxNumberOfInputs];
    std::atomic<float>* gain[maxNumberOfInputs];
    std::atomic<float>* nGain[maxNumberOfInputs];
    std::atomic<float>* mute[maxNumberOfInputs];
    std::atomic<float>* solo[maxNumberOfInputs];
    std::atomic<float>* pitch[maxNumberOfInputs];
    std::atomic<float>* repF[maxNumberOfInputs];
    std::atomic<float>* lpf[maxNumberOfInputs];

    juce::BigInteger muteMask, soloMask;

    std::atomic<float>* inputSetting;
    std::atomic<float>* orderSetting;
    std::atomic<float>* useSN3D;
    
    std::atomic<float>* masterAzimuth;
    std::atomic<float>* masterElevation;
    std::atomic<float>* masterRoll;
    std::atomic<float>* masterGain;
    std::atomic<float>* lockedToMaster;

    std::atomic<float>* gainMin;
    std::atomic<float>* pitchRange;
    std::atomic<float>* repFMin;
    std::atomic<float>* repFMax;
    std::atomic<float>* lpfRange;
    
    std::atomic<float>* play;
    std::atomic<float>* seamless;
    std::atomic<float>* guideActive;
    
    std::atomic<float>* gainActive;
    std::atomic<float>* pitchActive;
    std::atomic<float>* lpfActive;
    
    std::atomic<float>* gainOffset;
    std::atomic<float>* duckingAtten;
    std::atomic<float>* duckingActive;
    
    std::atomic<float>* analyzeRMS;
    std::atomic<float>* dynamicRange;

    std::vector<float> rms;
    
    juce::Colour elementColours[maxNumberOfInputs];

    //helper functions
    void updateBuffers() override;
    void updateQuaternions();
    void updateVoices();
    void playbackControl(bool p);
    void limitBuffer (juce::AudioBuffer<float>& buff, float ceilingDb);
    void ducking(juce::AudioBuffer<float>& bufferToDuck, juce::AudioBuffer<float>& sidechainBuffer, float attenuation);
    void gainUpdate();
    void cycleUpdate();
    void symmetry(int chA, int chB);
    void bufferCleaner(juce::AudioBuffer<float>& bufferToClean, float state, juce::AudioBuffer<float>& referenceBuffer);
    
    //CB Selectors
    static const juce::StringArray headphoneEQs;
    static const juce::StringArray sampleLib;
    
private:
    //==============================================================================
    void wrapSphericalCoordinates (float* azi, float* ele);
    
    juce::SmoothedValue<float> normGain;
    juce::SmoothedValue<float> gainToDuck;
    
    juce::ADSR::Parameters adsrParams;
    std::atomic<bool> adsrDirty { true };
        
    juce::Synthesiser mSampler;
    
    juce::AudioFormatManager mFormatManager;
    juce::AudioFormatReader* mFormatReader {nullptr};
    
    juce::File lastDir;
    std::unique_ptr<juce::PropertiesFile> properties;

    bool dontTriggerMasterUpdate = false;
    bool locked = false;
    bool moving = false;
    bool guideChanged = false;
    bool mSamplerSoundsLoaded = false;
    bool processorUpdatingParams, wasLockedBefore;
    
    int fftLength = -1;
    int irLength = 236;
    int irLengthMinusOne = irLength - 1;
    int _handleCh = 0;
    
    int nMidCh, nSideCh;
    int tempChannelCount, tempSampleID, guideSampleID;

    float binBuffCeil, feedBuffCeil, ambiOver;
    
    float masterYpr[3];
    float dist[maxNumberOfInputs];
    float SH[maxNumberOfInputs][Defaults::dfMaxCh];
    float _SH[maxNumberOfInputs][Defaults::dfMaxCh];
    float _gain[maxNumberOfInputs];
    
    iem::Quaternion<float> quats[maxNumberOfInputs];

    //CB Selector
    std::atomic<float>* applyHeadphoneEq;
    std::atomic<float>* pickSample;

    juce::dsp::Convolution EQ;

    std::vector<std::complex<float>> fftBuffer;
    std::vector<std::complex<float>>  accumMid;
    std::vector<std::complex<float>>  accumSide;

    std::unique_ptr<juce::dsp::FFT> fft;

    juce::AudioBuffer<float> sampleBuffer,
        bufferClone,
        binBuffer,
        overlapBuffer,
        ambiBuffer;
    
    juce::AudioBuffer<float> irs[7];
    juce::AudioBuffer<float> irsFrequencyDomain;
    
    double irsSampleRate = 44100.0;
    
    //mapping between mid-channel index and channel index
    const int mix2cix[36] = { 0,  2,  3,  6,  7,  8,  12, 13, 14, 15, 20, 21,
                              22, 23, 24, 30, 31, 32, 33, 34, 35, 42, 43, 44,
                              45, 46, 47, 48, 56, 57, 58, 59, 60, 61, 62, 63 };
    //mapping between side-channel index and channel index
    const int six2cix[28] = { 1,  4,  5,  9,  10, 11, 16, 17, 18, 19, 25, 26, 27, 28,
                              29, 36, 37, 38, 39, 40, 41, 49, 50, 51, 52, 53, 54, 55 };
    
    
    //juce::ValueTree lastState { juce::Identifier { "LastState" } };
    std::vector<juce::String> savedParams;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectionalSamplerAudioProcessor)
};

//=============

class LoopingSound : public juce::SynthesiserSound
{
public:
    LoopingSound(juce::AudioBuffer<float> bufferToOwn,
                 double sampleRate,
                 int rootNote)
        : buffer(std::move(bufferToOwn)),
          sourceSampleRate(sampleRate),
          midiRootNote(rootNote)
    {}

    bool appliesToNote(int) override      { return true; }
    bool appliesToChannel(int) override   { return true; }

    const juce::AudioBuffer<float>& getBuffer() const { return buffer; }
    int getLength() const { return buffer.getNumSamples(); }
    double getSampleRate() const { return sourceSampleRate; }
    int getRootNote() const { return midiRootNote; }

private:
    juce::AudioBuffer<float> buffer;
    
    double sourceSampleRate;
    int midiRootNote;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopingSound)
};


class LoopingSamplerVoice : public juce::SynthesiserVoice
{
public:
    LoopingSamplerVoice(int channelId)
    : outputCh (channelId)
    {
        // default ADSR
        juce::ADSR::Parameters p;
        p.attack  = 0.001f;
        p.decay   = 0.0f;
        p.sustain = 1.0f;
        p.release = 0.02f;
        adsr.setParameters(p);
        
        sampleRate = 48000.0;
        loopCounter = 0.0;
        loopLengthSamples = 48000.0;
        sourcePos = 0.0;
        currentPitchRatio = 1.0;
        lpCutoffHz = 20000.0f;
        
        smoothedCutoff.reset(sampleRate, 0.02);
        smoothedCutoff.setCurrentAndTargetValue(lpCutoffHz);
        smoothedPitchRatio.reset(sampleRate, 0.02);
        smoothedPitchRatio.setCurrentAndTargetValue(currentPitchRatio);
    }

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<LoopingSound*>(s) != nullptr;
    }

    void startNote(int /*midiNote*/, float velocity,
                   juce::SynthesiserSound* s,
                   int /*pitchWheel*/) override
    {
        auto* sound = static_cast<LoopingSound*>(s);
        finalCycle = false;

        buffer = &sound->getBuffer();
        
        if (fixedLength)
                loopLengthSamples = buffer->getNumSamples();

        level = velocity;
        sourcePos = 0.0;
        loopCounter = loopLengthSamples;
        
        lowpass.reset();
    }


    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
            clearCurrentNote();
    }

    void renderNextBlock(juce::AudioBuffer<float>& output,
                         int startSample,
                         int numSamples) override
    {
        if (!buffer)
        {
            return;
        }

        const int numChannels = buffer->getNumChannels();
        double releaseSamples = adsr.getParameters().release * sampleRate;
        releaseSamples = juce::jmin(releaseSamples, loopLengthSamples * 0.99);
        double loopEnd = loopLengthSamples;
        double releaseStart = loopEnd - releaseSamples;
        while (--numSamples >= 0)
        {
            // Advance loop phase
            loopCounter += 1.0;

            // Compute release start (in samples)
            const double releaseStart = loopLengthSamples - releaseSamples;

            // Trigger release once per loop
            if (!releaseTriggered && loopCounter >= releaseStart)
            {
                adsr.noteOff();
                releaseTriggered = true;
            }

            // Handle loop wrap
            if (loopCounter >= loopLengthSamples && loopLengthSamples > 1.0)
            {
                if (!finalCycle)
                {
                    loopCounter -= loopLengthSamples;
                    sourcePos = 0.0;
                    
                    adsr.reset();
                    adsr.noteOn();
                    releaseTriggered = false;
                }
                else
                {
                    buffer = nullptr;
                    return;
                }
            }

            // Advance envelope
            const float env = adsr.getNextSample();
            float sampleValue = 0.0f;

            // Read sample (linear interpolation)
            if (sourcePos < buffer->getNumSamples())
            {
                const int pos = (int) sourcePos;
                const int nextPos = juce::jmin (pos + 1,
                                                buffer->getNumSamples() - 1);
                const float frac = (float) (sourcePos - pos);

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float s1 = buffer->getSample (ch, pos);
                    const float s2 = buffer->getSample (ch, nextPos);
                    sampleValue += s1 * (1.0f - frac) + s2 * frac;
                }

                sourcePos += smoothedPitchRatio.getNextValue();;
            }
            
            // Apply envelope + velocity
            sampleValue *= level * env;

            float currentCutoff = smoothedCutoff.getNextValue();
            lowpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, currentCutoff);
            sampleValue = lowpass.processSample(sampleValue);
            
            //make sure shit doesn't blow up
            jassert (std::isfinite(sampleValue));

            // Write to output
            output.addSample (outputCh, startSample, sampleValue);
            
            ++startSample;
        }


        if (!adsr.isActive())
            clearCurrentNote();
    }
    
    // Set loop length in milliseconds
    void setLoopLengthMs(double ms)
    {
        if (fixedLength)
            return;
        
        double newLength = (ms/1000) * sampleRate;

        if (newLength > 1.0 && loopLengthSamples > 1.0)
        {
            double phase = loopCounter / loopLengthSamples;
            loopLengthSamples = newLength;
            loopCounter = phase * loopLengthSamples;
        }
    }

    void setSampleRate(double newSampleRate)
    {
        jassert (newSampleRate > 0.0);
        
        sampleRate = newSampleRate;
        loopLengthSamples = sampleRate;
        adsr.setSampleRate(newSampleRate);
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;// safe value
        spec.numChannels = 1;          // IMPORTANT (see below)

        lowpass.prepare (spec);
        lowpass.reset();
        
        smoothedCutoff.reset(sampleRate, 0.02); // 20 ms smoothing
        smoothedCutoff.setCurrentAndTargetValue(lpCutoffHz);
    }

    // Pitch in cents
    void updatePitch(double pitchInCents)
    {
        double targetRatio = std::pow(2.0, pitchInCents / 1200.0);
        smoothedPitchRatio.setTargetValue(targetRatio);
    }

    void setADSR(const juce::ADSR::Parameters& p)
    {
        adsr.setParameters(p);
    }
    
    void stopLoop(bool p)
    {
        finalCycle = p;
    }
    
    void setFixedLength (bool fixed)
    {
        fixedLength = fixed;

        if (fixedLength && buffer != nullptr)
            loopLengthSamples = buffer->getNumSamples();
    }
    
    void updateLowpass()
    {
        jassert (sampleRate > 0.0);

        const float cutoff = juce::jlimit (20.0f, 20000.0f, lpCutoffHz);
        lowpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoff);
    }

    
    void setLowpassCutoff (float freqHz)
    {
        jassert (sampleRate > 0.0);
        
        lpCutoffHz = freqHz;
        smoothedCutoff.setTargetValue(lpCutoffHz);
    }
    
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    

private:
    juce::ADSR adsr;
    const juce::AudioBuffer<float>* buffer { nullptr };

    juce::dsp::IIR::Filter<float> lowpass;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCutoff;
    float lpCutoffHz = 20000.0f;

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> smoothedPitchRatio;
    
    bool releaseTriggered = false;
    bool finalCycle, fixedLength;
    
    int outputCh = 0;
    
    float level;
    
    double sampleRate,
        loopCounter,
        loopLengthSamples,
        sourcePos,
        currentPitchRatio;
};
