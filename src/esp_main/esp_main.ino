/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include "constantes.h"

#define DEBUG

bool evtConectado, srvRestart, apagarCredencial;

String parametrosVariaveis[qtdArquivos];
String selCred;

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

#ifdef DEBUG
void imprimeTodosArquivos(const char * msg) {
  Serial.print("Inicia imprime todos os arquivos em: "); Serial.println(msg);
  File file[qtdArquivos];
  for(size_t i = 0; i < qtdArquivos; i ++) {
    Serial.print(paramVec[i]);
    file[i] = LittleFS.open(paths[i], "r");
    Serial.print(": "); Serial.println(file[i].size());
    while(file[i].available())
      Serial.print((char) file[i].read());
    Serial.println("-----");
    file[i].close();
  }
  Serial.print("Encerra imprime todos os arquivos em: "); Serial.println(msg);
}
#endif

String modelos(const String& var) {
  String retorno;
  if(var == "modeloLista") {
    File fileSSID = LittleFS.open(paths[iSSID], "r");
    while(fileSSID.available()) {
      String nomeSSID = fileSSID.readStringUntil(caractereFinal);
      retorno += "<option value=" + nomeSSID + ">" + nomeSSID + "</option>";
    }
    fileSSID.close();
  } else if(var == "modeloIP") {
    if(WiFi.getMode() == WIFI_AP)
      retorno = "<p>Robo nao conseguiu se conectar.</p>";
    else
      retorno = "<p>IP: " + WiFi.localIP().toString() + "</p><p>Gateway: " + WiFi.gatewayIP().toString() + "</p>"
                  + (WiFi.getMode() == WIFI_AP_STA ? R"==(<button class="button" id="dAP" onclick="desligaAP()">Desligar AP</button>)==" : "");
  }
  return retorno;
}

inline bool caractereValido(char c) __attribute__((always_inline));
bool caractereValido(char c) {
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
  #ifdef DEBUG
    imprimeTodosArquivos("appendFile");
  #endif
}

void apagaCredencial(fs::FS &fs, const char *credencial) {
  File fileSSID = LittleFS.open(paths[iSSID], "r");
  uint8_t linhaLeitura = 0;
  bool credCadastrada = false;

  while(fileSSID.available() and !credCadastrada)
    if(linhaLeitura ++; fileSSID.readStringUntil(caractereFinal) == credencial)
      credCadastrada = true;

  fileSSID.close();
  if(!credCadastrada) return;

  for(size_t i = 0; i < qtdArquivos; i ++) {
    #ifdef DEBUG
      imprimeTodosArquivos("apagaCredencial1");
    #endif
    uint8_t linhaEscrita = 1;
    String novoConteudo = "";
    char caractereArquivo;
    File file = LittleFS.open(paths[i], "r");
    while(file.available()) {
      caractereArquivo = file.read();
      if(linhaEscrita != linhaLeitura) novoConteudo += caractereArquivo;
      if(caractereArquivo == caractereFinal) linhaEscrita ++;
    }
    file.close();
    file = LittleFS.open(paths[i], "w");
    file.print(novoConteudo);
    if(linhaLeitura == linhaEscrita) file.print(caractereFinal);
    file.close();    
  }
  #ifdef DEBUG
    imprimeTodosArquivos("apagaCredencial2");
  #endif
}

bool initWiFi(bool &deveIniciarAPSTA) {
  #ifdef DEBUG
    imprimeTodosArquivos("initWifi");
  #endif
  WiFi.mode(WIFI_STA); // configura o modo como STA
  bool controleWiFi = false, IPouGatewayVazios = false;

  File file[qtdArquivos];
  for(size_t i = 0; i < qtdArquivos; i ++)
    file[i] = LittleFS.open(paths[i], "r");
  
  while(file[0].available()) {                       //  enquanto houver redes cadastradas a serem verificadas
    for(size_t i = 0; i < qtdArquivos; i ++)
      parametrosVariaveis[i] = file[i].readStringUntil(caractereFinal);

    IPouGatewayVazios = false;
    if(parametrosVariaveis[iIP] == "" or parametrosVariaveis[iGateway] == "") {
      #ifdef DEBUG
        Serial.println("IP e Gateway vazios");
      #endif
      IPouGatewayVazios = true;
    } else {         //  define o Gateway
      IPAddress IPLocal, gatewayLocal;
      IPLocal.fromString(parametrosVariaveis[iIP].c_str());
      gatewayLocal.fromString(parametrosVariaveis[iGateway].c_str());
      if(!WiFi.config(IPLocal, gatewayLocal, IPAddress(255, 255, 0, 0))) continue; //  se falhar na configuracao, pula a parte do loop
    }
    #ifdef DEBUG
    Serial.print("rede testada por initWiFi:");
    for(size_t i = 0; i < qtdArquivos; i ++) {
      Serial.print(' '); Serial.print(parametrosVariaveis[i]);
    } 
    Serial.println(" encerra detalhes da rede testada por initWiFi:");
    #endif
    WiFi.begin(parametrosVariaveis[iSSID].c_str(), parametrosVariaveis[iPass].c_str());           //  inicia o WiFi
    delay(tempoInicioWiFi);                           //  aguarda o tempo estimado
    if(controleWiFi = (WiFi.status() == WL_CONNECTED)) break; //  se conectar, sai do loop
  }

  for(size_t i = 0; i < qtdArquivos; i ++)
    file[i].close();
  #ifdef DEBUG
  if(controleWiFi) {
    Serial.print("rede conectada por initWiFi:");
    for(size_t i = 0; i < qtdArquivos; i ++) {
      Serial.print(' '); Serial.print(parametrosVariaveis[i]);
    }
    Serial.print(" IP real: "); Serial.print(WiFi.localIP());
    Serial.print(" Gateway real: "); Serial.print(WiFi.gatewayIP());
    Serial.println(" encerra detalhes da rede conectada por initWiFi:");
  } else
    Serial.println("Nao conectou :(");
  #endif
  if(controleWiFi and IPouGatewayVazios)
    deveIniciarAPSTA = true;
  return controleWiFi;  // retorna o estado da conexao
}

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  #endif

  pinMode(pinIn, INPUT);      // inicia a entrada
  pinMode(pinOut, OUTPUT);    // e a saída
  digitalWrite(pinOut, HIGH); // digitais

  bool deveIniciarAPSTA = false;
  evtConectado = srvRestart = apagarCredencial = false;

  LittleFS.begin(); // inicia o sistema de arquivos

  ws.onEvent(onWsEvent);  // indica qual função deve ser chamada ao perceber um evento
  server.addHandler(&ws); // indica que o servidor será tratado de acordo com o WebSocket

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // quando alguém se conectar ao servidor do joystick
      request->send(LittleFS, "/index.html");  });             // envia o script salvo em /index.html

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
  
  server.on("/WM", HTTP_GET, [](AsyncWebServerRequest *request) {  // quando se conectar à pagina do gerenciador
    request->send(LittleFS, "/wifimanager.html", "text/html", false, modelos); }); // envia /wifimanager.html
  
  server.on("/managerStyle.css", HTTP_GET, [](AsyncWebServerRequest *request) { // indica o arquivo
    request->send(LittleFS, "/managerStyle.css", "text/css"); });               // de elementos estéticos
  
  server.on("/Cadastra", HTTP_POST, [](AsyncWebServerRequest *request) {
    uint8_t params = request->params();                         //  registra a quantidade de parametros
    #ifdef DEBUG
      Serial.print("qtd de parametros: "); Serial.println(params);
    #endif
    for(uint8_t i = 0; i < params; i ++) {                      //  para cada parametro
      AsyncWebParameter* p = request->getParam(i);              //  registra o parametro
      #ifdef DEBUG
        Serial.print("parametro cadastrado: ");
        Serial.print(p->name()); Serial.print(' ');
      #endif
        for(size_t j = 0; j < qtdArquivos; j ++) {
          if(p->name() == paramVec[j]) {
            #ifdef DEBUG
              Serial.println(p->value()); 
            #endif
            parametrosVariaveis[j] = p->value();
            appendFile(LittleFS, paths[j], parametrosVariaveis[j].c_str());
          }
        }
    }
    #ifdef DEBUG
      Serial.println("/-----/");
    #endif
    request->send(200, "text/plain", "Credenciais cadastradas!"); // gera uma página avisando que as credenciais foram atualizadas
    srvRestart = true;  // registra que o chip deve reiniciar
  });

  server.on("/Apaga", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(AsyncWebParameter* p = request->getParam(0); p->name() == paramInputCred) {
      selCred = p->value();
      #ifdef DEBUG
        Serial.print("Rede deletada: "); Serial.println(selCred);
      #endif
      apagarCredencial = true;
    }
    request->send(200, "text/plain", "Credenciais apagadas!"); // gera uma página avisando que as credenciais foram atualizadas
    srvRestart = true;  // registra que o chip deve reiniciar
  });

  server.on("/dAP", HTTP_GET, [](AsyncWebServerRequest *request) {
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);
    #ifdef DEBUG
      Serial.print("AP desligada");
    #endif
    request->send(200);
  });

  if((!initWiFi(deveIniciarAPSTA)) or deveIniciarAPSTA) {                       // se não conseguir se conectar a uma rede
    WiFi.mode(deveIniciarAPSTA ? WIFI_AP_STA : WIFI_AP);
    WiFi.softAP("ROBO_ELETROGATE", NULL); // inicia a AP
    #ifdef DEBUG
      Serial.print("modo: "); Serial.println(WiFi.getMode());
    #endif
  }
  
  server.begin();     // inicia o servidor

  #ifndef DEBUG
  Serial.begin(9600); // inicia a serial
  #endif
}

void loop() {

  static bool controleAutoRec = false;
  static unsigned tempoDecorrido = 0;

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
//           onde eh usada (pra tentar reduzir globais | x -> eh global | v -> virou local| i -> inicializa mas n usa)
// variavel              setup loop onWsEvent apagaCredencial initWifi
// controleAutoRec       		   v	     		
// evtConectado          	i	   x	     x                           -> usa em callback
// srvRestart            	x	   x	                  		           -> usa em callback
// controleWiFi          		   		                  	          v
// credCadastrada        		   		                  v	          
// apagarCredencial      	x	   x	                  	          	
// IPouGatewayVazios     		   		                  	          v
// deveIniciarAPSTA      	v	   		                  	          
// tempoDecorrido        		   v	                  	          	
// parametrosVariaveis[] 	x	   		                  	          x  -> usa em callback
// selCred               	x	   x	                  	          	 -> pra compartilhar precisaria de funcao global
// server                	x	   		                  	             -> precisa ser global
// ws                    	x	   		                  	             -> precisa ser global
// localIP               		   		                  	          v
// localGateway          		   		                  	          v
// subnet                		   		                  	          v

// testar agora: se joystick continua funcionando e se a seleção entre sta, ap_sta e ap está funcionando de acordo com o que ocorre na conexao

// se desconectar, reconectar e não tiver escolhido o IP manualmente, abrir a AP pra ver o IP
// adicionar uma forma de resetar as credenciais pelo hardware (provavelmente enviar alguma coisa pela uno para o rx do esp)
// colocar debug nas estruturas referentes à robustes do wifi, mesmo que estejam funcionando legal
// convem passar littleFs como argumento? Permite usar SPIFFS, né. 
