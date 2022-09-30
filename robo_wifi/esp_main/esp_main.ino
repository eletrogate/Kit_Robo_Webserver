/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#include "constantes.h"

bool controleAutoRec = false;
bool evtConectado = false;
bool srvRestart = false;
unsigned tempoDecorrido = 0;

String ssid;
String pass;
String ip;
String gateway;

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

bool caractereValido(char c)  {
  return ((c >= '0' && c <= '9') || c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)  { // definição da tratativa de evento de servidor assíncrono
  if(type == WS_EVT_DATA) {                                                                                 // se for o recebimento de dados e o Arduino puder receber
    char *data_ = (char*) data;                                                                                                   // registra todos os dados recebidos do servidor
    Serial.write(caractereInicio);
    for(uint8_t i = 0; caractereValido(data_[i]) && i < tamanhoMaximoDados; i ++)                                          // enquanto for um caractere valido e o tamanho for inferior ao maximo
      Serial.write(data_[i]);                                                                                                     // envia este dado na serial
    Serial.write(caractereFinal); }                                                                                          // envia o caractere separador para tratamento
  else if(type == WS_EVT_CONNECT) {
    digitalWrite(pinOut, LOW);
    evtConectado = true;
  }
} 

String readFile(fs::FS &fs, const char *path) {
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
  delay(tempoInicioWiFi);
  return WiFi.status() == WL_CONNECTED;
}

void setup() {
  pinMode(pinIn, INPUT);     // inicia a entrada
  pinMode(pinOut, OUTPUT);   // e a saída
  digitalWrite(pinOut, HIGH); // digitais

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
      uint8_t params = request->params();
      for(uint8_t i = 0; i < params; i ++) {
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()) {
          if(p->name() == paramInput1) {
            ssid = p->value();
            writeFile(LittleFS, ssidPath, ssid.c_str()); }

          if(p->name() == paramInput2) {
            pass = p->value();
            writeFile(LittleFS, passPath, pass.c_str()); }

          if(p->name() == paramInput3) {
            ip = p->value();
            writeFile(LittleFS, ipPath, ip.c_str()); }

          if(p->name() == paramInput4) {
            gateway = p->value();
            writeFile(LittleFS, gatewayPath, gateway.c_str()); }
        }
      }
      srvRestart = true;
      request->send(200, "text/plain", "Credenciais cadastradas!");
    });
  }
  server.begin(); // inicia o servidor
  Serial.begin(9600);
}

void loop() {
  if(srvRestart) ESP.restart();

  if(evtConectado && digitalRead(pinIn)) {
    digitalWrite(pinOut, HIGH);
    evtConectado = false;
  }

  if(WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED && controleAutoRec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);    // ativa a autoreconexão
    WiFi.persistent(true);          // ativa a persistência
    controleAutoRec = true;   }   // indica que a robustez de WiFi já foi configurada após a reconexão

  if(controleAutoRec == true && (WiFi.getMode() != WIFI_STA || WiFi.status() != WL_CONNECTED))   // se estiver desconectado ou em modo diferente de STA
    controleAutoRec = false;      // indica que houve a desconexão

  if(millis() - tempoDecorrido >= intervaloWiFi) { // a cada intervaloWiFi ms
    tempoDecorrido = millis(); // atualiza o tempo
    if(WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) // se estiver desconectado e em modo STA
      WiFi.reconnect(); } // tenta reconectar
}
