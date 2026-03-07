<script setup>
import { inject, computed, ref } from 'vue'
import { useNeumorphism, angleToOffset, colorLuminance } from '../composables/useNeumorphism'

const cb = inject('chordBuilder')
const nue = useNeumorphism()

const BASE_COLOUR = '#ebebeb'
const INSET_INTENSITY = 0.15
const INSET_DISTANCE = 3
const INSET_BLUR = 3

nue.setLightPosition('top-right')

// ─── Categories ──────────────────────────────────────────────────────────────
const categories = ['Happy', 'Sad', 'Warm', 'Tense', 'Calm', 'Dark']
const activeCategoryIndex = ref(0)

const activeCategory = computed(() => categories[activeCategoryIndex.value])
const prevCategory = computed(() => categories[(activeCategoryIndex.value - 1 + 6) % 6])
const nextCategory = computed(() => categories[(activeCategoryIndex.value + 1) % 6])

function goLeft() {
  activeCategoryIndex.value = (activeCategoryIndex.value - 1 + 6) % 6
}
function goRight() {
  activeCategoryIndex.value = (activeCategoryIndex.value + 1) % 6
}

// ─── Emotion label helpers ───────────────────────────────────────────────────
// Backend sends 24 labels: 6 categories × 4 variants
// Index = categoryIndex * 4 + variantIndex

// Computed array of 4 labels for the active category — reactively updates
const variantLabels = computed(() => {
  const catIdx = activeCategoryIndex.value
  return [0, 1, 2, 3].map(vi => {
    const globalIndex = catIdx * 4 + vi
    const raw = cb.emotionLabels[globalIndex] || `Variant ${vi + 1}`
    // Strip the category prefix e.g. "Happy (Maj6)" → "Maj6"
    const m = raw.match(/\(([^)]+)\)/)
    return m ? m[1] : raw
  })
})

// Computed array of 4 global indices for the active category
const variantGlobalIndices = computed(() => {
  const catIdx = activeCategoryIndex.value
  return [0, 1, 2, 3].map(vi => catIdx * 4 + vi)
})

const isDisabled = computed(() => cb.selectedChordForEmotion.value < 0)

// ─── Alteration / Inversion ──────────────────────────────────────────────────
const alterations = [
  { id: 1, name: 'None' },     { id: 2, name: 'Major' },     { id: 3, name: 'Minor' },
  { id: 4, name: 'Diminished' },{ id: 5, name: 'Augmented' }, { id: 6, name: 'Maj7' },
  { id: 7, name: 'Min7' },     { id: 8, name: 'Dom7' },      { id: 9, name: 'Dim7' },
  { id: 10, name: 'm7♭5' },   { id: 11, name: 'Sus2' },     { id: 12, name: 'Sus4' },
  { id: 13, name: 'Add9' },    { id: 14, name: 'Maj9' },     { id: 15, name: 'Min9' },
  { id: 16, name: 'Dom9' },
]

const inversions = [
  { id: 1, name: 'Root Position' },
  { id: 2, name: '1st Inversion' },
  { id: 3, name: '2nd Inversion' },
  { id: 4, name: '3rd Inversion' },
]

function onAlterationChange(e) {
  console.log('[EW] alteration changed to:', e.target.value, typeof e.target.value);
  cb.setAlteration(Number(e.target.value))
}
function onInversionChange(e) {
  console.log('[EW] inversion changed to:', e.target.value, typeof e.target.value);
  cb.setInversion(Number(e.target.value))
}

// ─── Neumorphic styles ──────────────────────────────────────────────────────
const containerSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR, shape: 'circle', distance: 4, blur: 8,
})
const outerRing = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, shape: 'circle', distance: 6, blur: 12,
})
const rootRaised = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, shape: 'circle', distance: 6, blur: 12,
})
const rootSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR, shape: 'circle', distance: 4, blur: 8,
})

const selectProtruding = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, distance: 3, blur: 6, borderRadius: 8,
})

const hoveredCenter = ref(false)
const hoveredBtn = ref(-1)

// ─── Outer button positions ──────────────────────────────────────────────────
// 6 buttons evenly spaced at 60° intervals (same as ChordBuilder):
//   i=0: top (12 o'clock)     → variant 0
//   i=1: top-right (2 o'clock) → variant 1
//   i=2: bottom-right (4 o'clock) → nav right (next category)
//   i=3: bottom (6 o'clock)   → variant 2
//   i=4: bottom-left (8 o'clock) → nav left (prev category)
//   i=5: top-left (10 o'clock) → variant 3

// Map: which slot index is a nav button?
const NAV_RIGHT_SLOT = 1
const NAV_LEFT_SLOT = 4
// Map slot indices to variant indices (skipping nav slots)
const slotToVariant = { 0: 0, 2: 1, 3: 2, 5: 3 }

function buildButtonPositions() {
  const cx = 50, cy = 50, r = 33
  const positions = []
  for (let i = 0; i < 6; i++) {
    // Start at -60° so slot 1 lands at 0° (3 o'clock) and slot 4 at 180° (9 o'clock)
    const angle = (Math.PI * 2 * i) / 6 - Math.PI / 3
    const rotateDeg = (360 * i) / 6 + 210

    const raised = nue.styleOnce('protruding', {
      baseColour: BASE_COLOUR, clipped: true, rotation: rotateDeg, distance: 3, blur: 4,
    })

    const effectiveAngle = nue.lightAngle.value - rotateDeg
    const { x: sx, y: sy } = angleToOffset(effectiveAngle, INSET_DISTANCE)
    const darkColor = colorLuminance(BASE_COLOUR, -INSET_INTENSITY)
    const lightColor = colorLuminance(BASE_COLOUR, INSET_INTENSITY)

    const left = `${cx + r * Math.cos(angle)}%`
    const top = `${cy + r * Math.sin(angle)}%`

    positions.push({
      wrapper: {
        filter: raised.filter,
        left, top,
        transform: `translate(-50%, -50%) rotate(${rotateDeg}deg)`,
      },
      inner: { background: raised.background },
      filterId: `ew-btn-${i}`,
      sx, sy, darkColor, lightColor, rotateDeg,
      isNav: i === NAV_LEFT_SLOT || i === NAV_RIGHT_SLOT,
      navDir: i === NAV_LEFT_SLOT ? 'left' : i === NAV_RIGHT_SLOT ? 'right' : null,
      variantIndex: slotToVariant[i] ?? -1,
    })
  }
  return positions
}

const allPositions = computed(() => buildButtonPositions())
</script>

<template>
  <div class="emotion-wheel">
    <!-- Alteration / Inversion controls -->
    <div class="wheel-container">
      <div class="outer-ring" :style="outerRing">
      <div class="accent-ring">
      <div class="wheel-area" :style="containerSunken">
        <!-- SVG inset shadow filters for all 6 buttons -->
        <svg width="0" height="0" style="position:absolute">
          <defs>
            <filter v-for="(pos, i) in allPositions" :key="i"
                    :id="pos.filterId" x="-50%" y="-50%" width="200%" height="200%">
              <feComponentTransfer in="SourceAlpha" result="inv-alpha">
                <feFuncA type="table" tableValues="1 0" />
              </feComponentTransfer>
              <feOffset in="inv-alpha" :dx="pos.sx" :dy="pos.sy" result="dark-off" />
              <feGaussianBlur in="dark-off" :stdDeviation="INSET_BLUR" result="dark-blur" />
              <feComposite in="dark-blur" in2="SourceAlpha" operator="in" result="dark-mask" />
              <feFlood :flood-color="pos.darkColor" result="dark-color" />
              <feComposite in="dark-color" in2="dark-mask" operator="in" result="dark-shadow" />
              <feOffset in="inv-alpha" :dx="-pos.sx" :dy="-pos.sy" result="light-off" />
              <feGaussianBlur in="light-off" :stdDeviation="INSET_BLUR" result="light-blur" />
              <feComposite in="light-blur" in2="SourceAlpha" operator="in" result="light-mask" />
              <feFlood :flood-color="pos.lightColor" result="light-color" />
              <feComposite in="light-color" in2="light-mask" operator="in" result="light-shadow" />
              <feMerge>
                <feMergeNode in="SourceGraphic" />
                <feMergeNode in="dark-shadow" />
                <feMergeNode in="light-shadow" />
              </feMerge>
            </filter>
          </defs>
        </svg>

        <!-- Center button — current category name -->
        <button class="center-btn"
                :style="hoveredCenter ? rootSunken : rootRaised"
                @mouseenter="hoveredCenter = true"
                @mouseleave="hoveredCenter = false"
                :title="activeCategory">
          <span class="center-label">{{ activeCategory }}</span>
        </button>

        <!-- 6 outer buttons: 4 variants + 2 nav -->
        <div v-for="(pos, i) in allPositions" :key="i"
             class="outer-wrapper"
             :style="hoveredBtn === i
               ? { ...pos.wrapper, filter: `url(#${pos.filterId})` }
               : pos.wrapper"
             @mouseenter="hoveredBtn = i"
             @mouseleave="hoveredBtn = -1">

          <!-- Nav button -->
          <button v-if="pos.isNav"
                  class="outer-btn nav-btn"
                  :style="pos.inner"
                  @click="pos.navDir === 'left' ? goLeft() : goRight()"
                  :title="pos.navDir === 'left' ? ('← ' + prevCategory) : (nextCategory + ' →')">
            <span class="outer-btn-content"
                  :style="{ transform: `rotate(${-pos.rotateDeg}deg)` }">
              <span class="nav-arrow">{{ pos.navDir === 'left' ? '‹' : '›' }}</span>
              <span class="nav-label">{{ pos.navDir === 'left' ? prevCategory : nextCategory }}</span>
            </span>
          </button>

          <!-- Variant emotion button -->
          <button v-else
                  class="outer-btn variant-btn"
                  :class="{ disabled: isDisabled }"
                  :disabled="isDisabled"
                  :style="pos.inner"
                  @click="cb.applyEmotion(variantGlobalIndices[pos.variantIndex])"
                  @mousedown.right.prevent="cb.previewEmotion(variantGlobalIndices[pos.variantIndex])"
                  :title="variantLabels[pos.variantIndex]">
            <span class="outer-btn-content"
                  :style="{ transform: `rotate(${-pos.rotateDeg}deg)` }">
              <span class="numeral">{{ variantLabels[pos.variantIndex] }}</span>
            </span>
          </button>
        </div>
      </div>
      </div>
      </div>
    </div>
    <div class="controls-row">
      <label>
        Alteration
        <select class="nue-select" :style="selectProtruding"
                :value="cb.selectedAlteration.value" @change="onAlterationChange">
          <option v-for="a in alterations" :key="a.id" :value="a.id">{{ a.name }}</option>
        </select>
      </label>

      <label>
        Inversion
        <select class="nue-select" :style="selectProtruding"
                :value="cb.selectedInversion.value" @change="onInversionChange">
          <option v-for="inv in inversions" :key="inv.id" :value="inv.id">{{ inv.name }}</option>
        </select>
      </label>
    </div>

    <!-- Description -->
    <div class="emotion-desc">{{ cb.emotionDescription.value || 'Select a chord in the progression, then choose an emotion' }}</div>
  </div>
</template>

<style scoped>
.emotion-wheel {
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.wheel-container {
  display: flex;
  align-items: center;
  justify-content: center;
  flex: 1;
  min-height: 0;
}

/* ─── Rings (matches ChordBuilder) ──────────────────────────────────────── */
.outer-ring {
  width: 310px;
  height: 310px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.accent-ring {
  border-radius: 50%;
  background-color: #d78453;
  height: 302px;
  width: 302px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.wheel-area {
  position: relative;
  width: 296px;
  height: 296px;
  aspect-ratio: 1;
  border-radius: 50%;
}

/* ─── Center button ─────────────────────────────────────────────────────── */
.center-btn {
  position: absolute;
  left: 50%;
  top: 50%;
  width: 86px;
  height: 86px;
  transform: translate(-50%, -50%);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  border: none;
  cursor: default;
  transition: filter 0.12s, box-shadow 0.12s;
  color: var(--text-primary);
  background: none;
}

.center-label {
  font-size: 16px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

/* ─── Shared outer button / wrapper styles ──────────────────────────────── */
.outer-wrapper {
  position: absolute;
  transition: filter 0.15s;
}

.outer-btn {
  position: relative;
  width: 110px;
  height: 65px;
  padding: 0;
  border: none;
  cursor: pointer;
  color: var(--text-primary);
  font-weight: 600;
  clip-path: path('M 27,0 Q 55,7 83,0 Q 90,0 92,8 L 108,44 Q 110,52 102,52 Q 55,65 8,52 Q 0,52 2,44 L 18,8 Q 20,0 27,0 Z');
  display: flex;
  align-items: center;
  justify-content: center;
}

.outer-btn.disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.outer-btn-content {
  position: absolute;
  inset: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  pointer-events: none;
  white-space: nowrap;
}

/* ─── Nav buttons (left / right) ────────────────────────────────────────── */
.nav-btn {
  cursor: pointer;
}

.nav-arrow {
  font-size: 22px;
  line-height: 1;
  font-weight: 700;
}

.nav-label {
  font-size: 8px;
  text-transform: uppercase;
  letter-spacing: 0.3px;
  color: var(--text-secondary);
  margin-top: 1px;
  max-width: 70px;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* ─── Variant buttons ───────────────────────────────────────────────────── */
.numeral {
  font-size: 12px;
  line-height: 1.1;
  text-align: center;
  max-width: 80px;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* ─── Controls row ──────────────────────────────────────────────────────── */
.controls-row {
  display: flex;
  gap: 12px;
  align-items: center;
  align-self: stretch;
}

.controls-row label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--text-secondary);
}

.nue-select {
  border: none;
  outline: none;
  padding: 6px 28px 6px 10px;
  font-size: 12px;
  font-weight: 600;
  color: var(--text-primary);
  cursor: pointer;
  appearance: none;
  -webkit-appearance: none;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23888'/%3E%3C/svg%3E");
  background-repeat: no-repeat;
  background-position: right 8px center;
}

.emotion-desc {
  font-size: 11px;
  font-style: italic;
  text-align: center;
  color: var(--text-secondary);
  min-height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
