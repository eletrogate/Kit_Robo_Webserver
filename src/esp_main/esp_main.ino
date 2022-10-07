/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include "constantes.h"

bool controleAutoRec = false; //
bool evtConectado = false;    // declara e inicia as variaveis
bool srvRestart = false;      // de controle
bool controleWiFi = false;    //
unsigned tempoDecorrido = 0;  //

String ssid;    //
String pass;    // declara as variaveis
String ip;      // de credenciais
String gateway; //

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

IPAddress localIP;                // declara as variaveis
IPAddress localGateway;           // de conexão
IPAddress subnet(255, 255, 0, 0); //

bool caractereValido(char c)  {
  return ((c >= '0' && c <= '9') || c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)  { // definição da tratativa de evento de servidor assíncrono
  if(type == WS_EVT_DATA) {                                                       // se for o recebimento de dados e o Arduino puder receber
    char *data_ = (char*) data;                                                   // registra todos os dados recebidos do servidor
    Serial.write(caractereInicio);                                                // envia o caractere de inicio de transmissao
    for(uint8_t i = 0; caractereValido(data_[i]) && i < tamanhoMaximoDados; i ++) // enquanto for um caractere valido e o tamanho for inferior ao maximo
      Serial.write(data_[i]);                                                     // envia este dado na serial
    Serial.write(caractereFinal); }                                               // envia o caractere final para tratamento
  else if(type == WS_EVT_CONNECT) { // se for a conexao à pagina
    digitalWrite(pinOut, LOW);      // indica colocando a saída em nível baixo
    evtConectado = true;            // registra que conectou
  }
} 

void appendFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, "a"); // abre o arquivo para escrita
  file.print(message);            // escreve o conteudo no arquivo
  file.print(caractereFinal);     // escreve o caractere finalizador                     
  file.close();                   // fecha o arquivo
}

bool initWiFi() {
  WiFi.mode(WIFI_STA); // configura o modo como STA

  controleWiFi = false;
  
  File fileSSID = LittleFS.open(ssidPath, "r");       //
  File filePass = LittleFS.open(passPath, "r");       //  abre os arquivos
  File fileIP = LittleFS.open(ipPath, "r");           //  para leitura
  File fileGateway = LittleFS.open(gatewayPath, "r"); //
  while(fileSSID.available()) {                       //  enquanto houver redes cadastradas a serem verificadas
    ssid = fileSSID.readStringUntil('\n');            //  le a linha
    pass = filePass.readStringUntil('\n');            //  atual
    ip = fileIP.readStringUntil('\n');                //  de cada arquivo
    gateway = fileGateway.readStringUntil('\n');      //
    localIP.fromString(ip.c_str());                   //  define o IP
    localGateway.fromString(gateway.c_str());         //  define o Gateway
    if(!WiFi.config(localIP, localGateway, subnet)) continue; //  se falhar na configuracao, pula a parte do loop
    WiFi.begin(ssid.c_str(), pass.c_str());           //  inicia o WiFi
    delay(tempoInicioWiFi);                           //  aguarda o tempo estimado
    if(controleWiFi = (WiFi.status() == WL_CONNECTED)) break; //  se conectar, sai do loop
  }
  fileSSID.close();     //
  filePass.close();     // fecha os arquivos
  fileIP.close();       //
  fileGateway.close();  //
  return controleWiFi;  // retorna o estado da conexao
}

void setup() {
  pinMode(pinIn, INPUT);      // inicia a entrada
  pinMode(pinOut, OUTPUT);    // e a saída
  digitalWrite(pinOut, HIGH); // digitais

  LittleFS.begin(); // inicia o sistema de arquivos

  ws.onEvent(onWsEvent);  // indica qual função deve ser chamada ao perceber um evento
  server.addHandler(&ws); // indica que o servidor será tratado de acordo com o WebSocket

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // quando alguém se conectar ao servidor do joystick
      request->send(LittleFS, "/index.html");  });             // envia o script salvo em /index.html

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
  
  server.on("/WM", HTTP_GET, [](AsyncWebServerRequest *request) {  // quando se conectar à pagina do gerenciador
    request->send(LittleFS, "/wifimanager.html", "text/html"); }); // envia /wifimanager.html
  
  server.on("/managerStyle.css", HTTP_GET, [](AsyncWebServerRequest *request) { // indica o arquivo
    request->send(LittleFS, "/managerStyle.css", "text/css"); });               // de elementos estéticos
  
  server.on("/WM", HTTP_POST, [](AsyncWebServerRequest *request) {
    uint8_t params = request->params();                         //  registra a quantidade de parametros
    for(uint8_t i = 0; i < params; i ++) {                      //  para cada parametro
      AsyncWebParameter* p = request->getParam(i);              //  registra o parametro
      if(p->isPost()) {                                         //  se for post
        if(p->name() == paramInput1) {                          //  se o nome estiver de acordo
          ssid = p->value();                                    //  registra o dado
          appendFile(LittleFS, ssidPath, ssid.c_str()); }        //  escreve no sistema de arquivos

        if(p->name() == paramInput2) {                          //  se o nome estiver de acordo
          pass = p->value();                                    //  registra o dado
          appendFile(LittleFS, passPath, pass.c_str()); }        //  escreve no sistema de arquivos

        if(p->name() == paramInput3) {                          //  se o nome estiver de acordo
          ip = p->value();                                      //  registra o dado
          appendFile(LittleFS, ipPath, ip.c_str()); }            //  escreve no sistema de arquivos

        if(p->name() == paramInput4) {                          //  se o nome estiver de acordo
          gateway = p->value();                                 //  registra o dado
          appendFile(LittleFS, gatewayPath, gateway.c_str()); }  //  escreve no sistema de arquivos
      }
    }
    srvRestart = true;  // registra que o chip deve reiniciar
    request->send(200, "text/plain", "Credenciais cadastradas!"); // gera uma página avisando que as credenciais foram cadastradas
  });  

  if(!initWiFi()) WiFi.softAP("ROBO_ELETROGATE", NULL); // se não conseguir se conectar a uma rede, inicia a AP
  
  server.begin();     // inicia o servidor
  Serial.begin(9600); // inicia a serial
}

void loop() {
  if(srvRestart) ESP.restart(); // se for para o sistema reiniciar, reinicia

  if(evtConectado && digitalRead(pinIn)) {  // se houve conexão à página
    digitalWrite(pinOut, HIGH);             // envia um sinal digital de nível alto
    evtConectado = false; }                 // reseta a variavel de controle

  if(WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED && controleAutoRec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);  // ativa a autoreconexão
    WiFi.persistent(true);        // ativa a persistência
    controleAutoRec = true;   }   // indica que a robustez de WiFi já foi configurada após a reconexão

  if(controleAutoRec == true && (WiFi.getMode() != WIFI_STA || WiFi.status() != WL_CONNECTED))   // se estiver desconectado ou em modo diferente de STA
    controleAutoRec = false;      // indica que houve a desconexão

  if(millis() - tempoDecorrido >= intervaloWiFi) { // a cada intervaloWiFi ms
    tempoDecorrido = millis(); // atualiza o tempo
    if(WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) // se estiver desconectado e em modo STA
      WiFi.reconnect(); } // tenta reconectar
}
