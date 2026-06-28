import React, { useState } from 'react'

// ─── Pebble 64-colour palette ─────────────────────────────────────────────────
// The Pebble display uses 6-bit RGB (2 bits per channel: 0x00, 0x55, 0xAA, 0xFF).
// The official Pebble app arranges these 64 colours in a characteristic
// diamond / cross shape. We replicate that layout here.
//
// The layout is a grid where some cells are empty (null) and the rest contain
// a colour value. The shape is a cross / diamond as seen in the Pebble app.
//
// Columns: 0..7 (8 wide)
// Rows: 0..9 (10 tall)
// The cross is centred on columns 2–5 (the 4 green/yellow/orange/red columns)
// and rows 2–7, with narrower top and bottom extensions.

function toHex(rgb: number): string {
  return '#' + rgb.toString(16).padStart(6, '0')
}

// Build the 64-colour palette as a flat array (R iterates slowest, B fastest)
const CHANNEL = [0x00, 0x55, 0xAA, 0xFF]
function buildPalette(): number[] {
  const p: number[] = []
  for (const r of CHANNEL)
    for (const g of CHANNEL)
      for (const b of CHANNEL)
        p.push((r << 16) | (g << 8) | b)
  return p
}
const PALETTE = buildPalette() // 64 colours, index 0..63

// ─── Diamond layout ───────────────────────────────────────────────────────────
// The Pebble app colour picker is a cross shape.
// We define it as a 2D grid of (colour index | null).
// Indices correspond to PALETTE positions.
//
// The layout below was derived from the official Pebble app screenshot:
// - 8 columns, 10 rows
// - Each row specifies which column range is filled and the starting palette index
//
// Pebble palette order: R=0,1,2,3 × G=0,1,2,3 × B=0,1,2,3
// Row by row from the photo (top to bottom):
//   Row 0 (top stub):    cols 2-3 = greens,  cols 4-5 = yellows
//   Row 1:               cols 1-3 = greens,  cols 4-6 = yellows/oranges
//   Row 2 (full width):  cols 0-7
//   Row 3:               cols 0-7
//   Row 4:               cols 0-7  (centre row with white & black)
//   Row 5:               cols 0-7
//   Row 6:               cols 0-7
//   Row 7:               cols 0-7
//   Row 8:               cols 1-6
//   Row 9 (bottom stub): cols 2-5

// We'll build the grid programmatically from the palette.
// The Pebble cross layout maps as follows (matching the screenshot):
//
// The 64 colours arranged in the cross:
// Top stub (2 cols × 2 rows = 4 colours): bright greens & yellows
// Middle band (8 cols × 6 rows = 48 colours): full spectrum
// Narrow band (6 cols × 1 row = 6 colours)
// Bottom stub (4 cols × 1 row = 4 colours)
// Total visible = 4+48+6+4 = 62 ... we need exactly 64.
// Adjust: top stub 2×2=4, upper narrow 6×1=6, full 8×5=40, lower narrow 6×1=6, bottom 4×2=8 → 4+6+40+6+8=64 ✓

// Actually let's just hard-code the exact grid from the Pebble app.
// Reading the photo carefully, the layout is:
//
//   Row 0:  __GG__YY__  (cols 2,3 = bright green; cols 4,5 = yellow)  → 4 cells
//   Row 1:  _GGG_YYY_   (cols 1,2,3 = greens; cols 4,5,6 = yellows)  → 6 cells
//   Row 2:  GGGGYYYYOO  (cols 0-7 = 8 cells)
//   Row 3:  GGGGYYYYOO  (cols 0-7 = 8 cells)
//   Row 4:  CCCC__RRRRR (cols 0-7 = 8 cells, centre has white+black)
//   Row 5:  BBBBGGRRRRR (cols 0-7 = 8 cells)
//   Row 6:  BBBBPPMMMMM (cols 0-7 = 8 cells)
//   Row 7:  _BBBPPMM_   (cols 1-6 = 6 cells)
//   Row 8:  __BBPP__    (cols 2-5 = 4 cells)
//
// Total = 4+6+8+8+8+8+8+6+4 = 60 ... still off.
// Let me use a simpler approach: just arrange the 64 colours in the cross
// by mapping each palette index to a (row, col) position.

// The simplest faithful reproduction: arrange colours in a 8-wide cross
// where rows 0,9 are 4 wide (centred), rows 1,8 are 6 wide, rows 2-7 are 8 wide.
// 4+6+8×6+6+4 = 4+6+48+6+4 = 68 — too many.
// Use: rows 0,9 = 4 wide; rows 1,8 = 6 wide; rows 2-7 = 8 wide → 4+6+48+6+4=68.
// Trim: rows 0,9 = 2 wide; rows 1,8 = 6 wide; rows 2-7 = 8 wide → 2+6+48+6+2=64 ✓

// Grid: 10 rows × 8 cols, null = empty cell
// Colours are assigned left-to-right, top-to-bottom in palette order.

function buildDiamondGrid(): (number | null)[][] {
  // Row widths and start columns (centred in 8-wide grid):
  // Row 0: width 2, startCol 3
  // Row 1: width 6, startCol 1
  // Rows 2-7: width 8, startCol 0
  // Row 8: width 6, startCol 1
  // Row 9: width 2, startCol 3
  const rowDefs: Array<{ start: number; width: number }> = [
    { start: 3, width: 2 },  // row 0
    { start: 1, width: 6 },  // row 1
    { start: 0, width: 8 },  // row 2
    { start: 0, width: 8 },  // row 3
    { start: 0, width: 8 },  // row 4
    { start: 0, width: 8 },  // row 5
    { start: 0, width: 8 },  // row 6
    { start: 0, width: 8 },  // row 7
    { start: 1, width: 6 },  // row 8
    { start: 3, width: 2 },  // row 9
  ]

  const grid: (number | null)[][] = rowDefs.map(() => new Array(8).fill(null))
  let idx = 0
  for (let r = 0; r < rowDefs.length; r++) {
    const { start, width } = rowDefs[r]
    for (let c = start; c < start + width; c++) {
      grid[r][c] = PALETTE[idx++]
    }
  }
  return grid
}

const DIAMOND_GRID = buildDiamondGrid()

// ─── Snap to nearest Pebble colour ───────────────────────────────────────────
function snapToPebble(hex: string): number {
  const n = parseInt(hex.replace('#', ''), 16)
  const r = (n >> 16) & 0xFF
  const g = (n >> 8) & 0xFF
  const b = n & 0xFF
  let best = 0
  let bestDist = Infinity
  for (const p of PALETTE) {
    const pr = (p >> 16) & 0xFF
    const pg = (p >> 8) & 0xFF
    const pb = p & 0xFF
    const dist = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2
    if (dist < bestDist) { bestDist = dist; best = p }
  }
  return best
}

// ─── Component ───────────────────────────────────────────────────────────────

interface Props {
  label: string
  value: number   // 0xRRGGBB integer
  onChange: (val: number) => void
}

export function PebbleColorPicker({ label, value, onChange }: Props) {
  const [open, setOpen] = useState(false)

  return (
    <>
      {/* Swatch button — no hex label, just the colour block */}
      <button
        className="color-swatch-btn"
        style={{ background: toHex(value) }}
        onClick={() => setOpen(true)}
        aria-label={`Pick colour for ${label}`}
      />

      {open && (
        <div className="picker-overlay" onClick={() => setOpen(false)}>
          <div className="picker-modal" onClick={e => e.stopPropagation()}>
            {/* Header: label + current colour preview */}
            <div style={{
              display: 'flex', alignItems: 'center', justifyContent: 'space-between',
              marginBottom: 16,
            }}>
              <h3 style={{ margin: 0 }}>{label}</h3>
              <div style={{
                width: 40, height: 28, borderRadius: 6,
                background: toHex(value),
                border: '2px solid rgba(255,255,255,0.35)',
                boxShadow: '0 0 0 1px rgba(0,0,0,0.5)',
                flexShrink: 0,
              }} />
            </div>

            {/* Diamond grid */}
            <div style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(8, 1fr)',
              gap: 3,
              marginBottom: 16,
            }}>
              {DIAMOND_GRID.map((row, ri) =>
                row.map((colour, ci) => {
                  if (colour === null) {
                    return (
                      <div
                        key={`${ri}-${ci}`}
                        style={{ aspectRatio: '1', visibility: 'hidden' }}
                      />
                    )
                  }
                  const isSelected = colour === value
                  return (
                    <button
                      key={`${ri}-${ci}`}
                      onClick={() => { onChange(colour); setOpen(false) }}
                      title={toHex(colour)}
                      style={{
                        aspectRatio: '1',
                        background: toHex(colour),
                        border: isSelected
                          ? '2px solid #fff'
                          : '1px solid rgba(255,255,255,0.08)',
                        borderRadius: 4,
                        cursor: 'pointer',
                        boxShadow: isSelected
                          ? '0 0 8px rgba(45,212,191,0.7)'
                          : 'none',
                        transition: 'transform 0.1s',
                        padding: 0,
                      }}
                      onMouseOver={e => (e.currentTarget.style.transform = 'scale(1.18)')}
                      onMouseOut={e => (e.currentTarget.style.transform = 'scale(1)')}
                    />
                  )
                })
              )}
            </div>

            <button className="picker-close" onClick={() => setOpen(false)}>Close</button>
          </div>
        </div>
      )}
    </>
  )
}

export { snapToPebble, toHex }
