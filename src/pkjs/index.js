// AWW2 - Analogue Weather Watch 2
// Phone-side JavaScript
// Based on AWW1 by pbhgl

// Weather icon codes (must match main.c)
const ICON_UNKNOWN            = 0;
const ICON_UNDEFINED          = 1;
const ICON_CLEAR              = 2;
const ICON_CLEAR_N            = 3;
const ICON_PARTLY_CLOUDY      = 4;
const ICON_PARTLY_CLOUDY_N    = 5;
const ICON_MOSTLY_CLOUDY      = 6;
const ICON_MOSTLY_CLOUDY_N    = 7;
const ICON_CLOUDY             = 8;
const ICON_CLOUDY_N           = 9;
const ICON_CHANCE_FLURRIES    = 10;
const ICON_FLURRIES           = 11;
const ICON_CHANCE_FLURRIES_N  = 12;
const ICON_FLURRIES_N         = 13;
const ICON_CHANCE_RAIN        = 14;
const ICON_RAIN               = 15;
const ICON_CHANCE_RAIN_N      = 16;
const ICON_RAIN_N             = 17;
const ICON_CHANCE_SLEET       = 18;
const ICON_SLEET              = 19;
const ICON_CHANCE_SLEET_N     = 20;
const ICON_SLEET_N            = 21;
const ICON_CHANCE_SNOW        = 22;
const ICON_SNOW               = 23;
const ICON_CHANCE_SNOW_N      = 24;
const ICON_SNOW_N             = 25;
const ICON_CHANCE_TSTORMS     = 26;
const ICON_TSTORMS            = 27;
const ICON_CHANCE_TSTORMS_N   = 28;
const ICON_TSTORMS_N          = 29;
const ICON_FOG                = 30;
const ICON_HAZE               = 31;
const ICON_FOG_N              = 32;
const ICON_HAZE_N             = 33;

// Message keys (must match package.json)
const KEY_ICON_0              = 0;
const KEY_WEATHER_CITY        = 24;
const KEY_TZ_OFFSET_MIN       = 29;
const KEY_MOON_AGE            = 30;
const KEY_PERCENT_ILLUMINATED = 31;
const KEY_NORTHERN_HEMISPHERE = 32;
const KEY_USE_OM              = 34;
const KEY_USE_LAT_LON         = 35;
const KEY_LATITUDE            = 36;
const KEY_LONGITUDE           = 37;
const KEY_TIMEZONE_NAME       = 43;
const KEY_TEMP_C              = 58;
const KEY_TEMP_F              = 59;
const KEY_JS_READY            = 99;
const KEY_CUSTOM_LOCATION     = 113;
const KEY_ICON_STYLE          = 127;

const MOON_DEFAULT_PERCENT = 20;

let useOM = true;
let useLatLon = false;
let latitude = null;
let longitude = null;
let tzname = "";
let invalidSettings = false;
let newSettings = false;
let customLocation = "";

let xhrRequest = function(url, type, callback) {
  let xhr = new XMLHttpRequest();
  xhr.onload = function() {
    callback(this.responseText);
  };
  xhr.onerror = function() {
    console.log('XHR error for: ' + url);
  };
  xhr.open(type, url);
  xhr.send();
};

function getIconFromData(data) {
  let icon;

  if (useOM) {
    icon = data;
  } else {
    if (!data.next_1_hours ||
        !data.next_1_hours.summary ||
        !data.next_1_hours.summary.symbol_code) {
      return ICON_UNDEFINED;
    }
    icon = data.next_1_hours.summary.symbol_code;
  }

  switch (icon) {
    case '0_day':
    case 'clearsky_day':
    case 'clearsky_polartwilight':
      return ICON_CLEAR;
    case '0_night':
    case 'clearsky_night':
      return ICON_CLEAR_N;
    case '1_day':
    case 'fair_day':
    case 'fair_polartwilight':
      return ICON_PARTLY_CLOUDY;
    case '1_night':
    case 'fair_night':
      return ICON_PARTLY_CLOUDY_N;
    case '2_day':
    case 'partlycloudy_day':
    case 'partlycloudy_polartwilight':
      return ICON_MOSTLY_CLOUDY;
    case '2_night':
    case 'partlycloudy_night':
      return ICON_MOSTLY_CLOUDY_N;
    case '3_day':
    case '3_night':
    case 'cloudy':
      return ICON_CLOUDY;
    case '51_day': case '51_night':
    case '53_day': case '53_night':
    case '55_day': case '55_night':
    case '56_day': case '56_night':
    case '57_day': case '57_night':
    case '61_day': case '61_night':
    case '63_day': case '63_night':
    case '65_day': case '65_night':
    case '66_day': case '66_night':
    case '67_day': case '67_night':
    case '80_day': case '80_night':
    case '81_day': case '81_night':
    case '82_day': case '82_night':
    case 'heavyrainshowers_day':
    case 'heavyrainshowers_polartwilight':
    case 'heavyrain':
    case 'lightrainshowers_day':
    case 'lightrainshowers_polartwilight':
    case 'lightrain':
    case 'rainshowers_day':
    case 'rainshowers_polartwilight':
    case 'rain':
    case 'heavyrainshowers_night':
    case 'lightrainshowers_night':
    case 'rainshowers_night':
      return ICON_RAIN;
    case '95_day': case '95_night':
    case '96_day': case '96_night':
    case '99_day': case '99_night':
    case 'heavyrainandthunder':
    case 'heavyrainshowersandthunder_day':
    case 'heavyrainshowersandthunder_polartwilight':
    case 'lightrainandthunder':
    case 'lightrainshowersandthunder_day':
    case 'lightrainshowersandthunder_polartwilight':
    case 'rainandthunder':
    case 'rainshowersandthunder_day':
    case 'rainshowersandthunder_polartwilight':
    case 'heavysleetandthunder':
    case 'heavysleetshowersandthunder_day':
    case 'heavysleetshowersandthunder_polartwilight':
    case 'lightsleetandthunder':
    case 'lightssleetshowersandthunder_day':
    case 'lightssleetshowersandthunder_polartwilight':
    case 'sleetandthunder':
    case 'sleetshowersandthunder_day':
    case 'sleetshowersandthunder_polartwilight':
    case 'heavysnowandthunder':
    case 'heavysnowshowersandthunder_day':
    case 'heavysnowshowersandthunder_polartwilight':
    case 'lightsnowandthunder':
    case 'lightssnowshowersandthunder_day':
    case 'lightssnowshowersandthunder_polartwilight':
    case 'snowandthunder':
    case 'snowshowersandthunder_day':
    case 'snowshowersandthunder_polartwilight':
    case 'heavyrainshowersandthunder_night':
    case 'lightrainshowersandthunder_night':
    case 'rainshowersandthunder_night':
    case 'heavysleetshowersandthunder_night':
    case 'lightssleetshowersandthunder_night':
    case 'sleetshowersandthunder_night':
    case 'heavysnowshowersandthunder_night':
    case 'lightssnowshowersandthunder_night':
    case 'snowshowersandthunder_night':
      return ICON_TSTORMS;
    case '77_day': case '77_night':
    case 'heavysleetshowers_day':
    case 'heavysleetshowers_polartwilight':
    case 'heavysleet':
    case 'lightsleetshowers_day':
    case 'lightsleetshowers_polartwilight':
    case 'lightsleet':
    case 'sleetshowers_day':
    case 'sleetshowers_polartwilight':
    case 'sleet':
    case 'heavysleetshowers_night':
    case 'lightsleetshowers_night':
    case 'sleetshowers_night':
      return ICON_SLEET;
    case '71_day': case '71_night':
    case '73_day': case '73_night':
    case '75_day': case '75_night':
    case '85_day': case '85_night':
    case '86_day': case '86_night':
    case 'heavysnowshowers_day':
    case 'heavysnowshowers_polartwilight':
    case 'heavysnow':
    case 'lightsnowshowers_day':
    case 'lightsnowshowers_polartwilight':
    case 'lightsnow':
    case 'snowshowers_day':
    case 'snowshowers_polartwilight':
    case 'snow':
    case 'heavysnowshowers_night':
    case 'lightsnowshowers_night':
    case 'snowshowers_night':
      return ICON_SNOW;
    case '45_day': case '45_night':
    case '48_day': case '48_night':
    case 'fog':
      return ICON_FOG;
    default:
      console.log('Icon undefined: ' + icon);
      return ICON_UNDEFINED;
  }
}

function isLatLonValid(lat, lon) {
  if (isNaN(lat) || isNaN(lon)) {
    console.log('error: location undefined for latitude ' + lat + ', longitude ' + lon);
    Pebble.showSimpleNotificationOnPebble("AWW2 Weather Error",
      "Location undefined. Check Settings.");
    invalidSettings = true;
    return false;
  }
  return true;
}

function geocodeCity(cityName, callback) {
  // Use Open-Meteo geocoding API
  let url = 'https://geocoding-api.open-meteo.com/v1/search?name=' +
    encodeURIComponent(cityName) + '&count=1&language=en&format=json';
  xhrRequest(url, 'GET', function(responseText) {
    try {
      let json = JSON.parse(responseText);
      if (json.results && json.results.length > 0) {
        callback(json.results[0].latitude, json.results[0].longitude);
      } else {
        console.log('City not found: ' + cityName);
        Pebble.showSimpleNotificationOnPebble("AWW2 Weather Error",
          "City not found: " + cityName);
      }
    } catch(e) {
      console.log('Geocoding error: ' + e);
    }
  });
}

function callbackWeather(responseText) {
  let json;
  try {
    json = JSON.parse(responseText);
  } catch(e) {
    console.log('Error parsing weather: ' + e);
    return;
  }

  let dictionary;
  if (useOM) {
    dictionary = getDictionaryOM(json);
  } else {
    dictionary = getDictionaryMN(json);
  }

  if (dictionary) {
    sendMessage(dictionary);
  }
}

function getDictionaryOM(json) {
  if (!json.hourly) {
    console.log('no weather data available');
    return null;
  }

  let city = parseFloat(latitude).toFixed(2) + ',' + parseFloat(longitude).toFixed(2);
  let tz_offset_minutes = Math.floor(json.utc_offset_seconds / 60);
  let temp_c = Math.round(json.current.temperature_2m);
  let temp_f = Math.round(temp_c * 9 / 5 + 32);

  let percent = MOON_DEFAULT_PERCENT;
  let percent_illuminated = percent <= 50 ? percent * 2 : (100 - percent) * 2;
  let moon_age = percent * 29.5 / 100;
  let northern_hemisphere = latitude >= 0 ? 1 : 0;

  let icons = new Array(24).fill(-1);

  // Find the current time in local timezone
  const now = new Date();
  const currentHour = now.getHours();
  const currentMin = now.getMinutes();
  const nowMs = now.getTime();

  // Find the starting index in the API data (first forecast >= current time)
  let startIndex = 0;
  for (let i = 0; i < json.hourly.time.length; i++) {
    const dt = new Date(`${json.hourly.time[i]}Z`).getTime() - json.utc_offset_seconds * 1000;
    if (nowMs <= dt) {
      startIndex = i;
      break;
    }
  }

  // Process up to 24 hourly forecasts
  let maxIndex = Math.min(startIndex + 24, json.hourly.weather_code.length);
  
  // Map each forecast to its clock position (0-23)
  // Clock position 0 = 12 AM (hour 0), 1-11 = 1-11 AM/PM, 12-23 = 12 PM - 11 PM
  // For display, we only use positions 0-11 (12 o'clock through 11 o'clock)
  for (let i = startIndex; i < maxIndex; i++) {
    let time = json.hourly.time[i];
    let weather_code = json.hourly.weather_code[i] + (json.hourly.is_day[i] ? '_day' : '_night');
    let forecastHour = new Date(time).getHours();
    
    // Convert 24-hour format to clock position
    // Hour 0 (12 AM) -> position 0
    // Hours 1-11 -> positions 1-11
    // Hours 12-23 -> positions 12-23 (but we only use 0-11 for display)
    let clockPos = forecastHour;
    
    // Store the FIRST (closest) forecast for each hour
    // This ensures we get the next occurrence of each hour
    if (icons[clockPos] === -1) {
      icons[clockPos] = getIconFromData(weather_code);
    }
  }
  
  // For any unfilled positions (hours we don't have data for),
  // try to fill them with the next available forecast for that hour
  for (let clockPos = 0; clockPos < 24; clockPos++) {
    if (icons[clockPos] === -1) {
      for (let i = startIndex; i < maxIndex; i++) {
        let forecastHour = new Date(json.hourly.time[i]).getHours();
        if (forecastHour === clockPos) {
          let weather_code = json.hourly.weather_code[i] + (json.hourly.is_day[i] ? '_day' : '_night');
          icons[clockPos] = getIconFromData(weather_code);
          break;
        }
      }
    }
  }

  let dictionary = {
    KEY_WEATHER_CITY: city,
    KEY_TIMEZONE_NAME: json.timezone,
    KEY_TZ_OFFSET_MIN: tz_offset_minutes,
    KEY_MOON_AGE: moon_age,
    KEY_PERCENT_ILLUMINATED: percent_illuminated,
    KEY_NORTHERN_HEMISPHERE: northern_hemisphere,
    KEY_TEMP_C: temp_c,
    KEY_TEMP_F: temp_f
  };

  for (let i = 0; i < 24; i++) {
    dictionary[KEY_ICON_0 + i] = icons[i];
  }

  return dictionary;
}

function getDictionaryMN(json) {
  if (!json.properties || !json.properties.timeseries || json.properties.timeseries.length === 0) {
    console.log('no weather data available');
    return null;
  }

  let city = parseFloat(latitude).toFixed(2) + ',' + parseFloat(longitude).toFixed(2);
  let tz_offset_minutes = -new Date().getTimezoneOffset();
  let temp_c = Math.round(json.properties.timeseries[0].data.instant.details.air_temperature);
  let temp_f = Math.round(temp_c * 9 / 5 + 32);

  let percent = MOON_DEFAULT_PERCENT;
  let percent_illuminated = percent <= 50 ? percent * 2 : (100 - percent) * 2;
  let moon_age = percent * 29.5 / 100;
  let northern_hemisphere = latitude >= 0 ? 1 : 0;

  let icons = new Array(24).fill(-1);
  let maxIndex = Math.min(24, json.properties.timeseries.length);
  
  // Map each forecast to its clock position (0-23)
  // Clock position 0 = 12 AM (hour 0), 1-11 = 1-11 AM/PM, 12-23 = 12 PM - 11 PM
  for (let i = 0; i < maxIndex; i++) {
    let time = json.properties.timeseries[i].time;
    let data = json.properties.timeseries[i].data;
    let forecastHour = new Date(time).getHours();
    let clockPos = forecastHour;
    
    // Store the FIRST (closest) forecast for each hour
    if (icons[clockPos] === -1) {
      icons[clockPos] = getIconFromData(data);
    }
  }
  
  // For any unfilled positions, try to fill them
  for (let clockPos = 0; clockPos < 24; clockPos++) {
    if (icons[clockPos] === -1) {
      for (let i = 0; i < maxIndex; i++) {
        let forecastHour = new Date(json.properties.timeseries[i].time).getHours();
        if (forecastHour === clockPos) {
          icons[clockPos] = getIconFromData(json.properties.timeseries[i].data);
          break;
        }
      }
    }
  }

  let dictionary = {
    KEY_WEATHER_CITY: city,
    KEY_TIMEZONE_NAME: "",
    KEY_TZ_OFFSET_MIN: tz_offset_minutes,
    KEY_MOON_AGE: moon_age,
    KEY_PERCENT_ILLUMINATED: percent_illuminated,
    KEY_NORTHERN_HEMISPHERE: northern_hemisphere,
    KEY_TEMP_C: temp_c,
    KEY_TEMP_F: temp_f
  };

  for (let i = 0; i < 24; i++) {
    dictionary[KEY_ICON_0 + i] = icons[i];
  }

  return dictionary;
}

function sendMessage(dictionary) {
  Pebble.sendAppMessage(dictionary,
    function(data) {
      console.log("AWW2: Weather sent to Pebble successfully!");
    },
    function(data, error) {
      console.log("AWW2: Error sending weather: " + JSON.stringify(data) + ", " + error);
    }
  );
}

function requestWeather(lat, lon) {
  if (invalidSettings) return;
  if (!isLatLonValid(lat, lon)) return;

  lat = '' + lat;
  lon = '' + lon;
  if (lat.indexOf('.') === -1) lat += '.0';
  if (lon.indexOf('.') === -1) lon += '.0';

  let url = '';
  let timezone = (typeof tzname === "string" && tzname.indexOf('/') > 0) ? tzname : "auto";

  if (useOM) {
    url = `https://api.open-meteo.com/v1/forecast` +
      `?latitude=${lat}` +
      `&longitude=${lon}` +
      `&hourly=weather_code,is_day` +
      `&current=temperature_2m` +
      `&forecast_days=2` +
      `&timezone=${timezone}`;
  } else {
    url = `https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=${lat}&lon=${lon}`;
  }

  xhrRequest(url, 'GET', callbackWeather);
}

function locationSuccess(pos) {
  latitude = pos.coords.latitude.toFixed(4);
  longitude = pos.coords.longitude.toFixed(4);

  let configData = JSON.parse(localStorage.getItem("aww2ConfigData")) || {};
  configData.latitude = latitude;
  configData.longitude = longitude;
  localStorage.setItem("aww2ConfigData", JSON.stringify(configData));

  requestWeather(latitude, longitude);
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);

  if (latitude && longitude) {
    if (newSettings) {
      Pebble.showSimpleNotificationOnPebble("AWW2 Location Error",
        "Failed to determine location. Using previous location.");
    }
    requestWeather(latitude, longitude);
  } else {
    // Fallback: use IP-based geolocation
    console.log('Falling back to IP-based geolocation...');
    getLocationFromIP();
  }
}

function getLocationFromIP() {
  // Use IP-based geolocation as fallback when GPS fails
  xhrRequest('https://ipapi.co/json/', 'GET', function(responseText) {
    try {
      let json = JSON.parse(responseText);
      if (json.latitude && json.longitude) {
        latitude = parseFloat(json.latitude).toFixed(4);
        longitude = parseFloat(json.longitude).toFixed(4);
        console.log('IP geolocation: ' + latitude + ', ' + longitude);
        requestWeather(latitude, longitude);
      } else {
        console.log('IP geolocation failed: no coordinates');
      }
    } catch(e) {
      console.log('IP geolocation error: ' + e);
    }
  });
}

function getWeather(e) {
  if (useLatLon && latitude && longitude) {
    requestWeather(latitude, longitude);
  } else if (customLocation && customLocation.trim().length > 0) {
    // Check if it's lat,lon coordinates
    let parts = customLocation.split(',');
    if (parts.length === 2 && !isNaN(parseFloat(parts[0])) && !isNaN(parseFloat(parts[1]))) {
      latitude = parseFloat(parts[0]).toFixed(4);
      longitude = parseFloat(parts[1]).toFixed(4);
      requestWeather(latitude, longitude);
    } else {
      // Geocode city name
      geocodeCity(customLocation.trim(), function(lat, lon) {
        latitude = lat.toFixed(4);
        longitude = lon.toFixed(4);
        requestWeather(latitude, longitude);
      });
    }
  } else {
    navigator.geolocation.getCurrentPosition(
      locationSuccess,
      locationError,
      {timeout: 15000, maximumAge: 60000}
    );
  }
  newSettings = false;
}

function getConfigValues(configData) {
  if (configData.hasOwnProperty("useLatLon")) {
    useLatLon = configData.useLatLon;
  }
  if (useLatLon) {
    if (configData.hasOwnProperty("latitude")) {
      latitude = parseFloat(configData.latitude).toFixed(4);
    }
    if (configData.hasOwnProperty("longitude")) {
      longitude = parseFloat(configData.longitude).toFixed(4);
    }
  }
  if (configData.hasOwnProperty("useOM")) {
    useOM = configData.useOM;
  }
  if (configData.hasOwnProperty("customLocation")) {
    customLocation = configData.customLocation;
  }
}

// Settings key maps — defined once, used by sendSavedSettings and webviewclosed.
const SETTINGS_KEY_MAP = {
  'KEY_DISPLAY_HOUR_MARKERS':        40,
  'KEY_DISPLAY_MINOR_MARKERS':       41,
  'KEY_BT_DISCONNECT_MIN_INNER_RED': 53,
  'KEY_VIBRATE_BT_DISCONNECT':       54,
  'KEY_VIBRATE_BT_RECONNECT':        55,
  'KEY_SHAKE_MODE':                 107,
  'KEY_TEMP_UNIT':                  110,
  'KEY_DATE_VISIBLE':               118,
  'KEY_TEMP_VISIBLE':               119,
  'KEY_WEATHER_SERVICE':            120,
  'KEY_NUMBER_FONT':                121,
  'KEY_BATTERY_INDICATOR_ENABLED':  138,
  'KEY_SECONDS_HAND_MODE':          142,
  'KEY_SECONDS_SHAKE_DUR':          143
};

const SETTINGS_COLOR_MAP = {
  'KEY_HOUR_HAND_OUTER':            114,
  'KEY_HOUR_HAND_INNER':            115,
  'KEY_MIN_HAND_OUTER':             116,
  'KEY_MIN_HAND_INNER':             117,
  'KEY_BACKGROUND_COLOR':           126,
  'KEY_NUMBER_COLOR':               127,
  'KEY_ICON_COLOR':                 128,
  'KEY_HOUR_MARKER_COLOR':          129,
  'KEY_MINUTE_MARKER_COLOR':        130,
  'KEY_CENTER_DOT_50_COLOR':        131,
  'KEY_CENTER_DOT_20_COLOR':        132,
  'KEY_MIDDLE_RING_20_COLOR':       133,
  'KEY_DATE_COLOR':                 134,
  'KEY_TEMP_COLOR':                 135,
  'KEY_BT_DISCONNECT_OUTER_COLOR':  136,
  'KEY_BT_DISCONNECT_INNER_COLOR':  137,
  'KEY_SECONDS_HAND_COLOR':         141
};

// Re-sends all saved settings from localStorage to the watch.
function sendSavedSettings(configData) {
  let dict = {};
  let hasAny = false;

  Object.keys(SETTINGS_KEY_MAP).forEach(function(name) {
    if (configData.hasOwnProperty(name)) {
      dict[SETTINGS_KEY_MAP[name]] = parseInt(configData[name]);
      hasAny = true;
    }
  });
  Object.keys(SETTINGS_COLOR_MAP).forEach(function(name) {
    if (configData.hasOwnProperty(name)) {
      dict[SETTINGS_COLOR_MAP[name]] = parseInt(configData[name]);
      hasAny = true;
    }
  });

  if (hasAny) {
    Pebble.sendAppMessage(dict,
      function() { console.log('AWW2: Saved settings restored to watch'); },
      function(e) { console.log('AWW2: Failed to restore settings: ' + JSON.stringify(e)); }
    );
  }
}

function init() {
  invalidSettings = false;

  // Load persisted config values (location, useOM, etc.)
  let configData = JSON.parse(localStorage.getItem("aww2ConfigData")) || {};
  getConfigValues(configData);

  // Re-send saved settings to watch immediately so they are applied
  // even after a watchface reinstall that cleared the watch's persist storage.
  sendSavedSettings(configData);

  // Fetch weather immediately on load with a short timeout.
  // If GPS fails or takes too long, IP geolocation will kick in as fallback.
  getWeather(null);
}

Pebble.addEventListener('ready', init);

// When app message connection opens (watch connects to phone),
// immediately send saved settings so they load even on first install.
Pebble.addEventListener('appmessage', function(e) {
  // This fires when the watch sends a message, but we use it as a signal
  // that the connection is active. Send settings proactively.
  let configData = JSON.parse(localStorage.getItem("aww2ConfigData")) || {};
  if (Object.keys(configData).length > 0) {
    sendSavedSettings(configData);
  }
});

Pebble.addEventListener('showConfiguration', function() {
  // Use the stored URL (which includes all saved settings as query params)
  // so the page reopens pre-populated with the user's last choices.
  // Falls back to the base URL on first open.
  var baseUrl = 'https://aww2setts-au3w7dkw.manus.space/';
  var stored = null;
  try {
    var cd = JSON.parse(localStorage.getItem('aww2ConfigData'));
    if (cd && cd.configureUrl) stored = cd.configureUrl;
  } catch(e) {}
  Pebble.openURL(stored || baseUrl);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) return;

  let response;
  try {
    response = JSON.parse(decodeURIComponent(e.response));
  } catch(err) {
    console.log('Brolly: Error parsing config response: ' + err);
    return;
  }

  let configData = JSON.parse(localStorage.getItem("aww2ConfigData")) || {};

  // Build numeric-keyed dict for sendAppMessage
  let dict = {};

  Object.keys(SETTINGS_KEY_MAP).forEach(function(name) {
    if (response.hasOwnProperty(name)) {
      let val = parseInt(response[name]);
      dict[SETTINGS_KEY_MAP[name]] = val;
      configData[name] = val;
    }
  });

  Object.keys(SETTINGS_COLOR_MAP).forEach(function(name) {
    if (response.hasOwnProperty(name)) {
      let val = parseInt(response[name]);
      dict[SETTINGS_COLOR_MAP[name]] = val;
      configData[name] = val;
    }
  });

  // Custom location — store for weather fetch, not sent to watch directly
  if (response.hasOwnProperty('KEY_CUSTOM_LOCATION')) {
    customLocation = response['KEY_CUSTOM_LOCATION'] || '';
    configData.customLocation = customLocation;
  }

  // Weather service — update useOM flag (KEY_USE_OM = 34)
  if (response.hasOwnProperty('KEY_WEATHER_SERVICE')) {
    useOM = parseInt(response['KEY_WEATHER_SERVICE']) === 0;  // 0=OpenMeteo, 1=Pebble native
    configData.useOM = useOM;
    dict[KEY_USE_OM] = useOM ? 1 : 0;
  }

  localStorage.setItem("aww2ConfigData", JSON.stringify(configData));

  // Send settings to watch
  Pebble.sendAppMessage(dict,
    function(e) { console.log('Brolly: Config sent to watch'); },
    function(e) { console.log('Brolly: Failed to send config: ' + JSON.stringify(e)); }
  );

  // Refresh weather with new settings
  newSettings = true;
  getWeather(null);
});
