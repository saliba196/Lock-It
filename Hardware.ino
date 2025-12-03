#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/*
botãoItem (pino 22): indica se há algo dentro do armário.
botãoPorta (pino 23): indica se o armário está trancado.

ledItem (pino 26): acende quando o botão 1 é pressionado.
ledPorta (pino 27): acende quando o botão 2 é pressionado.

Se os dois botões estiverem pressionados → os dois LEDs acendem → 
o código envia status = 1 (ocupado).
Caso contrário, envia status = 0 (livre).
*/

/* ======== CONFIGURAÇÕES DE REDE ======== */
const char* ssid = "UNIFESP_CONVIDADOS";  // rede sem senha

/* ======== CONFIGURAÇÕES DO SUPABASE ======== */
const char* endpoint = "https://qhrntzqqduboalfhpxmu.supabase.co/functions/v1/esp_webhook";
const char* device_token = "SECRET_TOKEN_DO_ESP_01";  
const char* device_id = "esp32_01";                   
const int id_armario = 3;                             

/* ======== PINOS ======== */
const int botaoItem = 22;   // botão: há algo dentro do armário
const int botaoPorta = 23;  // botão: porta trancada
const int ledItem = 21;     // LED do botão item
const int ledPorta = 19;    // LED do botão porta

/* ======== VARIÁVEIS ======== */
int statusArmario = 0;   // 0 = livre, 1 = ocupado/trancado
int ultimoStatus = -1;   // evita envio repetido de mesmo status

void setup() {
  Serial.begin(115200);

  // Configura os botões como entrada
  pinMode(botaoItem, INPUT);
  pinMode(botaoPorta, INPUT);

  // Configura os LEDs como saída
  pinMode(ledItem, OUTPUT);
  pinMode(ledPorta, OUTPUT);

  // Inicializa LEDs apagados
  digitalWrite(ledItem, LOW);
  digitalWrite(ledPorta, LOW);

  // Conecta ao Wi-Fi (sem senha)
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar ao Wi-Fi. Continuando tentativa automática...");
  }
}

void loop() {
  int estadoItem = digitalRead(botaoItem);
  int estadoPorta = digitalRead(botaoPorta);

  // Controle dos LEDs — acende quando o botão é pressionado (HIGH)
  digitalWrite(ledItem, estadoItem);
  digitalWrite(ledPorta, estadoPorta);

  // Define o status do armário
  if (estadoItem == HIGH && estadoPorta == HIGH) {
    statusArmario = 1;  // Ocupado e trancado
  } else {
    statusArmario = 0;  // Livre
  }

  // Envia apenas se o status mudou
  if (statusArmario != ultimoStatus) {
    enviarStatus(statusArmario, estadoItem, estadoPorta);
    ultimoStatus = statusArmario;
    Serial.println("mudou :)");
  }

  delay(1000);  // leitura a cada 1 segundo
}

void enviarStatus(int status, int item, int porta) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", ("Bearer " + String(device_token)).c_str());

    // Monta o JSON a ser enviado
    StaticJsonDocument<256> doc;
    doc["device_id"] = device_id;
    doc["id_armario"] = id_armario;
    doc["status"] = status;  // 1 = ocupado, 0 = livre
    doc["item"] = item;      // estado do botão item
    doc["porta"] = porta;    // estado do botão porta

    String payload;
    serializeJson(doc, payload);

    Serial.print("Enviando dados: ");
    Serial.println(payload);

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.printf("Código HTTP: %d\n", httpResponseCode);
      String resposta = http.getString();
      Serial.println("Resposta: " + resposta);
    } else {
      Serial.printf("Erro ao enviar POST: %d\n", httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    WiFi.reconnect();
  }
}
