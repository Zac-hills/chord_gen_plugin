/**
 * Vue composable for JUCE backend communication.
 * Provides reactive state and methods to interact with the C++ chord builder.
 *
 * Reactive state is stored on window.__CB_STATE__ so that Vite HMR
 * re-mounts don't lose data fetched from the backend.
 */
import { ref, reactive, onMounted, onUnmounted } from 'vue';
import { getNativeFunction, addEventListener, removeEventListener, isRunningInJuce } from '../juce';

// Persist reactive state across HMR reloads
if (!window.__CB_STATE__) {
  window.__CB_STATE__ = {
    currentKey: ref(1),
    timeSignature: ref(1),
    tempo: ref(120),
    rootBoost: ref(0.5),
    isPlaying: ref(false),
    isLooping: ref(false),
    selectedAlteration: ref(1),
    selectedInversion: ref(1),
    selectedChordForEmotion: ref(-1),
    progression: reactive([]),
    chordLabels: reactive([]),
    emotionLabels: reactive([]),
    emotionDescription: ref(''),
    connectedToBackend: ref(false),
  };
}

export function useChordBuilder() {
  // ─── Reactive State (persisted across HMR) ─────────────────────
  const {
    currentKey, timeSignature, tempo, rootBoost,
    isPlaying, isLooping, selectedAlteration, selectedInversion,
    selectedChordForEmotion, progression, chordLabels, emotionLabels,
    emotionDescription, connectedToBackend,
  } = window.__CB_STATE__;

  // ─── Native Functions (lazy-loaded) ────────────────────────────
  let _nativeFns = null;

  function nativeFns() {
    if (!_nativeFns) {
      _nativeFns = {
        setKey:              getNativeFunction('setKey'),
        setTimeSignature:    getNativeFunction('setTimeSignature'),
        setTempo:            getNativeFunction('setTempo'),
        setRootBoost:        getNativeFunction('setRootBoost'),
        addChord:            getNativeFunction('addChord'),
        removeChord:         getNativeFunction('removeChord'),
        clearProgression:    getNativeFunction('clearProgression'),
        playProgression:     getNativeFunction('playProgression'),
        stopProgression:     getNativeFunction('stopProgression'),
        toggleLoop:          getNativeFunction('toggleLoop'),
        previewChord:        getNativeFunction('previewChord'),
        previewEmotion:      getNativeFunction('previewEmotion'),
        selectChordForEmotion: getNativeFunction('selectChordForEmotion'),
        applyEmotion:        getNativeFunction('applyEmotion'),
        setAlteration:       getNativeFunction('setAlteration'),
        setInversion:        getNativeFunction('setInversion'),
        loadProgression:     getNativeFunction('loadProgression'),
        exportMidi:          getNativeFunction('exportMidi'),
        startMidiDrag:       getNativeFunction('startMidiDrag'),
        getState:            getNativeFunction('getState'),
      };
    }
    return _nativeFns;
  }

  // ─── Event Listener Tokens ─────────────────────────────────────
  const tokens = [];

  function listen(eventId, callback) {
    const token = addEventListener(eventId, callback);
    if (token) tokens.push(token);
  }

  // Helper: sync progression from NativeFunction response
  function _syncProgression(result) {
    console.log('[CB] _syncProgression called, result:', JSON.stringify(result));
    if (result?.chords) {
      console.log('[CB] syncing', result.chords.length, 'chords');
      progression.splice(0, progression.length, ...result.chords);
    } else {
      console.warn('[CB] _syncProgression: no chords in result');
    }
  }

  function _syncPlayState(result) {
    if (result?.isPlaying !== undefined) isPlaying.value = result.isPlaying;
  }

  // ─── Actions ───────────────────────────────────────────────────
  async function setKey(id) {
    currentKey.value = id;
    const result = await nativeFns().setKey(id);
    if (result && result.labels) {
      chordLabels.splice(0, chordLabels.length, ...result.labels);
    }
  }

  async function setTimeSignature(id) {
    timeSignature.value = id;
    await nativeFns().setTimeSignature(id);
  }

  async function setTempo(bpm) {
    tempo.value = bpm;
    await nativeFns().setTempo(bpm);
  }

  async function setRootBoost(value) {
    rootBoost.value = value;
    await nativeFns().setRootBoost(value);
  }

  async function addChord(scaleDegree) {
    const result = await nativeFns().addChord(scaleDegree);
    _syncProgression(result);
  }

  async function removeChord(index) {
    const result = await nativeFns().removeChord(index);
    _syncProgression(result);
  }

  async function clearProgression() {
    const result = await nativeFns().clearProgression();
    _syncProgression(result);
  }

  async function playProgression() {
    const result = await nativeFns().playProgression();
    _syncPlayState(result);
  }

  async function stopProgression() {
    const result = await nativeFns().stopProgression();
    _syncPlayState(result);
  }

  async function toggleLoop() {
    await nativeFns().toggleLoop();
  }

  async function previewChord(scaleDegree) {
    await nativeFns().previewChord(scaleDegree);
  }

  async function previewEmotion(emotionIndex) {
    await nativeFns().previewEmotion(emotionIndex);
  }

  async function selectChordForEmotion(chordIndex) {
    selectedChordForEmotion.value = chordIndex;
    await nativeFns().selectChordForEmotion(chordIndex);
  }

  async function applyEmotion(emotionIndex) {
    const result = await nativeFns().applyEmotion(emotionIndex);
    _syncProgression(result);
  }

  async function setAlteration(id) {
    console.log('[CB] setAlteration called with id:', id, 'selectedChord:', selectedChordForEmotion.value);
    selectedAlteration.value = id;
    const result = await nativeFns().setAlteration(id);
    console.log('[CB] setAlteration native result:', JSON.stringify(result));
    _syncProgression(result);
  }

  async function setInversion(id) {
    console.log('[CB] setInversion called with id:', id, 'selectedChord:', selectedChordForEmotion.value);
    selectedInversion.value = id;
    const result = await nativeFns().setInversion(id);
    console.log('[CB] setInversion native result:', JSON.stringify(result));
    _syncProgression(result);
  }

  async function loadProgression(id) {
    const result = await nativeFns().loadProgression(id);
    _syncProgression(result);
  }

  async function exportMidi() {
    await nativeFns().exportMidi();
  }

  async function startMidiDrag() {
    await nativeFns().startMidiDrag();
  }

  // ─── Lifecycle ─────────────────────────────────────────────────
  onMounted(() => {
    connectedToBackend.value = isRunningInJuce();

    // Listen for state updates from C++ backend
    listen('stateChanged', (state) => {
      if (state.key !== undefined) currentKey.value = state.key;
      if (state.timeSignature !== undefined) timeSignature.value = state.timeSignature;
      if (state.tempo !== undefined) tempo.value = state.tempo;
      if (state.rootBoost !== undefined) rootBoost.value = state.rootBoost;
      if (state.isPlaying !== undefined) isPlaying.value = state.isPlaying;
      if (state.isLooping !== undefined) isLooping.value = state.isLooping;
      if (state.selectedAlteration !== undefined) selectedAlteration.value = state.selectedAlteration;
      if (state.selectedInversion !== undefined) selectedInversion.value = state.selectedInversion;
      if (state.selectedChordForEmotion !== undefined) selectedChordForEmotion.value = state.selectedChordForEmotion;
      if (state.emotionDescription !== undefined) emotionDescription.value = state.emotionDescription;
    });

    listen('progressionChanged', (data) => {
      console.log('[CB] progressionChanged event received, chords:', data.chords?.length);
      progression.splice(0, progression.length, ...(data.chords || []));
    });

    listen('chordLabelsChanged', (data) => {
      chordLabels.splice(0, chordLabels.length, ...(data.labels || []));
    });

    listen('emotionLabelsChanged', (data) => {
      emotionLabels.splice(0, emotionLabels.length, ...(data.labels || []));
    });

    // Request initial state from backend
    if (connectedToBackend.value) {
      nativeFns().getState().then((state) => {
        if (state) {
          if (state.key !== undefined) currentKey.value = state.key;
          if (state.timeSignature !== undefined) timeSignature.value = state.timeSignature;
          if (state.tempo !== undefined) tempo.value = state.tempo;
          if (state.rootBoost !== undefined) rootBoost.value = state.rootBoost;
          if (state.isPlaying !== undefined) isPlaying.value = state.isPlaying;
          if (state.isLooping !== undefined) isLooping.value = state.isLooping;
          if (state.labels) {
            chordLabels.splice(0, chordLabels.length, ...state.labels);
          }
          if (state.chords) {
            progression.splice(0, progression.length, ...state.chords);
          }
        }
      });
    }
  });

  onUnmounted(() => {
    tokens.forEach(t => removeEventListener(t));
    tokens.length = 0;
  });

  return {
    // State
    currentKey, timeSignature, tempo, rootBoost,
    isPlaying, isLooping,
    selectedAlteration, selectedInversion, selectedChordForEmotion,
    progression, chordLabels, emotionLabels, emotionDescription,
    connectedToBackend,
    // Actions
    setKey, setTimeSignature, setTempo, setRootBoost,
    addChord, removeChord, clearProgression,
    playProgression, stopProgression, toggleLoop,
    previewChord, previewEmotion,
    selectChordForEmotion, applyEmotion,
    setAlteration, setInversion,
    loadProgression, exportMidi, startMidiDrag,
  };
}
