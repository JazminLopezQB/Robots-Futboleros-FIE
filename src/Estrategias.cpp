// Librerias
#include <PS4Controller.h>
#include <Arduino.h>
#include <Estrategias.hpp>
#include <ManejoConBotones.hpp>


// Estrategia 1 - giro brusco de 180° a la izquierda
void giro180Izquierda() {
  // Motor izquierdo apagado
  ledcWrite(Adelante_Izq, 0);
  ledcWrite(Atras_Izq, 0);
  ledcWrite(Atras_Der, 0);

  // Motor derecho gira hacia adelante
  ledcWrite(Adelante_Der, PWM_MAX); // + K);  // PWM calibrado para giro
  delay(tiempoGiro180);   // Tiempo estimado para giro completo
  detenerMotores();       // Detener ambos motores
}

// Estrategia 2 - giro brusco de 180° a la derecha
void giro180Derecha() {
  // Motor derecho apagado
  ledcWrite(Adelante_Der, 0);
  ledcWrite(Atras_Der, 0);
  ledcWrite(Atras_Izq, 0);

  // Motor izquierdo gira hacia adelante
  ledcWrite(Adelante_Izq, PWM_MAX);  // PWM calibrado para giro
  delay(tiempoGiro180);   // Tiempo estimado para giro completo
  detenerMotores();       // Detener ambos motores
}