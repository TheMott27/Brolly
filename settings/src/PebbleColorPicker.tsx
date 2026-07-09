import React, { useState } from 'react'

// ─── Helpers ──────────────────────────────────────────────────────────────────
function toHex(rgb: number): string {
  if (rgb === -1) return 'transparent'
  return '#' + rgb.toString(16).padStart(6, '0')
}

// ─── Pebble 64-colour cross layout ───────────────────────────────────────────
// All 64 Pebble colours (R,G,B ∈ {0x00,0x55,0xAA,0xFF}) arranged in the
// cross/diamond shape used by the official Pebble app.
//
// Cross shape — 8 columns, 10 rows (2+6+8×6+6+2 = 64 cells):
//   Row 0: cols 3-4   (2 cells)
//   Row 1: cols 1-6   (6 cells)
//   Rows 2-7: cols 0-7 (8 cells each)
//   Row 8: cols 1-6   (6 cells)
//   Row 9: cols 3-4   (2 cells)
//
// Colours are sorted by HSV hue (then saturation desc, value desc) so the
// spectrum flows naturally around the cross: reds at top, greens middle-left,
// blues/purples at bottom — matching the official Pebble app layout.

const N = null

const GRID: (number | null)[][] = [
  [N,        N,        N,        0xff0000, 0xaa0000, N,        N,        -1      ],  // row 0 (-1 is transparent)
  [N,        0x550000, 0xff5555, 0xaa5555, 0xffaaaa, 0xffffff, 0xaaaaaa, N       ],  // row 1
  [0x555555, 0x000000, 0xff5500, 0xaa5500, 0xffaa55, 0xffaa00, 0xffff00, 0xaaaa00],  // row 2
  [0x555500, 0xffff55, 0xaaaa55, 0xffffaa, 0xaaff00, 0x55aa00, 0xaaff55, 0x55ff00],  // row 3
  [0x00ff00, 0x00aa00, 0x005500, 0x55ff55, 0x55aa55, 0xaaffaa, 0x00ff55, 0x00aa55],  // row 4
  [0x55ffaa, 0x00ffaa, 0x00ffff, 0x00aaaa, 0x005555, 0x55ffff, 0x55aaaa, 0xaaffff],  // row 5
  [0x00aaff, 0x0055aa, 0x55aaff, 0x0055ff, 0x0000ff, 0x0000aa, 0x000055, 0x5555ff],  // row 6
  [0x5555aa, 0xaaaaff, 0x5500ff, 0x5500aa, 0xaa55ff, 0xaa00ff, 0xff00ff, 0xaa00aa],  // row 7
  [N,        0x550055, 0xff55ff, 0xaa55aa, 0xffaaff, 0xff00aa, 0xaa0055, N       ],  // row 8
  [N,        N,        N,        0xff55aa, 0xff0055, N,        N,        N       ],  // row 9
]

// ─── Snap to nearest Pebble colour ───────────────────────────────────────────
const PALETTE: number[] = []
const CH = [0x00, 0x55, 0xAA, 0xFF]
for (const r of CH) for (const g of CH) for (const b of CH)
  PALETTE.push((r << 16) | (g << 8) | b)

function snapToPebble(hex: string): number {
  const n = parseInt(hex.replace('#', ''), 16)
  const r = (n >> 16) & 0xFF
  const g = (n >> 8) & 0xFF
  const b = n & 0xFF
  let best = 0, bestDist = Infinity
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

            {/* Cross/diamond grid — transparent background */}
            <div style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(8, 1fr)',
              gap: 4,
              marginBottom: 16,
              background: 'transparent',
            }}>
              {GRID.map((row, ri) =>
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
                  const isTransparent = colour === -1
                  return (
                    <button
                      key={`${ri}-${ci}`}
                      onClick={() => { onChange(colour); setOpen(false) }}
                      title={isTransparent ? 'Transparent' : toHex(colour)}
                      style={{
                        aspectRatio: '1',
                        background: isTransparent 
                          ? 'repeating-conic-gradient(#ccc 0% 25%, #fff 0% 50%) 50% / 8px 8px'
                          : toHex(colour),
                        border: isSelected
                          ? '2px solid #fff'
                          : colour === 0x000000
                            ? '1px solid rgba(255,255,255,0.3)'
                            : '1px solid rgba(255,255,255,0.08)',
                        borderRadius: 6,
                        cursor: 'pointer',
                        boxShadow: isSelected ? '0 0 8px rgba(255,255,255,0.7)' : 'none',
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
