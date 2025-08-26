#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <ClosedCube_HDC1080.h>

// ---------- CONFIG PINES ----------
static const int RXPin = 2, TXPin = 0;
static const uint32_t GPSBaud = 9600;
static const int MEASURE_COUNT = 10;

// ---------- OBJETOS ----------
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
ClosedCube_HDC1080 sensor;

// ---------- ESTADOS ----------
enum State { STATE_CAPTURE, STATE_PRUNING, STATE_BUNDLING };
State currentState = STATE_CAPTURE;

// ---------- VARIABLES ----------
void capture();
void pruning();
void bundling();

float temp_vector[MEASURE_COUNT];
float hum_vector[MEASURE_COUNT];
float temperatura = 0.0, humedad = 0.0;
double lat = 0.0, lon = 0.0;


void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  Wire.begin();
  sensor.begin(0x40);
  Serial.println("(GPS + HDC1080) listo!");
}

void loop() {
  switch (currentState) {
    case STATE_CAPTURE:
      capture();
      currentState = STATE_PRUNING;
      break;
    case STATE_PRUNING:
      pruning();
      currentState = STATE_BUNDLING;
      break;
    case STATE_BUNDLING:
      bundling();
      currentState = STATE_CAPTURE;
      break;
  }
}

// ---------- FUNCIONES ----------

// Procesa datos del GPS sin bloquear
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available()) {
      gps.encode(ss.read());
    }
  } while (millis() - start < ms);
}

// Captura de datos crudos en vectores
void capture() {
  smartDelay(0);
  Serial.println("[CAPTURE] Capturando datos...");
  
  for(int i = 0; i < MEASURE_COUNT; i++) {
    hum_vector[i] = sensor.readHumidity();
    smartDelay(10); // Delay entre medidas de humedad y temperatura
    temp_vector[i] = sensor.readTemperature();
    smartDelay(10); // Delay entre iteraciones
  }
  
  Serial.println("[CAPTURE] Datos brutos obtenidos");
}

// Promediado de medidas
void pruning() {
  smartDelay(0);
  Serial.println("[PRUNING] Procesando promedios...");
  
  float sum_temp = 0.0;
  float sum_hum = 0.0;
  int valid_temp = 0;
  int valid_hum = 0;
  
  for(int i = 0; i < MEASURE_COUNT; i++) {
    if(!isnan(temp_vector[i])) {
      sum_temp += temp_vector[i];
      valid_temp++;
    }
    if(!isnan(hum_vector[i])) {
      sum_hum += hum_vector[i];
      valid_hum++;
    }
  }
  
  // Calculo de promedios con validación
  temperatura = valid_temp > 0 ? sum_temp / valid_temp : -999.0;
  humedad = valid_hum > 0 ? sum_hum / valid_hum : -999.0;
}

// Empaquetado y salida final con GPS
void bundling() {
  smartDelay(0);
  Serial.println("[BUNDLING] Empaquetando datos finales...");
  
  // Captura de GPS solo en el bundling
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
  } else {
    lat = -999.0;
    lon = -999.0;
  }
  
  Serial.printf("{\"lat\":%.6f, \"lon\":%.6f, \"temp\":%.2f, \"hum\":%.2f}\n", 
                lat, lon, temperatura, humedad);
}