<script setup>
import { createApp } from 'vue'
import TopBar from './components/TopBar.vue'
import ChordBuilder from './components/ChordBuilder.vue'
import EmotionWheel from './components/EmotionWheel.vue'
import ProgressionBar from './components/ProgressionBar.vue'
import Logo from './components/Logo.vue'
import DebugLog from './components/DebugLog.vue'
import { useChordBuilder } from './composables/useChordBuilder.js'
import { provide } from 'vue'

const chordBuilder = useChordBuilder()
provide('chordBuilder', chordBuilder)
</script>

<template>
  <div class="app">
    <TopBar />
    <div class="main-row">
      <Logo class="logo-overlay" />
      <ChordBuilder />
      <EmotionWheel />
    </div>
    <ProgressionBar />
    <div v-if="!chordBuilder.connectedToBackend.value" class="dev-banner">
      ⚠ Running in dev mode — JUCE backend not connected
    </div>
  </div>
</template>

<style scoped>
.app {
  display: flex;
  flex-direction: column;
  height: 100vh;
  background: var(--bg-main);
  color: var(--text-primary);
  overflow: hidden;
  border: 3px solid var(--border);
}
.main-row {
  position: relative;
  display: flex;
  align-items: flex-start;
  justify-content: space-around;
  flex: 1;
  min-height: 0;
  padding: 10px 16px;
}

.logo-overlay {
  position: absolute;
  top: 0;
  left: 50%;
  transform: translateX(-50%);
  z-index: 10;
  pointer-events: none;
}

.dev-banner {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  background: var(--warning);
  color: var(--bg-main);
  text-align: center;
  padding: 4px 8px;
  font-size: 12px;
  font-weight: 600;
  z-index: 100;
}
</style>
