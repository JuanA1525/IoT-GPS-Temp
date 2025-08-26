#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <ClosedCube_HDC1080.h>

// ---------- CONFIG PINES ----------
static const int RXPin = 2, TXPin = 0;
static const uint32_t GPSBaud = 9600;

// ---------- OBJETOS ----------
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
ClosedCube_HDC1080 sensor;  // Sensor HDC1080 por I2C (dirección 0x40)

// ---------- ESTADOS ----------
enum State { STATE_CAPTURE, STATE_PRUNING, STATE_BUNDLING };
State currentState = STATE_CAPTURE;

unsigned long lastChange = 0;
const unsigned long interval = 2000; // 2 segundos por estado, 2000 ms

// ---------- VARIABLES ----------
void capture();
void pruning();
void bundling();
float temp_raw = 0.0, hum_raw = 0.0;
double lat_raw = 0.0, lon_raw = 0.0;

float temperatura = 0.0, humedad = 0.0;
double lat = 0.0, lon = 0.0;


void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  Wire.begin();

  sensor.begin(0x40); // dirección del HDC1080
  Serial.println("(GPS + HDC1080) listO!");
}

void loop() {
  // Control de la máquina de estados
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
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available()) {
      gps.encode(ss.read());
    }
  } while (millis() - start < ms);
  Serial.println("[SMART DELAY] Procesando GPS...");
}
// Captura de datos crudos
void capture() {
  smartDelay(0);
  lat_raw = gps.location.lat();
  lon_raw = gps.location.lng();
  temp_raw = sensor.readTemperature();
  hum_raw  = sensor.readHumidity();

  Serial.println("[CAPTURE] Datos brutos obtenidos");
}

// Limpieza / validación de datos
void pruning() {

  smartDelay(0);
  Serial.println("[PRUNING] Limpiando datos...");

  // Validar temperatura y humedad
  if (isnan(temp_raw)) {
    temperatura = -999; // marcador de error
  } else {
    temperatura = temp_raw;
  }

  if (isnan(hum_raw)) {
    humedad = -999; // marcador de error
  } else {
    humedad = hum_raw;
  }

  // Validar GPS
  if (gps.location.isValid()) {
    lat = lat_raw;
    lon = lon_raw;
  } else {
    lat = -999.0;
    lon = -999.0;
  }
}

// Empaquetado y salida final
void bundling() {
  smartDelay(0);
  Serial.println("[BUNDLING] Empaquetando datos finales...");
  Serial.printf("{\"lat\":%.6f, \"lon\":%.6f, \"temp\":%.2f, \"hum\":%.2f}\n", lat, lon, temperatura, humedad);
}