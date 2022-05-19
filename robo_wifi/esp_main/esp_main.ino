#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#define   WEBSERVER_H
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <Arduino.h>
#include "FS.h"
#include "macros.h"

bool controle_auto_rec = false;
bool conectado = false;
unsigned int tempo_decorrido = 0;

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

bool caractere_valido_data(char c)  {
  return ((c >= '0' && c <= '9') || c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)  { // definição da tratativa de evento de servidor assíncrono
  if(type == WS_EVT_DATA && !digitalRead(PIN_IN)) {                                                                                 // se for o recebimento de dados e o Arduino puder receber
      char* data_ = (char*) data;                                                                                                   // registra todos os dados recebidos do servidor
      for(size_t i = 0; caractere_valido_data(data_[i]) && i < TAMANHO_MAXIMO_DADOS; i ++)                                          // enquanto for um caractere valido e o tamanho for inferior ao maximo
        Serial.write(data_[i]);                                                                                                     // envia este dado na serial
      Serial.write(CARACTERE_SEPARADOR); }                                                                                          // envia o caractere separador para tratamento
}

void setup() {
  pinMode(PIN_IN, INPUT);     // inicia a entrada
  pinMode(PIN_OUT, OUTPUT);   // e a saída
  digitalWrite(PIN_OUT, LOW); // digitais

  SPIFFS.begin(); // inicia a spiffs

  ws.onEvent(onWsEvent);  // indica qual função deve ser chamada ao perceber um evento
  server.addHandler(&ws); // indica que o servidor será tratado de acordo com o WebSocket

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // quando alguém se conectar ao servidor do joystick
    request->send(SPIFFS, "/index.html");  });                 // envia o script salvo em htmlResponse

  server.on("/IP", HTTP_GET, [](AsyncWebServerRequest *request) {                  // quando alguém se conectar ao servidor do ip
    request->send(200, "text/plain", "Seu IP: " + WiFi.localIP().toString());  }); // envia o IP recebido pelo roteador

  //************** as chamadas do método a seguir constroem a pagina do joystick ******************//

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/joystick.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/joystick.js", "text/javascript");
  });

  server.on("/icone.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/icone.ico", "icone/ico");
  });

  server.on("/escritaHeader.png", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(SPIFFS, "/escritaHeader.png", "image/png");
  });

  server.on("/logoFundo.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/logoFundo.png", "image/png");
  });

  server.on("/loja.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/loja.png", "image/png");
  });

  server.on("/blog.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/blog.png", "image/png");
  });
  
  //****************************************************************************************

  WiFiManager wifiManager;  wifiManager.setDebugOutput(false); wifiManager.setHttpPort(81); // cria o WiFiManager na porta 81 e desativa a depuração serial
  wifiManager.autoConnect("ROBO_ELETROGATE","12345678"); wifiManager.stopConfigPortal();    // abre o WiFiManager e, ao fim, o desconecta

  WiFi.softAP("ROBO_ELETROGATE", "12345678"); // abre a AP para controle e verificação de IP
  server.begin();                             // inicia o servidor

  Serial.begin(9600); // inicia a serial
}

void loop() {

  if(WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED && controle_auto_rec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);    // ativa a autoreconexão
    WiFi.persistent(true);          // ativa a persistência
    controle_auto_rec = true;       // indica que a robustez de WiFi já foi configurada após a reconexão
    digitalWrite(PIN_OUT, LOW);  }  // avisa ao Arduino que pode voltar a acionar os motores

  if(controle_auto_rec == true && (WiFi.getMode() != WIFI_STA || WiFi.status() != WL_CONNECTED))  { // se estiver desconectado em modo STA
    controle_auto_rec = false;      // indica que houve a desconexão
    digitalWrite(PIN_OUT, HIGH);  } // aviso ao Arduino para que os motores sejam freiados

  if(millis() - tempo_decorrido >= INTERVALO_WIFI) { // a cada INTERVALO_WIFI ms
    tempo_decorrido = millis(); // atualiza o tempo
    if(WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) // se estiver desconectado e em modo STA
      WiFi.reconnect(); } // tenta reconectar
}