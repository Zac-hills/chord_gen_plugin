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
        
        // Add mouse enter listener to play chord on hover
        chordButtons[i].addMouseListener(this, false);
        
        addAndMakeVisible(chordButtons[i]);
    }
    
    // Apply circular LookAndFeel to the root button (I)
    chordButtons[0].setLookAndFeel(&circularButtonLookAndFeel);
    
    
    
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
    
    // Setup tempo as an editable text field
    tempoEditor.setText("120", juce::dontSendNotification);
    tempoEditor.setEditable(true);
    tempoEditor.setJustificationType(juce::Justification::centred);
    tempoEditor.onTextChange = [this]() {
        int tempo = tempoEditor.getText().getIntValue();
        if (tempo >= 60 && tempo <= 200) {
            updateChordDuration();
        }
    };
    addAndMakeVisible(tempoEditor);
    
    tempoLabel.setText("Tempo (BPM):", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);
    
    // MIDI drag button
    midiDragButton.setButtonText("Drag MIDI");
    midiDragButton.addMouseListener(this, false);
    addAndMakeVisible(midiDragButton);
    
    // Setup emotion buttons
    for (int i = 0; i < 24; ++i)
    {
        emotionButtons[i].onClick = [this, i]() {
            selectedEmotionIndex = i;
            updateEmotionDescription();
            applyEmotionToChord();
        };
        
        // Add mouse listener to play emotion chord on hover
        emotionButtons[i].addMouseListener(this, false);
        
        addAndMakeVisible(emotionButtons[i]);
    }
    
    emotionDescriptionLabel.setText("", juce::dontSendNotification);
    emotionDescriptionLabel.setFont(juce::Font(12.0f, juce::Font::italic));
    emotionDescriptionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(emotionDescriptionLabel);
    
    emotionWheelGroup.setText("Emotion Wheel");
    emotionWheelGroup.setTextLabelPosition(juce::Justification::centredTop);
    addAndMakeVisible(emotionWheelGroup);
    
    // Setup group components
    progressionBuilderGroup.setText("Chord Progression Builder");
    progressionBuilderGroup.setTextLabelPosition(juce::Justification::centredTop);
    addAndMakeVisible(progressionBuilderGroup);
    
    // Initialize progression badge buttons
    for (int i = 0; i < MAX_PROGRESSION_SIZE; ++i)
    {
        auto* badgeButton = new ButtonWithBadge();
        chordButtonsWithBadges.add(badgeButton);
        addAndMakeVisible(badgeButton);
        badgeButton->setVisible(false);  // Hide initially
        
        // Set up main button click handler to select chord for emotion wheel
        int chordIndex = i;
        badgeButton->mainButton.onClick = [this, chordIndex]() {
            selectChordForEmotionWheel(chordIndex);
        };
        
        // Set up badge button click handler to remove chord
        badgeButton->badgeButton.onClick = [this, chordIndex]() {
            removeChordAtIndex(chordIndex);
        };
    }
    
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
    setSize(1200, 700);
    
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
    
    // Title bar area
    bounds.removeFromTop(30);  // Title bar height
    
    // Top control bar with settings, key, time signature, and tempo
    auto topControlBar = bounds.removeFromTop(40);
    topControlBar.reduce(10, 5);  // Add padding
    
    // Settings button on far left
    audioSettingsButton.setBounds(topControlBar.removeFromLeft(50).reduced(5));
    
    topControlBar.removeFromLeft(20);  // Spacing
    
    // Key dropdown
    keyComboBox.setBounds(topControlBar.removeFromLeft(120).reduced(5));
    
    topControlBar.removeFromLeft(10);  // Spacing
    
    // Time signature dropdown
    timeSignatureComboBox.setBounds(topControlBar.removeFromLeft(100).reduced(5));
    
    topControlBar.removeFromLeft(10);  // Spacing
    
    // Tempo field
    tempoLabel.setBounds(topControlBar.removeFromLeft(90).reduced(5));
    tempoEditor.setBounds(topControlBar.removeFromLeft(60).reduced(5));
    
    // Add some spacing after top bar
    bounds.removeFromTop(10);
    
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
    
    // Progressions dropdown at the top of builder group - centered
    auto progressionsRow = builderContent.removeFromTop(35);
    int progressionsWidth = 150;
    int progressionsCenterX = progressionsRow.getCentreX();
    progressionsDropdown.setBounds(progressionsCenterX - progressionsWidth/2, progressionsRow.getY(), progressionsWidth, progressionsRow.getHeight());
    
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
    
    // Right column: Emotion Wheel Group
    auto emotionWheelSection = rightColumn.removeFromTop(350);
    emotionWheelGroup.setBounds(emotionWheelSection);
    
    auto emotionContent = emotionWheelSection.reduced(15, 25);  // Reduce for group border and title
    
    emotionContent.removeFromTop(5);  // Top padding
    
    // Emotion buttons in a 6×4 grid (6 columns = 6 categories, 4 rows = 4 variants each)
    auto buttonGridArea = emotionContent.removeFromTop(200);  // Space for 4 rows of buttons
    int buttonWidth = buttonGridArea.getWidth() / 6;  // 6 columns
    int buttonHeight = 50;  // Height for each button
    
    for (int col = 0; col < 6; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            int index = col * 4 + row;
            int x = buttonGridArea.getX() + col * buttonWidth;
            int y = buttonGridArea.getY() + row * buttonHeight;
            emotionButtons[index].setBounds(x + 2, y + 2, buttonWidth - 4, buttonHeight - 4);
        }
    }
    
    emotionContent.removeFromTop(5);  // Spacing
    
    // Description at the bottom
    emotionDescriptionLabel.setBounds(emotionContent.removeFromTop(40));
    
    // Back to full width for remaining components
    bounds = getLocalBounds();
    bounds.removeFromTop(30 + 40 + 10 + 350 + 20);  // Skip title, top control bar, spacing, groups, and gap
    bounds.reduce(10, 0);  // Add outer padding
    
    // Chord progression badge buttons area at the bottom with play buttons to the right
    auto progressionArea = bounds.removeFromBottom(160);
    
    // Reserve space on the right for play controls
    auto playControlArea = progressionArea.removeFromRight(180);
    
    // Layout badge buttons horizontally in the progression area
    auto badgeButtonArea = progressionArea.reduced(20, 40);  // Add padding
    int badgeButtonWidth = 90;
    int badgeButtonHeight = 70;
    int spacing = 10;
    int numButtons = std::min(static_cast<int>(customProgressionDegrees.size()), static_cast<int>(MAX_PROGRESSION_SIZE));
    
    for (int i = 0; i < MAX_PROGRESSION_SIZE; ++i)
    {
        if (i < numButtons && chordButtonsWithBadges[i] != nullptr)
        {
            int x = badgeButtonArea.getX() + i * (badgeButtonWidth + spacing);
            int y = badgeButtonArea.getY();
            chordButtonsWithBadges[i]->setBounds(x, y, badgeButtonWidth, badgeButtonHeight);
        }
    }
    
    // Position play/stop, loop, and MIDI drag buttons aligned with badge buttons
    int playButtonY = badgeButtonArea.getY();
    playStopButton.setBounds(playControlArea.getX() + 10, playButtonY, playControlArea.getWidth() - 20, 25);
    loopButton.setBounds(playControlArea.getX() + 10, playButtonY + 30, playControlArea.getWidth() - 20, 25);
    midiDragButton.setBounds(playControlArea.getX() + 10, playButtonY + 60, playControlArea.getWidth() - 20, 25);
    
    // Hide the keyboard (keep for MIDI functionality but don't display)
    keyboard.setBounds(0, 0, 0, 0);
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
    double currentTempo = tempoEditor.getText().getIntValue();
    if (currentTempo < 60) currentTempo = 60;
    if (currentTempo > 200) currentTempo = 200;
    
    // Calculate samples per beat (quarter note)
    double quarterNoteDuration = 60.0 / currentTempo;
    
    // Adjust for beat unit (e.g., 8th notes are half a quarter note)
    double beatDuration = quarterNoteDuration * (4.0 / beatUnit);
    
    // Each chord lasts for the full measure
    samplesPerBeat = static_cast<int>(beatDuration * beatsPerMeasure * sampleRate);
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
}

void MainComponent::mouseEnter(const juce::MouseEvent& event)
{
    // Check if the mouse is over one of the chord buttons
    for (int i = 0; i < 7; ++i)
    {
        if (event.eventComponent == &chordButtons[i])
        {
            // Generate and play the chord for this scale degree
            auto scaleDegree = static_cast<KeyManager::ScaleDegree>(i + 1);
            bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
            
            std::vector<int> chord;
            if (useSevenths)
            {
                chord = keyManager.generateSeventh(scaleDegree);
            }
            else
            {
                chord = keyManager.generateTriad(scaleDegree);
            }
            
            playChord(chord);
            return;
        }
    }
    
    // Check if the mouse is over one of the emotion buttons
    for (int i = 0; i < 24; ++i)
    {
        if (event.eventComponent == &emotionButtons[i])
        {
            // Only play if a chord is selected and the button is enabled
            if (selectedChordIndexForEmotion >= 0 && 
                selectedChordIndexForEmotion < customProgressionDegrees.size() &&
                emotionButtons[i].isEnabled())
            {
                // Get the base chord
                int degree = customProgressionDegrees[selectedChordIndexForEmotion];
                auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
                bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
                
                std::vector<int> baseChord;
                if (useSevenths)
                {
                    baseChord = keyManager.generateSeventh(scaleDegree);
                }
                else
                {
                    baseChord = keyManager.generateTriad(scaleDegree);
                }
                
                // Get the tonality to find the right emotion
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
                
                // Get the emotion and apply it
                auto emotions = emotionWheel.getEmotionsByTonality(tonality);
                if (i < emotions.size())
                {
                    auto emotion = emotions[i];
                    int rootNote = baseChord[0];
                    std::vector<int> emotionChord = emotionWheel.applyEmotion(rootNote, emotion);
                    playChord(emotionChord);
                }
            }
            return;
        }
    }
}

void MainComponent::mouseExit(const juce::MouseEvent& event)
{
    // Check if the mouse is leaving one of the chord buttons
    for (int i = 0; i < 7; ++i)
    {
        if (event.eventComponent == &chordButtons[i])
        {
            stopCurrentChord();
            currentChordNotes.clear();
            return;
        }
    }
    
    // Check if the mouse is leaving one of the emotion buttons
    for (int i = 0; i < 24; ++i)
    {
        if (event.eventComponent == &emotionButtons[i])
        {
            stopCurrentChord();
            currentChordNotes.clear();
            return;
        }
    }
}

void MainComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Check if dragging from MIDI drag button
    if (event.eventComponent == &midiDragButton && event.getDistanceFromDragStart() > 10)
    {
        if (customProgressionDegrees.empty())
            return;
            
        // Create a MIDI sequence from the current progression
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(960);
        
        juce::MidiMessageSequence track;
        
        // Get time signature
        int beatsPerBar = 4;
        switch (timeSignatureComboBox.getSelectedId())
        {
            case 1: beatsPerBar = 4; break;  // 4/4
            case 2: beatsPerBar = 3; break;  // 3/4
            case 3: beatsPerBar = 6; break;  // 6/8
            case 4: beatsPerBar = 5; break;  // 5/4
            default: beatsPerBar = 4; break;
        }
        
        // Calculate ticks per chord based on time signature
        int ticksPerChord = midiFile.getTimeFormat() * beatsPerBar;
        
        // Add each chord to the MIDI sequence
        int currentTick = 0;
        bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
        
        for (size_t i = 0; i < customProgressionDegrees.size(); ++i)
        {
            int degree = customProgressionDegrees[i];
            auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
            
            // Generate base chord
            std::vector<int> chord;
            if (useSevenths)
            {
                chord = keyManager.generateSeventh(scaleDegree);
            }
            else
            {
                chord = keyManager.generateTriad(scaleDegree);
            }
            
            // Apply emotion if one exists for this chord
            if (i < customProgressionEmotions.size())
            {
                auto emotion = customProgressionEmotions[i];
                int rootNote = chord[0];
                chord = emotionWheel.applyEmotion(rootNote, emotion);
            }
            
            // Add MIDI notes for this chord
            for (int note : chord)
            {
                if (note >= 0 && note < 128)
                {
                    track.addEvent(juce::MidiMessage::noteOn(1, note, 0.7f), currentTick);
                    track.addEvent(juce::MidiMessage::noteOff(1, note), currentTick + ticksPerChord);
                }
            }
            
            currentTick += ticksPerChord;
        }
        
        midiFile.addTrack(track);
        
        // Write MIDI file to temporary location
        auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("chord_progression.mid");
        
        juce::FileOutputStream stream(tempFile);
        if (stream.openedOk())
        {
            midiFile.writeTo(stream);
            stream.flush();
            
            // Create drag description with file
            juce::StringArray files;
            files.add(tempFile.getFullPathName());
            
            // Use DragAndDropContainer to perform the drag operation
            juce::DragAndDropContainer::performExternalDragDropOfFiles(files, true, &midiDragButton, nullptr);
        }
    }
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
    // If at max capacity and a chord is selected, replace the selected chord
    if (customProgressionDegrees.size() >= MAX_PROGRESSION_SIZE && 
        selectedChordIndexForEmotion >= 0 && 
        selectedChordIndexForEmotion < customProgressionDegrees.size())
    {
        customProgressionDegrees[selectedChordIndexForEmotion] = scaleDegree;
        // Reset to default emotion for the replaced chord
        if (selectedChordIndexForEmotion < customProgressionEmotions.size())
        {
            customProgressionEmotions[selectedChordIndexForEmotion] = EmotionWheel::Emotion::Happy_Maj6;
        }
    }
    // Otherwise, add if under max capacity
    else if (customProgressionDegrees.size() < MAX_PROGRESSION_SIZE)
    {
        customProgressionDegrees.push_back(scaleDegree);
    }
    // If at max and no selection, don't add (silently ignore)
    
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

void MainComponent::removeChordAtIndex(int index)
{
    if (index >= 0 && index < customProgressionDegrees.size())
    {
        customProgressionDegrees.erase(customProgressionDegrees.begin() + index);
        
        if (index < customProgressionEmotions.size())
            customProgressionEmotions.erase(customProgressionEmotions.begin() + index);
        
        // If the removed chord was selected, clear the selection
        if (index == selectedChordIndexForEmotion)
        {
            selectedChordIndexForEmotion = -1;
        }
        // If a chord after the removed one was selected, adjust the index
        else if (index < selectedChordIndexForEmotion)
        {
            selectedChordIndexForEmotion--;
        }
        
        updateCustomProgressionDisplay();
        updateChordSelector();  // Update emotion wheel UI
        
        // Stop playback if currently playing
        if (isPlaying)
        {
            stopProgression();
        }
    }
}

void MainComponent::updateCustomProgressionDisplay()
{
    // Update badge buttons
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    const auto& colors = themeManager.getColors();
    
    for (int i = 0; i < MAX_PROGRESSION_SIZE; ++i)
    {
        if (chordButtonsWithBadges[i] != nullptr)
        {
            if (i < customProgressionDegrees.size())
            {
                // Show and update button
                int degree = customProgressionDegrees[i];
                auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
                auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
                std::string chordName = keyManager.getChordName(scaleDegree, chordType);
                
                chordButtonsWithBadges[i]->mainButton.setButtonText(juce::String(chordName));
                chordButtonsWithBadges[i]->setVisible(true);
                
                // Apply highlighting if this is the selected chord
                if (i == selectedChordIndexForEmotion)
                {
                    chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::buttonColourId, colors.accentPrimary);
                    chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::textColourOffId, colors.backgroundMain);
                }
                else
                {
                    chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::buttonColourId, colors.buttonBackground);
                    chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::textColourOffId, colors.buttonText);
                }
            }
            else
            {
                // Hide unused buttons
                chordButtonsWithBadges[i]->setVisible(false);
            }
        }
    }
    
    // Trigger layout update
    resized();
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

void MainComponent::selectChordForEmotionWheel(int chordIndex)
{
    if (chordIndex < 0 || chordIndex >= customProgressionDegrees.size())
        return;
    
    // Store the selected chord index
    selectedChordIndexForEmotion = chordIndex;
    
    // Update button highlighting
    const auto& colors = themeManager.getColors();
    for (int i = 0; i < chordButtonsWithBadges.size(); ++i)
    {
        if (chordButtonsWithBadges[i] != nullptr)
        {
            if (i == chordIndex)
            {
                // Highlight selected button
                chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::buttonColourId, colors.accentPrimary);
                chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::textColourOffId, colors.backgroundMain);
            }
            else
            {
                // Normal button colors
                chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::buttonColourId, colors.buttonBackground);
                chordButtonsWithBadges[i]->mainButton.setColour(juce::TextButton::textColourOffId, colors.buttonText);
            }
        }
    }
    
    // Update the emotion combo box for this chord
    updateEmotionComboBox();
}

void MainComponent::updateChordSelector()
{
    if (customProgressionDegrees.empty())
    {
        for (auto& btn : emotionButtons)
            btn.setEnabled(false);
        emotionDescriptionLabel.setText("Build a progression first", juce::dontSendNotification);
        return;
    }
    
    // If there's a valid selection, populate emotion buttons
    if (selectedChordIndexForEmotion >= 0 && selectedChordIndexForEmotion < customProgressionDegrees.size())
    {
        updateEmotionComboBox();
    }
    else
    {
        for (auto& btn : emotionButtons)
            btn.setEnabled(false);
        emotionDescriptionLabel.setText("Click a chord to select it", juce::dontSendNotification);
    }
}

void MainComponent::updateEmotionComboBox()
{
    
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
    {
        // Disable all emotion buttons
        for (auto& btn : emotionButtons)
            btn.setEnabled(false);
        return;
    }
    
    // Determine if the selected chord is major or minor
    int degree = customProgressionDegrees[selectedChordIndexForEmotion];
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
    
    // Get emotions for this tonality and populate buttons
    auto emotions = emotionWheel.getEmotionsByTonality(tonality);
    
    for (size_t i = 0; i < 24; ++i)
    {
        if (i < emotions.size())
        {
            auto emotion = emotions[i];
            juce::String emotionName = EmotionWheel::getEmotionName(emotion);
            emotionButtons[i].setButtonText(emotionName);
            emotionButtons[i].setEnabled(true);
        }
        else
        {
            emotionButtons[i].setButtonText("");
            emotionButtons[i].setEnabled(false);
        }
    }
    
    selectedEmotionIndex = -1;
    emotionDescriptionLabel.setText("", juce::dontSendNotification);
}

void MainComponent::updateEmotionDescription()
{
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
    {
        emotionDescriptionLabel.setText("", juce::dontSendNotification);
        return;
    }
    
    if (selectedEmotionIndex < 0)
    {
        emotionDescriptionLabel.setText("", juce::dontSendNotification);
        return;
    }
    
    // Get the tonality to find the right emotion
    int degree = customProgressionDegrees[selectedChordIndexForEmotion];
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
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
        return;
    
    if (selectedEmotionIndex < 0)
        return;
    
    // Get the selected emotion
    int degree = customProgressionDegrees[selectedChordIndexForEmotion];
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
    customProgressionEmotions[selectedChordIndexForEmotion] = emotion;
    
    // Update display to show the change
    updateCustomProgressionDisplay();
    
    // If currently playing, restart with the new emotion applied
    if (isPlaying)
    {
        playProgression();
    }
}

