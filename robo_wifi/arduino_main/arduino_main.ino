/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include "macros.h" // inclui cabeçalho com as macros

bool dado_novo;
uint8_t v, a, r, vel_int, val_mA, val_mB, quadrante;
char vel[4], angulo[4], recebido[16], c;
unsigned  ang_int;

bool caractere_valido(char c)  {
  return ((c >= '0' && c <= '9') || c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void setup() {
  delay(500);
  Serial.begin(9600);
  pinMode(PIN_IN, INPUT);         // inicia os pinos
  pinMode(PIN_OUT, OUTPUT);
  digitalWrite(PIN_OUT, LOW);
  pinMode(LED_BUILTIN, OUTPUT);   // de entrada e saida
  digitalWrite(LED_BUILTIN, LOW); //
}

void loop() {
  if(!digitalRead(PIN_IN)) {
    Serial.end();
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(PIN_OUT, LOW);
    delay(100);
    Serial.begin(9600);
    digitalWrite(PIN_OUT, HIGH);
  }

  if(Serial.available() && Serial.read() == INICIALIZADOR) {
    dado_novo = true;
    r = -1;
    while((c = Serial.read()) != FINALIZADOR)
      if(caractere_valido(c))
        recebido[++ r] = c;

    for(v = 0; recebido[v] != SEPARADOR; v ++)
      vel[v] = recebido[v];
    vel[v] = '\0';

    for(a = v + 1; a <= r; a ++)
      angulo[a - v - 1] = recebido[a];

    angulo[r - v - 1 < 3 ?
            r - v : 3] = '\0';
  }

  if(dado_novo) { // se recebeu um novo dado
    vel_int = atoi(vel);  vel_int = map(vel_int, 0, 100, 0, 255); // transforma a velocidade em um inteiro e, então, em um float entre 0 e 1
    ang_int = atoi(angulo);                  // transforma o angulo em um inteiro

    if(ang_int < 90) quadrante = 1; else if(ang_int < 180) quadrante = 2; else if(ang_int < 270) quadrante = 3; else if(ang_int < 360) quadrante = 4; // determina em qual quadrante o joystick está
    
    if(quadrante == 1 || quadrante == 4) {                                      // se estiver na direita
      val_mB = (uint8_t) vel_int;                                               // o motor B recebe a velocidade indicada no joystick
      val_mA = (uint8_t) (vel_int * (1.0 - cos(ang_int * PI / 180.0) / 2.0)); } // o motor A recebe esta velocidade reduzida proporcionalmente ao cosseno do ângulo
    else {                                                                      // caso não
      val_mB = (uint8_t) (vel_int * (1.0 + cos(ang_int * PI / 180.0) / 2.0));   // o motor B recebe a velocidade acrescida proporcionalmente ao cosseno (que será negativo) do ângulo
      val_mA = (uint8_t) vel_int; }                                             // o motor A recebe a velocidade indicada no joystick

    if(quadrante < 3) {                                     // se estiver em cima/na frente
      analogWrite(INT2_, val_mA); analogWrite(INT1_, 0);    // aciona os motores
      analogWrite(INT4_, val_mB); analogWrite(INT3_, 0);  } // para frente
    else {                                                  // senão
      analogWrite(INT2_, 0); analogWrite(INT1_, val_mA);    // aciona os motores
      analogWrite(INT4_, 0); analogWrite(INT3_, val_mB);  } // para trás
  } dado_novo = false;                                      // indica que o dado já foi tratado
}
