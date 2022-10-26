/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include "constantes.h"

bool controleAutoRec;  //
bool evtConectado;     // declara e inicia as variaveis
bool srvRestart;       // de controle
bool controleWiFi;     //
bool credCadastrada;   //
bool apagarCredencial; //
bool IPouGatewayVazios;
bool deveIniciarAP;
unsigned tempoDecorrido = 0;

String ssid;    //
String pass;    // declara as variaveis
String ip;      // de credenciais
String gateway; //
String selCred; //

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

IPAddress localIP;                // declara as variaveis
IPAddress localGateway;           // de conexão
IPAddress subnet(255, 255, 0, 0); //

String listaRedes(const String& var) {
  String retorno;
  File fileSSID = LittleFS.open(ssidPath, "r");
  if(var == "modelo")
    while(fileSSID.available()) {
      String nomeSSID = fileSSID.readStringUntil('\n');
      retorno += "<option value=" + nomeSSID + ">" + nomeSSID + "</option>";
    }
  fileSSID.close();
  return retorno;
}

String modeloIPeG(const String& var) {
  String retorno;
  File fileSSID = LittleFS.open(ssidPath, "r");
  if(var == "modelo")
    while(fileSSID.available()) {
      String nomeSSID = fileSSID.readStringUntil('\n');
      retorno += "<option value=" + nomeSSID + ">" + nomeSSID + "</option>";
    }
  fileSSID.close();
  return retorno;
}

bool caractereValido(char c)  {
  return ((c >= '0' and c <= '9') or c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)  { // definição da tratativa de evento de servidor assíncrono
  if(type == WS_EVT_DATA) {                                                       // se for o recebimento de dados e o Arduino puder receber
    char *data_ = (char*) data;                                                   // registra todos os dados recebidos do servidor
    Serial.write(caractereInicio);                                                // envia o caractere de inicio de transmissao
    for(uint8_t i = 0; caractereValido(data_[i]) and i < tamanhoMaximoDados; i ++) // enquanto for um caractere valido e o tamanho for inferior ao maximo
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

void apagaCredencial(fs::FS &fs, const char *credencial) {
  File fileSSID = LittleFS.open(ssidPath, "r");
  uint8_t linhaLeitura = 0;
  uint8_t linhaEscrita = 1;
  credCadastrada = false;
  while(fileSSID.available() and !credCadastrada)
    if(linhaLeitura ++; fileSSID.readStringUntil('\n') == credencial)
      credCadastrada = true;

  fileSSID.close();
  if(!credCadastrada) return;

  String novoConteudo = "";
  char caractereArquivo;
  fileSSID = LittleFS.open(ssidPath, "r");
  while(fileSSID.available()) {
    caractereArquivo = fileSSID.read();
    if(linhaEscrita != linhaLeitura) novoConteudo += caractereArquivo;
    if(caractereArquivo == '\n') linhaEscrita ++;
  }
  fileSSID.close();
  fileSSID = LittleFS.open(ssidPath, "w");
  fileSSID.print(novoConteudo);
  if(linhaLeitura == linhaEscrita) fileSSID.print('\n');
  fileSSID.close();

  linhaEscrita = 1;
  novoConteudo = "";
  File filePass = LittleFS.open(passPath, "r");
  while(filePass.available()) {
    caractereArquivo = filePass.read();
    if(linhaEscrita != linhaLeitura) novoConteudo += caractereArquivo;
    if(caractereArquivo == '\n') linhaEscrita ++;
  }
  filePass.close();
  filePass = LittleFS.open(passPath, "w");
  filePass.print(novoConteudo);
  if(linhaLeitura == linhaEscrita) filePass.print('\n');
  filePass.close();

  linhaEscrita = 1;
  novoConteudo = "";
  File fileIP = LittleFS.open(ipPath, "r");
  while(fileIP.available()) {
    caractereArquivo = fileIP.read();
    if(linhaEscrita != linhaLeitura) novoConteudo += caractereArquivo;
    if(caractereArquivo == '\n') linhaEscrita ++;
  }
  fileIP.close();
  fileIP = LittleFS.open(ipPath, "w");
  fileIP.print(novoConteudo);
  if(linhaLeitura == linhaEscrita) fileIP.print('\n');
  fileIP.close();

  linhaEscrita = 1;
  novoConteudo = "";
  File fileGateway = LittleFS.open(gatewayPath, "r");
  while(fileGateway.available()) {
    caractereArquivo = fileGateway.read();
    if(linhaEscrita != linhaLeitura) novoConteudo += caractereArquivo;
    if(caractereArquivo == '\n') linhaEscrita ++;
  }
  fileGateway.close();
  fileGateway = LittleFS.open(gatewayPath, "w");
  fileGateway.print(novoConteudo);
  if(linhaLeitura == linhaEscrita) fileGateway.print('\n');
  fileGateway.close();
}

bool initWiFi() {
  WiFi.mode(WIFI_STA); // configura o modo como STA
  
  File fileSSID = LittleFS.open(ssidPath, "r");       //
  File filePass = LittleFS.open(passPath, "r");       //  abre os arquivos
  File fileIP = LittleFS.open(ipPath, "r");           //  para leitura
  File fileGateway = LittleFS.open(gatewayPath, "r"); //
  while(fileSSID.available()) {                       //  enquanto houver redes cadastradas a serem verificadas
    ssid = fileSSID.readStringUntil('\n');            //  le a linha
    pass = filePass.readStringUntil('\n');            //  atual
    ip = fileIP.readStringUntil('\n');                //  de cada arquivo
    gateway = fileGateway.readStringUntil('\n');      //
    IPouGatewayVazios = false;
    if(ip == "" or gateway == "") {
      Serial.println("ish ficou vazio");
      IPouGatewayVazios = true;
    } else {
      localIP.fromString(ip.c_str());                   //  define o IP
      localGateway.fromString(gateway.c_str());         //  define o Gateway
      if(!WiFi.config(localIP, localGateway, subnet)) continue; //  se falhar na configuracao, pula a parte do loop
    }
    WiFi.begin(ssid.c_str(), pass.c_str());           //  inicia o WiFi
    delay(tempoInicioWiFi);                           //  aguarda o tempo estimado
    if(controleWiFi = (WiFi.status() == WL_CONNECTED)) break; //  se conectar, sai do loop
  }
  fileSSID.close();     //
  filePass.close();     // fecha os arquivos
  fileIP.close();       //
  fileGateway.close();  //
  if(controleWiFi and IPouGatewayVazios) {
    deveIniciarAP = true;
    server.on("/IP", HTTP_GET, [](AsyncWebServerRequest *request) {                  // quando alguém se conectar ao servidor do ip
      request->send(LittleFS, "/IP.html", "text/html", false, modeloIPeGateway);  }); // envia o IP recebido pelo roteador
  }
  return controleWiFi;  // retorna o estado da conexao
}

void setup() {

  Serial.begin(9600); // inicia a serial
  pinMode(pinIn, INPUT);      // inicia a entrada
  pinMode(pinOut, OUTPUT);    // e a saída
  digitalWrite(pinOut, HIGH); // digitais

  controleAutoRec = false; 
  evtConectado = false;    
  srvRestart = false;      
  controleWiFi = false;    
  credCadastrada = false;  
  apagarCredencial = false;
  deveIniciarAP = false;

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
    request->send(LittleFS, "/wifimanager.html", "text/html", false, listaRedes); }); // envia /wifimanager.html
  
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

        if(p->name() == paramInputCred) {
          selCred = p->value();
          apagarCredencial = true;
        }
      }
    }
    srvRestart = true;  // registra que o chip deve reiniciar
    request->send(200, "text/plain", "Credenciais atualizadas!"); // gera uma página avisando que as credenciais foram atualizadas
  });  

  if((!initWiFi()) or deveIniciarAP) {                       // se não conseguir se conectar a uma rede
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ROBO_ELETROGATE", NULL); // inicia a AP
  }
  
  server.begin();     // inicia o servidor
  //Serial.begin(9600); // inicia a serial
}

void loop() {
  if(srvRestart and !apagarCredencial) ESP.restart(); // se for para o sistema reiniciar, reinicia

  if(apagarCredencial) {
    apagaCredencial(LittleFS, selCred.c_str());
    apagarCredencial = false;
  }

  if(evtConectado and digitalRead(pinIn)) { // se houve conexão à página
    digitalWrite(pinOut, HIGH);             // envia um sinal digital de nível alto
    evtConectado = false;                   // reseta a variavel de controle
  }

  if(WiFi.getMode() == WIFI_STA and WiFi.status() == WL_CONNECTED and controleAutoRec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);  // ativa a autoreconexão
    WiFi.persistent(true);        // ativa a persistência
    controleAutoRec = true;       // indica que a robustez de WiFi já foi configurada após a reconexão
  }

  if(controleAutoRec == true and (WiFi.getMode() != WIFI_STA or WiFi.status() != WL_CONNECTED))   // se estiver desconectado ou em modo diferente de STA
    controleAutoRec = false;      // indica que houve a desconexão

  if(millis() - tempoDecorrido >= intervaloWiFi) { // a cada intervaloWiFi ms
    tempoDecorrido = millis(); // atualiza o tempo
    if(WiFi.status() != WL_CONNECTED and WiFi.getMode() == WIFI_STA) // se estiver desconectado e em modo STA
      WiFi.reconnect();   // tenta reconectar
  }
}
