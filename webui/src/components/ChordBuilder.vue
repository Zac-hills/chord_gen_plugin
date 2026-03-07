<script setup>
import { inject, computed, ref } from 'vue'
import { useNeumorphism, angleToOffset, colorLuminance } from '../composables/useNeumorphism'

const cb = inject('chordBuilder')
const nue = useNeumorphism()
const hoveredBtn = ref(-1)
const romanNumerals = ['I', 'II', 'III', 'IV', 'V', 'VI', 'VII']

const BASE_COLOUR = '#ebebeb'
const INSET_INTENSITY = 0.15
const INSET_DISTANCE = 3
const INSET_BLUR = 3

const hoveredRoot = ref(false)

nue.setLightPosition('top-right')

// Neumorphic styles for the container shape
const containerSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR,
  shape: 'circle',
  distance: 4,
  blur: 8,
})
const outerRing = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR,
  shape: 'circle',
  distance: 6,
  blur: 12,
})
// Neumorphic styles for the center circle button
const rootRaised = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR,
  shape: 'circle',
  distance: 6,
  blur: 12,
})
const rootSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR,
  shape: 'circle',
  distance: 4,
  blur: 8,
})

const selectProtruding = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR,
  distance: 3,
  blur: 6,
  borderRadius: 8,
})

const progressionPresets = [
  { id: 1, name: 'Select Progression...' },
  ...Array.from({ length: 34 }, (_, i) => ({ id: i + 2, name: `Progression ${i + 1}` }))
]

function onPresetChange(e) {
  cb.loadProgression(Number(e.target.value))
}

// Get label for each chord button (includes note names from backend)
// Backend sends "I\nC", "II\nD", etc. — split to get just the note name.
function getNoteName(index) {
  const labels = cb.chordLabels
  if (!labels[index]) return ''
  const raw = labels[index]
  // Try splitting on newline (real or escaped)
  const parts = raw.split(/\\n|\n/)
  return parts.length > 1 ? parts[parts.length - 1] : raw
}

// Circular layout positions for the 6 outer buttons (trapezoid via clip-path, wide side outward)
const outerPositions = computed(() => {
  const cx = 50  // center %
  const cy = 50
  const r = 33   // radius %
  const positions = []
  for (let i = 0; i < 6; i++) {
    const angle = (Math.PI * 2 * i) / 6 - Math.PI / 2
    const rotateDeg = (360 * i) / 6 + 180

    const raised = nue.styleOnce('protruding', {
      baseColour: BASE_COLOUR,
      clipped: true,
      rotation: rotateDeg,
      distance: 3,
      blur: 4,
    })

    // Compute inset shadow offset (rotation-compensated) for SVG filter
    const effectiveAngle = nue.lightAngle.value - rotateDeg
    const { x: sx, y: sy } = angleToOffset(effectiveAngle, INSET_DISTANCE)
    const darkColor  = colorLuminance(BASE_COLOUR, -INSET_INTENSITY)
    const lightColor = colorLuminance(BASE_COLOUR,  INSET_INTENSITY)

    const left = `${cx + r * Math.cos(angle)}%`
    const top  = `${cy + r * Math.sin(angle)}%`
    positions.push({
      wrapper: {
        filter: raised.filter,
        left, top,
        transform: `translate(-50%, -50%) rotate(${rotateDeg}deg)`,
      },
      inner: {
        background: raised.background,
      },
      // SVG inset shadow data
      filterId: `nm-inset-${i}`,
      sx, sy,
      darkColor,
      lightColor,
      rotateDeg,
    })
  }
  return positions
})
</script>

<template>
  <div class="chord-builder">
    <div class="outer-ring" :style="outerRing">
    <div class="blue-ring">
    <div class="wheel-area" :style="containerSunken">
      <!-- SVG filters for true inset neumorphic shadows (follows clip-path shape) -->
      <svg width="0" height="0" style="position:absolute">
        <defs>
          <filter v-for="(pos, i) in outerPositions" :key="i"
                  :id="pos.filterId" x="-50%" y="-50%" width="200%" height="200%">
            <!-- Invert SourceAlpha: shape→transparent, outside→opaque -->
            <feComponentTransfer in="SourceAlpha" result="inv-alpha">
              <feFuncA type="table" tableValues="1 0" />
            </feComponentTransfer>
            <!-- Dark shadow: offset inverted alpha, blur, clip back into shape, colorize -->
            <feOffset in="inv-alpha" :dx="pos.sx" :dy="pos.sy" result="dark-off" />
            <feGaussianBlur in="dark-off" :stdDeviation="INSET_BLUR" result="dark-blur" />
            <feComposite in="dark-blur" in2="SourceAlpha" operator="in" result="dark-mask" />
            <feFlood :flood-color="pos.darkColor" result="dark-color" />
            <feComposite in="dark-color" in2="dark-mask" operator="in" result="dark-shadow" />
            <!-- Light shadow: offset opposite direction, blur, clip, colorize -->
            <feOffset in="inv-alpha" :dx="-pos.sx" :dy="-pos.sy" result="light-off" />
            <feGaussianBlur in="light-off" :stdDeviation="INSET_BLUR" result="light-blur" />
            <feComposite in="light-blur" in2="SourceAlpha" operator="in" result="light-mask" />
            <feFlood :flood-color="pos.lightColor" result="light-color" />
            <feComposite in="light-color" in2="light-mask" operator="in" result="light-shadow" />
            <!-- Merge: original + both inset shadows -->
            <feMerge>
              <feMergeNode in="SourceGraphic" />
              <feMergeNode in="dark-shadow" />
              <feMergeNode in="light-shadow" />
            </feMerge>
          </filter>
        </defs>
      </svg>

      <!-- Center (root / I) button -->
      <button class="chord-btn root-btn"
              :style="hoveredRoot ? rootSunken : rootRaised"
              @mouseenter="hoveredRoot = true"
              @mouseleave="hoveredRoot = false"
              @click="cb.addChord(1)"
              @mousedown.right.prevent="cb.previewChord(1)"
              :title="romanNumerals[0] + ' ' + getNoteName(0)">
        <span class="numeral">I</span>
        <span class="notes">{{ getNoteName(0) }}</span>
      </button>

      <!-- Outer 6 buttons (II–VII) — trapezoid via clip-path, wide side outward -->
      <div v-for="(pos, i) in outerPositions" :key="i"
           class="outer-wrapper"
           :style="hoveredBtn === i
             ? { ...pos.wrapper, filter: `url(#${pos.filterId})` }
             : pos.wrapper"
           @mouseenter="hoveredBtn = i"
           @mouseleave="hoveredBtn = -1">
        <button class="chord-btn outer-btn"
                :style="pos.inner"
                @click="cb.addChord(i + 2)"
                @mousedown.right.prevent="cb.previewChord(i + 2)"
                :title="romanNumerals[i + 1] + ' ' + getNoteName(i + 1)">
          <span class="outer-btn-content" :style="{ transform: `rotate(${-pos.rotateDeg}deg)` }">
            <span class="numeral">{{ romanNumerals[i + 1] }}</span>
            <span class="notes">{{ getNoteName(i + 1) }}</span>
          </span>
        </button>
      </div>
    </div>
    </div>
    </div>
    <select class="preset-dropdown" :style="selectProtruding" @change="onPresetChange">
      <option v-for="p in progressionPresets" :key="p.id" :value="p.id">{{ p.name }}</option>
    </select>
  </div>
</template>

<style scoped>
.chord-builder {
  height: 100%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: space-around;
  gap: 10px;
}

.preset-dropdown {
  width: 180px;
  align-self: flex-start;
  border: none;
  outline: none;
  padding: 8px 12px;
  font-size: 13px;
  font-weight: 600;
  color: var(--text-primary);
  cursor: pointer;
  appearance: none;
  -webkit-appearance: none;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23888'/%3E%3C/svg%3E");
  background-repeat: no-repeat;
  background-position: right 10px center;
  padding-right: 28px;
  margin-top: -22px;
  margin-left: auto;
  margin-right: auto;
}

.wheel-area {
  position: relative;
  width: 296px;
  height: 296px;
  border-radius: 50%;
}
.outer-ring {
    width: 310px;
    height: 310px;
    display: flex;
    align-items: center;
    justify-content: center;
    margin-top:24px;
}
.chord-btn {
  position: absolute;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  font-weight: 600;
  transition: filter 0.12s, box-shadow 0.12s;
  color: var(--text-primary);
  background: none;
  border: none;
  cursor: pointer;
}

.root-btn {
  left: 50%;
  top: 50%;
  width: 78px;
  height: 78px;
  transform: translate(-50%, -50%);
}

.outer-wrapper {
  position: absolute;
  /* filter (neumorphism drop-shadow) applied via inline style */
  transition: filter 0.15s;
}

.blue-ring {
    border-radius: 50%;
    background-color: #50bddb;
    height: 302px;
    width: 302px;
    display: flex;
    align-items: center;
    justify-content: center;
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
  /* Arc-segment shape: wider than tall, curving around the center circle.
     Top (inner) edge: concave arc (narrow, ~70px, bows toward body).
     Bottom (outer) edge: convex arc (full width, bows outward).
     Left/right sides: straight radial edges converging toward top.
     All 4 corners rounded via quadratic bezier curves. */
  clip-path: path('M 27,0 Q 55,7 83,0 Q 90,0 92,8 L 108,44 Q 110,52 102,52 Q 55,65 8,52 Q 0,52 2,44 L 18,8 Q 20,0 27,0 Z');
  display: flex;
  align-items: center;
  justify-content: center;
}

.outer-btn-content {
  /* Fill the button, flex-center the text, then counter-rotate */
  position: absolute;
  inset: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  pointer-events: none;
  white-space: nowrap;
}

.numeral {
  font-size: 18px;
  line-height: 1;
}

.notes {
  font-size: 9px;
  color: var(--text-secondary);
  margin-top: 2px;
  max-width: 80px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>
