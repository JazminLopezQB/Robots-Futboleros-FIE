// Librerias
#include <Arduino.h>
#include <Bluepad32.h>
#include "config.h"
#include "DeclaracionDeFunciones.h"

// Adelante con triangulo
void adelante() {
  switch(nivelActual){
    case 0:
        escribirPWM(
            180,  // adelante izquierda
            0,  // atrás izquierda
            180,  // adelante derecha
            0   // atrás derecha
        );
    break;
    case 1:
        escribirPWM(
            206,  // adelante izquierda
            0,  // atrás izquierda
            206,  // adelante derecha
            0   // atrás derecha
        );
    break;
    case 2:
        escribirPWM(
            PWM_MAX,  // adelante izquierda
            0,  // atrás izquierda
            PWM_MAX,  // adelante derecha
            0   // atrás derecha
        );
    break;
  }
  return;
}

// Atrás con cruz
void atras() {
  switch(nivelActual){
    case 0:
        escribirPWM(
            0,  // adelante izquierda
            180,  // atrás izquierda
            0,  // adelante derecha
            180   // atrás derecha
        );
    break;
    case 1:
        escribirPWM(
            0,  // adelante izquierda
            206,  // atrás izquierda
            0,  // adelante derecha
            206   // atrás derecha
        );
    break;
    case 2:
        escribirPWM(
            0,  // adelante izquierda
            PWM_MAX,  // atrás izquierda
            0,  // adelante derecha
            PWM_MAX   // atrás derecha
        );
    break;
  }
  return;
}

// En los viejos y la bestia 1.0 funcion izq y der al reves

// Izquierda con círculo
void izquierda() {
  switch(nivelActual){
    case 0:
        escribirPWM(
            180,  // adelante izquierda
            0,  // atrás izquierda
            0,  // adelante derecha
            0   // atrás derecha
        );
    break;
    case 1:
        escribirPWM(
            206,  // adelante izquierda
            0,  // atrás izquierda
            0,  // adelante derecha
            0   // atrás derecha
        );
    break;
    case 2:
        escribirPWM(
            PWM_MAX,  // adelante izquierda
            0,  // atrás izquierda
            0,  // adelante derecha
            0   // atrás derecha
        );
    break;
  }
    return;
}

// Derecha con cuadrado
void derecha() {
  switch(nivelActual){
    case 0:
        escribirPWM(
            0,  // adelante izquierda
            0,  // atrás izquierda
            180,  // adelante derecha
            0   // atrás derecha
        );
      break;
    case 1:
        escribirPWM(
            0,  // adelante izquierda
            0,  // atrás izquierda
            206,  // adelante derecha
            0   // atrás derecha
        );
      break;
    case 2:
        escribirPWM(
            0,  // adelante izquierda
            0,  // atrás izquierda
            PWM_MAX,  // adelante derecha
            0   // atrás derecha
        );
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
