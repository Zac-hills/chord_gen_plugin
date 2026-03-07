/**
 * Global debug log — renders visually since we can't access the WebView console.
 * Usage:
 *   import { dbg } from '../composables/useDebugLog'
 *   dbg('myLabel', someValue)
 */
import { reactive } from 'vue'

const MAX = 50
const entries = reactive([])

export function dbg(label, value) {
  const ts = new Date().toLocaleTimeString('en-US', { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' })
  let display
  try {
    display = typeof value === 'string' ? value : JSON.stringify(value)
  } catch {
    display = String(value)
  }
  entries.unshift({ ts, label, display })
  if (entries.length > MAX) entries.length = MAX
}

export function clearDbg() {
  entries.splice(0, entries.length)
}

export function useDebugLog() {
  return { entries, dbg, clearDbg }
}
