#pragma once

#include <JuceHeader.h>
#include "KeyManager.h"
#include "ThemeManager.h"  // Temporarily disabled

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
        level = velocity * 0.15;
        tailOff = 0.0;
        
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
                    auto currentSample = (float)(generateWaveform(currentAngle) * level * tailOff);
                    
                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);
                    
                    currentAngle += angleDelta;
                    ++startSample;
                    
                    tailOff *= 0.99;
                    
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
                    auto currentSample = (float)(generateWaveform(currentAngle) * level);
                    
                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);
                    
                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

private:
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
class MainComponent  : public juce::AudioAppComponent
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
    void applyTheme();  // Temporarily disabled
    // void updateTheme();  // Temporarily disabled

private:
    //==============================================================================
    ThemeManager themeManager;  // Temporarily disabled
    // UI Components
    juce::GroupComponent songSetupGroup;
    juce::GroupComponent refinementGroup;
    juce::GroupComponent progressionBuilderGroup;
    juce::ComboBox keyComboBox;
    juce::ComboBox progressionComboBox;
    juce::ComboBox chordTypeComboBox;
    juce::ComboBox timeSignatureComboBox;
    juce::ComboBox voicingComboBox;
    juce::ComboBox waveformComboBox;
    juce::TextButton playStopButton;  // Combined play/stop button
    juce::ToggleButton loopButton;
    juce::Slider tempoSlider;
    juce::TextButton audioSettingsButton;
    
    // Chord progression builder components
    std::array<juce::TextButton, 7> chordButtons;  // Buttons for scale degrees I-VII
    juce::TextButton clearProgressionButton;
    juce::TextButton removeLastChordButton;
    juce::Label progressionBuilderLabel;
    juce::Label customProgressionDisplayLabel;
    
    // Labels
    juce::Label progressionLabel;
    juce::Label tempoLabel;
    juce::Label timeSignatureLabel;
    juce::Label voicingLabel;
    juce::Label waveformLabel;
    
    // Key Manager
    KeyManager keyManager;
    
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
    std::vector<int> currentChordNotes;
    std::vector<int> customProgressionDegrees;  // Stores the scale degrees (1-7) for custom progression

    
    //==============================================================================
    // Callback functions
    void keySelectionChanged();
    void progressionSelectionChanged();
    void updateDisplay();
    void updateTimeSignature();
    void updateChordDuration();
    void updateWaveform();
    
    // Chord progression builder functions
    void addChordToProgression(int scaleDegree);
    void clearCustomProgression();
    void removeLastChordFromProgression();
    void updateCustomProgressionDisplay();
    void playCustomProgression();
    void updateChordButtonLabels();

    
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
