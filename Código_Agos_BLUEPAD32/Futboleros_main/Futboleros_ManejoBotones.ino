// Librerias
#include <Arduino.h>
#include <Bluepad32.h>
#include "config.h"

// Adelante con triangulo
void adelante() {
  switch(nivelActual){
    case 0:
      ledcWrite(CH_ADELANTE_IZQ, 180);
      ledcWrite(CH_ADELANTE_DER, 180); // + K);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
    break;
    case 1:
      ledcWrite(CH_ADELANTE_IZQ, 206);
      ledcWrite(CH_ADELANTE_DER, 206); // + K);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
    break;
    case 2:
      ledcWrite(CH_ADELANTE_IZQ, PWM_MAX);
      ledcWrite(CH_ADELANTE_DER, PWM_MAX); // + K);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
    break;
  }
    return;
}

// Atrás con cruz
void atras() {
  switch(nivelActual){
    case 0:
      ledcWrite(CH_ADELANTE_IZQ, 0);
      ledcWrite(CH_ADELANTE_DER, 0); // + K);
      ledcWrite(CH_ATRAS_IZQ, 180);
      ledcWrite(CH_ATRAS_DER, 180);
    break;
    case 1:
      ledcWrite(CH_ADELANTE_IZQ, 0);
      ledcWrite(CH_ADELANTE_DER, 0); // + K);
      ledcWrite(CH_ATRAS_IZQ, 206);
      ledcWrite(CH_ATRAS_DER, 206);
    break;
    case 2:
      ledcWrite(CH_ADELANTE_IZQ, 0);
      ledcWrite(CH_ADELANTE_DER, 0); // + K);
      ledcWrite(CH_ATRAS_IZQ, PWM_MAX);
      ledcWrite(CH_ATRAS_DER, PWM_MAX);
    break;
  }
    return;
}

// En los viejos y la bestia 1.0 funcion izq y der al reves

// Izquierda con círculo
void izquierda() {
  switch(nivelActual){
    case 0:
      ledcWrite(CH_ADELANTE_IZQ, 180);
      ledcWrite(CH_ADELANTE_DER, 0);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
    break;
    case 1:
      ledcWrite(CH_ADELANTE_IZQ, 206);
      ledcWrite(CH_ADELANTE_DER, 0);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
    break;
    case 2:
      ledcWrite(CH_ADELANTE_IZQ, PWM_MAX);
      ledcWrite(CH_ADELANTE_DER, 0);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
    break;
  }
    return;
}

// Derecha con cuadrado
void derecha() {
  switch(nivelActual){
    case 0:
      ledcWrite(CH_ADELANTE_DER, 180); // + K);
      ledcWrite(CH_ADELANTE_IZQ, 0);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);    
      break;
    case 1:
      ledcWrite(CH_ADELANTE_DER, 206); // + K);
      ledcWrite(CH_ADELANTE_IZQ, 0);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
      break;
    case 2:
      ledcWrite(CH_ADELANTE_DER, PWM_MAX); // + K);
      ledcWrite(CH_ADELANTE_IZQ, 0);
      ledcWrite(CH_ATRAS_IZQ, 0);
      ledcWrite(CH_ATRAS_DER, 0);
      break;
    }
    return;
}

// FUNCIONES DE ANTIREBOTE PARA EVITAR DATOS BASURA
void antiReboteTriang (ControllerPtr ctl){
  static unsigned long ultimoTriangulo = 0;
  const unsigned long tiempoRebote = 200;

  if (ctl->y() && millis() - ultimoTriangulo > tiempoRebote) {
    adelante();
    ultimoTriangulo = millis();
  }
}

void antiReboteCruz (ControllerPtr ctl){
  static unsigned long ultimoCruz = 0;
  const unsigned long tiempoRebote = 200;

  if (ctl->a() && millis() - ultimoCruz > tiempoRebote) {
    atras();
    ultimoCruz = millis();
  }
}

void antiReboteCirc (ControllerPtr ctl){
  static unsigned long ultimoCirculo = 0;
  const unsigned long tiempoRebote = 200;

  if (ctl->b() && millis() - ultimoCirculo > tiempoRebote) {
    izquierda();
    ultimoCirculo = millis();
  }
}

void antiReboteCuad (ControllerPtr ctl){
  static unsigned long ultimoCuadrado = 0;
  const unsigned long tiempoRebote = 200;

  if (ctl->x() && millis() - ultimoCuadrado > tiempoRebote) {
    derecha();
    ultimoCuadrado = millis();
  }
}
