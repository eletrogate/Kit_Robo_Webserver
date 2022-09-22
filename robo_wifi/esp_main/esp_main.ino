/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#include "macros.h"

bool controle_auto_rec = false;
bool conectado = false;
bool restart = false;
unsigned int tempo_decorrido = 0;

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
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
  if(type == WS_EVT_DATA && !digitalRead(PIN_IN)) {                                                                                 // se for o recebimento de dados e o Arduino puder receber
      char* data_ = (char*) data;                                                                                                   // registra todos os dados recebidos do servidor
      for(size_t i = 0; caractere_valido_data(data_[i]) && i < TAMANHO_MAXIMO_DADOS; i ++)                                          // enquanto for um caractere valido e o tamanho for inferior ao maximo
        Serial.write(data_[i]);                                                                                                     // envia este dado na serial
      Serial.write(CARACTERE_SEPARADOR); }                                                                                          // envia o caractere separador para tratamento
} 

// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  //-*Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    //-*Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  //-*Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if(!file){
    //-*Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    //-*Serial.println("- file written");
  } else {
    //-*Serial.println("- frite failed");
  }
  file.close();
}

bool initWiFi() {
  if(ssid=="" || ip==""){
    //-*Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)){
    //-*Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());

  //-*Serial.println("Connecting to WiFi...");
  delay(20000);
  if(WiFi.status() != WL_CONNECTED) {
    //-*Serial.println("Failed to connect.");
    return false;
  }

  //-*Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  pinMode(PIN_IN, INPUT);     // inicia a entrada
  pinMode(PIN_OUT, OUTPUT);   // e a saída
  digitalWrite(PIN_OUT, LOW); // digitais
  //-*Serial.begin(9600); // inicia a serial

  if (!LittleFS.begin()) {
    //-*Serial.println("An error has occurred while mounting LittleFS");
  } else {
    //-*Serial.println("LittleFS mounted successfully");
  }

  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile (LittleFS, gatewayPath);
  //-*Serial.println(ssid);
  //-*Serial.println(pass);
  //-*Serial.println(ip);
  //-*Serial.println(gateway);



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
    server.begin();                                                 // inicia o servidor
  } else {
    // Connect to Wi-Fi network with SSID and password
    //-*Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    //-*Serial.print("AP IP address: ");
    //-*Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            //-*Serial.print("SSID set to: ");
            //-*Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            //-*Serial.print("Password set to: ");
            //-*Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            //-*Serial.print("IP Address set to: ");
            //-*Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            //-*Serial.print("Gateway set to: ");
            //-*Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
        }
      }
      restart = true;
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
    });
    server.begin();
  }
  Serial.begin(9600); //-*
}

void loop() {
  if(restart) ESP.restart();

  if(WiFi.getMode() == WIFI_STA && WiFi.status() ==   WL_CONNECTED && controle_auto_rec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);    // ativa a autoreconexão
    WiFi.persistent(true);          // ativa a persistência
    controle_auto_rec = true;       // indica que a robustez de WiFi já foi configurada após a reconexão
    digitalWrite(PIN_OUT, LOW);  }  // avisa ao Arduino que pode voltar a acionar os motores

  if(controle_auto_rec == true && (WiFi.getMode() != WIFI_STA || WiFi.status() != WL_CONNECTED))  { // se estiver desconectado em modo STA
    controle_auto_rec = false;      // indica que houve a desconexão
    digitalWrite(PIN_OUT, HIGH);  } // avisa ao Arduino para que os motores sejam parados

  if(millis() - tempo_decorrido >= INTERVALO_WIFI) { // a cada INTERVALO_WIFI ms
    tempo_decorrido = millis(); // atualiza o tempo
    if(WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) // se estiver desconectado e em modo STA
      WiFi.reconnect(); } // tenta reconectar
}
