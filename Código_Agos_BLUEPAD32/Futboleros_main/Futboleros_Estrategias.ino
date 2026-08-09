#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"

void giroIzquierda(int grados) {

    // Motor izquierdo apagado
    ledcWrite(CH_ADELANTE_IZQ, 0);
    ledcWrite(CH_ATRAS_IZQ, 0);

    // Motor derecho hacia adelante
    ledcWrite(CH_ADELANTE_DER, PWM_MAX);
    ledcWrite(CH_ATRAS_DER, 0);

    // Tiempo proporcional al ángulo
    int tiempoGiro = (tiempoGiro180 * grados) / 180;

    Serial.print("Girando izquierda ");
    Serial.print(grados);
    Serial.print(" grados durante ");
    Serial.print(tiempoGiro);
    Serial.println(" ms");

    delay(tiempoGiro);

    detenerMotores();
}

void giroDerecha(int grados) {

    // Motor derecho apagado
    ledcWrite(CH_ADELANTE_DER, 0);
    ledcWrite(CH_ATRAS_DER, 0);

    // Motor izquierdo hacia adelante
    ledcWrite(CH_ADELANTE_IZQ, PWM_MAX);
    ledcWrite(CH_ATRAS_IZQ, 0);

    // Tiempo proporcional al ángulo
    int tiempoGiro = (tiempoGiro180 * grados) / 180;

    Serial.print("Girando derecha ");
    Serial.print(grados);
    Serial.print(" grados durante ");
    Serial.print(tiempoGiro);
    Serial.println(" ms");

    delay(tiempoGiro);

    detenerMotores();
}

void giro180Izquierda() {
    giroIzquierda(180);
}

void giro180Derecha() {
    giroDerecha(180);
}