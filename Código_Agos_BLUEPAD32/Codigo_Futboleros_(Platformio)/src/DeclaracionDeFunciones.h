#pragma once
#include "config.h"

// Conexión
void onConnectedController(ControllerPtr ctl);
void onDisconnectedController(ControllerPtr ctl);
void processControllers();
void processControllers();
bool isControllerAllowed(ControllerPtr ctl);
void controlVelocidad(bool flecha_arr, bool flecha_abaj);

// Estrategia y giros
void giro180Izquierda();
void giro180Derecha();
void giroIzquierda(int grados);
void giroDerecha(int grados);

// Access Point
void iniciarAccessPoint(bool temporal);
void apagarAccessPoint(); 
void controlarAccessPoint();

// Motores
void cargarConfiguracionMotores();
void guardarConfiguracionMotores();
void escribirPWM();
void movimiento(ControllerPtr ctl, bool turbo);
void detenerMotores();

// Batería y ADC
void leerBateria();
String obtenerHistorialJSON();

void antiReboteTriang(ControllerPtr ctl);
void antiReboteCruz(ControllerPtr ctl);
void antiReboteCirc(ControllerPtr ctl);
void antiReboteCuad(ControllerPtr ctl);

void escribirPWM (int adelanteIzq, int atrasIzq, int adelanteDer, int atrasDer);