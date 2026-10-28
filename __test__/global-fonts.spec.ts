import { readFileSync } from 'node:fs'
import { join, dirname } from 'node:path'
import { fileURLToPath } from 'node:url'

import test from 'ava'

import { GlobalFonts, FontKey, createCanvas } from '../index'

const __dirname = dirname(fileURLToPath(import.meta.url))
const fontPath = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
const fontData = readFileSync(fontPath)
const defaultCount = GlobalFonts.families.length

const renderText = (fontString: string, text = 'Hello registry') => {
  const canvas = createCanvas(320, 80)
  const ctx = canvas.getContext('2d')!
  ctx.fillStyle = 'white'
  ctx.fillRect(0, 0, 320, 80)
  ctx.fillStyle = 'black'
  ctx.font = fontString
  ctx.textBaseline = 'alphabetic'
  ctx.fillText(text, 8, 52)
  return Buffer.from(ctx.getImageData(0, 0, 320, 80).data)
}

const MEMORY_BOUND = 96 * 1024 * 1024

test.serial('should be able to get global font family names', (t) => {
  t.notThrows(() => GlobalFonts.families)
})

test.serial('should be able to register font and test font existence', (t) => {
  t.is(GlobalFonts.has('114514'), false)

  if (!GlobalFonts.has('Source Serif Pro')) {
    const fontKey = GlobalFonts.register(fontData)
    t.true(fontKey instanceof FontKey)
    t.is(GlobalFonts.families.length, defaultCount + 1)
    // Verify remove returns true on first removal
    t.true(GlobalFonts.remove(fontKey!))
  } else {
    t.is(GlobalFonts.families.length, defaultCount)
  }
})

test.serial('multiple identical fonts should only exist within one font family', (t) => {
  const first = GlobalFonts.register(fontData)
  const count = GlobalFonts.families.length
  const styles = GlobalFonts.families.find(({ family }) => family === 'Source Serif Pro')!.styles.length
  const second = GlobalFonts.register(fontData)
  const third = GlobalFonts.register(fontData)
  t.is(GlobalFonts.families.length, count, 'identical bytes add no family')
  t.is(
    GlobalFonts.families.find(({ family }) => family === 'Source Serif Pro')!.styles.length,
    styles,
    'identical bytes share one face, so the family lists no second style',
  )
  GlobalFonts.remove(first!)
  GlobalFonts.remove(second!)
  GlobalFonts.remove(third!)
})

test.serial('return false if font path not existed', (t) => {
  t.is(GlobalFonts.register(Buffer.from('whatever')), null)
})

test.serial('should be able to register font with name alias', (t) => {
  const fontAliasName = 'Cascadia-skr-canvas-test'
  const fontKey = GlobalFonts.registerFromPath(join(__dirname, 'fonts', 'Cascadia.woff2'), fontAliasName)
  t.true(fontKey instanceof FontKey, 'registerFromPath should return a FontKey')
  const styleSet = GlobalFonts.families.find(({ family }) => family === fontAliasName)
  t.deepEqual(styleSet, {
    family: 'Cascadia-skr-canvas-test',
    styles: [{ weight: 400, width: 'normal', style: 'normal' }],
  })
})

test.serial('should be able to register fonts from dir', (t) => {
  t.is(GlobalFonts.loadFontsFromDir(join(__dirname, 'fonts-dir')), 3)
})

test.serial('registerFromPath should return FontKey that can be used for removal', (t) => {
  const fontAliasName = 'RegisterFromPath-RemovalTest'
  const fontKey = GlobalFonts.registerFromPath(join(__dirname, 'fonts', 'Lato-Regular.ttf'), fontAliasName)

  // Verify FontKey is returned
  t.true(fontKey instanceof FontKey, 'registerFromPath should return a FontKey')
  t.is(typeof fontKey!.typefaceId, 'number', 'FontKey should have a numeric typefaceId')

  // Verify font exists
  t.true(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should exist after registration',
  )

  // Remove using the returned FontKey
  const removeResult = GlobalFonts.remove(fontKey!)
  t.true(removeResult, 'remove() should succeed with FontKey from registerFromPath')

  // Verify font is gone
  t.false(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should be removed',
  )
})

test.serial('registerFromPath should return null for non-existent path', (t) => {
  const result = GlobalFonts.registerFromPath('/non/existent/path/font.ttf')
  t.is(result, null, 'registerFromPath should return null for non-existent file')
})

test.serial('remove() should mark font as removed and return correct status', (t) => {
  const fontAliasName = 'RemovalTest-Font'
  const anotherFontPath = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const anotherFontData = readFileSync(anotherFontPath)

  // Register font with unique alias
  const fontKey = GlobalFonts.register(anotherFontData, fontAliasName)
  t.true(fontKey instanceof FontKey)

  // Verify font exists in families
  const familiesBeforeRemoval = GlobalFonts.families
  const hasFont = familiesBeforeRemoval.some(({ family }) => family === fontAliasName)
  t.true(hasFont, 'Font should exist after registration')

  // Remove font and verify it returns true on first call
  const removeResult = GlobalFonts.remove(fontKey!)
  t.true(removeResult, 'remove() should return true on first removal')

  // Verify remove returns false when trying to remove again
  const secondRemoveResult = GlobalFonts.remove(fontKey!)
  t.false(secondRemoveResult, 'remove() should return false when font already removed')
})

test.serial('remove() should actually remove font from families list', (t) => {
  const fontAliasName = 'TrueRemovalTest-Font'
  const fontPath2 = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontData2 = readFileSync(fontPath2)

  // Register font with unique alias
  const fontKey = GlobalFonts.register(fontData2, fontAliasName)
  t.true(fontKey instanceof FontKey)

  // Verify font exists after registration
  t.true(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should exist after registration',
  )

  // Remove font
  const removeResult = GlobalFonts.remove(fontKey!)
  t.true(removeResult, 'remove() should return true')

  // Verify font is no longer in families list
  t.false(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should NOT exist after removal',
  )
})

test.serial('register override should place font under the requested style slot', (t) => {
  // Lato-Regular.ttf is upright weight-400. Override it as italic + bold and
  // verify the style set reports what we asked for, not what the file says.
  const fontAliasName = 'LatoOverrideTest-Italic'
  const latoData = readFileSync(join(__dirname, 'fonts', 'Lato-Regular.ttf'))
  const fontKey = GlobalFonts.register(latoData, fontAliasName, {
    style: 'italic',
    weight: 700,
  })
  t.true(fontKey instanceof FontKey)

  const styleSet = GlobalFonts.families.find(({ family }) => family === fontAliasName)
  t.truthy(styleSet, 'alias family should be present')
  t.true(
    styleSet!.styles.some((s) => s.style === 'italic' && s.weight === 700),
    'override should surface as an italic/700 style entry',
  )

  GlobalFonts.remove(fontKey!)
})

test.serial('register with invalid override values rejects', (t) => {
  const latoData = readFileSync(join(__dirname, 'fonts', 'Lato-Regular.ttf'))
  t.throws(() => GlobalFonts.register(latoData, 'InvalidOverrideWeight', { weight: 2000 }))
  t.throws(() => GlobalFonts.register(latoData, 'InvalidOverrideStyle', { style: 'slanted' as any }))
  t.throws(() => GlobalFonts.register(latoData, 'InvalidOverrideStretch', { stretch: 'extra-wide' }))
})

test.serial('the same bytes under two overrides are two registrations with two faces', (t) => {
  const fontAliasName = 'OverrideIdentityTest'
  const latoData = readFileSync(join(__dirname, 'fonts', 'Lato-Regular.ttf'))
  const first = GlobalFonts.register(latoData, fontAliasName, { style: 'italic' })
  const second = GlobalFonts.register(latoData, fontAliasName, { style: 'oblique', weight: 900 })
  t.true(first instanceof FontKey)
  t.true(second instanceof FontKey)
  t.not(first!.typefaceId, second!.typefaceId, 'each register call gets a key of its own')

  const styles = () => GlobalFonts.families.find(({ family }) => family === fontAliasName)?.styles ?? []
  t.true(
    styles().some((s) => s.style === 'italic' && s.weight === 400),
    "the first call's override is a face",
  )
  t.true(
    styles().some((s) => s.style === 'oblique' && s.weight === 900),
    "the second call's override is a face",
  )

  GlobalFonts.remove(first!)
  t.deepEqual(
    styles(),
    [{ weight: 900, width: 'normal', style: 'oblique' }],
    'removing one registration leaves the other face',
  )
  GlobalFonts.remove(second!)
  t.false(GlobalFonts.has(fontAliasName), 'the family goes with its last registration')
})

test.serial('ctx.font italic keyword picks the italic-classified variant when two variants share an alias', (t) => {
  // Integration regression for user report #2: register two distinct font
  // files under one family alias, the second overridden to italic. Rendering
  // with `italic ...` in ctx.font must produce different pixels than rendering
  // with `normal ...`. The prior regex parser silently dropped the italic
  // keyword whenever `normal` appeared after it (e.g. `italic normal 48px F`
  // was parsed the same as `normal normal 48px F`), which collapsed both
  // renders to the same variant regardless of how many variants were
  // registered.
  const alias = 'StyleKeywordRegression'
  const regBytes = readFileSync(join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf'))
  const italBytes = readFileSync(join(__dirname, 'fonts', 'Lato-Regular.ttf'))
  const k1 = GlobalFonts.register(regBytes, alias, { style: 'normal', weight: 400 })
  const k2 = GlobalFonts.register(italBytes, alias, { style: 'italic', weight: 400 })

  const render = (fontStr: string) => {
    const c = createCanvas(240, 80)
    const ctx = c.getContext('2d')!
    ctx.fillStyle = 'white'
    ctx.fillRect(0, 0, 240, 80)
    ctx.fillStyle = 'black'
    ctx.font = fontStr
    ctx.textBaseline = 'alphabetic'
    ctx.fillText('Helo', 8, 56)
    return ctx.getImageData(0, 0, 240, 80).data
  }
  const pixelDiff = (x: Uint8ClampedArray, y: Uint8ClampedArray) => {
    let d = 0
    for (let i = 0; i < x.length; i++) if (x[i] !== y[i]) d++
    return d
  }

  // Exact reproduction from user report #2 — three tokens, with a trailing
  // `normal`. Pre-fix this collapsed to 0 byte difference.
  t.true(
    pixelDiff(render(`italic normal 48px ${alias}`), render(`normal normal 48px ${alias}`)) > 0,
    'italic keyword followed by `normal` must still pick the italic variant',
  )
  // Two-token style/size shorthand.
  t.true(
    pixelDiff(render(`italic 48px ${alias}`), render(`normal 48px ${alias}`)) > 0,
    'two-token italic/normal shorthand must pick distinct variants',
  )
  // Style keyword at the end.
  t.true(
    pixelDiff(render(`normal italic 48px ${alias}`), render(`normal normal 48px ${alias}`)) > 0,
    'italic keyword at the end must still pick the italic variant',
  )

  GlobalFonts.remove(k1!)
  GlobalFonts.remove(k2!)
})

test.serial('re-registering font after removal should make it visible again', (t) => {
  const fontAliasName = 'ReRegisterTest-Font'
  const fontPath2 = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontData2 = readFileSync(fontPath2)

  // Register font
  const fontKey1 = GlobalFonts.register(fontData2, fontAliasName)
  t.true(fontKey1 instanceof FontKey)
  t.true(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should exist after first registration',
  )

  // Remove font
  t.true(GlobalFonts.remove(fontKey1!))
  t.false(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should NOT exist after removal',
  )

  // Re-register font with same alias
  const fontKey2 = GlobalFonts.register(fontData2, fontAliasName)
  t.true(fontKey2 instanceof FontKey)
  t.true(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should exist after re-registration',
  )

  // Clean up
  GlobalFonts.remove(fontKey2!)
})

test.serial('font registered with alias should be accessible under both original name and alias', (t) => {
  const fontAliasName = 'DualAccess-Alias'
  const originalFamilyName = 'Source Serif Pro'
  const fontPath2 = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontData2 = readFileSync(fontPath2)

  const originalStylesBefore = GlobalFonts.families.find((f) => f.family === originalFamilyName)?.styles.length ?? 0

  const fontKey = GlobalFonts.register(fontData2, fontAliasName)
  t.true(fontKey instanceof FontKey)

  t.true(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Font should exist under alias',
  )
  t.true(
    GlobalFonts.families.some(({ family }) => family === originalFamilyName),
    'Font should exist under original family name',
  )
  const originalStylesAfterReg = GlobalFonts.families.find((f) => f.family === originalFamilyName)?.styles.length ?? 0
  t.is(
    originalStylesAfterReg,
    Math.max(originalStylesBefore, 1),
    'the original family lists the face once however many registrations share it',
  )

  GlobalFonts.remove(fontKey!)

  t.false(
    GlobalFonts.families.some(({ family }) => family === fontAliasName),
    'Alias should be removed',
  )
  const originalStylesAfterRemove =
    GlobalFonts.families.find((f) => f.family === originalFamilyName)?.styles.length ?? 0
  t.is(
    originalStylesAfterRemove,
    originalStylesBefore,
    'Original family style count should return to pre-registration level',
  )
})

test.serial("a font registered again after another font's removal resolves under one family entry", (t) => {
  const fontAAlias = 'ReRegisterAfterRemoval-FontA'
  const fontBAlias = 'ReRegisterAfterRemoval-FontB'
  const fontAData = readFileSync(join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf'))
  const fontBData = readFileSync(join(__dirname, 'fonts', 'Cascadia.woff2'))

  const fontKeyA = GlobalFonts.register(fontAData, fontAAlias)
  t.true(fontKeyA instanceof FontKey, 'Font A should register successfully')
  const fontKeyB1 = GlobalFonts.register(fontBData, fontBAlias)
  t.true(fontKeyB1 instanceof FontKey, 'Font B should register successfully')

  t.true(GlobalFonts.remove(fontKeyA!), 'Font A removal should succeed')

  const fontKeyB2 = GlobalFonts.register(fontBData, fontBAlias)
  t.true(fontKeyB2 instanceof FontKey, 'Font B re-registration should succeed')
  t.not(fontKeyB2!.typefaceId, fontKeyB1!.typefaceId, 'the second registration is a key of its own')
  t.is(
    GlobalFonts.families.filter(({ family }) => family === fontBAlias).length,
    1,
    'Font B should appear exactly once in families list',
  )
  t.is(
    GlobalFonts.families.find(({ family }) => family === fontBAlias)!.styles.length,
    1,
    'both registrations share one face',
  )

  t.true(GlobalFonts.remove(fontKeyB1!))
  t.true(GlobalFonts.has(fontBAlias), 'the family stays while a registration of it remains')
  t.true(GlobalFonts.remove(fontKeyB2!))
  t.false(GlobalFonts.has(fontBAlias), 'the family goes with its last registration')
})

test.serial('removeBatch() removes several registrations in one call', (t) => {
  const fontData1 = readFileSync(join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf'))
  const fontData2 = readFileSync(join(__dirname, 'fonts', 'Cascadia.woff2'))
  const fontData3 = readFileSync(join(__dirname, 'fonts', 'Lato-Regular.ttf'))

  // Register multiple fonts with unique aliases
  const fontKey1 = GlobalFonts.register(fontData1, 'BatchTest-Font1')
  const fontKey2 = GlobalFonts.register(fontData2, 'BatchTest-Font2')
  const fontKey3 = GlobalFonts.register(fontData3, 'BatchTest-Font3')

  t.true(fontKey1 instanceof FontKey)
  t.true(fontKey2 instanceof FontKey)
  t.true(fontKey3 instanceof FontKey)

  t.not(fontKey1!.typefaceId, fontKey2!.typefaceId, 'Font1 and Font2 should have different IDs')
  t.not(fontKey2!.typefaceId, fontKey3!.typefaceId, 'Font2 and Font3 should have different IDs')
  t.not(fontKey1!.typefaceId, fontKey3!.typefaceId, 'Font1 and Font3 should have different IDs')

  // Verify all exist
  t.true(GlobalFonts.families.some((f) => f.family === 'BatchTest-Font1'))
  t.true(GlobalFonts.families.some((f) => f.family === 'BatchTest-Font2'))
  t.true(GlobalFonts.families.some((f) => f.family === 'BatchTest-Font3'))

  // Remove all in batch
  const removedCount = GlobalFonts.removeBatch([fontKey1!, fontKey2!, fontKey3!])
  t.is(removedCount, 3, 'Should report 3 fonts removed')

  // Verify all are gone
  t.false(GlobalFonts.families.some((f) => f.family === 'BatchTest-Font1'))
  t.false(GlobalFonts.families.some((f) => f.family === 'BatchTest-Font2'))
  t.false(GlobalFonts.families.some((f) => f.family === 'BatchTest-Font3'))
})

test.serial('removeBatch() should return 0 when no fonts are removed', (t) => {
  const fontPath2 = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontData2 = readFileSync(fontPath2)

  // Register and then remove a font to get a stale key
  const fontKey = GlobalFonts.register(fontData2, 'StaleKeyTest-Font')
  t.true(fontKey instanceof FontKey)
  t.true(GlobalFonts.remove(fontKey!))

  // Try to remove with stale key - should return 0
  const removedCount = GlobalFonts.removeBatch([fontKey!])
  t.is(removedCount, 0, 'Should report 0 fonts removed for stale keys')
})

test.serial('removeBatch() with empty array should return 0', (t) => {
  const removedCount = GlobalFonts.removeBatch([])
  t.is(removedCount, 0, 'Should report 0 fonts removed for empty array')
})

test.serial('removeAll() should remove all registered fonts', (t) => {
  const fontPath2 = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontData2 = readFileSync(fontPath2)

  // Register some fonts
  const fontKey1 = GlobalFonts.register(fontData2, 'RemoveAllTest-Font1')
  const fontKey2 = GlobalFonts.register(fontData2, 'RemoveAllTest-Font2')

  t.true(fontKey1 instanceof FontKey)
  t.true(fontKey2 instanceof FontKey)

  // Get count of registered fonts (excluding system fonts - check for our test fonts)
  t.true(GlobalFonts.families.some((f) => f.family === 'RemoveAllTest-Font1'))
  t.true(GlobalFonts.families.some((f) => f.family === 'RemoveAllTest-Font2'))

  // Remove all
  const removedCount = GlobalFonts.removeAll()
  t.true(removedCount >= 2, 'Should remove at least the 2 fonts we registered')

  // Verify our test fonts are gone
  t.false(GlobalFonts.families.some((f) => f.family === 'RemoveAllTest-Font1'))
  t.false(GlobalFonts.families.some((f) => f.family === 'RemoveAllTest-Font2'))
})

test.serial('setAlias() should return false for non-existent font', (t) => {
  const result = GlobalFonts.setAlias('NonExistentFont12345', 'MyAlias')
  t.false(result, 'setAlias should return false for non-existent font')
})

test.serial('setAlias() should return true for existing font', (t) => {
  // Register a font first
  const fontPath2 = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontData2 = readFileSync(fontPath2)
  const fontKey = GlobalFonts.register(fontData2, 'SetAliasTest-Font')
  t.true(fontKey instanceof FontKey)

  // Set alias should succeed
  const result = GlobalFonts.setAlias('SetAliasTest-Font', 'SetAliasTest-Alias')
  t.true(result, 'setAlias should return true for existing font')

  // Clean up
  GlobalFonts.remove(fontKey!)
})

test.serial('stale aliases should not transfer to newly registered fonts with same family name', (t) => {
  const fontAPath = join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf')
  const fontBPath = join(__dirname, 'fonts', 'Cascadia.woff2')
  const fontAData = readFileSync(fontAPath)
  const fontBData = readFileSync(fontBPath)

  // Step 1: Register Font A with alias "StaleAliasTest"
  const fontKeyA = GlobalFonts.register(fontAData, 'StaleAliasTest')
  t.true(fontKeyA instanceof FontKey)

  // Step 2: Set an alias pointing to "StaleAliasTest"
  const aliasSet = GlobalFonts.setAlias('StaleAliasTest', 'StaleAliasX')
  t.true(aliasSet, 'setAlias should succeed')

  // Verify alias exists
  t.true(
    GlobalFonts.families.some((f) => f.family === 'StaleAliasX'),
    'Alias should exist',
  )

  // Step 3: Remove Font A
  t.true(GlobalFonts.remove(fontKeyA!), 'Font A removal should succeed')

  // Verify alias is gone (font was removed)
  t.false(
    GlobalFonts.families.some((f) => f.family === 'StaleAliasX'),
    'Alias should be gone after font removal',
  )

  // Step 4: Register Font B with the SAME alias name "StaleAliasTest"
  const fontKeyB = GlobalFonts.register(fontBData, 'StaleAliasTest')
  t.true(fontKeyB instanceof FontKey)

  // Step 5: Verify "StaleAliasX" does NOT exist (the stale alias should NOT transfer)
  t.false(
    GlobalFonts.families.some((f) => f.family === 'StaleAliasX'),
    'Stale alias should NOT transfer to newly registered font',
  )

  const fontKeyC = GlobalFonts.register(fontAData, 'UnrelatedFont')
  GlobalFonts.remove(fontKeyC!)

  t.false(
    GlobalFonts.families.some((f) => f.family === 'StaleAliasX'),
    'Stale alias should NOT appear after another removal',
  )

  // Clean up
  GlobalFonts.remove(fontKeyB!)
})

test.serial('identical bytes registered under two names: removing one key leaves the other drawing', (t) => {
  // The failure this guards against: two documents register the same font under names of their own, the first closes, and the second's text falls to the default font.
  const oswaldData = readFileSync(join(__dirname, 'fonts', 'Oswald.ttf'))
  const keyA = GlobalFonts.register(oswaldData, 'SharedBytes-A')
  const keyB = GlobalFonts.register(oswaldData, 'SharedBytes-B')
  t.true(keyA instanceof FontKey)
  t.true(keyB instanceof FontKey)
  t.not(keyA!.typefaceId, keyB!.typefaceId, 'each register call gets a key of its own')

  const withB = renderText('40px SharedBytes-B')
  t.true(renderText('40px SharedBytes-A').equals(withB), 'both names draw the one shared typeface')

  t.true(GlobalFonts.remove(keyA!))
  t.false(GlobalFonts.has('SharedBytes-A'), 'the removed name is gone')
  t.true(GlobalFonts.has('SharedBytes-B'), 'the other registration of the same bytes stays')
  t.true(
    renderText('40px SharedBytes-B').equals(withB),
    "the other name still draws the font after the first key's removal",
  )

  t.true(GlobalFonts.remove(keyB!))
  t.false(GlobalFonts.has('SharedBytes-B'), 'the family goes with its last registration')
  t.false(renderText('40px SharedBytes-B').equals(withB), 'with both keys gone the name no longer draws the font')
})

test.serial('drawing a removed name at the size drawn before falls back without crashing', (t) => {
  // Earlier releases kept the removed typeface reachable through a cache after freeing it, so a draw at the size drawn before could reach freed memory.
  const oswaldData = readFileSync(join(__dirname, 'fonts', 'Oswald.ttf'))
  const key = GlobalFonts.register(oswaldData, 'RemovedName-Test')
  t.true(key instanceof FontKey)
  const before = renderText('40px RemovedName-Test')

  t.true(GlobalFonts.remove(key!))
  const after = renderText('40px RemovedName-Test')
  t.false(after.equals(before), 'the removed name no longer draws the font at the size drawn before')
  t.true(after.equals(renderText('40px NoSuchFamily-RemovedName')), 'the removed name draws what an unknown name draws')
  t.false(GlobalFonts.has('RemovedName-Test'))
})

test.serial("two faces under one name: removing one leaves the other's match", (t) => {
  const alias = 'TwoFaces-Test'
  const serifData = readFileSync(join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf'))
  const latoData = readFileSync(join(__dirname, 'fonts', 'Lato-Regular.ttf'))
  const upright = GlobalFonts.register(serifData, alias, { style: 'normal', weight: 400 })
  const italic = GlobalFonts.register(latoData, alias, { style: 'italic', weight: 400 })
  t.true(upright instanceof FontKey)
  t.true(italic instanceof FontKey)
  const styles = () => GlobalFonts.families.find(({ family }) => family === alias)?.styles ?? []
  t.is(styles().length, 2, 'the family lists both faces')

  const uprightBefore = renderText(`normal 40px ${alias}`)
  const italicBefore = renderText(`italic 40px ${alias}`)
  t.false(uprightBefore.equals(italicBefore), 'the italic request picks the italic face')

  t.true(GlobalFonts.remove(italic!))
  t.deepEqual(styles(), [{ weight: 400, width: 'normal', style: 'normal' }], 'the upright face remains')
  t.true(renderText(`normal 40px ${alias}`).equals(uprightBefore), 'the upright request still draws the upright face')
  t.false(
    renderText(`italic 40px ${alias}`).equals(italicBefore),
    'the italic request no longer draws the removed face',
  )

  t.true(GlobalFonts.remove(upright!))
  t.false(GlobalFonts.has(alias))
})

test.serial('ten thousand register, draw and remove cycles keep memory bounded', (t) => {
  // The canvas is disposed each cycle, since a canvas's deferred recording holds every typeface drawn on it until then.
  const serifData = readFileSync(join(__dirname, 'fonts', 'SourceSerifPro-Regular.ttf'))
  let removed = 0
  let afterWarmUp = 0
  for (let i = 0; i < 10000; i++) {
    const key = GlobalFonts.register(serifData, `MemoryCycle-${i}`)
    const canvas = createCanvas(200, 60)
    const ctx = canvas.getContext('2d')!
    ctx.font = `30px MemoryCycle-${i}`
    ctx.fillText('cycle', 4, 40)
    canvas.dispose()
    if (GlobalFonts.remove(key!)) removed++
    if (i === 1999) afterWarmUp = process.memoryUsage().rss
  }
  t.is(removed, 10000, 'every cycle removed its registration')
  t.false(GlobalFonts.has('MemoryCycle-9999'))
  const growth = process.memoryUsage().rss - afterWarmUp
  t.true(
    growth < MEMORY_BOUND,
    `memory grew ${(growth / 1048576).toFixed(0)} MB over the eight thousand cycles after warm-up`,
  )
})
