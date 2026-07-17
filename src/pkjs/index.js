// Brolly v2.0.0 — pkjs/index.js
// PebbleKit JS: weather fetch (Open-Meteo) + config bridge
//
// IMPORTANT: Pebble.sendAppMessage() requires NUMERIC keys only.
// String key names are NOT accepted. All message keys are mapped to their
// numeric IDs from package.json messageKeys.

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
// Numeric message key IDs (from package.json messageKeys)
// ALL Pebble.sendAppMessage calls must use these numbers, not string names.
// ─────────────────────────────────────────────────────────────────────────────
var KEY = {
  // Weather / icons (0–23 = icon slots)
  ICON_0:  0,  ICON_1:  1,  ICON_2:  2,  ICON_3:  3,
  ICON_4:  4,  ICON_5:  5,  ICON_6:  6,  ICON_7:  7,
  ICON_8:  8,  ICON_9:  9,  ICON_10: 10, ICON_11: 11,
  ICON_12: 12, ICON_13: 13, ICON_14: 14, ICON_15: 15,
  ICON_16: 16, ICON_17: 17, ICON_18: 18, ICON_19: 19,
  ICON_20: 20, ICON_21: 21, ICON_22: 22, ICON_23: 23,
  SUNRISE_HOUR:   25,
  SUNRISE_MINUTE: 26,
  SUNSET_HOUR:    27,
  SUNSET_MINUTE:  28,
  // Alerts / BT
  BT_DISCONNECT_MIN_INNER_RED: 53,
  VIBRATE_BT_DISCONNECT:       54,
  VIBRATE_BT_RECONNECT:        55,
  // Temperature
  TEMP_C: 58,
  TEMP_F: 59,
  // Settings
  SHAKE_MODE:              107,
  TEMP_UNIT:               110,
  HOUR_HAND_OUTER:         114,
  HOUR_HAND_INNER:         115,
  MIN_HAND_OUTER:          116,
  MIN_HAND_INNER:          117,
  DATE_VISIBLE:            118,
  TEMP_VISIBLE:            119,
  NUMBER_FONT:             121,
  BACKGROUND_COLOR:        126,
  NUMBER_COLOR:            127,
  ICON_COLOR:              128,
  HOUR_MARKER_COLOR:       129,
  MINUTE_MARKER_COLOR:     130,
  DATE_COLOR:              134,
  TEMP_COLOR:              135,
  BT_DISCONNECT_OUTER_COLOR: 136,
  BT_DISCONNECT_INNER_COLOR: 137,
  BATTERY_RING_THRESHOLD:  138,
  BATTERY_CENTER_THRESHOLD:139,
  SECONDS_HAND_COLOR:      141,
  SECONDS_HAND_MODE:       142,
  SECONDS_SHAKE_DUR:       143,
  // Test buttons
  TEST_BATTERY_ALERT:          144,
  TEST_BT_DISCONNECT:          145,
  TEST_CRITICAL_BATTERY_ALERT: 146,
  // Sunrise/sunset markers
  SUNRISE_MARKER_VISIBLE: 147,
  SUNRISE_MARKER_COLOR:   148,
  SUNSET_MARKER_COLOR:    149,
  // Appearance
  NUMBER_SIZE:    150,
  ICON_SIZE:      151,
  ICON_COLOR_MODE:153,
  DISPLAY_MODE:   158,
  // City name display
  CITY_NAME:         159,
  CITY_DISPLAY_MODE: 160,
  CITY_COLOR:        161,
  COMPLICATION_LAYER:162,
  // Markers
  DISPLAY_HOUR_MARKERS:  40,
  DISPLAY_MINOR_MARKERS: 41
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
var s_resolvedCityName = '';
var s_weatherIntervalId = null;
var s_currentWeatherInterval = 30; // default 30 mins

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
          var r = data.results[0];
          s_resolvedCityName = r.name || cityName;
          callback(null, r.latitude, r.longitude);
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

// Reverse geocode lat/lon to city name using Nominatim (OpenStreetMap)
function reverseGeocode(lat, lon, callback) {
  var url = 'https://nominatim.openstreetmap.org/reverse?lat=' +
            lat + '&lon=' + lon + '&format=json&zoom=10&accept-language=en';
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.setRequestHeader('Accept-Language', 'en');
  xhr.onload = function() {
    if (xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        var addr = data.address || {};
        var city = addr.city || addr.town || addr.village || addr.hamlet || addr.county || '';
        callback(null, city);
      } catch (e) {
        callback(null, '');
      }
    } else {
      callback(null, '');
    }
  };
  xhr.onerror = function() { callback(null, ''); };
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

  // Find today's start index in hourly data
  var times = data.hourly.time;
  var codes = data.hourly.weather_code;
  var isDays = data.hourly.is_day;

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

  // Build message using NUMERIC keys only
  var msg = {};
  for (var k = 0; k < 24; k++) {
    msg[k] = icons[k]; // KEY_ICON_0..23 = numeric keys 0..23
  }
  msg[KEY.TEMP_C]          = tempC;
  msg[KEY.TEMP_F]          = tempF;
  msg[KEY.SUNRISE_HOUR]    = srHour;
  msg[KEY.SUNRISE_MINUTE]  = srMin;
  msg[KEY.SUNSET_HOUR]     = ssHour;
  msg[KEY.SUNSET_MINUTE]   = ssMin;
  // Send city name if we have one
  if (s_resolvedCityName !== undefined) {
    msg[KEY.CITY_NAME] = s_resolvedCityName.substring(0, 31);
  }

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
      // Use cached geocode result if available for the same city
      if (s_storedLat !== null && s_storedLon !== null && s_useLatLon) {
        callback(null, s_storedLat, s_storedLon);
      } else {
        geocodeCity(loc, function(err, lat, lon) {
          if (!err) {
            // Cache the geocoded coordinates to avoid re-geocoding every 30 min
            s_storedLat = lat;
            s_storedLon = lon;
            s_useLatLon = true;
          }
          callback(err, lat, lon);
        });
      }
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
        // Reverse geocode to get city name
        reverseGeocode(s_storedLat, s_storedLon, function(err2, cityName) {
          if (cityName) { s_resolvedCityName = cityName; } else { s_resolvedCityName = ""; }
          callback(null, s_storedLat, s_storedLon);
        });
      },
      function(err) {
        console.log('GPS error: ' + err.message + ', falling back to IP');
        // 4. IP fallback
        ipGeolocate(function(err2, lat, lon) {
          if (!err2) {
            reverseGeocode(lat, lon, function(err3, cityName) {
              if (cityName) { s_resolvedCityName = cityName; } else { s_resolvedCityName = ""; }
              callback(null, lat, lon);
            });
          } else {
            callback(err2);
          }
        });
      },
      { timeout: 15000 }
    );
  } else {
    // 4. IP fallback
    ipGeolocate(function(err2, lat, lon) {
      if (!err2) {
        reverseGeocode(lat, lon, function(err3, cityName) {
          if (cityName) { s_resolvedCityName = cityName; } else { s_resolvedCityName = ""; }
          callback(null, lat, lon);
        });
      } else {
        callback(err2);
      }
    });
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

function updateWeatherInterval(mins) {
  if (mins < 15) mins = 30; // safety minimum
  if (mins === s_currentWeatherInterval && s_weatherIntervalId !== null) return;
  console.log('Updating weather interval to ' + mins + ' minutes');
  s_currentWeatherInterval = mins;
  if (s_weatherIntervalId !== null) {
    clearInterval(s_weatherIntervalId);
  }
  s_weatherIntervalId = setInterval(doWeatherFetch, mins * 60 * 1000);
}

// ─────────────────────────────────────────────────────────────────────────────
// Config / settings bridge
// ─────────────────────────────────────────────────────────────────────────────
function sendSettingsToWatch(settings) {
  // Test buttons: send ONLY the test key using its numeric ID, then return.
  // These must be checked FIRST before building the regular settings message.
  if (settings.KEY_TEST_BATTERY_ALERT) {
    var msg144 = {};
    msg144[KEY.TEST_BATTERY_ALERT] = 1;
    Pebble.sendAppMessage(msg144, function() {
      console.log('Test battery alert sent');
    }, function(e) {
      console.log('Test battery alert failed: ' + JSON.stringify(e));
    });
    return;
  }
  if (settings.KEY_TEST_BT_DISCONNECT) {
    var msg145 = {};
    msg145[KEY.TEST_BT_DISCONNECT] = 1;
    Pebble.sendAppMessage(msg145, function() {
      console.log('Test BT disconnect sent');
    }, function(e) {
      console.log('Test BT disconnect failed: ' + JSON.stringify(e));
    });
    return;
  }
  if (settings.KEY_TEST_CRITICAL_BATTERY_ALERT) {
    var msg146 = {};
    msg146[KEY.TEST_CRITICAL_BATTERY_ALERT] = 1;
    Pebble.sendAppMessage(msg146, function() {
      console.log('Test critical battery alert sent');
    }, function(e) {
      console.log('Test critical battery alert failed: ' + JSON.stringify(e));
    });
    return;
  }

  // Regular settings: build numeric-keyed message
  var msg = {};

  // Map string setting names → numeric key IDs
  var keyMap = {
    KEY_BT_DISCONNECT_MIN_INNER_RED: KEY.BT_DISCONNECT_MIN_INNER_RED,
    KEY_VIBRATE_BT_DISCONNECT:       KEY.VIBRATE_BT_DISCONNECT,
    KEY_VIBRATE_BT_RECONNECT:        KEY.VIBRATE_BT_RECONNECT,
    KEY_SHAKE_MODE:                  KEY.SHAKE_MODE,
    KEY_TEMP_UNIT:                   KEY.TEMP_UNIT,
    KEY_HOUR_HAND_OUTER:             KEY.HOUR_HAND_OUTER,
    KEY_HOUR_HAND_INNER:             KEY.HOUR_HAND_INNER,
    KEY_MIN_HAND_OUTER:              KEY.MIN_HAND_OUTER,
    KEY_MIN_HAND_INNER:              KEY.MIN_HAND_INNER,
    KEY_DATE_VISIBLE:                KEY.DATE_VISIBLE,
    KEY_TEMP_VISIBLE:                KEY.TEMP_VISIBLE,
    KEY_NUMBER_FONT:                 KEY.NUMBER_FONT,
    KEY_BACKGROUND_COLOR:            KEY.BACKGROUND_COLOR,
    KEY_NUMBER_COLOR:                KEY.NUMBER_COLOR,
    KEY_ICON_COLOR:                  KEY.ICON_COLOR,
    KEY_HOUR_MARKER_COLOR:           KEY.HOUR_MARKER_COLOR,
    KEY_MINUTE_MARKER_COLOR:         KEY.MINUTE_MARKER_COLOR,
    KEY_DATE_COLOR:                  KEY.DATE_COLOR,
    KEY_TEMP_COLOR:                  KEY.TEMP_COLOR,
    KEY_BT_DISCONNECT_OUTER_COLOR:   KEY.BT_DISCONNECT_OUTER_COLOR,
    KEY_BT_DISCONNECT_INNER_COLOR:   KEY.BT_DISCONNECT_INNER_COLOR,
    KEY_BATTERY_RING_THRESHOLD:      KEY.BATTERY_RING_THRESHOLD,
    KEY_BATTERY_CENTER_THRESHOLD:    KEY.BATTERY_CENTER_THRESHOLD,
    KEY_SECONDS_HAND_COLOR:          KEY.SECONDS_HAND_COLOR,
    KEY_SECONDS_HAND_MODE:           KEY.SECONDS_HAND_MODE,
    KEY_SECONDS_SHAKE_DUR:           KEY.SECONDS_SHAKE_DUR,
    KEY_SUNRISE_MARKER_VISIBLE:      KEY.SUNRISE_MARKER_VISIBLE,
    KEY_SUNRISE_MARKER_COLOR:        KEY.SUNRISE_MARKER_COLOR,
    KEY_SUNSET_MARKER_COLOR:         KEY.SUNSET_MARKER_COLOR,
    KEY_NUMBER_SIZE:                 KEY.NUMBER_SIZE,
    KEY_ICON_SIZE:                   KEY.ICON_SIZE,
    KEY_ICON_COLOR_MODE:             KEY.ICON_COLOR_MODE,
    KEY_DISPLAY_MODE:                KEY.DISPLAY_MODE,
    KEY_CITY_DISPLAY_MODE:           KEY.CITY_DISPLAY_MODE,
    KEY_CITY_COLOR:                  KEY.CITY_COLOR,
    KEY_COMPLICATION_LAYER:          KEY.COMPLICATION_LAYER,
    KEY_DISPLAY_HOUR_MARKERS:        KEY.DISPLAY_HOUR_MARKERS,
    KEY_DISPLAY_MINOR_MARKERS:       KEY.DISPLAY_MINOR_MARKERS
  };

  Object.keys(keyMap).forEach(function(strKey) {
    if (settings.hasOwnProperty(strKey)) {
      msg[keyMap[strKey]] = parseInt(settings[strKey], 10);
    }
  });

    // Custom location: store locally for weather fetch (trimmed).
  if (settings.KEY_CUSTOM_LOCATION !== undefined) {
    s_customLocation = String(settings.KEY_CUSTOM_LOCATION).trim();
  }
  // Weather interval: store locally (not sent to watch, JS-only setting).
  if (settings.KEY_WEATHER_INTERVAL !== undefined) {
    var interval = parseInt(settings.KEY_WEATHER_INTERVAL, 10);
    localStorage.setItem('weather_interval', interval);
    updateWeatherInterval(interval);
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
  // Load stored interval if available
  var storedInterval = localStorage.getItem('weather_interval');
  if (storedInterval) {
    s_currentWeatherInterval = parseInt(storedInterval, 10);
  }
  doWeatherFetch();
  updateWeatherInterval(s_currentWeatherInterval);
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage from watch: ' + JSON.stringify(e.payload));
});

Pebble.addEventListener('showConfiguration', function(e) {
  var configUrl = 'https://themott27.github.io/Test_Brolly_v2_Settings/?v=' + Date.now();
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
        s_resolvedCityName = "";
        s_useLatLon = false;  // discard any cached GPS coords
        s_storedLat = null;
        s_storedLon = null;
      }

      // Check if this is a test-button-only payload — if so, don't re-fetch weather.
      var isTestAction = payload.KEY_TEST_BATTERY_ALERT ||
                         payload.KEY_TEST_BT_DISCONNECT ||
                         payload.KEY_TEST_CRITICAL_BATTERY_ALERT;

      sendSettingsToWatch(payload);

      // Only re-fetch weather when weather-relevant settings changed
      // (location, temp unit, interval), not for visual-only changes.
      var weatherRelevant = payload.KEY_CUSTOM_LOCATION !== undefined ||
                            payload.KEY_TEMP_UNIT !== undefined ||
                            payload.KEY_WEATHER_INTERVAL !== undefined;
      if (!isTestAction && weatherRelevant) {
        if (payload.KEY_WEATHER_INTERVAL !== undefined) {
          var newInterval = parseInt(payload.KEY_WEATHER_INTERVAL, 10);
          localStorage.setItem('weather_interval', newInterval);
          updateWeatherInterval(newInterval);
        }
        doWeatherFetch();
      }

    } catch (err) {
      console.log('Config parse error: ' + err);
    }
  }
});
