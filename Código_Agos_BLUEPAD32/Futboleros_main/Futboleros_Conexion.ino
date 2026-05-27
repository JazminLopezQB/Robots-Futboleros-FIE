#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"

extern GamepadInfo gamepads[BP32_MAX_GAMEPADS];

extern bool whitelistEnabled;

extern uint8_t allowedController[6];

extern Preferences prefs;

extern String authorizedGamepad;

extern String connectedGamepad;

// ===================================== Función para Detectar Conexión =====================================
void onConnectedController(ControllerPtr ctl) { // Se crea un objeto ctl
// Se inicializa al Slot a emparejar como vacío.
	bool foundEmptySlot = false; 
    ControllerProperties properties = ctl->getProperties();

// Se recorre la cantidad máxima de mandos a conectar (hasta 4 a la vez)
	for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
	// Si hay slots libres, se conecta el mando:
		if (myControllers[i] == nullptr) {
			Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
		// Creo objeto propiedades y obtengo las mismas
			ControllerProperties properties = ctl->getProperties();
		// Imprimo características de nuestro mando 'ctl'
			Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id, properties.product_id);
		// Le adjudico al slot libre, mi mando		
        myControllers[i] = ctl;

        gamepads[i].ctl = ctl;
        gamepads[i].connected = true;
        gamepads[i].name = ctl->getModelName().c_str();

        foundEmptySlot = true;

			break;
		}
	}
	if (!foundEmptySlot) {
		Serial.println("CALLBACK: Controller connected, but could not found empty slot");
	}

    // =========================================
    // ======== OBTENER MAC DEL MANDO ==========
    // =========================================

    char macStr[18];

    sprintf(
        macStr,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        ctl->getProperties().btaddr[0],
        ctl->getProperties().btaddr[1],
        ctl->getProperties().btaddr[2],
        ctl->getProperties().btaddr[3],
        ctl->getProperties().btaddr[4],
        ctl->getProperties().btaddr[5]
    );

    String currentMac = String(macStr);

    Serial.print("Mando conectado: ");
    Serial.println(currentMac);

    for(int i = 0; i < BP32_MAX_GAMEPADS; i++){

        if(gamepads[i].ctl == ctl){

            gamepads[i].mac = currentMac;
        }
    }

    // =========================================
    // ========= VALIDAR WHITELIST =============
    // =========================================

    if (whitelistEnabled && !pairingMode) {

        bool allowed = memcmp(
            ctl->getProperties().btaddr,
            allowedController,
            6
        ) == 0;

        if (!allowed) {

            Serial.println("Mando NO autorizado");

            ctl->disconnect();

            return;
        }

        Serial.println("Mando autorizado");
    }
}

bool isControllerAllowed(ControllerPtr ctl) {
    if (!whitelistEnabled) return false;

    return memcmp(
        ctl->getProperties().btaddr,
        allowedController,
        6
    ) == 0;
}

// ===================================== Función de Desconexión de Mando =====================================
// Es la misma función de conexión, pero borro con 'nullptr' el mando de la lista de mandos conectados.
void onDisconnectedController(ControllerPtr ctl) {

	bool foundController = false;
	
	for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
		if (myControllers[i] == ctl) {
			Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
		// Borro las características del mando de la lista de mandos conectados
            myControllers[i] = nullptr;

            gamepads[i].connected = false;
            gamepads[i].mac = "";
            gamepads[i].name = "";
            gamepads[i].ctl = nullptr;

            foundController = true;

			break;
		}
	}

	if (!foundController) {
		Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
	}
}

// ===================================== Prueba Serial de Mando =====================================
void dumpGamepad(ControllerPtr ctl) {
	Serial.printf( "a:%d, b:%d, x:%d, y:%d, axis L: %4d, %4d, axis R: %4d, %4d, stickL:%d, stickR: %d, l2:%d, r2:%d, l1: %d, r1: %d \n",
		ctl->a(), // True si A es presionado
		ctl->b(), // True si B es presionado
		ctl->x(), //True si X es presionado
		ctl->y(), // True si Y es presionado
		ctl->axisX(), // (-511 - 512) Eje X Izquierdo
		ctl->axisY(), // (-511 - 512) Eje Y Izquierdo
		ctl->axisRX(), // (-511 - 512) Eje X Derecho
		ctl->axisRY(), // (-511 - 512) Eje Y Derecho
		ctl->thumbL(), // Botón de Stick Izquierdo
		ctl->thumbR(), // Botón de Stick Derecho
	// Botones de R1, R2, L1, L2
		ctl->l1(), 
		ctl->r1(), 
		ctl->brake(), // (0 - 1023)
		ctl->throttle() // (0 - 1023)
	);
}

void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...
    if (ctl->a()) {
        static int colorIdx = 0;
        // Some gamepads like DS4 and DualSense support changing the color LED.
        // It is possible to change it by calling:
        switch (colorIdx % 3) {
            case 0:
                // Red
                ctl->setColorLED(255, 0, 0);
                break;
            case 1:
                // Green
                ctl->setColorLED(0, 255, 0);
                break;
            case 2:
                // Blue
                ctl->setColorLED(0, 0, 255);
                break;
        }
        colorIdx++;
    }

    if (ctl->b()) {
        // Turn on the 4 LED. Each bit represents one LED.
        static int led = 0;
        led++;
        // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
        // support changing the "Player LEDs": those 4 LEDs that usually indicate
        // the "gamepad seat".
        // It is possible to change them by calling:
        ctl->setPlayerLEDs(led & 0x0f);
    }

    if (ctl->x()) {
        // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
        // It is possible to set it by calling:
        // Some controllers have two motors: "strong motor", "weak motor".
        // It is possible to control them independently.
        ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                            0x40 /* strongMagnitude */);
    }

    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    dumpGamepad(ctl);
}

// Función para detectar si soporta el mando a conectar =============================
void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}
