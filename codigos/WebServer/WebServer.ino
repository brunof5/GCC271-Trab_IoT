#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Para enviar dados em formato JSON

//Informacoes da rede WiFi
const char* ssid     = "LAN_VGA";
const char* password = "Tr252870172";

// --- Configurações do Broker MQTT ---
const char* mqtt_broker = "01847d1558da4697a5180f12ed9f504a.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;// Porta padrao MQTT sem segurança
const char* mqtt_client_id = "ESP32Server"; // ID único para o seu cliente MQTT
const char* mqtt_user = "EspServer"; // Usuário MQTT criado no Broker
const char* mqtt_pass = "Esp123456";

// --- Configurações Gerais ---
String horario_inicio = "00:00";
String horario_fim = "00:00";
String ipFlask = "192.168.2.107";

// --- Tópicos MQTT ---
const char* topic_comando = "comandos/init";
const char* topic_horario = "comandos/horario";

// --- Endereços das Câmeras ---
String ipCam1 = "";
String ipCam2 = "";

// --- Web Server ---
WebServer server(80); // Porta HTTP padrão

// --- Pino do LED Built-in ---
const int LED_PIN = LED_BUILTIN;

WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- Função para reconectar ao broker MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
      client.subscribe("cameras/cam1");
      client.subscribe("cameras/cam2");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

// --- Função callback obter os endereços das câmeras ---
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<100> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("Erro ao decodificar JSON");
    return;
  }

  String ip = doc["ip"];
  if (String(topic) == "cameras/cam1") {
    ipCam1 = ip;
  } else if (String(topic) == "cameras/cam2") {
    ipCam2 = ip;
  }

  Serial.print("Atualizado IP de ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(ip);
}

// --- Função para envio manual de mensagem MQTT via web ---
void handleRequisitarIPs() {
  String payload = "{\"discovery\":\"cameras\"}";
  client.publish(topic_comando, payload.c_str());

  Serial.println("Requisição enviada via WebServer!");

  server.send(200, "text/plain", "OK");
}

void handleSalvarHorario() {
  if (server.hasArg("inicio")) {
    horario_inicio = server.arg("inicio");
  }
  if (server.hasArg("fim")) {
    horario_fim = server.arg("fim");
  }

  Serial.println("Horário configurado:");
  Serial.println("Início: " + horario_inicio);
  Serial.println("Fim: " + horario_fim);

  String payload = "Ini: " + horario_inicio + " Fim: " + horario_fim;
  client.publish(topic_horario, payload.c_str());

  server.send(200, "text/html", "<html><body><h1>Horario salvo com sucesso!</h1><a href='/'>Voltar</a></body></html>");
}

// --- Página inicial da web ---
void handleRoot() {
  String html = "<html><head><meta charset=\"UTF-8\"><title>Detecção de movimentação suspeita</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f8f8f8; color: #333; padding: 20px; }";
  html += "h1, h2, h3 { color: #222; }";
  html += "form, button { margin-top: 10px; margin-bottom: 20px; }";
  html += "input[type='time'], input[type='submit'], button { padding: 10px; border: none; border-radius: 5px; font-size: 16px; }";
  html += "input[type='submit'], button { background-color: #4CAF50; color: white; cursor: pointer; }";
  html += "input[type='submit']:hover, button:hover { background-color: #45a049; }";
  html += "img { border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.3); margin: 10px 0; }";
  html += "ul { list-style-type: none; padding: 0; }";
  html += "li { margin-bottom: 10px; }";
  html += ".cameras { display: flex; gap: 20px; flex-wrap: wrap; margin-top: 20px; }";
  html += ".camera-box { background: #ffffff; padding: 10px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); width: fit-content; }";
  html += ".camera-box h3 { margin-top: 0; }";
  html += "</style></head><body>";

  html += "<h1>Detecção SUS</h1>";

  html += "<button onclick=\"requisitarIPs()\">🔍 Descobrir dispositivos</button>";

  html += "<script>";
  html += "function requisitarIPs() {";
  html += "  fetch('/requisitar_ips')";
  html += "    .then(response => { if (response.ok) { setTimeout(() => location.reload(), 500); } else { alert('Erro ao requisitar IPs.'); } })";
  html += "    .catch(error => { alert('Erro de rede: ' + error); });";
  html += "}";
  html += "</script>";

  html += "<script>";
  html += "function salvarHorario(event) {";
  html += "  event.preventDefault();";  // Impede o recarregamento da página
  html += "  const inicio = document.getElementById('inicio').value;";
  html += "  const fim = document.getElementById('fim').value;";
  html += "  const dados = new URLSearchParams();";
  html += "  dados.append('inicio', inicio);";
  html += "  dados.append('fim', fim);";
  html += "  fetch('/salvar_horario', {";
  html += "    method: 'POST',";
  html += "    body: dados,";
  html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' }";
  html += "  })";
  html += "  .then(response => response.ok ? 'Horário salvo com sucesso!' : 'Erro ao salvar horário.')";
  html += "  .then(msg => document.getElementById('mensagem').innerText = msg)";
  html += "  .catch(err => document.getElementById('mensagem').innerText = 'Erro de rede: ' + err);";
  html += "}";
  html += "</script>";

  html += "<h1>Configurar Intervalo de Captura</h1>";
  html += "<form onsubmit=\"salvarHorario(event)\">";
  html += "Início (HH:MM): <input type=\"time\" id=\"inicio\" value=\"" + horario_inicio + "\"><br><br>";
  html += "Fim (HH:MM): <input type=\"time\" id=\"fim\" value=\"" + horario_fim + "\"><br><br>";
  html += "<input type=\"submit\" value=\"Salvar\">";
  html += "</form>";
  html += "<p id=\"mensagem\"></p>";

  html += "<h2>Câmeras Detectadas:</h2><ul>";

  html += "<div class='cameras'>";

  if (ipCam1 != "") {
    html += "<div class='camera-box'>";
    html += "<h3>Câmera 1</h3>";
    html += "<img src='http://" + ipCam1 + ":81/stream' width='480' height='320' />";
    html += "<p><a href='http://" + ipCam1 + "'>Acessar Câmera 1</a></p>";
    html += "</div>";
  } else {
    html += "<div class='camera-box'><h3>Câmera 1</h3><p>Aguardando conexão...</p></div>";
  }

  if (ipCam2 != "") {
    html += "<div class='camera-box'>";
    html += "<h3>Câmera 2</h3>";
    html += "<img src='http://" + ipCam2 + ":81/stream' width='480' height='320' />";
    html += "<p><a href='http://" + ipCam2 + "'>Acessar Câmera 2</a></p>";
    html += "</div>";
  } else {
    html += "<div class='camera-box'><h3>Câmera 2</h3><p>Aguardando conexão...</p></div>";
  }

  html += "</div>";

  html += "</ul>";

  html += "<form action=\"http://" + ipFlask + ":5000/galeria\">";
  html += "<button type=\"submit\">🖼️ Ver Galeria</button>";
  html += "</form>";

  html += "<form action=\"http://" + ipFlask + ":5000/galeria_suspeitas\">";
  html += "<button type=\"submit\">🚨 Ver Fotos Suspeitas</button>";
  html += "</form>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// --- Configuração inicial ---
void setup() {
  Serial.begin(115200);

  // Configura o pino do LED como saída
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Começa desligado

  //Gerar numeros aleatorios
  randomSeed(analogRead(0));

  // Conectar ao Wi-Fi
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  espClient.setInsecure();
  // Configurar o cliente MQTT
  client.setServer(mqtt_broker, mqtt_port);

  client.setCallback(callback);

  // Inicia o servidor web
  server.on("/", handleRoot);
  server.on("/requisitar_ips", HTTP_GET, handleRequisitarIPs);
  server.on("/salvar_horario", HTTP_POST, handleSalvarHorario);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

// --- Loop principal ---
void loop() {
  if (!client.connected()) {
    reconnect(); // Se desconectado, tenta reconectar e subscrever novamente
  }
  client.loop(); // Mantém conexão com broker
  server.handleClient();
}
