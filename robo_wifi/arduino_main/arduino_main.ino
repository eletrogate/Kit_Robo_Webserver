#include "macros.h" // inclui cabeçalho com as macros

bool dado_novo;                                  //
unsigned short i, v, a, q;                       // declara variaveis
char vel[4], angulo[4];                          // que serão usadas
unsigned short vel_int, ang_int, val_mA, val_mB; // ao longo do código
float speed;                                     //
char aux;                                        //
byte quadrante;                                  //

void setup() {
  delay(500);                     // aguarda a estabilização do sistema para iniciar a serial
  Serial.begin(9600);             // inicia a serial
  pinMode(PIN_OUT, OUTPUT);       // 
  pinMode(PIN_IN, INPUT);         // inicia os pinos
  pinMode(LED_BUILTIN, OUTPUT);   // de entrada e saida
  digitalWrite(LED_BUILTIN, LOW); //
  
  while(!Serial.available()); aux = Serial.read(); // espera a primeira conexão e descarta o ruído
  
  digitalWrite(PIN_OUT, HIGH);    // avisa ao ESP que pode receber dados
}

void loop() {
  if(Serial.available())  {                         // se houverem dados
      dado_novo = true; digitalWrite(PIN_OUT, LOW); // registra que há dados novos e avisa o ESP para parar de enviar
      
      LESERIAL(aux)                                                 // le o primeiro dígito
      for(i = 0; aux != ' ' && aux != CARACTERE_SEPARADOR; i ++)  { // enquanto "aux" for um dígito
        vel[i] = aux;                                               // o adiciona à velocidade
        v = i;                                                      // registra a quantidade de dígitos da velocidade
        LESERIAL(aux)  }                                            // lê o próximo caractere

      LESERIAL(aux)                                                 // descarta o caractere anterior
      for(i = 0; aux != CARACTERE_SEPARADOR && i < 4; i ++)  {      // enquanto "aux" for um dígito
        angulo[i] = aux;                                            // o adiciona ao ângulo
        a = i;                                                      // registra a quantidade de dígitos do ângulo
        LESERIAL(aux)  }                                            // lê o próximo caractere

      digitalWrite(PIN_OUT, HIGH);  // avisa ao ESP que pode receber dados
    }
    

  if(dado_novo) { // se recebeu um novo dado
    if(v < 3) vel[v + 1] = '\0'; else vel[3] = '\0';  vel_int = atoi(vel);  speed = vel_int / 100.0; // transforma a velocidade em um inteiro e, então, em um float entre 0 e 1
    if(a < 3) angulo[a + 1] = '\0'; else angulo[3] = '\0';  ang_int = atoi(angulo);                  // transforma o angulo em um inteiro

  if(ang_int < 90) quadrante = 1; else if(ang_int < 180) quadrante = 2; else if(ang_int < 270) quadrante = 3; else if(ang_int < 360) quadrante = 4; // determina em qual quadrante o joystick está
  
  if(quadrante == 1 || quadrante == 4) {                                      // se estiver na direita
    val_mB = (int) (speed * 255);                                             // o motor B recebe a velocidade indicada no joystick
    val_mA = (int) (speed * 255 * (1.0 - cos(ang_int * PI / 180.0) / 2.0)); } // o motor A recebe esta velocidade reduzida proporcionalmente ao cosseno do ângulo
  else {                                                                      // caso não
    val_mB = (int) (speed * 255 * (1.0 + cos(ang_int * PI / 180.0) / 2.0));   // o motor B recebe a velocidade acrescida proporcionalmente ao cosseno (que será negativo) do ângulo
    val_mA = (int) (speed * 255); }                                           // o motor A recebe a velocidade indicada no joystick

  if(quadrante < 3) {                                     // se estiver em cima/na frente
    analogWrite(INT2_, val_mA); analogWrite(INT1_, 0);    // aciona os motores
    analogWrite(INT4_, val_mB); analogWrite(INT3_, 0);  } // para frente
  else {                                                  // senão
    analogWrite(INT2_, 0); analogWrite(INT1_, val_mA);    // aciona os motores
    analogWrite(INT4_, 0); analogWrite(INT3_, val_mB);  } // para trás
  } dado_novo = false;                                    // indica que o dado já foi tratado

  if(digitalRead(PIN_IN)) {                               // em caso de perda de conexão
    analogWrite(INT1_, 0); analogWrite(INT2_, 0);         // freia todos
    analogWrite(INT4_, 0); analogWrite(INT3_, 0);         // os motores
  }
}