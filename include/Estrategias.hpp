#pragma once

extern const int Atras_Izq;
extern const int Adelante_Izq;
extern const int Atras_Der;
extern const int Adelante_Der;

extern int tiempoGiro180;

// Estrategia 1 - giro brusco de 180° a la izquierda
void giro180Izquierda();

// Estrategia 2 - giro brusco de 180° a la derecha
void giro180Derecha();

extern void detenerMotores();   // sacar si se hace un modulo para el movimiento