#pragma once

// LIBRERÍAS
#include <Arduino.h>
#include <stdint.h>
#include <Preferences.h>
#include <WebServer.h>
#include <Bluepad32.h>

// Corrección del drift
#define ZONA_DRIFT 10

// PINES DE MOTORES
#define Atras_Izq 26
#define Adelante_Izq 25

#define Atras_Der 21
#define Adelante_Der 18

// Ajustes - Recordatorio:
    // Bestia 2.0 - 25 26 19 18
    // Bella 26 25 21 18
    // Bestia tranqui 25 26 21 18

    // Las IP's:
        //Bestia 1.0 192.168.23.1
        //Bestia 2.0 192.168.24.1
        //Bella 192.168.25.1
    
    // En la función movimiento, en la mezcla diferencial: 
        // Tener en cuenta que para la Bestia 1.0 y versiones anteriores puede que 
        // haya que invertir los signos de pwmIzq y pwmDer. Se cambia lo de abajo pero donde está implementado
        
        /*
        float pwmIzq = (normY + normX) * 127.0 * factorVelocidad;
        float pwmDer = (normY - normX) * 127.0 * factorVelocidad;
        */
    
        // También tener en cuenta que para esas versiones, también hay que cambiar 
        // los izq y der del manejo con botones. 

// PIN BATERÍA
#define PIN_BATERIA 32

// Valores del divisor resistivo del ADC
#define RESISTENCIA_SUPERIOR 2700.0
#define RESISTENCIA_INFERIOR 1500.0

// LED
#define PIN_LED 2 // 13

// HISTORIAL
#define MAX_HISTORIAL 300

// PWM - Canales, freq, resolución
#define CH_ADELANTE_DER 0
#define CH_ATRAS_DER 1

#define CH_ADELANTE_IZQ 2
#define CH_ATRAS_IZQ 3

#define PWM_FREQ 20000
#define PWM_RES 8

// ESTRUCTURA DEL GAMEPAD
struct GamepadInfo { ControllerPtr ctl; String mac; String name; bool connected; };

// Definición de VARIABLES -----------------------

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
// Permite realizar una corrección de los motores,
// por ejemplo si uno va más potente que el otro. 
extern uint8_t K_IZQ;
extern uint8_t K_DER;

// PWM ACTUAL
// Permite luego en la web, actualizar los valores
// de PWM. 
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
// Son los botones para prender y apagar
// el access point.
extern bool botonStartAnterior;
extern bool botonSelectAnterior;

// BATERÍA - Lectura ADC
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

// Estructuras y variables para el manejo de perfiles de configuración en la web:
// IDs de Botones soportados para mapeo
enum BotonAccion {
    BOTON_NINGUNO = 0,
    BOTON_Y = 1,       // Triángulo / Y
    BOTON_A = 2,       // Cruz / A
    BOTON_B = 3,       // Círculo / B
    BOTON_X = 4,       // Cuadrado / X
    BOTON_L1 = 5,
    BOTON_R1 = 6,
    BOTON_L2 = 7,
    BOTON_R2 = 8
};

struct PerfilConfig {
    char nombre[30];
    int pwmMax;
    int kIzq;
    int kDer;
    int anguloGiro;
    bool invertirEjeX;
    bool invertirEjeY;
    int zonaMuerta;
    int btnAdelante;
    int btnAtras;
    int btnIzq;
    int btnDer;
    
    // 📍 NUEVOS CAMPOS:
    float kGiro;      // Multiplicador/Constante para la corrección del turbo (ej. 0.5 a 2.0)
    int stickGiro;    // 0 = Stick Izquierdo, 1 = Stick Derecho
};

extern int perfilActualID;
extern int totalPerfilesGuardados;

extern PerfilConfig perfilActivo;
extern int perfilActualID;