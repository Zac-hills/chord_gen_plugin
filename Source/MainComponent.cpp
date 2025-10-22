#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : keyboard(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    keyComboBox.addItem("C Major", 1);
    keyComboBox.addItem("C# Major", 2);
    keyComboBox.addItem("D Major", 3);
    keyComboBox.addItem("D# Major", 4);
    keyComboBox.addItem("E Major", 5);
    keyComboBox.addItem("F Major", 6);
    keyComboBox.addItem("F# Major", 7);
    keyComboBox.addItem("G Major", 8);
    keyComboBox.addItem("G# Major", 9);
    keyComboBox.addItem("A Major", 10);
    keyComboBox.addItem("A# Major", 11);
    keyComboBox.addItem("B Major", 12);
    
    keyComboBox.setSelectedId(1);
    keyComboBox.onChange = [this] { keySelectionChanged(); };
    addAndMakeVisible(keyComboBox);
    
    // Add progression combo box
    auto progressions = keyManager.getAvailableProgressions();
    progressionComboBox.addItem("Select Progression", 1);
    for (int i = 0; i < progressions.size(); ++i)
    {
        progressionComboBox.addItem(juce::String(progressions[i]), i + 2);
    }
    progressionComboBox.setSelectedId(1);
    progressionComboBox.onChange = [this] { progressionSelectionChanged(); };
    addAndMakeVisible(progressionComboBox);
    
    // Add chord type combo box
    chordTypeComboBox.addItem("Triads", 1);
    chordTypeComboBox.addItem("Seventh Chords", 2);
    chordTypeComboBox.setSelectedId(1);
    chordTypeComboBox.onChange = [this] { updateDisplay(); };
    addAndMakeVisible(chordTypeComboBox);
    
    scaleNotesLabel.setText("Scale Notes: ", juce::dontSendNotification);
    scaleNotesLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(scaleNotesLabel);
    
    chordsLabel.setText("Chords: ", juce::dontSendNotification);
    chordsLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(chordsLabel);
    
    progressionLabel.setText("Progression: ", juce::dontSendNotification);
    progressionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(progressionLabel);
    
    // Initialize MIDI components
    keyboard.setKeyWidth(40.0f);
    addAndMakeVisible(keyboard);
    
    playButton.setButtonText("Play");
    playButton.onClick = [this] { playProgression(); };
    addAndMakeVisible(playButton);
    
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopProgression(); };
    addAndMakeVisible(stopButton);
    
    tempoSlider.setRange(60.0, 200.0);
    tempoSlider.setValue(120.0);
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(tempoSlider);
    
    tempoLabel.setText("Tempo (BPM):", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);
    
    audioSettingsButton.setButtonText("Audio Settings");
    audioSettingsButton.onClick = [this] { showAudioSettings(); };
    addAndMakeVisible(audioSettingsButton);
    
    testToneButton.setButtonText("Test Tone");
    testToneButton.onClick = [this] { 
        // Play middle C for testing
        keyboardState.noteOn(1, 60, 0.7f);
        juce::Timer::callAfterDelay(1000, [this]() {
            keyboardState.noteOff(1, 60, 0.0f);
        });
    };
    addAndMakeVisible(testToneButton);
    
    refreshAudioButton.setButtonText("Refresh Audio");
    refreshAudioButton.onClick = [this] { 
        DBG("=== Manual Audio Refresh ===");
        detectSystemAudioDevices();
        tryInitializeAudioDevice();
    };
    addAndMakeVisible(refreshAudioButton);
    
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
}

MainComponent::~MainComponent()
{
    shutdownAudio();
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
                    
                    // Update tempo-based timing
                    double currentTempo = tempoSlider.getValue();
                    samplesPerBeat = static_cast<int>((60.0 / currentTempo) * sampleRate);
                    samplesUntilNextChord = samplesPerBeat;
                }
                else
                {
                    // End of progression
                    stopProgression();
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
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("Key Manager - Chord Generator", getLocalBounds().removeFromTop(50), 
               juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Space for title
    
    auto topSection = bounds.removeFromTop(100);
    keyComboBox.setBounds(topSection.removeFromLeft(200).reduced(10));
    chordTypeComboBox.setBounds(topSection.removeFromLeft(200).reduced(10));
    progressionComboBox.setBounds(topSection.reduced(10));
    
    bounds.removeFromTop(20);
    
    // Control buttons and tempo
    auto controlSection = bounds.removeFromTop(50);
    playButton.setBounds(controlSection.removeFromLeft(80).reduced(5));
    stopButton.setBounds(controlSection.removeFromLeft(80).reduced(5));
    
    auto tempoSection = controlSection.removeFromLeft(200).reduced(5);
    tempoLabel.setBounds(tempoSection.removeFromTop(20));
    tempoSlider.setBounds(tempoSection);
    
    audioSettingsButton.setBounds(controlSection.removeFromLeft(120).reduced(5));
    testToneButton.setBounds(controlSection.removeFromLeft(100).reduced(5));
    refreshAudioButton.setBounds(controlSection.removeFromLeft(120).reduced(5));
    
    bounds.removeFromTop(20);
    
    scaleNotesLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    
    chordsLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    
    progressionLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(20);
    
    // MIDI keyboard at the bottom
    keyboard.setBounds(bounds.removeFromBottom(80));
}

void MainComponent::keySelectionChanged()
{
    int selectedKey = keyComboBox.getSelectedId() - 1;
    keyManager.setCurrentKey(static_cast<KeyManager::Key>(selectedKey));
    updateDisplay();
}

void MainComponent::progressionSelectionChanged()
{
    updateDisplay();
}

void MainComponent::updateDisplay()
{
    // Update scale notes
    auto scaleNoteNames = keyManager.getScaleNoteNames();
    juce::String scaleText = "Scale Notes: ";
    for (int i = 0; i < scaleNoteNames.size(); ++i)
    {
        scaleText += juce::String(scaleNoteNames[i]);
        if (i < scaleNoteNames.size() - 1)
            scaleText += " - ";
    }
    scaleNotesLabel.setText(scaleText, juce::dontSendNotification);
    
    // Update chords based on selected type
    juce::String chordsText = "Chords: ";
    bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
    
    for (int i = 1; i <= 7; ++i)
    {
        auto degree = static_cast<KeyManager::ScaleDegree>(i);
        auto chordType = useSevenths ? keyManager.analyzeSeventh(degree) : keyManager.analyzeTriad(degree);
        std::string chordName = keyManager.getChordName(degree, chordType);
        
        chordsText += juce::String(chordName);
        if (i < 7)
            chordsText += " - ";
    }
    chordsLabel.setText(chordsText, juce::dontSendNotification);
    
    // Update progression if selected
    if (progressionComboBox.getSelectedId() > 1)
    {
        auto progressions = keyManager.getAvailableProgressions();
        int progressionIndex = progressionComboBox.getSelectedId() - 2;
        
        if (progressionIndex >= 0 && progressionIndex < progressions.size())
        {
            bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
            auto progression = keyManager.getCommonProgression(progressions[progressionIndex], useSevenths);
            juce::String progressionText = "Progression (" + juce::String(progressions[progressionIndex]) + "): ";
            
            for (int i = 0; i < progression.size(); ++i)
            {
                if (!progression[i].empty())
                {
                    // Get the root note name - convert MIDI note to pitch class
                    auto chromaticNames = keyManager.getChromaticNoteNames();
                    int pitchClass = progression[i][0] % 12; // Convert MIDI note to pitch class (0-11)
                    juce::String rootNote = juce::String(chromaticNames[pitchClass]);
                    progressionText += rootNote;
                    
                    if (i < progression.size() - 1)
                        progressionText += " - ";
                }
            }
            progressionLabel.setText(progressionText, juce::dontSendNotification);
        }
    }
    else
    {
        progressionLabel.setText("Progression: Select a progression above", juce::dontSendNotification);
    }
}

//==============================================================================
// MIDI Playback Methods

void MainComponent::playProgression()
{
    if (progressionComboBox.getSelectedId() > 1)
    {
        stopProgression(); // Stop any current playback
        
        auto progressions = keyManager.getAvailableProgressions();
        int progressionIndex = progressionComboBox.getSelectedId() - 2;
        
        if (progressionIndex >= 0 && progressionIndex < progressions.size())
        {
            bool useSevenths = chordTypeComboBox.getSelectedId() == 2;
            currentProgression = keyManager.getCommonProgression(progressions[progressionIndex], useSevenths);
            currentChordIndex = 0;
            isPlaying = true;
            
            // Calculate initial timing
            double currentTempo = tempoSlider.getValue();
            samplesPerBeat = static_cast<int>((60.0 / currentTempo) * sampleRate);
            samplesUntilNextChord = 0; // Start immediately
        }
    }
}

void MainComponent::stopProgression()
{
    isPlaying = false;
    currentChordIndex = 0;
    currentProgression.clear();
    stopCurrentChord();
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