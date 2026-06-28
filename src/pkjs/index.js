// Brolly v2.0.0 — pkjs/index.js
// PebbleKit JS: weather fetch (Open-Meteo) + config bridge

'use strict';

// ─────────────────────────────────────────────────────────────────────────────
// Icon code constants (must match C defines)
// ─────────────────────────────────────────────────────────────────────────────
var ICON = {
  UNKNOWN:          0,
  UNDEFINED:        1,
  CLEAR:            2,
  CLEAR_N:          3,
  PARTLY_CLOUDY:    4,
  PARTLY_CLOUDY_N:  5,
  MOSTLY_CLOUDY:    6,
  MOSTLY_CLOUDY_N:  7,
  CLOUDY:           8,
  CLOUDY_N:         9,
  CHANCE_FLURRIES:  10,
  FLURRIES:         11,
  CHANCE_FLURRIES_N:12,
  FLURRIES_N:       13,
  CHANCE_RAIN:      14,
  RAIN:             15,
  CHANCE_RAIN_N:    16,
  RAIN_N:           17,
  CHANCE_SLEET:     18,
  SLEET:            19,
  CHANCE_SLEET_N:   20,
  SLEET_N:          21,
  CHANCE_SNOW:      22,
  SNOW:             23,
  CHANCE_SNOW_N:    24,
  SNOW_N:           25,
  CHANCE_TSTORMS:   26,
  TSTORMS:          27,
  CHANCE_TSTORMS_N: 28,
  TSTORMS_N:        29,
  FOG:              30,
  HAZE:             31,
  FOG_N:            32,
  HAZE_N:           33
};

// ─────────────────────────────────────────────────────────────────────────────
// WMO weather code → icon code mapping
// ─────────────────────────────────────────────────────────────────────────────
function wmoToIcon(code, isDay) {
  var d = isDay ? 1 : 0;
  switch (code) {
    case 0:  return d ? ICON.CLEAR        : ICON.CLEAR_N;
    case 1:  return d ? ICON.PARTLY_CLOUDY: ICON.PARTLY_CLOUDY_N;
    case 2:  return d ? ICON.MOSTLY_CLOUDY: ICON.MOSTLY_CLOUDY_N;
    case 3:  return d ? ICON.CLOUDY       : ICON.CLOUDY_N;
    case 45: case 48: return d ? ICON.FOG : ICON.FOG_N;
    case 51: case 53: case 55:
    case 56: case 57:
    case 61: case 63: case 65:
    case 66: case 67:
    case 80: case 81: case 82:
      return d ? ICON.RAIN : ICON.RAIN_N;
    case 71: case 73: case 75:
    case 77:
    case 85: case 86:
      return d ? ICON.SNOW : ICON.SNOW_N;
    case 95: case 96: case 99:
      return d ? ICON.TSTORMS : ICON.TSTORMS_N;
    default: return ICON.UNKNOWN;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Stored settings
// ─────────────────────────────────────────────────────────────────────────────
var s_customLocation = '';
var s_useLatLon = false;
var s_storedLat = null;
var s_storedLon = null;

// ─────────────────────────────────────────────────────────────────────────────
// Geocoding helper
// ─────────────────────────────────────────────────────────────────────────────
function geocodeCity(cityName, callback) {
  var url = 'https://geocoding-api.open-meteo.com/v1/search?name=' +
            encodeURIComponent(cityName) + '&count=1&language=en&format=json';
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.onload = function() {
    if (xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        if (data.results && data.results.length > 0) {
          callback(null, data.results[0].latitude, data.results[0].longitude);
        } else {
          callback('No results for: ' + cityName);
        }
      } catch (e) {
        callback('Geocode parse error: ' + e);
      }
    } else {
      callback('Geocode HTTP error: ' + xhr.status);
    }
  };
  xhr.onerror = function() { callback('Geocode network error'); };
  xhr.send();
}

// ─────────────────────────────────────────────────────────────────────────────
// IP geolocation fallback
// ─────────────────────────────────────────────────────────────────────────────
function ipGeolocate(callback) {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', 'https://ipapi.co/json/', true);
  xhr.onload = function() {
    if (xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        callback(null, data.latitude, data.longitude);
      } catch (e) {
        callback('IP geo parse error: ' + e);
      }
    } else {
      callback('IP geo HTTP error: ' + xhr.status);
    }
  };
  xhr.onerror = function() { callback('IP geo network error'); };
  xhr.send();
}

// ─────────────────────────────────────────────────────────────────────────────
// Fetch weather from Open-Meteo and send to watch
// ─────────────────────────────────────────────────────────────────────────────
function fetchWeather(lat, lon) {
  var tz = Intl.DateTimeFormat().resolvedOptions().timeZone || 'auto';
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat +
    '&longitude=' + lon +
    '&hourly=weather_code,is_day' +
    '&current=temperature_2m' +
    '&daily=sunrise,sunset' +
    '&forecast_days=2' +
    '&timezone=' + encodeURIComponent(tz);

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.onload = function() {
    if (xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        processWeatherData(data);
      } catch (e) {
        console.log('Weather parse error: ' + e);
      }
    } else {
      console.log('Weather HTTP error: ' + xhr.status);
    }
  };
  xhr.onerror = function() { console.log('Weather network error'); };
  xhr.send();
}

function processWeatherData(data) {
  // Build 24-hour icon array
  var icons = new Array(24);
  var now = new Date();
  var curHour = now.getHours();

  // Find today's start index in hourly data
  // hourly.time is array of ISO strings like "2024-01-01T00:00"
  var times = data.hourly.time;
  var codes = data.hourly.weather_code;
  var isDays = data.hourly.is_day;

  // Find the index corresponding to today 00:00
  var todayStr = now.toISOString().slice(0, 10); // "YYYY-MM-DD"
  var startIdx = 0;
  for (var i = 0; i < times.length; i++) {
    if (times[i].slice(0, 10) === todayStr) {
      startIdx = i;
      break;
    }
  }

  for (var h = 0; h < 24; h++) {
    var idx = startIdx + h;
    if (idx < codes.length) {
      icons[h] = wmoToIcon(codes[idx], isDays[idx] === 1);
    } else {
      icons[h] = ICON.UNKNOWN;
    }
  }

  // Current temperature
  var tempC = Math.round(data.current.temperature_2m);
  var tempF = Math.round(tempC * 9 / 5 + 32);

  // Sunrise / sunset (today)
  var sunriseStr = data.daily.sunrise[0]; // "YYYY-MM-DDTHH:MM"
  var sunsetStr  = data.daily.sunset[0];
  var srParts = sunriseStr.split('T')[1].split(':');
  var ssParts = sunsetStr.split('T')[1].split(':');
  var srHour = parseInt(srParts[0], 10);
  var srMin  = parseInt(srParts[1], 10);
  var ssHour = parseInt(ssParts[0], 10);
  var ssMin  = parseInt(ssParts[1], 10);

  // Build message
  var msg = {};
  for (var k = 0; k < 24; k++) {
    msg['KEY_ICON_' + k] = icons[k];
  }
  msg.KEY_TEMP_C       = tempC;
  msg.KEY_TEMP_F       = tempF;
  msg.KEY_SUNRISE_HOUR = srHour;
  msg.KEY_SUNRISE_MINUTE = srMin;
  msg.KEY_SUNSET_HOUR  = ssHour;
  msg.KEY_SUNSET_MINUTE = ssMin;

  Pebble.sendAppMessage(msg, function() {
    console.log('Weather sent to watch');
  }, function(e) {
    console.log('Weather send failed: ' + JSON.stringify(e));
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Location resolution
// ─────────────────────────────────────────────────────────────────────────────
function resolveLocation(callback) {
  // 1. Stored GPS coords
  if (s_useLatLon && s_storedLat !== null && s_storedLon !== null) {
    callback(null, s_storedLat, s_storedLon);
    return;
  }

  // 2. Custom location string
  if (s_customLocation && s_customLocation.trim().length > 0) {
    var loc = s_customLocation.trim();
    // Check if it's "lat,lon"
    var parts = loc.split(',');
    if (parts.length === 2 && !isNaN(parseFloat(parts[0])) && !isNaN(parseFloat(parts[1]))) {
      callback(null, parseFloat(parts[0]), parseFloat(parts[1]));
    } else {
      geocodeCity(loc, callback);
    }
    return;
  }

  // 3. GPS via navigator.geolocation
  if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      function(pos) {
        s_storedLat = pos.coords.latitude;
        s_storedLon = pos.coords.longitude;
        s_useLatLon = true;
        callback(null, s_storedLat, s_storedLon);
      },
      function(err) {
        console.log('GPS error: ' + err.message + ', falling back to IP');
        // 4. IP fallback
        ipGeolocate(callback);
      },
      { timeout: 15000 }
    );
  } else {
    // 4. IP fallback
    ipGeolocate(callback);
  }
}

function doWeatherFetch() {
  resolveLocation(function(err, lat, lon) {
    if (err) {
      console.log('Location error: ' + err);
      return;
    }
    fetchWeather(lat, lon);
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Config / settings bridge
// ─────────────────────────────────────────────────────────────────────────────
function sendSettingsToWatch(settings) {
  var msg = {};

  // Map each setting key to its numeric message key
  var keyMap = {
    KEY_DISPLAY_HOUR_MARKERS:    40,
    KEY_DISPLAY_MINOR_MARKERS:   41,
    KEY_BT_DISCONNECT_MIN_INNER_RED: 53,
    KEY_VIBRATE_BT_DISCONNECT:   54,
    KEY_VIBRATE_BT_RECONNECT:    55,
    KEY_SHAKE_MODE:              107,
    KEY_TEMP_UNIT:               110,
    KEY_HOUR_HAND_OUTER:         114,
    KEY_HOUR_HAND_INNER:         115,
    KEY_MIN_HAND_OUTER:          116,
    KEY_MIN_HAND_INNER:          117,
    KEY_DATE_VISIBLE:            118,
    KEY_TEMP_VISIBLE:            119,
    KEY_NUMBER_FONT:             121,
    KEY_BACKGROUND_COLOR:        126,
    KEY_NUMBER_COLOR:            127,
    KEY_ICON_COLOR:              128,
    KEY_HOUR_MARKER_COLOR:       129,
    KEY_MINUTE_MARKER_COLOR:     130,
    KEY_DATE_COLOR:              134,
    KEY_TEMP_COLOR:              135,
    KEY_BT_DISCONNECT_OUTER_COLOR: 136,
    KEY_BT_DISCONNECT_INNER_COLOR: 137,
    KEY_BATTERY_RING_THRESHOLD:  138,
    KEY_BATTERY_CENTER_THRESHOLD:139,
    KEY_SECONDS_HAND_COLOR:      141,
    KEY_SECONDS_HAND_MODE:       142,
    KEY_SECONDS_SHAKE_DUR:       143,
    KEY_SUNRISE_MARKER_VISIBLE:  147,
    KEY_SUNRISE_MARKER_COLOR:    148,
    KEY_SUNSET_MARKER_COLOR:     149,
    KEY_NUMBER_SIZE:             150,
    KEY_ICON_SIZE:               151,
    KEY_ICON_COLOR_MODE:         153,
    KEY_DISPLAY_MODE:            158
  };

  Object.keys(keyMap).forEach(function(key) {
    if (settings.hasOwnProperty(key)) {
      msg[key] = parseInt(settings[key], 10);
    }
  });

  // Custom location: store locally for weather fetch (trimmed).
  // The actual fetch is triggered from webviewclosed after this returns.
  if (settings.KEY_CUSTOM_LOCATION !== undefined) {
    s_customLocation = String(settings.KEY_CUSTOM_LOCATION).trim();
  }

  // Test buttons — send directly without saving
  if (settings.KEY_TEST_BATTERY_ALERT) {
    Pebble.sendAppMessage({ KEY_TEST_BATTERY_ALERT: 1 });
    return;
  }
  if (settings.KEY_TEST_BT_DISCONNECT) {
    Pebble.sendAppMessage({ KEY_TEST_BT_DISCONNECT: 1 });
    return;
  }
  if (settings.KEY_TEST_CRITICAL_BATTERY_ALERT) {
    Pebble.sendAppMessage({ KEY_TEST_CRITICAL_BATTERY_ALERT: 1 });
    return;
  }

  if (Object.keys(msg).length > 0) {
    Pebble.sendAppMessage(msg, function() {
      console.log('Settings sent to watch');
    }, function(e) {
      console.log('Settings send failed: ' + JSON.stringify(e));
    });
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Pebble event handlers
// ─────────────────────────────────────────────────────────────────────────────
Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready');
  doWeatherFetch();
});

Pebble.addEventListener('appmessage', function(e) {
  // Watch requesting weather refresh (not used in this version)
  console.log('AppMessage from watch: ' + JSON.stringify(e.payload));
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Build config URL with current settings as query params
  // The settings page reads these on load
  var configUrl = 'https://themott27.github.io/Test_Brolly_v2_Settings/';
  Pebble.openURL(configUrl);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response && e.response.length > 0) {
    try {
      var payload = JSON.parse(decodeURIComponent(e.response));

      // Update custom location BEFORE calling sendSettingsToWatch so that
      // s_customLocation is already set when doWeatherFetch runs.
      if (payload.KEY_CUSTOM_LOCATION !== undefined) {
        s_customLocation = String(payload.KEY_CUSTOM_LOCATION).trim();
        s_useLatLon = false;  // discard any cached GPS coords
        s_storedLat = null;
        s_storedLon = null;
      }

      sendSettingsToWatch(payload);

      // Always re-fetch weather after settings close so a new city name
      // (or clearing the field to revert to GPS) takes effect immediately.
      doWeatherFetch();

    } catch (err) {
      console.log('Config parse error: ' + err);
    }
  }
});
