#ifndef CONST_H
#define CONST_H

const uint8_t pinIn = 2;
const uint8_t pinOut = 0;
const uint8_t pinRX = 3;
const uint8_t tamanhoMaximoDados = 7;
const uint8_t iSSID = 0, iPass = 1, iIP = 2, iGateway = 3;
const unsigned tempoInicioWiFi = 5000;
const unsigned intervaloWiFi = 15000;
const uint8_t qtdArquivos = 4;
const char caractereInicio = ':';
const char caractereFinal = '\n';
const char *paramInputCred = "cred";
const char *nomeAP = "ROBO_ELETROGATE", *senhaAP = "123456789";
const char paramVec[][8] = {"ssid", "pass", "ip", "gateway"};
const char paths[][13] = {"/ssid.txt" ,"/pass.txt" ,"/ip.txt" ,"/gateway.txt"};

#endif