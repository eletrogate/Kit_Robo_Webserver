/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#include "macros.h"

bool controle_auto_rec = false;
bool evt_conectado = false;
bool srv_restart = false;
unsigned int tempo_decorrido = 0;

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

String ssid;
String pass;
String ip;
String gateway;

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

bool caractere_valido_data(char c)  {
  return ((c >= '0' && c <= '9') || c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)  { // definição da tratativa de evento de servidor assíncrono
  if(type == WS_EVT_DATA) {                                                                                 // se for o recebimento de dados e o Arduino puder receber
    char* data_ = (char*) data;                                                                                                   // registra todos os dados recebidos do servidor
    Serial.write(INICIALIZADOR);
    for(size_t i = 0; caractere_valido_data(data_[i]) && i < TAMANHO_MAXIMO_DADOS; i ++)                                          // enquanto for um caractere valido e o tamanho for inferior ao maximo
      Serial.write(data_[i]);                                                                                                     // envia este dado na serial
    Serial.write(FINALIZADOR); }                                                                                          // envia o caractere separador para tratamento
  else if(type == WS_EVT_CONNECT) {
    digitalWrite(PIN_OUT, LOW);
    evt_conectado = true;
  }
} 

String readFile(fs::FS &fs, const char * path) {
  File file = fs.open(path, "r");
  String fileContent;
  if(file.available()) fileContent = file.readStringUntil('\n');
  file.close();
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, "w");
  file.print(message);
  file.close();
}

bool initWiFi() {
  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)) return false;

  WiFi.begin(ssid.c_str(), pass.c_str());
  delay(TEMPO_INICIO_WIFI);
  return WiFi.status() == WL_CONNECTED;
}

void setup() {
  pinMode(PIN_IN, INPUT);     // inicia a entrada
  pinMode(PIN_OUT, OUTPUT);   // e a saída
  digitalWrite(PIN_OUT, HIGH); // digitais

  LittleFS.begin();

  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile (LittleFS, gatewayPath);

  ws.onEvent(onWsEvent);  // indica qual função deve ser chamada ao perceber um evento
  server.addHandler(&ws); // indica que o servidor será tratado de acordo com o WebSocket

  if(initWiFi()) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // quando alguém se conectar ao servidor do joystick
      request->send(LittleFS, "/index.html");  });                 // envia o script salvo em /index.html

    server.on("/IP", HTTP_GET, [](AsyncWebServerRequest *request) {                  // quando alguém se conectar ao servidor do ip
      request->send(200, "text/plain", "Seu IP: " + WiFi.localIP().toString());  }); // envia o IP recebido pelo roteador

    //************** as chamadas do método a seguir constroem a pagina do joystick ******************//

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/style.css", "text/css"); });
    
    server.on("/joystick.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/joystick.js", "text/javascript"); });

    server.on("/icone.ico", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/icone.ico", "icone/ico"); });

    server.on("/escritaHeader.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/escritaHeader.png", "image/png"); });

    server.on("/logoFundo.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/logoFundo.png", "image/png"); });

    server.on("/loja.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/loja.png", "image/png"); });

    server.on("/blog.png", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/blog.png", "image/png"); });
    //****************************************************************************************
  } else {
    WiFi.softAP("ROBO_ELETROGATE", NULL);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html"); });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++) {
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()) {
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value();
            writeFile(LittleFS, ssidPath, ssid.c_str()); }

          if (p->name() == PARAM_INPUT_2) {
            pass = p->value();
            writeFile(LittleFS, passPath, pass.c_str()); }

          if (p->name() == PARAM_INPUT_3) {
            ip = p->value();
            writeFile(LittleFS, ipPath, ip.c_str()); }

          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value();
            writeFile(LittleFS, gatewayPath, gateway.c_str()); }
        }
      }
      srv_restart = true;
      request->send(200, "text/plain", "Credenciais cadastradas!");
    });
  }
  server.begin(); // inicia o servidor
  Serial.begin(9600);
}

void loop() {
  if(srv_restart) ESP.restart();

  if(evt_conectado && digitalRead(PIN_IN)) {
    digitalWrite(PIN_OUT, HIGH);
    evt_conectado = false;
  }

  if(WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED && controle_auto_rec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);    // ativa a autoreconexão
    WiFi.persistent(true);          // ativa a persistência
    controle_auto_rec = true;   }   // indica que a robustez de WiFi já foi configurada após a reconexão

  if(controle_auto_rec == true && (WiFi.getMode() != WIFI_STA || WiFi.status() != WL_CONNECTED))   // se estiver desconectado ou em modo diferente de STA
    controle_auto_rec = false;      // indica que houve a desconexão

  if(millis() - tempo_decorrido >= INTERVALO_WIFI) { // a cada INTERVALO_WIFI ms
    tempo_decorrido = millis(); // atualiza o tempo
    if(WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) // se estiver desconectado e em modo STA
      WiFi.reconnect(); } // tenta reconectar
}
