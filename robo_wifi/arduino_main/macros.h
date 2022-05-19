#define LESERIAL(A)  {q=0; while(!Serial.available()) {if(q>49) break; delay(1); q++;} A=Serial.read();}
        // aguarda, por 50 ms, por algum dado na serial. Se receber, o salva na variavel passada. Se não, sai do laço.
#define INT1_  3
#define INT2_  5
#define INT3_  9
#define INT4_  10
#define PIN_IN  4
#define PIN_OUT 2
#define CARACTERE_SEPARADOR 'X'