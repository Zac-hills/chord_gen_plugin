#include "WebUIComponent.h"

#if JUCE_WEB_BROWSER

//==============================================================================
// Simple Sampler Voice (duplicated from MainComponent - shared via header later)
class WebUISamplerVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<const juce::SamplerSound*>(sound) != nullptr;
    }

    void startNote(int /*midiNoteNumber*/, float velocity, juce::SynthesiserSound* s, int) override
    {
        if (auto* sound = dynamic_cast<const juce::SamplerSound*>(s))
        {
            pitchRatio = 1.0;
            sourceSamplePosition = 0.0;
            lgain = velocity;
            rgain = velocity;
            adsr.setSampleRate(getSampleRate());
            adsr.setParameters({ 0.01, 0.0, 1.0, 0.1 });
            adsr.noteOn();
            audioData = sound->getAudioData();
        }
    }

    void setGainMultiplier(float multiplier) { gainMultiplier = multiplier; }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff) { adsr.noteOff(); }
        else { clearCurrentNote(); adsr.reset(); }
        gainMultiplier = 1.0f;
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (audioData == nullptr || !adsr.isActive()) { clearCurrentNote(); return; }

        auto numChannels = juce::jmin(outputBuffer.getNumChannels(), audioData->getNumChannels());
        auto dataLength = audioData->getNumSamples();

        while (--numSamples >= 0)
        {
            auto pos = static_cast<int>(sourceSamplePosition);
            auto envelopeValue = adsr.getNextSample();

            if (pos >= dataLength - 1) { clearCurrentNote(); adsr.reset(); return; }

            auto alpha = static_cast<float>(sourceSamplePosition - pos);
            auto invAlpha = 1.0f - alpha;
            auto nextPos = pos + 1;
            auto* channelDataL = audioData->getReadPointer(0);
            float sampleL = channelDataL[pos] * invAlpha + channelDataL[nextPos] * alpha;
            float sampleR = (numChannels > 1)
                ? (audioData->getReadPointer(1)[pos] * invAlpha + audioData->getReadPointer(1)[nextPos] * alpha)
                : sampleL;

            outputBuffer.addSample(0, startSample, sampleL * envelopeValue * lgain * gainMultiplier);
            if (outputBuffer.getNumChannels() > 1)
                outputBuffer.addSample(1, startSample, sampleR * envelopeValue * rgain * gainMultiplier);

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
// Helper: read a file into a vector<std::byte>
static std::vector<std::byte> fileToBytes(const juce::File& file)
{
    juce::MemoryBlock mb;
    file.loadFileAsData(mb);
    std::vector<std::byte> v(mb.getSize());
    std::memcpy(v.data(), mb.getData(), mb.getSize());
    return v;
}

//==============================================================================
WebUIComponent::WebUIComponent()
{
    // ─── Build browser options with native functions & resource provider ─────
    auto options = buildBrowserOptions();

    webBrowser = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webBrowser);

    // In dev mode, navigate to the Vite dev server for hot module reloading.
    // In production, serve the built dist files via the ResourceProvider.
#if USE_DEV_SERVER
    DBG("WebUI: Loading from Vite dev server at " << localDevServerAddress);
    webBrowser->goToURL(localDevServerAddress);
#else
    DBG("WebUI: Loading from ResourceProvider (built dist)");
    webBrowser->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    // ─── Audio setup (same as MainComponent) ─────────────────────────────────
    formatManager.registerBasicFormats();
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new WebUISamplerVoice());

    detectSystemAudioDevices();
    tryInitializeAudioDevice();

    // Load samples
    juce::File assetsFolder;
    auto appBundle = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    DBG("WebUI: App bundle: " << appBundle.getFullPathName());
    assetsFolder = appBundle.getChildFile("Contents/Resources/assets");
    if (!assetsFolder.exists())
        assetsFolder = juce::File::getCurrentWorkingDirectory().getChildFile("assets");
    if (!assetsFolder.exists())
        assetsFolder = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                           .getParentDirectory().getChildFile("assets");
    if (!assetsFolder.exists())
    {
        // Walk up from the executable to find the project root assets folder
        auto execFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto dir = execFile.getParentDirectory();
        for (int i = 0; i < 10 && dir.getParentDirectory() != dir; ++i)
        {
            auto candidate = dir.getChildFile("assets");
            if (candidate.exists() && candidate.getChildFile("C4.aif").exists())
            {
                assetsFolder = candidate;
                break;
            }
            dir = dir.getParentDirectory();
        }
    }
    if (assetsFolder.exists())
    {
        DBG("WebUI: Found assets at: " << assetsFolder.getFullPathName());
        loadSamples(assetsFolder);
    }
    else
    {
        DBG("WebUI: WARNING — assets folder NOT FOUND!");
    }

    setSize(1200, 700);
    setAudioChannels(0, 2);
}

WebUIComponent::~WebUIComponent() noexcept
{
    shutdownAudio();
}

//==============================================================================
juce::WebBrowserComponent::Options WebUIComponent::buildBrowserOptions()
{
    auto options = juce::WebBrowserComponent::Options{}
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withNativeIntegrationEnabled()
        .withResourceProvider(
            [this](const auto& url) { return getResource(url); },
            juce::URL(localDevServerAddress).getOrigin()
        );

    // Register all native functions
    registerNativeFunctions(options);

    return options;
}

//==============================================================================
void WebUIComponent::registerNativeFunctions(juce::WebBrowserComponent::Options& options)
{
    // ─── setKey ──────────────────────────────────────────────────────────────
    options = options.withNativeFunction("setKey",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int keyId = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, keyId, complete]() {
                    keySelectionChanged(keyId);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("labels", buildChordLabelsArray());
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── setTimeSignature ────────────────────────────────────────────────────
    options = options.withNativeFunction("setTimeSignature",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                currentTimeSignatureId = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, complete]() {
                    switch (currentTimeSignatureId) {
                        case 1: beatsPerMeasure = 4; beatUnit = 4; break;
                        case 2: beatsPerMeasure = 3; beatUnit = 4; break;
                        case 3: beatsPerMeasure = 6; beatUnit = 8; break;
                        case 4: beatsPerMeasure = 5; beatUnit = 4; break;
                        case 5: beatsPerMeasure = 7; beatUnit = 8; break;
                        case 6: beatsPerMeasure = 2; beatUnit = 4; break;
                        default: beatsPerMeasure = 4; beatUnit = 4; break;
                    }
                    updateChordDuration();
                    complete(juce::var());
                });
            } else { complete(juce::var()); }
        });

    // ─── setTempo ────────────────────────────────────────────────────────────
    options = options.withNativeFunction("setTempo",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                currentTempo = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, complete]() {
                    updateChordDuration();
                    complete(juce::var());
                });
            } else { complete(juce::var()); }
        });

    // ─── setRootBoost ────────────────────────────────────────────────────────
    options = options.withNativeFunction("setRootBoost",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                rootBoostAmount = static_cast<float>(static_cast<double>(args[0]));
            }
            complete(juce::var());
        });

    // ─── addChord ────────────────────────────────────────────────────────────
    options = options.withNativeFunction("addChord",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int degree = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, degree, complete]() {
                    addChordToProgression(degree);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("chords", buildProgressionArray());
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── removeChord ─────────────────────────────────────────────────────────
    options = options.withNativeFunction("removeChord",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int index = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, index, complete]() {
                    removeChordAtIndex(index);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("chords", buildProgressionArray());
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── clearProgression ────────────────────────────────────────────────────
    options = options.withNativeFunction("clearProgression",
        [this](const juce::Array<juce::var>&, auto complete) {
            juce::MessageManager::callAsync([this, complete]() {
                clearCustomProgression();
                auto* result = new juce::DynamicObject();
                result->setProperty("chords", buildProgressionArray());
                complete(juce::var(result));
            });
        });

    // ─── playProgression ─────────────────────────────────────────────────────
    options = options.withNativeFunction("playProgression",
        [this](const juce::Array<juce::var>&, auto complete) {
            juce::MessageManager::callAsync([this, complete]() {
                playProgression();
                auto* result = new juce::DynamicObject();
                result->setProperty("isPlaying", isPlaying);
                complete(juce::var(result));
            });
        });

    // ─── stopProgression ─────────────────────────────────────────────────────
    options = options.withNativeFunction("stopProgression",
        [this](const juce::Array<juce::var>&, auto complete) {
            juce::MessageManager::callAsync([this, complete]() {
                stopProgression();
                auto* result = new juce::DynamicObject();
                result->setProperty("isPlaying", isPlaying);
                complete(juce::var(result));
            });
        });

    // ─── toggleLoop ──────────────────────────────────────────────────────────
    options = options.withNativeFunction("toggleLoop",
        [this](const juce::Array<juce::var>&, auto complete) {
            shouldLoop = !shouldLoop;
            emitStateChanged();
            complete(juce::var());
        });

    // ─── previewChord ────────────────────────────────────────────────────────
    options = options.withNativeFunction("previewChord",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int degree = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, degree, complete]() {
                    if (isPreviewPlaying) { stopCurrentChord(); currentChordNotes.clear(); stopTimer(); }

                    auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
                    auto chord = keyManager.generateTriad(scaleDegree);
                    playChord(chord);
                    isPreviewPlaying = true;

                    int tempo = currentTempo;
                    if (tempo < 60 || tempo > 200) tempo = 120;
                    startTimer(60000 / tempo);
                    complete(juce::var());
                });
            } else { complete(juce::var()); }
        });

    // ─── previewEmotion ──────────────────────────────────────────────────────
    options = options.withNativeFunction("previewEmotion",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0 && selectedChordIndexForEmotion >= 0) {
                int emotionIdx = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, emotionIdx, complete]() {
                    if (selectedChordIndexForEmotion >= 0 &&
                        selectedChordIndexForEmotion < static_cast<int>(customProgressionDegrees.size()))
                    {
                        if (isPreviewPlaying) { stopCurrentChord(); currentChordNotes.clear(); stopTimer(); }

                        int degree = customProgressionDegrees[selectedChordIndexForEmotion];
                        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
                        auto baseChord = keyManager.generateTriad(scaleDegree);

                        auto chordType = keyManager.analyzeTriad(scaleDegree);
                        auto tonality = (chordType == KeyManager::ChordType::Minor || chordType == KeyManager::ChordType::Diminished)
                            ? EmotionWheel::Tonality::Minor : EmotionWheel::Tonality::Major;

                        auto emotions = emotionWheel.getEmotionsByTonality(tonality);
                        if (emotionIdx < static_cast<int>(emotions.size()))
                        {
                            auto emotion = emotions[emotionIdx];
                            int rootNote = baseChord[0];
                            auto emotionChord = emotionWheel.applyEmotion(rootNote, emotion);
                            playChord(emotionChord);
                            isPreviewPlaying = true;
                            int tempo = currentTempo;
                            if (tempo < 60 || tempo > 200) tempo = 120;
                            startTimer(60000 / tempo);
                        }
                    }
                    complete(juce::var());
                });
            } else { complete(juce::var()); }
        });

    // ─── selectChordForEmotion ───────────────────────────────────────────────
    options = options.withNativeFunction("selectChordForEmotion",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int index = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, index, complete]() {
                    selectChordForEmotionWheel(index);
                    complete(juce::var());
                });
            } else { complete(juce::var()); }
        });

    // ─── applyEmotion ────────────────────────────────────────────────────────
    options = options.withNativeFunction("applyEmotion",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int emotionIdx = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, emotionIdx, complete]() {
                    applyEmotionToChord(emotionIdx);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("chords", buildProgressionArray());
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── setAlteration ───────────────────────────────────────────────────────
    options = options.withNativeFunction("setAlteration",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int altId = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, altId, complete]() {
                    applyAlterationToChord(altId);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("chords", juce::var(buildProgressionArray()));
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── setInversion ────────────────────────────────────────────────────────
    options = options.withNativeFunction("setInversion",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int invId = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, invId, complete]() {
                    applyInversionToChord(invId);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("chords", juce::var(buildProgressionArray()));
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── loadProgression ─────────────────────────────────────────────────────
    options = options.withNativeFunction("loadProgression",
        [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0) {
                int presetId = static_cast<int>(args[0]);
                juce::MessageManager::callAsync([this, presetId, complete]() {
                    loadSelectedProgression(presetId);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("chords", juce::var(buildProgressionArray()));
                    complete(juce::var(result));
                });
            } else { complete(juce::var()); }
        });

    // ─── exportMidi (save dialog fallback) ────────────────────────────────────
    options = options.withNativeFunction("exportMidi",
        [this](const juce::Array<juce::var>&, auto complete) {
            juce::MessageManager::callAsync([this, complete]() {
                exportMidiFile();
                complete(juce::var());
            });
        });

    // ─── startMidiDrag (drag-and-drop to DAW) ────────────────────────────────
    options = options.withNativeFunction("startMidiDrag",
        [this](const juce::Array<juce::var>&, auto complete) {
            juce::MessageManager::callAsync([this, complete]() {
                startMidiDrag();
                complete(juce::var());
            });
        });

    // ─── getState ────────────────────────────────────────────────────────────
    options = options.withNativeFunction("getState",
        [this](const juce::Array<juce::var>&, auto complete) {
            auto* obj = new juce::DynamicObject();
            obj->setProperty("key", currentKeyId);
            obj->setProperty("timeSignature", currentTimeSignatureId);
            obj->setProperty("tempo", currentTempo);
            obj->setProperty("rootBoost", static_cast<double>(rootBoostAmount));
            obj->setProperty("isPlaying", isPlaying);
            obj->setProperty("isLooping", shouldLoop);
            obj->setProperty("selectedAlteration", currentAlterationId);
            obj->setProperty("selectedInversion", currentInversionId);
            obj->setProperty("selectedChordForEmotion", selectedChordIndexForEmotion);
            obj->setProperty("labels", buildChordLabelsArray());
            obj->setProperty("chords", buildProgressionArray());
            complete(juce::var(obj));

            // Also emit events (for any other listeners)
            emitChordLabelsChanged();
            emitProgressionChanged();
        });
}

//==============================================================================
// Resource Provider — serves built Vue files from the webui/dist folder
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource> WebUIComponent::getResource(const juce::String& url)
{
    auto urlToRetrieve = url == "/" ? juce::String("index.html")
                                   : url.fromFirstOccurrenceOf("/", false, false);

    DBG("WebUI ResourceProvider: requesting " << urlToRetrieve);

    // Try to find the dist folder
    juce::File distFolder;

    // 1. Inside app bundle (for deployed builds)
    auto appBundle = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    distFolder = appBundle.getChildFile("Contents/Resources/webui");
    DBG("  Try 1 (bundle): " << distFolder.getFullPathName() << " exists=" << (int)distFolder.exists());

    if (!distFolder.exists())
    {
        // 2. Development: walk up from the executable to the project root
        //    Executable is at: <project>/Builds/MacOSX/build/Debug/App.app/Contents/MacOS/App
        //    We need:          <project>/webui/dist
        auto execFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto projectRoot = execFile.getParentDirectory()  // MacOS
                                   .getParentDirectory()  // Contents
                                   .getParentDirectory()  // App.app
                                   .getParentDirectory()  // Debug
                                   .getParentDirectory()  // build
                                   .getParentDirectory()  // MacOSX
                                   .getParentDirectory()  // Builds
                                   .getParentDirectory(); // <project root>
        distFolder = projectRoot.getChildFile("webui").getChildFile("dist");
        DBG("  Try 2 (dev exec): " << distFolder.getFullPathName() << " exists=" << (int)distFolder.exists());
    }

    if (!distFolder.exists())
    {
        // 3. Relative to CWD
        distFolder = juce::File::getCurrentWorkingDirectory().getChildFile("webui/dist");
        DBG("  Try 3 (cwd): " << distFolder.getFullPathName() << " exists=" << (int)distFolder.exists());
    }

    if (!distFolder.exists())
    {
        // 4. Hardcoded project path as last resort during development
        distFolder = juce::File("/Users/zac/Programming/chord_gen_plugin/webui/dist");
        DBG("  Try 4 (hardcoded): " << distFolder.getFullPathName() << " exists=" << (int)distFolder.exists());
    }

    auto file = distFolder.getChildFile(urlToRetrieve);

    if (file.existsAsFile())
    {
        auto ext = file.getFileExtension().toLowerCase();
        auto mime = getMimeType(ext);
        auto data = fileToBytes(file);
        return juce::WebBrowserComponent::Resource{ std::move(data), std::move(mime) };
    }

    // Fallback: serve index.html for SPA routing
    if (!urlToRetrieve.contains("."))
    {
        auto indexFile = distFolder.getChildFile("index.html");
        if (indexFile.existsAsFile())
        {
            auto data = fileToBytes(indexFile);
            return juce::WebBrowserComponent::Resource{ std::move(data), juce::String("text/html") };
        }
    }

    DBG("WebUI ResourceProvider: 404 for " << urlToRetrieve);
    return std::nullopt;
}

juce::String WebUIComponent::getMimeType(const juce::String& extension)
{
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".js")                          return "application/javascript";
    if (extension == ".css")                         return "text/css";
    if (extension == ".json")                        return "application/json";
    if (extension == ".png")                         return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".svg")                         return "image/svg+xml";
    if (extension == ".ico")                         return "image/x-icon";
    if (extension == ".woff")                        return "font/woff";
    if (extension == ".woff2")                       return "font/woff2";
    if (extension == ".ttf")                         return "font/ttf";
    return "application/octet-stream";
}

//==============================================================================
// State emission to frontend
//==============================================================================
void WebUIComponent::emitStateChanged()
{
    if (webBrowser == nullptr) return;

    auto* obj = new juce::DynamicObject();
    obj->setProperty("key", currentKeyId);
    obj->setProperty("timeSignature", currentTimeSignatureId);
    obj->setProperty("tempo", currentTempo);
    obj->setProperty("rootBoost", static_cast<double>(rootBoostAmount));
    obj->setProperty("isPlaying", isPlaying);
    obj->setProperty("isLooping", shouldLoop);
    obj->setProperty("selectedAlteration", currentAlterationId);
    obj->setProperty("selectedInversion", currentInversionId);
    obj->setProperty("selectedChordForEmotion", selectedChordIndexForEmotion);

    webBrowser->emitEventIfBrowserIsVisible("stateChanged", juce::var(obj));
}

void WebUIComponent::emitProgressionChanged()
{
    if (webBrowser == nullptr) return;

    auto* obj = new juce::DynamicObject();
    obj->setProperty("chords", buildProgressionArray());
    webBrowser->emitEventIfBrowserIsVisible("progressionChanged", juce::var(obj));
}

juce::Array<juce::var> WebUIComponent::buildProgressionArray()
{
    juce::Array<juce::var> chords;
    const juce::StringArray romanNumeralsMajor = { "I", "II", "III", "IV", "V", "VI", "VII" };
    const juce::StringArray romanNumeralsMinor = { "i", "ii", "iii", "iv", "v", "vi", "vii" };
    auto scaleNotes = keyManager.getScaleNoteNamesWithProperSpelling();

    for (int i = 0; i < static_cast<int>(customProgressionDegrees.size()); ++i)
    {
        auto* chord = new juce::DynamicObject();
        int degree = customProgressionDegrees[i];
        chord->setProperty("degree", degree);

        // Determine effective chord type
        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
        KeyManager::ChordType chordType;

        if (i < static_cast<int>(customProgressionAlterations.size()) &&
            customProgressionAlterations[i] != KeyManager::ChordType::Major)
        {
            chordType = customProgressionAlterations[i];
        }
        else
        {
            chordType = keyManager.analyzeTriad(scaleDegree);
        }

        // Determine if minor-type for roman numeral casing
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

        juce::String romanNumeral = (degree >= 1 && degree <= 7)
            ? (isMinorType ? romanNumeralsMinor[degree - 1] : romanNumeralsMajor[degree - 1])
            : "?";

        // Add voicing suffix to roman numeral
        switch (chordType)
        {
            case KeyManager::ChordType::Major7:          romanNumeral += "maj7"; break;
            case KeyManager::ChordType::Minor7:          romanNumeral += "7"; break;
            case KeyManager::ChordType::Dominant7:       romanNumeral += "7"; break;
            case KeyManager::ChordType::Diminished7:     romanNumeral += juce::String::fromUTF8("\xC2\xB0") + "7"; break;
            case KeyManager::ChordType::HalfDiminished7: romanNumeral += juce::String::fromUTF8("\xC3\xB8") + "7"; break;
            case KeyManager::ChordType::Diminished:      romanNumeral += juce::String::fromUTF8("\xC2\xB0"); break;
            case KeyManager::ChordType::Augmented:       romanNumeral += "+"; break;
            case KeyManager::ChordType::Sus2:            romanNumeral += "sus2"; break;
            case KeyManager::ChordType::Sus4:            romanNumeral += "sus4"; break;
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
            default: break;
        }

        chord->setProperty("label", romanNumeral);

        // Full chord name (e.g. "Am7", "Csus4")
        auto chordName = juce::String(keyManager.getChordName(scaleDegree, chordType));

        // Add inversion indicator
        int inversion = 0;
        if (i < static_cast<int>(customProgressionInversions.size()))
            inversion = customProgressionInversions[i];

        if (inversion > 0)
            chordName += "/" + juce::String(inversion);

        chord->setProperty("chordName", chordName);
        chord->setProperty("inversion", inversion);

        // Include the actual note name (e.g. "C", "D#", "Bb")
        if (degree >= 1 && degree <= 7 && (degree - 1) < static_cast<int>(scaleNotes.size()))
            chord->setProperty("noteName", juce::String(scaleNotes[degree - 1]));
        else
            chord->setProperty("noteName", "?");

        // Include emotion name if one has been applied
        if (i < static_cast<int>(hasEmotionApplied.size()) && hasEmotionApplied[i])
        {
            auto emotionName = EmotionWheel::getEmotionName(customProgressionEmotions[i]);
            // Extract just the part in parentheses, e.g. "Happy (Maj6)" → "Maj6"
            auto full = juce::String(emotionName);
            auto start = full.indexOfChar('(');
            auto end = full.indexOfChar(')');
            if (start >= 0 && end > start)
                chord->setProperty("emotion", full.substring(start + 1, end));
            else
                chord->setProperty("emotion", full);
        }

        chords.add(juce::var(chord));
    }
    return chords;
}

juce::Array<juce::var> WebUIComponent::buildChordLabelsArray()
{
    juce::Array<juce::var> labels;
    const juce::StringArray romanNumerals = { "I", "II", "III", "IV", "V", "VI", "VII" };
    auto scaleNotes = keyManager.getScaleNoteNamesWithProperSpelling();

    for (int i = 0; i < 7; ++i)
    {
        juce::String noteLabel;
        if (i < static_cast<int>(scaleNotes.size()))
            noteLabel = juce::String(scaleNotes[i]);
        else
            noteLabel = "?";
        labels.add(juce::var(romanNumerals[i] + "\n" + noteLabel));
    }
    return labels;
}

void WebUIComponent::emitChordLabelsChanged()
{
    if (webBrowser == nullptr) return;

    auto* obj = new juce::DynamicObject();
    obj->setProperty("labels", buildChordLabelsArray());
    webBrowser->emitEventIfBrowserIsVisible("chordLabelsChanged", juce::var(obj));
}

void WebUIComponent::emitEmotionLabelsChanged()
{
    if (webBrowser == nullptr) return;

    auto* obj = new juce::DynamicObject();
    juce::Array<juce::var> labels;

    // Major tonality emotions first, then minor
    auto majorEmotions = emotionWheel.getEmotionsByTonality(EmotionWheel::Tonality::Major);
    auto minorEmotions = emotionWheel.getEmotionsByTonality(EmotionWheel::Tonality::Minor);

    // Get the current tonality context
    EmotionWheel::Tonality tonality = EmotionWheel::Tonality::Major;
    if (selectedChordIndexForEmotion >= 0 &&
        selectedChordIndexForEmotion < static_cast<int>(customProgressionDegrees.size()))
    {
        int degree = customProgressionDegrees[selectedChordIndexForEmotion];
        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
        auto chordType = keyManager.analyzeTriad(scaleDegree);
        if (chordType == KeyManager::ChordType::Minor || chordType == KeyManager::ChordType::Diminished)
            tonality = EmotionWheel::Tonality::Minor;
    }

    auto emotions = emotionWheel.getEmotionsByTonality(tonality);
    for (size_t i = 0; i < emotions.size() && i < 24; ++i)
    {
        auto profile = emotionWheel.getEmotionProfile(emotions[i]);
        if (profile != nullptr)
            labels.add(juce::var(juce::String(profile->name)));
        else
            labels.add(juce::var(juce::String("—")));
    }

    // Pad to 24
    while (labels.size() < 24)
        labels.add(juce::var("—"));

    obj->setProperty("labels", labels);
    webBrowser->emitEventIfBrowserIsVisible("emotionLabelsChanged", juce::var(obj));
}

//==============================================================================
// Music logic implementations
//==============================================================================
void WebUIComponent::keySelectionChanged(int keyId)
{
    currentKeyId = keyId;

    const int majorKeyMap[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    const int minorKeyMap[12] = { 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

    if (keyId <= 12)
    {
        int keyIndex = majorKeyMap[keyId - 1];
        bool isSharpKey = (keyId == 2 || keyId == 3 || keyId == 5 ||
                          keyId == 7 || keyId == 8 || keyId == 10 || keyId == 12);
        keyManager.setCurrentKey(static_cast<KeyManager::Key>(keyIndex), KeyManager::Tonality::Major, isSharpKey);
    }
    else
    {
        int keyIndex = minorKeyMap[keyId - 13];
        bool isSharpKey = (keyId == 14 || keyId == 15 || keyId == 17 ||
                          keyId == 19 || keyId == 20 || keyId == 22 || keyId == 24);
        keyManager.setCurrentKey(static_cast<KeyManager::Key>(keyIndex), KeyManager::Tonality::Minor, isSharpKey);
    }

    selectedChordIndexForEmotion = -1;
    hasEmotionApplied.clear();
    hasEmotionApplied.resize(customProgressionDegrees.size(), false);
    customProgressionEmotions.clear();
    customProgressionEmotions.resize(customProgressionDegrees.size(), EmotionWheel::Emotion::Happy_Maj6);

    updateChordButtonLabels();
    emitStateChanged();
    emitChordLabelsChanged();
    emitProgressionChanged();
}

void WebUIComponent::updateChordDuration()
{
    int tempo = currentTempo;
    if (tempo < 60 || tempo > 200) tempo = 120;
    samplesPerBeat = static_cast<int>((60.0 / static_cast<double>(tempo)) * sampleRate);
}

void WebUIComponent::playChord(const std::vector<int>& chord)
{
    stopCurrentChord();
    currentChordNotes = chord;

    // Find the lowest note in the chord for bass boost
    int lowestNote = chord.empty() ? -1 : *std::min_element(chord.begin(), chord.end());

    for (size_t i = 0; i < chord.size(); ++i)
    {
        bool isBassNote = (chord[i] == lowestNote);
        float velocity = 0.7f;
        // Apply bass boost to the lowest note in the chord
        if (isBassNote)
            velocity *= rootBoostAmount;

        auto* voice = dynamic_cast<WebUISamplerVoice*>(synth.getVoice(static_cast<int>(i) % synth.getNumVoices()));
        if (voice != nullptr)
            voice->setGainMultiplier(isBassNote ? rootBoostAmount : 1.0f);

        synth.noteOn(1, chord[i], velocity);
    }
}

void WebUIComponent::stopCurrentChord()
{
    for (auto note : currentChordNotes)
        synth.noteOff(1, note, 0.0f, true);
    currentChordNotes.clear();
}

void WebUIComponent::playProgression()
{
    if (customProgressionDegrees.empty()) return;

    // Build the progression from current degrees, alterations, inversions, and emotions
    currentProgression.clear();
    for (size_t i = 0; i < customProgressionDegrees.size(); ++i)
    {
        int degree = customProgressionDegrees[i];
        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
        std::vector<int> chord;

        // Check if this chord has an emotion applied
        if (i < hasEmotionApplied.size() && hasEmotionApplied[i] && i < customProgressionEmotions.size())
        {
            auto scaleNotes = keyManager.getScaleNotes();
            if (degree - 1 < static_cast<int>(scaleNotes.size()))
            {
                int rootNote = 48 + scaleNotes[degree - 1];
                chord = emotionWheel.applyEmotion(rootNote, customProgressionEmotions[i]);
            }
            else
            {
                chord = keyManager.generateTriad(scaleDegree);
            }
        }
        // Check if this chord has an alteration applied
        else if (i < customProgressionAlterations.size())
        {
            auto scaleNotes = keyManager.getScaleNotes();
            if (degree - 1 < static_cast<int>(scaleNotes.size()))
            {
                int rootNote = 48 + scaleNotes[degree - 1];
                std::string scale = (keyManager.getCurrentTonality() == KeyManager::Tonality::Major) ? "Major" : "Minor";

                int inversion = 0;
                if (i < customProgressionInversions.size())
                    inversion = customProgressionInversions[i];

                chord = keyManager.generateChordWithAlteration(degree, rootNote, scale,
                                                                customProgressionAlterations[i], inversion);
            }
            else
            {
                chord = keyManager.generateTriad(scaleDegree);
            }
        }
        else
        {
            chord = keyManager.generateTriad(scaleDegree);

            // Apply inversion even without alteration
            if (i < customProgressionInversions.size() && customProgressionInversions[i] > 0)
                chord = keyManager.applyInversion(chord, customProgressionInversions[i]);
        }

        currentProgression.push_back(chord);
    }

    isPlaying = true;
    currentChordIndex = 0;
    updateChordDuration();
    samplesUntilNextChord = 0;
    emitStateChanged();
}

void WebUIComponent::stopProgression()
{
    isPlaying = false;
    stopCurrentChord();
    emitStateChanged();
}

void WebUIComponent::addChordToProgression(int scaleDegree)
{
    if (static_cast<int>(customProgressionDegrees.size()) >= MAX_PROGRESSION_SIZE) return;

    customProgressionDegrees.push_back(scaleDegree);
    customProgressionAlterations.push_back(KeyManager::ChordType::Major);
    customProgressionInversions.push_back(0);
    customProgressionEmotions.push_back(EmotionWheel::Emotion::Happy_Maj6);
    hasEmotionApplied.push_back(false);

    emitProgressionChanged();
    emitStateChanged();
}

void WebUIComponent::removeChordAtIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(customProgressionDegrees.size())) return;

    customProgressionDegrees.erase(customProgressionDegrees.begin() + index);
    customProgressionAlterations.erase(customProgressionAlterations.begin() + index);
    customProgressionInversions.erase(customProgressionInversions.begin() + index);
    customProgressionEmotions.erase(customProgressionEmotions.begin() + index);
    hasEmotionApplied.erase(hasEmotionApplied.begin() + index);

    if (selectedChordIndexForEmotion == index)
        selectedChordIndexForEmotion = -1;
    else if (selectedChordIndexForEmotion > index)
        selectedChordIndexForEmotion--;

    emitProgressionChanged();
    emitStateChanged();
}

void WebUIComponent::clearCustomProgression()
{
    customProgressionDegrees.clear();
    customProgressionAlterations.clear();
    customProgressionInversions.clear();
    customProgressionEmotions.clear();
    hasEmotionApplied.clear();
    selectedChordIndexForEmotion = -1;

    emitProgressionChanged();
    emitStateChanged();
}

void WebUIComponent::updateCustomProgressionDisplay()
{
    emitProgressionChanged();
}

void WebUIComponent::updateChordButtonLabels()
{
    emitChordLabelsChanged();
}

void WebUIComponent::applyEmotionToChord(int emotionIndex)
{
    if (selectedChordIndexForEmotion < 0 ||
        selectedChordIndexForEmotion >= static_cast<int>(customProgressionDegrees.size()))
        return;

    int degree = customProgressionDegrees[selectedChordIndexForEmotion];
    auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
    auto chordType = keyManager.analyzeTriad(scaleDegree);
    auto tonality = (chordType == KeyManager::ChordType::Minor || chordType == KeyManager::ChordType::Diminished)
        ? EmotionWheel::Tonality::Minor : EmotionWheel::Tonality::Major;

    auto emotions = emotionWheel.getEmotionsByTonality(tonality);
    if (emotionIndex < static_cast<int>(emotions.size()))
    {
        customProgressionEmotions[selectedChordIndexForEmotion] = emotions[emotionIndex];
        hasEmotionApplied[selectedChordIndexForEmotion] = true;
        emitProgressionChanged();
    }
}

void WebUIComponent::applyAlterationToChord(int alterationId)
{
    DBG("applyAlterationToChord: alterationId=" << alterationId
        << " selectedChord=" << selectedChordIndexForEmotion
        << " progressionSize=" << (int)customProgressionDegrees.size());

    if (selectedChordIndexForEmotion < 0 ||
        selectedChordIndexForEmotion >= static_cast<int>(customProgressionDegrees.size()))
    {
        DBG("  → early return: no chord selected or out of range");
        return;
    }

    currentAlterationId = alterationId;

    if (alterationId == 1)  // "Natural" means no alteration
    {
        if (static_cast<int>(customProgressionAlterations.size()) > selectedChordIndexForEmotion)
            customProgressionAlterations[selectedChordIndexForEmotion] = KeyManager::ChordType::Major;

        if (static_cast<int>(hasEmotionApplied.size()) > selectedChordIndexForEmotion)
            hasEmotionApplied[selectedChordIndexForEmotion] = false;
    }
    else
    {
        KeyManager::ChordType alteration;
        switch (alterationId)
        {
            case 2:  alteration = KeyManager::ChordType::Major; break;
            case 3:  alteration = KeyManager::ChordType::Minor; break;
            case 4:  alteration = KeyManager::ChordType::Diminished; break;
            case 5:  alteration = KeyManager::ChordType::Augmented; break;
            case 6:  alteration = KeyManager::ChordType::Major7; break;
            case 7:  alteration = KeyManager::ChordType::Minor7; break;
            case 8:  alteration = KeyManager::ChordType::Dominant7; break;
            case 9:  alteration = KeyManager::ChordType::Diminished7; break;
            case 10: alteration = KeyManager::ChordType::HalfDiminished7; break;
            case 11: alteration = KeyManager::ChordType::Sus2; break;
            case 12: alteration = KeyManager::ChordType::Sus4; break;
            case 13: alteration = KeyManager::ChordType::Add9; break;
            case 14: alteration = KeyManager::ChordType::Major9; break;
            case 15: alteration = KeyManager::ChordType::Minor9; break;
            case 16: alteration = KeyManager::ChordType::Dominant9; break;
            default: alteration = KeyManager::ChordType::Major; break;
        }

        if (static_cast<int>(customProgressionAlterations.size()) <= selectedChordIndexForEmotion)
            customProgressionAlterations.resize(customProgressionDegrees.size(), KeyManager::ChordType::Major);

        customProgressionAlterations[selectedChordIndexForEmotion] = alteration;

        DBG("  → stored alteration " << static_cast<int>(alteration)
            << " at index " << selectedChordIndexForEmotion);

        if (static_cast<int>(hasEmotionApplied.size()) > selectedChordIndexForEmotion)
            hasEmotionApplied[selectedChordIndexForEmotion] = false;
    }

    emitProgressionChanged();
    emitStateChanged();

    if (isPlaying)
        playProgression();
}

void WebUIComponent::applyInversionToChord(int inversionId)
{
    DBG("applyInversionToChord: inversionId=" << inversionId
        << " selectedChord=" << selectedChordIndexForEmotion
        << " progressionSize=" << (int)customProgressionDegrees.size());

    if (selectedChordIndexForEmotion < 0 ||
        selectedChordIndexForEmotion >= static_cast<int>(customProgressionDegrees.size()))
    {
        DBG("  → early return: no chord selected or out of range");
        return;
    }

    currentInversionId = inversionId;
    int inversion = inversionId - 1;  // 1=root(0), 2=1st(1), 3=2nd(2), 4=3rd(3)

    if (static_cast<int>(customProgressionInversions.size()) <= selectedChordIndexForEmotion)
        customProgressionInversions.resize(customProgressionDegrees.size(), 0);

    customProgressionInversions[selectedChordIndexForEmotion] = inversion;

    emitProgressionChanged();
    emitStateChanged();

    if (isPlaying)
        playProgression();
}

juce::MidiFile WebUIComponent::buildMidiFile()
{
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(960);

    if (customProgressionDegrees.empty())
        return midiFile;

    juce::MidiMessageSequence track;

    // Get time signature
    int beatsPerBar = 4;
    switch (currentTimeSignatureId)
    {
        case 1: beatsPerBar = 4; break;  // 4/4
        case 2: beatsPerBar = 3; break;  // 3/4
        case 3: beatsPerBar = 6; break;  // 6/8
        case 4: beatsPerBar = 5; break;  // 5/4
        default: beatsPerBar = 4; break;
    }

    int ticksPerChord = midiFile.getTimeFormat() * beatsPerBar;
    int currentTick = 0;

    for (size_t i = 0; i < customProgressionDegrees.size(); ++i)
    {
        int degree = customProgressionDegrees[i];
        auto scaleDegree = static_cast<KeyManager::ScaleDegree>(degree);
        std::vector<int> chord;

        // Check if this chord has an emotion applied
        if (i < hasEmotionApplied.size() && hasEmotionApplied[i] && i < customProgressionEmotions.size())
        {
            auto scaleNotes = keyManager.getScaleNotes();
            if (degree - 1 < static_cast<int>(scaleNotes.size()))
            {
                int rootNote = 48 + scaleNotes[degree - 1];
                chord = emotionWheel.applyEmotion(rootNote, customProgressionEmotions[i]);
            }
            else
            {
                chord = keyManager.generateTriad(scaleDegree);
            }
        }
        // Check if this chord has an alteration applied
        else if (i < customProgressionAlterations.size())
        {
            auto scaleNotes = keyManager.getScaleNotes();
            if (degree - 1 < static_cast<int>(scaleNotes.size()))
            {
                int rootNote = 48 + scaleNotes[degree - 1];
                std::string scale = (keyManager.getCurrentTonality() == KeyManager::Tonality::Major) ? "Major" : "Minor";

                int inversion = 0;
                if (i < customProgressionInversions.size())
                    inversion = customProgressionInversions[i];

                chord = keyManager.generateChordWithAlteration(degree, rootNote, scale,
                                                                customProgressionAlterations[i], inversion);
            }
            else
            {
                chord = keyManager.generateTriad(scaleDegree);
            }
        }
        else
        {
            chord = keyManager.generateTriad(scaleDegree);

            // Apply inversion even without alteration
            if (i < customProgressionInversions.size() && customProgressionInversions[i] > 0)
                chord = keyManager.applyInversion(chord, customProgressionInversions[i]);
        }

        // Check if chord contains the key root note — if not, add bass root
        auto scaleNotes = keyManager.getScaleNotes();
        int keyRootPitchClass = scaleNotes.empty() ? 0 : scaleNotes[0];

        bool hasRootNote = false;
        for (int note : chord)
        {
            if ((note % 12) == keyRootPitchClass)
            {
                hasRootNote = true;
                break;
            }
        }

        if (!hasRootNote)
        {
            int bassRootNote = 48 + keyRootPitchClass;
            if (bassRootNote >= 0 && bassRootNote < 128)
            {
                track.addEvent(juce::MidiMessage::noteOn(1, bassRootNote, (juce::uint8) 90), currentTick);
                track.addEvent(juce::MidiMessage::noteOff(1, bassRootNote), currentTick + ticksPerChord);
            }
        }

        // Add MIDI notes for this chord
        for (int note : chord)
        {
            if (note >= 0 && note < 128)
            {
                track.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) 90), currentTick);
                track.addEvent(juce::MidiMessage::noteOff(1, note), currentTick + ticksPerChord);
            }
        }

        currentTick += ticksPerChord;
    }

    midiFile.addTrack(track);
    return midiFile;
}

void WebUIComponent::exportMidiFile()
{
    if (customProgressionDegrees.empty())
    {
        DBG("MIDI export: no progression to export");
        return;
    }

    auto midiFile = buildMidiFile();

    auto chooser = std::make_shared<juce::FileChooser>(
        "Save MIDI File",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("chord_progression.mid"),
        "*.mid"
    );

    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [chooser, midiFile](const juce::FileChooser& fc) mutable
        {
            auto result = fc.getResult();
            if (result == juce::File())
                return;

            if (!result.hasFileExtension(".mid"))
                result = result.withFileExtension(".mid");

            if (result.existsAsFile())
                result.deleteFile();

            juce::FileOutputStream stream(result);
            if (stream.openedOk())
            {
                midiFile.writeTo(stream);
                stream.flush();
                DBG("MIDI exported to: " << result.getFullPathName());
            }
        });
}

void WebUIComponent::startMidiDrag()
{
    if (customProgressionDegrees.empty())
        return;

    auto midiFile = buildMidiFile();

    // Write to a temp file
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile(juce::Uuid().toString() + "_chord_progression.mid");

    juce::FileOutputStream stream(tempFile);
    if (!stream.openedOk())
    {
        DBG("MIDI drag: failed to create temp file");
        return;
    }

    midiFile.writeTo(stream);
    stream.flush();

    DBG("MIDI drag: wrote temp file " << tempFile.getFullPathName());

    juce::StringArray files;
    files.add(tempFile.getFullPathName());

    // Start native drag-and-drop — user can drop into DAW, Finder, etc.
    juce::DragAndDropContainer::performExternalDragDropOfFiles(
        files, true, this,
        [tempFile]() {
            // Clean up temp file when drag ends
            tempFile.deleteFile();
            DBG("MIDI drag completed, temp file cleaned up");
        });
}

void WebUIComponent::selectChordForEmotionWheel(int chordIndex)
{
    selectedChordIndexForEmotion = chordIndex;
    emitStateChanged();
    emitEmotionLabelsChanged();
}

void WebUIComponent::loadSelectedProgression(int presetId)
{
    if (presetId <= 1) // "Select Progression..." or invalid
        return;

    // Clear current progression
    clearCustomProgression();
    currentProgression.clear();

    // Check if we're in major or minor key
    bool isMinorKey = (keyManager.getCurrentTonality() == KeyManager::Tonality::Minor);

    // Define progressions using ChordDefinition (scale degree, chord type, inversion)
    std::vector<ChordDefinition> chordDefs;

    switch (presetId)
    {
        case 2: // Progression 1
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Minor),
                    ChordDefinition(6, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(2, KeyManager::ChordType::Minor),
                    ChordDefinition(4, KeyManager::ChordType::Major)
                };
            }
            break;

        case 3: // Progression 2
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(5, KeyManager::ChordType::MinorAdd9),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(3, KeyManager::ChordType::MinorAdd9),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(2, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 4: // Progression 3
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Add4),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Add4),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Major)
                };
            }
            break;

        case 5: // Progression 4
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Sus4),
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(3, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Sus4),
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(1, KeyManager::ChordType::Major)
                };
            }
            break;

        case 6: // Progression 5
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Sus4),
                    ChordDefinition(4, KeyManager::ChordType::Minor),
                    ChordDefinition(6, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Sus4),
                    ChordDefinition(2, KeyManager::ChordType::Minor),
                    ChordDefinition(4, KeyManager::ChordType::Major)
                };
            }
            break;

        case 7: // Progression 6
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(2, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major)
                };
            }
            break;

        case 8: // Progression 7 (same as 6)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(2, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major)
                };
            }
            break;

        case 9: // Progression 8
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add11)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add11)
                };
            }
            break;

        case 10: // Progression 9
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(2, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 11: // Progression 10 (same for major and minor)
            chordDefs = {
                ChordDefinition(1, KeyManager::ChordType::Minor),
                ChordDefinition(7, KeyManager::ChordType::Major),
                ChordDefinition(6, KeyManager::ChordType::Major),
                ChordDefinition(7, KeyManager::ChordType::Major)
            };
            break;

        case 12: // Progression 11
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 13: // Progression 12
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 14: // Progression 13
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 15: // Progression 14
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Minor7)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(2, KeyManager::ChordType::Minor7)
                };
            }
            break;

        case 16: // Progression 15
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::MinorAdd9)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(2, KeyManager::ChordType::Minor7),
                    ChordDefinition(2, KeyManager::ChordType::MinorAdd9)
                };
            }
            break;

        case 17: // Progression 16
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Major)
                };
            }
            break;

        case 18: // Progression 17
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Major)
                };
            }
            break;

        case 19: // Progression 18
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Sus2),
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Sus2),
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(4, KeyManager::ChordType::Major)
                };
            }
            break;

        case 20: // Progression 19
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(6, KeyManager::ChordType::Major7)
                };
            } else {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(4, KeyManager::ChordType::Major7)
                };
            }
            break;

        case 21: // Progression 20
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Sus2),
                    ChordDefinition(7, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Sus2),
                    ChordDefinition(5, KeyManager::ChordType::Major)
                };
            }
            break;

        case 22: // Progression 21
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add4)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add4)
                };
            }
            break;

        case 23: // Progression 22
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Sixth),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Sixth),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Major)
                };
            }
            break;

        case 24: // Progression 23
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(7, KeyManager::ChordType::Add9),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Add9)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(5, KeyManager::ChordType::Add9),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Add9)
                };
            }
            break;

        case 25: // Progression 24
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add4),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Add4)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add4),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Add4)
                };
            }
            break;

        case 26: // Progression 25
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add4),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(3, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add4),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(1, KeyManager::ChordType::Major)
                };
            }
            break;

        case 27: // Progression 26
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 28: // Progression 27 (same as 24)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add4),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Add4)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add4),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Add4)
                };
            }
            break;

        case 29: // Progression 28
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add9),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(7, KeyManager::ChordType::Add9)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add9),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(5, KeyManager::ChordType::Add9)
                };
            }
            break;

        case 30: // Progression 29
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(5, KeyManager::ChordType::Minor7),
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(7, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(3, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(5, KeyManager::ChordType::Major)
                };
            }
            break;

        case 31: // Progression 30
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Major)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Major)
                };
            }
            break;

        case 32: // Progression 31
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(3, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Add4),
                    ChordDefinition(1, KeyManager::ChordType::Minor)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Add4),
                    ChordDefinition(6, KeyManager::ChordType::Minor)
                };
            }
            break;

        case 33: // Progression 32
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major),
                    ChordDefinition(7, KeyManager::ChordType::Sus4),
                    ChordDefinition(1, KeyManager::ChordType::Minor),
                    ChordDefinition(4, KeyManager::ChordType::Minor7)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major),
                    ChordDefinition(5, KeyManager::ChordType::Sus4),
                    ChordDefinition(6, KeyManager::ChordType::Minor),
                    ChordDefinition(2, KeyManager::ChordType::Minor7)
                };
            }
            break;

        case 34: // Progression 33
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Minor7)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(2, KeyManager::ChordType::Minor7)
                };
            }
            break;

        case 35: // Progression 34 (same as 33)
            if (isMinorKey) {
                chordDefs = {
                    ChordDefinition(6, KeyManager::ChordType::Major7),
                    ChordDefinition(7, KeyManager::ChordType::Major),
                    ChordDefinition(1, KeyManager::ChordType::Minor7),
                    ChordDefinition(4, KeyManager::ChordType::Minor7)
                };
            } else {
                chordDefs = {
                    ChordDefinition(4, KeyManager::ChordType::Major7),
                    ChordDefinition(5, KeyManager::ChordType::Major),
                    ChordDefinition(6, KeyManager::ChordType::Minor7),
                    ChordDefinition(2, KeyManager::ChordType::Minor7)
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

    DBG("Loaded progression preset " << presetId << " with " << currentProgression.size() << " chords");
}

std::vector<int> WebUIComponent::generateChordFromDefinition(const ChordDefinition& def)
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

void WebUIComponent::timerCallback()
{
    if (isPreviewPlaying)
    {
        stopCurrentChord();
        currentChordNotes.clear();
        isPreviewPlaying = false;
        stopTimer();
    }
}

//==============================================================================
// Sample loading (same logic as MainComponent)
//==============================================================================
void WebUIComponent::loadSamples(const juce::File& sampleDirectory)
{
    DBG("WebUI: Loading samples from: " << sampleDirectory.getFullPathName());

    auto files = sampleDirectory.findChildFiles(juce::File::findFiles, false, "*.aif");
    for (auto& file : files)
    {
        auto noteName = file.getFileNameWithoutExtension();
        int midiNote = parseMidiNoteFromName(noteName);
        if (midiNote >= 0)
            loadSample(midiNote, file);
    }
}

void WebUIComponent::loadSample(int midiNote, const juce::File& audioFile)
{
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(audioFile));
    if (reader != nullptr)
    {
        juce::BigInteger range;
        range.setBit(midiNote);
        synth.addSound(new juce::SamplerSound(audioFile.getFileNameWithoutExtension(),
                                               *reader, range, midiNote, 0.01, 0.1, 10.0));
    }
}

int WebUIComponent::parseMidiNoteFromName(const juce::String& noteName)
{
    // Same parsing as MainComponent
    static const std::map<juce::String, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"D", 2}, {"D#", 3}, {"E", 4}, {"F", 5},
        {"F#", 6}, {"G", 7}, {"G#", 8}, {"A", 9}, {"A#", 10}, {"B", 11}
    };

    juce::String noteStr;
    int octave = -1;

    for (int i = 0; i < noteName.length(); ++i)
    {
        if (juce::CharacterFunctions::isDigit(noteName[i]) || noteName[i] == '-')
        {
            noteStr = noteName.substring(0, i);
            octave = noteName.substring(i).getIntValue();
            break;
        }
    }

    auto it = noteMap.find(noteStr);
    if (it != noteMap.end() && octave >= -1 && octave <= 9)
        return (octave + 1) * 12 + it->second;

    return -1;
}

void WebUIComponent::tryInitializeAudioDevice()
{
    // Basic audio device initialization
}

void WebUIComponent::detectSystemAudioDevices()
{
    // Basic detection
}

//==============================================================================
// AudioAppComponent overrides
//==============================================================================
void WebUIComponent::prepareToPlay(int /*samplesPerBlockExpected*/, double sr)
{
    synth.setCurrentPlaybackSampleRate(sr);
    sampleRate = sr;
    samplesPerBeat = static_cast<int>((60.0 / 120.0) * sr);
}

void WebUIComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    if (isPlaying && !currentProgression.empty())
    {
        auto numSamples = bufferToFill.numSamples;
        int samplePosition = 0;

        while (samplePosition < numSamples)
        {
            if (samplesUntilNextChord <= 0)
            {
                if (currentChordIndex >= static_cast<int>(currentProgression.size()))
                {
                    if (shouldLoop)
                        currentChordIndex = 0;
                    else
                    {
                        juce::MessageManager::callAsync([this]() { stopProgression(); });
                        break;
                    }
                }

                stopCurrentChord();
                playChord(currentProgression[currentChordIndex]);
                currentChordIndex++;
                samplesUntilNextChord = samplesPerBeat * beatsPerMeasure;
            }

            int samplesToProcess = juce::jmin(numSamples - samplePosition, samplesUntilNextChord);

            juce::MidiBuffer emptyMidi;
            juce::AudioSourceChannelInfo subBlock(bufferToFill.buffer, samplePosition, samplesToProcess);
            synth.renderNextBlock(*bufferToFill.buffer, emptyMidi, samplePosition, samplesToProcess);

            samplePosition += samplesToProcess;
            samplesUntilNextChord -= samplesToProcess;
        }
    }
    else
    {
        juce::MidiBuffer emptyMidi;
        synth.renderNextBlock(*bufferToFill.buffer, emptyMidi, 0, bufferToFill.numSamples);
    }
}

void WebUIComponent::releaseResources()
{
    stopProgression();
}

//==============================================================================
void WebUIComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
}

void WebUIComponent::resized()
{
    if (webBrowser != nullptr)
        webBrowser->setBounds(getLocalBounds());
}

#endif // JUCE_WEB_BROWSER
