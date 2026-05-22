import { join, dirname } from 'node:path'
import { fileURLToPath } from 'node:url'

import test from 'ava'

import { GlobalFonts, createCanvas } from '../index'

const __dirname = dirname(fileURLToPath(import.meta.url))

// `default-ignorable-crash.otf` maps U+200B to a zero-length glyph. 
// Shaping a lone default-ignorable codepoint against it used to SIGSEGV Skia's
// ParagraphBuilder and abort the whole process. 
// The fix (src/sk.rs) skips the shaping FFI when a run is entirely default-ignorable or NUL.
const CRASH_FONT = 'CrashRepro'
GlobalFonts.registerFromPath(join(__dirname, 'fonts', 'default-ignorable-crash.otf'), CRASH_FONT)

const STRIPPABLE_CODEPOINTS = [0x200b, 0x200c, 0x200d, 0x2060, 0xfeff, 0x180e, 0x00ad, 0x034f]

for (const cp of STRIPPABLE_CODEPOINTS) {
  const label = `U+${cp.toString(16).toUpperCase().padStart(4, '0')}`
  test(`default-ignorable ${label} shapes without crashing and measures zero`, (t) => {
    const ctx = createCanvas(64, 32).getContext('2d')!
    ctx.font = `12px "${CRASH_FONT}"`
    const s = String.fromCodePoint(cp)
    t.notThrows(() => ctx.fillText(s, 10, 10))
    t.notThrows(() => ctx.strokeText(s, 10, 10))
    t.is(ctx.measureText(s).width, 0)
  })
}

test('NUL run shapes without crashing and measures zero', (t) => {
  const ctx = createCanvas(64, 32).getContext('2d')!
  ctx.font = `12px "${CRASH_FONT}"`
  const nul = String.fromCharCode(0)
  t.notThrows(() => ctx.fillText(nul, 10, 10))
  t.is(ctx.measureText(nul).width, 0)
})

test('embedded default-ignorable does not change the measured width of a visible run', (t) => {
  const ctx = createCanvas(64, 32).getContext('2d')!
  ctx.font = `12px "${CRASH_FONT}"`
  const zwsp = String.fromCharCode(0x200b)
  t.is(ctx.measureText(`A${zwsp}B`).width, ctx.measureText('AB').width)
})
