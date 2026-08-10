#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Declaraciones:
void onConnectedController(ControllerPtr ctl);
void onDisconnectedController(ControllerPtr ctl);
void processControllers();
void processControllers();
void giro180Izquierda();
void giro180Derecha();
void iniciarAccessPoint();
void apagarAccessPoint(); 
void controlarAccessPoint();
void cargarConfiguracionMotores();
void escribirPWM();

void giroIzquierda(int grados);
void giroDerecha(int grados);
//

bool apActivo = false;
unsigned long inicioAP = 0;
const unsigned long DURACION_AP = 120000; // 2 minutos
bool botonMenuAnterior = false;

// Variables Globales para Web
WebServer server(80);
Preferences prefs;

// ================= CALIBRACIÓN DE MOTORES =================

uint8_t pwmMaxConfigurado = PWM_MAX;
uint8_t K_IZQ = 0;
uint8_t K_DER = 0;

uint8_t allowedController[6];

bool whitelistEnabled = false;
bool pairingMode = false;

unsigned long pairingStart = 0;
const unsigned long pairingTimeout = 30000;

String authorizedGamepad = "Ninguno";

struct GamepadInfo {

    ControllerPtr ctl;

    String mac;

    String name;

    bool connected;
};

String connectedGamepad = "Ninguno";

GamepadInfo gamepads[BP32_MAX_GAMEPADS];
ControllerPtr myControllers[BP32_MAX_GAMEPADS];


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
    escribirPWM(
        0,  // adelante izquierda
        0,  // atrás izquierda
        0,  // adelante derecha
        0   // atrás derecha
    );

    String sentidoIzq = "DETENIDO";
    String sentidoDer = "DETENIDO";
}

// =============== Función para mover los motores con 255 ===============
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

// =============== Función para mover los motores ===============
void movimiento(ControllerPtr ctl) {

    float x = ctl->axisRX();
    float y = -ctl->axisRY();

    // Serial.printf("X:%d Y:%d\n", x, y);

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

    float pwmIzq = (normY + normX) * 127.0 * factorVelocidad; // -Bestia1.0 //otros al reves
    float pwmDer = (normY - normX) * 127.0 * factorVelocidad; // +Bestia1.0 //otros al reves

    // Se limita el PWM de los motores con los valores predefinidos en 'config.h'
    pwmIzq = constrain(pwmIzq, -PWM_MAX, PWM_MAX);
    pwmDer = constrain(pwmDer, -PWM_MAX, PWM_MAX);

    // Detecto los sentidos y activo los motores correspondientes para el giro:
    if (pwmIzq >= 0) {
        ledcWrite(CH_ADELANTE_IZQ, pwmIzq + K_IZQ);
        ledcWrite(CH_ATRAS_IZQ, 0);
    } else {
        ledcWrite(CH_ADELANTE_IZQ, 0);
        ledcWrite(CH_ATRAS_IZQ, -pwmIzq + K_IZQ);
    }

    if (pwmDer >= 0) {
        ledcWrite(CH_ADELANTE_DER, pwmDer + K_DER);
        ledcWrite(CH_ATRAS_DER, 0);
    } else {
        ledcWrite(CH_ADELANTE_DER, 0);
        ledcWrite(CH_ATRAS_DER, - pwmDer + K_DER);
    }
}

void iniciarAccessPoint() {

    WiFi.mode(WIFI_AP);

    // IMPORTANTE:
    // No usamos WiFi.setSleep(false)
    // porque queremos reducir consumo.

    //Bestia 1.0 192.168.9.1
    //Bestia 2.0 192.168.10.1
    //Bella 192.168.11.1

    IPAddress local_IP(192, 168, 10, 1);
    IPAddress gateway(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAPConfig(local_IP, gateway, subnet);


    bool ok = WiFi.softAP(
        "Robot-Bestia2.0",
        "Fulbo123",
        1,
        false,
        1
    );

    if (ok) {

        apActivo = true;
        inicioAP = millis();

        Serial.println("========== WIFI AP ==========");
        Serial.println("AP iniciado");
        Serial.print("IP: ");
        Serial.println(WiFi.softAPIP());

    } else {

        Serial.println("ERROR AL INICIAR AP");

    }
}

void apagarAccessPoint() {

    if (!apActivo)
        return;

    Serial.println("========== WIFI AP ==========");
    Serial.println("2 minutos cumplidos.");
    Serial.println("Apagando Access Point...");

    // Apaga solamente el Access Point.
    // NO apagamos Bluetooth.
    WiFi.softAPdisconnect(false);

    apActivo = false;

    Serial.println("Access Point apagado.");
    Serial.println("Bluetooth permanece activo.");
}

void controlarAccessPoint() {
    if (!apActivo)
        return;

    if (millis() - inicioAP >= DURACION_AP) {

        apagarAccessPoint();

    }
}

void cargarConfiguracionMotores() {

    prefs.begin("config", true);

    PWM_MAX = prefs.getUChar("pwmMax", 220);
    K_IZQ = prefs.getUChar("kIzq", 0);
    K_DER = prefs.getUChar("kDer", 0);
    anguloGiro = prefs.getInt("anguloGiro", 180);

    prefs.end();

    Serial.println("===== CONFIGURACION MOTORES =====");
    Serial.printf("PWM MAX: %d\n", PWM_MAX);
    Serial.printf("K IZQ: %d\n", K_IZQ);
    Serial.printf("K DER: %d\n", K_DER);
}

void guardarConfiguracionMotores() {

    prefs.begin("config", false);

    prefs.putUChar("pwmMax", PWM_MAX);
    prefs.putUChar("kIzq", K_IZQ);
    prefs.putUChar("kDer", K_DER);

    // Guardar también el ángulo
    prefs.putInt("anguloGiro", anguloGiro);

    prefs.end();

    Serial.println("Configuracion de motores guardada.");
    Serial.print("PWM MAX: ");
    Serial.println(PWM_MAX);
    Serial.print("K IZQ: ");
    Serial.println(K_IZQ);
    Serial.print("K DER: ");
    Serial.println(K_DER);
    Serial.print("ANGULO GIRO: ");
    Serial.println(anguloGiro);
}

void escribirPWM(
    int adelanteIzq,
    int atrasIzq,
    int adelanteDer,
    int atrasDer
) {

    pwmActualAdelanteIzq = adelanteIzq;
    pwmActualAtrasIzq = atrasIzq;
    pwmActualAdelanteDer = adelanteDer;
    pwmActualAtrasDer = atrasDer;

    ledcWrite(CH_ADELANTE_IZQ, adelanteIzq);
    ledcWrite(CH_ATRAS_IZQ, atrasIzq);
    ledcWrite(CH_ADELANTE_DER, adelanteDer);
    ledcWrite(CH_ATRAS_DER, atrasDer);
}
// ========================================================================
// ============================== SETUP ==============================
// ========================================================================

void setup() {
  Serial.begin(115200);

  cargarConfiguracionMotores();
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

    // ====================== WIFI AP ==========================
    iniciarAccessPoint();


    // =================== CARGAR WHITELIST ====================

    prefs.begin("gamepad", true);

    if (prefs.isKey("mac")) {

        prefs.getBytes("mac", allowedController, 6);

        whitelistEnabled = true;

        char macStr[18];

        sprintf(
            macStr,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            allowedController[0],
            allowedController[1],
            allowedController[2],
            allowedController[3],
            allowedController[4],
            allowedController[5]
        );

        authorizedGamepad = String(macStr);

        Serial.println("MAC cargada:");
        Serial.println(authorizedGamepad);
    }

    prefs.end();

    // =================== WEB ====================

    server.on("/", []() {

    String html = R"rawliteral(

    <!DOCTYPE html>
    <html>

    <head>

    <meta name="viewport" content="width=device-width, initial-scale=1">

    <title>Robot Futbolero</title>

    <style>
    .config-item select{
        width:100%;
        box-sizing:border-box;
        border:none;
        border-radius:14px;
        padding:14px;
        font-size:18px;
        background:#1c1f26;
        color:white;
        outline:none;
    }

    .config-item select:focus{
        box-shadow:0 0 0 2px #00d4ff;
    }

    .giro-botones{
        display:flex;
        gap:10px;
        margin-top:10px;
    }

    .giro-botones button{
        width:50%;
    }

    .giro-izq{
        background:#7c4dff;
        color:white;
    }

    .giro-der{
        background:#00bfa5;
        color:white;
    }

    body{
        margin:0;
        padding:0;
        background:#0f1117;
        color:white;
        font-family:Arial;
    }

    .container{
        width:90%;
        max-width:500px;
        margin:auto;
        margin-top:40px;
    }

    .card{
        background:#1c1f26;
        border-radius:25px;
        padding:25px;
        box-shadow:0 0 25px rgba(0,0,0,0.4);
    }

    .title{
        text-align:center;
        font-size:34px;
        font-weight:bold;
        color:#00d4ff;
        margin-bottom:25px;
    }

    .status{
        background:#2a2f3a;
        border-radius:18px;
        padding:18px;
        margin-top:15px;
    }

    .status h2{
        margin:0;
        font-size:20px;
        color:#b0bec5;
    }

    .status p{
        margin-top:10px;
        font-size:18px;
        word-break:break-all;
    }

    button{
        width:100%;
        border:none;
        border-radius:18px;
        padding:18px;
        font-size:18px;
        font-weight:bold;
        margin-top:18px;
        cursor:pointer;
        transition:0.2s;
    }

    button:hover{
        transform:scale(1.02);
    }

    .pair{
        background:#00c853;
        color:white;
    }

    .disconnect{
        background:#ff9800;
        color:white;
    }

    .clear{
        background:#ff5252;
        color:white;
    }

    .refresh{
        background:#2979ff;
        color:white;
    }
    .config-title{
    margin-top:30px;
    margin-bottom:10px;
    font-size:22px;
    }

    .config-item{
        margin-top:18px;
    }

    .config-item label{
        display:block;
        margin-bottom:8px;
        font-size:16px;
        color:#b0bec5;
    }

    .config-item input{
        width:100%;
        box-sizing:border-box;
        border:none;
        border-radius:14px;
        padding:14px;
        font-size:18px;
        background:#1c1f26;
        color:white;
        outline:none;
    }

    .config-item input:focus{
        box-shadow:0 0 0 2px #00d4ff;
    }

    .save{
        background:#00c853;
        color:white;
    }

    .motor{
        display:flex;
        justify-content:space-between;
        align-items:center;
        margin-top:12px;
    }

    .motor-value{
        font-size:20px;
        font-weight:bold;
        color:#00d4ff;
    }

    .motor-direction{
        font-size:15px;
        color:#b0bec5;
    }

    .message{
        text-align:center;
        margin-top:15px;
        color:#00d4ff;
    }

    .footer{
        text-align:center;
        margin-top:25px;
        color:#777;
        font-size:14px;
    }

    </style>

    </head>

    <body>

    <div class="container">

    <div class="card">

    <div class="title">
    Robot Futbolero
    </div>

    <div class="status">

    <h2>Joystick autorizado</h2>

    <p>
    )rawliteral";

    html += authorizedGamepad;

html += R"rawliteral(
    </p>

    </div>
)rawliteral";

    html += "<h2>Mandos conectados</h2>";

    for(int i = 0; i < BP32_MAX_GAMEPADS; i++){

        if(gamepads[i].connected){

            html += "<div class='status'>";

            html += "<p><b>";
            html += gamepads[i].name;
            html += "</b></p>";

            html += "<p>";
            html += gamepads[i].mac;
            html += "</p>";

            html += "<button class='pair' onclick=\"fetch('/authorize?id=";
            html += String(i);
            html += "').then(()=>location.reload())\">Autorizar</button>";

            html += "<button class='disconnect' onclick=\"fetch('/disconnect?id=";
            html += String(i);
            html += "').then(()=>location.reload())\">Desconectar</button>";

            html += "</div>";
        }
    }
    

html += R"rawliteral(
 <button class="clear"
    onclick="fetch('/clear').then(()=>location.reload())">

    Borrar whitelist

    </button>

    <button class="refresh"
    onclick="location.reload()">

    Actualizar

    </button>
    
<div class="status">

<h2>Configuracion de giro</h2>

    <div class="config-item">

        <label>Angulo de giro</label>

        <select id="anguloGiro">

            <option value="45">45</option>
            <option value="90">90</option>
            <option value="180" selected>180</option>
            <option value="270">270</option>
            <option value="360">360</option>

        </select>

    </div>

    <div class="giro-botones">

        <button class="giro-izq"
            onclick="girar('izquierda')">
            Girar izquierda
        </button>

        <button class="giro-der"
            onclick="girar('derecha')">
            Girar derecha
        </button>

    </div>

    <div id="mensajeGiro" class="message"></div>

<h2>Calibracion de motores</h2>

    <div class="config-item">
        <label>PWM maximo</label>
        <input
            type="number"
            id="pwmMax"
            min="0"
            max="255"
            value="220">
    </div>

    <div class="config-item">
        <label>K izquierdo</label>
        <input
            type="number"
            id="kIzq"
            min="0"
            max="255"
            value="0">
    </div>

    <div class="config-item">
        <label>K derecho</label>
        <input
            type="number"
            id="kDer"
            min="0"
            max="255"
            value="0">
    </div>

    <button
        class="save"
        onclick="guardarMotores()">
        Guardar configuración
    </button>

    <div id="mensajeConfig" class="message"></div>

</div>

    <div class="footer">

    ESP32 + Bluepad32

    </div>

    </div>

    </div>

    <script>
    function girar(direccion){

    const angulo =
        document.getElementById("anguloGiro").value;

    const url =
        "/girar" +
        "?direccion=" + direccion +
        "&angulo=" + angulo;

    fetch(url)

    .then(response => response.text())

    .then(data => {

        document.getElementById("mensajeGiro").innerText =
            "✓ " + data;

    })

    .catch(error => {

        document.getElementById("mensajeGiro").innerText =
            "✗ Error al realizar el giro";

        console.log(error);

    });
}

    function cargarConfiguracion(){

        fetch('/config')

        .then(response => response.json())

        .then(data => {

            document.getElementById("pwmMax").value = data.pwmMax;
            document.getElementById("kIzq").value = data.kIzq;
            document.getElementById("kDer").value = data.kDer;
            document.getElementById("anguloGiro").value = data.anguloGiro;

        })

        .catch(error => {

            console.log("Error cargando configuración:", error);

        });
    }

function guardarMotores(){

    const pwmMax =
        document.getElementById("pwmMax").value;

    const kIzq =
        document.getElementById("kIzq").value;

    const kDer =
        document.getElementById("kDer").value;

    const anguloGiro =
        document.getElementById("anguloGiro").value;


    const url =
        "/guardar" +
        "?pwmMax=" + pwmMax +
        "&kIzq=" + kIzq +
        "&kDer=" + kDer +
        "&anguloGiro=" + anguloGiro;


    fetch(url)

    .then(response => response.text())

    .then(data => {

        document.getElementById("mensajeConfig").innerText =
            "✓ Configuración guardada";

    })

    .catch(error => {

        document.getElementById("mensajeConfig").innerText =
            "✗ Error al guardar";

        console.log(error);

    });

}


    window.onload = function(){

        cargarConfiguracion();

    };

    </script>

    </body>
    </html>

)rawliteral";

    server.send(200, "text/html", html);

});

server.on("/authorize", [](){

    if(server.hasArg("id")){

        int id = server.arg("id").toInt();

        if(gamepads[id].connected){

            memcpy(
                allowedController,
                gamepads[id].ctl->getProperties().btaddr,
                6
            );

            prefs.begin("gamepad", false);

            prefs.putBytes(
                "mac",
                allowedController,
                6
            );

            prefs.end();

            whitelistEnabled = true;

            authorizedGamepad = gamepads[id].mac;

            server.send(200, "text/plain", "Autorizado");

            return;
        }
    }

    server.send(400, "text/plain", "Error");
});

server.on("/clear", []() {

    prefs.begin("gamepad", false);

    prefs.clear();

    prefs.end();

    whitelistEnabled = false;

    authorizedGamepad = "Ninguno";

    server.send(200, "text/plain", "Whitelist borrada");
});

server.on("/disconnect", [](){

    if(server.hasArg("id")){

        int id = server.arg("id").toInt();

        if(gamepads[id].connected){

            gamepads[id].ctl->disconnect();

            server.send(200, "text/plain", "Desconectado");

            return;
        }
    }

    server.send(400, "text/plain", "Error");
});

server.on("/config", HTTP_GET, []() {

    String json = "{";

    json += "\"pwmMax\":" + String(PWM_MAX);
    json += ",\"kIzq\":" + String(K_IZQ);
    json += ",\"kDer\":" + String(K_DER);
    json += ",\"anguloGiro\":" + String(anguloGiro);

    json += "}";

    server.send(200, "application/json", json);
});

server.on("/guardar", HTTP_GET, []() {

    if (server.hasArg("pwmMax"))
        PWM_MAX = server.arg("pwmMax").toInt();

    if (server.hasArg("kIzq"))
        K_IZQ = server.arg("kIzq").toInt();

    if (server.hasArg("kDer"))
        K_DER = server.arg("kDer").toInt();

if (server.hasArg("anguloGiro"))
    anguloGiro = server.arg("anguloGiro").toInt();

    guardarConfiguracionMotores();
    server.send(200, "text/plain", "Configuración guardada");
});

server.on("/girar", HTTP_GET, []() {

    if (!server.hasArg("direccion") || !server.hasArg("angulo")) {

        server.send(400, "text/plain", "Faltan parametros");
        return;
    }

    String direccion = server.arg("direccion");

    int grados = server.arg("angulo").toInt();

    if (grados != 45 &&
        grados != 90 &&
        grados != 180 &&
        grados != 270 &&
        grados != 360) {

        server.send(400, "text/plain", "Angulo no permitido");
        return;
    }


    Serial.print("Giro solicitado: ");
    Serial.print(direccion);
    Serial.print(" ");
    Serial.print(anguloGiro);
    Serial.println(" grados");


    if (direccion == "izquierda") {

        giroIzquierda(grados);

        server.send(
            200,
            "text/plain",
            "Giro izquierda " + String(anguloGiro) + " grados"
        );

        return;
    }


    if (direccion == "derecha") {

        giroDerecha(grados);

        server.send(
            200,
            "text/plain",
            "Giro derecha " + String(anguloGiro) + " grados"
        );

        return;
    }


    server.send(400, "text/plain", "Direccion no valida");
});

    server.begin();
    Serial.println("Servidor web iniciado");

    // Setear los callbacks del Bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // El siguiente comando permite borrar la lista de conexiones para reset.
    // BP32.forgetBluetoothKeys();

}

// ========================================================================
// ============================== LOOP ==============================
// ========================================================================

void loop() {
    controlarAccessPoint();

    if (apActivo) {
        server.handleClient();
    }

    bool dataUpdated = BP32.update();

    if (dataUpdated)
        processControllers();

    // Se recorre la lista de mandos conectados, y si hay uno conectado y está enviando información: 
    for (auto ctl : myControllers) {

        if (!ctl || !ctl->isConnected() || !ctl->hasData())
            continue;

        // 🔥 CAMBIO CLAVE: bloqueo si no está autorizado
        if (!isControllerAllowed(ctl))
            continue;            

        // ================= CONTROL DEL ACCESS POINT =================

        bool botonMenu = ctl->miscStart();

        if (botonMenu && !botonMenuAnterior) {

            //Serial.println("================================");
            //Serial.println("BOTON MENU DETECTADO");
            //Serial.println("Encendiendo Access Point...");
            //Serial.println("================================");

            iniciarAccessPoint();
        }

        botonMenuAnterior = botonMenu;

            // Booleano para detectar si se detectó los botones para subir y bajar la velocidad
            // sabiendo también si antes se presionó o no para que no haya efecto rebote.
            static bool prevY = false;
            static bool prevA = false;

            bool y = ctl->dpad() & DPAD_UP; // subir
            bool a = ctl->dpad() & DPAD_DOWN; // bajar

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
            // ================= CONTROL POR BOTONES =================

            antiReboteTriang(ctl);
            antiReboteCruz(ctl);
            antiReboteCirc(ctl);
            antiReboteCuad(ctl);


            // ================= CONTROL POR STICK =================

            bool botonMovimiento =
                ctl->y() ||
                ctl->a() ||
                ctl->b() ||
                ctl->x();


            static unsigned long ultimoIzquierda = 0;
            if (ctl->brake() && millis() - ultimoIzquierda > tiempoRebote) {
                giroIzquierda(anguloGiro);
                ultimoIzquierda = millis();
            }
            
            static unsigned long ultimoDerecha = 0;
            if (ctl->throttle() && millis() - ultimoDerecha > tiempoRebote) {
                giroDerecha(anguloGiro);
                ultimoDerecha = millis();
            }

            //Si apreto R1, entonces se activa el turbo y los motores de adelante aceleran en línea recta a todo lo que da el PWM
            bool turbo = ctl->r1();
    
            //Serial.printf("Botones: %2X:%2X:%2X\n", ctl->brake(), ctl->throttle(), ctl->r1());

            if (turbo) {
                ledcWrite(CH_ADELANTE_IZQ, 255);
                ledcWrite(CH_ADELANTE_DER, 255);
                ledcWrite(CH_ATRAS_IZQ, 0);
                ledcWrite(CH_ATRAS_DER, 0);
            } else if (!botonMovimiento) {
                movimiento(ctl);
            }
        }

    delay(1);
}