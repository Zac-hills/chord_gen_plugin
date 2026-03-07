<script setup>
import { inject, ref } from 'vue'
import { useNeumorphism } from '../composables/useNeumorphism'

const cb = inject('chordBuilder')
const nue = useNeumorphism()

const BASE_COLOUR = '#ebebeb'
const selectProtruding = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, distance: 3, blur: 6, borderRadius: 8,
})

const inputProtruding = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, distance: 3, blur: 6, borderRadius: 8,
})

const circleSunken = nue.styleOnce('sunken', {
  baseColour: BASE_COLOUR, shape: 'circle', distance: 4, blur: 8,
})

const keys = [
  { id: 1, name: 'C Major' },   { id: 2, name: 'C♯ Major' },  { id: 3, name: 'D Major' },
  { id: 4, name: 'E♭ Major' },  { id: 5, name: 'E Major' },   { id: 6, name: 'F Major' },
  { id: 7, name: 'F♯ Major' },  { id: 8, name: 'G Major' },   { id: 9, name: 'A♭ Major' },
  { id: 10, name: 'A Major' },  { id: 11, name: 'B♭ Major' }, { id: 12, name: 'B Major' },
  { id: 13, name: 'A Minor' },  { id: 14, name: 'A♯ Minor' }, { id: 15, name: 'B Minor' },
  { id: 16, name: 'C Minor' },  { id: 17, name: 'C♯ Minor' }, { id: 18, name: 'D Minor' },
  { id: 19, name: 'D♯ Minor' }, { id: 20, name: 'E Minor' },  { id: 21, name: 'F Minor' },
  { id: 22, name: 'F♯ Minor' }, { id: 23, name: 'G Minor' },  { id: 24, name: 'G♯ Minor' },
]

const timeSigs = [
  { id: 1, name: '4/4' }, { id: 2, name: '3/4' }, { id: 3, name: '6/8' },
  { id: 4, name: '5/4' }, { id: 5, name: '7/8' }, { id: 6, name: '2/4' },
]

const sliderStyle = nue.styleOnce('protruding', {
  baseColour: BASE_COLOUR, distance: 3, blur: 6, borderRadius: 8,
})

function onKeyChange(e) {
  cb.setKey(Number(e.target.value))
}

function onTimeSigChange(e) {
  cb.setTimeSignature(Number(e.target.value))
}

function onTempoChange(e) {
  const val = Number(e.target.value)
  if (val >= 60 && val <= 200) cb.setTempo(val)
}

function onBoostChange(e) {
  cb.setRootBoost(Number(e.target.value))
}
</script>

<template>
  <div class="top-bar">
    <div class="controls-left">
      <select class="nue-select" :style="selectProtruding"
              :value="cb.currentKey.value" @change="onKeyChange">
        <option v-for="k in keys" :key="k.id" :value="k.id">{{ k.name }}</option>
      </select>

      <select class="nue-select" :style="selectProtruding"
              :value="cb.timeSignature.value" @change="onTimeSigChange">
        <option v-for="ts in timeSigs" :key="ts.id" :value="ts.id">{{ ts.name }}</option>
      </select>
    </div>

    <div class="controls-right">
      <label class="inline-label">
        BPM
        <input type="number" min="60" max="200"
                class="input"
                :style="inputProtruding"
               :value="cb.tempo.value" @change="onTempoChange" />
      </label>

      <label class="inline-label">
        Bass Boost
        <input type="range" min="0.5" max="3.0" step="0.1"
                class="slider"
               :value="cb.rootBoost.value" @input="onBoostChange"
               :style="sliderStyle" />
        <span class="range-val">{{ cb.rootBoost.value.toFixed(1) }}</span>
      </label>
    </div>
  </div>
</template>

<style scoped>
input[type="range"]::-webkit-slider-thumb {
border-radius: 150px;
background: #c7c7c0;
box-shadow:  -4px 4px 8px #c8c8c8,
             4px -4px 8px #ffffff;
margin-top: 1px;
}
.slider {
  height: 20px;
  display: flex;
  align-items: center;
}
.input {
  border: None;
  outline: None;
  padding: 6px 12px;
}
.top-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 12px;
  background: var(--bg-secondary);
  gap: 12px;
  min-height: 36px;
  flex-shrink: 0;
}

.controls-left,
.controls-right {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.settings-btn {
  padding: 4px 8px;
  font-size: 16px;
}

.inline-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--text-secondary);
  white-space: nowrap;
}

.range-val {
  min-width: 28px;
  font-variant-numeric: tabular-nums;
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
</style>
