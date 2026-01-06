#pragma once

#include <JuceHeader.h>
#include "KeyManager.h"
#include "ThemeManager.h"  // Temporarily disabled
#include "EmotionWheel.h"

//==============================================================================
// Custom LookAndFeel for circular button
class CircularButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour(baseColour);
        g.fillEllipse(bounds);
        
        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawEllipse(bounds.reduced(1.0f), 2.0f);
    }
};

//==============================================================================
// Custom LookAndFeel for square button
class SquareButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour(baseColour);
        g.fillRect(bounds);
        
        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawRect(bounds.reduced(1.0f), 2.0f);
    }
};

//==============================================================================
// Button with badge component
class ButtonWithBadge : public juce::Component
{
public:
    ButtonWithBadge()
    {
        addAndMakeVisible(mainButton);
        addAndMakeVisible(badgeButton);
        
        // Make main button square
        mainButton.setLookAndFeel(&squareLookAndFeel);
        
        // Make badge circular and set "x" text
        badgeButton.setButtonText("x");
        badgeButton.setLookAndFeel(&circularLookAndFeel);
    }
    
    ~ButtonWithBadge() noexcept override
    {
        mainButton.setLookAndFeel(nullptr);
        badgeButton.setLookAndFeel(nullptr);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        mainButton.setBounds(bounds);
        
        // Position small badge at top-left
        int badgeSize = 16;
        badgeButton.setBounds(0, 0, badgeSize, badgeSize);
    }
    
    juce::TextButton mainButton;
    juce::TextButton badgeButton;
    
private:
    SquareButtonLookAndFeel squareLookAndFeel;
    CircularButtonLookAndFeel circularLookAndFeel;
};

//==============================================================================
// Split button component with main action and play button
class SplitButton : public juce::Component
{
public:
    SplitButton()
    {
        addAndMakeVisible(mainButton);
        addAndMakeVisible(playButton);
        
        // Set play button text to simple ASCII character
        playButton.setButtonText(">");
        
        // Override play button to trigger on mouse down instead of click
        playButton.setTriggeredOnMouseDown(true);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        int playWidth = 20;  // Thin play button
        
        // Main button takes most of the space
        mainButton.setBounds(bounds.removeFromLeft(bounds.getWidth() - playWidth));
        
        // Play button takes the remaining thin strip on the right
        playButton.setBounds(bounds);
    }
    
    juce::TextButton mainButton;
    juce::TextButton playButton;
};

//==============================================================================
// Forward declaration
class SineWaveSound;

//==============================================================================
// Waveform types
enum class WaveformType
{
    Sine,
    Sawtooth,
    Square,
    Triangle
};

//==============================================================================
// Simple Sampler Voice - just plays the sample file naturally
class SimpleSamplerVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<const juce::SamplerSound*>(sound) != nullptr;
    }
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* s, int /*currentPitchWheelPosition*/) override
    {
        if (auto* sound = dynamic_cast<const juce::SamplerSound*>(s))
        {
            pitchRatio = 1.0;
            sourceSamplePosition = 0.0;
            lgain = velocity;
            rgain = velocity;
            // Note: gainMultiplier should be set via setGainMultiplier() before calling noteOn
            // Don't reset it here or it will override the value we just set
            
            adsr.setSampleRate(getSampleRate());
            adsr.setParameters({0.01, 0.0, 1.0, 0.1}); // Quick attack, full sustain, quick release
            adsr.noteOn();
            
            audioData = sound->getAudioData();
            DBG("SimpleSamplerVoice: Playing note " << midiNoteNumber << " with gain multiplier " << gainMultiplier);
        }
    }
    
    void setGainMultiplier(float multiplier)
    {
        gainMultiplier = multiplier;
    }
    
    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
        }
        // Reset gain multiplier when note stops so voice is clean for next use
        gainMultiplier = 1.0f;
    }
    
    void pitchWheelMoved(int /*newValue*/) override {}
    void controllerMoved(int /*controllerNumber*/, int /*newValue*/) override {}
    
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (audioData == nullptr || !adsr.isActive())
        {
            clearCurrentNote();
            return;
        }
        
        auto numChannels = juce::jmin(outputBuffer.getNumChannels(), audioData->getNumChannels());
        auto dataLength = audioData->getNumSamples();
        
        while (--numSamples >= 0)
        {
            auto pos = static_cast<int>(sourceSamplePosition);
            auto envelopeValue = adsr.getNextSample();
            
            if (pos >= dataLength - 1)
            {
                // Sample finished - stop the note
                clearCurrentNote();
                adsr.reset();
                return;
            }
            
            // Get interpolated sample from audio data
            auto alpha = static_cast<float>(sourceSamplePosition - pos);
            auto invAlpha = 1.0f - alpha;
            auto nextPos = pos + 1;
            
            auto* channelDataL = audioData->getReadPointer(0);
            float sampleL = channelDataL[pos] * invAlpha + channelDataL[nextPos] * alpha;
            
            float sampleR;
            if (numChannels > 1)
            {
                auto* channelDataR = audioData->getReadPointer(1);
                sampleR = channelDataR[pos] * invAlpha + channelDataR[nextPos] * alpha;
            }
            else
            {
                sampleR = sampleL;
            }
            
            outputBuffer.addSample(0, startSample, sampleL * envelopeValue * lgain * gainMultiplier);
            if (outputBuffer.getNumChannels() > 1)
            {
                outputBuffer.addSample(1, startSample, sampleR * envelopeValue * rgain * gainMultiplier);
            }
            
            sourceSamplePosition += pitchRatio;
            ++startSample;
        }
    }
    
private:
    double pitchRatio = 0.0;
    double sourceSamplePosition = 0.0;
    float lgain = 0.0f, rgain = 0.0f;
    float gainMultiplier = 1.0f;
    juce::ADSR adsr;
    const juce::AudioBuffer<float>* audioData = nullptr;
};

//==============================================================================
// Sample loader utility
class SampleLoader
{
public:
    static std::unique_ptr<juce::AudioFormatReader> loadAudioFile(const juce::File& file)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        return std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    }
};

//==============================================================================
// Enums for musical concepts
enum class KeySignature
{
    C = 0,      // C Major / A Minor
    Db,         // Db Major / Bb Minor
    D,          // D Major / B Minor
    Eb,         // Eb Major / C Minor
    E,          // E Major / C# Minor
    F,          // F Major / D Minor
    Gb,         // Gb Major / Eb Minor
    G,          // G Major / E Minor
    Ab,         // Ab Major / F Minor
    A,          // A Major / F# Minor
    Bb,         // Bb Major / G Minor
    B           // B Major / G# Minor
};

enum class ScaleType
{
    Major,
    Minor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Aeolian,     // Natural Minor
    Locrian
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                       private juce::Timer,
                       public juce::FileDragAndDropTarget
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() noexcept override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    //==============================================================================
    void mouseDrag(const juce::MouseEvent& event) override;
    
    //==============================================================================
    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    //==============================================================================
    // Sample loading
    void loadSamples(const juce::File& sampleDirectory);
    void loadSample(int midiNote, const juce::File& audioFile);
    int parseMidiNoteFromName(const juce::String& noteName);

    //==============================================================================
    // Chord analysis structure
    struct AnalyzedChord
    {
        std::vector<int> midiNotes;           // Raw MIDI notes
        int scaleDegree;                       // 1-7 in the key
        KeyManager::ChordType quality;         // Major, Minor, Dom7, etc.
        int inversion;                         // 0=root, 1=1st, 2=2nd, 3=3rd
        
        AnalyzedChord() : scaleDegree(1), quality(KeyManager::ChordType::Major), inversion(0) {}
    };
    
    //==============================================================================
    // Chord definition for roman numeral progressions (transposable to any key)
    struct ChordDefinition
    {
        int scaleDegree;                       // 1-7 (I-VII)
        KeyManager::ChordType quality;         // Chord quality (Major, Minor, etc.)
        int inversion;                         // 0=root, 1=1st, 2=2nd, 3=3rd
        
        ChordDefinition(int degree = 1, KeyManager::ChordType q = KeyManager::ChordType::Major, int inv = 0)
            : scaleDegree(degree), quality(q), inversion(inv) {}
    };
    
    // Progression definition with both major and minor versions
    struct ProgressionDefinition
    {
        std::string name;
        std::vector<ChordDefinition> majorVersion;  // Roman numerals for major key
        std::vector<ChordDefinition> minorVersion;  // Roman numerals for minor key
    };
    
    // Generate MIDI notes from a ChordDefinition in the current key
    std::vector<int> generateChordFromDefinition(const ChordDefinition& def);

    //==============================================================================
    void applyTheme();  // Temporarily disabled
    // void updateTheme();  // Temporarily disabled

private:
    //==============================================================================
    ThemeManager themeManager;
    const short MAX_PROGRESSION_SIZE = 8;
    // UI Components
    juce::GroupComponent songSetupGroup;
    juce::GroupComponent progressionBuilderGroup;
    juce::GroupComponent emotionWheelGroup;
    juce::ComboBox keyComboBox;
    juce::ComboBox progressionComboBox;
    juce::ComboBox progressionsDropdown;  // Dummy dropdown for progressions
    juce::ComboBox chordTypeComboBox;
    juce::ComboBox timeSignatureComboBox;
    juce::ComboBox alterationComboBox;  // Chord quality/alteration selector
    juce::ComboBox inversionComboBox;   // Inversion selector (root, 1st, 2nd, etc.)
    juce::TextButton playStopButton;  // Combined play/stop button
    juce::ToggleButton loopButton;
    juce::Label tempoEditor;  // Text field for tempo entry
    juce::TextButton audioSettingsButton;
    juce::TextButton midiDragButton;  // Button to drag MIDI progression to DAW
    juce::Slider rootBoostSlider;  // Slider to control root note volume boost
    juce::Label rootBoostLabel;    // Label for root boost slider
    
    // Emotion Wheel components
    std::array<SplitButton, 24> emotionButtons;  // Grid of emotion split buttons
    juce::Label emotionDescriptionLabel;
    int selectedEmotionIndex = -1;
    
    // Chord progression builder components
    std::array<SplitButton, 7> chordButtons;  // Split buttons for scale degrees I-VII
    juce::Label progressionBuilderLabel;
    juce::Label customProgressionDisplayLabel;
    
    // Labels
    juce::Label progressionLabel;
    juce::Label tempoLabel;
    juce::Label timeSignatureLabel;
    juce::Label voicingLabel;
    juce::Label alterationLabel;
    juce::Label inversionLabel;
    
    // Key Manager
    KeyManager keyManager;
    
    // Emotion Wheel
    EmotionWheel emotionWheel;
    
    // Custom LookAndFeel for circular root button
    CircularButtonLookAndFeel circularButtonLookAndFeel;
    
    // MIDI and Audio Components
    juce::Synthesiser synth;
    juce::AudioFormatManager formatManager;
    std::map<int, std::unique_ptr<juce::AudioBuffer<float>>> sampleCache;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard;
    
    // Playback state
    bool isPlaying;
    bool shouldLoop;
    int currentChordIndex;
    juce::int64 nextChordTime;
    juce::int64 chordDuration;
    double sampleRate;
    int samplesPerBeat;
    int samplesUntilNextChord;
    int beatsPerMeasure;
    int beatUnit;
    std::vector<std::vector<int>> currentProgression;
    juce::OwnedArray<ButtonWithBadge> chordButtonsWithBadges;
    std::vector<int> currentChordNotes;
    std::vector<int> customProgressionDegrees;  // Stores the scale degrees (1-7) for custom progression
    std::vector<EmotionWheel::Emotion> customProgressionEmotions;  // Stores applied emotions (parallel to customProgressionDegrees)
    std::vector<bool> hasEmotionApplied;  // Tracks which chords actually have emotions applied
    std::vector<KeyManager::ChordType> customProgressionAlterations;  // Stores chord qualities/alterations
    std::vector<int> customProgressionInversions;  // Stores inversions (0=root, 1=1st, 2=2nd, etc.)
    int selectedChordIndexForEmotion = -1;  // Track which chord is selected for emotion editing
    bool isPreviewPlaying = false;  // Track if preview from play button is active
    float rootBoostAmount = 0.7f;  // Root note volume multiplier (1.0 = no boost, 2.0 = double volume)
    
    //==============================================================================
    // Callback functions
    void keySelectionChanged();
    void progressionSelectionChanged();
    void updateDisplay();
    void updateTimeSignature();
    void updateChordDuration();
    
    // Timer callback for preview chord stop
    void timerCallback() override;
    
    // Chord progression builder functions
    void addChordToProgression(int scaleDegree);
    void clearCustomProgression();
    void removeLastChordFromProgression();
    void removeChordAtIndex(int index);
    void updateCustomProgressionDisplay();
    void playCustomProgression();
    void updateChordButtonLabels();
    void loadSelectedProgression();
    
    // Emotion Wheel functions
    void updateChordSelector();
    void updateEmotionComboBox();
    void applyEmotionToChord();
    void updateEmotionDescription();
    void selectChordForEmotionWheel(int chordIndex);
    
    // Alteration and Inversion functions
    void updateAlterationComboBox();
    void updateInversionComboBox();
    void applyAlterationToChord();
    void applyInversionToChord();

    
    // MIDI Playback functions
    void playProgression();
    void stopProgression();
    void playChord(const std::vector<int>& chord);
    void stopCurrentChord();
    void showAudioSettings();
    void tryInitializeAudioDevice();
    void detectSystemAudioDevices();
    
    // MIDI file processing
    void processMidiFile(const juce::File& file);
    juce::String extractChordsFromMidi(const juce::MidiFile& midiFile);
    
    // Chord analysis helpers
    AnalyzedChord analyzeChord(const std::vector<int>& midiNotes, int keyRoot);
    KeyManager::ChordType detectChordQuality(const std::vector<int>& intervals);
    int detectInversion(const std::vector<int>& midiNotes, const std::vector<int>& intervals);
    int detectScaleDegree(int rootNote, int keyRoot, bool isMajorKey);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
