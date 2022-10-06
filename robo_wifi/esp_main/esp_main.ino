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

String readFile(fs::FS &fs, const char *path) {
  File file = fs.open(path, "r");                                 // abre o arquivo para leitura
  String fileContent;                                             // declara variavel para armazenamento do conteudo
  if(file.available()) fileContent = file.readStringUntil('\n');  // se disponivel, salva o conteudo na variavel
  file.close();                                                   // fecha o arquivo
  return fileContent;                                             // retorna o conteudo
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, "w"); // abre o arquivo para escrita
  file.print(message);            // escreve o conteudo no arquivo                          
  file.close();                   // fecha o arquivo
}

bool initWiFi() {
  WiFi.mode(WIFI_STA);                                            // configura o modo como STA
  localIP.fromString(ip.c_str());                                 // define o IP
  localGateway.fromString(gateway.c_str());                       // define o Gateway

  if (!WiFi.config(localIP, localGateway, subnet)) return false;  // se a configuração falhar, retorna false

  WiFi.begin(ssid.c_str(), pass.c_str());                         // tenta se conectar
  delay(tempoInicioWiFi);                                         // aguarda o tempo estimado para conexao
  return WiFi.status() == WL_CONNECTED;                           // retorna o estado da conexao
}

void setup() {
  pinMode(pinIn, INPUT);      // inicia a entrada
  pinMode(pinOut, OUTPUT);    // e a saída
  digitalWrite(pinOut, HIGH); // digitais

  LittleFS.begin(); // inicia o sistema de arquivos

  ssid = readFile(LittleFS, ssidPath);        // le o arquivo com o nome da rede e salva seu conteudo na variavel
  pass = readFile(LittleFS, passPath);        // le o arquivo com a senha da rede e salva seu conteudo na variavel
  ip = readFile(LittleFS, ipPath);            // le o arquivo com o ip e salva seu conteudo na variavel
  gateway = readFile (LittleFS, gatewayPath); // le o arquivo com o gateway e salva seu conteudo na variavel

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

  if(!initWiFi()) {
    WiFi.softAP("ROBO_ELETROGATE", NULL); // inicia a AP

    server.on("/WM", HTTP_GET, [](AsyncWebServerRequest *request) {  // quando se conectar à pagina do gerenciador
      request->send(LittleFS, "/wifimanager.html", "text/html"); }); // envia /wifimanager.html
    
    server.on("/manager_style.css", HTTP_GET, [](AsyncWebServerRequest *request) { // indica o arquivo
      request->send(LittleFS, "/manager_style.css", "text/css"); });               // de elementos estéticos
    
    server.on("/WM", HTTP_POST, [](AsyncWebServerRequest *request) {
      uint8_t params = request->params();                         //  registra a quantidade de parametros
      for(uint8_t i = 0; i < params; i ++) {                      //  para cada parametro
        AsyncWebParameter* p = request->getParam(i);              //  registra o parametro
        if(p->isPost()) {                                         //  se for post
          if(p->name() == paramInput1) {                          //  se o nome estiver de acordo
            ssid = p->value();                                    //  registra o dado
            writeFile(LittleFS, ssidPath, ssid.c_str()); }        //  escreve no sistema de arquivos

          if(p->name() == paramInput2) {                          //  se o nome estiver de acordo
            pass = p->value();                                    //  registra o dado
            writeFile(LittleFS, passPath, pass.c_str()); }        //  escreve no sistema de arquivos

          if(p->name() == paramInput3) {                          //  se o nome estiver de acordo
            ip = p->value();                                      //  registra o dado
            writeFile(LittleFS, ipPath, ip.c_str()); }            //  escreve no sistema de arquivos

          if(p->name() == paramInput4) {                          //  se o nome estiver de acordo
            gateway = p->value();                                 //  registra o dado
            writeFile(LittleFS, gatewayPath, gateway.c_str()); }  //  escreve no sistema de arquivos
        }
      }
      srvRestart = true;  // registra que o chip deve reiniciar
      request->send(200, "text/plain", "Credenciais cadastradas!"); // gera uma página avisando que as credenciais foram cadastradas
    });
  }
  
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
