// Librerias
#include <Arduino.h>
#include <Bluepad32.h>
#include "config.h"
#include "DeclaracionDeFunciones.h"

// El manejo con botones, a diferencia del giro en la palanca, para izq y der, 
// es que con los botones solo se activa el motor contrario y gira; mientras que 
// para el caso del stick, se activa el motor contrario marcha adelante y el otro en reversa.


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

// RECORDATORIO:  En los viejos y la bestia 1.0 funcion izq y der van al reves
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

// Función auxiliar para mapear el mando en la web:
// para consultar si un botón específico mapeado está presionado
bool estaBotonPresionado(ControllerPtr ctl, uint8_t botonID) {
    switch (botonID) {
        case BOTON_Y:  return ctl->y();
        case BOTON_A:  return ctl->a();
        case BOTON_B:  return ctl->b();
        case BOTON_X:  return ctl->x();
        case BOTON_L1: return ctl->l1();
        case BOTON_R1: return ctl->r1();
        case BOTON_L2: return ctl->brake() > 0;
        case BOTON_R2: return ctl->throttle() > 0;
        default: return false;
    }
}

void procesarBotonesDinamicos(ControllerPtr ctl) {
    static unsigned long ultimoBoton = 0;
    const unsigned long tiempoRebote = 200;

    if (millis() - ultimoBoton < tiempoRebote) return;

    if (estaBotonPresionado(ctl, perfilActivo.btnAdelante)) {
        adelante();
        ultimoBoton = millis();
    } else if (estaBotonPresionado(ctl, perfilActivo.btnAtras)) {
        atras();
        ultimoBoton = millis();
    } else if (estaBotonPresionado(ctl, perfilActivo.btnIzq)) {
        izquierda();
        ultimoBoton = millis();
    } else if (estaBotonPresionado(ctl, perfilActivo.btnDer)) {
        derecha();
        ultimoBoton = millis();
    }
}