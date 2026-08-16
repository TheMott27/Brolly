/**
 * Brolly Settings Page — v2.1.2
 * Weather: Open-Meteo only (no API key).
 * Location: empty field = GPS; placeholder shows live GPS city.
 * Reset All: resets every field to DEFAULTS and clears localStorage.
 */

import React, { useState, useEffect, useRef } from 'react'
import { DEFAULTS, BrollySettings } from './defaults'
import { PebbleColorPicker, toHex } from './PebbleColorPicker'

const VERSION = 'v2.1.5'

// ─── Helpers ─────────────────────────────────────────────────────────────────

function getReturnTo(): string {
  const params = new URLSearchParams(window.location.search)
  return params.get('return_to') || 'pebblejs://close#'
}

/** Resolve a free-form location to the canonical City, Country form. */
async function verifyCustomLocation(rawLocation: string): Promise<string> {
  const query = rawLocation.trim().replace(/\s+/g, ' ')
  if (!query) return ''

  const response = await fetch(
    `https://geocoding-api.open-meteo.com/v1/search?name=${encodeURIComponent(query)}&count=1&language=en&format=json`
  )
  if (!response.ok) throw new Error('Location lookup failed')

  const data = await response.json()
  const match = data?.results?.[0]
  if (!match) throw new Error('No matching city found')

  const city = String(match.name || match.admin1 || '').trim()
  const country = String(match.country || '').trim()
  if (!city || !country) throw new Error('Incomplete location result')
  return `${city}, ${country}`
}

// ─── Tiny reusable components ─────────────────────────────────────────────────

const Sep = () => (
  <div style={{ height: 1, background: 'rgba(255,255,255,0.07)', margin: '10px 0' }} />
)

const SectionTitle = ({ children }: { children: React.ReactNode }) => (
  <h2 style={{
    margin: '0 0 14px', fontSize: 11, fontWeight: 700, letterSpacing: '0.1em',
    textTransform: 'uppercase', color: '#2dd4bf',
  }}>{children}</h2>
)

const SubTitle = ({ children }: { children: React.ReactNode }) => (
  <h3 style={{
    margin: '14px 0 8px', fontSize: 10, fontWeight: 700, letterSpacing: '0.08em',
    textTransform: 'uppercase', color: '#14b8a6', opacity: 0.8,
  }}>{children}</h3>
)

const Card = ({ children }: { children: React.ReactNode }) => (
  <div style={{
    background: 'rgba(15,30,46,0.85)',
    border: '1px solid rgba(255,255,255,0.07)',
    borderRadius: 12, padding: '18px 18px 14px',
    marginBottom: 16,
  }}>
    {children}
  </div>
)

const RowLabel = ({ label, desc }: { label: string; desc?: string }) => (
  <div style={{ flex: 1, minWidth: 0 }}>
    <div style={{ fontSize: 14, fontWeight: 600, color: '#f0f4f8' }}>{label}</div>
    {desc && <div style={{ fontSize: 11, color: '#64748b', marginTop: 2 }}>{desc}</div>}
  </div>
)

function ToggleRow({ label, desc, checked, onChange }: {
  label: string; desc?: string; checked: boolean; onChange: (v: boolean) => void
}) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '4px 0' }}>
      <RowLabel label={label} desc={desc} />
      <button
        role="switch" aria-checked={checked} onClick={() => onChange(!checked)}
        style={{
          flexShrink: 0, width: 46, height: 26, borderRadius: 13, border: 'none',
          background: checked ? '#0d9488' : '#1e3a5f', cursor: 'pointer',
          position: 'relative', transition: 'background 0.2s',
        }}
      >
        <span style={{
          position: 'absolute', top: 4, left: checked ? 24 : 4,
          width: 18, height: 18, borderRadius: '50%', background: '#fff',
          transition: 'left 0.18s',
        }} />
      </button>
    </div>
  )
}

function SelectRow({ label, desc, value, options, onChange }: {
  label: string; desc?: string; value: number;
  options: { label: string; value: number }[];
  onChange: (v: number) => void
}) {
  return (
    <div style={{ padding: '4px 0' }}>
      <RowLabel label={label} desc={desc} />
      <select
        value={value}
        onChange={e => onChange(parseInt(e.target.value))}
        style={{
          marginTop: 8, width: '100%',
          background: '#0a1929', border: '1px solid rgba(255,255,255,0.12)',
          color: '#f0f4f8', borderRadius: 8, padding: '9px 12px',
          // 16px prevents iOS/Pebble webview zooming when a select is focused.
          fontSize: 16, outline: 'none', cursor: 'pointer',
        }}
      >
        {options.map(o => <option key={o.value} value={o.value}>{o.label}</option>)}
      </select>
    </div>
  )
}

function SliderRow({ label, desc, value, min, max, onChange }: {
  label: string; desc?: string; value: number; min: number; max: number;
  onChange: (v: number) => void
}) {
  return (
    <div style={{ padding: '4px 0' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
        <RowLabel label={label} desc={desc} />
        <span style={{ fontFamily: 'JetBrains Mono, monospace', fontSize: 13, color: '#2dd4bf', marginLeft: 12, flexShrink: 0 }}>{value}</span>
      </div>
      <input
        type="range" min={min} max={max} value={value}
        onChange={e => onChange(parseInt(e.target.value))}
        style={{ width: '100%', marginTop: 8, accentColor: '#0d9488', cursor: 'pointer' }}
      />
    </div>
  )
}

function ColorRow({ label, desc, value, onChange }: {
  label: string; desc?: string; value: number; onChange: (v: number) => void
}) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '4px 0' }}>
      <RowLabel label={label} desc={desc} />
      <div style={{ flexShrink: 0 }}>
        <PebbleColorPicker label={label} value={value} onChange={onChange} />
      </div>
    </div>
  )
}

function ActionBtn({ children, onClick, variant = 'primary', small = false }: {
  children: React.ReactNode; onClick: () => void;
  variant?: 'primary' | 'danger' | 'warning' | 'ghost'; small?: boolean
}) {
  const [hover, setHover] = useState(false)
  const colours = {
    primary:  { base: '#0d9488', hover: '#0f766e' },
    danger:   { base: '#991b1b', hover: '#7f1d1d' },
    warning:  { base: '#b45309', hover: '#92400e' },
    ghost:    { base: 'rgba(30,58,95,0.6)', hover: 'rgba(30,58,95,0.9)' },
  }
  const c = colours[variant]
  return (
    <button
      onClick={onClick}
      onMouseOver={() => setHover(true)}
      onMouseOut={() => setHover(false)}
      style={{
        width: '100%', padding: small ? '8px 0' : '11px 0',
        background: hover ? c.hover : c.base,
        border: variant === 'ghost' ? '1px solid rgba(255,255,255,0.12)' : 'none',
        color: '#fff', borderRadius: 8,
        fontSize: small ? 12 : 14, fontWeight: 600,
        cursor: 'pointer', transition: 'background 0.15s',
        marginBottom: 8,
      }}
    >
      {children}
    </button>
  )
}

// ─── Toast ────────────────────────────────────────────────────────────────────

function Toast({ message }: { message: string }) {
  return (
    <div style={{
      position: 'fixed', top: 16, left: '50%', transform: 'translateX(-50%)',
      background: '#0d9488', color: '#fff', padding: '10px 20px',
      borderRadius: 8, fontSize: 14, fontWeight: 600,
      zIndex: 9999, boxShadow: '0 4px 20px rgba(0,0,0,0.5)',
      whiteSpace: 'nowrap', pointerEvents: 'none',
    }}>
      {message}
    </div>
  )
}

// ─── GPS resolution helper ────────────────────────────────────────────────────
// Resolves the device's current GPS position to a human-readable string
// in the format "City, Country, Postcode" (e.g. "Copenhagen, Denmark, 2300").

async function resolveGpsLabel(): Promise<string> {
  return new Promise((resolve) => {
    if (!navigator.geolocation) {
      resolve('(GPS: not available)')
      return
    }
    navigator.geolocation.getCurrentPosition(
      async (pos) => {
        const { latitude, longitude } = pos.coords
        try {
          const res = await fetch(
            `https://nominatim.openstreetmap.org/reverse?lat=${latitude}&lon=${longitude}&format=json&zoom=10&accept-language=en`,
            { headers: { 'Accept-Language': 'en' } }
          )
          const data = await res.json()
          const addr = data.address || {}
          const city = addr.city || addr.town || addr.village || addr.hamlet || addr.county || ''
          const country = addr.country || ''
          const postcode = addr.postcode || ''
          const parts: string[] = []
          if (city) parts.push(city)
          if (country) parts.push(country)
          if (postcode) parts.push(postcode)
          if (parts.length > 0) {
            resolve(`(GPS: ${parts.join(', ')})`)
          } else {
            resolve(`(GPS: ${latitude.toFixed(3)}, ${longitude.toFixed(3)})`)
          }
        } catch {
          resolve(`(GPS: ${latitude.toFixed(3)}, ${longitude.toFixed(3)})`)
        }
      },
      () => resolve('(GPS: permission denied)'),
      { timeout: 10000, maximumAge: 60000 }
    )
  })
}

function useGpsPlaceholder(): { placeholder: string; refresh: () => void } {
  const [placeholder, setPlaceholder] = useState('(GPS: detecting…)')

  function refresh() {
    setPlaceholder('(GPS: detecting…)')
    resolveGpsLabel().then(setPlaceholder)
  }

  useEffect(() => {
    resolveGpsLabel().then(setPlaceholder)
  }, [])

  return { placeholder, refresh }
}

// ─── Tab content components ───────────────────────────────────────────────────

function DisplayTab({ s, set, onResetColours }: {
  s: BrollySettings;
  set: <K extends keyof BrollySettings>(k: K, v: BrollySettings[K]) => void;
  onResetColours: () => void
}) {
  return (
    <>
      <Card>
        <SectionTitle>Appearance</SectionTitle>
        <SelectRow
          label="Number font"
          desc="Choose the font style for hour numbers"
          value={s.KEY_NUMBER_FONT}
          options={[
            { label: 'Digital', value: 0 },
            { label: 'Regular', value: 1 },
            { label: 'Traditional', value: 2 },
            { label: 'Thin', value: 3 },
            { label: 'Tall', value: 4 },
          ]}
          onChange={v => set('KEY_NUMBER_FONT', v)}
        />
        <Sep />
        <SliderRow
          label="Number size"
          desc="Adjust the size of hour numbers"
          value={s.KEY_NUMBER_SIZE} min={1} max={5}
          onChange={v => set('KEY_NUMBER_SIZE', v)}
        />
        <Sep />
        <SliderRow
          label="Icon size"
          desc="Adjust the size of weather icons"
          value={s.KEY_ICON_SIZE} min={1} max={5}
          onChange={v => set('KEY_ICON_SIZE', v)}
        />
      </Card>

      <Card>
        <SectionTitle>Watch Face</SectionTitle>
        <ColorRow
          label="Background colour"
          desc="Dial background colour"
          value={s.KEY_BACKGROUND_COLOR}
          onChange={v => set('KEY_BACKGROUND_COLOR', v)}
        />
        <Sep />
        <SubTitle>Hour hand</SubTitle>
        <ColorRow label="Outer colour" value={s.KEY_HOUR_HAND_OUTER} onChange={v => set('KEY_HOUR_HAND_OUTER', v)} />
        <Sep />
        <ColorRow label="Inner colour" value={s.KEY_HOUR_HAND_INNER} onChange={v => set('KEY_HOUR_HAND_INNER', v)} />
        <Sep />
        <SubTitle>Minute hand</SubTitle>
        <ColorRow label="Outer colour" value={s.KEY_MIN_HAND_OUTER} onChange={v => set('KEY_MIN_HAND_OUTER', v)} />
        <Sep />
        <ColorRow label="Inner colour" value={s.KEY_MIN_HAND_INNER} onChange={v => set('KEY_MIN_HAND_INNER', v)} />
        <Sep />
        <SubTitle>Seconds hand</SubTitle>
        <ColorRow label="Colour" value={s.KEY_SECONDS_HAND_COLOR} onChange={v => set('KEY_SECONDS_HAND_COLOR', v)} />
        <Sep />
        <SelectRow
          label="Seconds hand visibility"
          desc="When to show the seconds hand"
          value={s.KEY_SECONDS_HAND_MODE}
          options={[
            { label: 'Never', value: 0 },
            { label: 'Always', value: 1 },
            { label: 'Show on shake only', value: 2 },
          ]}
          onChange={v => set('KEY_SECONDS_HAND_MODE', v)}
        />
        <Sep />
        <SliderRow
          label="Seconds display duration"
          desc="How long (seconds) to show seconds hand after a shake"
          value={s.KEY_SECONDS_SHAKE_DUR} min={1} max={30}
          onChange={v => set('KEY_SECONDS_SHAKE_DUR', v)}
        />
      </Card>

      <Card>
        <SectionTitle>Markers, Numbers &amp; Icons</SectionTitle>
        <SubTitle>Hour markers</SubTitle>
        <ColorRow label="Hour marker colour" value={s.KEY_HOUR_MARKER_COLOR} onChange={v => set('KEY_HOUR_MARKER_COLOR', v)} />
        <Sep />
        <SubTitle>Minute markers</SubTitle>
        <ColorRow label="Minute marker colour" value={s.KEY_MINUTE_MARKER_COLOR} onChange={v => set('KEY_MINUTE_MARKER_COLOR', v)} />
        <Sep />
        <SubTitle>Numbers</SubTitle>
        <ColorRow label="Number colour" value={s.KEY_NUMBER_COLOR} onChange={v => set('KEY_NUMBER_COLOR', v)} />
        <Sep />
        <SubTitle>Icons</SubTitle>
        <ColorRow label="Icon colour" desc="Used when icon colour mode is set to single colour" value={s.KEY_ICON_COLOR} onChange={v => set('KEY_ICON_COLOR', v)} />
      </Card>

      <ActionBtn onClick={onResetColours} variant="ghost">Reset colours to default</ActionBtn>
    </>
  )
}

function WeatherTab({ s, set, notify }: {
  s: BrollySettings;
  set: <K extends keyof BrollySettings>(k: K, v: BrollySettings[K]) => void;
  notify: (message: string) => void;
}) {
  const [updatingGps, setUpdatingGps] = useState(false)
  const [verifyingLocation, setVerifyingLocation] = useState(false)
  const locationInputRef = useRef<HTMLInputElement>(null)
  const locationVerificationInFlight = useRef(false)
  const lastVerifiedLocation = useRef('')
  const { placeholder: gpsPlaceholder, refresh: refreshGps } = useGpsPlaceholder()

  async function confirmCustomLocation() {
    const rawLocation = (locationInputRef.current?.value || s.KEY_CUSTOM_LOCATION)
      .trim()
      .replace(/\s+/g, ' ')

    // An empty location deliberately means GPS; it needs no city lookup.
    if (!rawLocation) {
      lastVerifiedLocation.current = ''
      return
    }
    // Blur and button-click can fire together; allow one lookup only.
    if (locationVerificationInFlight.current || rawLocation === lastVerifiedLocation.current) {
      return
    }

    locationVerificationInFlight.current = true
    setVerifyingLocation(true)
    try {
      const formattedLocation = await verifyCustomLocation(rawLocation)
      set('KEY_CUSTOM_LOCATION', formattedLocation)
      lastVerifiedLocation.current = formattedLocation
      notify(`Verified: ${formattedLocation}`)
    } catch {
      lastVerifiedLocation.current = ''
      notify('Location not found — please enter a city and country')
    } finally {
      locationVerificationInFlight.current = false
      setVerifyingLocation(false)
    }
  }

  async function handleUpdateGps() {
    // Clear any custom location text and re-run GPS resolution.
    // The text box will be empty (= GPS mode) and the placeholder will update.
    set('KEY_CUSTOM_LOCATION', '')
    setUpdatingGps(true)
    refreshGps()
    // Give the GPS a moment to resolve before re-enabling the button
    setTimeout(() => setUpdatingGps(false), 3000)
  }

  return (
    <>
      <Card>
        <SectionTitle>Weather Icon Visibility</SectionTitle>
        <SelectRow
          label="Weather icon visibility"
          desc="When to show weather icons on the dial"
          value={s.KEY_SHAKE_MODE}
          options={[
            { label: 'Show icons on shake', value: 0 },
            { label: 'Always show icons', value: 1 },
            { label: 'Always hide icons', value: 2 },
            { label: 'Side-By-Side', value: 3 },
          ]}
          onChange={v => set('KEY_SHAKE_MODE', v)}
        />
        <Sep />
        <SelectRow
          label="Icon style"
          desc="Use a single colour or weather-based colours for icons"
          value={s.KEY_ICON_COLOR_MODE}
          options={[
            { label: 'Single colour', value: 0 },
            { label: 'Coloured icons', value: 1 },
            { label: 'Coloured w/shading', value: 3 },
            { label: 'Rainbow', value: 2 },
          ]}
          onChange={v => set('KEY_ICON_COLOR_MODE', v)}
        />
      </Card>

      <Card>
        <SectionTitle>Location</SectionTitle>
        <div>
          <RowLabel
            label="Custom location"
            desc="LEAVE BLANK FOR GPS. Type city or country & postcode for custom location."
          />
          <input
            ref={locationInputRef}
            type="text"
            inputMode="text"
            autoCapitalize="words"
            value={s.KEY_CUSTOM_LOCATION}
            placeholder={gpsPlaceholder}
            onChange={e => {
              lastVerifiedLocation.current = ''
              set('KEY_CUSTOM_LOCATION', e.target.value)
            }}
            onBlur={() => { void confirmCustomLocation() }}
            onKeyDown={e => {
              if (e.key === 'Enter') {
                e.preventDefault()
                void confirmCustomLocation()
              }
            }}
            style={{
              marginTop: 8, width: '100%', boxSizing: 'border-box',
              background: '#0a1929', border: '1px solid rgba(255,255,255,0.12)',
              color: '#f0f4f8', borderRadius: 8, padding: '10px 12px',
              // A 16px native input prevents iOS/Pebble webview focus zoom.
              fontSize: 16, outline: 'none', touchAction: 'manipulation',
            }}
          />
          <button
            onClick={() => { void confirmCustomLocation() }}
            disabled={verifyingLocation}
            style={{
              marginTop: 8, padding: '7px 12px',
              background: 'rgba(13,148,136,0.16)', border: '1px solid rgba(45,212,191,0.55)',
              color: verifyingLocation ? '#64748b' : '#5eead4', borderRadius: 7,
              fontSize: 14, fontWeight: 600, cursor: verifyingLocation ? 'wait' : 'pointer',
              transition: 'background 0.15s', touchAction: 'manipulation',
            }}
          >
            {verifyingLocation ? 'Verifying location…' : 'Confirm city & country'}
          </button>
          <button
            onClick={handleUpdateGps}
            disabled={updatingGps}
            style={{
              marginTop: 8, width: '100%', padding: '10px 0',
              background: 'transparent', border: '1px solid rgba(45,212,191,0.4)',
              color: updatingGps ? '#64748b' : '#2dd4bf', borderRadius: 8,
              fontSize: 13, fontWeight: 600, cursor: updatingGps ? 'not-allowed' : 'pointer',
              transition: 'background 0.15s',
            }}
          >
            {updatingGps ? 'Detecting GPS…' : 'Update GPS Location'}
          </button>
          <div style={{ marginTop: 16 }}>
            <Sep />
            <SelectRow
              label="City name on dial"
              desc="Show the current weather city on the opposite side from date/temp"
              value={s.KEY_CITY_DISPLAY_MODE}
              options={[
                { label: 'Off', value: 0 },
                { label: 'Show on shake', value: 1 },
                { label: 'Always show', value: 2 },
              ]}
              onChange={v => set('KEY_CITY_DISPLAY_MODE', v)}
            />
            <Sep />
            <ColorRow
              label="City name colour"
              value={s.KEY_CITY_COLOR}
              onChange={v => set('KEY_CITY_COLOR', v)}
            />
            <Sep />
            <SelectRow
              label="Weather update interval"
              desc="How often to fetch new weather data from the phone. Longer intervals save more battery."
              value={s.KEY_WEATHER_INTERVAL}
              options={[
                { label: '30 minutes', value: 30 },
                { label: '60 minutes', value: 60 },
                { label: '120 minutes', value: 120 },
              ]}
              onChange={v => set('KEY_WEATHER_INTERVAL', v)}
            />
          </div>
        </div>
      </Card>

      <Card>
        <SectionTitle>Date &amp; Temperature</SectionTitle>
        <SelectRow
          label="Display"
          desc="Which information to show on the dial"
          value={s.KEY_DISPLAY_MODE}
          options={[
            { label: 'Date and temperature', value: 0 },
            { label: 'Temperature only', value: 1 },
            { label: 'Date only', value: 2 },
            { label: 'None', value: 3 },
          ]}
          onChange={v => set('KEY_DISPLAY_MODE', v)}
        />
        <Sep />
        <SelectRow
          label="Date visibility"
          desc="When to show the date"
          value={s.KEY_DATE_VISIBLE}
          options={[
            { label: 'Always show', value: 0 },
            { label: 'Always hide', value: 1 },
            { label: 'Show on shake', value: 2 },
          ]}
          onChange={v => set('KEY_DATE_VISIBLE', v)}
        />
        <Sep />
        <SelectRow
          label="Temperature visibility"
          desc="When to show the temperature"
          value={s.KEY_TEMP_VISIBLE}
          options={[
            { label: 'Always show', value: 0 },
            { label: 'Always hide', value: 1 },
            { label: 'Show on shake', value: 2 },
          ]}
          onChange={v => set('KEY_TEMP_VISIBLE', v)}
        />
        <Sep />
        <SelectRow
          label="Temperature unit"
          value={s.KEY_TEMP_UNIT}
          options={[
            { label: 'Celsius (°C)', value: 0 },
            { label: 'Fahrenheit (°F)', value: 1 },
          ]}
          onChange={v => set('KEY_TEMP_UNIT', v)}
        />
        <Sep />
        <ColorRow label="Date colour" value={s.KEY_DATE_COLOR} onChange={v => set('KEY_DATE_COLOR', v)} />
        <Sep />
        <ColorRow label="Temperature colour" value={s.KEY_TEMP_COLOR} onChange={v => set('KEY_TEMP_COLOR', v)} />
        <Sep />
        <SelectRow
          label="Layering"
          desc="Display date and temperature behind or on top of watch hands"
          value={s.KEY_COMPLICATION_LAYER}
          options={[
            { label: 'Behind hands', value: 0 },
            { label: 'On top of hands', value: 1 },
          ]}
          onChange={v => set('KEY_COMPLICATION_LAYER', v)}
        />
      </Card>

      <Card>
        <SectionTitle>Sunrise / Sunset</SectionTitle>
        <SelectRow
          label="Sunrise/Sunset markers"
          desc="Show sunrise and sunset marker lines on the dial"
          value={s.KEY_SUNRISE_MARKER_VISIBLE}
          options={[
            { label: 'Always show', value: 0 },
            { label: 'Show with weather icons only', value: 1 },
            { label: 'Off', value: 2 },
          ]}
          onChange={v => set('KEY_SUNRISE_MARKER_VISIBLE', v)}
        />
        <Sep />
        <ColorRow label="Sunrise marker colour" value={s.KEY_SUNRISE_MARKER_COLOR} onChange={v => set('KEY_SUNRISE_MARKER_COLOR', v)} />
        <Sep />
        <ColorRow label="Sunset marker colour" value={s.KEY_SUNSET_MARKER_COLOR} onChange={v => set('KEY_SUNSET_MARKER_COLOR', v)} />
      </Card>
    </>
  )
}

function AlertsTab({ s, set, onResetColours, onResetAll }: {
  s: BrollySettings;
  set: <K extends keyof BrollySettings>(k: K, v: BrollySettings[K]) => void;
  onResetColours: () => void;
  onResetAll: () => void;
}) {
  const [confirmReset, setConfirmReset] = useState(false)

  function sendAction(payload: Record<string, number>) {
    // Pebble requires the webview to close to send data back to the watch.
    // The settings page will close — this is expected behaviour.
    window.location.href = getReturnTo() + encodeURIComponent(JSON.stringify(payload))
  }

  function handleResetAllClick() {
    if (confirmReset) {
      onResetAll()
      setConfirmReset(false)
    } else {
      setConfirmReset(true)
    }
  }

  return (
    <>
      <Card>
        <SectionTitle>Bluetooth</SectionTitle>
        <ToggleRow
          label="Vibrate on disconnect"
          desc="Vibrate when Bluetooth disconnects"
          checked={s.KEY_VIBRATE_BT_DISCONNECT === 1}
          onChange={v => set('KEY_VIBRATE_BT_DISCONNECT', v ? 1 : 0)}
        />
        <Sep />
        <ToggleRow
          label="Vibrate on reconnect"
          desc="Vibrate when Bluetooth reconnects"
          checked={s.KEY_VIBRATE_BT_RECONNECT === 1}
          onChange={v => set('KEY_VIBRATE_BT_RECONNECT', v ? 1 : 0)}
        />
        <Sep />
        <ToggleRow
          label="Minute hand inner red on disconnect"
          desc="Turn minute hand inner stripe red when disconnected"
          checked={s.KEY_BT_DISCONNECT_MIN_INNER_RED === 1}
          onChange={v => set('KEY_BT_DISCONNECT_MIN_INNER_RED', v ? 1 : 0)}
        />
        <Sep />
        <ColorRow
          label="Disconnect colour"
          desc="Colour shown on the dial when Bluetooth is disconnected"
          value={s.KEY_BT_DISCONNECT_OUTER_COLOR}
          onChange={v => {
            set('KEY_BT_DISCONNECT_OUTER_COLOR', v)
            set('KEY_BT_DISCONNECT_INNER_COLOR', v)
          }}
        />
      </Card>

      <Card>
        <SectionTitle>Battery Alerts</SectionTitle>
        <SelectRow
          label="Low battery alert threshold"
          desc="Show ring alert when battery drops below this level"
          value={s.KEY_BATTERY_RING_THRESHOLD}
          options={[
            { label: 'Off', value: 0 },
            { label: '50%', value: 50 },
            { label: '40%', value: 40 },
            { label: '30%', value: 30 },
            { label: '20%', value: 20 },
            { label: '10%', value: 10 },
          ]}
          onChange={v => set('KEY_BATTERY_RING_THRESHOLD', v)}
        />
        <Sep />
        <SelectRow
          label="Critical battery alert threshold"
          desc="Show centre dot alert when battery drops below this level"
          value={s.KEY_BATTERY_CENTER_THRESHOLD}
          options={[
            { label: 'Off', value: 0 },
            { label: '20%', value: 20 },
            { label: '10%', value: 10 },
            { label: '5%', value: 5 },
          ]}
          onChange={v => set('KEY_BATTERY_CENTER_THRESHOLD', v)}
        />
      </Card>

      <Card>
        <SectionTitle>Test Alerts</SectionTitle>
        <ActionBtn onClick={() => sendAction({ KEY_TEST_BATTERY_ALERT: 1 })} variant="warning">
          Test low battery alert
        </ActionBtn>
        <ActionBtn onClick={() => sendAction({ KEY_TEST_CRITICAL_BATTERY_ALERT: 1 })} variant="danger">
          Test critical battery alert
        </ActionBtn>
        <ActionBtn onClick={() => sendAction({ KEY_TEST_BT_DISCONNECT: 1 })} variant="primary">
          Test Bluetooth disconnect warning
        </ActionBtn>
      </Card>

      <ActionBtn onClick={onResetColours} variant="ghost">Reset colours to default</ActionBtn>
      {confirmReset ? (
        <div style={{ display: 'flex', gap: 8 }}>
          <button
            onClick={() => { onResetAll(); setConfirmReset(false) }}
            style={{
              flex: 1, padding: '9px 0', background: '#991b1b', border: 'none',
              color: '#fff', borderRadius: 8, fontSize: 13, fontWeight: 700,
              cursor: 'pointer',
            }}
          >
            Confirm reset
          </button>
          <button
            onClick={() => setConfirmReset(false)}
            style={{
              flex: 1, padding: '9px 0', background: 'rgba(30,58,95,0.6)',
              border: '1px solid rgba(255,255,255,0.12)',
              color: '#fff', borderRadius: 8, fontSize: 13, fontWeight: 700,
              cursor: 'pointer',
            }}
          >
            Cancel
          </button>
        </div>
      ) : (
        <ActionBtn onClick={handleResetAllClick} variant="danger" small>Reset ALL settings</ActionBtn>
      )}
    </>
  )
}

// ─── Main App ─────────────────────────────────────────────────────────────────

export default function App() {
  const [settings, setSettings] = useState<BrollySettings>(() => {
    const saved = localStorage.getItem('brolly_settings')
    const params = new URLSearchParams(window.location.hash.slice(1))
    const urlSettings: Partial<BrollySettings> = {}
    params.forEach((v, k) => {
      if (v === 'true') (urlSettings as any)[k] = 1
      else if (v === 'false') (urlSettings as any)[k] = 0
      else if (!isNaN(Number(v))) (urlSettings as any)[k] = Number(v)
      else (urlSettings as any)[k] = v
    })
    return { ...DEFAULTS, ...(saved ? JSON.parse(saved) : {}), ...urlSettings }
  })

  const [tab, setTab] = useState<'display' | 'weather' | 'alerts'>('display')
  const [toast, setToast] = useState<string | null>(null)

  useEffect(() => {
    localStorage.setItem('brolly_settings', JSON.stringify(settings))
  }, [settings])

  useEffect(() => {
    if (!toast) return
    const t = setTimeout(() => setToast(null), 2500)
    return () => clearTimeout(t)
  }, [toast])

  function set<K extends keyof BrollySettings>(k: K, v: BrollySettings[K]) {
    setSettings(prev => ({ ...prev, [k]: v }))
  }

  function handleSave() {
    const payload: Record<string, number | string> = { ...settings }
    localStorage.setItem('brolly_settings', JSON.stringify(settings))
    setToast('Settings saved!')
    setTimeout(() => {
      window.location.href = getReturnTo() + encodeURIComponent(JSON.stringify(payload))
    }, 600)
  }

  function handleResetColours() {
    setSettings(prev => ({
      ...prev,
      KEY_BACKGROUND_COLOR: DEFAULTS.KEY_BACKGROUND_COLOR,
      KEY_HOUR_HAND_OUTER: DEFAULTS.KEY_HOUR_HAND_OUTER,
      KEY_HOUR_HAND_INNER: DEFAULTS.KEY_HOUR_HAND_INNER,
      KEY_MIN_HAND_OUTER: DEFAULTS.KEY_MIN_HAND_OUTER,
      KEY_MIN_HAND_INNER: DEFAULTS.KEY_MIN_HAND_INNER,
      KEY_SECONDS_HAND_COLOR: DEFAULTS.KEY_SECONDS_HAND_COLOR,
      KEY_HOUR_MARKER_COLOR: DEFAULTS.KEY_HOUR_MARKER_COLOR,
      KEY_MINUTE_MARKER_COLOR: DEFAULTS.KEY_MINUTE_MARKER_COLOR,
      KEY_NUMBER_COLOR: DEFAULTS.KEY_NUMBER_COLOR,
      KEY_ICON_COLOR: DEFAULTS.KEY_ICON_COLOR,
      KEY_DATE_COLOR: DEFAULTS.KEY_DATE_COLOR,
      KEY_TEMP_COLOR: DEFAULTS.KEY_TEMP_COLOR,
      KEY_SUNRISE_MARKER_COLOR: DEFAULTS.KEY_SUNRISE_MARKER_COLOR,
      KEY_SUNSET_MARKER_COLOR: DEFAULTS.KEY_SUNSET_MARKER_COLOR,
      KEY_BT_DISCONNECT_OUTER_COLOR: DEFAULTS.KEY_BT_DISCONNECT_OUTER_COLOR,
      KEY_BT_DISCONNECT_INNER_COLOR: DEFAULTS.KEY_BT_DISCONNECT_INNER_COLOR,
    }))
    setToast('Colours reset to defaults')
  }

  // Reset ALL settings: replace every field with DEFAULTS and wipe localStorage.
  // Does NOT use window.confirm — that is blocked in the Pebble webview.
  // Instead, the AlertsTab shows an inline confirm/cancel button pair.
  function handleResetAll() {
    // Replace—not merge—settings and save the complete defaults object
    // immediately, so every selector is reset (including Icon Style = Single colour).
    const fresh: BrollySettings = { ...DEFAULTS }
    localStorage.setItem('brolly_settings', JSON.stringify(fresh))
    setSettings(fresh)
    setToast('All settings reset — Icon Style is Single colour')
  }

  const TABS = [
    { id: 'display' as const, label: 'DISPLAY' },
    { id: 'weather' as const, label: 'WEATHER' },
    { id: 'alerts' as const, label: 'ALERTS' },
  ]

  return (
    <div style={{
      minHeight: '100vh',
      background: 'linear-gradient(160deg, #020c18 0%, #030e1c 50%, #020a14 100%)',
      color: '#f0f4f8',
      fontFamily: "'Space Grotesk', 'Inter', system-ui, sans-serif",
    }}>
      {toast && <Toast message={toast} />}

      {/* Sticky header */}
      <div style={{
        position: 'sticky', top: 0, zIndex: 50,
        background: 'rgba(2,12,24,0.95)',
        backdropFilter: 'blur(12px)',
        borderBottom: '1px solid rgba(255,255,255,0.07)',
        padding: '14px 16px 12px',
      }}>
        <div style={{ maxWidth: 640, margin: '0 auto' }}>
          {/* Title + version badge */}
          <div style={{ display: 'flex', alignItems: 'baseline', gap: 10, marginBottom: 12 }}>
            <h1 style={{
              margin: 0, fontSize: 20, fontWeight: 800, letterSpacing: '-0.3px',
              background: 'linear-gradient(90deg, #2dd4bf, #38bdf8)',
              WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent',
            }}>
              BROLLY SETTINGS
            </h1>
            <span style={{
              fontSize: 11, color: '#475569', fontWeight: 500,
              background: 'rgba(45,212,191,0.08)', border: '1px solid rgba(45,212,191,0.2)',
              borderRadius: 4, padding: '1px 6px',
            }}>{VERSION}</span>
          </div>

          {/* Tabs + Save */}
          <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <div style={{
              flex: 1, display: 'grid', gridTemplateColumns: '1fr 1fr 1fr',
              background: '#0a1929', border: '1px solid rgba(255,255,255,0.08)',
              borderRadius: 8, overflow: 'hidden',
            }}>
              {TABS.map(t => (
                <button
                  key={t.id}
                  onClick={() => setTab(t.id)}
                  style={{
                    padding: '9px 4px', fontSize: 11, fontWeight: 700,
                    letterSpacing: '0.06em', border: 'none', cursor: 'pointer',
                    transition: 'background 0.15s, color 0.15s',
                    background: tab === t.id
                      ? 'linear-gradient(135deg, #0d9488, #0891b2)'
                      : 'transparent',
                    color: tab === t.id ? '#fff' : '#64748b',
                  }}
                >
                  {t.label}
                </button>
              ))}
            </div>
            <button
              onClick={handleSave}
              style={{
                padding: '9px 16px', fontSize: 13, fontWeight: 700,
                background: 'linear-gradient(135deg, #0d9488, #0891b2)',
                color: '#fff', border: 'none', borderRadius: 8,
                cursor: 'pointer', whiteSpace: 'nowrap',
                boxShadow: '0 2px 8px rgba(13,148,136,0.4)',
              }}
            >
              ✓ Save
            </button>
          </div>
        </div>
      </div>

      {/* Tab content */}
      <div style={{ maxWidth: 640, margin: '0 auto', padding: '20px 16px 60px' }}>
        {tab === 'display' && (
          <DisplayTab s={settings} set={set} onResetColours={handleResetColours} />
        )}
        {tab === 'weather' && (
          <WeatherTab s={settings} set={set} notify={setToast} />
        )}
        {tab === 'alerts' && (
          <AlertsTab s={settings} set={set} onResetColours={handleResetColours} onResetAll={handleResetAll} />
        )}
      </div>
    </div>
  )
}
