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
const char* serverURL = "http://10.17.172.70:1010/update_data";

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
unsigned long lastBundlingTime = 0;
const unsigned long BUNDLING_INTERVAL = 10000;

// ---------- CIFRADO ----------
const char encryptionKey[] = "K3Y$3CR3T"; // Clave de 9 bytes
const int keyLength = 9;

// Función de cifrado/descifrado XOR
void encryptData(char* data, int length) {
  for(int i = 0; i < length; i++) {
    data[i] = data[i] ^ encryptionKey[i % keyLength];
  }
}

// Función para convertir datos binarios a hexadecimal
String toHex(const char* data, int length) {
  String hexString = "";
  for(int i = 0; i < length; i++) {
    if((unsigned char)data[i] < 16) {
      hexString += "0";
    }
    hexString += String((unsigned char)data[i], HEX);
  }
  hexString.toUpperCase();
  return hexString;
}

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
      currentState = STATE_CAPTURE;
      break;
    case STATE_BUNDLING:
      bundling();
      currentState = STATE_CAPTURE;
      break;
  }

  if (millis() - lastBundlingTime >= BUNDLING_INTERVAL) {
    currentState = STATE_BUNDLING;
    lastBundlingTime = millis();
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
  Serial.println("JSON sin cifrar: " + jsonPayload);

  char jsonBuffer[512];
  int length = jsonPayload.length();
  jsonPayload.toCharArray(jsonBuffer, length + 1);

  encryptData(jsonBuffer, length);

  String encryptedHex = toHex(jsonBuffer, length);

  Serial.println("Datos cifrados (hex): " + encryptedHex);

  String finalPayload = "{\"encrypted_data\":\"" + encryptedHex + "\"}";

  http.begin(wifiClient, serverURL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(finalPayload);

  String response = http.getString();
  if (httpResponseCode > 0) {
    Serial.printf("✓ Enviado exitosamente (Código: %d)\n", httpResponseCode);
    Serial.println("Respuesta: " + response);
  } else {
    Serial.printf("✗ Error en envío (Código: %d)\n", httpResponseCode);
    Serial.println("Respuesta: " + response);
  }
  Serial.println("========================================\n");

  http.end();
}

// Empaquetado y salida final con GPS
void bundling() {
  smartDelay(0);
  // Serial.println("\n[BUNDLING] Empaquetando datos finales...");
  // Serial.println("----------------------------------------");

  // Actualizar coordenadas GPS
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
  } else {
    lat = -999.0;
    lon = -999.0;
  }

  // Llamar a la función de envío de datos
  sendDataToServer();

  /*
  // Todo el código de diagnóstico GPS comentado
  // Serial.println("Diagnóstico GPS:");
  // Serial.println("----------------------------------------");
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

  // Serial.println("\nEstado de coordenadas:");
  // Serial.println("- Estado: Válidas");
  // Serial.printf("- Latitud: %.6f\n", lat);
  // Serial.printf("- Longitud: %.6f\n", lon);
  // Serial.printf("- Edad de los datos: %lu ms\n", gps.location.age());
  // Serial.println("- Estado: NO VÁLIDAS");
  // Serial.println("- Causa posible: Sin fix GPS");

  // Diagnóstico de sensores
  // Serial.println("\nDiagnóstico de sensores HDC1080:");
  // Serial.printf("- Temperatura: %.2f°C\n", temperatura);
  // Serial.printf("- Humedad: %.2f%%\n", humedad);
  */

  // Serial.println("----------------------------------------\n");
}