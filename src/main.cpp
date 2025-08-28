#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// ---------- CONFIG WIFI ----------
const char* ssid = "Juan celular";
const char* password = "juancho9";
const char* serverURL = "http://98.85.128.81/update_data";

// ---------- CONFIG PINES ----------
static const int RXPin = 2, TXPin = 0;
static const uint32_t GPSBaud = 9600;
static const int MEASURE_COUNT = 10;

// ---------- OBJETOS ----------
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
ClosedCube_HDC1080 sensor;
WiFiClient wifiClient;
HTTPClient http;

// ---------- ESTADOS ----------
enum State { STATE_CAPTURE, STATE_PRUNING, STATE_BUNDLING };
State currentState = STATE_CAPTURE;

// ---------- VARIABLES ----------
void capture();
void pruning();
void bundling();
void sendDataToServer();
unsigned long calculateHash(String data);

float temp_vector[MEASURE_COUNT];
float hum_vector[MEASURE_COUNT];
float temperatura = 0.0, humedad = 0.0;
double lat = 0.0, lon = 0.0;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 10000;

// ---------- CIFRADO (COMENTADO) ----------
/*
const char encryptionKey[] = "K3Y$3CR3T"; // Clave de 9 bytes
const int keyLength = 9;

// Función de cifrado comentada
void encryptData(char* data, int length) {
  for(int i = 0; i < length; i++) {
    data[i] = data[i] ^ encryptionKey[i % keyLength];
  }
}
*/

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  Wire.begin();
  sensor.begin(0x40);

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Serial.println("(GPS + HDC1080 + WiFi) listo!");
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

  // Enviar datos cada 10 segundos
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendDataToServer();
    lastSendTime = millis();
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
  // Serial.println("\n[CAPTURE] Iniciando captura de datos...");
  // Serial.println("----------------------------------------");

  for(int i = 0; i < MEASURE_COUNT; i++) {
    hum_vector[i] = sensor.readHumidity();
    // Serial.printf("Humedad[%d]: %.2f%%\n", i, hum_vector[i]);
    smartDelay(10);

    temp_vector[i] = sensor.readTemperature();
    // Serial.printf("Temperatura[%d]: %.2f°C\n", i, temp_vector[i]);
    smartDelay(10);

    // Serial.println("----------------------------------------");
  }

  // Serial.println("[CAPTURE] Datos brutos capturados exitosamente");
}

// Promediado de medidas
void pruning() {
  smartDelay(0);
  // Serial.println("\n[PRUNING] Iniciando procesamiento de promedios...");
  // Serial.println("----------------------------------------");

  float sum_temp = 0.0;
  float sum_hum = 0.0;
  int valid_temp = 0;
  int valid_hum = 0;

  // Serial.println("Analizando medidas válidas:");
  for(int i = 0; i < MEASURE_COUNT; i++) {
    if(!isnan(temp_vector[i])) {
      sum_temp += temp_vector[i];
      valid_temp++;
      // Serial.printf("Temperatura[%d]: %.2f°C (válida)\n", i, temp_vector[i]);
    } else {
      // Serial.printf("Temperatura[%d]: Inválida\n", i);
    }

    if(!isnan(hum_vector[i])) {
      sum_hum += hum_vector[i];
      valid_hum++;
      // Serial.printf("Humedad[%d]: %.2f%% (válida)\n", i, hum_vector[i]);
    } else {
      // Serial.printf("Humedad[%d]: Inválida\n", i);
    }
    // Serial.println("----------------------------------------");
  }

  // Calculo de promedios con validación
  temperatura = valid_temp > 0 ? sum_temp / valid_temp : -999.0;
  humedad = valid_hum > 0 ? sum_hum / valid_hum : -999.0;

  // Serial.println("\nResultados del procesamiento:");
  // Serial.printf("- Medidas válidas de temperatura: %d/%d\n", valid_temp, MEASURE_COUNT);
  // Serial.printf("- Medidas válidas de humedad: %d/%d\n", valid_hum, MEASURE_COUNT);
  // Serial.printf("- Temperatura promedio: %.2f°C\n", temperatura);
  // Serial.printf("- Humedad promedio: %.2f%%\n", humedad);
  // Serial.println("----------------------------------------");
}

// Función para calcular hash simple
unsigned long calculateHash(String data) {
  unsigned long hash = 0;
  for (int i = 0; i < data.length(); i++) {
    hash = hash * 31 + data.charAt(i);
  }
  return hash;
}

void sendDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi no conectado, no se puede enviar datos");
    return;
  }

  double send_lat = (lat != -999.0) ? lat : 0.0;
  double send_lon = (lon != -999.0) ? lon : 0.0;

  String dataForHash = "point07" + String(send_lat, 9) + String(send_lon, 9) +
                       String(temperatura, 2) + String(humedad, 0);

  unsigned long dataHash = calculateHash(dataForHash);

  String jsonPayload = "{\"id\":\"point07\",\"lat\":" + String(send_lat, 9) +
                       ",\"lon\":" + String(send_lon, 9) +
                       ",\"temperatura\":" + String(temperatura, 2) +
                       ",\"humedad\":" + String(humedad, 0) +
                       ",\"metadata\":{\"hash\":\"" + String(dataHash) + "\"}}";

  Serial.println("\n============ ENVÍO DE DATOS ============");
  Serial.printf("Temperatura: %.1f°C | Humedad: %.0f%%\n", temperatura, humedad);
  Serial.printf("Coordenadas: %.6f, %.6f\n", send_lat, send_lon);
  Serial.printf("GPS Status: %s\n", (lat != -999.0) ? "VÁLIDO" : "SIN FIX");
  Serial.printf("Hash: %lu\n", dataHash);
  Serial.println("JSON: " + jsonPayload);

  http.begin(wifiClient, serverURL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("✓ Enviado exitosamente (Código: %d)\n", httpResponseCode);
    Serial.println("Respuesta: " + response);
  } else {
    Serial.printf("✗ Error en envío (Código: %d)\n", httpResponseCode);
  }
  Serial.println("========================================\n");

  http.end();
}

// Empaquetado y salida final con GPS
void bundling() {
  smartDelay(0);
  // Serial.println("\n[BUNDLING] Empaquetando datos finales...");
  // Serial.println("----------------------------------------");

  // Serial.println("Diagnóstico GPS:");
  // Serial.println("----------------------------------------");

  // Estado general del GPS
  // Serial.printf("- Satélites conectados: %d\n", gps.satellites.value());
  // Serial.printf("- Precisión (HDOP): %.2f\n", gps.hdop.hdop());
  // Serial.printf("- Altura sobre nivel del mar: %.2f metros\n", gps.altitude.meters());
  // Serial.printf("- Velocidad: %.2f km/h\n", gps.speed.kmph());
  // Serial.printf("- Curso: %.2f°\n", gps.course.deg());

  // Fecha y hora
  // if (gps.date.isValid() && gps.time.isValid()) {
  //   Serial.printf("- Fecha/Hora: %02d/%02d/%d %02d:%02d:%02d UTC\n",
  //                 gps.date.day(), gps.date.month(), gps.date.year(),
  //                 gps.time.hour(), gps.time.minute(), gps.time.second());
  // } else {
  //   Serial.println("- Fecha/Hora: No disponible");
  // }

  // Estado de las coordenadas
  // Serial.println("\nEstado de coordenadas:");
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
    // Serial.println("- Estado: Válidas");
    // Serial.printf("- Latitud: %.6f\n", lat);
    // Serial.printf("- Longitud: %.6f\n", lon);
    // Serial.printf("- Edad de los datos: %lu ms\n", gps.location.age());
  } else {
    lat = -999.0;
    lon = -999.0;
    // Serial.println("- Estado: NO VÁLIDAS");
    // Serial.println("- Causa posible: Sin fix GPS");
  }

  // Diagnóstico de sensores
  // Serial.println("\nDiagnóstico de sensores HDC1080:");
  // Serial.printf("- Temperatura: %.2f°C\n", temperatura);
  // Serial.printf("- Humedad: %.2f%%\n", humedad);

  /*
  // Código de cifrado comentado
  // Crear el buffer para el JSON
  char jsonBuffer[256]; // Ajustar tamaño según necesidades
  int length = snprintf(jsonBuffer, sizeof(jsonBuffer),
    "{\"gps\":{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f,\"sat\":%d,\"hdop\":%.2f},\"sensors\":{\"temp\":%.2f,\"hum\":%.2f}}",
    lat, lon, gps.altitude.meters(), gps.satellites.value(),
    gps.hdop.hdop(), temperatura, humedad);

  // Mostrar datos sin cifrar (para debug)
  Serial.println("\nDatos sin cifrar:");
  Serial.println(jsonBuffer);

  // Cifrar datos
  encryptData(jsonBuffer, length);

  // Mostrar datos cifrados en hexadecimal
  Serial.println("\nDatos cifrados (hex):");
  for(int i = 0; i < length; i++) {
    Serial.printf("%02X", (unsigned char)jsonBuffer[i]);
  }
  */

  // Serial.println("----------------------------------------\n");
}