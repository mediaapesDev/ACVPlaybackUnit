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

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <Presets.h>
#include <EQData.h>
#include <IRData.h>
#include <SamplingData.h>

using namespace Defaults;

const juce::StringArray DirectionalSamplerAudioProcessor::headphoneEQs =
    juce::StringArray ("AKG-K141MK2",
                       "AKG-K240DF",
                       "AKG-K240MK2",
                       "AKG-K271MK2",
                       "AKG-K271STUDIO",
                       "AKG-K601",
                       "AKG-K701",
                       "AKG-K702",
                       "AKG-K1000-Closed",
                       "AKG-K1000-Open",
                       "AudioTechnica-ATH-M50",
                       "Beyerdynamic-DT250",
                       "Beyerdynamic-DT770PRO-250Ohms",
                       "Beyerdynamic-DT880",
                       "Beyerdynamic-DT990PRO",
                       "Presonus-HD7",
                       "Sennheiser-HD430",
                       "Sennheiser-HD480",
                       "Sennheiser-HD560ovationII",
                       "Sennheiser-HD565ovation",
                       "Sennheiser-HD600",
                       "Sennheiser-HD650",
                       "SHURE-SRH940");

const juce::StringArray DirectionalSamplerAudioProcessor::sampleLib =
    juce::StringArray ("ACV_TestSound",
                       "ACV_Guide_01",
                       "L_ACV_Noise_01",
                       "L_ACV_Noise_02",
                       "L_ACV_Noise_03",
                       "L_ACV_Tonal_01",
                       "L_ACV_Tonal_02",
                       "L_ACV_Tonal_03",
                       "L_ACV_Tonal_04",
                       "L_ACV_Tonal_05",
                       "O_ACV_Pluck_01",
                       "O_ACV_Pluck_02",
                       "O_ACV_Pluck_03",
                       "O_ACV_Pluck_04");

//==============================================================================
DirectionalSamplerAudioProcessor::DirectionalSamplerAudioProcessor() :
    AudioProcessorBase (
#ifndef JucePlugin_PreferredChannelConfigurations
        BusesProperties()
    #if ! JucePlugin_IsMidiEffect
        #if ! JucePlugin_IsSynth
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        #endif
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
    #endif
            ,
#endif
        createParameterLayout()),
    rms (dfMaxCh)
{
    // global properties
    juce::PropertiesFile::Options options;
    options.applicationName = "DirectionalSampler";
    options.filenameSuffix = "settings";
    options.folderName = "ACV";
    options.osxLibrarySubFolder = "Preferences";

    properties.reset (new juce::PropertiesFile (options));
    lastDir = juce::File (properties->getValue ("presetFolder"));
    
    //File Loader
    mFormatManager.registerBasicFormats();
    
    //Sampler
    for (int i=0; i<dfVoices; i++)
        mSampler.addVoice(new LoopingSamplerVoice(i));
    
    samplerSize = mSampler.getNumVoices();
    
    adsrParams.attack  = 0.001f;
    adsrParams.decay   = 0.0f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = 0.02f;
    adsrDirty.store(true);

    //Parameters
    parameters.addParameterListener ("inputSetting", this);
    parameters.addParameterListener ("orderSetting", this);
    
    parameters.addParameterListener ("masterAzimuth", this);
    parameters.addParameterListener ("masterElevation", this);
    parameters.addParameterListener ("masterRoll", this);
    parameters.addParameterListener ("masterGain", this);
    parameters.addParameterListener ("lockedToMaster", this);
    
    parameters.addParameterListener("gainMin", this);
    parameters.addParameterListener("pitchRange", this);
    parameters.addParameterListener("repFMin", this);
    parameters.addParameterListener("repFMax", this);
    parameters.addParameterListener("lpfMin", this);
    
    parameters.addParameterListener("play", this);
    parameters.addParameterListener("seamless", this);
    parameters.addParameterListener("guideActive", this);
    
    parameters.addParameterListener("gainActive", this);
    parameters.addParameterListener("pitchActive", this);
    parameters.addParameterListener("lpfActive", this);
    
    parameters.addParameterListener("gainOffset", this);
    parameters.addParameterListener("duckingAtten", this);
    parameters.addParameterListener("duckingActive", this);
    
    parameters.addParameterListener ("applyHeadphoneEq", this);
    parameters.addParameterListener ("pickSample", this);

    muteMask.clear();
    soloMask.clear();

    for (int i = 0; i < maxNumberOfInputs; ++i)
    {
        pitch[i] = parameters.getRawParameterValue("pitch" + juce::String (i));
        repF[i] = parameters.getRawParameterValue("repF" + juce::String (i));
        lpf[i] = parameters.getRawParameterValue("lpf" + juce::String (i));
        azimuth[i] = parameters.getRawParameterValue ("azimuth" + juce::String (i));
        elevation[i] = parameters.getRawParameterValue ("elevation" + juce::String (i));
        gain[i] = parameters.getRawParameterValue ("gain" + juce::String (i));
        nGain[i] = parameters.getRawParameterValue ("nGain" + juce::String (i));
        mute[i] = parameters.getRawParameterValue ("mute" + juce::String (i));
        solo[i] = parameters.getRawParameterValue ("solo" + juce::String (i));

        if (*mute[i] >= 0.5f)
            muteMask.setBit (i);
        if (*solo[i] >= 0.5f)
            soloMask.setBit (i);

        parameters.addParameterListener("pitch" + juce::String (i), this);
        parameters.addParameterListener("repF" + juce::String (i), this);
        parameters.addParameterListener("lpf" + juce::String (i), this);
        parameters.addParameterListener ("azimuth" + juce::String (i), this);
        parameters.addParameterListener ("elevation" + juce::String (i), this);
        parameters.addParameterListener ("gain" + juce::String (i), this);
        parameters.addParameterListener ("nGain" + juce::String (i), this);
        parameters.addParameterListener ("mute" + juce::String (i), this);
        parameters.addParameterListener ("solo" + juce::String (i), this);
    }
    
    inputSetting = parameters.getRawParameterValue ("inputSetting");
    orderSetting = parameters.getRawParameterValue ("orderSetting");
    useSN3D = parameters.getRawParameterValue ("useSN3D");
    
    masterAzimuth = parameters.getRawParameterValue ("masterAzimuth");
    masterElevation = parameters.getRawParameterValue ("masterElevation");
    masterRoll = parameters.getRawParameterValue ("masterRoll");
    masterGain = parameters.getRawParameterValue ("masterGain");
    lockedToMaster = parameters.getRawParameterValue ("locking");

    gainMin = parameters.getRawParameterValue("gainMin");
    pitchRange = parameters.getRawParameterValue("pitchRange");
    repFMin = parameters.getRawParameterValue("repFMin");
    repFMax = parameters.getRawParameterValue("repFMax");
    lpfRange = parameters.getRawParameterValue("lpfMin");
    
    play = parameters.getRawParameterValue("play");
    seamless = parameters.getRawParameterValue("seamless");
    guideActive = parameters.getRawParameterValue("guideActive");
    
    gainActive = parameters.getRawParameterValue("gainActive");
    pitchActive = parameters.getRawParameterValue("pitchActive");
    lpfActive = parameters.getRawParameterValue("lpfActive");
    
    gainOffset = parameters.getRawParameterValue("gainOffset");
    duckingAtten = parameters.getRawParameterValue("duckingAtten");
    duckingActive = parameters.getRawParameterValue("duckingActive");
    
    analyzeRMS = parameters.getRawParameterValue ("analyzeRMS");
    dynamicRange = parameters.getRawParameterValue ("dynamicRange");
    
    applyHeadphoneEq = parameters.getRawParameterValue ("applyHeadphoneEq");
    pickSample = parameters.getRawParameterValue ("pickSample");
    
    processorUpdatingParams = false;

    yprInput = true; //input from ypr

    for (int i = 0; i < maxNumberOfInputs; ++i)
    {
        juce::FloatVectorOperations::clear (SH[i], 64);
        _gain[i] = 0.0f;
        //elemActive[i] = *gain[i] >= -59.9f;
        elementColours[i] = juce::Colours::cyan;
    }

    updateQuaternions();
    
    // load IRs
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    juce::WavAudioFormat wavFormat;

    juce::MemoryInputStream* mis[7];
    mis[0] = new juce::MemoryInputStream (IRData::irsOrd1_wav, IRData::irsOrd1_wavSize, false);
    mis[1] = new juce::MemoryInputStream (IRData::irsOrd2_wav, IRData::irsOrd2_wavSize, false);
    mis[2] = new juce::MemoryInputStream (IRData::irsOrd3_wav, IRData::irsOrd3_wavSize, false);
    mis[3] = new juce::MemoryInputStream (IRData::irsOrd4_wav, IRData::irsOrd4_wavSize, false);
    mis[4] = new juce::MemoryInputStream (IRData::irsOrd5_wav, IRData::irsOrd5_wavSize, false);
    mis[5] = new juce::MemoryInputStream (IRData::irsOrd6_wav, IRData::irsOrd6_wavSize, false);
    mis[6] = new juce::MemoryInputStream (IRData::irsOrd7_wav, IRData::irsOrd7_wavSize, false);

    for (int i = 0; i < 7; ++i)
    {
        irs[i].setSize (juce::square (i + 2), irLength);
        std::unique_ptr<juce::AudioFormatReader> reader (wavFormat.createReaderFor (mis[i], true));
        reader->read (&irs[i], 0, irLength, 0, true, false);
        irs[i].applyGain (0.3f);
    }
    
    std::fill (rms.begin(), rms.end(), 0.0f);
    
    //+1 cause param ref
    guideSampleID = sampleLib.indexOf(dfGuideSample, false) + 1;
    
    //setup osc config
    oscParameterInterface.getOSCReceiver().connect(dfOSCPort);
}

DirectionalSamplerAudioProcessor::~DirectionalSamplerAudioProcessor()
{
    mFormatReader = nullptr;
}

int DirectionalSamplerAudioProcessor::getNumPrograms()
{
    return 6;
}

int DirectionalSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DirectionalSamplerAudioProcessor::setCurrentProgram (int index)
{
    juce::String preset;
    switch (index)
    {
        case 0:
            return;
        case 1:
            preset = juce::String (Presets::t_design_12ch_json, Presets::t_design_12ch_jsonSize);
            break;
        case 2:
            preset = juce::String (Presets::t_design_24ch_json, Presets::t_design_24ch_jsonSize);
            break;
        case 3:
            preset = juce::String (Presets::t_design_36ch_json, Presets::t_design_36ch_jsonSize);
            break;
        case 4:
            preset = juce::String (Presets::t_design_48ch_json, Presets::t_design_48ch_jsonSize);
            break;
        case 5:
            preset = juce::String (Presets::t_design_60ch_json, Presets::t_design_60ch_jsonSize);
            break;

        default:
            preset = "";
            break;
    }
    loadConfigFromString (preset);
}

const juce::String DirectionalSamplerAudioProcessor::getProgramName (int index)
{
    switch (index)
    {
        case 0:
            return "---";
        case 1:
            return "t-design (12 channels)";
        case 2:
            return "t-design (24 channels)";
        case 3:
            return "t-design (36 channels)";
        case 4:
            return "t-design (48 channels)";
        case 5:
            return "t-design (60 channels)";

        default:
            return {};
    }
}

void DirectionalSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DirectionalSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //Setup
    checkInputAndOutput (this, input.getSize(), output.getSize(), true);
    
    const int bufferCh = input.getSize();
    const int sampleCh = *inputSetting;
    const int ambiCh = juce::square((float)*orderSetting);
    const int ambiOrder = *orderSetting-1;
    const int handleCh = juce::jmax(sampleCh, ambiCh);
    symmetry(sampleCh, ambiCh);
    
    //Prevent Auto
    jassert(ambiOrder > 0);
    jassert(sampleCh > 0);
    
    //Value Setup
    timeConstant = exp (-1.0 / (sampleRate * 0.1 / samplesPerBlock)); // 100ms RMS averaging
    std::fill (rms.begin(), rms.end(), 0.0f);

    normGain.reset (sampleRate, 0.05);
    normGain.setCurrentAndTargetValue (1.0f);

    //Binaural Decoder
    juce::dsp::ProcessSpec convSpec;
    convSpec.sampleRate = sampleRate;
    convSpec.maximumBlockSize = samplesPerBlock;
    convSpec.numChannels =
        2; // convolve two channels (which actually point two one and the same input channel)
    
    //Sampler Setup
    updateVoices();
    if (!mSamplerSoundsLoaded)
            {
                auto* param = parameters.getParameter("pickSample");
                if (param->getValue() <= 0.5f)
                {
                    param->setValueNotifyingHost(juce::jmap(1.0f, 0.f, (float)sampleLib.size(), 0.f, 1.f));
                }
                    
                mSamplerSoundsLoaded = true;
            }

    if (mSamplerSoundsLoaded)
        mSampler.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < mSampler.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
            v->setSampleRate(sampleRate);
    
    //Wrapping Up
    EQ.prepare (convSpec);
    gainUpdate();
    cycleUpdate();
}

void DirectionalSamplerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void DirectionalSamplerAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    //Setup
    juce::ScopedNoDenormals noDenormals;
    checkInputAndOutput (this, input.getSize(), output.getSize(), false);

    const int bufferCh = input.getSize();
    const int sampleCh = *inputSetting;
    const int ambiCh = juce::square((float)*orderSetting);
    const int ambiOrder = *orderSetting-1;
    const int handleCh = juce::jmax(sampleCh, ambiCh);
    
    if (adsrDirty.exchange(false))
    {
        for (int i = 0; i < mSampler.getNumVoices(); ++i)
            if (auto* voice = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                voice->setADSR(adsrParams);
    }
    
    if (*analyzeRMS > 0.5f && ! isNonRealtime())
    {
        const float oneMinusTimeConstant = 1.0f - timeConstant;
        for (int ch = 0; ch < bufferCh; ++ch)
            rms[ch] = timeConstant * rms[ch]
                      + oneMinusTimeConstant * buffer.getRMSLevel (ch, 0, buffer.getNumSamples());
    }
    
    //Input Signal Clone
    bufferClone.clear();
    bufferClone.makeCopyOf(buffer, true);
    
    //prepare Buffers
    sampleBuffer.clear();
    buffer.clear();
    binBuffer.clear();
    ambiBuffer.clear();
        
    //---------------------------------------------------------------------------
    //SAMPLER - Audio Genertation
    mSampler.renderNextBlock(sampleBuffer, midiMessages, 0, sampleBuffer.getNumSamples());
    bufferCleaner(sampleBuffer, ambiOver, ambiBuffer);
    
    //---------------------------------------------------------------------------
    //Ambisonics Encoding
    for (int i = 0; i < handleCh; ++i)
    {
        juce::FloatVectorOperations::copy (_SH[i], SH[i], handleCh);

        float currGain = 0.0f;

        if (! soloMask.isZero())
        {
            if (soloMask[i])
                currGain = juce::Decibels::decibelsToGain (gain[i]->load());
        }
        else
        {
            if (! muteMask[i])
                currGain = juce::Decibels::decibelsToGain (gain[i]->load());
        }

        const float azimuthInRad = juce::degreesToRadians (azimuth[i]->load());
        const float elevationInRad = juce::degreesToRadians (elevation[i]->load());

        const juce::Vector3D<float> pos {
            Conversions<float>::sphericalToCartesian (azimuthInRad, elevationInRad)
        };

        SHEval (ambiOrder, pos.x, pos.y, pos.z, SH[i]);
        
        if (*useSN3D >= 0.5f)
            juce::FloatVectorOperations::multiply (SH[i], SH[i], n3d2sn3d, handleCh);
        
        const float* inpReadPtr = sampleBuffer.getReadPointer (i);
        for (int ch = 0; ch < handleCh; ++ch)
            ambiBuffer.addFromWithRamp (ch,
                                    0,
                                    inpReadPtr,
                                    ambiBuffer.getNumSamples(),
                                    _SH[i][ch] * _gain[i],
                                    SH[i][ch] * currGain);
        _gain[i] = currGain;
    }
    
   bufferCleaner(ambiBuffer, ambiOver, sampleBuffer);
    //---------------------------------------------------------------------------
    //BINAURAL DECODING
    if (ambiBuffer.getNumChannels() < 2)
    {
        ambiBuffer.clear();
        return;
    }

    const int L = ambiBuffer.getNumSamples();
    const int nCh = juce::jmin (L, bufferCh);
    const int ergL = overlapBuffer.getNumSamples();
    const int overlap = irLengthMinusOne;
    const int copyL = juce::jmin (L, overlap); // copy max L samples of the overlap data

    if (*useSN3D >= 0.5f)
        for (int ch = 1; ch < nCh; ++ch)
            ambiBuffer.applyGain (ch, 0, ambiBuffer.getNumSamples(), sn3d2n3d[ch]);

    // clear accumulation buffers
    juce::FloatVectorOperations::clear (reinterpret_cast<float*> (accumMid.data()), fftLength + 2);
    juce::FloatVectorOperations::clear (reinterpret_cast<float*> (accumSide.data()), fftLength + 2);

    const int nZeros = fftLength - L;

    //compute mid signal in frequency domain
    for (int midix = 0; midix < nMidCh; ++midix)
    {
        const int ch = mix2cix[midix];

        juce::FloatVectorOperations::copy (reinterpret_cast<float*> (fftBuffer.data()),
                                           ambiBuffer.getReadPointer (ch),
                                           L);
        juce::FloatVectorOperations::clear (reinterpret_cast<float*> (fftBuffer.data()) + L,
                                            nZeros);

        fft->performRealOnlyForwardTransform (reinterpret_cast<float*> (fftBuffer.data()));

        const auto tfMid =
            reinterpret_cast<const std::complex<float>*> (irsFrequencyDomain.getReadPointer (ch));
        for (int i = 0; i < fftLength / 2 + 1; ++i)
            accumMid[i] += fftBuffer[i] * tfMid[i];
    }

    //compute side signal in frequency domain
    for (int sidix = 0; sidix < nSideCh; ++sidix)
    {
        const int ch = six2cix[sidix];

        juce::FloatVectorOperations::copy (reinterpret_cast<float*> (fftBuffer.data()),
                                           ambiBuffer.getReadPointer (ch),
                                           L);
        juce::FloatVectorOperations::clear (reinterpret_cast<float*> (fftBuffer.data()) + L,
                                            nZeros);

        fft->performRealOnlyForwardTransform (reinterpret_cast<float*> (fftBuffer.data()));

        const auto tfSide =
            reinterpret_cast<const std::complex<float>*> (irsFrequencyDomain.getReadPointer (ch));
        for (int i = 0; i < fftLength / 2 + 1; ++i)
            accumSide[i] += fftBuffer[i] * tfSide[i];
    }

    fft->performRealOnlyInverseTransform (reinterpret_cast<float*> (accumMid.data()));
    fft->performRealOnlyInverseTransform (reinterpret_cast<float*> (accumSide.data()));

    // MS -> LR
    juce::FloatVectorOperations::copy (binBuffer.getWritePointer (0),
                                       reinterpret_cast<float*> (accumMid.data()),
                                       L);
    juce::FloatVectorOperations::copy (binBuffer.getWritePointer (1),
                                       reinterpret_cast<float*> (accumMid.data()),
                                       L);
    juce::FloatVectorOperations::add (binBuffer.getWritePointer (0),
                                      reinterpret_cast<float*> (accumSide.data()),
                                      L);
    juce::FloatVectorOperations::subtract (binBuffer.getWritePointer (1),
                                           reinterpret_cast<float*> (accumSide.data()),
                                           L);

    juce::FloatVectorOperations::add (binBuffer.getWritePointer (0),
                                      overlapBuffer.getWritePointer (0),
                                      copyL);
    juce::FloatVectorOperations::add (binBuffer.getWritePointer (1),
                                      overlapBuffer.getWritePointer (1),
                                      copyL);

    if (copyL < overlap) // there is some overlap left, want some?
    {
        const int howManyAreLeft = overlap - L;
        //shift the overlap buffer to the left
        juce::FloatVectorOperations::copy (overlapBuffer.getWritePointer (0),
                                           overlapBuffer.getReadPointer (0, L),
                                           howManyAreLeft);
        juce::FloatVectorOperations::copy (overlapBuffer.getWritePointer (1),
                                           overlapBuffer.getReadPointer (1, L),
                                           howManyAreLeft);

        //clear the tail
        juce::FloatVectorOperations::clear (overlapBuffer.getWritePointer (0, howManyAreLeft),
                                            ergL - howManyAreLeft);
        juce::FloatVectorOperations::clear (overlapBuffer.getWritePointer (1, howManyAreLeft),
                                            ergL - howManyAreLeft);

        // MS -> LR
        juce::FloatVectorOperations::add (overlapBuffer.getWritePointer (0),
                                          reinterpret_cast<float*> (accumMid.data()) + L,
                                          irLengthMinusOne);
        juce::FloatVectorOperations::add (overlapBuffer.getWritePointer (1),
                                          reinterpret_cast<float*> (accumMid.data()) + L,
                                          irLengthMinusOne);
        juce::FloatVectorOperations::add (overlapBuffer.getWritePointer (0),
                                          reinterpret_cast<float*> (accumSide.data()) + L,
                                          irLengthMinusOne);
        juce::FloatVectorOperations::subtract (overlapBuffer.getWritePointer (1),
                                               reinterpret_cast<float*> (accumSide.data()) + L,
                                               irLengthMinusOne);
    }
    else
    {
        // MS -> LR
        juce::FloatVectorOperations::copy (overlapBuffer.getWritePointer (0),
                                           reinterpret_cast<float*> (accumMid.data()) + L,
                                           irLengthMinusOne);
        juce::FloatVectorOperations::copy (overlapBuffer.getWritePointer (1),
                                           reinterpret_cast<float*> (accumMid.data()) + L,
                                           irLengthMinusOne);
        juce::FloatVectorOperations::add (overlapBuffer.getWritePointer (0),
                                          reinterpret_cast<float*> (accumSide.data()) + L,
                                          irLengthMinusOne);
        juce::FloatVectorOperations::subtract (overlapBuffer.getWritePointer (1),
                                               reinterpret_cast<float*> (accumSide.data()) + L,
                                               irLengthMinusOne);
    }
    //---------------------------------------------------------------------------
    //Output Processing & Signal Management
    //HP EQ
    if (*applyHeadphoneEq >= 0.5f)
    {
        float* channelData[2] = { binBuffer.getWritePointer (0), binBuffer.getWritePointer (1) };
        juce::dsp::AudioBlock<float> sumBlock (channelData, 2, L);
        juce::dsp::ProcessContextReplacing<float> eqContext (sumBlock);
        EQ.process (eqContext);
    }
    
    bufferCleaner(binBuffer, 2.0f, binBuffer);
    
    //Norm
    limitBuffer(binBuffer, binBuffCeil);
    limitBuffer(bufferClone, feedBuffCeil);

    //Ducking
    if(*duckingActive)
        ducking(binBuffer, bufferClone, *duckingAtten);
    else
        ducking(bufferClone, binBuffer, *duckingAtten);
    
    //Mixing
    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.addFrom (ch, 0, binBuffer, ch, 0, buffer.getNumSamples());
        buffer.addFrom (ch, 0, bufferClone, ch, 0, buffer.getNumSamples());
    }
    
    //Output Limit
    limitBuffer(buffer, *masterGain);
}

//==============================================================================
bool DirectionalSamplerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DirectionalSamplerAudioProcessor::createEditor()
{
    return new DirectionalSamplerAudioProcessorEditor (*this, parameters);
}

void DirectionalSamplerAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    DBG (parameterID << ": " << newValue);
    if (parameterID == "inputSetting" || parameterID == "orderSetting")
    {
        if (parameterID == "orderSetting")
            updateBuffers();
        else
            userChangedIOSettings = true;
    }
    else if (parameterID.startsWith ("solo"))
    {
        const int id = parameterID.substring (4).getIntValue();
        soloMask.setBit (id, newValue >= 0.5f);
        soloMuteChanged = true;
    }
    else if (parameterID.startsWith ("mute"))
    {
        const int id = parameterID.substring (4).getIntValue();
        muteMask.setBit (id, newValue >= 0.5f);
        soloMuteChanged = true;
    }
    else if (parameterID == "lockedToMaster")
    {
        if (newValue >= 0.5f && ! locked)
        {
            const int nChIn = input.getSize();
            float ypr[3];
            ypr[2] = 0.0f;
            for (int i = 0; i < nChIn; ++i)
            {
                iem::Quaternion<float> masterQuat;
                float masterypr[3];
                masterypr[0] = juce::degreesToRadians (masterAzimuth->load());
                masterypr[1] = juce::degreesToRadians (masterElevation->load());
                masterypr[2] = -juce::degreesToRadians (masterRoll->load());
                masterQuat.fromYPR (masterypr);
                masterQuat.conjugate();

                ypr[0] = juce::degreesToRadians (azimuth[i]->load());
                ypr[1] = juce::degreesToRadians (elevation[i]->load());
                quats[i].fromYPR (ypr);
                quats[i] = masterQuat * quats[i];
            }
            locked = true;
        }
        else if (newValue < 0.5f)
            locked = false;
    }
    else if (locked
             && ((parameterID == "masterAzimuth") || (parameterID == "masterElevation")
                 || (parameterID == "masterRoll")))
    {
        //if (dontTriggerMasterUpdate)
            //return;
        moving = true;
        iem::Quaternion<float> masterQuat;
        float ypr[3];
        ypr[0] = juce::degreesToRadians (masterAzimuth->load());
        ypr[1] = juce::degreesToRadians (masterElevation->load());
        ypr[2] = -juce::degreesToRadians (masterRoll->load());
        masterQuat.fromYPR (ypr);

        const int nChIn = input.getSize();
        for (int i = 0; i < nChIn; ++i)
        {
            iem::Quaternion<float> temp = masterQuat * quats[i];
            temp.toYPR (ypr);
            parameters.getParameter ("azimuth" + juce::String (i))
                ->setValueNotifyingHost (parameters.getParameterRange ("azimuth" + juce::String (i))
                                             .convertTo0to1 (juce::radiansToDegrees (ypr[0])));
            parameters.getParameter ("elevation" + juce::String (i))
                ->setValueNotifyingHost (
                    parameters.getParameterRange ("elevation" + juce::String (i))
                        .convertTo0to1 (juce::radiansToDegrees (ypr[1])));
        }
        moving = false;
        updateSphere = true;
    }
    else if (locked && ! moving
             && (parameterID.startsWith ("azimuth") || parameterID.startsWith ("elevation")))
    {
        float ypr[3];
        ypr[2] = 0.0f;
        const int nChIn = input.getSize();
        for (int i = 0; i < nChIn; ++i)
        {
            iem::Quaternion<float> masterQuat;
            float masterypr[3];
            masterypr[0] = juce::degreesToRadians (masterAzimuth->load());
            masterypr[1] = juce::degreesToRadians (masterElevation->load());
            masterypr[2] = -juce::degreesToRadians (masterRoll->load());
            masterQuat.fromYPR (masterypr);
            masterQuat.conjugate();

            ypr[0] = juce::degreesToRadians (azimuth[i]->load());
            ypr[1] = juce::degreesToRadians (elevation[i]->load());
            quats[i].fromYPR (ypr);
            quats[i] = masterQuat * quats[i];
        }
        updateSphere = true;
    }
    else if (parameterID.startsWith ("azimuth") || parameterID.startsWith ("elevation")
             || (parameterID == "masterAzimuth") || (parameterID == "masterElevation"))
    {
        updateSphere = true;
    }
    else if (parameterID.startsWith("nGain") || parameterID == "gainMin" ||
             parameterID == "gainActive")
    {
        gainUpdate();
    }
    else if (parameterID.startsWith("pitch") || parameterID == "pitchRange" ||
             parameterID == "pitchActive")
    {
        if (*seamless >= 0.5f)
            return;
        
        const int nChIn = mSampler.getNumVoices();
        if (*pitchActive)
        {
            for (int i = 0; i < nChIn; ++i)
            {
                float compPitch = *pitch[i] * *pitchRange;
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                    v->updatePitch(compPitch);
            }
        }
        else
        {
            for (int i = 0; i < nChIn; ++i)
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                    v->updatePitch(0.0f);
        }
    }
    else if (parameterID.startsWith("repF")|| parameterID == "repFMax" ||
             parameterID == "repFMin")
    {
        cycleUpdate();
    }
    else if (parameterID.startsWith("lpf") || parameterID == "lpfMin" ||
             parameterID == "lpfActive")
    {
        const int nChIn = mSampler.getNumVoices();
        if (*lpfActive)
        {
            for (int i = 0; i < nChIn; ++i)
            {
                float compFreq = ((20000.0f - *lpfRange) * *lpf[i]) + *lpfRange;
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                    v->setLowpassCutoff(compFreq);
            }
        }
        else
        {
            for (int i = 0; i < nChIn; ++i)
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                    v->setLowpassCutoff(20000.0f);
        }
    }
    else if (parameterID == "play")
    {
        if (mSampler.getNumVoices() == 0 || mSampler.getNumSounds() == 0)
            return;
        
        if (newValue == 1.f)
        {
            mSampler.allNotesOff(1, true);
            for (int i = 0; i < mSampler.getNumVoices(); ++i)
            {
                int pass = 59+i;
                mSampler.noteOn(1, pass, 1.0f);
            }
        }
        else
        {
            for (int i = 0; i < mSampler.getNumVoices(); ++i)
            {
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                    v->stopLoop(true);
            }
        }
    }
    else if (parameterID == "seamless")
    {
        const int nChIn = input.getSize();
        if (*seamless >= 0.5)
        {
            for (int i = 0; i < nChIn; ++i)
            {
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                {
                    v->stopLoop(true);
                    v->setFixedLength(true);
                    v->stopLoop(false);
                }
            }
            adsrParams.attack  = 0.0f;
            adsrParams.decay   = 0.0f;
            adsrParams.sustain = 1.0f;
            adsrParams.release = 0.0f;
        }
        else
        {
            for (int i = 0; i < nChIn; ++i)
            {
                double loopMs = ((*repFMax * 1000 - *repFMin) * *repF[i]) + *repFMin;
                if (loopMs < loopMin)
                    loopMs = loopMin;
                if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
                {
                    v->stopLoop(true);
                    v->setFixedLength(false);
                    v->setLoopLengthMs(loopMs);
                    v->stopLoop(false);
                }
            }
            adsrParams.attack  = 0.001f;
            adsrParams.decay   = 0.0f;
            adsrParams.sustain = 1.0f;
            adsrParams.release = 0.02f;
        }
        adsrDirty = true;
    }
    else if (parameterID == "guideActive")
    {
        guideChanged = true;
        if (*guideActive >= 0.5f)
        {
            playbackControl(false);
            mSamplerSoundsLoaded = false;
            
            auto* paramB = parameters.getParameter("inputSetting");
            paramB->setValueNotifyingHost(juce::jmap(1.f, 0.f, (float)maxNumberOfInputs, 0.0f, 1.0f));

            auto* paramA = parameters.getParameter("pickSample");
            paramA->setValueNotifyingHost(juce::jmap((float)guideSampleID, 0.f, (float)sampleLib.size(), 0.f, 1.f));
            
        }
        else
        {
            playbackControl(false);
            mSamplerSoundsLoaded = false;
            
            auto* paramB = parameters.getParameter("inputSetting");
            paramB->setValueNotifyingHost(juce::jmap((float)tempChannelCount, 0.f, (float)maxNumberOfInputs, 0.0f, 1.0f));

            auto* paramA = parameters.getParameter("pickSample");
            paramA->setValueNotifyingHost(juce::jmap((float)tempSampleID, 0.f, (float)sampleLib.size(), 0.f, 1.f));
            
            updateBuffers();
        }
    }
    else if (parameterID == "gainOffset")
    {
        if (newValue != 0.0f)
        {
            if (newValue < 0.0f)
            {
                binBuffCeil = -3.0f;
                feedBuffCeil = -3.0 + newValue;
            }
            else
            {
                binBuffCeil = -3.0 - newValue;
                feedBuffCeil = -3.0f;
            }
        }
        else
        {
            binBuffCeil = -3.0f;
            feedBuffCeil = -3.0f;
        }
        
    }
    
    else if (parameterID == "applyHeadphoneEq")
    {
        const int sel (juce::roundToInt (newValue));
        if (sel > 0)
        {
            int sourceDataSize;
            juce::String name = headphoneEQs[sel - 1].replace ("-", "") + "_wav";
            auto* sourceData = EQData::getNamedResource (name.toUTF8(), sourceDataSize);
            if (sourceData == nullptr)
                DBG ("error");
            EQ.loadImpulseResponse (sourceData,
                                    sourceDataSize,
                                    juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::no,
                                    2048,
                                    juce::dsp::Convolution::Normalise::no);
        }
    }
    else if (parameterID == "pickSample")
    {
        playbackControl(false);
        mSamplerSoundsLoaded = false;
        const int sel (juce::roundToInt(newValue));
        
        if (sel>0)
        {
            juce::String fileName = sampleLib[sel - 1].replace ("-", "") + "_wav";
            if (! *guideActive)
                if (sel != guideSampleID)
                tempSampleID = sel;
            if (fileName.startsWith("L_"))
            {
                auto* param = parameters.getParameter("seamless");
                param->setValueNotifyingHost(1.0f);
            }
            else
            {
                if (*seamless >= 0.5f)
                {
                    auto* param = parameters.getParameter("seamless");
                    param->setValueNotifyingHost(0.0f);
                }
            }
            loadSample(fileName);
        }
        
        if (guideChanged)
        {
            updateBuffers();
            guideChanged = false;
        }
        else
            playbackControl(true);
    }
    
}

//==============================================================================
void DirectionalSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    //enable and modify if needed in a vst
    //leave as is for standalone
    /*
    savedParams = { "memory",
        "inputSetting",
        "orderSetting",
        "masterGain",
        "applyHeadphoneEq",
        "pitchRange",
        "repFMin",
        "repFMax",
        "gainMin",
        "lpfMin",
        "gainOffset",
        "duckingAtten",
        "lpfActive",
        "gainActive",
        "pitchActive",
        "duckingActive"};

    if (*memory != 1.0f)
    {
        auto* p = parameters.getRawParameterValue("memory");
        lastState.setProperty("memory", p->load(), nullptr);
        DBG("MEM OFF STORED");
    }
    
    
    if (*memory == 1.0f)
    {
        for (auto& id : savedParams)
        {
            if (auto* p = parameters.getRawParameterValue(id))
                lastState.setProperty(id, p->load(), nullptr);  // Dereference atomic<float>
        }
        
        std::unique_ptr<juce::XmlElement> xml (lastState.createXml());
        copyXmlToBinary (*xml, destData);
        mem = destData;
        DBG("Data Stored");
    }
     */
}

void DirectionalSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    //enable and modify if needed in a vst
    //leave as is for standalone
    /*
    DBG("You even here?");
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState == nullptr)
        return;
    DBG("BRUH");
    if (!xmlState->hasTagName("ManualDefaults"))
        return;

    auto stateOnClose = juce::ValueTree::fromXml(*xmlState);

    //dontTriggerMasterUpdate = true;

    if (stateOnClose.hasProperty("memory"))
    {
        float value = (float) stateOnClose.getProperty("memory");
        auto* param = parameters.getParameter("memory");
        param->setValueNotifyingHost(param->convertTo0to1(value));
        DBG("MEM STATE " << value);
    }
    
    if (*memory == 1.0f)
    {
        for (auto& id : savedParams)
        {
            if (stateOnClose.hasProperty(id))
            {
                float value = (float) stateOnClose.getProperty(id);
                auto* param = parameters.getParameter(id);
                param->setValueNotifyingHost(param->convertTo0to1(value));
            }
        }
    }
    
    //dontTriggerMasterUpdate = false;
    
    DBG("Data Loaded");
    */
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DirectionalSamplerAudioProcessor();
}

void DirectionalSamplerAudioProcessor::updateBuffers()
{
    //update Buffers on parameter changes
    //setup
    const int bufferCh = input.getSize();
    const int sampleCh = *inputSetting;
    const int ambiCh = juce::square((float)*orderSetting);
    int ambiOrder = *orderSetting-1;
    const int handleCh = juce::jmax(sampleCh, ambiCh);
    symmetry(sampleCh, ambiCh);

    //prevent auto
    jassert(ambiOrder > 0);
    jassert(sampleCh > 0);
    
    auto block = getBlockSize();
    sampleBuffer.setSize (handleCh, block);
    ambiBuffer.setSize(handleCh, block);
    binBuffer.setSize(handleCh, block);
    
    //store tempChannelCount
    if (*guideActive != 1.0f)
        tempChannelCount = sampleCh;
    
    // disable solo and mute for deleted input channels
    for (int i = handleCh; i < _handleCh; ++i)
    {
        parameters.getParameter ("mute" + juce::String (i))->setValueNotifyingHost (0.0f);
        parameters.getParameter ("solo" + juce::String (i))->setValueNotifyingHost (0.0f);
    }
    
    //BinauralDecoder Block
    const double sampleRate = getSampleRate();
    const int blockSize = getBlockSize();

    int order = juce::jmax (ambiOrder, 1);
    const int nCh = ambiCh;

    int tmpOrder = sqrt (nCh) - 1;
    if (tmpOrder < order)
    {
        order = tmpOrder;
    }

    //get number of mid- and side-channels
    nSideCh = order * (order + 1) / 2;
    nMidCh = juce::square (order + 1)
             - nSideCh; //nMidCh = nCh - nSideCh; //nCh should be equalt to (order+1)^2

    if (order < 1)
        order = 1; // just use first order filters

    juce::AudioBuffer<float> resampledIRs;
    bool useResampled = false;
    irLength = 236;

    if (sampleRate != irsSampleRate && order != 0) // do resampling!
    {
        useResampled = true;
        double factorReading = irsSampleRate / sampleRate;
        irLength = juce::roundToInt (irLength / factorReading + 0.49);

        juce::MemoryAudioSource memorySource (irs[order - 1], false);
        juce::ResamplingAudioSource resamplingSource (&memorySource, false, nCh);

        resamplingSource.setResamplingRatio (factorReading);
        resamplingSource.prepareToPlay (irLength, sampleRate);

        resampledIRs.setSize (nCh, irLength);
        juce::AudioSourceChannelInfo info;
        info.startSample = 0;
        info.numSamples = irLength;
        info.buffer = &resampledIRs;

        resamplingSource.getNextAudioBlock (info);

        // compensate for more (correlated) samples contributing to output signal
        resampledIRs.applyGain (irsSampleRate / sampleRate);
    }

    irLengthMinusOne = irLength - 1;
    const int prevFftLength = fftLength;
    const int ergL = blockSize + irLength - 1; //max number of nonzero output samples
    fftLength = juce::nextPowerOfTwo (ergL); //fftLength >= ergL

    overlapBuffer.setSize (2, irLengthMinusOne);
    overlapBuffer.clear();

    if (prevFftLength != fftLength)
    {
        const int fftOrder = std::log2 (fftLength);
        fft = std::make_unique<juce::dsp::FFT> (fftOrder);
        fftBuffer.resize (fftLength);
        accumMid.resize (fftLength);
        accumSide.resize (fftLength);
    }

    irsFrequencyDomain.setSize (nCh, 2 * (fftLength / 2 + 1));
    irsFrequencyDomain.clear();

    for (int i = 0; i < nCh; ++i)
    {
        float* inOut = reinterpret_cast<float*> (fftBuffer.data());
        const float* src =
            useResampled ? resampledIRs.getReadPointer (i) : irs[order - 1].getReadPointer (i);
        juce::FloatVectorOperations::copy (inOut, src, irLength);
        juce::FloatVectorOperations::clear (inOut + irLength, fftLength - irLength); // zero padding
        fft->performRealOnlyForwardTransform (inOut);
        juce::FloatVectorOperations::copy (irsFrequencyDomain.getWritePointer (i),
                                           inOut,
                                           2 * (fftLength / 2 + 1));
    }

    //Wrap up
    _handleCh = handleCh;
    updateVoices();
};

void DirectionalSamplerAudioProcessor::updateQuaternions()
{
    float ypr[3];
    ypr[2] = 0.0f;

    iem::Quaternion<float> masterQuat;
    float masterypr[3];
    masterypr[0] = juce::degreesToRadians (masterAzimuth->load());
    masterypr[1] = juce::degreesToRadians (masterElevation->load());
    masterypr[2] = -juce::degreesToRadians (masterRoll->load());
    masterQuat.fromYPR (masterypr);
    masterQuat.conjugate();

    for (int i = 0; i < maxNumberOfInputs; ++i)
    {
        ypr[0] = juce::degreesToRadians (azimuth[i]->load());
        ypr[1] = juce::degreesToRadians (elevation[i]->load());
        quats[i].fromYPR (ypr);
        quats[i] = masterQuat * quats[i];
    }
}

void DirectionalSamplerAudioProcessor::updateVoices()
{
    //Voice Refresh on parameterchanges
    //setup
    const int sampleCh = *inputSetting;
    const int ambiCh = juce::square((float)*orderSetting);
    const int handleCh = juce::jmax(sampleCh, ambiCh);
    int v = mSampler.getNumVoices();
    float p = *play;

    //determine and fix assymetry
    if (sampleCh != v)
    {
        //kill Playback
        playbackControl(false);
        
        //adjust voices to channelcount
        if (v>sampleCh)
            for (int i=v-1; i > sampleCh-1; i--)
                mSampler.removeVoice (i);
        else
            for (int i=v; i < sampleCh; i++)
                mSampler.addVoice(new LoopingSamplerVoice(i));
                
        samplerSize = mSampler.getNumVoices();
        
        //OPT return to Playback
        cycleUpdate();
        gainUpdate();
        playbackControl(true);
    }
}

void DirectionalSamplerAudioProcessor::playbackControl(bool p)
{
    //safety mechanism on sample loads and voice changes
    if (! *play)
        return;
    if (! p)
        mSampler.allNotesOff(1, false);
    else
    {
        if (p)
        {
            mSampler.allNotesOff(1, true);
            for (int i = 0; i < mSampler.getNumVoices(); ++i)
            {
                int pass = 59+i;
                mSampler.noteOn(1, pass, 1.0f);
            }
        }
    }
}

void DirectionalSamplerAudioProcessor::limitBuffer (juce::AudioBuffer<float>& buff,
                                                    float ceilingDb)
{
    //limiter with adjustable output ceil
    //limited to 0, output to target ceil
    const int numSamples  = buff.getNumSamples();
    const int numChannels = buff.getNumChannels();

    //ceil to lin
    const float ceilingGain =
        juce::Decibels::decibelsToGain (ceilingDb);

    //limit
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buff.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = juce::jlimit (-1.0f, 1.0f, data[i]);
    }

    //pull to ceil
    normGain.setTargetValue (ceilingGain);

    const float startGain = normGain.getCurrentValue();
    const float endGain   = normGain.getNextValue();

    for (int ch = 0; ch < numChannels; ++ch)
        buff.applyGain(ch, 0, numSamples, ceilingGain);
}

void DirectionalSamplerAudioProcessor::ducking(juce::AudioBuffer<float>& bufferToDuck,
                                               juce::AudioBuffer<float>& sidechainBuffer,
                                               float attenuation)
{
    //ducking the signal set to be ducked by a certain amount by the key input
    const int numSamples  = bufferToDuck.getNumSamples();
    const int numChannels = bufferToDuck.getNumChannels();

    //measure max rmsavg across channels)
    float sidechainRms = 0.0f;
    for (int ch = 0; ch < sidechainBuffer.getNumChannels(); ++ch)
        sidechainRms = juce::jmax (
            sidechainRms,
            sidechainBuffer.getRMSLevel (ch, 0, numSamples));

    const bool sidechainActive = sidechainRms > 0.01f;
        
    //determine gains
    constexpr float normalGain = 1.0f;
    const float duckGain =  juce::Decibels::decibelsToGain (attenuation);

    //set state variable
    if (sidechainActive)
        gainToDuck = duckGain;
    else
        gainToDuck = normalGain;

    //apply gain ramp
    for (int ch = 0; ch < numChannels; ++ch)
    {
        bufferToDuck.applyGainRamp (
            ch,
            0,
            numSamples,
            gainToDuck.getCurrentValue(),
            gainToDuck.getNextValue());
    }
}

void DirectionalSamplerAudioProcessor::gainUpdate()
{
    //adjust the sampler's gain values on change or in setup scenarios
    //also passes the computed gain to the according array
    if (*gainActive)
    {
        const int nCh = mSampler.getNumVoices();
        for (int i = 0; i < nCh; ++i)
        {
            float curr = nGain[i]->load();
            float floor  = *gainMin;
            float g = juce::jmap(curr, -1.0f, 0.0f, floor, 0.f);
            gain[i]->store(g);
        }
    }
    else
    {
        const int nCh = mSampler.getNumVoices();
        for (int i = 0; i < nCh; ++i)
            gain[i]->store(0.0);
    }
}

void DirectionalSamplerAudioProcessor::cycleUpdate()
{
    //adjust the sampler's looplength on change or in setup scenarios
    if (mSampler.getNumVoices() == 0 || mSampler.getNumSounds() == 0 ||
        *seamless >= 0.5f)
        return;
    
    const int nCh = mSampler.getNumVoices();
    for (int i = 0; i < nCh; ++i)
    {
        double loopMs = ((*repFMax * 1000 - *repFMin) * *repF[i]) + *repFMin;
        if (loopMs < loopMin)
            loopMs = loopMin;
        if (auto* v = dynamic_cast<LoopingSamplerVoice*>(mSampler.getVoice(i)))
            v->setLoopLengthMs(loopMs);
    }
}

void DirectionalSamplerAudioProcessor::symmetry(int chA, int chB)
{
    //sets the state variable indicating the bufferCleaner what to do
    //chA is always meant to be sampleBuffer, chB = ambiBuffer
    if (chA != chB)
    {
        if (chA > chB)
            ambiOver = 0.0f;
        else
            ambiOver = 1.0f;
    }
    else
        ambiOver = 0.5f;
}

void DirectionalSamplerAudioProcessor::bufferCleaner(juce::AudioBuffer<float>& bufferToClean, float state, juce::AudioBuffer<float>& referenceBuffer)
{
    //safety mechanism to ensure channels that are meant to be empty, are empty!
    
    //if channelCounts are Equal
    if (state == 0.5f)
        return;
    
    //if BinBuffer
    if (state == 2.0f)
        for (int ch = 2; ch < referenceBuffer.getNumChannels(); ++ch)
            bufferToClean.clear (ch, 0, bufferToClean.getNumSamples());
    
    //sampleBuffer passed and sampleCh>ambiCh
    if (bufferToClean == sampleBuffer && state == 0.0)
        return;
    //ambiBuffer passed and ambiCh>sampleCh
    if (bufferToClean == ambiBuffer && state == 1.0)
        return;
    
    //Clean empty channels
    for (int ch = bufferToClean.getNumChannels(); ch < referenceBuffer.getNumChannels(); ++ch)
        bufferToClean.clear (ch, 0, bufferToClean.getNumSamples());
    
}

//==============================================================================
std::vector<std::unique_ptr<juce::RangedAudioParameter>>
    DirectionalSamplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //General
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "inputSetting",
        "Number of input channels ",
        "",
        juce::NormalisableRange<float> (0.0f, maxNumberOfInputs, 1.0f),
        startNnumberOfInputs,
        [] (float value) { return juce::String (value); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "orderSetting",
        "Ambisonics Order",
        "",
        juce::NormalisableRange<float> (0.0f, 8.0f, 1.0f),
        dfOrderSetting,
        [] (float value)
        {
            if (value >= 0.5f && value < 1.5f)
                return "0th";
            else if (value >= 1.5f && value < 2.5f)
                return "1st";
            else if (value >= 2.5f && value < 3.5f)
                return "2nd";
            else if (value >= 3.5f && value < 4.5f)
                return "3rd";
            else if (value >= 4.5f && value < 5.5f)
                return "4th";
            else if (value >= 5.5f && value < 6.5f)
                return "5th";
            else if (value >= 6.5f && value < 7.5f)
                return "6th";
            else if (value >= 7.5f)
                return "7th";
            else
                return "Auto";
        },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "useSN3D",
        "Normalization",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        1.0f,
        [] (float value)
        {
            if (value >= 0.5f)
                return "SN3D";
            else
                return "N3D";
        },
        nullptr));
    
    //Master
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "masterAzimuth",
        "Master azimuth angle",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "masterElevation",
        "Master elevation angle",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "masterRoll",
        "Master roll angle",
        juce::CharPointer_UTF8 (R"(°)"),
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.01f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "masterGain",
        "Master Gain",
        "dB",
        juce::NormalisableRange<float> (dfMasterGainBot, dfMasterGainCeil, 0.1f),
        dfMasterGain,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lockedToMaster",
        "Lock Directions relative to Master",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value >= 0.5f) ? "locked" : "not locked"; },
        nullptr));

    //Sources
    for (int i = 0; i < maxNumberOfInputs; ++i)
    {
        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "azimuth" + juce::String (i),
            "Azimuth angle " + juce::String (i + 1),
            juce::CharPointer_UTF8 (R"(°)"),
            juce::NormalisableRange<float> (-180.0f, 180.0f, 0.01f),
            0.0,
            [] (float value) { return juce::String (value, 2); },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "elevation" + juce::String (i),
            "Elevation angle " + juce::String (i + 1),
            juce::CharPointer_UTF8 (R"(°)"),
            juce::NormalisableRange<float> (-180.0f, 180.0f, 0.01f),
            0.0,
            [] (float value) { return juce::String (value, 2); },
            nullptr));
        
        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "gain" + juce::String (i),
            "Gain " + juce::String (i + 1),
            "dB",
            juce::NormalisableRange<float> (dfGainBot, 0.0f, 0.01f),
            0.0f,
            [] (float value) { return juce::String (value*100, 2); },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "nGain" + juce::String (i),
            "Gain " + juce::String (i + 1),
            "%",
            juce::NormalisableRange<float> (-1.0f, 0.0f, 0.01f),
            -0.5f,
            [] (float value) { return juce::String (value*100, 1); },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "mute" + juce::String (i),
            "Mute input " + juce::String (i + 1),
            "",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
            0.0f,
            [] (float value) { return (value >= 0.5f) ? "muted" : "not muted"; },
            nullptr));

        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "solo" + juce::String (i),
            "Solo input " + juce::String (i + 1),
            "",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
            0.0f,
            [] (float value) { return (value >= 0.5f) ? "soloed" : "not soloed"; },
            nullptr));
        
        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "pitch" + juce::String (i),
            "Pitch" + juce::String (i + 1),
            "%",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f),
            0.0f,
            [] (float value) { return juce::String (value*100, 2); },
            nullptr));
                    
        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "repF" + juce::String (i),
            "Repetition Freq" + juce::String (i + 1),
            "%",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
            1.0f,
            [] (float value) { return juce::String (value*100, 2); },
            nullptr));
                    
        params.push_back (OSCParameterInterface::createParameterTheOldWay (
            "lpf" + juce::String (i),
            "LPF" + juce::String (i + 1),
            "%",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f, 0.5f),
            1.0f,
            [] (float value) { return juce::String (value*100, 2); },
            nullptr));
    }

    //Sampler Sliders
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "gainMin",
        "Gain Min",
        "dB",
        juce::NormalisableRange<float> (dfGainBot, dfGainMinCeil, 0.1f),
        dfGainMin,
        [] (float value) { return (value >= -59.9f) ? juce::String (value, 1) : "-inf"; },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "pitchRange",
        "Pitch Range",
        "ct",
        juce::NormalisableRange<float> (dfPitchRangeBot, dfPitchRangeCeil, 1.0f),
        dfPitchRange,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "repFMin",
        "Cycle Min",
        "ms",
        juce::NormalisableRange<float> (dfRepFMinBot, dfRepFMinCeil, 1.0f),
        dfRepFMin,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
            
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "repFMax",
        "Cycle Max",
        "s",
        juce::NormalisableRange<float> (dfRepFMaxBot, dfRepFMaxCeil, 0.1f),
        dfRepFMax,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lpfMin",
        "LPF Min",
        "Hz",
        juce::NormalisableRange<float> (20.0f, dfLpfMinCeil, 1.0f),
        dfLpfMin,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
        
    //Sampler Toggles
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "play",
        "Play",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
            
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "seamless",
        "Seamless",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        dfSeamless,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "guideActive",
        "Guide",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "gainActive",
        "Enable Gain",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        dfGainActive,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "pitchActive",
        "Enable Pitch",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        dfPitchActive,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "lpfActive",
        "Enable LPF",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        dfLpfActive,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
    
    //Volume Management
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "gainOffset",
        "Thru Gain Offset",
        "dB",
        juce::NormalisableRange<float> (-dfGainOffset, dfGainOffset, 0.1f),
        0.0f,
        [] (float value) { return juce::String (value, 1); },
        nullptr));
            
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "duckingAtten",
        "Ducking Level",
        "dB",
        juce::NormalisableRange<float> (dfDuckingAttenBot, 0.0f, 0.1f),
        dfDuckingAtten,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
        
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "duckingActive",
        "Duck BIN",
        "Duck BIN: N/Y",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        dfDuckigActive,
        [] (float value) { return juce::String (value, 2); },
        nullptr));
        
    //Utility
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "analyzeRMS",
        "Analzes RMS",
        "",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f),
        0.0f,
        [] (float value) { return (value >= 0.5f) ? "on" : "off"; },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "peakLevel",
        "Peak level",
        "dB",
        juce::NormalisableRange<float> (-50.0f, 10.0f, 0.1f),
        0.0,
        [] (float value) { return juce::String (value, 1); },
        nullptr));

    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "dynamicRange",
        "Dynamic juce::Range",
        "dB",
        juce::NormalisableRange<float> (10.0f, 60.0f, 1.f),
        35.0,
        [] (float value) { return juce::String (value, 0); },
        nullptr));
        
    //CB Selectors
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "applyHeadphoneEq",
        "Headphone Equalization",
        "",
        juce::NormalisableRange<float> (0.0f, float (headphoneEQs.size()), 1.0f),
        0.0f,
        [this] (float value)
        {
            if (value < 0.5f)
                return juce::String ("OFF");
            else
                return juce::String (this->headphoneEQs[juce::roundToInt (value) - 1]);
        },
        nullptr));
            
    params.push_back (OSCParameterInterface::createParameterTheOldWay (
        "pickSample",
        "Sample Chooser",
        "",
        juce::NormalisableRange<float> (0.0f, float (sampleLib.size()), 1.0f),
        0.0f,
        [this] (float value)
        {
            if (value < 0.5f)
                return juce::String ("None");
            else
                return juce::String (this->sampleLib[juce::roundToInt (value) - 1]);
        },
        nullptr));
        
    return params;
}

//==============================================================================

juce::Result DirectionalSamplerAudioProcessor::loadConfiguration (const juce::File& presetFile)
{
    if (! presetFile.exists())
    {
        return juce::Result::fail ("File does not exist!");
    }

    const juce::String jsonString = presetFile.loadFileAsString();

    return loadConfigFromString (jsonString);
}

juce::Result DirectionalSamplerAudioProcessor::loadConfigFromString (juce::String configString)
{
    if (configString.isEmpty())
        return juce::Result::fail ("Empty configuration string!");

    juce::var parsedJson;
    juce::Result result = juce::JSON::parse (configString, parsedJson);

    if (result.failed())
        return result;

    juce::ValueTree newSources ("NewSources");
    result = ConfigurationHelper::parseVarForLoudspeakerLayout (parsedJson, newSources, nullptr);

    if (result.wasOk())
    {
        int nSrc = 0;
        const auto nElements = newSources.getNumChildren();

        for (int i = 0; i < nElements; ++i)
        {
            auto src = newSources.getChild (i);
            const int ch = src.getProperty ("Channel");
            const bool isImaginary = src.getProperty ("Imaginary");
            if (! isImaginary && ch > nSrc)
                nSrc = ch;
        }

        DBG (nSrc << " Sources!");
        parameters.getParameterAsValue ("inputSetting").setValue (nSrc);

        for (int s = 0; s < nSrc; ++s)
            parameters.getParameterAsValue ("mute" + juce::String (s)).setValue (1);

        for (int e = 0; e < nElements; ++e)
        {
            const auto src = newSources.getChild (e);
            const int ch = static_cast<int> (src.getProperty ("Channel", 0)) - 1;
            const bool isImaginary = src.getProperty ("Imaginary");

            if (isImaginary || ch < 0 || ch >= 64)
                continue;

            parameters.getParameterAsValue ("mute" + juce::String (ch)).setValue (0);

            float azi = src.getProperty ("Azimuth", 0.0f);
            float ele = src.getProperty ("Elevation", 0.0f);

            if (azi > 180.0f || azi < -180.0f || ele > 90.0f || ele < -90.0f)
                wrapSphericalCoordinates (&azi, &ele);

            parameters.getParameterAsValue ("azimuth" + juce::String (ch)).setValue (azi);
            parameters.getParameterAsValue ("elevation" + juce::String (ch)).setValue (ele);
        }
    }
    return result;
}

void DirectionalSamplerAudioProcessor::loadSample(const juce::String& resourceName)
{
    //Retrieve
    int dataSize = 0;
    const void* data = SampleData::getNamedResource(
        resourceName.toRawUTF8(), dataSize);
    if (data == nullptr || dataSize == 0)
    {
        DBG("Resource not found: " + resourceName);
        return;
    }

    // Create memory stream & Reader
    auto stream = std::make_unique<juce::MemoryInputStream>(
        data,
        static_cast<size_t>(dataSize),
        false
    );

    std::unique_ptr<juce::AudioFormatReader> reader(
        mFormatManager.createReaderFor(std::move(stream)));

    if (!reader)
    {
        DBG("Could not create reader from binary data!");
        return;
    }

    //prepare
    mSampler.clearSounds();

    juce::AudioBuffer<float> samBuffer(
        (int)reader->numChannels,
        (int)reader->lengthInSamples);
    
    //safety
    suspendProcessing(true);
    
    //push to sampler
    reader->read(&samBuffer, 0,
                 (int)reader->lengthInSamples,
                 0, true, true);

    mSampler.addSound(
        new LoopingSound(std::move(samBuffer),
                         reader->sampleRate,
                         60));
    
    DBG("Sample loaded successfully!");
    
    manualSample = false;
    
    //Resolve
    mSamplerSoundsLoaded = true;
    suspendProcessing(false);
}

void DirectionalSamplerAudioProcessor::loadSampleManually()
{
    juce::File fileToLoad;

    //in case you change your mind
    juce::FileChooser chooser("Load sample");
    if (chooser.browseForFileToOpen())
    {
            fileToLoad = chooser.getResult(); // override default if user picks a file
    }

    std::unique_ptr<juce::AudioFormatReader> reader(mFormatManager.createReaderFor(fileToLoad));
    
    if (!reader)
    {
        DBG("Could not create reader from binary data!");
        return;
    }

    mSampler.clearSounds();

    juce::AudioBuffer<float> buffi((int)reader->numChannels, (int)reader->lengthInSamples);
    buffi.clear();
    suspendProcessing(true);
    
    reader->read(&buffi, 0, (int)reader->lengthInSamples, 0, true, true);

    mSampler.addSound(new LoopingSound(std::move(buffi), reader->sampleRate, 60));
    
    DBG("Sample loaded successfully!");
    
    auto* param = parameters.getParameter("pickSample");
    param->setValueNotifyingHost(0.0f);

    //Resolve
    mSamplerSoundsLoaded = true;
    manualSample = true;
    suspendProcessing(false);
}

void DirectionalSamplerAudioProcessor::wrapSphericalCoordinates (float* azi, float* ele)
{
    // Wrap elevation
    if (*ele > 180.0f || *ele < -180.0f)
    {
        float fullRotations = std::copysign (std::ceil (std::abs (*ele) / 360.0f), *ele);
        *ele -= fullRotations * 360.0f;
    }
    if (*ele > 90.0f || *ele < -90.0f)
    {
        *ele = std::copysign (180.0 - std::abs (*ele), *ele);
        *azi += 180.0f;
    }

    // Wrap azimuth
    if (*azi > 180.0f || *azi < -180.0f)
    {
        float fullRotations = std::copysign (std::ceil (std::abs (*azi) / 360.0f), *azi);
        *azi -= fullRotations * 360.0f;
    }
}

void DirectionalSamplerAudioProcessor::setLastDir (juce::File newLastDir)
{
    lastDir = newLastDir;
    const juce::var v (lastDir.getFullPathName());
    properties->setValue ("presetFolder", v);
}

