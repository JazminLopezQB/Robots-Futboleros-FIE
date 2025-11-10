#pragma once

extern const int PWM_MIN;
extern const int PWM_MAX;

extern const int Atras_Izq;
extern const int Adelante_Izq;
extern const int Atras_Der;
extern const int Adelante_Der;

extern int nivelActual;

extern unsigned long tiempoUltimoCambio;
extern  const unsigned long tiempoRebote;

// Adelante con triangulo
void adelante();
// Atrás con cruz
void atras();
// Izquierda con círculo
void izquierda();

// Derecha con cuadrado
void derecha();

// FUNCIONES DE ANTIREBOTE PARA EVITAR DATOS BASURA
void antiReboteTriang ();
void antiReboteCruz ();
void antiReboteCirc ();
void antiReboteCuad ();
