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

// Función para Leer la Batería del ADC -----------------------------
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

        int posicion = (inicio + i) % MAX_HISTORIAL;

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

// condicional para evitar el rebote de tiempo
    if (ahora - tiempoUltimoCambio >= tiempoRebote) { 

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

int aplicarZonaMuerta(int valor, int zonaMuerta) {
    if (abs(valor) < zonaMuerta) {
        return 0;
    }
    if (valor > 0) {
        return map(valor, zonaMuerta, 512, 0, 512);
    } else {
        return map(valor, -zonaMuerta, -512, 0, -512);
    }
}

// Función para mover los motores -----------------------------
void movimiento(ControllerPtr ctl, bool turbo) {

    // ==========================================
    // 1. MODO TURBO (R1 presionado)
    // ==========================================
    if (turbo) {
        // Avance fijo a la potencia configurada en el perfil
        float baseAvance = perfilActivo.pwmMax; 

        // 📍 Elegir qué stick hace la CORRECCIÓN DE GIRO durante el Turbo:
        // 0 = Stick Izquierdo (X), 1 = Stick Derecho (RX)
        int ejeGiroTurbo = (perfilActivo.stickGiro == 0) ? ctl->axisX() : ctl->axisRX();

        // Inversión de eje si está configurado en el perfil
        if (perfilActivo.invertirEjeX) ejeGiroTurbo = -ejeGiroTurbo;

        // Aplicar Zona Muerta
        int zm = (perfilActivo.zonaMuerta > 0) ? perfilActivo.zonaMuerta : ZONA_DRIFT;
        int giroTurboRaw = aplicarZonaMuerta(ejeGiroTurbo, zm);

        // Escalado y aplicación del multiplicador kGiro
        float giroTurbo = (giroTurboRaw / 512.0) * perfilActivo.kGiro * 20.0;
        giroTurbo = constrain(giroTurbo, -100.0, 100.0);

        float pwmIzq = baseAvance + giroTurbo;
        float pwmDer = baseAvance - giroTurbo;

        // Limitar a 0-255
        pwmIzq = constrain(pwmIzq, 0, 255);
        pwmDer = constrain(pwmDer, 0, 255);

        // Sumar calibración de motores del perfil activo
        int adelanteIzq = constrain((int)pwmIzq + perfilActivo.kIzq, 0, 255);
        int adelanteDer = constrain((int)pwmDer + perfilActivo.kDer, 0, 255);

        escribirPWM(adelanteIzq, 0, adelanteDer, 0);
        return;
    }

    // ==========================================
    // 2. MOVIMIENTO NORMAL (Siempre con Stick Derecho: RX / RY)
    // ==========================================
    float x = ctl->axisRX();
    float y = -ctl->axisRY(); // Invertimos Y para que arriba sea positivo

    // Aplicar Inversión de Ejes según el perfil
    if (perfilActivo.invertirEjeX) x = -x;
    if (perfilActivo.invertirEjeY) y = -y;

    // Aplicar Zona Muerta
    int zm = (perfilActivo.zonaMuerta > 0) ? perfilActivo.zonaMuerta : ZONA_DRIFT;
    int avance = aplicarZonaMuerta((int)y, zm);
    int giroRaw = aplicarZonaMuerta((int)x, zm);

    if (avance == 0 && giroRaw == 0) {
        detenerMotores();
        return;
    }

    float normY = avance / 512.0;
    float normX = giroRaw / 512.0;

    // Mezcla diferencial normal
    float pwmIzq = (normY + normX) * 127.0 * factorVelocidad;
    float pwmDer = (normY - normX) * 127.0 * factorVelocidad;

    // Limitar al valor PWM Máximo configurado en el perfil activo
    pwmIzq = constrain(pwmIzq, (float)-perfilActivo.pwmMax, (float)perfilActivo.pwmMax);
    pwmDer = constrain(pwmDer, (float)-perfilActivo.pwmMax, (float)perfilActivo.pwmMax);

    int adelanteIzq = 0, atrasIzq = 0;
    int adelanteDer = 0, atrasDer = 0;

    // Motor Izquierdo
    if (pwmIzq >= 0) {
        adelanteIzq = constrain((int)pwmIzq + perfilActivo.kIzq, 0, 255);
    } else {
        atrasIzq = constrain((int)(-pwmIzq) + perfilActivo.kIzq, 0, 255);
    }

    // Motor Derecho
    if (pwmDer >= 0) {
        adelanteDer = constrain((int)pwmDer + perfilActivo.kDer, 0, 255);
    } else {
        atrasDer = constrain((int)(-pwmDer) + perfilActivo.kDer, 0, 255);
    }

    escribirPWM(adelanteIzq, atrasIzq, adelanteDer, atrasDer);
}


// ====== ACCESS POINT ======
void iniciarAccessPoint(bool temporal) {

// Si ya está encendido, no hacemos nada
    if (apActivo)
        return;

    WiFi.mode(WIFI_AP);

    IPAddress local_IP(192, 168, 25, 1);
    IPAddress gateway(192, 168, 25, 1);
    IPAddress subnet(255, 255, 255, 0);

// IP's para cada web (conviene para que no se mezclen si más de uno está prendido)
    //Bestia 1.0 192.168.23.1
    //Bestia 2.0 192.168.24.1
    //Bella 192.168.25.1

    WiFi.softAPConfig(local_IP, gateway, subnet);

// Acá podés cambiar el nombre del wifi, también para que no sea confuso
    bool ok = WiFi.softAP("Robot-Bella", "Fulbo123", 1, false, 1);

    if (ok) {
        apActivo = true;
        apTemporal = temporal;
        inicioAP = millis();

    /*
        if (apTemporal) {
            Serial.println("Modo temporal: 2 minutos");
        } else {
            Serial.println("Modo manual: sin limite de tiempo");
        }
    */
    
    } else {
        Serial.println("ERROR AL INICIAR AP");
    }
}

void apagarAccessPoint() {

    if (!apActivo)
        return;

    //Serial.println("========== WIFI AP ==========");
    //Serial.println("Apagando Access Point...");

    // Apaga solamente el Access Point - NO apagamos Bluetooth.
    WiFi.softAPdisconnect(false);

    apActivo = false;
    apTemporal = false;

    //Serial.println("Access Point apagado.");
    //Serial.println("Bluetooth permanece activo.");
}

void controlarAccessPoint() {
    if (!apActivo)
        return;

    // Si fue iniciado manualmente con START, no tiene tiempo límite.
    if (!apTemporal)
        return;

    // Si fue iniciado automáticamente al arrancar, dura solamente 2 minutos.
    if (millis() - inicioAP >= DURACION_AP) {
        apagarAccessPoint();
    }
}

// ===== CONFIGURACIÓN DE MOTORES =======
// Es lo que se carga para actualizar la configuración de los motores
// en la página web.
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

// Esta función es la que permite escribir el pwm, sólo se llama a la función y se le pasa los 
// parámetros en una sola línea.
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

// Las siguientes funciones permiten cargar y guardar los perfiles de la web:
PerfilConfig perfilActivo;
int perfilActualID = 0;

#include <Preferences.h>

void guardarPerfil(int id, PerfilConfig p) {
    Preferences prefs;
    prefs.begin("perfiles", false);

    String key = "p_" + String(id) + "_";
    prefs.putString((key + "nom").c_str(), p.nombre);
    prefs.putInt((key + "pwm").c_str(), p.pwmMax);
    prefs.putInt((key + "ki").c_str(), p.kIzq);
    prefs.putInt((key + "kd").c_str(), p.kDer);
    prefs.putInt((key + "ang").c_str(), p.anguloGiro);
    prefs.putBool((key + "ix").c_str(), p.invertirEjeX);
    prefs.putBool((key + "iy").c_str(), p.invertirEjeY);
    prefs.putInt((key + "zm").c_str(), p.zonaMuerta);
    prefs.putInt((key + "ba").c_str(), p.btnAdelante);
    prefs.putInt((key + "bb").c_str(), p.btnAtras);
    prefs.putInt((key + "bi").c_str(), p.btnIzq);
    prefs.putInt((key + "bd").c_str(), p.btnDer);

    // 📍 GUARDAR NUEVOS CAMPOS
    prefs.putFloat((key + "kg").c_str(), p.kGiro);
    prefs.putInt((key + "sg").c_str(), p.stickGiro);

    int total = prefs.getInt("total", 1);
    if (id + 1 > total) prefs.putInt("total", id + 1);

    prefs.putInt("activo", id);
    prefs.end();
    perfilActualID = id;
}

void cargarPerfil(int id) {
    Preferences prefs;
    prefs.begin("perfiles", true);

    String key = "p_" + String(id) + "_";
    String nombreDefecto = "Perfil " + String(id + 1);
    String nom = prefs.getString((key + "nom").c_str(), nombreDefecto.c_str());
    snprintf(perfilActivo.nombre, sizeof(perfilActivo.nombre), "%s", nom.c_str());

    perfilActivo.pwmMax = prefs.getInt((key + "pwm").c_str(), 220);
    perfilActivo.kIzq = prefs.getInt((key + "ki").c_str(), 0);
    perfilActivo.kDer = prefs.getInt((key + "kd").c_str(), 0);
    perfilActivo.anguloGiro = prefs.getInt((key + "ang").c_str(), 180);
    perfilActivo.invertirEjeX = prefs.getBool((key + "ix").c_str(), false);
    perfilActivo.invertirEjeY = prefs.getBool((key + "iy").c_str(), false);
    perfilActivo.zonaMuerta = prefs.getInt((key + "zm").c_str(), 10);
    perfilActivo.btnAdelante = prefs.getInt((key + "ba").c_str(), 1);
    perfilActivo.btnAtras = prefs.getInt((key + "bb").c_str(), 2);
    perfilActivo.btnIzq = prefs.getInt((key + "bi").c_str(), 3);
    perfilActivo.btnDer = prefs.getInt((key + "bd").c_str(), 4);

    // 📍 CARGAR NUEVOS CAMPOS CON VALORES POR DEFECTO
    perfilActivo.kGiro = prefs.getFloat((key + "kg").c_str(), 1.0f);
    perfilActivo.stickGiro = prefs.getInt((key + "sg").c_str(), 0); // Default: Izquierdo

    perfilActualID = id;
    prefs.end();
}

// Tarea dedicada al Servidor Web en Core 0
void TaskWebServer(void *pvParameters) {
    for (;;) {
        if (apActivo) {
            server.handleClient();
        }
        vTaskDelay(2 / portTICK_PERIOD_MS); // Pequeña pausa para no saturar el kernel
    }
}