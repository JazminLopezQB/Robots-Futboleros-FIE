#include "config.h"

// ARCHIVO DE CONFIGURACIÓN DE VARIABLES ================================================
// Este archivo posee los valores de las variables definidas en 'config.h'.

float factorVelocidad = 2.0; 
int nivelActual = 2;

const int cantNiveles = 3;
const float niveles[cantNiveles] = {1.5, 1.65, 2.0};

int tiempoGiro180 = 685;
int tiempoCarga = 300;
int tiempoGolpe = 300;
const unsigned long tiempoRebote = 200;

// Ángulo seleccionado para los giros desde la web
int anguloGiro = 180;

// ================= PWM ACTUAL DE LOS MOTORES =================

int pwmActualAdelanteIzq = 0;
int pwmActualAtrasIzq = 0;
int pwmActualAdelanteDer = 0;
int pwmActualAtrasDer = 0;

String sentidoIzq = "DETENIDO";
String sentidoDer = "DETENIDO";

uint8_t PWM_MAX = 220;
