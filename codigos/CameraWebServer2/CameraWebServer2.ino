#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE  // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

// ===========================
// WiFi credentials
// ===========================
const char *ssid     = "LAN_VGA";
const char *password = "Tr252870172";

// Endpoint Flask
const char* api_host = "192.168.2.107";
const int api_port = 5000;
const char* api_route = "/upload";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- Configurações do Broker MQTT ---
const char* mqtt_broker = "01847d1558da4697a5180f12ed9f504a.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;// Porta padrao MQTT sem segurança
const char* mqtt_client_id = "ESP32cam2"; // ID único para o seu cliente MQTT
const char* mqtt_topic = "cameras/cam2";  // Tópico da câmera
const char* mqtt_topic_listener = "comandos/init";  // Tópico da câmera para inicia-la
String mqtt_topic_listener_horario = "comandos/horario";  // Tópico para receber o envio de horário de funcionamento
const char* mqtt_user = "EspServer"; // Usuário MQTT criado no Broker
const char* mqtt_pass = "Esp123456";

// --- Outras configurações ---
const int camId = 2;
unsigned long ms = 0;
const unsigned long interval = 30000;
unsigned long seq = 0;
String horario_inicio = "00:00";
String horario_fim = "00:00";

// --- Configurações NTP ---
const char* ntpServer = "a.ntp.br";       // Servidor NTP para sincronização de tempo
const long gmtOffset_sec = -3 * 60 * 60;  // Offset de GMT-3

void startCameraServer();
void setupLedFlash(int pin);

// --- Função para publicar o IP no broker MQTT ---
void publishIP() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  StaticJsonDocument<100> doc;
  doc["seq"] = seq++;
  doc["ip"] = WiFi.localIP().toString();

  char payload[100];
  serializeJson(doc, payload);
  client.publish(mqtt_topic, payload);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.print("Payload recebido: ");
  Serial.println(msg);

  if (String(topic) == mqtt_topic_listener_horario) {
    int idxIni = msg.indexOf("Ini: ");
    int idxFim = msg.indexOf("Fim: ");

    if (idxIni != -1 && idxFim != -1) {
      horario_inicio = msg.substring(idxIni + 5, idxIni + 10);
      horario_fim = msg.substring(idxFim + 5, idxFim + 10);

      Serial.println("Horário recebido:");
      Serial.println("Início: " + horario_inicio);
      Serial.println("Fim: " + horario_fim);
    }
  } else {
    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
      Serial.print("Erro ao fazer parsing do JSON: ");
      Serial.println(error.c_str());
      return;
    }

    if (doc.containsKey("discovery") && doc["discovery"] == "cameras") {
      publishIP();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  configTime(gmtOffset_sec, 0, ntpServer);

  espClient.setInsecure();

  // Configurar o cliente MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);  
  client.subscribe("comandos/init");
  client.subscribe("comandos/horario");
  Serial.println("Inscrito em comandos/init");
  Serial.println("Inscrito em comandos/horario");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

// --- Função para reconectar ao broker MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
      client.subscribe("comandos/init");
      client.subscribe("comandos/horario");
      Serial.println("Inscrito em comandos/init");
      Serial.println("Inscrito em comandos/horario");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

bool dentroIntervaloHora(String inicio, String fim) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Erro ao obter hora local.");
    return false;
  }

  int horaAtual = timeinfo.tm_hour * 60 + timeinfo.tm_min;

  int h1 = inicio.substring(0, 2).toInt();
  int m1 = inicio.substring(3, 5).toInt();
  int h2 = fim.substring(0, 2).toInt();
  int m2 = fim.substring(3, 5).toInt();

  int inicioMin = h1 * 60 + m1;
  int fimMin = h2 * 60 + m2;

  if (inicioMin <= fimMin) {
    return horaAtual >= inicioMin && horaAtual <= fimMin;
  } else {
    return horaAtual >= inicioMin || horaAtual <= fimMin;
  }
}

// --- Função para enviar uma imagem ao servidor Flask ---
void sendImage() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter horário");
    return;
  }
  uint32_t timestamp = mktime(&timeinfo);

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Erro ao capturar imagem");
    return;
  }

  const char* boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

  // Monta partes do corpo
  String bodyStart = "--" + String(boundary) + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"camId\"\r\n\r\n" + String(camId) + "\r\n";
  bodyStart += "--" + String(boundary) + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"timestamp\"\r\n\r\n" + String(timestamp) + "\r\n";
  bodyStart += "--" + String(boundary) + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"image\"; filename=\"capture.jpg\"\r\n";
  bodyStart += "Content-Type: image/jpeg\r\n\r\n";

  String bodyEnd = "\r\n--" + String(boundary) + "--\r\n";

  int totalLen = bodyStart.length() + fb->len + bodyEnd.length();

  // Conecta diretamente via WiFiClient
  WiFiClient client;
  if (!client.connect(api_host, api_port)) {
    Serial.println("Conexão com servidor falhou");
    esp_camera_fb_return(fb);
    return;
  }

  // Envia headers
  client.print("POST ");
  client.print(api_route);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(api_host);
  client.println("Content-Type: multipart/form-data; boundary=" + String(boundary));
  client.println("Content-Length: " + String(totalLen));
  client.println("Connection: close");
  client.println();  // Linha em branco entre headers e body

  // Envia corpo: parte 1 (form fields)
  client.print(bodyStart);

  // Envia corpo: parte 2 (imagem binária)
  client.write(fb->buf, fb->len);

  // Envia corpo: parte 3 (final)
  client.print(bodyEnd);

  esp_camera_fb_return(fb);

  // Espera resposta do servidor
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;  // fim dos headers
  }

  String resposta = client.readString();
  Serial.println("Resposta do servidor:");
  Serial.println(resposta);

  client.stop();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (dentroIntervaloHora(horario_inicio, horario_fim)) {
    if (millis() - ms > interval) {
      ms = millis();
      sendImage();
    }
  }
}
