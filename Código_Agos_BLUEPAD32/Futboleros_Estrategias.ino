#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"

// Estrategia 1 - giro brusco de 180° a la izquierda
void giro180Izquierda() {
  // Motor izquierdo apagado
  ledcWrite(CH_ADELANTE_IZQ, 0);
  ledcWrite(CH_ATRAS_IZQ, 0);
  ledcWrite(CH_ATRAS_DER, 0);

  // Motor derecho gira hacia adelante
  ledcWrite(CH_ADELANTE_DER, PWM_MAX); // + K);  // PWM calibrado para giro
  delay(tiempoGiro180);   // Tiempo estimado para giro completo
  detenerMotores();       // Detener ambos motores
}

// Estrategia 2 - giro brusco de 180° a la derecha
void giro180Derecha() {
  // Motor derecho apagado
  ledcWrite(CH_ADELANTE_DER, 0);
  ledcWrite(CH_ATRAS_DER, 0);
  ledcWrite(CH_ATRAS_IZQ, 0);

  // Motor izquierdo gira hacia adelante
  ledcWrite(CH_ADELANTE_IZQ, PWM_MAX);  // PWM calibrado para giro
  delay(tiempoGiro180);   // Tiempo estimado para giro completo
  detenerMotores();       // Detener ambos motores
}