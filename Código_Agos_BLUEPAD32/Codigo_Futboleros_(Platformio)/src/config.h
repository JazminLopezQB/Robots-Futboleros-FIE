#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <Preferences.h>
#include <WebServer.h>
#include <Bluepad32.h>

#define ZONA_DRIFT 10

// PINES DE MOTORES
#define Atras_Izq 26
#define Adelante_Izq 25

#define Atras_Der 21
#define Adelante_Der 18

// Bestia 2.0 - 25 26 14 12
// Bella 26 25 12 14
// Bestia tranqui 25 26 18 21

// BATERÍA
#define PIN_BATERIA 32

#define RESISTENCIA_SUPERIOR 2700.0
#define RESISTENCIA_INFERIOR 1500.0

// LED ESP32
#define PIN_LED 13 // 2

// HISTORIAL
#define MAX_HISTORIAL 300

// PWM
#define CH_ADELANTE_DER 0
#define CH_ATRAS_DER 1

#define CH_ADELANTE_IZQ 2
#define CH_ATRAS_IZQ 3

#define PWM_FREQ 20000
#define PWM_RES 8

// ESTRUCTURA DEL GAMEPAD
struct GamepadInfo {
    ControllerPtr ctl;
    String mac;
    String name;
    bool connected;
};

// PWM
extern uint8_t PWM_MAX;
extern float factorVelocidad;
extern int nivelActual;
extern const int cantNiveles;
extern const float niveles[];

// TIEMPOS
extern int tiempoGiro180;
extern int tiempoCarga;
extern int tiempoGolpe;
extern const unsigned long tiempoRebote;
extern int anguloGiro;

// CONSTANTES DE MOTORES
extern uint8_t K_IZQ;
extern uint8_t K_DER;

// PWM ACTUAL
extern int pwmActualAdelanteIzq;
extern int pwmActualAtrasIzq;
extern int pwmActualAdelanteDer;
extern int pwmActualAtrasDer;

// MANDOS
extern GamepadInfo gamepads[BP32_MAX_GAMEPADS];
extern ControllerPtr myControllers[BP32_MAX_GAMEPADS];
extern bool whitelistEnabled;
extern uint8_t allowedController[6];
extern bool pairingMode;
extern unsigned long pairingStart;
extern const unsigned long pairingTimeout;
extern String authorizedGamepad;
extern String connectedGamepad;

// PREFERENCES
extern Preferences prefs;

// ACCESS POINT
extern bool apActivo;
extern bool apTemporal;
extern unsigned long inicioAP;
extern const unsigned long DURACION_AP;

// BOTONES
extern bool botonStartAnterior;
extern bool botonSelectAnterior;

// BATERÍA
extern float voltajeBateria;
extern float voltajeADC;
extern unsigned long ultimaLecturaBateria;
extern const unsigned long INTERVALO_BATERIA;

// HISTORIAL DE BATERÍA
extern float historialVoltaje[MAX_HISTORIAL];
extern unsigned long historialTiempo[MAX_HISTORIAL];
extern int indiceHistorial;
extern int cantidadHistorial;
extern unsigned long inicioHistorial;

// CALIBRACIÓN
extern uint8_t pwmMaxConfigurado;

// WEB SERVER
extern WebServer server;
