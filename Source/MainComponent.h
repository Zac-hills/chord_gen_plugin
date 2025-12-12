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
// Synthesizer Voice with Multiple Waveforms
class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    SineWaveVoice() : waveformType(WaveformType::Sine) {}
    
    void setWaveform(WaveformType type) { waveformType = type; }
    
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return true; // Accept any sound for simplicity
    }
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        level = velocity * 0.3;
        tailOff = 0.0;
        envelope = 1.0;
        sampleCount = 0;
        
        auto cyclesPerSample = frequency / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
    }
    
    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }
    
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float)(generatePianoSound(currentAngle) * level * tailOff);
                    
                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);
                    
                    currentAngle += angleDelta;
                    ++startSample;
                    sampleCount++;
                    
                    tailOff *= 0.9995; // Slower release for piano-like sustain
                    
                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote();
                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    // Piano-like envelope: fast attack, exponential decay
                    double attackTime = 0.002 * getSampleRate(); // 2ms attack
                    double decayTime = 0.5 * getSampleRate(); // 500ms decay
                    
                    if (sampleCount < attackTime)
                    {
                        envelope = sampleCount / attackTime;
                    }
                    else
                    {
                        double decayPhase = (sampleCount - attackTime) / decayTime;
                        envelope = 0.3 + 0.7 * std::exp(-3.0 * decayPhase); // Decay to 30% sustain level
                    }
                    
                    auto currentSample = (float)(generatePianoSound(currentAngle) * level * envelope);
                    
                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);
                    
                    currentAngle += angleDelta;
                    ++startSample;
                    sampleCount++;
                }
            }
        }
    }

private:
    double generatePianoSound(double angle)
    {
        // Piano-like sound: fundamental + harmonics with decreasing amplitudes
        double fundamental = std::sin(angle);
        double harmonic2 = 0.5 * std::sin(2.0 * angle);
        double harmonic3 = 0.25 * std::sin(3.0 * angle);
        double harmonic4 = 0.125 * std::sin(4.0 * angle);
        double harmonic5 = 0.0625 * std::sin(5.0 * angle);
        
        // Mix harmonics for piano-like timbre
        return (fundamental + harmonic2 + harmonic3 + harmonic4 + harmonic5) / 2.0;
    }
    
    double generateWaveform(double angle)
    {
        // Normalize angle to 0-2π range
        double normalizedAngle = std::fmod(angle, 2.0 * juce::MathConstants<double>::pi);
        if (normalizedAngle < 0)
            normalizedAngle += 2.0 * juce::MathConstants<double>::pi;
        
        switch (waveformType)
        {
            case WaveformType::Sine:
                return std::sin(angle);
                
            case WaveformType::Sawtooth:
                // Sawtooth: ramp from -1 to 1
                return 2.0 * (normalizedAngle / (2.0 * juce::MathConstants<double>::pi)) - 1.0;
                
            case WaveformType::Square:
                // Square: -1 or 1 based on angle
                return (std::sin(angle) >= 0.0) ? 1.0 : -1.0;
                
            case WaveformType::Triangle:
                // Triangle: folded sawtooth
                {
                    double t = normalizedAngle / (2.0 * juce::MathConstants<double>::pi);
                    return 4.0 * std::abs(t - 0.5) - 1.0;
                }
                
            default:
                return std::sin(angle);
        }
    }
    
    WaveformType waveformType;
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
    double frequency = 0.0;
    double envelope = 0.0;
    int sampleCount = 0;
};

//==============================================================================
// Simple Synthesizer Sound
class SineWaveSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
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
                       private juce::Timer
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
    juce::TextButton playStopButton;  // Combined play/stop button
    juce::ToggleButton loopButton;
    juce::Label tempoEditor;  // Text field for tempo entry
    juce::TextButton audioSettingsButton;
    juce::TextButton midiDragButton;  // Button to drag MIDI progression to DAW
    
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
    
    // Key Manager
    KeyManager keyManager;
    
    // Emotion Wheel
    EmotionWheel emotionWheel;
    
    // Custom LookAndFeel for circular root button
    CircularButtonLookAndFeel circularButtonLookAndFeel;
    
    // MIDI and Audio Components
    juce::Synthesiser synth;
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
    int selectedChordIndexForEmotion = -1;  // Track which chord is selected for emotion editing
    bool isPreviewPlaying = false;  // Track if preview from play button is active

    
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
    
    // Emotion Wheel functions
    void updateChordSelector();
    void updateEmotionComboBox();
    void applyEmotionToChord();
    void updateEmotionDescription();
    void selectChordForEmotionWheel(int chordIndex);

    
    // MIDI Playback functions
    void playProgression();
    void stopProgression();
    void playChord(const std::vector<int>& chord);
    void stopCurrentChord();
    void showAudioSettings();
    void tryInitializeAudioDevice();
    void detectSystemAudioDevices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
