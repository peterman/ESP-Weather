void setsensor() {
  // weather monitoring
    Serial.println("-- Weather Station Scenario --");
    Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
    Serial.println("filter off");
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
                      
    // suggested rate is 1/60Hz (1m)
    delayTime = 60000; // in milliseconds
}



// Relative to absolute humidity
// Based on https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
float absoluteHumidity(float temperature, float humidity) {
  return (13.2471*pow(EULER,17.67*temperature/(temperature+243.5))*humidity/(cToKOffset+temperature));
}

// Calculate saturation vapor pressure
// Based on dew.js, Copyright 2011 Wolfgang Kuehn, Apache License 2.0
float saturationVaporPressure(float temperature) {
  if(temperature < 173 || temperature > 678) return -112; //Temperature out of range

  float svp = 0;
  if(temperature <= cToKOffset) {
    /**
      * -100-0°C -> Saturation vapor pressure over ice
      * ITS-90 Formulations by Bob Hardy published in 
      * "The Proceedings of the Third International 
      * Symposium on Humidity & Moisture",
      * Teddington, London, England, April 1998
      */

    svp = exp(-5.8666426e3/temperature + 2.232870244e1 + (1.39387003e-2 + (-3.4262402e-5 + (2.7040955e-8*temperature)) * temperature) * temperature + 6.7063522e-1 * log(temperature)); 
  }else{
    /**
      * 0°C-400°C -> Saturation vapor pressure over water
      * IAPWS Industrial Formulation 1997
      * for the Thermodynamic Properties of Water and Steam
      * by IAPWS (International Association for the Properties
      * of Water and Steam), Erlangen, Germany, September 1997.
      * Equation 30 in Section 8.1 "The Saturation-Pressure 
      * Equation (Basic Equation)"
      */

    const float th = temperature + -0.23855557567849 / (temperature - 0.65017534844798e3);
    const float a  = (th + 0.11670521452767e4) * th + -0.72421316703206e6;
    const float b  = (-0.17073846940092e2 * th + 0.12020824702470e5) * th + -0.32325550322333e7;
    const float c  = (0.14915108613530e2 * th + -0.48232657361591e4) * th + 0.40511340542057e6;

    svp = 2 * c / (-b + sqrt(b * b - 4 * a * c));
    svp *= svp;
    svp *= svp;
    svp *= 1e6;
  }
  
  yield();

  return svp;
}


// Calculate dew point in °C
// Based on dew.js, Copyright 2011 Wolfgang Kuehn, Apache License 2.0
float dewPoint(float temperature, float humidity)
{
  temperature += cToKOffset; //Celsius to Kelvin

  if(humidity < 0 || humidity > 100) return -111; //Invalid humidity
  if(temperature < 173 || temperature > 678) return -112; //Temperature out of range

  humidity = humidity / 100 * saturationVaporPressure(temperature);
  
  byte mc = 10;

  float xNew;
  float dx;
  float z;

  do {
    dx = temperature / 1000;
    z = saturationVaporPressure(temperature);
    xNew = temperature + dx * (humidity - z) / (saturationVaporPressure(temperature + dx) - z);
    if (abs((xNew - temperature) / xNew) < 0.0001) {
        return xNew - cToKOffset;
    }
    temperature = xNew;
    mc--;
  } while(mc > 0);

  return -113; //Solver did not get a close result
}

void getvalues() {
  bme.takeForcedMeasurement();
  temperature = bme.readTemperature();
  humidity_r = bme.readHumidity();
  humidity = absoluteHumidity(temperature, humidity_r);
  dewpoint = dewPoint(temperature,humidity_r);
  pressure = bme.readPressure() / 100.0F;
  pressure_r = bme.seaLevelForAltitude(ALTITUDE, pressure);
  
  
}
