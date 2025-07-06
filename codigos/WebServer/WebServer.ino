#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Para enviar dados em formato JSON

//Informacoes da rede WiFi
const char* ssid     = "ssid";
const char* password = "password";

// --- Configurações do Broker MQTT ---
const char* mqtt_broker = "mqtt_broker";
const int mqtt_port = 8883;// Porta padrao MQTT sem segurança
const char* mqtt_client_id = "ESP32Server"; // ID único para o seu cliente MQTT
const char* mqtt_user = "mqtt_user"; // Usuário MQTT criado no Broker
const char* mqtt_pass = "mqtt_pass";

// --- Tópicos MQTT ---
const char* topic_comando = "comandos/teste";

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
void handleEnviar() {
  String payload = "{\"mensagem\":\"Comando manual enviado via web\"}";
  client.publish(topic_comando, payload.c_str());

  Serial.println("Mensagem enviada via WebServer!");

  server.send(200, "text/html", "<h1>Mensagem enviada com sucesso!</h1>");
}

// --- Página inicial da web ---
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="pt-BR">
  <head>
    <meta charset="UTF-8">
    <title>Monitoramento IoT - ESP32</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #f5f5f5;
        margin: 0;
        padding: 0;
      }
      header {
        background-color: #007bff;
        color: white;
        padding: 20px;
        text-align: center;
      }
      main {
        padding: 20px;
        max-width: 900px;
        margin: auto;
      }
      section {
        background-color: white;
        padding: 20px;
        margin-bottom: 20px;
        border-radius: 8px;
        box-shadow: 0 0 8px rgba(0,0,0,0.1);
      }
      h2 { color: #333; }
      button {
        padding: 10px 20px;
        background-color: #28a745;
        color: white;
        border: none;
        border-radius: 5px;
        font-size: 16px;
        cursor: pointer;
      }
      button:hover { background-color: #218838; }
      .camera-container {
        display: flex;
        flex-wrap: wrap;
        gap: 20px;
      }
      .camera {
        flex: 1 1 45%;
        border: 1px solid #ccc;
        padding: 10px;
        border-radius: 6px;
        text-align: center;
      }
      .camera iframe {
        width: 100%;
        height: 200px;
        border: none;
      }
      .images-gallery {
        display: flex;
        flex-wrap: wrap;
        gap: 10px;
      }
      .images-gallery img {
        width: 150px;
        height: auto;
        border: 1px solid #ddd;
        border-radius: 5px;
      }
      .hidden { display: none; }
    </style>
  </head>
  <body>
    <header>
      <h1>Monitoramento de Câmeras - ESP32 IoT</h1>
    </header>

    <main>
      <section>
        <h2>1. Dispositivos</h2>
        <button onclick="descobrirDispositivos()">Descobrir Dispositivos</button>
      </section>

      <section id="sec-cameras" class="hidden">
        <h2>2. Visualização das Câmeras</h2>
        <div id="camera-container" class="camera-container"></div>
      </section>

      <section>
        <h2>3. Agendar Horário de Vigilância</h2>
        <form onsubmit="enviarHorario(event)">
          <input type="time" id="horario" required>
          <button type="submit">Confirmar</button>
        </form>
      </section>

      <section>
        <h2>4. Imagens Suspeitas Detectadas</h2>
        <form onsubmit="carregarImagens(event)">
          <input type="date" id="data-inicio">
          <input type="date" id="data-fim">
          <button type="submit">Filtrar</button>
        </form>
        <div id="gallery" class="images-gallery"></div>
      </section>
    </main>

    <script>
      function descobrirDispositivos() {
        fetch('/enviar')
          .then(() => {
            const sec = document.getElementById('sec-cameras');
            const container = document.getElementById('camera-container');
            sec.classList.remove('hidden');

            container.innerHTML = "";

            const cams = [
              { id: 1, ip: "$CAM1" },
              { id: 2, ip: "$CAM2" }
            ];

            cams.forEach(cam => {
              if (!cam.ip) return;
              container.innerHTML += `
                <div class="camera">
                  <h3>Câmera ${cam.id}</h3>
                  <iframe src="http://${cam.ip}:81/stream"></iframe>
                  <p><a href="http://${cam.ip}" target="_blank">Abrir Configuração</a></p>
                </div>`;
            });
          });
      }

      function enviarHorario(e) {
        e.preventDefault();
        const horario = document.getElementById("horario").value;
        alert("Horário " + horario + " enviado (simulado).");
        // fetch(`/vig?horario=${horario}`); // se quiser enviar via rota
      }

      function carregarImagens(e) {
        const inicio = document.getElementById("data-inicio").value;
        const fim = document.getElementById("data-fim").value;

        const galeria = document.getElementById("gallery");
        galeria.innerHTML = "";

        //por IP do flask auqi quando for implementado
        const servidor = "http...";

        fetch(`${servidor}/imagens?inicio=${inicio}&fim=${fim}`)
          .then(res => res.json())
          .then(data => {
            if (!data.imagens || data.imagens.length === 0) {
              galeria.innerHTML = "<p>Nenhuma imagem encontrada no período.</p>";
              return;
            }

            data.imagens.forEach(url => {
              const img = document.createElement("img");
              img.src = url;
              img.alt = "Imagem suspeita";
              galeria.appendChild(img);
            });
          })
          .catch(err => {
            galeria.innerHTML = "<p>Erro ao carregar imagens.</p>";
            console.error(err);
        });
      }
    </script>
  </body>
  </html>
  )rawliteral";

  // Substituir variáveis com IPs das câmeras recebidas via MQTT
  html.replace("$CAM1", ipCam1);
  html.replace("$CAM2", ipCam2);

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
  server.on("/enviar", handleEnviar);
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
