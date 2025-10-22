#pragma once

#include <JuceHeader.h>
#include "KeyManager.h"

//==============================================================================
// Forward declaration
class SineWaveSound;

//==============================================================================
// Simple Sine Wave Synthesizer Voice
class SineWaveVoice : public juce::SynthesiserVoice
{
public:
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
                    auto currentSample = (float)(std::sin(currentAngle) * level * tailOff);
                    
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
                    auto currentSample = (float)(std::sin(currentAngle) * level);
                    
                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);
                    
                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }
    
private:
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
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // UI Components
    juce::ComboBox keyComboBox;
    juce::ComboBox progressionComboBox;
    juce::ComboBox chordTypeComboBox;
    juce::ComboBox timeSignatureComboBox;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::ToggleButton loopButton;
    juce::Slider tempoSlider;
    juce::TextButton audioSettingsButton;
    juce::TextButton testToneButton;
    juce::TextButton refreshAudioButton;
    
    // Labels
    juce::Label scaleNotesLabel;
    juce::Label chordsLabel;
    juce::Label progressionLabel;
    juce::Label tempoLabel;
    juce::Label timeSignatureLabel;
    
    // Key Manager
    KeyManager keyManager;
    
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
    
    //==============================================================================
    // Callback functions
    void keySelectionChanged();
    void progressionSelectionChanged();
    void updateDisplay();
    void updateTimeSignature();
    void updateChordDuration();
    
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
