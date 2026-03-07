<script setup>
import { inject, computed, ref } from 'vue'
import { useNeumorphism } from '../composables/useNeumorphism'

const cb = inject('chordBuilder')
const nue = useNeumorphism()

const BASE_COLOUR = '#ebebeb'

const romanNumerals = ['I', 'II', 'III', 'IV', 'V', 'VI', 'VII']

// Neumorphic styles
const cardRaised = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, distance: 4, blur: 8, borderRadius: 10,
})
const cardSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR, distance: 3, blur: 6, borderRadius: 10,
})
const btnRaised = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, distance: 3, blur: 6, borderRadius: 8,
})
const btnSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR, distance: 2, blur: 4, borderRadius: 8,
})
const emptySunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR, distance: 2, blur: 5, borderRadius: 10,
})
const chordContainer = nue.styleOnce('protruding', {
    baseColour: BASE_COLOUR, distance: 4, blur: 8, borderRadius: 12,
})

const MAX_SLOTS = 8

const slots = computed(() => {
  const filled = cb.progression.map((chord, i) => ({ type: 'chord', chord, index: i }))
  const empty = Array.from({ length: MAX_SLOTS - filled.length }, (_, i) => ({
    type: 'empty', index: filled.length + i,
  }))
  return [...filled, ...empty]
})

// Context menu state
const contextMenuIndex = ref(-1)
const contextMenuX = ref(0)
const contextMenuY = ref(0)

function onCardRightClick(e, index) {
  e.preventDefault()
  contextMenuIndex.value = index
  contextMenuX.value = e.clientX
  contextMenuY.value = e.clientY
}

function closeContextMenu() {
  contextMenuIndex.value = -1
}

function deleteChord() {
  if (contextMenuIndex.value >= 0) {
    cb.removeChord(contextMenuIndex.value)
  }
  closeContextMenu()
}

function getDegreeLabel(chord) {
  return chord.label || romanNumerals[(chord.degree || 1) - 1] || '?'
}

function getChordName(chord) {
  return chord.chordName || ''
}

function getNoteName(chord) {
  if (chord.noteName) return chord.noteName
  const idx = (chord.degree || 1) - 1
  const raw = cb.chordLabels[idx]
  if (raw) {
    const parts = raw.split(/\n|\\n/)
    return parts[1] || ''
  }
  return ''
}

function togglePlayStop() {
  if (cb.isPlaying.value) {
    cb.stopProgression()
  } else {
    cb.playProgression()
  }
}
</script>

<template>
  <div class="progression-bar" @click="closeContextMenu">
    <!-- Chord cards -->
    <div class="chord-cards" :style="chordContainer">
      <div v-for="slot in slots" :key="slot.index"
           class="chord-card"
           :class="{ 'empty-slot': slot.type === 'empty' }"
           :style="slot.type === 'chord'
             ? (cb.selectedChordForEmotion.value === slot.index ? cardSunken : cardRaised)
             : emptySunken"
           @click.stop="slot.type === 'chord' ? cb.selectChordForEmotion(slot.index) : null"
           @contextmenu="slot.type === 'chord' ? onCardRightClick($event, slot.index) : null">
        <div v-if="slot.type === 'chord'" class="card-label">
          <span class="degree">{{ getDegreeLabel(slot.chord) }}</span>
          <span class="chord-name">{{ getChordName(slot.chord) || getNoteName(slot.chord) }}</span>
          <span v-if="slot.chord.emotion" class="emotion-tag">{{ slot.chord.emotion }}</span>
        </div>
        <span v-else class="empty-label">{{ slot.index + 1 }}</span>
      </div>
    </div>

    <!-- Play controls -->
    <div class="play-controls">
      <button class="ctrl-btn play-btn"
              :style="cb.isPlaying.value ? btnSunken : btnRaised"
              @click="togglePlayStop">
        {{ cb.isPlaying.value ? '■ Stop' : '▶ Play' }}
      </button>
      <button class="ctrl-btn loop-btn"
              :style="cb.isLooping.value ? btnSunken : btnRaised"
              @click="cb.toggleLoop()">
        🔁 Loop
      </button>
      <button class="ctrl-btn midi-btn"
              :style="btnRaised"
              @mousedown.prevent="cb.startMidiDrag()"
              @contextmenu.prevent="cb.exportMidi()"
              title="Drag to DAW · Right-click to save">
        🎹 Drag MIDI
      </button>
    </div>

    <!-- Right-click context menu -->
    <Teleport to="body">
      <div v-if="contextMenuIndex >= 0"
           class="context-overlay"
           @click="closeContextMenu"
           @contextmenu.prevent="closeContextMenu">
        <div class="context-menu"
             :style="{ left: contextMenuX + 'px', top: contextMenuY + 'px' }"
             @click.stop>
          <button class="context-item danger" @click="deleteChord">
            🗑 Remove Chord
          </button>
        </div>
      </div>
    </Teleport>
  </div>
</template>

<style scoped>
.progression-bar {
  display: flex;
  align-items: stretch;
  gap: 12px;
  padding: 10px 12px;
  background: var(--bg-secondary);
  min-height: 100px;
  flex-shrink: 0;
}

.chord-cards {
  display: flex;
  justify-content: space-evenly;
  flex: 1;
  align-items: center;
  padding: 6px 12px;
}

.chord-card {
  position: relative;
  width: 80px;
  height: 72px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: box-shadow 0.15s, background 0.15s;
  flex-shrink: 0;
  user-select: none;
}

.chord-card.empty-slot {
  cursor: default;
  opacity: 0.45;
}

.empty-label {
  font-size: 13px;
  font-weight: 600;
  color: var(--text-secondary);
  opacity: 0.5;
}

.card-label {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  text-align: center;
}

.card-label .degree {
  font-size: 15px;
  font-weight: 700;
  color: var(--text-primary);
}

.card-label .chord-name {
  font-size: 11px;
  font-weight: 500;
  color: var(--text-secondary);
  opacity: 0.8;
}

.card-label .emotion-tag {
  font-size: 9px;
  font-weight: 600;
  color: var(--accent-primary);
  background: rgba(233, 69, 96, 0.1);
  padding: 1px 4px;
  border-radius: 3px;
  max-width: 70px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.empty-hint {
  color: var(--text-secondary);
  font-style: italic;
  font-size: 12px;
}

/* ─── Play controls ─────────────────────────────────────────────────── */
.play-controls {
  display: flex;
  flex-direction: column;
  gap: 6px;
  min-width: 110px;
  justify-content: center;
}

.ctrl-btn {
  border: none;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  padding: 8px 14px;
  color: var(--text-primary);
  transition: box-shadow 0.15s, background 0.15s;
}

.midi-btn {
  font-size: 11px;
}

/* ─── Context menu ──────────────────────────────────────────────────── */
.context-overlay {
  position: fixed;
  inset: 0;
  z-index: 9999;
}

.context-menu {
  position: fixed;
  background: #f0f0f0;
  border-radius: 8px;
  box-shadow: 0 4px 16px rgba(0,0,0,0.18), 0 1px 4px rgba(0,0,0,0.1);
  padding: 4px;
  min-width: 150px;
  z-index: 10000;
}

.context-item {
  display: block;
  width: 100%;
  padding: 8px 12px;
  border: none;
  background: none;
  cursor: pointer;
  font-size: 13px;
  font-weight: 500;
  text-align: left;
  border-radius: 5px;
  color: var(--text-primary);
}

.context-item:hover {
  background: rgba(0,0,0,0.06);
}

.context-item.danger {
  color: #d9534f;
}

.context-item.danger:hover {
  background: rgba(217, 83, 79, 0.1);
}
</style>
