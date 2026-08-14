#include <Arduino.h>
#include "config.h"
#include "DeclaracionDeFunciones.h"

#include <Bluepad32.h>

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ----------------------------------------------------------
// Funciones
// ----------------------------------------------------------

// Función para Leer la Batería -----------------------------
void leerBateria() {

    int adc = analogRead(PIN_BATERIA);

    // Conversión de la lectura analógica a Voltaje del pin ADC
    voltajeADC = (adc / 4095.0) * 3.3;

    // Se calcula el voltaje real de la batería por el divisor resistivo
    voltajeBateria = voltajeADC * ((RESISTENCIA_SUPERIOR + RESISTENCIA_INFERIOR) / RESISTENCIA_INFERIOR);

    // Guardar en historial
    unsigned long tiempo = millis() - inicioHistorial;

    historialVoltaje[indiceHistorial] = voltajeBateria;
    historialTiempo[indiceHistorial] = tiempo;

    indiceHistorial++;

    if (indiceHistorial >= MAX_HISTORIAL) {
        indiceHistorial = 0;
    }

    if (cantidadHistorial < MAX_HISTORIAL) {
        cantidadHistorial++;
    }
}

// Historial JSON -----------------------------
String obtenerHistorialJSON() {

    String json = "[";
    int inicio;

    if (cantidadHistorial < MAX_HISTORIAL) {
        inicio = 0;
    } else {
        inicio = indiceHistorial;
    }

    for (int i = 0; i < cantidadHistorial; i++) {

        int posicion =
            (inicio + i) % MAX_HISTORIAL;

        json += "{";

        json += "\"t\":";
        json += String(historialTiempo[posicion] / 1000.0, 1);

        json += ",";

        json += "\"v\":";
        json += String(historialVoltaje[posicion], 2);

        json += "}";

        if (i < cantidadHistorial - 1) {
            json += ",";
        }
    }

    json += "]";

    return json;
}

// Función para ajustar nivel de velocidad con flechas -----------------------------
unsigned long tiempoUltimoCambio = 0;

void controlVelocidad(bool flecha_arr, bool flecha_abaj) {

  unsigned long ahora = millis();

  if (ahora - tiempoUltimoCambio >= tiempoRebote) { // condicional para evitar el rebote de tiempo

// Si se presiona la flecha de arriba y el nivel actual es menor a la cantidad de niveles...
    if (flecha_arr && nivelActual < cantNiveles-1) {
      nivelActual++; // Aumento el nivel
      factorVelocidad = niveles[nivelActual]; // Inicializo el factor de velocidad dentro del array
      //Serial.printf("Nivel %d | Factor %.2f\n", nivelActual, factorVelocidad);
      tiempoUltimoCambio = ahora; // Guardo el tiempo de cambio
    }

// Si se presiona la flecha de abajo y el nivel actual es mayor a cero...
    if (flecha_abaj && nivelActual > 0) {
      nivelActual--; // Disminuyo el nivel
      factorVelocidad = niveles[nivelActual]; // Inicializo el factor de velocidad dentro del array
      //Serial.printf("Nivel %d | Factor %.2f\n", nivelActual, factorVelocidad);
      tiempoUltimoCambio = ahora; // Guardo el tiempo de cambio
    }
  }
}

// Función para detener los motores -----------------------------
void detenerMotores() {
// Paso valor 0 de PWM a los motores:
    escribirPWM(
        0,  // adelante izquierda
        0,  // atrás izquierda
        0,  // adelante derecha
        0   // atrás derecha
    );

    String sentidoIzq = "DETENIDO";
    String sentidoDer = "DETENIDO";
}

// Función para mover los motores con 255 -----------------------------
void maxima() {
    escribirPWM(
        255,  // adelante izquierda
        0,  // atrás izquierda
        255,  // adelante derecha
        0   // atrás derecha
    );

    String sentidoIzq = "ADELANTE";
    String sentidoDer = "ADELANTE";
}

// Función para mover los motores -----------------------------
void movimiento(ControllerPtr ctl, bool turbo) {

    float x = ctl->axisRX();
    float y = -ctl->axisRY();

    // TURBO
    if (turbo) {
        // R1 mantiene SIEMPRE el avance hacia adelante
        // Velocidad base del turbo
        float avance = 255.0;

        // ------------------------------------------
        // La palanca solamente controla el giro
        float giro = x / 512.0;

        // Reducimos la influencia de la palanca
        // para que solamente haga una corrección suave.
        giro *= 100.0;

        // Limitar corrección
        giro = constrain(giro, -100.0, 100.0);

        float pwmIzq = avance + giro;
        float pwmDer = avance - giro;

        // Limitar a 0-255
        pwmIzq = constrain(pwmIzq, 0, 255);
        pwmDer = constrain(pwmDer, 0, 255);

        int adelanteIzq =
            constrain((int)pwmIzq + K_IZQ, 0, 255);

        int adelanteDer =
            constrain((int)pwmDer + K_DER, 0, 255);

        escribirPWM(
            adelanteIzq,
            0,
            adelanteDer,
            0
        );

        return;
    }

    // MOVIMIENTO NORMAL
    if (abs(x) < ZONA_DRIFT && abs(y) < ZONA_DRIFT) {
        detenerMotores();
        return;
    }

    float normX = x / 512.0;
    float normY = y / 512.0;

    // Mezcla diferencial normal
    float pwmIzq = (normY + normX) * 127.0 * factorVelocidad;

    float pwmDer = (normY - normX) * 127.0 * factorVelocidad;

    // Movimiento normal respeta PWM_MAX
    pwmIzq = constrain(pwmIzq,-PWM_MAX,PWM_MAX);
    pwmDer = constrain(pwmDer,-PWM_MAX,PWM_MAX);


    // MOTOR IZQUIERDO
    int adelanteIzq = 0;
    int atrasIzq = 0;

    int adelanteDer = 0;
    int atrasDer = 0;

    if (pwmIzq >= 0) {
        adelanteIzq = constrain((int)pwmIzq + K_IZQ,0,255);
    } else {
        atrasIzq = constrain((int)(-pwmIzq) + K_IZQ,0,255);
    }

    // MOTOR DERECHO
    if (pwmDer >= 0) {
        adelanteDer = constrain((int)pwmDer + K_DER, 0, 255);
    } else {
        atrasDer = constrain((int)(-pwmDer) + K_DER, 0, 255);
    }

    escribirPWM(
        adelanteIzq,
        atrasIzq,
        adelanteDer,
        atrasDer
    );
}

// ====== ACCESS POINT =====
void iniciarAccessPoint(bool temporal) {

    // Si ya está encendido, no hacemos nada
    if (apActivo)
        return;

    WiFi.mode(WIFI_AP);

    //Bestia 1.0 192.168.23.1
    //Bestia 2.0 192.168.24.1
    //Bella 192.168.25.1

    IPAddress local_IP(192, 168, 25, 1);
    IPAddress gateway(192, 168, 25, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(local_IP, gateway, subnet);

    bool ok = WiFi.softAP("Robot-Bella", "Fulbo123", 1, false, 1);

    if (ok) {
        apActivo = true;
        apTemporal = temporal;
        inicioAP = millis();

        if (apTemporal) {
            Serial.println("Modo temporal: 2 minutos");
        } else {
            Serial.println("Modo manual: sin limite de tiempo");
        }
    
    } else {
        Serial.println("ERROR AL INICIAR AP");
    }
}

void apagarAccessPoint() {

    if (!apActivo)
        return;

    //Serial.println("========== WIFI AP ==========");
    //Serial.println("Apagando Access Point...");

    // Apaga solamente el Access Point.
    // NO apagamos Bluetooth.
    WiFi.softAPdisconnect(false);

    apActivo = false;
    apTemporal = false;

    //Serial.println("Access Point apagado.");
    //Serial.println("Bluetooth permanece activo.");
}

void controlarAccessPoint() {
    if (!apActivo)
        return;

    // Si fue iniciado manualmente con START,
    // no tiene tiempo límite.
    if (!apTemporal)
        return;

    // Si fue iniciado automáticamente al arrancar,
    // dura solamente 2 minutos.
    if (millis() - inicioAP >= DURACION_AP) {
        apagarAccessPoint();
    }
}

// ===== CONFIGURACIÓN DE MOTORES =======
void cargarConfiguracionMotores() {
    prefs.begin("config", true);

    PWM_MAX = prefs.getUChar("pwmMax", 220);
    K_IZQ = prefs.getUChar("kIzq", 0);
    K_DER = prefs.getUChar("kDer", 0);
    anguloGiro = prefs.getInt("anguloGiro", 180);

    prefs.end();
/*
    Serial.println("===== CONFIGURACION MOTORES =====");
    Serial.printf("PWM MAX: %d\n", PWM_MAX);
    Serial.printf("K IZQ: %d\n", K_IZQ);
    Serial.printf("K DER: %d\n", K_DER);
*/
}

void guardarConfiguracionMotores() {
    prefs.begin("config", false);

    prefs.putUChar("pwmMax", PWM_MAX);
    prefs.putUChar("kIzq", K_IZQ);
    prefs.putUChar("kDer", K_DER);

    // Guardar también el ángulo
    prefs.putInt("anguloGiro", anguloGiro);

    prefs.end();
/*
    Serial.println("Configuracion de motores guardada.");
    Serial.print("PWM MAX: ");
    Serial.println(PWM_MAX);
    Serial.print("K IZQ: ");
    Serial.println(K_IZQ);
    Serial.print("K DER: ");
    Serial.println(K_DER);
    Serial.print("ANGULO GIRO: ");
    Serial.println(anguloGiro);
*/
}

void escribirPWM (int adelanteIzq, int atrasIzq, int adelanteDer, int atrasDer) {

    pwmActualAdelanteIzq = adelanteIzq;
    pwmActualAtrasIzq = atrasIzq;
    pwmActualAdelanteDer = adelanteDer;
    pwmActualAtrasDer = atrasDer;

    ledcWrite(CH_ADELANTE_IZQ, adelanteIzq);
    ledcWrite(CH_ATRAS_IZQ, atrasIzq);
    ledcWrite(CH_ADELANTE_DER, adelanteDer);
    ledcWrite(CH_ATRAS_DER, atrasDer);
}