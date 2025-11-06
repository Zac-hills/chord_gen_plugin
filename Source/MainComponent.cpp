#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : keyboard(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    keyComboBox.addItem("C", 1);
    keyComboBox.addItem("C#", 2);
    keyComboBox.addItem("D", 3);
    keyComboBox.addItem("D#", 4);
    keyComboBox.addItem("E", 5);
    keyComboBox.addItem("F", 6);
    keyComboBox.addItem("F#", 7);
    keyComboBox.addItem("G", 8);
    keyComboBox.addItem("G#", 9);
    keyComboBox.addItem("A", 10);
    keyComboBox.addItem("A#", 11);
    keyComboBox.addItem("B", 12);
    
    keyComboBox.setSelectedId(1);
    keyComboBox.onChange = [this] { keySelectionChanged(); };
    addAndMakeVisible(keyComboBox);
    
    // Setup progressions dropdown (dummy)
    progressionsDropdown.addItem("Progressions", 1);
    progressionsDropdown.setSelectedId(1);
    addAndMakeVisible(progressionsDropdown);
    
    // Setup chord progression builder
    progressionBuilderLabel.setText("Build Your Progression:", juce::dontSendNotification);
    progressionBuilderLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(progressionBuilderLabel);
    
    // Setup chord buttons for each scale degree
    const juce::StringArray romanNumerals = { "I", "II", "III", "IV", "V", "VI", "VII" };
    for (int i = 0; i < 7; ++i)
    {
        chordButtons[i].setButtonText(romanNumerals[i]);
        chordButtons[i].onClick = [this, i] { addChordToProgression(i + 1); };
        addAndMakeVisible(chordButtons[i]);
    }
    
    // Apply circular LookAndFeel to the root button (I)
    chordButtons[0].setLookAndFeel(&circularButtonLookAndFeel);
    
    clearProgressionButton.setButtonText("Clear");
    clearProgressionButton.onClick = [this] { clearCustomProgression(); };
    addAndMakeVisible(clearProgressionButton);
    
    removeLastChordButton.setButtonText("Remove Last");
    removeLastChordButton.onClick = [this] { removeLastChordFromProgression(); };
    addAndMakeVisible(removeLastChordButton);
    
    customProgressionDisplayLabel.setText("Progression: (empty)", juce::dontSendNotification);
    customProgressionDisplayLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(customProgressionDisplayLabel);
    
    // Add chord type combo box
    // chordTypeComboBox.addItem("Triads", 1);
    // chordTypeComboBox.addItem("Seventh Chords", 2);
    // chordTypeComboBox.setSelectedId(1);
    // chordTypeComboBox.onChange = [this] { 
    //     updateDisplay();
    //     // If currently playing, reload the progression with new chord type
    //     if (isPlaying)
    //     {
    //         playProgression();
    //     }
    // };
    // addAndMakeVisible(chordTypeComboBox);
    
    // Add time signature combo box
    timeSignatureComboBox.addItem("4/4", 1);
    timeSignatureComboBox.addItem("3/4", 2);
    timeSignatureComboBox.addItem("6/8", 3);
    timeSignatureComboBox.addItem("5/4", 4);
    timeSignatureComboBox.addItem("7/8", 5);
    timeSignatureComboBox.addItem("2/4", 6);
    timeSignatureComboBox.setSelectedId(1);
    timeSignatureComboBox.onChange = [this] { 
        updateTimeSignature();
        if (isPlaying)
        {
            updateChordDuration();
        }
    };
    addAndMakeVisible(timeSignatureComboBox);
    
    timeSignatureLabel.setText("Time Signature:", juce::dontSendNotification);
    addAndMakeVisible(timeSignatureLabel);
    
    // Add voicing combo box
   // voicingComboBox.addItem("Close Position", 1);
   // voicingComboBox.addItem("Open Position", 2);
   // voicingComboBox.addItem("Drop 2", 3);
   // voicingComboBox.addItem("Drop 3", 4);
   // voicingComboBox.addItem("1st Inversion", 5);
   // voicingComboBox.addItem("2nd Inversion", 6);
   // voicingComboBox.addItem("Spread", 7);
   // voicingComboBox.setSelectedId(1);
   // voicingComboBox.onChange = [this] { 
   //     if (isPlaying)
   //     {
   //         playProgression();
   //     }
   // };
   // addAndMakeVisible(voicingComboBox);
    
    voicingLabel.setText("Voicing:", juce::dontSendNotification);
    addAndMakeVisible(voicingLabel);
    
    // Add waveform combo box
    waveformComboBox.addItem("Sine Wave", 1);
    waveformComboBox.addItem("Sawtooth", 2);
    waveformComboBox.addItem("Square Wave", 3);
    waveformComboBox.addItem("Triangle Wave", 4);
    waveformComboBox.setSelectedId(1);
    waveformComboBox.onChange = [this] { updateWaveform(); };
    addAndMakeVisible(waveformComboBox);
    
    waveformLabel.setText("Waveform:", juce::dontSendNotification);
    addAndMakeVisible(waveformLabel);
    
    progressionLabel.setText("Progression: ", juce::dontSendNotification);
    progressionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(progressionLabel);
    
    // Initialize MIDI components
    keyboard.setKeyWidth(40.0f);
    addAndMakeVisible(keyboard);
    
    playStopButton.setButtonText("Play");
    playStopButton.onClick = [this] { 
        if (isPlaying) {
            stopProgression();
        } else {
            playProgression();
        }
        // Update button text based on current state
        playStopButton.setButtonText(isPlaying ? "Stop" : "Play");
    };
    addAndMakeVisible(playStopButton);
    
    loopButton.setButtonText("Loop");
    loopButton.setToggleState(false, juce::dontSendNotification);
    loopButton.onClick = [this] { shouldLoop = loopButton.getToggleState(); };
    addAndMakeVisible(loopButton);
    
    tempoSlider.setRange(60.0, 200.0);
    tempoSlider.setValue(120.0);
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(tempoSlider);
    
    tempoLabel.setText("Tempo (BPM):", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);
    
    // Setup Emotion Wheel UI
    chordSelectorLabel.setText("Select Chord:", juce::dontSendNotification);
    addAndMakeVisible(chordSelectorLabel);
    
    chordSelectorComboBox.onChange = [this] { updateEmotionComboBox(); };
    addAndMakeVisible(chordSelectorComboBox);
    
    emotionLabel.setText("Apply Emotion:", juce::dontSendNotification);
    addAndMakeVisible(emotionLabel);
    
    emotionComboBox.onChange = [this] { updateEmotionDescription(); };
    addAndMakeVisible(emotionComboBox);
    
    emotionDescriptionLabel.setText("", juce::dontSendNotification);
    emotionDescriptionLabel.setFont(juce::Font(12.0f, juce::Font::italic));
    emotionDescriptionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(emotionDescriptionLabel);
    
    applyEmotionButton.setButtonText("Apply");
    applyEmotionButton.onClick = [this] { applyEmotionToChord(); };
    addAndMakeVisible(applyEmotionButton);
    
    emotionWheelGroup.setText("Emotion Wheel");
    emotionWheelGroup.setTextLabelPosition(juce::Justification::centredTop);
    addAndMakeVisible(emotionWheelGroup);
    
    // Setup group components
    progressionBuilderGroup.setText("Chord Progression Builder");
    progressionBuilderGroup.setTextLabelPosition(juce::Justification::centredTop);
    addAndMakeVisible(progressionBuilderGroup);
    
    refinementGroup.setText("Refinement");
    refinementGroup.setTextLabelPosition(juce::Justification::centredTop);
    addAndMakeVisible(refinementGroup);

    
    // Audio settings button in title bar
    audioSettingsButton.setButtonText("...");  // Three dots for settings menu
    audioSettingsButton.onClick = [this] { showAudioSettings(); };
    addAndMakeVisible(audioSettingsButton);
    
    // Initialize playback state
    isPlaying = false;
    shouldLoop = false;
    currentChordIndex = 0;
    samplesUntilNextChord = 0;
    beatsPerMeasure = 4;
    beatUnit = 4;
    
    // Detect system audio devices first
    detectSystemAudioDevices();
    
    // Try to initialize audio device
    tryInitializeAudioDevice();
    
    // Setup synthesizer with sine wave voices
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new SineWaveVoice());
    
    // Add a simple sine wave sound for all notes
    synth.addSound(new SineWaveSound());
    
    updateDisplay();
    updateChordButtonLabels();  // Initialize chord button labels with notes
    setSize(800, 700);
    
    // Initialize audio with 0 input channels and 2 output channels
    setAudioChannels(0, 2);
    
    // Debug audio device information
    auto* audioDeviceManager = &deviceManager;
    
    // List all available audio device types
    auto& audioDeviceTypes = audioDeviceManager->getAvailableDeviceTypes();
    DBG("Available audio device types:");
    for (int i = 0; i < audioDeviceTypes.size(); ++i)
    {
        auto* deviceType = audioDeviceTypes[i];
        DBG("  Type " << i << ": " << deviceType->getTypeName());
        
        deviceType->scanForDevices();
        auto deviceNames = deviceType->getDeviceNames();
        for (int j = 0; j < deviceNames.size(); ++j)
        {
            DBG("    Device " << j << ": " << deviceNames[j]);
        }
    }
    
    auto currentAudioDevice = audioDeviceManager->getCurrentAudioDevice();
    if (currentAudioDevice != nullptr)
    {
        DBG("Current audio device: " << currentAudioDevice->getName());
        DBG("Output channels: " << currentAudioDevice->getOutputChannelNames().size());
        DBG("Sample rate: " << currentAudioDevice->getCurrentSampleRate());
    }
    else
    {
        DBG("No audio device found! Trying to initialize default device...");
        
        // Try to initialize with ALSA first, then JACK
        juce::String error;
        for (auto* deviceType : audioDeviceTypes)
        {
            if (deviceType->getTypeName() == "ALSA" || deviceType->getTypeName() == "JACK")
            {
                deviceType->scanForDevices();
                auto deviceNames = deviceType->getDeviceNames();
                if (!deviceNames.isEmpty())
                {
                    auto setup = audioDeviceManager->getAudioDeviceSetup();
                    setup.outputDeviceName = deviceNames[0];
                    setup.inputDeviceName = juce::String();
                    error = audioDeviceManager->setAudioDeviceSetup(setup, true);
                    if (error.isEmpty())
                    {
                        DBG("Successfully initialized: " << deviceNames[0]);
                        break;
                    }
                }
            }
        }
        
        if (!error.isEmpty())
        {
            DBG("Audio device setup error: " << error);
        }
    }
    themeManager.setTheme(ThemeManager::Theme::Default);  // Temporarily disabled
    applyTheme();  // Temporarily disabled
}

MainComponent::~MainComponent()
{
    // Reset LookAndFeel to avoid dangling pointer
    chordButtons[0].setLookAndFeel(nullptr);
    shutdownAudio();
}

// Temporarily disabled - ThemeManager not yet in build
void MainComponent::applyTheme() {
    const auto& colors = themeManager.getColors();

    // Set background
    getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, colors.backgroundMain);

    // Loop through all child components
    for (auto* child : getChildren())
    {
        // Apply to ComboBoxes
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(child))
        {
            comboBox->setColour(juce::ComboBox::backgroundColourId, colors.comboBoxBackground);
            comboBox->setColour(juce::ComboBox::textColourId, colors.comboBoxText);
            comboBox->setColour(juce::ComboBox::outlineColourId, colors.comboBoxOutline);
            comboBox->setColour(juce::ComboBox::arrowColourId, colors.comboBoxText);
        }
        
        // Apply to TextButtons
        else if (auto* button = dynamic_cast<juce::TextButton*>(child))
        {
            button->setColour(juce::TextButton::buttonColourId, colors.buttonBackground);
            button->setColour(juce::TextButton::textColourOffId, colors.buttonText);
            button->setColour(juce::TextButton::textColourOnId, colors.buttonText);
            button->setColour(juce::TextButton::buttonOnColourId, colors.buttonHighlight);
        }
        
        // Apply to ToggleButtons
        else if (auto* toggleButton = dynamic_cast<juce::ToggleButton*>(child))
        {
            toggleButton->setColour(juce::ToggleButton::textColourId, colors.textPrimary);
            toggleButton->setColour(juce::ToggleButton::tickColourId, colors.accentPrimary);
            toggleButton->setColour(juce::ToggleButton::tickDisabledColourId, colors.textSecondary);
        }
        
        // Apply to Labels
        else if (auto* label = dynamic_cast<juce::Label*>(child))
        {
            label->setColour(juce::Label::textColourId, colors.labelText);
            label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        }
        
        // Apply to Sliders
        else if (auto* slider = dynamic_cast<juce::Slider*>(child))
        {
            slider->setColour(juce::Slider::thumbColourId, colors.accentPrimary);
            slider->setColour(juce::Slider::trackColourId, colors.accentSecondary);
            slider->setColour(juce::Slider::backgroundColourId, colors.backgroundControl);
            slider->setColour(juce::Slider::textBoxTextColourId, colors.textPrimary);
            slider->setColour(juce::Slider::textBoxBackgroundColourId, colors.backgroundControl);
            slider->setColour(juce::Slider::textBoxOutlineColourId, colors.border);
        }
        
        // Apply to MidiKeyboardComponent
        else if (auto* keyboard = dynamic_cast<juce::MidiKeyboardComponent*>(child))
        {
            keyboard->setColour(juce::MidiKeyboardComponent::whiteNoteColourId, colors.backgroundMain);
            keyboard->setColour(juce::MidiKeyboardComponent::blackNoteColourId, colors.backgroundSecondary);
            keyboard->setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, colors.border);
            keyboard->setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colors.accentPrimary.withAlpha(0.3f));
            keyboard->setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, colors.accentPrimary.withAlpha(0.6f));
        }
        
        // Apply to GroupComponent
        else if (auto* group = dynamic_cast<juce::GroupComponent*>(child))
        {
            group->setColour(juce::GroupComponent::textColourId, colors.textPrimary);
            group->setColour(juce::GroupComponent::outlineColourId, colors.border);
        }
    }
    
    repaint();
}


//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.
    
    // Debug output
    DBG("Audio prepared - Sample Rate: " << sampleRate << ", Block Size: " << samplesPerBlockExpected);
    
    synth.setCurrentPlaybackSampleRate(sampleRate);
    this->sampleRate = sampleRate;
    
    // Calculate samples per beat based on initial tempo
    samplesPerBeat = static_cast<int>((60.0 / 120.0) * sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Clear the buffer first
    bufferToFill.clearActiveBufferRegion();
    
    // Handle MIDI playback timing
    if (isPlaying && currentProgression.size() > 0)
    {
        auto numSamples = bufferToFill.numSamples;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (samplesUntilNextChord <= 0)
            {
                // Stop current chord
                stopCurrentChord();
                
                // Play next chord if available
                if (currentChordIndex < currentProgression.size())
                {
                    playChord(currentProgression[currentChordIndex]);
                    currentChordIndex++;
                    
                    // Update timing based on time signature and tempo
                    updateChordDuration();
                    samplesUntilNextChord = samplesPerBeat;
                }
                else
                {
                    // End of progression
                    if (shouldLoop)
                    {
                        // Loop back to the beginning
                        currentChordIndex = 0;
                        stopCurrentChord();
                    }
                    else
                    {
                        stopProgression();
                    }
                }
            }
            else
            {
                samplesUntilNextChord--;
            }
        }
    }
    
    // Process MIDI keyboard input
    juce::MidiBuffer incomingMidi;
    keyboardState.processNextMidiBuffer(incomingMidi, 0, bufferToFill.numSamples, true);
    
    // Debug MIDI messages
    if (!incomingMidi.isEmpty())
    {
        DBG("MIDI messages received: " << incomingMidi.getNumEvents());
    }
    
    // Add any progression playback MIDI messages here if needed
    
    // Render synthesizer audio
    synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, 0, bufferToFill.numSamples);
    
    // Check if we're generating any audio
    auto magnitude = bufferToFill.buffer->getMagnitude(0, bufferToFill.numSamples);
    if (magnitude > 0.001f)
    {
        DBG("Audio magnitude: " << magnitude);
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.
    stopProgression();
}

void MainComponent::paint(juce::Graphics& g)
{
    const auto& colors = themeManager.getColors();
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw custom title bar
    auto titleBarArea = getLocalBounds().removeFromTop(30);
    g.setColour(colors.textPrimary);
    g.fillRect(titleBarArea);
    
    // Draw title text in title bar
    g.setColour(colors.textSecondary);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("Chord Builder", titleBarArea.reduced(10, 0), 
               juce::Justification::centredLeft, true);
    
    // Draw border around window
    g.setColour(colors.border);
    g.drawRect(getLocalBounds(), 4);
    
    // Load and draw logo (below title bar)
   //auto logoFile = juce::File::getCurrentWorkingDirectory()
   //                    .getChildFile("assets")
   //                    .getChildFile("logo.jpg");
        
    //if (logoFile.existsAsFile())
    //{
    //    auto logoImage = juce::ImageCache::getFromFile(logoFile);
    //    if (logoImage.isValid())
    //    {
    //        auto logoBounds = getLocalBounds().removeFromTop(60).removeFromRight(100).reduced(5);
    //        g.drawImage(logoImage, logoBounds.toFloat(), juce::RectanglePlacement::centred);
    //    }
    //}
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Audio settings button in title bar (top right) - use absolute positioning
    auto settingsButtonBounds = juce::Rectangle<int>(getWidth() - 45, 5, 35, 20);
    audioSettingsButton.setBounds(settingsButtonBounds);
    
    // Title bar area
    bounds.removeFromTop(30);  // Title bar height
    
    // Add some spacing after title bar
    bounds.removeFromTop(30);
    
    // Add outer padding to prevent borders from touching window edges
    bounds.reduce(10, 0);
    
    // Create two columns: left for builder, right for refinement
    auto leftColumn = bounds.removeFromLeft(getWidth() / 2 - 10);  // Account for outer padding
    leftColumn.removeFromRight(5);  // Gap between columns
    auto rightColumn = bounds;
    rightColumn.removeFromLeft(5);  // Gap between columns
    
    // Left column: Progression Builder Group
    auto builderGroupBounds = leftColumn.removeFromTop(350);
    progressionBuilderGroup.setBounds(builderGroupBounds);
    
    auto builderContent = builderGroupBounds.reduced(15, 25);  // Reduce for group border and title
    
    // Key and Progressions at the top of builder group - far left and far right
    auto keyProgressionsRow = builderContent.removeFromTop(35);
    keyComboBox.setBounds(keyProgressionsRow.removeFromLeft(150).reduced(5));
    keyProgressionsRow.removeFromRight(0);  // Skip middle space
    progressionsDropdown.setBounds(keyProgressionsRow.removeFromRight(150).reduced(5));
    
    builderContent.removeFromTop(20);  // Increased spacing
    
    // Chord buttons in circular layout with root (I) in center
    auto chordButtonArea = builderContent.removeFromTop(150);
    int centerX = chordButtonArea.getCentreX();
    int centerY = chordButtonArea.getCentreY();
    int radius = 70;  // Distance from center
    int buttonSize = 60;  // Button diameter
    
    // Position root note (I) in the center
    chordButtons[0].setBounds(centerX - buttonSize/2, centerY - buttonSize/2, buttonSize, buttonSize);
    
    // Position other 6 buttons in a circle around the center
    for (int i = 1; i < 7; ++i)
    {
        // Calculate angle for each button (starting at top, going clockwise)
        // Offset by -90 degrees to start at top
        double angle = juce::MathConstants<double>::pi * 2.0 * (i - 1) / 6.0 - juce::MathConstants<double>::pi / 2.0;
        
        int x = centerX + static_cast<int>(radius * std::cos(angle)) - buttonSize/2;
        int y = centerY + static_cast<int>(radius * std::sin(angle)) - buttonSize/2;
        
        chordButtons[i].setBounds(x, y, buttonSize, buttonSize);
    }
    
    builderContent.removeFromTop(20);  // Increased spacing
    
    // Control buttons for progression - far left and far right
    auto progressionControlRow = builderContent.removeFromTop(35);
    clearProgressionButton.setBounds(progressionControlRow.removeFromLeft(100).reduced(5));
    progressionControlRow.removeFromRight(0);  // Skip middle space
    removeLastChordButton.setBounds(progressionControlRow.removeFromRight(120).reduced(5));
    
    builderContent.removeFromTop(10);
    
    // Custom progression display (detailed chord names)
    progressionLabel.setBounds(builderContent.removeFromTop(30).reduced(5));
    
    // Right column: Refinement Group
    auto refinementGroupBounds = rightColumn.removeFromTop(350);
    refinementGroup.setBounds(refinementGroupBounds);
    
    auto refinementContent = refinementGroupBounds.reduced(15, 25);  // Reduce for group border and title
    
    // Stack components vertically in refinement group
    // Tempo
    auto tempoRow = refinementContent.removeFromTop(55);
    tempoLabel.setBounds(tempoRow.removeFromTop(20).reduced(5, 0));
    tempoSlider.setBounds(tempoRow.reduced(5, 0));
    
    refinementContent.removeFromTop(5);
    
    // Time Signature
    auto timeSignatureRow = refinementContent.removeFromTop(55);
    timeSignatureLabel.setBounds(timeSignatureRow.removeFromTop(20).reduced(5, 0));
    timeSignatureComboBox.setBounds(timeSignatureRow.reduced(5, 0));
    
    refinementContent.removeFromTop(5);
    
    // Voicing
    auto voicingRow = refinementContent.removeFromTop(55);
    voicingLabel.setBounds(voicingRow.removeFromTop(20).reduced(5, 0));
    voicingComboBox.setBounds(voicingRow.reduced(5, 0));
    
    refinementContent.removeFromTop(5);
    
    // Waveform
    auto waveformRow = refinementContent.removeFromTop(55);
    waveformLabel.setBounds(waveformRow.removeFromTop(20).reduced(5, 0));
    waveformComboBox.setBounds(waveformRow.reduced(5, 0));
    
    // Back to full width for remaining components
    bounds = getLocalBounds();
    bounds.removeFromTop(30 + 30 + 350 + 20);  // Skip title, spacing, groups, and gap
    bounds.reduce(10, 0);  // Add outer padding
    
    // Emotion Wheel section
    auto emotionWheelSection = bounds.removeFromTop(140);  // Increased from 120 to 140
    emotionWheelGroup.setBounds(emotionWheelSection);
    
    auto emotionContent = emotionWheelSection.reduced(15, 25);  // Reduce for group border and title
    
    // First row: Chord selector
    auto chordSelectorRow = emotionContent.removeFromTop(30);
    chordSelectorLabel.setBounds(chordSelectorRow.removeFromLeft(100));
    chordSelectorComboBox.setBounds(chordSelectorRow.removeFromLeft(200).reduced(5, 0));
    
    emotionContent.removeFromTop(5);  // Padding below chord selector
    
    // Second row: Emotion selector
    auto emotionRow = emotionContent.removeFromTop(30);
    emotionLabel.setBounds(emotionRow.removeFromLeft(100));
    emotionComboBox.setBounds(emotionRow.removeFromLeft(300).reduced(5, 0));
    applyEmotionButton.setBounds(emotionRow.removeFromLeft(100).reduced(10, 0));
    
    // Third row: Description (increased height)
    emotionDescriptionLabel.setBounds(emotionContent.removeFromTop(40));
    
    bounds.removeFromTop(20);
    
    // MIDI keyboard at the bottom with play buttons to the right
    auto keyboardArea = bounds.removeFromBottom(160);  // Doubled from 80 to 160
    
    // Reserve space on the right for play controls
    auto playControlArea = keyboardArea.removeFromRight(180);
    
    // Position play/stop and loop buttons vertically stacked on the right
    playControlArea.removeFromTop(10);  // Top padding
    playStopButton.setBounds(playControlArea.removeFromTop(30).reduced(10, 0));
    loopButton.setBounds(playControlArea.removeFromTop(30).reduced(10, 0));
    
    // Keyboard takes the remaining space
    keyboard.setBounds(keyboardArea.withTrimmedLeft(50)    // Left padding
                               .withTrimmedRight(20)       // Small right padding
                               .withTrimmedTop(5)          // Top padding
                               .withTrimmedBottom(50));    // Bottom padding
}

void MainComponent::keySelectionChanged()
{
    int selectedKey = keyComboBox.getSelectedId() - 1;
    keyManager.setCurrentKey(static_cast<KeyManager::Key>(selectedKey));
    updateDisplay();
    updateChordButtonLabels();  // Update button labels when key changes
    
    // If currently playing, reload the progression with the new key
    if (isPlaying)
    {
        playProgression();
    }
}

void MainComponent::progressionSelectionChanged()
{
    updateDisplay();
    
    // If currently playing, reload the progression
    if (isPlaying)
    {
        playProgression();
    }
}

void MainComponent::updateDisplay()
{
    // Update custom progression display
    updateCustomProgressionDisplay();
}

void MainComponent::updateTimeSignature()
{
    int selectedId = timeSignatureComboBox.getSelectedId();
    switch (selectedId)
    {
        case 1: // 4/4
            beatsPerMeasure = 4;
            beatUnit = 4;
            break;
        case 2: // 3/4
            beatsPerMeasure = 3;
            beatUnit = 4;
            break;
        case 3: // 6/8
            beatsPerMeasure = 6;
            beatUnit = 8;
            break;
        case 4: // 5/4
            beatsPerMeasure = 5;
            beatUnit = 4;
            break;
        case 5: // 7/8
            beatsPerMeasure = 7;
            beatUnit = 8;
            break;
        case 6: // 2/4
            beatsPerMeasure = 2;
            beatUnit = 4;
            break;
        default:
            beatsPerMeasure = 4;
            beatUnit = 4;
            break;
    }
}

void MainComponent::updateChordDuration()
{
    // Update samples per beat based on the beat unit
    double currentTempo = tempoSlider.getValue();
    
    // Calculate samples per beat (quarter note)
    double quarterNoteDuration = 60.0 / currentTempo;
    
    // Adjust for beat unit (e.g., 8th notes are half a quarter note)
    double beatDuration = quarterNoteDuration * (4.0 / beatUnit);
    
    // Each chord lasts for the full measure
    samplesPerBeat = static_cast<int>(beatDuration * beatsPerMeasure * sampleRate);
}

void MainComponent::updateWaveform()
{
    WaveformType waveform = WaveformType::Sine;
    
    int selectedId = waveformComboBox.getSelectedId();
    switch (selectedId)
    {
        case 1: waveform = WaveformType::Sine; break;
        case 2: waveform = WaveformType::Sawtooth; break;
        case 3: waveform = WaveformType::Square; break;
        case 4: waveform = WaveformType::Triangle; break;
        default: waveform = WaveformType::Sine; break;
    }
    
    // Update all synth voices with the new waveform
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SineWaveVoice*>(synth.getVoice(i)))
        {
            voice->setWaveform(waveform);
        }
    }
}

//==============================================================================
// MIDI Playback Methods

void MainComponent::playProgression()
{
    // Play custom progression if it exists
    if (!customProgressionDegrees.empty())
    {
        stopProgression(); // Stop any current playback
        
        bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
        
        // Get selected voicing
        KeyManager::Voicing voicing = KeyManager::Voicing::Close;
        int voicingId = voicingComboBox.getSelectedId();
        switch (voicingId)
        {
            case 1: voicing = KeyManager::Voicing::Close; break;
            case 2: voicing = KeyManager::Voicing::Open; break;
            case 3: voicing = KeyManager::Voicing::Drop2; break;
            case 4: voicing = KeyManager::Voicing::Drop3; break;
            case 5: voicing = KeyManager::Voicing::FirstInversion; break;
            case 6: voicing = KeyManager::Voicing::SecondInversion; break;
            case 7: voicing = KeyManager::Voicing::Spread; break;
            default: voicing = KeyManager::Voicing::Close; break;
        }
        
        // Build progression from scale degrees
        currentProgression.clear();
        for (int i = 0; i < customProgressionDegrees.size(); ++i)
        {
            int degree = customProgressionDegrees[i];
            auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
            
            // Generate chord based on type
            std::vector<int> chord;
            
            // Check if this chord has an emotion applied
            if (i < customProgressionEmotions.size())
            {
                auto emotion = customProgressionEmotions[i];
                
                // Get the root note for this scale degree
                auto scaleNotes = keyManager.getScaleNotes();
                if (degree - 1 < scaleNotes.size())
                {
                    // Scale notes are 0-11 (pitch classes), so add base octave (60 = middle C)
                    int rootNote = 60 + scaleNotes[degree - 1];
                    
                    // Apply emotion to get chord notes
                    chord = emotionWheel.applyEmotion(rootNote, emotion);
                }
                else
                {
                    // Fallback to regular chord generation
                    if (useSevenths)
                        chord = keyManager.generateSeventh(scaleDegree);
                    else
                        chord = keyManager.generateTriad(scaleDegree);
                }
            }
            else
            {
                // No emotion applied, use regular chord generation
                if (useSevenths)
                    chord = keyManager.generateSeventh(scaleDegree);
                else
                    chord = keyManager.generateTriad(scaleDegree);
            }
            
            // Apply voicing
            chord = keyManager.applyVoicing(chord, voicing);
            
            currentProgression.push_back(chord);
        }
        
        currentChordIndex = 0;
        isPlaying = true;
        
        // Calculate initial timing based on time signature
        updateChordDuration();
        samplesUntilNextChord = 0; // Start immediately
    }
}

void MainComponent::stopProgression()
{
    isPlaying = false;
    currentChordIndex = 0;
    currentProgression.clear();
    stopCurrentChord();
    
    // Update button text on the message thread (safe when called from audio thread)
    // If called from message thread (e.g., onClick), this will queue but the onClick
    // handler's synchronous update happens first, so no conflict
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<MainComponent>(this)]() {
        if (safeThis != nullptr && !safeThis->isPlaying)
            safeThis->playStopButton.setButtonText("Play");
    });
}

void MainComponent::playChord(const std::vector<int>& chord)
{
    if (chord.empty()) return;
    
    // Stop current notes
    stopCurrentChord();
    
    // Clear current chord tracking
    currentChordNotes.clear();
    
    // Play new chord notes using the keyboard state
    for (int note : chord)
    {
        if (note >= 0 && note < 128)
        {
            // Note is already a proper MIDI note number
            keyboardState.noteOn(1, note, 0.7f);
            currentChordNotes.push_back(note);
        }
    }
}

void MainComponent::stopCurrentChord()
{
    // Stop all currently playing chord notes using keyboard state
    for (int note : currentChordNotes)
    {
        keyboardState.noteOff(1, note, 0.0f);
    }
    currentChordNotes.clear();
}

void MainComponent::showAudioSettings()
{
    // Create and show audio device selector component
    auto audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        0, 0,  // min/max input channels
        0, 256,  // min/max output channels
        false, // show MIDI inputs
        false, // show MIDI outputs
        false, // show channels as stereo pairs
        false  // hide advanced options
    );
    
    audioSetupComp->setSize(500, 400);
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setNonOwned(audioSetupComp.get());
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour = juce::Colours::lightgrey;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    
    options.launchAsync();
    
    // Keep the component alive
    audioSetupComp.release();
}

void MainComponent::tryInitializeAudioDevice()
{
    DBG("=== Initializing Default Audio Device ===");
    
    // Simply use the system default audio output device
    auto error = deviceManager.initialise(0, 2, nullptr, true);
    
    if (error.isEmpty())
    {
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        if (currentDevice != nullptr)
        {
            DBG("Successfully initialized default audio device: " << currentDevice->getName());
            DBG("Sample rate: " << currentDevice->getCurrentSampleRate());
            DBG("Buffer size: " << currentDevice->getCurrentBufferSizeSamples());
            DBG("Output channels: " << currentDevice->getOutputChannelNames().size());
        }
        else
        {
            DBG("Audio device manager initialized but no current device found");
        }
    }
    else
    {
        DBG("Failed to initialize default audio device: " << error);
    }
}

void MainComponent::detectSystemAudioDevices()
{
    DBG("=== Available Audio Device Types ===");
    
    auto& audioDeviceTypes = deviceManager.getAvailableDeviceTypes();
    for (int i = 0; i < audioDeviceTypes.size(); ++i)
    {
        auto* deviceType = audioDeviceTypes[i];
        DBG("Type " << i << ": " << deviceType->getTypeName());
    }
}

//==============================================================================
// Chord Progression Builder Methods

void MainComponent::addChordToProgression(int scaleDegree)
{
    customProgressionDegrees.push_back(scaleDegree);
    updateCustomProgressionDisplay();
    updateChordSelector();  // Update emotion wheel UI
}

void MainComponent::clearCustomProgression()
{
    customProgressionDegrees.clear();
    customProgressionEmotions.clear();  // Clear emotions too
    updateCustomProgressionDisplay();
    updateChordSelector();  // Update emotion wheel UI
    
    // Stop playback if currently playing
    if (isPlaying)
    {
        stopProgression();
    }
}

void MainComponent::removeLastChordFromProgression()
{
    if (!customProgressionDegrees.empty())
    {
        customProgressionDegrees.pop_back();
        if (!customProgressionEmotions.empty())
            customProgressionEmotions.pop_back();
        updateCustomProgressionDisplay();
        updateChordSelector();  // Update emotion wheel UI
    }
}

void MainComponent::updateCustomProgressionDisplay()
{
    if (customProgressionDegrees.empty())
    {
        progressionLabel.setText("Progression: Build a progression above", juce::dontSendNotification);
        return;
    }
    
    // Build detailed progression label with chord names
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    juce::String detailedText = "Progression: ";
    
    for (int i = 0; i < customProgressionDegrees.size(); ++i)
    {
        int degree = customProgressionDegrees[i];
        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
        auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
        std::string chordName = keyManager.getChordName(scaleDegree, chordType);
        
        detailedText += juce::String(chordName);
        
        if (i < customProgressionDegrees.size() - 1)
            detailedText += " - ";
    }
    
    progressionLabel.setText(detailedText, juce::dontSendNotification);
}

void MainComponent::updateChordButtonLabels()
{
    const juce::StringArray romanNumerals = { "I", "II", "III", "IV", "V", "VI", "VII" };
    auto scaleNotes = keyManager.getScaleNoteNames();
    
    for (int i = 0; i < 7; ++i)
    {
        if (i < scaleNotes.size())
        {
            juce::String buttonText = romanNumerals[i] + "\n" + juce::String(scaleNotes[i]);
            chordButtons[i].setButtonText(buttonText);
        }
    }
}

//==============================================================================
// Emotion Wheel Methods

void MainComponent::updateChordSelector()
{
    chordSelectorComboBox.clear();
    
    if (customProgressionDegrees.empty())
    {
        chordSelectorComboBox.setEnabled(false);
        emotionComboBox.setEnabled(false);
        applyEmotionButton.setEnabled(false);
        emotionDescriptionLabel.setText("Build a progression first", juce::dontSendNotification);
        return;
    }
    
    chordSelectorComboBox.setEnabled(true);
    
    // Populate chord selector with progression chords
    const juce::StringArray romanNumerals = { "I", "II", "III", "IV", "V", "VI", "VII" };
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    
    for (int i = 0; i < customProgressionDegrees.size(); ++i)
    {
        int degree = customProgressionDegrees[i];
        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
        auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
        std::string chordName = keyManager.getChordName(scaleDegree, chordType);
        
        juce::String itemText = juce::String(i + 1) + ". " + juce::String(chordName);
        chordSelectorComboBox.addItem(itemText, i + 1);
    }
    
    if (chordSelectorComboBox.getNumItems() > 0)
    {
        chordSelectorComboBox.setSelectedId(1);
        updateEmotionComboBox();
    }
}

void MainComponent::updateEmotionComboBox()
{
    emotionComboBox.clear();
    
    int selectedChordIndex = chordSelectorComboBox.getSelectedId() - 1;
    if (selectedChordIndex < 0 || selectedChordIndex >= customProgressionDegrees.size())
    {
        emotionComboBox.setEnabled(false);
        applyEmotionButton.setEnabled(false);
        return;
    }
    
    // Determine if the selected chord is major or minor
    int degree = customProgressionDegrees[selectedChordIndex];
    auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
    
    // Determine tonality based on chord type
    EmotionWheel::Tonality tonality = EmotionWheel::Tonality::Major;
    
    if (useSevenths)
    {
        if (chordType == KeyManager::ChordType::Minor7 ||
            chordType == KeyManager::ChordType::Minor9 ||
            chordType == KeyManager::ChordType::HalfDiminished7 ||
            chordType == KeyManager::ChordType::Diminished7)
        {
            tonality = EmotionWheel::Tonality::Minor;
        }
    }
    else
    {
        if (chordType == KeyManager::ChordType::Minor ||
            chordType == KeyManager::ChordType::Diminished)
        {
            tonality = EmotionWheel::Tonality::Minor;
        }
    }
    
    // Get emotions for this tonality
    auto emotions = emotionWheel.getEmotionsByTonality(tonality);
    
    for (size_t i = 0; i < emotions.size(); ++i)
    {
        auto emotion = emotions[i];
        juce::String emotionName = EmotionWheel::getEmotionName(emotion);
        emotionComboBox.addItem(emotionName, static_cast<int>(i + 1));
    }
    
    emotionComboBox.setEnabled(true);
    applyEmotionButton.setEnabled(true);
    
    if (emotionComboBox.getNumItems() > 0)
    {
        emotionComboBox.setSelectedId(1);
        updateEmotionDescription();
    }
}

void MainComponent::updateEmotionDescription()
{
    int selectedChordIndex = chordSelectorComboBox.getSelectedId() - 1;
    if (selectedChordIndex < 0 || selectedChordIndex >= customProgressionDegrees.size())
    {
        emotionDescriptionLabel.setText("", juce::dontSendNotification);
        return;
    }
    
    int selectedEmotionIndex = emotionComboBox.getSelectedId() - 1;
    if (selectedEmotionIndex < 0)
    {
        emotionDescriptionLabel.setText("", juce::dontSendNotification);
        return;
    }
    
    // Get the tonality to find the right emotion
    int degree = customProgressionDegrees[selectedChordIndex];
    auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
    
    EmotionWheel::Tonality tonality = EmotionWheel::Tonality::Major;
    if (useSevenths)
    {
        if (chordType == KeyManager::ChordType::Minor7 ||
            chordType == KeyManager::ChordType::Minor9 ||
            chordType == KeyManager::ChordType::HalfDiminished7 ||
            chordType == KeyManager::ChordType::Diminished7)
        {
            tonality = EmotionWheel::Tonality::Minor;
        }
    }
    else
    {
        if (chordType == KeyManager::ChordType::Minor ||
            chordType == KeyManager::ChordType::Diminished)
        {
            tonality = EmotionWheel::Tonality::Minor;
        }
    }
    
    auto emotions = emotionWheel.getEmotionsByTonality(tonality);
    if (selectedEmotionIndex < emotions.size())
    {
        auto emotion = emotions[selectedEmotionIndex];
        const auto* profile = emotionWheel.getEmotionProfile(emotion);
        
        if (profile)
        {
            emotionDescriptionLabel.setText(juce::String(profile->description), juce::dontSendNotification);
        }
    }
}

void MainComponent::applyEmotionToChord()
{
    int selectedChordIndex = chordSelectorComboBox.getSelectedId() - 1;
    if (selectedChordIndex < 0 || selectedChordIndex >= customProgressionDegrees.size())
        return;
    
    int selectedEmotionIndex = emotionComboBox.getSelectedId() - 1;
    if (selectedEmotionIndex < 0)
        return;
    
    // Get the selected emotion
    int degree = customProgressionDegrees[selectedChordIndex];
    auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
    
    EmotionWheel::Tonality tonality = EmotionWheel::Tonality::Major;
    if (useSevenths)
    {
        if (chordType == KeyManager::ChordType::Minor7 ||
            chordType == KeyManager::ChordType::Minor9 ||
            chordType == KeyManager::ChordType::HalfDiminished7 ||
            chordType == KeyManager::ChordType::Diminished7)
        {
            tonality = EmotionWheel::Tonality::Minor;
        }
    }
    else
    {
        if (chordType == KeyManager::ChordType::Minor ||
            chordType == KeyManager::ChordType::Diminished)
        {
            tonality = EmotionWheel::Tonality::Minor;
        }
    }
    
    auto emotions = emotionWheel.getEmotionsByTonality(tonality);
    if (selectedEmotionIndex >= emotions.size())
        return;
    
    auto emotion = emotions[selectedEmotionIndex];
    
    // Resize customProgressionEmotions if needed
    while (customProgressionEmotions.size() < customProgressionDegrees.size())
    {
        customProgressionEmotions.push_back(EmotionWheel::Emotion::Happy_Maj6);  // Default
    }
    
    // Store the emotion for this chord
    customProgressionEmotions[selectedChordIndex] = emotion;
    
    // Update display to show the change
    updateCustomProgressionDisplay();
    
    // If currently playing, restart with the new emotion applied
    if (isPlaying)
    {
        playProgression();
    }
}

