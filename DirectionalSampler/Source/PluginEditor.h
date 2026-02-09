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

#include "../../resources/customComponents/MuteSoloButton.h"
#include "../../resources/customComponents/ReverseSlider.h"
#include "../../resources/customComponents/SimpleLabel.h"
#include "../../resources/customComponents/SpherePanner.h"
#include "../../resources/customComponents/TitleBar.h"
#include "../../resources/lookAndFeel/IEM_LaF.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "CostumEncoderList.h"
#include "EnergySpherePanner.h"
#include "MasterControlWithText.h"
#include "PluginProcessor.h"

typedef ReverseSlider::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//==============================================================================
class DirectionalSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private SpherePanner::Listener
{
public:
    DirectionalSamplerAudioProcessorEditor (DirectionalSamplerAudioProcessor&,
                                      juce::AudioProcessorValueTreeState&);
    ~DirectionalSamplerAudioProcessorEditor();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void importLayout();

private:
    LaF globalLaF;
    TitleBar<AudioChannelsIOWidget<maxNumberOfInputs>, AmbisonicIOWidget<>> title;
    OSCFooter footer;

    void timerCallback() override;
    void mouseWheelOnSpherePannerMoved (SpherePanner* sphere,
                                        const juce::MouseEvent& event,
                                        const juce::MouseWheelDetails& wheel) override;

    DirectionalSamplerAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& valueTreeState;

    juce::GroupComponent masterGroup,
        samplerGlobalsGroup,
        encoderGroup,
        rmsGroup,
        globalRangeGroup,
        globalCtrlGroup,
        localGroup;
    
    ReverseSlider slMasterAzimuth,
        slMasterElevation,
        slMasterRoll,
        slMasterGain,
    
        gainMinSlider,
        pitchRangeSlider,
        repFMinSlider,
        repFMaxSlider,
        lpfMinSlider,
    
        gainOffsetSlider,
        duckingAttenSlider,
    
        slPeakLevel,
        slDynamicRange;
    
    juce::TextButton tbImport;
    
    juce::ToggleButton tbLockedToMaster,
        playButton,
        seamlessButton,
        guideActiveButton,
    
        gainActiveButton,
        pitchActiveButton,
        lpfActiveButton,
        
        duckingActiveButton,
    
        tbAnalyzeRMS;

    juce::ComboBox inputChooser,
        cbEq,
        cbSamples;

    std::unique_ptr<SliderAttachment> slMasterAzimuthAttachment,
        slMasterElevationAttachment,
        slMasterRollAttachment,
        slMasterGainAttachment,
    
        slGainMinAttachment,
        slPitchRangeAttachment,
        slrepFMinAttachment,
        slrepFMaxAttachment,
        slLpfMinAttachment,
        
        slGainOffsetAttachment,
        slDuckingAttenAttachment,
    
        slPeakLevelAttachment,
        slDynamicRangeAttachment;
    
    std::unique_ptr<ButtonAttachment> tbLockedToMasterAttachment,
        tbPlayAttachment,
        tbseamlessAttachment,
        tbguideActiveAttachment,
        
        tbGainActiveAttachment,
        tbPitchActiveAttachment,
        tbLpfActiveAttachment,
        
        tbDuckingActiveAttachment,
    
        tbAnalyzeRMSAttachment;

    std::unique_ptr<ComboBoxAttachment> cbNumInputChannelsAttachment,
        cbNormalizationAtachment,
        cbOrderAtachment,
        cbEqAttachment,
        cbSamplesAttachment;
    
    std::unique_ptr<CostumEncoderList> encoderList;

    juce::Viewport viewport;
    juce::TooltipWindow tooltipWin;
    
    EnergySpherePanner sphere;
    SpherePanner::AzimuthElevationParameterElement masterElement;

    int maxPossibleOrder = -1;
    int maxNumInputs = -1;
    int lastSetNumChIn = -1;

    // labels
    SimpleLabel lbMasterAzimuth,
        lbMasterElevation,
        lbMasterRoll,
        lbMasterGain,
    
        lbNum,
        lbGainMin,
        lbPitchRange,
        lbRepFMin,
        lbrepFMax,
        lbLpfMin,
    
        lbGainOffset,
        lbDuckingAtten,
    
        lbPeakLevel,
        lbDynamicRange;
        
    std::unique_ptr<MasterControlWithText> lbAzimuth,
        lbElevation,
        lbGain,
        lbPitch,
        lbRepF,
        lbLpf;
    
    juce::Label* getSliderTextBox (juce::Slider& slider)
    {
        for (int i = 0; i < slider.getNumChildComponents(); ++i)
            if (auto* label = dynamic_cast<juce::Label*> (slider.getChildComponent(i)))
                return label;

        return nullptr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectionalSamplerAudioProcessorEditor)
};
