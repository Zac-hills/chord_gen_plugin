#pragma once

/**
 * WebUIComponent — A JUCE component that hosts the Vue.js frontend
 * inside a WebBrowserComponent using JUCE 8's WebView integration.
 *
 * This component replaces the native JUCE GUI with a web-based UI.
 * It uses:
 *   - ResourceProvider to serve the built Vue app from memory
 *   - NativeFunction to expose C++ methods to JavaScript
 *   - emitEventIfBrowserIsVisible to push state changes to the frontend
 *
 * The native UI fallback is preserved in MainComponent_NativeUI.h/.cpp
 */

#include <JuceHeader.h>
#include "KeyManager.h"
#include "ThemeManager.h"
#include "EmotionWheel.h"

#if JUCE_WEB_BROWSER

//==============================================================================
class WebUIComponent : public juce::AudioAppComponent,
                       private juce::Timer
{
public:
    WebUIComponent();
    ~WebUIComponent() noexcept override;

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Resource serving for the Vue dist files
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    juce::String getMimeType(const juce::String& extension);

    // Build WebBrowserComponent options with all native functions
    juce::WebBrowserComponent::Options buildBrowserOptions();

    //==============================================================================
    // Native functions exposed to JavaScript
    void registerNativeFunctions(juce::WebBrowserComponent::Options& options);

    // Push state to the frontend
    void emitStateChanged();
    void emitProgressionChanged();
    void emitChordLabelsChanged();
    void emitEmotionLabelsChanged();

    // Build chord labels array (reusable for NativeFunction responses)
    juce::Array<juce::var> buildChordLabelsArray();

    // Build progression array (reusable for NativeFunction responses)
    juce::Array<juce::var> buildProgressionArray();

    //==============================================================================
    // Chord/music logic (same as MainComponent)
    void keySelectionChanged(int keyId);
    void updateChordDuration();
    void playChord(const std::vector<int>& chord);
    void stopCurrentChord();
    void playProgression();
    void stopProgression();
    void addChordToProgression(int scaleDegree);
    void removeChordAtIndex(int index);
    void clearCustomProgression();
    void updateCustomProgressionDisplay();
    void updateChordButtonLabels();
    void applyEmotionToChord(int emotionIndex);
    void applyAlterationToChord(int alterationId);
    void applyInversionToChord(int inversionId);
    void selectChordForEmotionWheel(int chordIndex);
    void loadSelectedProgression(int presetId);
    juce::MidiFile buildMidiFile();
    void exportMidiFile();
    void startMidiDrag();
    void timerCallback() override;

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

    // Generate MIDI notes from a ChordDefinition in the current key
    std::vector<int> generateChordFromDefinition(const ChordDefinition& def);

    // Sample loading
    void loadSamples(const juce::File& sampleDirectory);
    void loadSample(int midiNote, const juce::File& audioFile);
    int parseMidiNoteFromName(const juce::String& noteName);
    void tryInitializeAudioDevice();
    void detectSystemAudioDevices();

    //==============================================================================
    // WebBrowser component
    std::unique_ptr<juce::WebBrowserComponent> webBrowser;

    // Dev server support
    static constexpr const char* localDevServerAddress = "http://localhost:3000/";

    //==============================================================================
    // Audio / Music state (mirrors MainComponent)
    KeyManager keyManager;
    EmotionWheel emotionWheel;
    ThemeManager themeManager;
    juce::Synthesiser synth;
    juce::AudioFormatManager formatManager;
    std::map<int, std::unique_ptr<juce::AudioBuffer<float>>> sampleCache;
    juce::MidiKeyboardState keyboardState;

    // Playback state
    bool isPlaying = false;
    bool shouldLoop = false;
    int currentChordIndex = 0;
    double sampleRate = 44100.0;
    int samplesPerBeat = 0;
    int samplesUntilNextChord = 0;
    int beatsPerMeasure = 4;
    int beatUnit = 4;
    std::vector<std::vector<int>> currentProgression;
    std::vector<int> currentChordNotes;
    std::vector<int> customProgressionDegrees;
    std::vector<EmotionWheel::Emotion> customProgressionEmotions;
    std::vector<bool> hasEmotionApplied;
    std::vector<KeyManager::ChordType> customProgressionAlterations;
    std::vector<int> customProgressionInversions;
    int selectedChordIndexForEmotion = -1;
    bool isPreviewPlaying = false;
    float rootBoostAmount = 0.7f;

    // UI state
    int currentKeyId = 1;
    int currentTimeSignatureId = 1;
    int currentTempo = 120;
    int currentAlterationId = 1;
    int currentInversionId = 1;

    static const short MAX_PROGRESSION_SIZE = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebUIComponent)
};

#endif // JUCE_WEB_BROWSER
