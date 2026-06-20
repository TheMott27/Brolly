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
const KEY_SUNRISE_HOUR        = 25;
const KEY_SUNRISE_MINUTE      = 26;
const KEY_SUNSET_HOUR         = 27;
const KEY_SUNSET_MINUTE       = 28;
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
const KEY_SUNRISE_MARKER_VISIBLE = 147;
const KEY_SUNRISE_MARKER_COLOR   = 148;
const KEY_SUNSET_MARKER_COLOR    = 149;

const MOON_DEFAULT_PERCENT = 20;

let useOM = true;
let useLatLon = false;
let latitude = null;
let longitude = null;
let tzname = "";
let invalidSettings = false;
let newSettings = false;
let customLocation = "";
let weatherService = 0;  // 0=Open-Meteo, 1=Open Weather Map, 2=Native Pebble
let owmApiKey = "";

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

function callbackWeatherOWMWithOM(owmData, omData) {
  // Merge OWM weather icons with Open-Meteo sunrise/sunset
  try {
    let owmJson = JSON.parse(owmData);
    let omJson = JSON.parse(omData);
    
    // Get OWM dictionary (weather icons)
    let dictionary = getDictionaryOWM(owmJson);
    
    if (dictionary && omJson.daily && omJson.daily.sunrise && omJson.daily.sunset) {
      // Extract sunrise/sunset from Open-Meteo
      function parseHHMM(isoStr) {
        let tPart = isoStr.split('T')[1] || '00:00';
        let parts = tPart.split(':');
        return { h: parseInt(parts[0], 10), m: parseInt(parts[1], 10) };
      }
      
      let sunrise_hour = null, sunrise_min = null, sunset_hour = null, sunset_min = null;
      const now = new Date();
      const nowMinutes = now.getHours() * 60 + now.getMinutes();
      
      if (omJson.daily.sunrise && omJson.daily.sunrise.length > 0) {
        let sr = parseHHMM(omJson.daily.sunrise[0]);
        let srMin = sr.h * 60 + sr.m;
        if (srMin < nowMinutes && omJson.daily.sunrise.length > 1) {
          sr = parseHHMM(omJson.daily.sunrise[1]);
        }
        sunrise_hour = sr.h;
        sunrise_min = sr.m;
      }
      if (omJson.daily.sunset && omJson.daily.sunset.length > 0) {
        let ss = parseHHMM(omJson.daily.sunset[0]);
        let ssMin = ss.h * 60 + ss.m;
        if (ssMin < nowMinutes && omJson.daily.sunset.length > 1) {
          ss = parseHHMM(omJson.daily.sunset[1]);
        }
        sunset_hour = ss.h;
        sunset_min = ss.m;
      }
      
      // Add sunrise/sunset to dictionary only if found
      if (sunrise_hour !== null && sunrise_min !== null) {
        dictionary[KEY_SUNRISE_HOUR] = sunrise_hour;
        dictionary[KEY_SUNRISE_MINUTE] = sunrise_min;
      }
      if (sunset_hour !== null && sunset_min !== null) {
        dictionary[KEY_SUNSET_HOUR] = sunset_hour;
        dictionary[KEY_SUNSET_MINUTE] = sunset_min;
      }
    }
    
    if (dictionary) {
      sendMessage(dictionary);
    }
  } catch (e) {
    console.log('Error merging OWM+OM data: ' + e);
  }
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
  if (weatherService === 0) {
    dictionary = getDictionaryOM(json);
  } else if (weatherService === 1) {
    // For OWM: use OWM for weather icons, but we'll fetch OM for sunrise/sunset
    dictionary = getDictionaryOWM(json);
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

  // Parse sunrise/sunset from daily data (format: "2026-06-02T05:23", local time).
  // Parse manually to avoid JS Date() treating the string as UTC in some environments.
  // Use tomorrow's value when today's event has already passed.
  function parseHHMM(isoStr) {
    // isoStr = "2026-06-02T05:23" — extract HH and MM from the time part
    let tPart = isoStr.split('T')[1] || '00:00';
    let parts = tPart.split(':');
    return { h: parseInt(parts[0], 10), m: parseInt(parts[1], 10) };
  }
  let sunrise_hour = null, sunrise_min = null, sunset_hour = null, sunset_min = null;
  const nowMinutes = now.getHours() * 60 + now.getMinutes();
  if (json.daily && json.daily.sunrise && json.daily.sunrise.length > 0) {
    let sr = parseHHMM(json.daily.sunrise[0]);
    let srMin = sr.h * 60 + sr.m;
    // If today's sunrise has already passed and tomorrow's data exists, use tomorrow's
    if (srMin < nowMinutes && json.daily.sunrise.length > 1) {
      sr = parseHHMM(json.daily.sunrise[1]);
    }
    sunrise_hour = sr.h;
    sunrise_min  = sr.m;
  }
  if (json.daily && json.daily.sunset && json.daily.sunset.length > 0) {
    let ss = parseHHMM(json.daily.sunset[0]);
    let ssMin = ss.h * 60 + ss.m;
    // If today's sunset has already passed and tomorrow's data exists, use tomorrow's
    if (ssMin < nowMinutes && json.daily.sunset.length > 1) {
      ss = parseHHMM(json.daily.sunset[1]);
    }
    sunset_hour = ss.h;
    sunset_min  = ss.m;
  }

  let dictionary = {
    KEY_WEATHER_CITY: city,
    KEY_TIMEZONE_NAME: json.timezone,
    KEY_TZ_OFFSET_MIN: tz_offset_minutes,
    KEY_MOON_AGE: moon_age,
    KEY_PERCENT_ILLUMINATED: percent_illuminated,
    KEY_NORTHERN_HEMISPHERE: northern_hemisphere,
    KEY_TEMP_C: temp_c,
    KEY_TEMP_F: temp_f,
  };
  
  // Only add sunrise/sunset if actual data was found
  if (sunrise_hour !== null && sunrise_min !== null) {
    dictionary[KEY_SUNRISE_HOUR] = sunrise_hour;
    dictionary[KEY_SUNRISE_MINUTE] = sunrise_min;
  }
  if (sunset_hour !== null && sunset_min !== null) {
    dictionary[KEY_SUNSET_HOUR] = sunset_hour;
    dictionary[KEY_SUNSET_MINUTE] = sunset_min;
  }

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

function getDictionaryOWM(json) {
  if (!json.list || json.list.length === 0) {
    console.log('no weather data available from OWM');
    return null;
  }

  let city = json.city ? json.city.name : (parseFloat(latitude).toFixed(2) + ',' + parseFloat(longitude).toFixed(2));
  let tz_offset_minutes = json.city ? Math.round(json.city.timezone / 60) : -new Date().getTimezoneOffset();
  let temp_c = Math.round(json.list[0].main.temp);
  let temp_f = Math.round(temp_c * 9 / 5 + 32);

  let percent = MOON_DEFAULT_PERCENT;
  let percent_illuminated = percent <= 50 ? percent * 2 : (100 - percent) * 2;
  let moon_age = percent * 29.5 / 100;
  let northern_hemisphere = latitude >= 0 ? 1 : 0;

  // Note: sunrise/sunset is now fetched from Open-Meteo in callbackWeatherOWMWithOM
  // Do not extract from OWM response as it doesn't provide reliable daily sunrise/sunset data

  let icons = new Array(24).fill(-1);

  // OWM provides 3-hourly forecasts, map them to 24-hour positions
  for (let i = 0; i < json.list.length; i++) {
    let forecastTime = new Date(json.list[i].dt * 1000);
    let forecastHour = forecastTime.getHours();
    let clockPos = forecastHour;

    // Store the FIRST (closest) forecast for each hour
    if (icons[clockPos] === -1) {
      if (json.list[i].weather && json.list[i].weather.length > 0) {
        let icon = json.list[i].weather[0].main;
        let isDay = json.list[i].sys && json.list[i].sys.pod === 'd';
        let owmIcon = mapOWMIcon(icon, isDay);
        icons[clockPos] = owmIcon;
      }
    }
  }

  // For any unfilled positions, try to fill them
  for (let clockPos = 0; clockPos < 24; clockPos++) {
    if (icons[clockPos] === -1) {
      for (let i = 0; i < json.list.length; i++) {
        let forecastTime = new Date(json.list[i].dt * 1000);
        let forecastHour = forecastTime.getHours();
        if (forecastHour === clockPos) {
          if (json.list[i].weather && json.list[i].weather.length > 0) {
            let icon = json.list[i].weather[0].main;
            let isDay = json.list[i].sys && json.list[i].sys.pod === 'd';
            let owmIcon = mapOWMIcon(icon, isDay);
            icons[clockPos] = owmIcon;
            break;
          }
        }
      }
    }
  }

  let dictionary = {
    KEY_WEATHER_CITY: city,
    KEY_TIMEZONE_NAME: json.city ? json.city.timezone : "",
    KEY_TZ_OFFSET_MIN: tz_offset_minutes,
    KEY_MOON_AGE: moon_age,
    KEY_PERCENT_ILLUMINATED: percent_illuminated,
    KEY_NORTHERN_HEMISPHERE: northern_hemisphere,
    KEY_TEMP_C: temp_c,
    KEY_TEMP_F: temp_f
  };
  
  // Only add sunrise/sunset if actual data was found
  if (sunrise_hour !== null && sunrise_min !== null) {
    dictionary[KEY_SUNRISE_HOUR] = sunrise_hour;
    dictionary[KEY_SUNRISE_MINUTE] = sunrise_min;
  }
  if (sunset_hour !== null && sunset_min !== null) {
    dictionary[KEY_SUNSET_HOUR] = sunset_hour;
    dictionary[KEY_SUNSET_MINUTE] = sunset_min;
  }

  for (let i = 0; i < 24; i++) {
    dictionary[KEY_ICON_0 + i] = icons[i];
  }

  return dictionary;
}

function mapOWMIcon(owmIcon, isDay) {
  // Map OpenWeatherMap main weather conditions to our icon constants
  console.log('mapOWMIcon: ' + owmIcon + ', isDay: ' + isDay);
  switch (owmIcon) {
    case 'Clear':
      return isDay ? ICON_CLEAR : ICON_CLEAR_N;
    case 'Clouds':
      return isDay ? ICON_MOSTLY_CLOUDY : ICON_MOSTLY_CLOUDY_N;
    case 'Drizzle':
      return isDay ? ICON_CHANCE_RAIN : ICON_CHANCE_RAIN_N;
    case 'Rain':
      return isDay ? ICON_RAIN : ICON_RAIN_N;
    case 'Thunderstorm':
      return isDay ? ICON_TSTORMS : ICON_TSTORMS_N;
    case 'Snow':
      return isDay ? ICON_SNOW : ICON_SNOW_N;
    case 'Mist':
    case 'Smoke':
    case 'Haze':
    case 'Dust':
    case 'Fog':
    case 'Sand':
    case 'Ash':
    case 'Squall':
    case 'Tornado':
      return isDay ? ICON_FOG : ICON_FOG_N;
    default:
      console.log('OWM icon undefined: ' + owmIcon);
      return ICON_UNKNOWN;
  }
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

  // Always use Open-Meteo
  url = `https://api.open-meteo.com/v1/forecast` +
    `?latitude=${lat}` +
    `&longitude=${lon}` +
    `&hourly=weather_code,is_day` +
    `&current=temperature_2m` +
    `&daily=sunrise,sunset` +
    `&forecast_days=2` +
    `&timezone=${timezone}`;

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
      {timeout: 15000, maximumAge: 300000}
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
  if (configData.hasOwnProperty("KEY_WEATHER_SERVICE")) {
    weatherService = parseInt(configData.KEY_WEATHER_SERVICE);
  }
  if (configData.hasOwnProperty("KEY_OWM_API_KEY")) {
    owmApiKey = configData.KEY_OWM_API_KEY || "";
  }
  // Legacy support: useOM flag for backward compatibility
  if (configData.hasOwnProperty("useOM")) {
    useOM = configData.useOM;
    weatherService = useOM ? 0 : 2;
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
  'KEY_NUMBER_WEIGHT':              155,
  'KEY_BATTERY_RING_THRESHOLD':     138,
  'KEY_BATTERY_CENTER_THRESHOLD':   139,
  'KEY_SECONDS_HAND_MODE':          142,
  'KEY_SECONDS_SHAKE_DUR':          143,
  'KEY_TEST_BATTERY_ALERT':         144,
  'KEY_TEST_BT_DISCONNECT':         145,
  'KEY_TEST_BATTERY_50':            146,
  'KEY_SUNRISE_MARKER_VISIBLE':     147,
  'KEY_SUNRISE_MARKER_COLOR':       148,
  'KEY_SUNSET_MARKER_COLOR':        149,
  'KEY_NUMBER_SIZE':                150,
  'KEY_ICON_SIZE':                  151,
  'KEY_NUMBER_COLOR_MODE':          152,
  'KEY_ICON_COLOR_MODE':            153,
  'KEY_WU_API_KEY':                 154,
  'KEY_OWM_API_KEY':                156,
  'KEY_NUMBER_LAYOUT':               158,
  'KEY_DEBUG_SHOW_LINES':            159,
  'KEY_DEBUG_SHOW_ICON_BOXES':       160,
  'KEY_DEBUG_SHOW_PINK_LINES':       161
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
  'KEY_SECONDS_HAND_COLOR':         141,
  'KEY_SUNRISE_MARKER_COLOR':       148,
  'KEY_SUNSET_MARKER_COLOR':        149
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

// Note: appmessage listener removed for battery optimisation.
// Settings are already sent on 'ready' event (init). Re-sending on every
// watch→phone message caused unnecessary Bluetooth traffic.

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

  // Test commands: send only the test key to the watch, don't save settings
  var TEST_KEYS = {
    'KEY_TEST_BATTERY_ALERT': 144,
    'KEY_TEST_BT_DISCONNECT': 145,
    'KEY_TEST_BATTERY_50':    146
  };
  for (var tk in TEST_KEYS) {
    if (response.hasOwnProperty(tk) && parseInt(response[tk]) === 1) {
      var testDict = {};
      testDict[TEST_KEYS[tk]] = 1;
      Pebble.sendAppMessage(testDict,
        function() { console.log('Brolly: Test command sent'); },
        function(err) { console.log('Brolly: Test command failed: ' + JSON.stringify(err)); }
      );
      return;  // Don't process as a settings save
    }
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

  // Update all location/service variables from the new response so getWeather
  // uses the correct values (custom location, lat/lon, useOM) immediately.
  getConfigValues(response);

  // Persist location fields that getConfigValues may have updated
  if (response.hasOwnProperty('KEY_CUSTOM_LOCATION')) {
    customLocation = response['KEY_CUSTOM_LOCATION'] || '';
    configData.customLocation = customLocation;
    // If a custom location is provided, disable useLatLon so getWeather uses it
    if (customLocation.trim().length > 0) {
      useLatLon = false;
      configData.useLatLon = false;
    }
  }
  if (response.hasOwnProperty('useLatLon')) {
    configData.useLatLon = useLatLon;
  }
  if (response.hasOwnProperty('latitude')) {
    configData.latitude = latitude;
  }
  if (response.hasOwnProperty('longitude')) {
    configData.longitude = longitude;
  }

  // Weather service — update weather service type (0=Open-Meteo, 1=OWM, 2=Pebble native)
  if (response.hasOwnProperty('KEY_WEATHER_SERVICE')) {
    weatherService = parseInt(response['KEY_WEATHER_SERVICE']);
    configData.KEY_WEATHER_SERVICE = weatherService;
    // Legacy: set useOM for backward compatibility
    useOM = weatherService === 0;
    configData.useOM = useOM;
    dict[KEY_USE_OM] = useOM ? 1 : 0;
  }
  
  // Open Weather Map API key
  if (response.hasOwnProperty('KEY_OWM_API_KEY')) {
    owmApiKey = response['KEY_OWM_API_KEY'] || "";
    configData.KEY_OWM_API_KEY = owmApiKey;
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
