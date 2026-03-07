/**
 * Neumorphism style composable.
 *
 * Provides computed CSS box-shadow values for sunken (inset) and
 * protruding (raised) neumorphic surfaces.  A reactive global light
 * position controls the shadow direction so every component that uses
 * the composable stays consistent.
 *
 * Usage:
 *   import { useNeumorphism, setLightPosition } from '@/composables/useNeumorphism'
 *
 *   // In a component
 *   const { protruding, sunken } = useNeumorphism()
 *   // <div :style="protruding()">  or  <div :style="sunken({ distance: 8 })">
 *
 *   // Circular buttons
 *   const circleBtn = protruding({ shape: 'circle' })
 *
 *   // Rotated elements — pass the rotation so shadows compensate
 *   const rotated = protruding({ rotation: 45 })
 *
 *   // Border-trick trapezoid (height: 0, shape from borders)
 *   const trapBtn = protruding({
 *     shape: 'trapezoid',
 *     trapezoid: { width: 125, height: 50, slant: 25 },
 *   })
 *
 *   // Clip-path shapes — uses filter: drop-shadow instead
 *   const polyBtn = protruding({ clipped: true })
 *   // NOTE: sunken does NOT work with clipped/trapezoid (CSS limitation)
 *
 *   // Change the global light angle from anywhere
 *   setLightPosition(315)          // degrees, 0 = top, clockwise
 *   setLightPosition('top-left')   // convenience preset
 */
import { ref, computed, readonly } from 'vue'

// ─── Global light state (shared across all consumers) ────────────
const lightAngleDeg = ref(315) // default: top-left

/** Named presets for quick positioning. */
const PRESETS = {
  'top-left':     315,
  'top':          0,
  'top-right':    45,
  'right':        90,
  'bottom-right': 135,
  'bottom':       180,
  'bottom-left':  225,
  'left':         270,
}

/**
 * Set the global light position.
 * @param {number|string} angleOrPreset – degrees (0–360, clockwise from top)
 *                                        or a named preset string.
 */
export function setLightPosition(angleOrPreset) {
  if (typeof angleOrPreset === 'string') {
    const preset = PRESETS[angleOrPreset.toLowerCase()]
    if (preset !== undefined) {
      lightAngleDeg.value = preset
      return
    }
    console.warn(`[useNeumorphism] Unknown preset "${angleOrPreset}". Use one of: ${Object.keys(PRESETS).join(', ')}`)
    return
  }
  lightAngleDeg.value = ((angleOrPreset % 360) + 360) % 360
}

/** Read-only ref so components can display / watch the current angle. */
export const lightAngle = readonly(lightAngleDeg)

// ─── Internal helpers ────────────────────────────────────────────

/** Convert the angle (clockwise from 12 o'clock) to an x/y offset. */
export function angleToOffset(deg, distance) {
  const rad = ((deg - 90) * Math.PI) / 180 // CSS: 0° = right, we want 0° = top
  // We negate because CSS shadow offset is "where the shadow falls",
  // and the light is on the opposite side.
  return {
    x: Math.round(Math.cos(rad) * distance * -1),
    y: Math.round(Math.sin(rad) * distance * -1),
  }
}

/**
 * Shift a hex colour's luminance by a percentage (neumorphism.io algorithm).
 *   colorLuminance('#e0e0e0',  0.15) → '#ffffff'  (lighten 15%)
 *   colorLuminance('#e0e0e0', -0.15) → '#bebebe'  (darken  15%)
 * @param {string} hex – 6-digit hex colour (with or without #).
 * @param {number} lum – fraction to shift, e.g. 0.15 or -0.15.
 * @returns {string} hex colour.
 */
export function colorLuminance(hex, lum) {
  hex = String(hex).replace(/[^0-9a-f]/gi, '')
  if (hex.length < 6) {
    hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2]
  }
  let rgb = '#'
  for (let i = 0; i < 3; i++) {
    let c = parseInt(hex.substr(i * 2, 2), 16)
    c = Math.round(Math.min(Math.max(0, c + c * lum), 255))
    rgb += ('00' + c.toString(16)).substr(c.toString(16).length)
  }
  return rgb
}

// ─── Composable ──────────────────────────────────────────────────

/**
 * @param {object} [defaults] – optional default overrides applied to every call.
 * @param {string} [defaults.baseColour='#e0e0e0'] – surface colour (hex).
 * @param {number} [defaults.distance=6]           – shadow offset in px.
 * @param {number} [defaults.blur=12]              – shadow blur in px.
 * @param {number} [defaults.intensity=0.15]       – colour shift fraction (0–1). 0.15 = 15%.
 * @param {number|string} [defaults.borderRadius=12] – border-radius (px number, or string like '50%').
 * @param {'rect'|'circle'|'trapezoid'} [defaults.shape='rect']
 *        – 'circle' → borderRadius '50%'.
 *        – 'trapezoid' → border-trick shape (height 0, colour via border-bottom).
 *          Uses filter: drop-shadow automatically.
 * @param {object} [defaults.trapezoid]              – dimensions for shape: 'trapezoid'.
 * @param {number} [defaults.trapezoid.width=125]    – base width in px.
 * @param {number} [defaults.trapezoid.height=50]    – visible height (border-bottom width) in px.
 * @param {number} [defaults.trapezoid.slant=25]     – side inset (border-left/right width) in px.
 * @param {number} [defaults.rotation=0]             – element rotation in degrees (compensates shadow direction).
 * @param {boolean} [defaults.clipped=false]          – true for clip-path shapes (polygon, star, etc.).
 *                                                      Uses filter: drop-shadow instead of box-shadow.
 *                                                      Sunken mode returns CSS custom properties for
 *                                                      manual pseudo-element inner shadows.
 */
export function useNeumorphism(defaults = {}) {
  const {
    baseColour: defColour   = '#e0e0e0',
    distance:   defDistance  = 6,
    blur:       defBlur      = 12,
    intensity:  defIntensity = 0.15,
    borderRadius: defRadius  = 12,
    shape:      defShape     = 'rect',
    trapezoid:  defTrapezoid = { width: 125, height: 50, slant: 25 },
    rotation:   defRotation  = 0,
    clipped:    defClipped   = false,
  } = defaults

  /**
   * Build a neumorphic style object.
   * @param {'protruding'|'sunken'} mode
   * @param {object}                [overrides]
   */
  function buildStyle(mode, overrides = {}) {
    const colour    = overrides.baseColour   ?? defColour
    const dist      = overrides.distance     ?? defDistance
    const blur      = overrides.blur         ?? defBlur
    const intensity = overrides.intensity    ?? defIntensity
    const shape     = overrides.shape        ?? defShape
    const rotation  = overrides.rotation     ?? defRotation
    const clipped   = overrides.clipped      ?? defClipped
    const radius    = shape === 'circle' ? '50%' : (overrides.borderRadius ?? defRadius)

    // Merge trapezoid dimensions (per-call overrides → instance defaults → global defaults).
    const trap = { width: 125, height: 50, slant: 25, ...defTrapezoid, ...overrides.trapezoid }

    // Compensate for element rotation so the light direction stays
    // consistent in world-space (the element's box-shadow / filter
    // rotates with the element, so we subtract the rotation).
    const effectiveAngle = lightAngleDeg.value - rotation

    const { x, y } = angleToOffset(effectiveAngle, dist)
    const dark   = colorLuminance(colour, -intensity)
    const light  = colorLuminance(colour, intensity)
    const radiusVal = typeof radius === 'string' ? radius : `${radius}px`

    // ── Border-trick trapezoid ──
    // The shape is made entirely of borders on a zero-height element.
    // background / box-shadow are useless; filter: drop-shadow follows
    // the painted border pixels.
    if (shape === 'trapezoid') {
      const base = {
        height:      '0',
        width:       `${trap.width}px`,
        borderBottom: `${trap.height}px solid ${colour}`,
        borderLeft:  `${trap.slant}px solid transparent`,
        borderRight: `${trap.slant}px solid transparent`,
      }
      if (mode === 'sunken') {
        // No inset drop-shadow in CSS — expose custom properties for
        // a pseudo-element inner-shadow approach.
        return {
          ...base,
          '--nm-inset-x':     `${x}px`,
          '--nm-inset-y':     `${y}px`,
          '--nm-inset-blur':  `${blur}px`,
          '--nm-inset-dark':  dark,
          '--nm-inset-light': light,
        }
      }
      return {
        ...base,
        filter: `drop-shadow(${x}px ${y}px ${blur}px ${dark}) drop-shadow(${-x}px ${-y}px ${blur}px ${light})`,
      }
    }

    // ── Clipped path mode (polygon, star, etc.) ──
    // box-shadow is painted *behind* the element's box and gets clipped
    // by clip-path. filter: drop-shadow follows the painted outline.
    if (clipped) {
      if (mode === 'sunken') {
        // CSS has no inset drop-shadow. We return custom properties that
        // can be used by a pseudo-element overlay to fake an inner shadow.
        return {
          background: colour,
          borderRadius: radiusVal,
          '--nm-inset-x':     `${x}px`,
          '--nm-inset-y':     `${y}px`,
          '--nm-inset-blur':  `${blur}px`,
          '--nm-inset-dark':  dark,
          '--nm-inset-light': light,
        }
      }
      // Protruding: two drop-shadows (dark + light).
      return {
        background: colour,
        borderRadius: radiusVal,
        filter: `drop-shadow(${x}px ${y}px ${blur}px ${dark}) drop-shadow(${-x}px ${-y}px ${blur}px ${light})`,
      }
    }

    // ── Standard box-shadow mode ──
    const inset = mode === 'sunken' ? 'inset ' : ''
    return {
      background: colour,
      borderRadius: radiusVal,
      boxShadow: `${inset}${x}px ${y}px ${blur}px ${dark}, ${inset}${-x}px ${-y}px ${blur}px ${light}`,
    }
  }

  /** Returns a reactive style object for a **raised / protruding** surface. */
  function protruding(overrides = {}) {
    return computed(() => buildStyle('protruding', overrides))
  }

  /** Returns a reactive style object for a **sunken / inset** surface. */
  function sunken(overrides = {}) {
    return computed(() => buildStyle('sunken', overrides))
  }

  /**
   * Non-reactive one-shot style (useful in dynamic render functions).
   * Use `protruding()` / `sunken()` when you need reactivity to light changes.
   */
  function styleOnce(mode = 'protruding', overrides = {}) {
    return buildStyle(mode, overrides)
  }

  return {
    protruding,
    sunken,
    styleOnce,
    /** Expose the global angle so templates can react to it. */
    lightAngle: readonly(lightAngleDeg),
    /** Convenience re-export so you don't need a separate import. */
    setLightPosition,
  }
}

// ─── Feature Examples ────────────────────────────────────────────
//
// All examples assume:
//
//   import { useNeumorphism, setLightPosition } from '@/composables/useNeumorphism'
//   const { protruding, sunken, styleOnce, lightAngle } = useNeumorphism()
//
// ── 1. Basic protruding (raised) surface ─────────────────────────
//
//   <template>
//     <div :style="raised.value">Raised card</div>
//   </template>
//
//   <script setup>
//   const raised = protruding()
//   </script>
//
// ── 2. Basic sunken (inset) surface ──────────────────────────────
//
//   <template>
//     <div :style="inset.value">Sunken well</div>
//   </template>
//
//   <script setup>
//   const inset = sunken()
//   </script>
//
// ── 3. Overriding defaults per-call ──────────────────────────────
//
//   const deep = sunken({ distance: 10, blur: 20, intensity: 0.25 })
//   const subtle = protruding({ distance: 3, blur: 6, intensity: 0.10 })
//
// ── 4. Custom base colour ────────────────────────────────────────
//
//   const blueCard = protruding({ baseColour: '#2a2a4a' })
//
// ── 5. Instance-level defaults ───────────────────────────────────
//
//   // Every call from this instance inherits these defaults.
//   const { protruding, sunken } = useNeumorphism({
//     baseColour: '#2e1a2e',
//     distance: 8,
//     blur: 16,
//   })
//   const card = protruding()           // uses #2e1a2e, dist 8, blur 16
//   const well = sunken({ distance: 4 }) // uses #2e1a2e, dist 4, blur 16
//
// ── 6. Circular buttons ──────────────────────────────────────────
//
//   <template>
//     <button :style="circleBtn.value" class="w-12 h-12">⏵</button>
//   </template>
//
//   <script setup>
//   const circleBtn = protruding({ shape: 'circle' })
//   </script>
//
//   <!-- Equivalent to: protruding({ borderRadius: '50%' }) -->
//
// ── 7. Custom border-radius (pill shape, etc.) ───────────────────
//
//   const pill = protruding({ borderRadius: '999px' })
//   const rounded = protruding({ borderRadius: 20 })      // 20px
//
// ── 8. Rotated elements ──────────────────────────────────────────
//
//   <!-- Element is rotated 45° via CSS transform -->
//   <template>
//     <div :style="rotStyle.value" style="transform: rotate(45deg)">
//       Diamond
//     </div>
//   </template>
//
//   <script setup>
//   // Pass the same rotation so the shadow direction compensates
//   const rotStyle = protruding({ rotation: 45 })
//   </script>
//
// ── 9. Border-trick trapezoid ────────────────────────────────────
//
//   <!-- The composable generates ALL the border CSS for you -->
//   <template>
//     <div :style="trapStyle.value" />
//   </template>
//
//   <script setup>
//   const trapStyle = protruding({
//     shape: 'trapezoid',
//     trapezoid: { width: 125, height: 50, slant: 25 },
//   })
//   </script>
//
//   <!-- Rendered CSS:
//     height: 0;
//     width: 125px;
//     border-bottom: 50px solid #e0e0e0;
//     border-left: 25px solid transparent;
//     border-right: 25px solid transparent;
//     filter: drop-shadow(...) drop-shadow(...);
//   -->
//
//   // Trapezoid with instance defaults (reuse dimensions):
//   const { protruding } = useNeumorphism({
//     shape: 'trapezoid',
//     trapezoid: { width: 100, height: 40, slant: 20 },
//   })
//   const btn1 = protruding()
//   const btn2 = protruding({ trapezoid: { width: 200 } }) // wider, same height/slant
//
// ── 10. Clip-path shapes (polygon, star, etc.) ───────────────────
//
//   <template>
//     <div :style="polyStyle.value" style="clip-path: polygon(50% 0%, 100% 100%, 0% 100%)">
//       Triangle
//     </div>
//   </template>
//
//   <script setup>
//   const polyStyle = protruding({ clipped: true })
//   </script>
//
// ── 11. Combining rotation + clipped ─────────────────────────────
//
//   const rotatedPoly = protruding({ clipped: true, rotation: 30 })
//
// ── 12. Combining rotation + trapezoid ───────────────────────────
//
//   const rotatedTrap = protruding({
//     shape: 'trapezoid',
//     trapezoid: { width: 100, height: 40, slant: 15 },
//     rotation: 30,
//   })
//
// ── 13. Sunken + clipped/trapezoid (CSS limitation) ──────────────
//
//   // CSS has no inset drop-shadow. The composable returns custom
//   // properties you can use with a ::after pseudo-element:
//
//   const sunkenTrap = sunken({ shape: 'trapezoid' })
//
//   // The returned style includes:
//   //   --nm-inset-x, --nm-inset-y, --nm-inset-blur,
//   //   --nm-inset-dark, --nm-inset-light
//   //
//   // Use them in CSS for a pseudo-element inner shadow:
//   //   .trapezoid-sunken::after {
//   //     content: '';
//   //     position: absolute;
//   //     inset: 0;
//   //     box-shadow:
//   //       inset var(--nm-inset-x) var(--nm-inset-y) var(--nm-inset-blur) var(--nm-inset-dark),
//   //       inset calc(var(--nm-inset-x) * -1) calc(var(--nm-inset-y) * -1) var(--nm-inset-blur) var(--nm-inset-light);
//   //   }
//
// ── 14. Non-reactive one-shot style ──────────────────────────────
//
//   // Useful in render functions or when you don't need reactivity.
//   const style = styleOnce('protruding', { distance: 8 })
//   // Returns a plain object (not a computed ref).
//
// ── 15. Global light position ────────────────────────────────────
//
//   // Set by degrees (0 = top, clockwise)
//   setLightPosition(315)    // top-left (default)
//   setLightPosition(0)      // top
//   setLightPosition(90)     // right
//   setLightPosition(180)    // bottom
//
//   // Set by named preset
//   setLightPosition('top-left')
//   setLightPosition('top-right')
//   setLightPosition('bottom')
//   setLightPosition('left')
//
//   // Available presets:
//   //   'top-left' (315), 'top' (0), 'top-right' (45),
//   //   'right' (90), 'bottom-right' (135), 'bottom' (180),
//   //   'bottom-left' (225), 'left' (270)
//
// ── 16. Reading the current light angle ──────────────────────────
//
//   <template>
//     <span>Light: {{ lightAngle }}°</span>
//   </template>
//
//   // lightAngle is a readonly ref — reactive but not writable.
//   // Use setLightPosition() to change it.
