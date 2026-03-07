#include "MainComponent_NativeUI.h"

//==============================================================================
MainComponent::MainComponent() : keyboard(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Major keys
    keyComboBox.addItem(juce::String::fromUTF8("C Major"), 1);
    keyComboBox.addItem(juce::String::fromUTF8("C♯ Major"), 2);
    keyComboBox.addItem(juce::String::fromUTF8("D Major"), 3);
    keyComboBox.addItem(juce::String::fromUTF8("E♭ Major"), 4);
    keyComboBox.addItem(juce::String::fromUTF8("E Major"), 5);
    keyComboBox.addItem(juce::String::fromUTF8("F Major"), 6);
    keyComboBox.addItem(juce::String::fromUTF8("F♯ Major"), 7);
    keyComboBox.addItem(juce::String::fromUTF8("G Major"), 8);
    keyComboBox.addItem(juce::String::fromUTF8("A♭ Major"), 9);
    keyComboBox.addItem(juce::String::fromUTF8("A Major"), 10);
    keyComboBox.addItem(juce::String::fromUTF8("B♭ Major"), 11);
    keyComboBox.addItem(juce::String::fromUTF8("B Major"), 12);
    
    // Minor keys
    keyComboBox.addItem(juce::String::fromUTF8("A Minor"), 13);
    keyComboBox.addItem(juce::String::fromUTF8("A♯ Minor"), 14);
    keyComboBox.addItem(juce::String::fromUTF8("B Minor"), 15);
    keyComboBox.addItem(juce::String::fromUTF8("C Minor"), 16);
    keyComboBox.addItem(juce::String::fromUTF8("C♯ Minor"), 17);
    keyComboBox.addItem(juce::String::fromUTF8("D Minor"), 18);
    keyComboBox.addItem(juce::String::fromUTF8("D♯ Minor"), 19);
    keyComboBox.addItem(juce::String::fromUTF8("E Minor"), 20);
    keyComboBox.addItem(juce::String::fromUTF8("F Minor"), 21);
    keyComboBox.addItem(juce::String::fromUTF8("F♯ Minor"), 22);
    keyComboBox.addItem(juce::String::fromUTF8("G Minor"), 23);
    keyComboBox.addItem(juce::String::fromUTF8("G♯ Minor"), 24);
    
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
        chordButtons[i].mainButton.setButtonText(romanNumerals[i]);
        chordButtons[i].mainButton.onClick = [this, i] { addChordToProgression(i + 1); };
        
        // Add play button click handler
        chordButtons[i].playButton.onClick = [this, i] {
            // Stop any previous preview
            if (isPreviewPlaying)
            {
                stopCurrentChord();
                currentChordNotes.clear();
                stopTimer();
            }
            
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
            isPreviewPlaying = true;
            
            // Calculate quarter note duration in milliseconds
            int tempo = tempoEditor.getText().getIntValue();
            if (tempo < 60 || tempo > 200) tempo = 120;
            int quarterNoteDuration = (60000 / tempo);  // milliseconds per quarter note
            
            // Start timer to stop chord after quarter note
            startTimer(quarterNoteDuration);
        };
        
        addAndMakeVisible(chordButtons[i]);
    }
    
    // Apply circular LookAndFeel to the root button (I) main button
    chordButtons[0].mainButton.setLookAndFeel(&circularButtonLookAndFeel);
    
    
    
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
    
    // Add alteration combo box
    alterationComboBox.addItem("None", 1);  // No alteration
    alterationComboBox.addItem("Major", 2);
    alterationComboBox.addItem("Minor", 3);
    alterationComboBox.addItem("Diminished", 4);
    alterationComboBox.addItem("Augmented", 5);
    alterationComboBox.addItem("Maj7", 6);
    alterationComboBox.addItem("Min7", 7);
    alterationComboBox.addItem("Dom7", 8);
    alterationComboBox.addItem("Dim7", 9);
    alterationComboBox.addItem("m7b5", 10);
    alterationComboBox.addItem("Sus2", 11);
    alterationComboBox.addItem("Sus4", 12);
    alterationComboBox.addItem("Add9", 13);
    alterationComboBox.addItem("Maj9", 14);
    alterationComboBox.addItem("Min9", 15);
    alterationComboBox.addItem("Dom9", 16);
    alterationComboBox.setSelectedId(1);
    alterationComboBox.onChange = [this] { applyAlterationToChord(); };
    addAndMakeVisible(alterationComboBox);
    
    alterationLabel.setText("Alteration:", juce::dontSendNotification);
    addAndMakeVisible(alterationLabel);
    
    // Add inversion combo box
    inversionComboBox.addItem("Root Position", 1);
    inversionComboBox.addItem("1st Inversion", 2);
    inversionComboBox.addItem("2nd Inversion", 3);
    inversionComboBox.addItem("3rd Inversion", 4);
    inversionComboBox.setSelectedId(1);
    inversionComboBox.onChange = [this] { applyInversionToChord(); };
    addAndMakeVisible(inversionComboBox);
    
    inversionLabel.setText("Inversion:", juce::dontSendNotification);
    addAndMakeVisible(inversionLabel);
    
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
    
    // Root boost slider
    rootBoostSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rootBoostSlider.setRange(0.5, 3.0, 0.1);  // 1.0x to 3.0x boost
    rootBoostSlider.setValue(0.5);  // Start at 0.7x
    rootBoostSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    rootBoostSlider.onValueChange = [this]() {
        rootBoostAmount = static_cast<float>(rootBoostSlider.getValue());
        // If a chord is currently playing, restart it with new gain values
        if (!currentChordNotes.empty()) {
            auto notesToReplay = currentChordNotes;
            stopCurrentChord();
            playChord(notesToReplay);
        }
    };
    addAndMakeVisible(rootBoostSlider);
    
    rootBoostLabel.setText("Bass Boost:", juce::dontSendNotification);
    addAndMakeVisible(rootBoostLabel);
    
    // Setup emotion buttons
    for (int i = 0; i < 24; ++i)
    {
        emotionButtons[i].mainButton.onClick = [this, i]() {
            selectedEmotionIndex = i;
            updateEmotionDescription();
            applyEmotionToChord();
        };
        
        // Add play button click handler for emotion buttons
        emotionButtons[i].playButton.onClick = [this, i]() {
            // Only play if a chord is selected and the button is enabled
            if (selectedChordIndexForEmotion >= 0 && 
                selectedChordIndexForEmotion < customProgressionDegrees.size() &&
                emotionButtons[i].mainButton.isEnabled())
            {
                // Stop any previous preview
                if (isPreviewPlaying)
                {
                    stopCurrentChord();
                    currentChordNotes.clear();
                    stopTimer();
                }
                
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
                    isPreviewPlaying = true;
                    
                    // Calculate quarter note duration in milliseconds
                    int tempo = tempoEditor.getText().getIntValue();
                    if (tempo < 60 || tempo > 200) tempo = 120;
                    int quarterNoteDuration = (60000 / tempo);  // milliseconds per quarter note
                    
                    // Start timer to stop chord after quarter note
                    startTimer(quarterNoteDuration);
                }
            }
        };
        
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
    
    // Add progression dropdown items
    // Using roman numeral-based progressions that work in any key
    progressionsDropdown.addItem("Select Progression...", 1);
    for (int i = 1; i <= 34; ++i)
    {
        progressionsDropdown.addItem("Progression " + juce::String(i), i + 1);
    }
    
    progressionsDropdown.setSelectedId(1);
    progressionsDropdown.onChange = [this] { loadSelectedProgression(); };
    addAndMakeVisible(progressionsDropdown);
    
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
    
    // Setup synthesizer with simple sampler voices
    formatManager.registerBasicFormats();
    
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new SimpleSamplerVoice());
    
    // Load samples from assets folder - check multiple locations
    juce::File assetsFolder;
    
    // 1. Check inside app bundle Resources (for distribution)
    auto appBundle = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    assetsFolder = appBundle.getChildFile("Contents/Resources/assets");
    DBG("Trying app bundle Resources: " << assetsFolder.getFullPathName());
    
    if (!assetsFolder.exists())
    {
        // 2. Try current working directory
        assetsFolder = juce::File::getCurrentWorkingDirectory().getChildFile("assets");
        DBG("Trying CWD: " << assetsFolder.getFullPathName());
    }
    
    if (!assetsFolder.exists())
    {
        // 3. Try relative to executable
        assetsFolder = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                            .getParentDirectory().getChildFile("assets");
        DBG("Trying exec parent: " << assetsFolder.getFullPathName());
    }
    
    if (!assetsFolder.exists())
    {
        // 4. Try going up from executable to project root (for development)
        auto execFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        assetsFolder = execFile.getParentDirectory()      // MacOS -> Contents
                              .getParentDirectory()        // Contents -> App
                              .getParentDirectory()        // App -> Debug
                              .getParentDirectory()        // Debug -> build
                              .getParentDirectory()        // build -> MacOSX
                              .getParentDirectory()        // MacOSX -> Builds
                              .getParentDirectory()        // Builds -> project root
                              .getChildFile("assets");
        DBG("Trying project root: " << assetsFolder.getFullPathName());
    }
    
    if (assetsFolder.exists())
    {
        loadSamples(assetsFolder);
    }
    else
    {
        DBG("Warning: assets folder not found. Please place AIF files in assets/ folder.");
    }
    
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
    chordButtons[0].mainButton.setLookAndFeel(nullptr);
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
        int samplePosition = 0;
        
        while (samplePosition < numSamples)
        {
            if (samplesUntilNextChord <= 0)
            {
                // Stop previous chord notes directly with synth
                for (int note : currentChordNotes)
                {
                    synth.noteOff(1, note, 0.0f, true);
                }
                currentChordNotes.clear();
                
                // Play next chord if available
                if (currentChordIndex < currentProgression.size())
                {
                    auto& chord = currentProgression[currentChordIndex];
                    
                    // Find the lowest note in the chord for bass boost
                    int lowestNote = *std::min_element(chord.begin(), chord.end());
                    
                    // Start new chord notes with gain control
                    for (int note : chord)
                    {
                        if (note >= 0 && note < 128)
                        {
                            // Apply bass boost to the lowest note in the chord
                            bool isBassNote = (note == lowestNote);
                            
                            // Calculate gain multiplier: boost bass, keep others at 1.0
                            float gainMult = isBassNote ? rootBoostAmount : 1.0f;
                            gainMult = juce::jlimit(0.1f, 3.0f, gainMult);
                            
                            // Find an inactive voice and set its gain multiplier
                            for (int v = 0; v < synth.getNumVoices(); ++v)
                            {
                                if (auto* voice = dynamic_cast<SimpleSamplerVoice*>(synth.getVoice(v)))
                                {
                                    if (!voice->isVoiceActive())
                                    {
                                        voice->setGainMultiplier(gainMult);
                                        break;
                                    }
                                }
                            }
                            
                            synth.noteOn(1, note, 0.5f);
                            currentChordNotes.push_back(note);
                        }
                    }
                    
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
                    }
                    else
                    {
                        stopProgression();
                        break;
                    }
                }
            }
            
            int samplesToProcess = juce::jmin(numSamples - samplePosition, samplesUntilNextChord);
            samplesUntilNextChord -= samplesToProcess;
            samplePosition += samplesToProcess;
        }
    }
    
    // Process ALL MIDI through keyboard state (includes both progression and keyboard input)
    juce::MidiBuffer midiMessages;
    keyboardState.processNextMidiBuffer(midiMessages, 0, bufferToFill.numSamples, true);
    
    // Debug MIDI messages
    if (!midiMessages.isEmpty())
    {
        DBG("MIDI events: " << midiMessages.getNumEvents());
        int noteOnCount = 0;
        int noteOffCount = 0;
        for (const auto metadata : midiMessages)
        {
            auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                noteOnCount++;
                DBG("  Note ON: " << message.getNoteNumber() << " vel:" << message.getVelocity() << " @ " << metadata.samplePosition);
            }
            else if (message.isNoteOff())
            {
                noteOffCount++;
                DBG("  Note OFF: " << message.getNoteNumber() << " @ " << metadata.samplePosition);
            }
        }
        DBG("  Summary: " << noteOnCount << " note-ons, " << noteOffCount << " note-offs");
    }
    
    // Render synthesizer audio with MIDI messages from keyboard state
    synth.renderNextBlock(*bufferToFill.buffer, midiMessages, 0, bufferToFill.numSamples);
    
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
    
    topControlBar.removeFromLeft(20);  // Spacing
    
    // Root boost slider and label
    rootBoostLabel.setBounds(topControlBar.removeFromLeft(80).reduced(5));
    rootBoostSlider.setBounds(topControlBar.removeFromLeft(120).reduced(5));
    
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
    progressionsDropdown.setBounds(20, progressionsRow.getY(), progressionsWidth, progressionsRow.getHeight());
    
    builderContent.removeFromTop(20);  // Increased spacing
    
    // Chord buttons in circular layout with root (I) in center
    auto chordButtonArea = builderContent.removeFromTop(180);
    int centerX = chordButtonArea.getCentreX();
    int centerY = chordButtonArea.getCentreY();
    int radius = 85;  // Distance from center
    int buttonSize = 75;  // Button diameter - larger for roman numerals
    
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
    
    // Alteration and Inversion dropdowns above emotion description
    auto controlsRow = emotionContent.removeFromTop(30);
    alterationLabel.setBounds(controlsRow.removeFromLeft(80).reduced(2));
    alterationComboBox.setBounds(controlsRow.removeFromLeft(120).reduced(2));
    controlsRow.removeFromLeft(10);  // Spacing
    inversionLabel.setBounds(controlsRow.removeFromLeft(70).reduced(2));
    inversionComboBox.setBounds(controlsRow.removeFromLeft(110).reduced(2));
    
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
    int badgeButtonWidth = 100;
    int badgeButtonHeight = 80;
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
    int selectedId = keyComboBox.getSelectedId();
    
    // Major keys mapping: ID -> Key index
    // 1:C, 2:C♯, 3:D, 4:E♭(D♯), 5:E, 6:F, 7:F♯, 8:G, 9:A♭(G♯), 10:A, 11:B♭(A♯), 12:B
    const int majorKeyMap[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // C, C♯, D, D♯, E, F, F♯, G, G♯, A, A♯, B
    
    // Minor keys mapping: ID -> Key index  
    // 13:Am, 14:A♯m, 15:Bm, 16:Cm, 17:C♯m, 18:Dm, 19:D♯m, 20:Em, 21:Fm, 22:F♯m, 23:Gm, 24:G♯m
    const int minorKeyMap[12] = {9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8}; // A, A♯, B, C, C♯, D, D♯, E, F, F♯, G, G♯
    
    if (selectedId <= 12)
    {
        // Major key
        int keyIndex = majorKeyMap[selectedId - 1];
        // Determine if this is a sharp key based on the dropdown selection
        // Sharp major keys: C♯(2), D(3), E(5), F♯(7), G(8), A(10), B(12)
        // Flat major keys: E♭(4), F(6), A♭(9), B♭(11)
        bool isSharpKey = (selectedId == 2 || selectedId == 3 || selectedId == 5 || 
                          selectedId == 7 || selectedId == 8 || selectedId == 10 || selectedId == 12);
        keyManager.setCurrentKey(static_cast<KeyManager::Key>(keyIndex), KeyManager::Tonality::Major, isSharpKey);
    }
    else
    {
        // Minor key
        int keyIndex = minorKeyMap[selectedId - 13];
        // Determine if this is a sharp minor key based on the dropdown selection
        // Sharp minor keys: A♯(14), B(15), C♯(17), D♯(19), E(20), F♯(22), G♯(24)
        // Flat minor keys: C(16), D(18), F(21), G(23)
        bool isSharpKey = (selectedId == 14 || selectedId == 15 || selectedId == 17 || 
                          selectedId == 19 || selectedId == 20 || selectedId == 22 || selectedId == 24);
        keyManager.setCurrentKey(static_cast<KeyManager::Key>(keyIndex), KeyManager::Tonality::Minor, isSharpKey);
    }
    
    // Deselect any selected chord for emotion wheel
    selectedChordIndexForEmotion = -1;
    
    // Clear all applied emotions
    hasEmotionApplied.clear();
    hasEmotionApplied.resize(customProgressionDegrees.size(), false);
    customProgressionEmotions.clear();
    customProgressionEmotions.resize(customProgressionDegrees.size(), EmotionWheel::Emotion::Happy_Maj6);
    
    updateDisplay();
    updateChordButtonLabels();  // Update button labels when key changes
    
    // Reload the currently selected progression to apply correct major/minor version
    // This ensures changing from major to minor key gets the correct chord progression variant
    int progressionId = progressionsDropdown.getSelectedId();
    if (progressionId > 1)  // If a progression is selected (not "Select Progression...")
    {
        loadSelectedProgression();
    }
    else
    {
        // No preset progression loaded - just update the display for any custom chords
        updateCustomProgressionDisplay();
    }
    
    // If currently playing, restart the progression with the new key
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
            if (i < hasEmotionApplied.size() && hasEmotionApplied[i] && i < customProgressionEmotions.size())
            {
                auto emotion = customProgressionEmotions[i];
                
                // Get the root note for this scale degree
                auto scaleNotes = keyManager.getScaleNotes();
                if (degree - 1 < scaleNotes.size())
                {
                    // Scale notes are 0-11 (pitch classes), so add base octave (48 = C3)
                    int rootNote = 48 + scaleNotes[degree - 1];
                    
                    // Apply emotion to get chord notes (emotions reset inversion)
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
            // Check if this chord has an alteration applied
            else if (i < customProgressionAlterations.size())
            {
                auto alteration = customProgressionAlterations[i];
                
                // Get the root note and scale
                auto scaleNotes = keyManager.getScaleNotes();
                if (degree - 1 < scaleNotes.size())
                {
                    int rootNote = 48 + scaleNotes[degree - 1];
                    std::string scale = (keyManager.getCurrentTonality() == KeyManager::Tonality::Major) ? "Major" : "Minor";
                    
                    // Check if inversion is also applied
                    int inversion = 0;
                    if (i < customProgressionInversions.size())
                    {
                        inversion = customProgressionInversions[i];
                    }
                    
                    // Generate chord with alteration and inversion
                    chord = keyManager.generateChordWithAlteration(degree, rootNote, scale, alteration, inversion);
                }
                else
                {
                    // Fallback
                    if (useSevenths)
                        chord = keyManager.generateSeventh(scaleDegree);
                    else
                        chord = keyManager.generateTriad(scaleDegree);
                }
            }
            else
            {
                // No emotion or alteration applied, use regular chord generation
                if (useSevenths)
                    chord = keyManager.generateSeventh(scaleDegree);
                else
                    chord = keyManager.generateTriad(scaleDegree);
                
                // Check if inversion is applied without alteration
                if (i < customProgressionInversions.size() && customProgressionInversions[i] > 0)
                {
                    chord = keyManager.applyInversion(chord, customProgressionInversions[i]);
                }
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
    
    // Find the lowest note in the chord for bass boost
    int lowestNote = *std::min_element(chord.begin(), chord.end());
    
    // Play new chord notes directly with synth to control gain
    // Apply bass boost to the lowest note in the chord
    for (size_t i = 0; i < chord.size(); ++i)
    {
        int note = chord[i];
        if (note >= 0 && note < 128)
        {
            // Apply bass boost to the lowest note in the chord
            bool isBassNote = (note == lowestNote);
            
            // Calculate gain multiplier: boost bass, keep others at 1.0
            float gainMult = isBassNote ? rootBoostAmount : 1.0f;
            gainMult = juce::jlimit(0.1f, 3.0f, gainMult);
            
            // Find an inactive voice and set its gain multiplier before starting the note
            for (int v = 0; v < synth.getNumVoices(); ++v)
            {
                if (auto* voice = dynamic_cast<SimpleSamplerVoice*>(synth.getVoice(v)))
                {
                    if (!voice->isVoiceActive())
                    {
                        voice->setGainMultiplier(gainMult);
                        break;
                    }
                }
            }
            
            // Use synth.noteOn directly to trigger the voice we just configured
            synth.noteOn(1, note, 0.5f);
            currentChordNotes.push_back(note);
        }
    }
}

void MainComponent::stopCurrentChord()
{
    // Stop all currently playing chord notes directly with synth
    for (int note : currentChordNotes)
    {
        synth.noteOff(1, note, 0.0f, true);
    }
}

void MainComponent::timerCallback()
{
    // Stop the preview chord after timer expires
    if (isPreviewPlaying)
    {
        stopCurrentChord();
        currentChordNotes.clear();
        isPreviewPlaying = false;
        stopTimer();
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
            
            // Generate chord based on type - matching playProgression() logic
            std::vector<int> chord;
            
            // Check if this chord has an emotion applied
            if (i < hasEmotionApplied.size() && hasEmotionApplied[i] && i < customProgressionEmotions.size())
            {
                auto emotion = customProgressionEmotions[i];
                
                // Get the root note for this scale degree
                auto scaleNotes = keyManager.getScaleNotes();
                if (degree - 1 < scaleNotes.size())
                {
                    // Scale notes are 0-11 (pitch classes), so add base octave (48 = C3)
                    int rootNote = 48 + scaleNotes[degree - 1];
                    
                    // Apply emotion to get chord notes (emotions reset inversion)
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
            // Check if this chord has an alteration applied
            else if (i < customProgressionAlterations.size())
            {
                auto alteration = customProgressionAlterations[i];
                
                // Get the root note and scale
                auto scaleNotes = keyManager.getScaleNotes();
                if (degree - 1 < scaleNotes.size())
                {
                    int rootNote = 48 + scaleNotes[degree - 1];
                    std::string scale = (keyManager.getCurrentTonality() == KeyManager::Tonality::Major) ? "Major" : "Minor";
                    
                    // Check if inversion is also applied
                    int inversion = 0;
                    if (i < customProgressionInversions.size())
                    {
                        inversion = customProgressionInversions[i];
                    }
                    
                    // Generate chord with alteration and inversion
                    chord = keyManager.generateChordWithAlteration(degree, rootNote, scale, alteration, inversion);
                }
                else
                {
                    // Fallback
                    if (useSevenths)
                        chord = keyManager.generateSeventh(scaleDegree);
                    else
                        chord = keyManager.generateTriad(scaleDegree);
                }
            }
            else
            {
                // No emotion or alteration applied, use regular chord generation
                if (useSevenths)
                    chord = keyManager.generateSeventh(scaleDegree);
                else
                    chord = keyManager.generateTriad(scaleDegree);
                
                // Check if inversion is applied without alteration
                if (i < customProgressionInversions.size() && customProgressionInversions[i] > 0)
                {
                    chord = keyManager.applyInversion(chord, customProgressionInversions[i]);
                }
            }
            
            // Get the root note of the current key (0-11 pitch class)
            auto scaleNotes = keyManager.getScaleNotes();
            int keyRootPitchClass = scaleNotes.empty() ? 0 : scaleNotes[0];
            
            // Check if the chord contains the root note (any octave)
            bool hasRootNote = false;
            for (int note : chord)
            {
                if ((note % 12) == keyRootPitchClass)
                {
                    hasRootNote = true;
                    break;
                }
            }
            
            // If chord doesn't have root, add it as bass note to MIDI
            if (!hasRootNote)
            {
                int bassRootNote = 48 + keyRootPitchClass;
                if (bassRootNote >= 0 && bassRootNote < 128)
                {
                    track.addEvent(juce::MidiMessage::noteOn(1, bassRootNote, 0.7f), currentTick);
                    track.addEvent(juce::MidiMessage::noteOff(1, bassRootNote), currentTick + ticksPerChord);
                }
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
        // create UUID for the MIDI file
        juce::Uuid midiFileUuid;
        // initialize UUID
        juce::String uuidString = midiFileUuid.toString();
        
        // Write MIDI file to temporary location
        auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile(uuidString + "chord_progression.mid");
        
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
    // If a chord is selected, replace it
    if (selectedChordIndexForEmotion >= 0 && 
        selectedChordIndexForEmotion < customProgressionDegrees.size())
    {
        customProgressionDegrees[selectedChordIndexForEmotion] = scaleDegree;
        
        // Clear the emotion, alteration, and inversion for the replaced chord
        if (selectedChordIndexForEmotion < hasEmotionApplied.size())
        {
            hasEmotionApplied[selectedChordIndexForEmotion] = false;
        }
        if (selectedChordIndexForEmotion < customProgressionAlterations.size())
        {
            customProgressionAlterations[selectedChordIndexForEmotion] = KeyManager::ChordType::Major;  // Reset
        }
        if (selectedChordIndexForEmotion < customProgressionInversions.size())
        {
            customProgressionInversions[selectedChordIndexForEmotion] = 0;  // Reset to root
        }
        
        // Deselect after replacing
        selectedChordIndexForEmotion = -1;
    }
    // Otherwise, add if under max capacity
    else if (customProgressionDegrees.size() < MAX_PROGRESSION_SIZE)
    {
        customProgressionDegrees.push_back(scaleDegree);
        customProgressionAlterations.push_back(KeyManager::ChordType::Major);  // Default alteration
        customProgressionInversions.push_back(0);  // Default to root position
    }
    // If at max and no selection, don't add (silently ignore)
    
    updateCustomProgressionDisplay();
    updateChordSelector();  // Update emotion wheel UI
}

void MainComponent::clearCustomProgression()
{
    customProgressionDegrees.clear();
    customProgressionEmotions.clear();  // Clear emotions too
    customProgressionAlterations.clear();  // Clear alterations
    customProgressionInversions.clear();  // Clear inversions
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
        if (!customProgressionAlterations.empty())
            customProgressionAlterations.pop_back();
        if (!customProgressionInversions.empty())
            customProgressionInversions.pop_back();
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
        
        if (index < hasEmotionApplied.size())
            hasEmotionApplied.erase(hasEmotionApplied.begin() + index);
        
        if (index < customProgressionAlterations.size())
            customProgressionAlterations.erase(customProgressionAlterations.begin() + index);
        
        if (index < customProgressionInversions.size())
            customProgressionInversions.erase(customProgressionInversions.begin() + index);
        
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
    
    // Roman numeral arrays for display
    const juce::StringArray romanNumeralsMajor = { "", "I", "II", "III", "IV", "V", "VI", "VII" };
    const juce::StringArray romanNumeralsMinor = { "", "i", "ii", "iii", "iv", "v", "vi", "vii" };
    
    for (int i = 0; i < MAX_PROGRESSION_SIZE; ++i)
    {
        if (chordButtonsWithBadges[i] != nullptr)
        {
            if (i < customProgressionDegrees.size())
            {
                // Show and update button
                std::string chordName;
                
                // Get the base chord name from scale degree
                int degree = customProgressionDegrees[i];
                auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
                
                // Check if this chord has an alteration applied
                KeyManager::ChordType chordType;
                if (i < customProgressionAlterations.size() && 
                    customProgressionAlterations[i] != KeyManager::ChordType::Major)
                {
                    // Use the applied alteration
                    chordType = customProgressionAlterations[i];
                }
                else if (i < hasEmotionApplied.size() && hasEmotionApplied[i])
                {
                    // Emotion applied - use the natural chord type
                    chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
                }
                else
                {
                    // Use natural chord type from scale
                    chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
                }
                
                // Determine if this chord type is minor/diminished for roman numeral case
                bool isMinorType = (chordType == KeyManager::ChordType::Minor ||
                                    chordType == KeyManager::ChordType::Minor7 ||
                                    chordType == KeyManager::ChordType::Minor9 ||
                                    chordType == KeyManager::ChordType::MinorAdd9 ||
                                    chordType == KeyManager::ChordType::MinorAdd4 ||
                                    chordType == KeyManager::ChordType::MinorAdd11 ||
                                    chordType == KeyManager::ChordType::MinorSixth ||
                                    chordType == KeyManager::ChordType::Diminished ||
                                    chordType == KeyManager::ChordType::Diminished7 ||
                                    chordType == KeyManager::ChordType::HalfDiminished7);
                
                // Get appropriate roman numeral
                juce::String romanNumeral = isMinorType ? romanNumeralsMinor[degree] : romanNumeralsMajor[degree];
                
                // Add voicing suffix to roman numeral
                switch (chordType)
                {
                    case KeyManager::ChordType::Major7:          romanNumeral += "maj7"; break;
                    case KeyManager::ChordType::Minor7:          romanNumeral += "7"; break;
                    case KeyManager::ChordType::Dominant7:       romanNumeral += "7"; break;
                    case KeyManager::ChordType::Diminished7:     romanNumeral += juce::String::fromUTF8("°7"); break;
                    case KeyManager::ChordType::HalfDiminished7: romanNumeral += juce::String::fromUTF8("ø7"); break;
                    case KeyManager::ChordType::Diminished:      romanNumeral += juce::String::fromUTF8("°"); break;
                    case KeyManager::ChordType::Augmented:       romanNumeral += "+"; break;
                    case KeyManager::ChordType::Sus2:            romanNumeral += "(sus2)"; break;
                    case KeyManager::ChordType::Sus4:            romanNumeral += "(sus4)"; break;
                    case KeyManager::ChordType::Add4:            romanNumeral += "(add4)"; break;
                    case KeyManager::ChordType::MinorAdd4:       romanNumeral += "(add4)"; break;
                    case KeyManager::ChordType::Add9:            romanNumeral += "(add9)"; break;
                    case KeyManager::ChordType::MinorAdd9:       romanNumeral += "(add9)"; break;
                    case KeyManager::ChordType::Add11:           romanNumeral += "(add11)"; break;
                    case KeyManager::ChordType::MinorAdd11:      romanNumeral += "(add11)"; break;
                    case KeyManager::ChordType::Sixth:           romanNumeral += "6"; break;
                    case KeyManager::ChordType::MinorSixth:      romanNumeral += "6"; break;
                    case KeyManager::ChordType::Major9:          romanNumeral += "maj9"; break;
                    case KeyManager::ChordType::Minor9:          romanNumeral += "9"; break;
                    case KeyManager::ChordType::Dominant9:       romanNumeral += "9"; break;
                    default: break;  // Major and Minor triads have no suffix
                }
                
                std::string baseChordName = keyManager.getChordName(scaleDegree, chordType);
                
                // Add inversion indicator if not root position
                if (i < customProgressionInversions.size() && customProgressionInversions[i] > 0)
                {
                    int inv = customProgressionInversions[i];
                    if (inv == 1) baseChordName += "/1";
                    else if (inv == 2) baseChordName += "/2";
                    else if (inv == 3) baseChordName += "/3";
                }
                
                // Build display string: Roman numeral on first line, chord name on second
                juce::String displayText = romanNumeral + "\n" + juce::String(baseChordName);
                
                // Check if this chord has an applied emotion - add it on third line
                if (i < hasEmotionApplied.size() && hasEmotionApplied[i] && i < customProgressionEmotions.size())
                {
                    std::string emotionName = EmotionWheel::getEmotionName(customProgressionEmotions[i]);
                    displayText += "\n" + juce::String(emotionName);
                }
                
                chordButtonsWithBadges[i]->mainButton.setButtonText(displayText);
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
    const juce::StringArray romanNumeralsMajor = { "I", "II", "III", "IV", "V", "VI", "VII" };
    const juce::StringArray romanNumeralsMinor = { "i", "ii", "iii", "iv", "v", "vi", "vii" };
    auto scaleNotes = keyManager.getScaleNoteNamesWithProperSpelling();
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    
    for (int i = 0; i < 7; ++i)
    {
        if (i < scaleNotes.size())
        {
            auto scaleDegree = static_cast<KeyManager::ScaleDegree>(i + 1);
            auto chordType = useSevenths ? keyManager.analyzeSeventh(scaleDegree) : keyManager.analyzeTriad(scaleDegree);
            
            // Determine if chord is minor/diminished for roman numeral case
            bool isMinorType = (chordType == KeyManager::ChordType::Minor ||
                                chordType == KeyManager::ChordType::Minor7 ||
                                chordType == KeyManager::ChordType::Minor9 ||
                                chordType == KeyManager::ChordType::Diminished ||
                                chordType == KeyManager::ChordType::Diminished7 ||
                                chordType == KeyManager::ChordType::HalfDiminished7);
            
            // Get appropriate roman numeral
            juce::String romanNumeral = isMinorType ? romanNumeralsMinor[i] : romanNumeralsMajor[i];
            
            // Add voicing suffix to roman numeral based on chord type
            if (useSevenths)
            {
                switch (chordType)
                {
                    case KeyManager::ChordType::Major7:          romanNumeral += "maj7"; break;
                    case KeyManager::ChordType::Minor7:          romanNumeral += "7"; break;
                    case KeyManager::ChordType::Dominant7:       romanNumeral += "7"; break;
                    case KeyManager::ChordType::Diminished7:     romanNumeral += juce::String::fromUTF8("°7"); break;
                    case KeyManager::ChordType::HalfDiminished7: romanNumeral += juce::String::fromUTF8("ø7"); break;
                    default: break;
                }
            }
            else
            {
                if (chordType == KeyManager::ChordType::Diminished)
                    romanNumeral += juce::String::fromUTF8("°");
            }
            
            // Build the chord name with proper quality indicator
            juce::String noteName = juce::String(scaleNotes[i]);
            
            if (useSevenths)
            {
                if (chordType == KeyManager::ChordType::Minor7 || chordType == KeyManager::ChordType::Minor9)
                    noteName += "m7";
                else if (chordType == KeyManager::ChordType::HalfDiminished7)
                    noteName += juce::String::fromUTF8("ø7");
                else if (chordType == KeyManager::ChordType::Diminished7)
                    noteName += juce::String::fromUTF8("°7");
                else if (chordType == KeyManager::ChordType::Major7)
                    noteName += "M7";
                else if (chordType == KeyManager::ChordType::Dominant7)
                    noteName += "7";
            }
            else
            {
                if (chordType == KeyManager::ChordType::Minor)
                    noteName += "m";
                else if (chordType == KeyManager::ChordType::Diminished)
                    noteName += juce::String::fromUTF8("°");
            }
            
            // Combine roman numeral and note name on two lines
            juce::String buttonText = romanNumeral + "\n" + noteName;
            chordButtons[i].mainButton.setButtonText(buttonText);
        }
    }
}

//==============================================================================
// Emotion Wheel Methods

void MainComponent::selectChordForEmotionWheel(int chordIndex)
{
    if (chordIndex < 0 || chordIndex >= customProgressionDegrees.size())
        return;
    
    // Toggle: if clicking the already selected chord, deselect it
    if (selectedChordIndexForEmotion == chordIndex)
    {
        selectedChordIndexForEmotion = -1; // Deselect
    }
    else
    {
        selectedChordIndexForEmotion = chordIndex; // Select new chord
    }
    
    // Update button highlighting
    const auto& colors = themeManager.getColors();
    for (int i = 0; i < chordButtonsWithBadges.size(); ++i)
    {
        if (chordButtonsWithBadges[i] != nullptr)
        {
            if (i == selectedChordIndexForEmotion)
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
    
    // Update alteration and inversion dropdowns
    updateAlterationComboBox();
    updateInversionComboBox();
}

void MainComponent::updateChordSelector()
{
    if (customProgressionDegrees.empty())
    {
        for (auto& btn : emotionButtons)
            btn.mainButton.setEnabled(false);
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
            btn.mainButton.setEnabled(false);
        emotionDescriptionLabel.setText("Click a chord to select it", juce::dontSendNotification);
    }
}

void MainComponent::updateEmotionComboBox()
{
    
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
    {
        // Disable all emotion buttons
        for (auto& btn : emotionButtons)
            btn.mainButton.setEnabled(false);
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
            emotionButtons[i].mainButton.setButtonText(emotionName);
            emotionButtons[i].mainButton.setEnabled(true);
        }
        else
        {
            emotionButtons[i].mainButton.setButtonText("");
            emotionButtons[i].mainButton.setEnabled(false);
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
    
    // Ensure vectors are large enough
    if (customProgressionEmotions.size() <= selectedChordIndexForEmotion)
    {
        customProgressionEmotions.resize(customProgressionDegrees.size(), EmotionWheel::Emotion::Happy_Maj6);
    }
    if (hasEmotionApplied.size() <= selectedChordIndexForEmotion)
    {
        hasEmotionApplied.resize(customProgressionDegrees.size(), false);
    }
    
    // Store the emotion for this chord and mark it as applied
    customProgressionEmotions[selectedChordIndexForEmotion] = emotion;
    hasEmotionApplied[selectedChordIndexForEmotion] = true;
    
    // When emotion is applied, preserve alteration but reset inversion
    if (customProgressionInversions.size() > selectedChordIndexForEmotion)
    {
        customProgressionInversions[selectedChordIndexForEmotion] = 0;  // Reset to root position
    }
    
    // Update display to show the change (this will update the button text)
    updateCustomProgressionDisplay();
    
    // If currently playing, restart with the new emotion applied
    if (isPlaying)
    {
        playProgression();
    }
}

void MainComponent::updateAlterationComboBox()
{
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
    {
        alterationComboBox.setEnabled(false);
        alterationComboBox.setSelectedId(1, juce::dontSendNotification);  // Reset to "Natural"
        return;
    }
    
    alterationComboBox.setEnabled(true);
    
    // Check if this chord has an alteration
    if (selectedChordIndexForEmotion < customProgressionAlterations.size())
    {
        KeyManager::ChordType alteration = customProgressionAlterations[selectedChordIndexForEmotion];
        
        // Map ChordType back to dropdown ID
        int dropdownId = 1;  // Default to "Natural"
        switch (alteration)
        {
            case KeyManager::ChordType::Major: dropdownId = 2; break;
            case KeyManager::ChordType::Minor: dropdownId = 3; break;
            case KeyManager::ChordType::Diminished: dropdownId = 4; break;
            case KeyManager::ChordType::Augmented: dropdownId = 5; break;
            case KeyManager::ChordType::Major7: dropdownId = 6; break;
            case KeyManager::ChordType::Minor7: dropdownId = 7; break;
            case KeyManager::ChordType::Dominant7: dropdownId = 8; break;
            case KeyManager::ChordType::Diminished7: dropdownId = 9; break;
            case KeyManager::ChordType::HalfDiminished7: dropdownId = 10; break;
            case KeyManager::ChordType::Sus2: dropdownId = 11; break;
            case KeyManager::ChordType::Sus4: dropdownId = 12; break;
            case KeyManager::ChordType::Add9: dropdownId = 13; break;
            case KeyManager::ChordType::Major9: dropdownId = 14; break;
            case KeyManager::ChordType::Minor9: dropdownId = 15; break;
            case KeyManager::ChordType::Dominant9: dropdownId = 16; break;
            default: dropdownId = 1; break;
        }
        
        alterationComboBox.setSelectedId(dropdownId, juce::dontSendNotification);
    }
    else
    {
        alterationComboBox.setSelectedId(1, juce::dontSendNotification);  // "Natural"
    }
}

void MainComponent::updateInversionComboBox()
{
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
    {
        inversionComboBox.setEnabled(false);
        inversionComboBox.setSelectedId(1, juce::dontSendNotification);  // Reset to root position
        return;
    }
    
    inversionComboBox.setEnabled(true);
    
    // Check if this chord has an inversion
    if (selectedChordIndexForEmotion < customProgressionInversions.size())
    {
        int inversion = customProgressionInversions[selectedChordIndexForEmotion];
        inversionComboBox.setSelectedId(inversion + 1, juce::dontSendNotification);  // 0->1, 1->2, etc.
    }
    else
    {
        inversionComboBox.setSelectedId(1, juce::dontSendNotification);  // Root position
    }
}

void MainComponent::applyAlterationToChord()
{
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
        return;
    
    int alterationId = alterationComboBox.getSelectedId();
    if (alterationId == 1)  // "Natural" means no alteration
    {
        // Remove alteration by resizing the vector
        if (customProgressionAlterations.size() > selectedChordIndexForEmotion)
        {
            customProgressionAlterations[selectedChordIndexForEmotion] = KeyManager::ChordType::Major;  // Reset to default
        }
        
        // Also remove emotion since we're going back to natural
        if (hasEmotionApplied.size() > selectedChordIndexForEmotion)
        {
            hasEmotionApplied[selectedChordIndexForEmotion] = false;
        }
    }
    else
    {
        // Map alterationId to ChordType
        KeyManager::ChordType alteration;
        switch (alterationId)
        {
            case 2: alteration = KeyManager::ChordType::Major; break;
            case 3: alteration = KeyManager::ChordType::Minor; break;
            case 4: alteration = KeyManager::ChordType::Diminished; break;
            case 5: alteration = KeyManager::ChordType::Augmented; break;
            case 6: alteration = KeyManager::ChordType::Major7; break;
            case 7: alteration = KeyManager::ChordType::Minor7; break;
            case 8: alteration = KeyManager::ChordType::Dominant7; break;
            case 9: alteration = KeyManager::ChordType::Diminished7; break;
            case 10: alteration = KeyManager::ChordType::HalfDiminished7; break;
            case 11: alteration = KeyManager::ChordType::Sus2; break;
            case 12: alteration = KeyManager::ChordType::Sus4; break;
            case 13: alteration = KeyManager::ChordType::Add9; break;
            case 14: alteration = KeyManager::ChordType::Major9; break;
            case 15: alteration = KeyManager::ChordType::Minor9; break;
            case 16: alteration = KeyManager::ChordType::Dominant9; break;
            default: alteration = KeyManager::ChordType::Major; break;
        }
        
        // Ensure vectors are large enough
        if (customProgressionAlterations.size() <= selectedChordIndexForEmotion)
        {
            customProgressionAlterations.resize(customProgressionDegrees.size(), KeyManager::ChordType::Major);
        }
        
        customProgressionAlterations[selectedChordIndexForEmotion] = alteration;
        
        // Remove emotion when alteration is applied (alteration takes precedence)
        if (hasEmotionApplied.size() > selectedChordIndexForEmotion)
        {
            hasEmotionApplied[selectedChordIndexForEmotion] = false;
        }
    }
    
    // Update display
    updateCustomProgressionDisplay();
    
    // If currently playing, restart with the new alteration
    if (isPlaying)
    {
        playProgression();
    }
}

void MainComponent::applyInversionToChord()
{
    if (selectedChordIndexForEmotion < 0 || selectedChordIndexForEmotion >= customProgressionDegrees.size())
        return;
    
    int inversionId = inversionComboBox.getSelectedId();
    int inversion = inversionId - 1;  // 1=root(0), 2=1st(1), 3=2nd(2), 4=3rd(3)
    
    // Ensure vectors are large enough
    if (customProgressionInversions.size() <= selectedChordIndexForEmotion)
    {
        customProgressionInversions.resize(customProgressionDegrees.size(), 0);
    }
    
    customProgressionInversions[selectedChordIndexForEmotion] = inversion;
    
    // Update display
    updateCustomProgressionDisplay();
    
    // If currently playing, restart with the new inversion
    if (isPlaying)
    {
        playProgression();
    }
}

//==============================================================================
// File Drag and Drop Implementation

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any of the files is a MIDI file
    for (const auto& filename : files)
    {
        if (filename.endsWithIgnoreCase(".mid") || filename.endsWithIgnoreCase(".midi"))
            return true;
    }
    return false;
}

void MainComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    for (const auto& filename : files)
    {
        if (filename.endsWithIgnoreCase(".mid") || filename.endsWithIgnoreCase(".midi"))
        {
            juce::File midiFile(filename);
            processMidiFile(midiFile);
            break; // Process only the first MIDI file
        }
    }
}

void MainComponent::processMidiFile(const juce::File& file)
{
    juce::MidiFile midiFile;
    juce::FileInputStream inputStream(file);
    
    if (!inputStream.openedOk())
    {
        DBG("Failed to open MIDI file: " << file.getFullPathName());
        return;
    }
    
    if (!midiFile.readFrom(inputStream))
    {
        DBG("Failed to read MIDI file: " << file.getFullPathName());
        return;
    }
    
    // Create output directory
    juce::File outputDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("chord_prog_midi");
    
    if (!outputDir.exists())
        outputDir.createDirectory();
    
    // Create output file with same name as MIDI file
    juce::String outputFileName = file.getFileNameWithoutExtension() + "_analysis.txt";
    juce::File outputFile = outputDir.getChildFile(outputFileName);
    
    // Build the analysis text
    juce::String analysisText;
    
    analysisText << "========================================\n";
    analysisText << "MIDI FILE ANALYSIS: " << file.getFileName() << "\n";
    analysisText << "========================================\n";
    analysisText << "Number of tracks: " << midiFile.getNumTracks() << "\n";
    analysisText << "Time format: " << midiFile.getTimeFormat() << "\n";
    analysisText << "\n";
    
    // Extract chords and get the analysis text
    juce::String chordAnalysis = extractChordsFromMidi(midiFile);
    analysisText << chordAnalysis;
    
    // Write to file
    outputFile.replaceWithText(analysisText);
    
    DBG("MIDI analysis written to: " << outputFile.getFullPathName());
}

juce::String MainComponent::extractChordsFromMidi(const juce::MidiFile& midiFile)
{
    juce::String output;
    
    // Map to store notes at each time position
    std::map<double, std::vector<int>> notesAtTime;
    
    // Process all tracks
    for (int trackNum = 0; trackNum < midiFile.getNumTracks(); ++trackNum)
    {
        const juce::MidiMessageSequence* track = midiFile.getTrack(trackNum);
        if (!track) continue;
        
        output << "Track " << trackNum << " has " << track->getNumEvents() << " events\n";
        
        // Track active notes
        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            const juce::MidiMessageSequence::MidiEventHolder* event = track->getEventPointer(i);
            const juce::MidiMessage& msg = event->message;
            
            if (msg.isNoteOn())
            {
                double timeInSeconds = msg.getTimeStamp();
                int noteNumber = msg.getNoteNumber();
                notesAtTime[timeInSeconds].push_back(noteNumber);
            }
        }
    }
    
    // Print chords in a structured format
    output << "\n";
    output << "CHORD PROGRESSION DETECTED:\n";
    output << "========================================\n";
    
    int chordIndex = 0;
    for (const auto& [time, notes] : notesAtTime)
    {
        if (notes.empty()) continue;
        
        // Sort notes for consistent display
        std::vector<int> sortedNotes = notes;
        std::sort(sortedNotes.begin(), sortedNotes.end());
        
        // Remove duplicates
        sortedNotes.erase(std::unique(sortedNotes.begin(), sortedNotes.end()), sortedNotes.end());
        
        output << "\n";
        output << "Chord " << (chordIndex + 1) << " at time " << juce::String(time, 2) << "s:\n";
        
        // Print note numbers
        juce::String noteNumbersStr = "  MIDI Notes: ";
        for (int note : sortedNotes)
            noteNumbersStr << note << " ";
        output << noteNumbersStr << "\n";
        
        // Print note names
        juce::String noteNamesStr = "  Note Names: ";
        for (int note : sortedNotes)
        {
            int pitchClass = note % 12;
            const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            int octave = (note / 12) - 1;
            noteNamesStr << noteNames[pitchClass] << octave << " ";
        }
        output << noteNamesStr << "\n";
        
        // Analyze chord type (simplified)
        if (sortedNotes.size() >= 3)
        {
            // Get intervals from root
            std::vector<int> intervals;
            for (size_t i = 1; i < sortedNotes.size(); ++i)
            {
                int interval = (sortedNotes[i] - sortedNotes[0]) % 12;
                intervals.push_back(interval);
            }
            
            juce::String chordType = "  Chord Type: ";
            
            // Basic chord detection
            if (intervals.size() >= 2)
            {
                if (intervals[0] == 4 && intervals[1] == 7)
                    chordType << "Major";
                else if (intervals[0] == 3 && intervals[1] == 7)
                    chordType << "Minor";
                else if (intervals[0] == 3 && intervals[1] == 6)
                    chordType << "Diminished";
                else if (intervals[0] == 4 && intervals[1] == 8)
                    chordType << "Augmented";
                else
                    chordType << "Unknown";
                    
                // Check for 7th
                if (intervals.size() >= 3)
                {
                    if (intervals[2] == 10)
                        chordType << "7";
                    else if (intervals[2] == 11)
                        chordType << " Major7";
                }
            }
            
            output << chordType << "\n";
        }
        
        chordIndex++;
    }
    
    output << "\n";
    output << "========================================\n";
    output << "EASY COPY FORMAT (for chord progression):\n";
    output << "Total chords detected: " << notesAtTime.size() << "\n";
    output << "\n";
    
    chordIndex = 0;
    for (const auto& [time, notes] : notesAtTime)
    {
        if (notes.empty()) continue;
        
        std::vector<int> sortedNotes = notes;
        std::sort(sortedNotes.begin(), sortedNotes.end());
        sortedNotes.erase(std::unique(sortedNotes.begin(), sortedNotes.end()), sortedNotes.end());
        
        juce::String chordStr = "Chord " + juce::String(chordIndex + 1) + ": { ";
        for (int note : sortedNotes)
            chordStr << note << ", ";
        chordStr = chordStr.dropLastCharacters(2) + " }";
        
        output << chordStr << "\n";
        chordIndex++;
    }
    
    output << "========================================\n";
    
    return output;
}

//==============================================================================
// Generate MIDI notes from a ChordDefinition in the current key
std::vector<int> MainComponent::generateChordFromDefinition(const ChordDefinition& def)
{
    // Get the root note for this scale degree in the current key
    auto scaleDegree = static_cast<KeyManager::ScaleDegree>(def.scaleDegree);
    int rootNote = keyManager.getNoteFromDegree(scaleDegree);
    
    // Base octave (48 = C3)
    int baseOctave = 48;
    int chordRoot = baseOctave + rootNote;
    
    // Get chord intervals for this chord type
    auto intervals = keyManager.getChordIntervals(def.quality);
    
    std::vector<int> chord;
    for (int interval : intervals)
    {
        chord.push_back(chordRoot + interval);
    }
    
    // Apply inversion if needed
    if (def.inversion > 0)
    {
        chord = keyManager.applyInversion(chord, def.inversion);
    }
    
    return chord;
}

void MainComponent::loadSelectedProgression()
{
    int selectedId = progressionsDropdown.getSelectedId();
    
    if (selectedId == 1) // "Select Progression..."
        return;
    
    // Clear current progression
    clearCustomProgression();
    
    // Check if we're in major or minor key
    bool isMinorKey = (keyManager.getCurrentTonality() == KeyManager::Tonality::Minor);
    
    // Define progressions using ChordDefinition (scale degree, chord type, inversion)
    // Each progression has a major version and a minor version based on the current key
    std::vector<ChordDefinition> chordDefs;
    
    switch (selectedId)
    {
        case 2: // Progression 1
            // Major: vi – V – ii – IV
            // Minor: i – VII – iv – VI
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(4, KeyManager::ChordType::Minor),      // iv
                    ChordDefinition(6, KeyManager::ChordType::Major)       // VI
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(2, KeyManager::ChordType::Minor),      // ii
                    ChordDefinition(4, KeyManager::ChordType::Major)       // IV
                };
            }
            break;

        case 3: // Progression 2
            // Major: vi⁷ – iii(add9) – IV – ii
            // Minor: i⁷ – v(add9) – VI – iv
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(5, KeyManager::ChordType::MinorAdd9),  // v(add9)
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(4, KeyManager::ChordType::Minor)       // iv
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(3, KeyManager::ChordType::MinorAdd9),  // iii(add9)
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(2, KeyManager::ChordType::Minor)       // ii
                };
            }
            break;

        case 4: // Progression 3
            // Major: vi – V(add4) – IV – I
            // Minor: i – VII(add4) – VI – III
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Add4),       // VII(add4)
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(3, KeyManager::ChordType::Major)       // III
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Add4),       // V(add4)
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(1, KeyManager::ChordType::Major)       // I
                };
            }
            break;

        case 5: // Progression 4
            // Major: vi – V(sus4) – IVmaj7 – I
            // Minor: i – VII(sus4) – VImaj7 – III
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Sus4),       // VII(sus4)
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(3, KeyManager::ChordType::Major)       // III
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Sus4),       // V(sus4)
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(1, KeyManager::ChordType::Major)       // I
                };
            }
            break;

        case 6: // Progression 5
            // Major: vi – V(sus4) – ii – IV
            // Minor: i – VII(sus4) – iv – VI
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Sus4),       // VII(sus4)
                    ChordDefinition(4, KeyManager::ChordType::Minor),      // iv
                    ChordDefinition(6, KeyManager::ChordType::Major)       // VI
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Sus4),       // V(sus4)
                    ChordDefinition(2, KeyManager::ChordType::Minor),      // ii
                    ChordDefinition(4, KeyManager::ChordType::Major)       // IV
                };
            }
            break;

        case 7: // Progression 6
            // Major: vi – I – ii⁷ – IV
            // Minor: i – III – iv⁷ – VI
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(4, KeyManager::ChordType::Minor7),     // iv⁷
                    ChordDefinition(6, KeyManager::ChordType::Major)       // VI
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(2, KeyManager::ChordType::Minor7),     // ii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major)       // IV
                };
            }
            break;

        case 8: // Progression 7 (same as 6)
            // Major: vi – I – ii⁷ – IV
            // Minor: i – III – iv⁷ – VI
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(4, KeyManager::ChordType::Minor7),     // iv⁷
                    ChordDefinition(6, KeyManager::ChordType::Major)       // VI
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(2, KeyManager::ChordType::Minor7),     // ii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major)       // IV
                };
            }
            break;

        case 9: // Progression 8
            // Major: vi – IV – I – V(add11)
            // Minor: i – VI – III – VII(add11)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(7, KeyManager::ChordType::Add11)       // VII(add11)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(5, KeyManager::ChordType::Add11)       // V(add11)
                };
            }
            break;

        case 10: // Progression 9
            // Major: vi – IV – I – ii
            // Minor: i – VI – III – iv
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(4, KeyManager::ChordType::Minor)       // iv
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(2, KeyManager::ChordType::Minor)       // ii
                };
            }
            break;

        case 11: // Progression 10 (same for major and minor - written from minor perspective)
            // i – VII – VI – VII
            chordDefs = {
                ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                ChordDefinition(7, KeyManager::ChordType::Major)       // VII
            };
            break;

        case 12: // Progression 11
            // Major: IV – vi – V – iii
            // Minor: VI – i – VII – v
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(5, KeyManager::ChordType::Minor)       // v
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(3, KeyManager::ChordType::Minor)       // iii
                };
            }
            break;

        case 13: // Progression 12
            // Major: iii⁷ – IV – V – vi
            // Minor: v⁷ – VImaj7 – VII – i
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(1, KeyManager::ChordType::Minor)       // i
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(6, KeyManager::ChordType::Minor)       // vi
                };
            }
            break;

        case 14: // Progression 13
            // Major: iii – IV – V – vi
            // Minor: v – VI – VII – i
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor),      // v
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(1, KeyManager::ChordType::Minor)       // i
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor),      // iii
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(6, KeyManager::ChordType::Minor)       // vi
                };
            }
            break;

        case 15: // Progression 14
            // Major: vi⁷ – iii⁷ – IV – ii⁷
            // Minor: i⁷ – v⁷ – VI – iv⁷
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(4, KeyManager::ChordType::Minor7)      // iv⁷
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(2, KeyManager::ChordType::Minor7)      // ii⁷
                };
            }
            break;

        case 16: // Progression 15
            // Major: vi⁷ – iii⁷ – ii⁷ – ii(add9)
            // Minor: i⁷ – v⁷ – iv⁷ – iv(add9)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(4, KeyManager::ChordType::Minor7),     // iv⁷
                    ChordDefinition(4, KeyManager::ChordType::MinorAdd9)   // iv(add9)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(2, KeyManager::ChordType::Minor7),     // ii⁷
                    ChordDefinition(2, KeyManager::ChordType::MinorAdd9)   // ii(add9)
                };
            }
            break;

        case 17: // Progression 16
            // Major: vi⁷ – iii⁷ – IV – V
            // Minor: i⁷ – v⁷ – VI – VII
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Major)       // VII
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Major)       // V
                };
            }
            break;

        case 18: // Progression 17
            // Major: vi – I – IV – IV
            // Minor: i – III – VI – VI
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(6, KeyManager::ChordType::Major)       // VI
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(4, KeyManager::ChordType::Major)       // IV
                };
            }
            break;

        case 19: // Progression 18
            // Major: vi – I(sus2) – IV – IV
            // Minor: i – III(sus2) – VI – VI
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Sus2),       // III(sus2)
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(6, KeyManager::ChordType::Major)       // VI
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Sus2),       // I(sus2)
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(4, KeyManager::ChordType::Major)       // IV
                };
            }
            break;

        case 20: // Progression 19
            // Major: vi⁷ – iii⁷ – IVmaj7 – IVmaj7
            // Minor: i⁷ – v⁷ – VImaj7 – VImaj7
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(6, KeyManager::ChordType::Major7)      // VImaj7
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(4, KeyManager::ChordType::Major7)      // IVmaj7
                };
            }
            break;

        case 21: // Progression 20
            // Major: IV – vi – I(sus2) – V
            // Minor: VI – i – III(sus2) – VII
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Sus2),       // III(sus2)
                    ChordDefinition(7, KeyManager::ChordType::Major)       // VII
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Sus2),       // I(sus2)
                    ChordDefinition(5, KeyManager::ChordType::Major)       // V
                };
            }
            break;

        case 22: // Progression 21
            // Major: IV – vi – I – V(add4)
            // Minor: VI – i – III – VII(add4)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(7, KeyManager::ChordType::Add4)        // VII(add4)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(5, KeyManager::ChordType::Add4)        // V(add4)
                };
            }
            break;

        case 23: // Progression 22
            // Major: IV – I⁶ – vi – V
            // Minor: VI – III⁶ – i – VII
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(3, KeyManager::ChordType::Sixth),      // III⁶
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Major)       // VII
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(1, KeyManager::ChordType::Sixth),      // I⁶
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Major)       // V
                };
            }
            break;

        case 24: // Progression 23
            // Major: IVmaj7 – V(add9) – vi – V(add9)
            // Minor: VImaj7 – VII(add9) – i – VII(add9)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(7, KeyManager::ChordType::Add9),       // VII(add9)
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Add9)        // VII(add9)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(5, KeyManager::ChordType::Add9),       // V(add9)
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Add9)        // V(add9)
                };
            }
            break;

        case 25: // Progression 24
            // Major: IV – V(add4) – vi – V(add4)
            // Minor: VI – VII(add4) – i – VII(add4)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Add4),       // VII(add4)
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Add4)        // VII(add4)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Add4),       // V(add4)
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Add4)        // V(add4)
                };
            }
            break;

        case 26: // Progression 25
            // Major: IV – V(add4) – vi – I
            // Minor: VI – VII(add4) – i – III
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Add4),       // VII(add4)
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(3, KeyManager::ChordType::Major)       // III
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Add4),       // V(add4)
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(1, KeyManager::ChordType::Major)       // I
                };
            }
            break;

        case 27: // Progression 26
            // Major: iii⁷ – IVmaj7 – V – vi
            // Minor: v⁷ – VImaj7 – VII – i
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(1, KeyManager::ChordType::Minor)       // i
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(6, KeyManager::ChordType::Minor)       // vi
                };
            }
            break;

        case 28: // Progression 27 (same as 24)
            // Major: IV – V(add4) – vi – V(add4)
            // Minor: VI – VII(add4) – i – VII(add4)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Add4),       // VII(add4)
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Add4)        // VII(add4)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Add4),       // V(add4)
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Add4)        // V(add4)
                };
            }
            break;

        case 29: // Progression 28
            // Major: IV – V(add9) – vi – V(add9)
            // Minor: VI – VII(add9) – i – VII(add9)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Add9),       // VII(add9)
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(7, KeyManager::ChordType::Add9)        // VII(add9)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Add9),       // V(add9)
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(5, KeyManager::ChordType::Add9)        // V(add9)
                };
            }
            break;

        case 30: // Progression 29
            // Major: iii⁷ – IVmaj7 – vi⁷ – V
            // Minor: v⁷ – VImaj7 – i⁷ – VII
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor7),     // v⁷
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(7, KeyManager::ChordType::Major)       // VII
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor7),     // iii⁷
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(5, KeyManager::ChordType::Major)       // V
                };
            }
            break;

        case 31: // Progression 30
            // Major: IV – I – V – V
            // Minor: VI – III – VII – VII
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(7, KeyManager::ChordType::Major)       // VII
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(5, KeyManager::ChordType::Major)       // V
                };
            }
            break;

        case 32: // Progression 31
            // Major: IV – I – V(add4) – vi
            // Minor: VI – III – VII(add4) – i
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(3, KeyManager::ChordType::Major),      // III
                    ChordDefinition(7, KeyManager::ChordType::Add4),       // VII(add4)
                    ChordDefinition(1, KeyManager::ChordType::Minor)       // i
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(1, KeyManager::ChordType::Major),      // I
                    ChordDefinition(5, KeyManager::ChordType::Add4),       // V(add4)
                    ChordDefinition(6, KeyManager::ChordType::Minor)       // vi
                };
            }
            break;

        case 33: // Progression 32
            // Major: IV – V(sus4) – vi – ii⁷
            // Minor: VI – VII(sus4) – i – iv⁷
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),      // VI
                    ChordDefinition(7, KeyManager::ChordType::Sus4),       // VII(sus4)
                    ChordDefinition(1, KeyManager::ChordType::Minor),      // i
                    ChordDefinition(4, KeyManager::ChordType::Minor7)      // iv⁷
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),      // IV
                    ChordDefinition(5, KeyManager::ChordType::Sus4),       // V(sus4)
                    ChordDefinition(6, KeyManager::ChordType::Minor),      // vi
                    ChordDefinition(2, KeyManager::ChordType::Minor7)      // ii⁷
                };
            }
            break;

        case 34: // Progression 33
            // Major: IVmaj7 – V – vi⁷ – ii⁷
            // Minor: VImaj7 – VII – i⁷ – iv⁷
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(4, KeyManager::ChordType::Minor7)      // iv⁷
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(2, KeyManager::ChordType::Minor7)      // ii⁷
                };
            }
            break;

        case 35: // Progression 34 (same as 33)
            // Major: IVmaj7 – V – vi⁷ – ii⁷
            // Minor: VImaj7 – VII – i⁷ – iv⁷
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major7),     // VImaj7
                    ChordDefinition(7, KeyManager::ChordType::Major),      // VII
                    ChordDefinition(1, KeyManager::ChordType::Minor7),     // i⁷
                    ChordDefinition(4, KeyManager::ChordType::Minor7)      // iv⁷
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major7),     // IVmaj7
                    ChordDefinition(5, KeyManager::ChordType::Major),      // V
                    ChordDefinition(6, KeyManager::ChordType::Minor7),     // vi⁷
                    ChordDefinition(2, KeyManager::ChordType::Minor7)      // ii⁷
                };
            }
            break;
        
        default:
            return;
    }
    
    // Generate MIDI notes from chord definitions in the current key
    for (const auto& def : chordDefs)
    {
        std::vector<int> midiNotes = generateChordFromDefinition(def);
        
        // Store the MIDI notes
        currentProgression.push_back(midiNotes);
        
        // Store degree, alteration, and inversion
        customProgressionDegrees.push_back(def.scaleDegree);
        customProgressionAlterations.push_back(def.quality);
        customProgressionInversions.push_back(def.inversion);
    }
    
    // Resize the emotion tracking arrays
    hasEmotionApplied.clear();
    hasEmotionApplied.resize(customProgressionDegrees.size(), false);
    customProgressionEmotions.clear();
    customProgressionEmotions.resize(customProgressionDegrees.size());
    
    // Update the display
    updateCustomProgressionDisplay();
    
    DBG("Loaded progression: " << progressionsDropdown.getText() << " with " << currentProgression.size() << " chords");
}

//==============================================================================
// Old hardcoded loadSelectedProgression - REMOVED
// Keeping the old function commented out for reference
/*
void MainComponent::loadSelectedProgression()
{
    int selectedId = progressionsDropdown.getSelectedId();
    
    if (selectedId == 1) // "Select Progression..."
        return;
    
    // Clear current progression
    clearCustomProgression();
    
    // Define analyzed progressions with full chord data (degree, quality, inversion)
    std::vector<AnalyzedChord> analyzedChords;
    
    switch (selectedId)
    {
        case 2: // Pretty Uplifting - F-C-A (vi), A-E-A-C (I), G-D-A-B (vii), E-B-G-A (V)
            {
                AnalyzedChord c1; c1.midiNotes = {41, 48, 57}; c1.scaleDegree = 6; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {45, 52, 57, 60}; c2.scaleDegree = 1; c2.quality = KeyManager::ChordType::Minor; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {43, 50, 57, 59}; c3.scaleDegree = 7; c3.quality = KeyManager::ChordType::Diminished; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {40, 47, 55, 57}; c4.scaleDegree = 5; c4.quality = KeyManager::ChordType::Major; c4.inversion = 0;
                analyzedChords = {c1, c2, c3, c4};
            }
            break;
            
        case 3: // Pretty Uplifting 2 - A-E-C, C-G-E, F-C-A, F-C-A
            {
                AnalyzedChord c1; c1.midiNotes = {45, 52, 60}; c1.scaleDegree = 1; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {48, 55, 64}; c2.scaleDegree = 4; c2.quality = KeyManager::ChordType::Major; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {41, 48, 57}; c3.scaleDegree = 6; c3.quality = KeyManager::ChordType::Minor; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {41, 48, 57}; c4.scaleDegree = 6; c4.quality = KeyManager::ChordType::Minor; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {45, 52, 60}; c5.scaleDegree = 1; c5.quality = KeyManager::ChordType::Minor; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {48, 55, 62}; c6.scaleDegree = 4; c6.quality = KeyManager::ChordType::Sus2; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {41, 48, 57}; c7.scaleDegree = 6; c7.quality = KeyManager::ChordType::Minor; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {41, 48, 57}; c8.scaleDegree = 6; c8.quality = KeyManager::ChordType::Minor; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 4: // Pretty Uplifting 3 - F-C-A, A-E-C, C-G-D, G-D-B
            {
                AnalyzedChord c1; c1.midiNotes = {41, 48, 57}; c1.scaleDegree = 6; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {45, 52, 60}; c2.scaleDegree = 1; c2.quality = KeyManager::ChordType::Minor; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {48, 55, 62}; c3.scaleDegree = 4; c3.quality = KeyManager::ChordType::Major; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {43, 50, 59}; c4.scaleDegree = 7; c4.quality = KeyManager::ChordType::Diminished; c4.inversion = 0;
                analyzedChords = {c1, c2, c3, c4};
            }
            break;
            
        case 5: // Pretty Uplifting 4 - F-C-A, A-C-E-C, C-G-E, G-C-D-B, F-C-A (8 chords)
            {
                AnalyzedChord c1; c1.midiNotes = {41, 48, 57}; c1.scaleDegree = 6; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {45, 48, 52, 60}; c2.scaleDegree = 1; c2.quality = KeyManager::ChordType::Minor; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {48, 55, 64}; c3.scaleDegree = 4; c3.quality = KeyManager::ChordType::Major; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {43, 48, 50, 59}; c4.scaleDegree = 7; c4.quality = KeyManager::ChordType::Sus4; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {53, 60, 69}; c5.scaleDegree = 4; c5.quality = KeyManager::ChordType::Major; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {52, 60, 67}; c6.scaleDegree = 3; c6.quality = KeyManager::ChordType::Minor; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {45, 57, 60, 64}; c7.scaleDegree = 1; c7.quality = KeyManager::ChordType::Minor; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {43, 55, 59, 62}; c8.scaleDegree = 7; c8.quality = KeyManager::ChordType::Major; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 6: // Pretty Uplifting 5 - F-C-E, G-D-E, A-C-E, G-D-E (8 chords)
            {
                AnalyzedChord c1; c1.midiNotes = {53, 60, 64}; c1.scaleDegree = 4; c1.quality = KeyManager::ChordType::Major; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {55, 62, 64}; c2.scaleDegree = 5; c2.quality = KeyManager::ChordType::Sus2; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {57, 60, 64}; c3.scaleDegree = 6; c3.quality = KeyManager::ChordType::Minor; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {55, 62, 64}; c4.scaleDegree = 5; c4.quality = KeyManager::ChordType::Sus2; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {41, 53, 57, 60}; c5.scaleDegree = 6; c5.quality = KeyManager::ChordType::Minor; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {43, 55, 59, 60}; c6.scaleDegree = 7; c6.quality = KeyManager::ChordType::Major; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {45, 57, 60}; c7.scaleDegree = 1; c7.quality = KeyManager::ChordType::Minor; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {43, 55, 59, 60}; c8.scaleDegree = 7; c8.quality = KeyManager::ChordType::Major; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 7: // Pretty Uplifting 6 - 8 chords
            {
                AnalyzedChord c1; c1.midiNotes = {40, 50, 55, 59}; c1.scaleDegree = 5; c1.quality = KeyManager::ChordType::Sus2; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {41, 52, 57, 60}; c2.scaleDegree = 6; c2.quality = KeyManager::ChordType::Minor; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {43, 50, 55, 59}; c3.scaleDegree = 7; c3.quality = KeyManager::ChordType::Major; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {45, 52, 57, 60}; c4.scaleDegree = 1; c4.quality = KeyManager::ChordType::Minor; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {41, 48, 57}; c5.scaleDegree = 6; c5.quality = KeyManager::ChordType::Minor; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {43, 48, 50, 59}; c6.scaleDegree = 7; c6.quality = KeyManager::ChordType::Sus4; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {45, 48, 52, 60}; c7.scaleDegree = 1; c7.quality = KeyManager::ChordType::Minor; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {43, 48, 50, 59}; c8.scaleDegree = 7; c8.quality = KeyManager::ChordType::Sus4; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 8: // Pretty Uplifting 7 - 8 chords
            {
                AnalyzedChord c1; c1.midiNotes = {53, 60, 69}; c1.scaleDegree = 4; c1.quality = KeyManager::ChordType::Major; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {60, 64, 67, 76}; c2.scaleDegree = 1; c2.quality = KeyManager::ChordType::Major7; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {55, 62, 71}; c3.scaleDegree = 5; c3.quality = KeyManager::ChordType::Major; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {55, 62, 71}; c4.scaleDegree = 5; c4.quality = KeyManager::ChordType::Major; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {53, 60, 69}; c5.scaleDegree = 4; c5.quality = KeyManager::ChordType::Major; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {60, 64, 67, 76}; c6.scaleDegree = 1; c6.quality = KeyManager::ChordType::Major7; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {55, 60, 62, 71}; c7.scaleDegree = 5; c7.quality = KeyManager::ChordType::Sus2; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {57, 60, 64, 72}; c8.scaleDegree = 6; c8.quality = KeyManager::ChordType::Minor; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 9: // Pretty Uplifting 8 - F-A-C, G-C-D, A-C-E, D-A-C-F, F-A-C-E, G-B-D, A-C-E-G, D-A-C-F
            {
                AnalyzedChord c1; c1.midiNotes = {53, 57, 60}; c1.scaleDegree = 4; c1.quality = KeyManager::ChordType::Major; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {55, 60, 62}; c2.scaleDegree = 5; c2.quality = KeyManager::ChordType::Sus2; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {57, 60, 64}; c3.scaleDegree = 6; c3.quality = KeyManager::ChordType::Minor; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {50, 57, 60, 65}; c4.scaleDegree = 2; c4.quality = KeyManager::ChordType::Minor7; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {53, 57, 60, 64}; c5.scaleDegree = 4; c5.quality = KeyManager::ChordType::Major7; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {55, 59, 62}; c6.scaleDegree = 5; c6.quality = KeyManager::ChordType::Major; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {57, 60, 64, 67}; c7.scaleDegree = 6; c7.quality = KeyManager::ChordType::Minor7; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {50, 57, 60, 65}; c8.scaleDegree = 2; c8.quality = KeyManager::ChordType::Minor7; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 10: // Melancholic - A-C-E-A (i), G-B-D-A (VII), F-A-C-A (VI), G-B-D-A (VII)
            {
                AnalyzedChord c1; c1.midiNotes = {45, 48, 52, 57}; c1.scaleDegree = 1; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {43, 47, 50, 57}; c2.scaleDegree = 7; c2.quality = KeyManager::ChordType::Major; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {41, 45, 48, 57}; c3.scaleDegree = 6; c3.quality = KeyManager::ChordType::Major; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {43, 47, 50, 57}; c4.scaleDegree = 7; c4.quality = KeyManager::ChordType::Major; c4.inversion = 0;
                analyzedChords = {c1, c2, c3, c4};
            }
            break;
            
        case 11: // Jazzy 1 - 8 chords with extended voicings
            {
                AnalyzedChord c1; c1.midiNotes = {40, 43, 47, 50, 55}; c1.scaleDegree = 5; c1.quality = KeyManager::ChordType::Minor7; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {41, 45, 48, 52, 57}; c2.scaleDegree = 6; c2.quality = KeyManager::ChordType::Major7; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {43, 50, 55, 59}; c3.scaleDegree = 7; c3.quality = KeyManager::ChordType::Sus2; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {45, 52, 57, 60}; c4.scaleDegree = 1; c4.quality = KeyManager::ChordType::Minor; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {40, 47, 55}; c5.scaleDegree = 5; c5.quality = KeyManager::ChordType::Major; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {41, 48, 57}; c6.scaleDegree = 6; c6.quality = KeyManager::ChordType::Minor; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {43, 50, 59}; c7.scaleDegree = 7; c7.quality = KeyManager::ChordType::Diminished; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {45, 52, 60}; c8.scaleDegree = 1; c8.quality = KeyManager::ChordType::Minor; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 12: // Jazzy 2 - 8 chords
            {
                AnalyzedChord c1; c1.midiNotes = {45, 52, 55, 60}; c1.scaleDegree = 1; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {40, 47, 50, 55}; c2.scaleDegree = 5; c2.quality = KeyManager::ChordType::Sus2; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {41, 48, 52, 57}; c3.scaleDegree = 6; c3.quality = KeyManager::ChordType::Major; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {38, 45, 48, 53}; c4.scaleDegree = 4; c4.quality = KeyManager::ChordType::Minor; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {45, 52, 55, 60}; c5.scaleDegree = 1; c5.quality = KeyManager::ChordType::Minor; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {40, 47, 50, 55}; c6.scaleDegree = 5; c6.quality = KeyManager::ChordType::Sus2; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {38, 45, 48, 53}; c7.scaleDegree = 4; c7.quality = KeyManager::ChordType::Minor; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {38, 45, 48, 52}; c8.scaleDegree = 4; c8.quality = KeyManager::ChordType::Minor; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
            
        case 13: // Pop Club House 1 - 8 chords
            {
                AnalyzedChord c1; c1.midiNotes = {45, 52, 60}; c1.scaleDegree = 1; c1.quality = KeyManager::ChordType::Minor; c1.inversion = 0;
                AnalyzedChord c2; c2.midiNotes = {43, 50, 59}; c2.scaleDegree = 7; c2.quality = KeyManager::ChordType::Diminished; c2.inversion = 0;
                AnalyzedChord c3; c3.midiNotes = {38, 45, 53, 57}; c3.scaleDegree = 4; c3.quality = KeyManager::ChordType::Minor; c3.inversion = 0;
                AnalyzedChord c4; c4.midiNotes = {41, 48, 53, 57}; c4.scaleDegree = 6; c4.quality = KeyManager::ChordType::Minor; c4.inversion = 0;
                AnalyzedChord c5; c5.midiNotes = {45, 52, 57, 60}; c5.scaleDegree = 1; c5.quality = KeyManager::ChordType::Minor; c5.inversion = 0;
                AnalyzedChord c6; c6.midiNotes = {40, 47, 55, 57}; c6.scaleDegree = 5; c6.quality = KeyManager::ChordType::Major; c6.inversion = 0;
                AnalyzedChord c7; c7.midiNotes = {41, 48, 57}; c7.scaleDegree = 6; c7.quality = KeyManager::ChordType::Minor; c7.inversion = 0;
                AnalyzedChord c8; c8.midiNotes = {38, 45, 53, 57}; c8.scaleDegree = 4; c8.quality = KeyManager::ChordType::Minor; c8.inversion = 0;
                analyzedChords = {c1, c2, c3, c4, c5, c6, c7, c8};
            }
            break;
*/

//==============================================================================
// Chord Analysis Functions

MainComponent::AnalyzedChord MainComponent::analyzeChord(const std::vector<int>& midiNotes, int keyRoot)
{
    AnalyzedChord result;
    result.midiNotes = midiNotes;
    
    if (midiNotes.empty())
        return result;
    
    // Sort notes to find intervals
    std::vector<int> sortedNotes = midiNotes;
    std::sort(sortedNotes.begin(), sortedNotes.end());
    
    // Get the bass note (lowest note)
    int bassNote = sortedNotes[0] % 12;
    
    // Find the root note by analyzing intervals
    // For now, assume bass is root (we'll detect inversions later)
    int assumedRoot = bassNote;
    
    // Calculate intervals from assumed root
    std::vector<int> intervals;
    for (size_t i = 1; i < sortedNotes.size(); ++i)
    {
        int interval = (sortedNotes[i] - sortedNotes[0]) % 12;
        if (interval > 0)
            intervals.push_back(interval);
    }
    
    // Detect chord quality
    result.quality = detectChordQuality(intervals);
    
    // Detect inversion
    result.inversion = detectInversion(sortedNotes, intervals);
    
    // If inverted, recalculate root
    int rootNote = assumedRoot;
    if (result.inversion == 1 && intervals.size() >= 1)
    {
        // First inversion: bass is the 3rd, so root is 3rd or 4th below
        rootNote = (bassNote - intervals[0] + 12) % 12;
    }
    else if (result.inversion == 2 && intervals.size() >= 2)
    {
        // Second inversion: bass is the 5th, so root is 5th or 7th below
        rootNote = (bassNote - intervals[1] + 12) % 12;
    }
    
    // Detect scale degree
    bool isMajorKey = (keyManager.getCurrentTonality() == KeyManager::Tonality::Major);
    result.scaleDegree = detectScaleDegree(rootNote, keyRoot, isMajorKey);
    
    return result;
}

KeyManager::ChordType MainComponent::detectChordQuality(const std::vector<int>& intervals)
{
    if (intervals.empty())
        return KeyManager::ChordType::Major;
    
    // Sort intervals for consistent matching
    std::vector<int> sorted = intervals;
    std::sort(sorted.begin(), sorted.end());
    
    // Match against known chord patterns
    if (sorted.size() >= 2)
    {
        int third = sorted[0];
        int fifth = sorted[1];
        
        // Triads
        if (third == 4 && fifth == 7)
            return KeyManager::ChordType::Major;
        else if (third == 3 && fifth == 7)
            return KeyManager::ChordType::Minor;
        else if (third == 3 && fifth == 6)
            return KeyManager::ChordType::Diminished;
        else if (third == 4 && fifth == 8)
            return KeyManager::ChordType::Augmented;
        else if (third == 2 && fifth == 7)
            return KeyManager::ChordType::Sus2;
        else if (third == 5 && fifth == 7)
            return KeyManager::ChordType::Sus4;
        
        // Check for 7ths and extensions
        if (sorted.size() >= 3)
        {
            int seventh = sorted[2];
            
            // 7th chords
            if (third == 4 && fifth == 7 && seventh == 11)
                return KeyManager::ChordType::Major7;
            else if (third == 3 && fifth == 7 && seventh == 10)
                return KeyManager::ChordType::Minor7;
            else if (third == 4 && fifth == 7 && seventh == 10)
                return KeyManager::ChordType::Dominant7;
            else if (third == 3 && fifth == 6 && seventh == 10)
                return KeyManager::ChordType::HalfDiminished7;
            else if (third == 3 && fifth == 6 && seventh == 9)
                return KeyManager::ChordType::Diminished7;
            
            // Check for 9ths
            if (sorted.size() >= 4)
            {
                int ninth = sorted[3];
                if (ninth == 14 || ninth == 2)  // 9th can be in same or next octave
                {
                    if (third == 4 && fifth == 7 && seventh == 11)
                        return KeyManager::ChordType::Major9;
                    else if (third == 3 && fifth == 7 && seventh == 10)
                        return KeyManager::ChordType::Minor9;
                    else if (third == 4 && fifth == 7 && seventh == 10)
                        return KeyManager::ChordType::Dominant9;
                }
            }
        }
    }
    
    return KeyManager::ChordType::Major;  // Default
}

int MainComponent::detectInversion(const std::vector<int>& midiNotes, const std::vector<int>& intervals)
{
    if (intervals.size() < 2)
        return 0;  // Root position
    
    // Check interval patterns to detect inversions
    int lowest = intervals[0];
    
    // First inversion: lowest interval is 3rd or 4th (third in bass)
    if (lowest == 3 || lowest == 4)
    {
        // Verify by checking if next interval suggests root position
        if (intervals.size() >= 2)
        {
            int second = intervals[1];
            // If we have 3rd then 5th-3rd=minor3rd(3) or major3rd(4), it's 1st inv
            if ((lowest == 3 && (second == 6 || second == 7)) ||
                (lowest == 4 && (second == 7 || second == 8)))
                return 1;
        }
    }
    
    // Second inversion: lowest interval is 5th, 6th, or 7th (fifth in bass)
    if (lowest == 5 || lowest == 6 || lowest == 7)
    {
        if (intervals.size() >= 2)
        {
            int second = intervals[1];
            // If we have 5th then a 3rd or 4th above, it's 2nd inversion
            if (second == 8 || second == 9 || second == 10 || second == 11)
                return 2;
        }
    }
    
    return 0;  // Root position
}

int MainComponent::detectScaleDegree(int rootNote, int keyRoot, bool isMajorKey)
{
    // Calculate the interval from key root to chord root
    int interval = (rootNote - keyRoot + 12) % 12;
    
    // Map interval to scale degree based on key type
    if (isMajorKey)
    {
        // Major scale: C D E F G A B
        switch (interval)
        {
            case 0: return 1;   // I
            case 2: return 2;   // II
            case 4: return 3;   // III
            case 5: return 4;   // IV
            case 7: return 5;   // V
            case 9: return 6;   // VI
            case 11: return 7;  // VII
            default: return 1;  // Chromatic note, default to I
        }
    }
    else
    {
        // Natural minor scale: A B C D E F G
        switch (interval)
        {
            case 0: return 1;   // i
            case 2: return 2;   // ii
            case 3: return 3;   // III
            case 5: return 4;   // iv
            case 7: return 5;   // v
            case 8: return 6;   // VI
            case 10: return 7;  // VII
            default: return 1;  // Chromatic note, default to i
        }
    }
}

//==============================================================================
// Sample loading functions
void MainComponent::loadSamples(const juce::File& sampleDirectory)
{
    DBG("Loading samples from: " + sampleDirectory.getFullPathName());
    
    // Clear existing samples
    synth.clearSounds();
    sampleCache.clear();
    
    // Look for AIF files in the directory
    juce::Array<juce::File> audioFiles;
    sampleDirectory.findChildFiles(audioFiles, juce::File::findFiles, false, "*.aif");
    
    if (audioFiles.isEmpty())
    {
        sampleDirectory.findChildFiles(audioFiles, juce::File::findFiles, false, "*.aiff");
    }
    
    if (audioFiles.isEmpty())
    {
        DBG("No AIF/AIFF files found in assets folder");
        return;
    }
    
    DBG("Found " + juce::String(audioFiles.size()) + " audio files");
    
    // Try to parse filenames for MIDI note numbers
    // Expected formats: "60.aif", "C4.aif", "note60.aif", etc.
    for (const auto& file : audioFiles)
    {
        juce::String filename = file.getFileNameWithoutExtension();
        int midiNote = -1;
        
        // Try parsing as pure number first
        if (filename.containsOnly("0123456789"))
        {
            midiNote = filename.getIntValue();
        }
        // Try note name format (C4, D#3, etc.)
        else if (filename.length() >= 2)
        {
            midiNote = parseMidiNoteFromName(filename);
        }
        // Try "note60" format
        else if (filename.startsWith("note"))
        {
            midiNote = filename.substring(4).getIntValue();
        }
        
        if (midiNote >= 0 && midiNote <= 127)
        {
            loadSample(midiNote, file);
        }
        else
        {
            DBG("Could not parse MIDI note from filename: " + filename);
        }
    }
    
    DBG("Loaded " + juce::String(sampleCache.size()) + " samples");
}

void MainComponent::loadSample(int midiNote, const juce::File& audioFile)
{
    if (auto* reader = formatManager.createReaderFor(audioFile))
    {
        auto sampleLength = static_cast<int>(reader->lengthInSamples);
        double durationSeconds = sampleLength / reader->sampleRate;
        
        DBG("Loading MIDI " << midiNote << ": " << audioFile.getFileName() 
            << " - " << sampleLength << " samples, " 
            << reader->sampleRate << " Hz, "
            << durationSeconds << " seconds");
        
        // Create buffer and read audio data
        auto buffer = std::make_unique<juce::AudioBuffer<float>>(
            static_cast<int>(reader->numChannels),
            sampleLength + 4  // Add a few samples for interpolation safety
        );
        
        reader->read(buffer.get(), 0, sampleLength, 0, true, true);
        
        DBG("  Loaded successfully");
        
        // Create sampler sound
        juce::BigInteger notes;
        notes.setBit(midiNote);
        
        synth.addSound(new juce::SamplerSound(
            juce::String(midiNote),
            *reader,
            notes,
            midiNote,  // root note
            0.0,       // attack time
            0.1,       // release time
            60.0       // max sample length in seconds
        ));
        
        // Cache the buffer
        sampleCache[midiNote] = std::move(buffer);
        
        delete reader;
    }
    else
    {
        DBG("ERROR: Could not load " << audioFile.getFileName());
    }
}

// Helper function to parse note names like "C4", "D#3", "Bb2"
int MainComponent::parseMidiNoteFromName(const juce::String& noteName)
{
    if (noteName.length() < 2)
        return -1;
    
    // Note to semitone mapping (C=0, C#=1, D=2, etc.)
    std::map<juce::String, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"Db", 1},
        {"D", 2}, {"D#", 3}, {"Eb", 3},
        {"E", 4},
        {"F", 5}, {"F#", 6}, {"Gb", 6},
        {"G", 7}, {"G#", 8}, {"Ab", 8},
        {"A", 9}, {"A#", 10}, {"Bb", 10},
        {"B", 11}
    };
    
    // Extract note letter and octave
    juce::String note;
    int octave = -1;
    
    // Check for sharp/flat
    if (noteName.length() >= 3 && (noteName[1] == '#' || noteName[1] == 'b'))
    {
        note = noteName.substring(0, 2);
        octave = noteName.substring(2).getIntValue();
    }
    else
    {
        note = noteName.substring(0, 1);
        octave = noteName.substring(1).getIntValue();
    }
    
    if (noteMap.count(note) > 0 && octave >= -1 && octave <= 9)
    {
        return (octave + 1) * 12 + noteMap[note];
    }
    
    return -1;
}
