#pragma once

// ARCHIVO DE CONFIGURACIÓN ================================================
// Este archivo posee la definición de pines, canales PWM, y límites admisibles
// de la configuración del PWM.

// Para evitar drift y mejorar la presición de los sticks:
#define ZONA_DRIFT 10 // Cambiar valor según ajuste y joystick.

// Pines de los motores (NO CAMBIAR) =======
#define Atras_Izq 18
#define Adelante_Izq 21
#define Atras_Der 25
#define Adelante_Der 26

#define PIN_LED 13

// Ajustes de PWM ==========================
#define PWM_FREQ 20000
#define PWM_RES 8

// Los siguientes PWM son los límites admisibles de funcionamiento de los motores.
// Revisar muy bien la placa, y el motor previo a sugerir un cambio.
#define PWM_MIN 180
#define PWM_MAX 232

// Definiciónn de canales PWM ==============
#define CH_ADELANTE_DER 0
#define CH_ATRAS_DER 1
#define CH_ADELANTE_IZQ 2
#define CH_ATRAS_IZQ 3

// Variables globales ======================
extern float factorVelocidad; // Multiplicador de los niveles de velocidad
extern int nivelActual;       // El nivel actual es con el que inicia (el máximo)

// Esto es para el manejo de las velocidades
extern const int cantNiveles; 
extern const float niveles[];

// Estos tiempos son para las estrategias, sabiendo la duración de un giro, puedo 
// acotar el rango y la velocidad del giro.
extern int tiempoGiro180;
extern int tiempoCarga;
extern int tiempoGolpe;
extern const unsigned long tiempoRebote;

// Constante K para equiparar los motores, un motor va más lento que otro
extern uint8_t K;

