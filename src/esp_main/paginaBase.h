#ifndef PB_H
#define PB_H

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <fs.h>

String modelos(const String& var);

void constroiPag(AsyncWebServer &server, fs::FS &fs) {
  server.on("/", HTTP_GET, [&fs](AsyncWebServerRequest *request){ // quando alguém se conectar ao servidor do joystick
    request->send(fs, "/index.html");  });                        // envia o script salvo em /index.html

  server.on("/style.css", HTTP_GET, [&fs](AsyncWebServerRequest *request) {
    request->send(fs, "/style.css", "text/css"); });
  
  server.on("/joystick.js", HTTP_GET, [&fs](AsyncWebServerRequest *request){
    request->send(fs, "/joystick.js", "text/javascript"); });

  server.on("/icone.ico", HTTP_GET, [&fs](AsyncWebServerRequest *request){
    request->send(fs, "/icone.ico", "icone/ico"); });

  server.on("/escritaHeader.png", HTTP_GET, [&fs](AsyncWebServerRequest *request){
  request->send(fs, "/escritaHeader.png", "image/png"); });

  server.on("/logoFundo.png", HTTP_GET, [&fs](AsyncWebServerRequest *request){
    request->send(fs, "/logoFundo.png", "image/png"); });

  server.on("/loja.png", HTTP_GET, [&fs](AsyncWebServerRequest *request){
    request->send(fs, "/loja.png", "image/png"); });

  server.on("/blog.png", HTTP_GET, [&fs](AsyncWebServerRequest *request){
    request->send(fs, "/blog.png", "image/png"); });

  server.on("/wifi.png", HTTP_GET, [&fs](AsyncWebServerRequest *request){
    request->send(fs, "/wifi.png", "image/png"); });
  
  server.on("/WM", HTTP_GET, [&fs](AsyncWebServerRequest *request) {         // quando se conectar à pagina do gerenciador
    request->send(fs, "/wifimanager.html", "text/html", false, modelos); }); // envia /wifimanager.html
  
  server.on("/managerStyle.css", HTTP_GET, [&fs](AsyncWebServerRequest *request) {
    request->send(fs, "/managerStyle.css", "text/css"); });
}

#endif
