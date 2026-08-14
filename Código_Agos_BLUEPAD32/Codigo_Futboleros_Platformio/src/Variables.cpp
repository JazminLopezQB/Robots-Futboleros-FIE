#include "config.h"

// VELOCIDAD
float factorVelocidad = 2.0;
int nivelActual = 2;
const int cantNiveles = 3;
const float niveles[cantNiveles] = {1.5, 1.65, 2.0};

// TIEMPOS
int tiempoGiro180 = 685;
int tiempoCarga = 300;
int tiempoGolpe = 300;
const unsigned long tiempoRebote = 200;

// ÁNGULO
int anguloGiro = 180;

// PWM ACTUAL
int pwmActualAdelanteIzq = 0;
int pwmActualAtrasIzq = 0;
int pwmActualAdelanteDer = 0;
int pwmActualAtrasDer = 0;

// SENTIDO
String sentidoIzq = "DETENIDO";
String sentidoDer = "DETENIDO";

// PWM
uint8_t PWM_MAX = 220;

// CONSTANTES K
uint8_t K_IZQ = 0;
uint8_t K_DER = 0;

// ACCESS POINT
bool apActivo = false;
bool apTemporal = false;
unsigned long inicioAP = 0;
const unsigned long DURACION_AP = 120000;

// BOTONES
bool botonStartAnterior = false;
bool botonSelectAnterior = false;

// BATERÍA
float voltajeBateria = 0.0;
float voltajeADC = 0.0;
unsigned long ultimaLecturaBateria = 0;
const unsigned long INTERVALO_BATERIA = 1000;

// HISTORIAL
float historialVoltaje[MAX_HISTORIAL];
unsigned long historialTiempo[MAX_HISTORIAL];
int indiceHistorial = 0;
int cantidadHistorial = 0;
unsigned long inicioHistorial = 0;

// CALIBRACIÓN
uint8_t pwmMaxConfigurado = PWM_MAX;

// WHITELIST
uint8_t allowedController[6] = {0, 0, 0, 0, 0, 0};
bool whitelistEnabled = false;

// PAIRING
bool pairingMode = false;
unsigned long pairingStart = 0;
const unsigned long pairingTimeout = 30000;

// GAMEPAD
String authorizedGamepad = "Ninguno";
String connectedGamepad = "Ninguno";

// GAMEPADS
GamepadInfo gamepads[BP32_MAX_GAMEPADS];
ControllerPtr myControllers[BP32_MAX_GAMEPADS] = {nullptr, nullptr, nullptr, nullptr};

// WEB SERVER
WebServer server(80);

// PREFERENCES
Preferences prefs;