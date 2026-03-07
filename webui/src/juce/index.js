/**
 * JUCE Frontend Integration
 *
 * Uses JUCE's own official bridge module (juce-frontend.js) which contains
 * the tested PromiseHandler and getNativeFunction implementation.
 *
 * This file adds thin convenience wrappers for:
 *   - addEventListener / removeEventListener (backend events)
 *   - isRunningInJuce (environment detection)
 *
 * juce-frontend.js imports check_native_interop.js which ensures
 * window.__JUCE__ exists (creates a placeholder in dev mode).
 */

// ─── Import JUCE's official module ──────────────────────────────────────────
// This import also triggers check_native_interop.js (safe to run even if
// JUCE already injected it at document start — guards prevent double creation).
import { getNativeFunction } from './juce-frontend.js';

// Re-export JUCE's getNativeFunction directly — no wrapping needed
export { getNativeFunction };

// ─── Convenience wrappers ───────────────────────────────────────────────────

/**
 * Listen to events emitted from the C++ backend.
 * Returns a removal token that can be passed to removeEventListener.
 */
export function addEventListener(eventId, callback) {
  return window.__JUCE__?.backend?.addEventListener(eventId, callback) ?? null;
}

/**
 * Remove a previously registered event listener.
 */
export function removeEventListener(token) {
  if (token) window.__JUCE__?.backend?.removeEventListener(token);
}

/**
 * Check if we're running inside the JUCE WebBrowserComponent.
 * Uses the __juce__functions list which is only populated by real JUCE
 * (the placeholder from check_native_interop.js has an empty array).
 */
export function isRunningInJuce() {
  const fns = window.__JUCE__?.initialisationData?.__juce__functions;
  return Array.isArray(fns) && fns.length > 0;
}

// ─── Clean up stale state from previous custom bridge implementation ────────
if (window.__JUCE_BRIDGE__) {
  delete window.__JUCE_BRIDGE__;
}
