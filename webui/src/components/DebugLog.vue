<script setup>
import { ref } from 'vue'
import { useDebugLog } from '../composables/useDebugLog'

const { entries, clearDbg } = useDebugLog()
const collapsed = ref(false)
</script>

<template>
  <div class="debug-log" :class="{ collapsed }">
    <div class="debug-header" @click="collapsed = !collapsed">
      <span>🐛 Debug Log ({{ entries.length }})</span>
      <button @click.stop="clearDbg" style="margin-left:8px;font-size:10px;">Clear</button>
      <span style="margin-left:auto;">{{ collapsed ? '▸' : '▾' }}</span>
    </div>
    <div v-if="!collapsed" class="debug-entries">
      <div v-for="(e, i) in entries" :key="i" class="debug-entry">
        <span class="debug-ts">{{ e.ts }}</span>
        <span class="debug-label">{{ e.label }}</span>
        <span class="debug-value">{{ e.display }}</span>
      </div>
      <div v-if="entries.length === 0" class="debug-entry" style="opacity:0.5;">No log entries yet</div>
    </div>
  </div>
</template>

<style scoped>
.debug-log {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  z-index: 9999;
  background: #1e1e1e;
  color: #d4d4d4;
  font-family: 'SF Mono', Menlo, Monaco, 'Courier New', monospace;
  font-size: 11px;
  border-top: 2px solid #007acc;
  max-height: 200px;
  display: flex;
  flex-direction: column;
}
.debug-log.collapsed {
  max-height: 24px;
}
.debug-header {
  display: flex;
  align-items: center;
  padding: 3px 8px;
  background: #007acc;
  color: #fff;
  font-weight: bold;
  cursor: pointer;
  user-select: none;
  font-size: 11px;
  flex-shrink: 0;
}
.debug-entries {
  overflow-y: auto;
  flex: 1;
  padding: 2px 0;
}
.debug-entry {
  display: flex;
  gap: 8px;
  padding: 1px 8px;
  border-bottom: 1px solid #333;
  user-select: text;
  -webkit-user-select: text;
}
.debug-ts { color: #858585; flex-shrink: 0; white-space: nowrap; }
.debug-label { color: #4ec9b0; flex-shrink: 0; font-weight: bold; white-space: nowrap; }
.debug-value { color: #ce9178; word-break: break-all; white-space: pre-wrap; }
</style>
