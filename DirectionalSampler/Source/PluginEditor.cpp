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

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
DirectionalSamplerAudioProcessorEditor::DirectionalSamplerAudioProcessorEditor (
    DirectionalSamplerAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts) :
    juce::AudioProcessorEditor (&p),
    footer (p.getOSCParameterInterface()),
    processor (p),
    valueTreeState (vts),
    sphere (p.rms),
    masterElement (*valueTreeState.getParameter ("masterAzimuth"),
                   valueTreeState.getParameterRange ("masterAzimuth"),
                   *valueTreeState.getParameter ("masterElevation"),
                   valueTreeState.getParameterRange ("masterElevation"))
{
    setLookAndFeel (&globalLaF);

    // ==== SPHERE AND ELEMENTS ===============
    addAndMakeVisible (&sphere);
    sphere.addListener (this);

    sphere.addElement (&masterElement);
    masterElement.setColour (juce::Colours::black);
    masterElement.setTextColour (juce::Colours::white);
    masterElement.setLabel ("M");

    encoderList = std::make_unique<CostumEncoderList> (p, sphere, &vts);
    lbAzimuth = std::make_unique<MasterControlWithText> (encoderList->getAzimuthArray());
    lbElevation = std::make_unique<MasterControlWithText> (encoderList->getElevationArray());
    lbGain = std::make_unique<MasterControlWithText> (encoderList->getGainArray());
    lbPitch = std::make_unique<MasterControlWithText> (encoderList->getPitchArray());
    lbRepF = std::make_unique<MasterControlWithText> (encoderList->getRepFArray());
    lbLpf = std::make_unique<MasterControlWithText> (encoderList->getLpfArray());
    
    // ======================================

    addAndMakeVisible (&title);
    title.setTitle (juce::String ("Directional"), juce::String ("Sampler"));
    title.setFont (globalLaF.robotoBold, globalLaF.robotoLight);

    addAndMakeVisible (&footer);

    tooltipWin.setLookAndFeel (&globalLaF);
    tooltipWin.setMillisecondsBeforeTipAppears (500);
    tooltipWin.setOpaque (false);

    cbNumInputChannelsAttachment.reset (
        new ComboBoxAttachment (valueTreeState,
                                "inputSetting",
                                *title.getInputWidgetPtr()->getChannelsCbPointer()));
    cbNormalizationAtachment.reset (
        new ComboBoxAttachment (valueTreeState,
                                "useSN3D",
                                *title.getOutputWidgetPtr()->getNormCbPointer()));
    cbOrderAtachment.reset (
        new ComboBoxAttachment (valueTreeState,
                                "orderSetting",
                                *title.getOutputWidgetPtr()->getOrderCbPointer()));

    // ======================== Encoder group
    encoderGroup.setText ("Encoder settings");
    encoderGroup.setTextLabelPosition (juce::Justification::centredLeft);
    encoderGroup.setColour (juce::GroupComponent::outlineColourId, globalLaF.ClSeperator);
    encoderGroup.setColour (juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible (&encoderGroup);
    encoderGroup.setVisible (true);

    addAndMakeVisible (tbImport);
    tbImport.setButtonText ("IMPORT");
    tbImport.setColour (juce::TextButton::buttonColourId, juce::Colours::orange);
    tbImport.setTooltip ("Imports sources from a configuration file.");
    tbImport.onClick = [&]() { importLayout(); };

    addAndMakeVisible (&viewport);
    viewport.setViewedComponent (encoderList.get());

    // ====================== GRAB GROUP
    masterGroup.setText ("Master");
    masterGroup.setTextLabelPosition (juce::Justification::centredLeft);
    masterGroup.setColour (juce::GroupComponent::outlineColourId, globalLaF.ClSeperator);
    masterGroup.setColour (juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible (&masterGroup);
    masterGroup.setVisible (true);

    addAndMakeVisible (&slMasterAzimuth);
    slMasterAzimuthAttachment.reset (
        new SliderAttachment (valueTreeState, "masterAzimuth", slMasterAzimuth));
    slMasterAzimuth.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slMasterAzimuth.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slMasterAzimuth.setReverse (true);
    slMasterAzimuth.setColour (juce::Slider::rotarySliderOutlineColourId,
                               globalLaF.ClWidgetColours[0]);
    slMasterAzimuth.setRotaryParameters (juce::MathConstants<float>::pi,
                                         3 * juce::MathConstants<float>::pi,
                                         false);
    slMasterAzimuth.setTooltip ("Master azimuth angle");

    addAndMakeVisible (&slMasterElevation);
    slMasterElevationAttachment.reset (
        new SliderAttachment (valueTreeState, "masterElevation", slMasterElevation));
    slMasterElevation.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slMasterElevation.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slMasterElevation.setColour (juce::Slider::rotarySliderOutlineColourId,
                                 globalLaF.ClWidgetColours[0]);
    slMasterElevation.setRotaryParameters (0.5 * juce::MathConstants<float>::pi,
                                           2.5 * juce::MathConstants<float>::pi,
                                           false);
    slMasterElevation.setTooltip ("Master elevation angle");

    addAndMakeVisible (&slMasterRoll);
    slMasterRollAttachment.reset (
        new SliderAttachment (valueTreeState, "masterRoll", slMasterRoll));
    slMasterRoll.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slMasterRoll.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slMasterRoll.setColour (juce::Slider::rotarySliderOutlineColourId,
                            globalLaF.ClWidgetColours[2]);
    slMasterRoll.setRotaryParameters (juce::MathConstants<float>::pi,
                                      3 * juce::MathConstants<float>::pi,
                                      false);
    slMasterRoll.setTooltip ("Master roll angle");
    
    addAndMakeVisible (&slMasterGain);
    slMasterGainAttachment.reset (new SliderAttachment (valueTreeState, "masterGain", slMasterGain));
    slMasterGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slMasterGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slMasterGain.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[1]);

    addAndMakeVisible (&tbLockedToMaster);
    tbLockedToMasterAttachment.reset (
        new ButtonAttachment (valueTreeState, "lockedToMaster", tbLockedToMaster));
    tbLockedToMaster.setName ("locking");
    tbLockedToMaster.setButtonText ("Lock Directions");
    
    samplerGlobalsGroup.setText ("Sampler Globals");
    samplerGlobalsGroup.setTextLabelPosition (juce::Justification::centredLeft);
    samplerGlobalsGroup.setColour (juce::GroupComponent::outlineColourId, globalLaF.ClSeperator);
    samplerGlobalsGroup.setColour (juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible (&samplerGlobalsGroup);
    samplerGlobalsGroup.setVisible (true);
    
    addAndMakeVisible (&gainMinSlider);
    slGainMinAttachment.reset (new SliderAttachment (valueTreeState, "gainMin", gainMinSlider));
    gainMinSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainMinSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    gainMinSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    
    addAndMakeVisible (&pitchRangeSlider);
    slPitchRangeAttachment.reset (new SliderAttachment (valueTreeState, "pitchRange", pitchRangeSlider));
    pitchRangeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    pitchRangeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    //pitchRangeSlider.setReverse (true);
    pitchRangeSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    
    addAndMakeVisible (&repFMinSlider);
    slrepFMinAttachment.reset (new SliderAttachment (valueTreeState, "repFMin", repFMinSlider));
    repFMinSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    repFMinSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    repFMinSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[2]);

    addAndMakeVisible (&repFMaxSlider);
    slrepFMaxAttachment.reset (new SliderAttachment (valueTreeState, "repFMax", repFMaxSlider));
    repFMaxSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    repFMaxSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    repFMaxSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[2]);
    
    addAndMakeVisible (&lpfMinSlider);
    slLpfMinAttachment.reset (new SliderAttachment (valueTreeState, "lpfMin", lpfMinSlider));
    lpfMinSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lpfMinSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    lpfMinSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[0]);
    
    addAndMakeVisible (&gainOffsetSlider);
    slGainOffsetAttachment.reset (new SliderAttachment (valueTreeState, "gainOffset", gainOffsetSlider));
    gainOffsetSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainOffsetSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    gainOffsetSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[1]);
    
    addAndMakeVisible (&duckingAttenSlider);
    slDuckingAttenAttachment.reset (new SliderAttachment (valueTreeState, "duckingAtten", duckingAttenSlider));
    duckingAttenSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    duckingAttenSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    duckingAttenSlider.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[1]);
    
    addAndMakeVisible (&playButton);
    tbPlayAttachment.reset (
        new ButtonAttachment (valueTreeState, "play", playButton));
    playButton.setName ("play");
    playButton.setButtonText ("");
    
    addAndMakeVisible (&seamlessButton);
    tbseamlessAttachment.reset (
        new ButtonAttachment (valueTreeState, "seamless", seamlessButton));
    seamlessButton.setName ("seamless");
    seamlessButton.setButtonText ("Seamless");
    
    addAndMakeVisible (&guideActiveButton);
    tbguideActiveAttachment.reset (
        new ButtonAttachment (valueTreeState, "guideActive", guideActiveButton));
    guideActiveButton.setName ("guideActive");
    guideActiveButton.setButtonText ("Guide");
    
    addAndMakeVisible (&gainActiveButton);
    tbGainActiveAttachment.reset (
        new ButtonAttachment (valueTreeState, "gainActive", gainActiveButton));
    gainActiveButton.setName ("gainActive");
    gainActiveButton.setButtonText ("");
    
    addAndMakeVisible (&pitchActiveButton);
    tbPitchActiveAttachment.reset (
        new ButtonAttachment (valueTreeState, "pitchActive", pitchActiveButton));
    pitchActiveButton.setName ("pitchActive");
    pitchActiveButton.setButtonText ("");
    
    addAndMakeVisible (&lpfActiveButton);
    tbLpfActiveAttachment.reset (
        new ButtonAttachment (valueTreeState, "lpfActive", lpfActiveButton));
    lpfActiveButton.setName ("lpfActive");
    lpfActiveButton.setButtonText ("");
    
    addAndMakeVisible (&duckingActiveButton);
    tbDuckingActiveAttachment.reset (
        new ButtonAttachment (valueTreeState, "duckingActive", duckingActiveButton));
    duckingActiveButton.setName ("duckingActive");
    duckingActiveButton.setButtonText ("Duck AMBI");

    // ======================== RMS group
    rmsGroup.setText ("Visualize");
    rmsGroup.setTextLabelPosition (juce::Justification::centredLeft);
    rmsGroup.setColour (juce::GroupComponent::outlineColourId, globalLaF.ClSeperator);
    rmsGroup.setColour (juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible (&rmsGroup);

    addAndMakeVisible (&slPeakLevel);
    slPeakLevel.onValueChange = [&]() { sphere.setPeakLevel (slPeakLevel.getValue()); };
    slPeakLevelAttachment.reset (new SliderAttachment (valueTreeState, "peakLevel", slPeakLevel));
    slPeakLevel.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slPeakLevel.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 12);
    slPeakLevel.setTextValueSuffix (" dB");
    slPeakLevel.setColour (juce::Slider::rotarySliderOutlineColourId, globalLaF.ClWidgetColours[2]);

    addAndMakeVisible (&lbPeakLevel);
    lbPeakLevel.setText ("Peak");

    addAndMakeVisible (&slDynamicRange);
    slDynamicRange.onValueChange = [&]() { sphere.setDynamicRange (slDynamicRange.getValue()); };
    slDynamicRangeAttachment.reset (
        new SliderAttachment (valueTreeState, "dynamicRange", slDynamicRange));
    slDynamicRange.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slDynamicRange.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 12);
    slDynamicRange.setTextValueSuffix (" dB");
    slDynamicRange.setColour (juce::Slider::rotarySliderOutlineColourId,
                              globalLaF.ClWidgetColours[0]);
    slDynamicRange.setReverse (false);

    addAndMakeVisible (&lbDynamicRange);
    lbDynamicRange.setText ("Range");

    addAndMakeVisible (&tbAnalyzeRMS);
    tbAnalyzeRMS.onStateChange = [&]() { sphere.visualizeRMS (tbAnalyzeRMS.getToggleState()); };
    tbAnalyzeRMSAttachment.reset (
        new ButtonAttachment (valueTreeState, "analyzeRMS", tbAnalyzeRMS));
    tbAnalyzeRMS.setButtonText ("Visualize");

    // ================ LABELS ===================
    addAndMakeVisible (&lbNum);
    lbNum.setText ("#");

    addAndMakeVisible (*lbPitch);
    lbPitch->setText ("Pitch");
    
    addAndMakeVisible (*lbRepF);
    lbRepF->setText ("Cycle");
    
    addAndMakeVisible (*lbLpf);
    lbLpf->setText ("LPF");
    
    addAndMakeVisible (*lbAzimuth);
    lbAzimuth->setText ("Azi");

    addAndMakeVisible (*lbElevation);
    lbElevation->setText ("Elv");

    addAndMakeVisible (*lbGain);
    lbGain->setText ("Gain");

    addAndMakeVisible (&lbMasterAzimuth);
    lbMasterAzimuth.setText ("Azimuth");

    addAndMakeVisible (&lbMasterElevation);
    lbMasterElevation.setText ("Elevation");

    addAndMakeVisible (&lbMasterRoll);
    lbMasterRoll.setText ("Roll");
    
    addAndMakeVisible (&lbMasterGain);
    lbMasterGain.setText ("Gain");
    
    addAndMakeVisible (&lbPitchRange);
    lbPitchRange.setText ("Pitch");
    
    addAndMakeVisible (&lbRepFMin);
    lbRepFMin.setText ("Cycle Min");
    
    addAndMakeVisible (&lbrepFMax);
    lbrepFMax.setText ("Cycle Max");
    
    addAndMakeVisible (&lbGainMin);
    lbGainMin.setText ("Gain Min");
    
    addAndMakeVisible (&lbLpfMin);
    lbLpfMin.setText ("LPF Min");
    
    addAndMakeVisible (&lbGainOffset);
    lbGainOffset.setText ("Thru Offset");
    
    addAndMakeVisible (&lbDuckingAtten);
    lbDuckingAtten.setText ("Ducking");

    addAndMakeVisible (cbEq);
    cbEq.addItem ("Headphone Equalization (off)", 1);
    cbEq.addItemList (processor.headphoneEQs, 2);
    cbEqAttachment.reset (new ComboBoxAttachment (valueTreeState, "applyHeadphoneEq", cbEq));
    
    addAndMakeVisible (cbSamples);
    cbSamples.addItem ("No Sample or Manual Pick", 1);
    cbSamples.addItemList (processor.sampleLib, 3);
    cbSamplesAttachment.reset (new ComboBoxAttachment (valueTreeState, "pickSample", cbSamples));
    
    mLoadButton.onClick = [&]() {processor.loadSampleManually();};
    addAndMakeVisible(mLoadButton);

    setResizeLimits (870, 750, 1740, 1500);
    setResizable (true, true);
    
    startTimer (40);
}

DirectionalSamplerAudioProcessorEditor::~DirectionalSamplerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void DirectionalSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (globalLaF.ClBackground);
}

void DirectionalSamplerAudioProcessorEditor::timerCallback()
{
    // === update titleBar widgets according to available input/output channel counts
    title.setMaxSize (processor.getMaxSize());
    // ==========================================

    const int nChIn = processor.samplerSize;
    if (nChIn != lastSetNumChIn)
    {
        encoderList->setNumberOfChannels (nChIn);
        lastSetNumChIn = nChIn;
        sphere.repaint();
    }

    if (processor.soloMuteChanged)
    {
        if (! processor.soloMask.isZero())
        {
            for (int i = 0; i < lastSetNumChIn; ++i)
                encoderList->sphereElementArray[i]->setActive (processor.soloMask[i]);
        }
        else
        {
            for (int i = 0; i < lastSetNumChIn; ++i)
                encoderList->sphereElementArray[i]->setActive (! processor.muteMask[i]);
        }
        processor.soloMuteChanged = false;
        sphere.repaint();
    }

    if (processor.updateColours)
    {
        processor.updateColours = false;
        encoderList->updateColours();
        sphere.repaint();
    }

    if (tbAnalyzeRMS.getToggleState())
        sphere.repaint();

    if (processor.updateSphere)
    {
        processor.updateSphere = false;
        sphere.repaint();
    }
    
    if (processor.manualSample)
        mLoadButton.setButtonText("File Loaded");
    else
        mLoadButton.setButtonText("Manual File Loader");
}

void DirectionalSamplerAudioProcessorEditor::mouseWheelOnSpherePannerMoved (
    SpherePanner* sphere,
    const juce::MouseEvent& event,
    const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isCommandDown() && event.mods.isAltDown())
        slMasterRoll.mouseWheelMove (event, wheel);
    else if (event.mods.isAltDown())
        slMasterElevation.mouseWheelMove (event, wheel);
    else if (event.mods.isCommandDown())
        slMasterAzimuth.mouseWheelMove (event, wheel);
}

void DirectionalSamplerAudioProcessorEditor::resized()
{
    const int margin = 30;
    const int headerHeight = 60;

    juce::Rectangle<int> area (getLocalBounds());
    juce::Rectangle<int> footerArea (area.removeFromBottom (margin));
    
    area.removeFromLeft (margin);
    area.removeFromRight (margin);
    juce::Rectangle<int> headerArea = area.removeFromTop (headerHeight);
    juce::Rectangle<int> enable = headerArea.removeFromRight (40);
    juce::Rectangle<int> loader = headerArea.removeFromRight(100);
    
    enable.removeFromTop(10); enable.removeFromBottom(10); enable.removeFromLeft(10); enable.removeFromRight(10);
    playButton.setBounds(enable);
    
    loader.removeFromLeft(20);
    loader.removeFromBottom(10);loader.removeFromTop(10);
    mLoadButton.setBounds(loader);
    
    title.setBounds (headerArea);
    area.removeFromTop (10);
    area.removeFromBottom (5);


    // ============== SIDEBAR RIGHT ====================
    // =================================================
    juce::Rectangle<int> sideBarArea (area.removeFromRight (390));
    const int rotSliderSpacing = 10;
    const int rotSliderWidth = 42;

    // -------------- Azimuth Elevation Roll juce::Labels ------------------
    juce::Rectangle<int> yprArea (sideBarArea);
    encoderGroup.setBounds (yprArea);
    auto headlineArea = yprArea.removeFromTop (25); //for box headline
    tbImport.setBounds (headlineArea.removeFromRight (60).removeFromTop (15));

    //Sources
    juce::Rectangle<int> sourceRow;
    sourceRow = yprArea.removeFromTop (15);
    lbNum.setBounds (sourceRow.removeFromLeft (15));
    sourceRow.removeFromLeft (15);
    lbPitch->setBounds (sourceRow.removeFromLeft (rotSliderWidth + 8));
    sourceRow.removeFromLeft (rotSliderSpacing - 7);
    lbRepF->setBounds (sourceRow.removeFromLeft (rotSliderWidth + 8));
    sourceRow.removeFromLeft (rotSliderSpacing - 7);
    lbLpf->setBounds (sourceRow.removeFromLeft (rotSliderWidth + 8));
    sourceRow.removeFromLeft (rotSliderSpacing - 8);
    lbAzimuth->setBounds (sourceRow.removeFromLeft (rotSliderWidth + 8));
    sourceRow.removeFromLeft (rotSliderSpacing - 8);
    lbElevation->setBounds (sourceRow.removeFromLeft (rotSliderWidth + 8));
    sourceRow.removeFromLeft (rotSliderSpacing - 8);
    lbGain->setBounds (sourceRow.removeFromLeft (rotSliderWidth));

    viewport.setBounds (yprArea);

    juce::Rectangle<int> hpArea (footerArea.removeFromLeft(footerArea.getWidth()/3));
    juce::Rectangle<int> sampleArea (footerArea.removeFromLeft(hpArea.getWidth()));
    hpArea.removeFromLeft(margin);
    hpArea.removeFromRight(margin/2);
    hpArea.removeFromBottom(5);
    hpArea.removeFromTop(5);
    sampleArea.removeFromLeft(margin/2);
    sampleArea.removeFromRight(margin);
    sampleArea.removeFromBottom(5);
    sampleArea.removeFromTop(5);
    footer.setBounds (footerArea);
    
    // ============== SIDEBAR LEFT ====================
    const int controlAreaHeight = 266;
    area.removeFromRight (10); // spacing

    juce::Rectangle<int> sphereArea (area);
    sphereArea.removeFromBottom (controlAreaHeight);

    if ((float) sphereArea.getWidth() / sphereArea.getHeight() > 1)
        sphereArea.setWidth (sphereArea.getHeight());
    else
        sphereArea.setHeight (sphereArea.getWidth());
    
    sphere.setBounds (sphereArea);
    area.removeFromTop (sphereArea.getHeight());

    // ============= Settings =========================
    auto controls = area.removeFromTop (controlAreaHeight);

    // ------------- Master Grabber ------------------------
    juce::Rectangle<int> masterArea (controls.removeFromTop(113));
    juce::Rectangle<int> rmsArea = masterArea.removeFromRight (90);
    juce::Rectangle<int> samplerAreaA (controls.removeFromTop(133));
    
    masterGroup.setBounds (masterArea);
    juce::Rectangle<int> masterRow (masterArea.removeFromTop (25)); //for box headline

    masterRow = masterArea.removeFromTop (55);
    slMasterAzimuth.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing);
    slMasterElevation.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing);
    slMasterRoll.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing*3);
    slMasterGain.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing);

    masterRow = (masterArea.removeFromTop (15));
    lbMasterAzimuth.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing - 5);
    lbMasterElevation.setBounds (masterRow.removeFromLeft (rotSliderWidth + 10));
    masterRow.removeFromLeft (rotSliderSpacing - 5);
    lbMasterRoll.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing*3);
    lbMasterGain.setBounds (masterRow.removeFromLeft (rotSliderWidth));
    masterRow.removeFromLeft (rotSliderSpacing);

    masterArea.removeFromTop (3);
    masterRow = masterArea;
    tbLockedToMaster.setBounds (masterRow);
    
    //Sampler Row A
    samplerAreaA.removeFromTop(20);
    juce::Rectangle<int> samplerHeader = samplerAreaA.removeFromTop (25);
    samplerGlobalsGroup.setBounds (samplerHeader);
    juce::Rectangle<int> togglesArea (samplerHeader.removeFromRight(150));
    
    juce::Rectangle<int> samplerARow = samplerAreaA.removeFromTop(55);
    pitchRangeSlider.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing);
    lpfMinSlider.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing);
    gainMinSlider.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing*3);
    repFMinSlider.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing);
    repFMaxSlider.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing);
    
    gainOffsetSlider.setBounds (samplerARow.removeFromRight (rotSliderWidth));
    samplerARow.removeFromRight (rotSliderSpacing);
    duckingAttenSlider.setBounds (samplerARow.removeFromRight (rotSliderWidth));
        
    samplerARow = (samplerAreaA.removeFromTop (15));
    lbPitchRange.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing - 5);
    lbLpfMin.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing+5);
    lbGainMin.setBounds (samplerARow.removeFromLeft (rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing*2);
    lbRepFMin.setBounds (samplerARow.removeFromLeft (rotSliderWidth + 10));
    samplerARow.removeFromLeft (rotSliderSpacing - 5);
    lbrepFMax.setBounds (samplerARow.removeFromLeft (rotSliderWidth + 10));
    
    lbGainOffset.setBounds (samplerARow.removeFromRight (rotSliderWidth + 5));
    samplerARow.removeFromRight (rotSliderSpacing - 5);
    lbDuckingAtten.setBounds (samplerARow.removeFromRight (rotSliderWidth));
    
    samplerAreaA.removeFromTop (3);
    samplerARow = samplerAreaA;
    samplerARow.removeFromLeft(8);
    pitchActiveButton.setBounds(samplerARow.removeFromLeft(rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing);
    lpfActiveButton.setBounds(samplerARow.removeFromLeft(rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing);
    gainActiveButton.setBounds(samplerARow.removeFromLeft(rotSliderWidth));
    samplerARow.removeFromLeft (rotSliderSpacing*3);
    
    int doubleUnit = rotSliderWidth*2+rotSliderSpacing;
    seamlessButton.setBounds(samplerARow.removeFromLeft(doubleUnit));
    
    duckingActiveButton.setBounds(samplerARow.removeFromRight(doubleUnit-5));
    
    //Toggles
    const int labelSpacer = 5;
    const int labelHeight = 15;
    
    //togglesArea.removeFromTop(5);
    togglesArea.removeFromBottom(10);
    guideActiveButton.setBounds(togglesArea.removeFromRight(50));
    
    // ------------- RMS ------------------------
    controls.removeFromLeft (5);
    rmsGroup.setBounds (rmsArea);
    rmsArea.removeFromTop (25); //for box headline

    juce::Rectangle<int> rmsRow (rmsArea.removeFromTop (55));
    slPeakLevel.setBounds (rmsRow.removeFromLeft (rotSliderWidth));
    rmsRow.removeFromLeft (rotSliderSpacing);
    slDynamicRange.setBounds (rmsRow.removeFromLeft (rotSliderWidth));

    rmsRow = rmsArea.removeFromTop (15);
    lbPeakLevel.setBounds (rmsRow.removeFromLeft (rotSliderWidth));
    rmsRow.removeFromLeft (rotSliderSpacing);
    lbDynamicRange.setBounds (rmsRow.removeFromLeft (rotSliderWidth));

    rmsArea.removeFromTop (3);
    rmsRow = rmsArea;
    tbAnalyzeRMS.setBounds (rmsRow);
    
    rmsRow = rmsArea.removeFromTop (20);
    cbEq.setBounds (hpArea);
    cbSamples.setBounds (sampleArea);
}

void DirectionalSamplerAudioProcessorEditor::importLayout()
{
    juce::FileChooser myChooser (
        "Load configuration...",
        processor.getLastDir().exists()
            ? processor.getLastDir()
            : juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.json");
    if (myChooser.browseForFileToOpen())
    {
        juce::File configFile (myChooser.getResult());
        processor.setLastDir (configFile.getParentDirectory());
        auto result = processor.loadConfiguration (configFile);

        if (! result.wasOk())
        {
            auto component = std::make_unique<juce::TextEditor>();
            component->setMultiLine (true, true);
            component->setReadOnly (true);
            component->setText (result.getErrorMessage());
            component->setSize (200, 110);

            auto& myBox = juce::CallOutBox::launchAsynchronously (std::move (component),
                                                                  tbImport.getBounds(),
                                                                  this);
            myBox.setLookAndFeel (&getLookAndFeel());
        }
    }
}
