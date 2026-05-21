#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"

// ========================================================================
// ============================== FUNCIONES ==============================
// ========================================================================

// =============== Función para ajustar nivel de velocidad con flechas ===============
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

// =============== Función para detener los motores ===============
void detenerMotores() {
// Paso valor 0 de PWM a los motores:
    ledcWrite(CH_ADELANTE_DER, 0);
    ledcWrite(CH_ATRAS_DER, 0);
    ledcWrite(CH_ADELANTE_IZQ, 0);
    ledcWrite(CH_ATRAS_IZQ, 0);
}

// =============== Función para mover los motores con 255 ===============
void maxima() {
    ledcWrite(CH_ADELANTE_IZQ, 255);
    ledcWrite(CH_ADELANTE_DER, 255);
    ledcWrite(CH_ATRAS_IZQ, 0);
    ledcWrite(CH_ATRAS_DER, 0);
}

// =============== Función para mover los motores ===============
void movimiento(ControllerPtr ctl) {

    float x = ctl->axisRX();
    float y = -ctl->axisRY();

    //Serial.printf("X:%d Y:%d\n", x, y);

    // Zona muerta
    // Sin movimiento
    if (abs(x) < ZONA_DRIFT && abs(y) < ZONA_DRIFT) {
        detenerMotores();
        return;
    }

    // Normalización: para pasar a magnitud unitaria
    float normX = x / 512.0;
    float normY = y / 512.0;

    // == Mezcla diferencial simple ==
    // * Lógica de funcionamiento: el controlador suma y resta los valores de ambos ejes según los motores (y a su vez se multiplica
    // por los valores de velocidad y constante para el PWM).
    // * Casos con ejemplo externo:
    //      1. Avanzar línea recta: si Y = +100 y X = 0; ambos motores reciben +100.
    //      2. Avanzar y girar a la derecha: si Y = +100 y X = +50; motor izquierdo recibe +150 y el derecho +50 (La rueda izquierda gira más rápido, provocando un giro hacia la derecha).
    //      3. Girar en el lugar sobre el eje: si Y = 0 y X = +100; el motor izquierdo recibe +100 y el derecho -100 (Las ruedas giran en sentidos opuestos, rotando el vehículo en el lugar).

    float pwmIzq = (normY + normX) * 127.0 * factorVelocidad;
    float pwmDer = (normY - normX) * 127.0 * factorVelocidad;

    // Se limita el PWM de los motores con los valores predefinidos en 'config.h'
    pwmIzq = constrain(pwmIzq, -PWM_MIN, PWM_MAX);
    pwmDer = constrain(pwmDer, -PWM_MIN, PWM_MAX);

    // Detecto los sentidos y activo los motores correspondientes para el giro:
    if (pwmIzq >= 0) {
        ledcWrite(CH_ADELANTE_IZQ, pwmIzq);
        ledcWrite(CH_ATRAS_IZQ, 0);
    } else {
        ledcWrite(CH_ADELANTE_IZQ, 0);
        ledcWrite(CH_ATRAS_IZQ, -pwmIzq);
    }

    if (pwmDer >= 0) {
        ledcWrite(CH_ADELANTE_DER, pwmDer + K);
        ledcWrite(CH_ATRAS_DER, 0);
    } else {
        ledcWrite(CH_ADELANTE_DER, 0);
        ledcWrite(CH_ATRAS_DER, -pwmDer + K);
    }
}

// ========================================================================
// ============================== SETUP ==============================
// ========================================================================

void setup() {
  Serial.begin(115200);

// Pines PWM
// Configurar canales PWM:
    ledcSetup(CH_ADELANTE_DER, PWM_FREQ, PWM_RES);
    ledcSetup(CH_ATRAS_DER, PWM_FREQ, PWM_RES);
    ledcSetup(CH_ADELANTE_IZQ, PWM_FREQ, PWM_RES);
    ledcSetup(CH_ATRAS_IZQ, PWM_FREQ, PWM_RES);

// Asociar pines a canales:
    ledcAttachPin(Adelante_Der, CH_ADELANTE_DER);
    ledcAttachPin(Atras_Der, CH_ATRAS_DER);
    ledcAttachPin(Adelante_Izq, CH_ADELANTE_IZQ);
    ledcAttachPin(Atras_Izq, CH_ATRAS_IZQ);

    pinMode(PIN_LED, OUTPUT);

// Configuración para conexión de mandos:
    //Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    //Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Setear los callbacks del Bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // El siguiente comando permite borrar la lista de conexiones para reset.
    BP32.forgetBluetoothKeys();
}


// ========================================================================
// ============================== LOOP ==============================
// ========================================================================

void loop() {

    bool dataUpdated = BP32.update();
    if (dataUpdated) processControllers();

    // Se recorre la lista de mandos conectados, y si hay uno conectado y está enviando información: 
    for (auto ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->hasData()) {
            
            // Booleano para detectar si se detectó los botones para subir y bajar la velocidad
            // sabiendo también si antes se presionó o no para que no haya efecto rebote.
            static bool prevY = false;
            static bool prevA = false;

            bool y = ctl->y(); // subir
            bool a = ctl->a(); // bajar

            // Si se apretó un botón, y antes no estaba apretado, entonces le envía a la función para controlar la velocidad, 
            // el botón correspondiente en 'true'.
            if (y && !prevY) {
                controlVelocidad(true, false);
            }
            if (a && !prevA) {
                controlVelocidad(false, true);
            }

            // Se actualizan el estado previo del botón:
            prevY = y;
            prevA = a;

            // Por ahora se comentó la función de manejar al robot con los botones, tanto para pruebas como 
            // porque no es realmente necesario. En caso de mal funcionamiento de un stick, cambiar al otro.
            //antiReboteTriang(ctl);
            //antiReboteCruz(ctl);
            //antiReboteCirc(ctl);
            //antiReboteCuad(ctl);
            //movimiento(ctl);

            static unsigned long ultimoIzquierda = 0;
            if (ctl->x() && millis() - ultimoIzquierda > tiempoRebote) {
                giro180Izquierda();
                ultimoIzquierda = millis();
            }

            static unsigned long ultimoDerecha = 0;
            if (ctl->b() && millis() - ultimoDerecha > tiempoRebote) {
                giro180Derecha();
                ultimoDerecha = millis();
            }


            //Si apreto R2, entonces se activa el turbo y los motores de adelante aceleran en línea recta a todo lo que da el PWM
            bool turbo = ctl->r1();

            if (turbo) {
                ledcWrite(CH_ADELANTE_IZQ, 255);
                ledcWrite(CH_ADELANTE_DER, 255);
                ledcWrite(CH_ATRAS_IZQ, 0);
                ledcWrite(CH_ATRAS_DER, 0);
            } else {
                movimiento(ctl);
            }

        }
    }

    delay(1);
}