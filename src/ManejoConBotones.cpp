// Librerias
#include <PS4Controller.h>
#include <Arduino.h>
#include <ManejoConBotones.hpp>
#include <Estrategias.hpp>

// Adelante con triangulo
void adelante() {
  switch(nivelActual){
    case 0:
      ledcWrite(Adelante_Izq, 180);
      ledcWrite(Adelante_Der, 180); // + K);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);
    break;
    case 1:
      ledcWrite(Adelante_Izq, 206);
      ledcWrite(Adelante_Der, 206); // + K);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);
    break;
    case 2:
      ledcWrite(Atras_Izq, PWM_MAX);
      ledcWrite(Atras_Der, PWM_MAX); // + K);
      ledcWrite(Adelante_Izq, 0);
      ledcWrite(Adelante_Der, 0);
    break;
  }
    return;
}

// Atrás con cruz
void atras() {
  switch(nivelActual){
    case 0:
      ledcWrite(Adelante_Izq, 0);
      ledcWrite(Adelante_Der, 0); // + K);
      ledcWrite(Atras_Izq, 180);
      ledcWrite(Atras_Der, 180);
    break;
    case 1:
      ledcWrite(Adelante_Izq, 0);
      ledcWrite(Adelante_Der, 0); // + K);
      ledcWrite(Atras_Izq, 206);
      ledcWrite(Atras_Der, 206);
    break;
    case 2:
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0); // + K);
      ledcWrite(Adelante_Izq, PWM_MAX);
      ledcWrite(Adelante_Der, PWM_MAX);
    break;
  }
    return;
}

// Izquierda con círculo
void izquierda() {
  switch(nivelActual){
    case 0:
      ledcWrite(Adelante_Izq, 180);
      ledcWrite(Adelante_Der, 0);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);
    break;
    case 1:
      ledcWrite(Adelante_Izq, 206);
      ledcWrite(Adelante_Der, 0);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);
    break;
    case 2:
      ledcWrite(Adelante_Izq, PWM_MAX);
      ledcWrite(Adelante_Der, 0);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);
    break;
  }
    return;
}

// Derecha con cuadrado
void derecha() {
  switch(nivelActual){
    case 0:
      ledcWrite(Adelante_Der, 180); // + K);
      ledcWrite(Adelante_Izq, 0);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);    break;
    case 1:
    ledcWrite(Adelante_Der, 206); // + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
    break;
    case 2:
      ledcWrite(Adelante_Der, PWM_MAX); // + K);
      ledcWrite(Adelante_Izq, 0);
      ledcWrite(Atras_Izq, 0);
      ledcWrite(Atras_Der, 0);
    break;
  }
    return;
}

// FUNCIONES DE ANTIREBOTE PARA EVITAR DATOS BASURA

void antiReboteTriang (){
  unsigned long ultimoTriangulo = 0;
  const unsigned long tiempoRebote = 200;

  if (PS4.Triangle() && millis() - ultimoTriangulo > tiempoRebote) {
    adelante();
    ultimoTriangulo = millis();
  }
}

void antiReboteCruz (){
  unsigned long ultimoCruz = 0;
  if (PS4.Cross() && millis() - ultimoCruz > tiempoRebote) {
    atras();
    ultimoCruz = millis();
  }
}

void antiReboteCirc (){
  unsigned long ultimoCirculo = 0;
  if (PS4.Circle() && millis() - ultimoCirculo > tiempoRebote) {
    izquierda();
    ultimoCirculo = millis();
  }
}

void antiReboteCuad (){
  unsigned long ultimoCuadrado = 0;
  if (PS4.Square() && millis() - ultimoCuadrado > tiempoRebote) {
    derecha();
    ultimoCuadrado = millis();
  }
}
