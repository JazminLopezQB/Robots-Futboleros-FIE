#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"
#include "DeclaracionDeFunciones.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ========================================================================
// ============================== SETUP ==============================
// ========================================================================

void setup() {
// Inicialización del monitor Serial
    Serial.begin(115200);

// ================= ADC BATERÍA =================
    pinMode(PIN_BATERIA, INPUT);
    analogReadResolution(12);
    inicioHistorial = millis();

    for (int i = 0; i < MAX_HISTORIAL; i++) {
        historialVoltaje[i] = 0;
        historialTiempo[i] = 0;
    }

// Permite cargar la configuración previa de motores
// en la página para que al abrirla permita ver los parámetros 
// actuales.
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

// Pin del LED:
    pinMode(PIN_LED, OUTPUT);

// Configuración para conexión de mandos:
    //Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    //Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

// ================= WIFI AP =================
    iniciarAccessPoint(true);

// ================= CARGAR WHITELIST =================
// La WHitelist permite en realidad generar una lista de mandos
// "permitidos", sin embargo, en este código en la Web debemos autorizar
// cada mando y borrar la whitelist al emparejar otro .... se prefirió evitar 
// dos mandos que controlen un mismo robot.

    prefs.begin("gamepad", true);

    if (prefs.isKey("mac")) {
        prefs.getBytes("mac", allowedController, 6);
        whitelistEnabled = true;
        char macStr[18];
    
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", allowedController[0], allowedController[1], 
            allowedController[2], allowedController[3], allowedController[4], allowedController[5]
        );

        authorizedGamepad = String(macStr);
        Serial.println("MAC cargada:");
        Serial.println(authorizedGamepad);
    }

    prefs.end();

// ================= WEB =================
    server.on("/", []() {
    
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>

    <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Robot Futbolero</title>

<!-- Comentario: el Style es para configurar el estilo de la página web-->
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

    <!-- Comentario: Esta parte es la que se encarga de mostrar el nivel de batería -->
        <h2>Bateria</h2>

        <div style=" text-align:center; font-size:42px; font-weight:bold; color:#00d4ff; margin-top:15px; " id="voltajeBateria">
            -- V
        </div>

        <div style=" text-align:center; color:#b0bec5; margin-top:5px;">
            Tensión medida por ADC
        </div>
    </div>

<!-- Comentario: Esta parte es la que se encarga de mostrar el nivel de batería en el gráfico -->
    <div class="status">
        <h2>Historial de bateria</h2>
        <canvas id="graficoBateria" width="400" height="220" style="width:100%;"> </canvas>
    </div>

<!-- Comentario: Esta parte autoriza los mandos -->
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

<!-- Comentario: Esta parte es la de los botones de borrar la whitelist y actualizar -->
    <button class="clear" onclick="fetch('/clear').then(()=>location.reload())">
        Borrar whitelist
    </button>

    <button class="refresh" onclick="location.reload()">
        Actualizar
    </button>
    
<!-- Comentario: Esta parte es la que se encarga de mostrar y actualizar la configuración -->
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

            <button class="giro-izq" onclick="girar('izquierda')">
                Girar izquierda
            </button>

            <button class="giro-der" onclick="girar('derecha')">
                Girar derecha
            </button>

        </div>

    <div id="mensajeGiro" class="message"></div>

        <h2>Calibracion de motores</h2>

            <div class="config-item">
                <label>PWM maximo</label>
                <input type="number" id="pwmMax" min="0" max="255" value="220">
            </div>

            <div class="config-item">
                <label>K izquierdo</label>
                <input type="number" id="kIzq" min="0" max="255" value="0">
            </div>

            <div class="config-item">
                <label>K derecho</label>
                <input type="number" id="kDer" min="0" max="255" value="0">
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

<!-- Comentario: Esta parte es la del JavaScript que da las funcionalidades para guardar parámetros y comunicar las acciones de la web con la ESP -->
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

            const pwmMax = document.getElementById("pwmMax").value;

            const kIzq = document.getElementById("kIzq").value;

            const kDer = document.getElementById("kDer").value;

            const anguloGiro = document.getElementById("anguloGiro").value;

            const url = "/guardar" + "?pwmMax=" + pwmMax + "&kIzq=" + kIzq + "&kDer=" + kDer + "&anguloGiro=" + anguloGiro;

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

        function actualizarBateria() {

            fetch('/bateria')
            .then(response => response.json())

            .then(data => {
                document.getElementById("voltajeBateria").innerText =
                    data.voltaje.toFixed(2) + " V";

                dibujarGrafico(data.historial);
            })

            .catch(error => {
                console.log(
                    "Error leyendo batería:",
                    error
                );
            });
        }

        function dibujarGrafico(datos) {

            const canvas = document.getElementById("graficoBateria");

            const ctx = canvas.getContext("2d");

            const ancho = canvas.width;
            const alto = canvas.height;

            ctx.clearRect(0, 0, ancho, alto);

            if (datos.length < 2) {
                return;
            }

            // =========================================
            // BUSCAR MIN / MAX
            // =========================================

            let minV = datos[0].v;
            let maxV = datos[0].v;

            datos.forEach(p => {

                if (p.v < minV)
                    minV = p.v;

                if (p.v > maxV)
                    maxV = p.v;

            });


            // Un poco de margen vertical
            minV -= 0.1;
            maxV += 0.1;

            if (maxV - minV < 0.5) {
                const centro = (maxV + minV) / 2;
                minV = centro - 0.25;
                maxV = centro + 0.25;
            }

            // =========================================
            // EJES
            // =========================================

            ctx.beginPath();

            ctx.moveTo(40, 10);
            ctx.lineTo(40, alto - 30);
            ctx.lineTo(ancho - 10, alto - 30);

            ctx.stroke();


            // =========================================
            // TEXTO MIN / MAX
            // =========================================

            ctx.font = "12px Arial";
            ctx.fillText(maxV.toFixed(1) + " V", 5, 15);
            ctx.fillText(minV.toFixed(1) + " V", 5, alto - 35);

            // =========================================
            // CURVA
            // =========================================

            ctx.beginPath();

            datos.forEach((p, i) => {

                let x = 40 + (i / (datos.length - 1)) * (ancho - 50);

                let y = (alto - 30) - ((p.v - minV) / (maxV - minV)) * (alto - 40);


                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }

            });

            // COLOR DE LA LÍNEA
            ctx.strokeStyle = "#00d4ff";
            ctx.lineWidth = 2;
            ctx.stroke();

            // =========================================
            // INFORMACIÓN
            // =========================================

            ctx.fillText("Min: " + minV.toFixed(2) + " V", 50, alto - 10);
            ctx.fillText("Max: " + maxV.toFixed(2) + " V", 160, alto - 10);

        }

        window.onload = function(){

            cargarConfiguracion();
            actualizarBateria();
            setInterval(actualizarBateria, 1000);
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
            memcpy(allowedController, gamepads[id].ctl->getProperties().btaddr, 6);

            prefs.begin("gamepad", false);
            prefs.putBytes("mac", allowedController, 6);
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

// ==========================================================
// ================= DATOS DE BATERÍA =======================
// ==========================================================

server.on("/bateria", HTTP_GET, []() {

    String json = "{";

    json += "\"voltaje\":";
    json += String(voltajeBateria, 2);

    json += ",\"adc\":";
    json += String(analogRead(PIN_BATERIA));

    json += ",\"historial\":";
    json += obtenerHistorialJSON();

    json += "}";

    server.send(
        200,
        "application/json",
        json
    );
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

    if (grados != 45 && grados != 90 && grados != 180 && grados != 270 && grados != 360) {
        server.send(400, "text/plain", "Angulo no permitido");
        return;
    }

/*
    Serial.print("Giro solicitado: ");
    Serial.print(direccion);
    Serial.print(" ");
    Serial.print(anguloGiro);
    Serial.println(" grados");
*/

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
// ================= BATERÍA =================
    if (millis() - ultimaLecturaBateria >= INTERVALO_BATERIA) {
        ultimaLecturaBateria = millis();
        leerBateria();
    }

// Esta función permite controlar la duración del AP, ya sean 2 minutos al inciar el ESP
// o tiempo indefinido con los botones de start y select.
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

        // Se bloquea si no está autorizado
        if (!isControllerAllowed(ctl))
            continue;            

// ================= CONTROL MANUAL DEL AP =================
    // START → encender AP permanentemente
        bool botonStart = ctl->miscStart();

        if (botonStart && !botonStartAnterior) {
            //Serial.println("START -> Encendiendo AP");
            iniciarAccessPoint(false);
        }

        botonStartAnterior = botonStart;

    // SELECT → apagar AP
        bool botonSelect = ctl->miscSelect();

        if (botonSelect && !botonSelectAnterior) {
            //Serial.println("SELECT -> Apagando AP");
            apagarAccessPoint();
        }

        botonSelectAnterior = botonSelect;

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
        bool botonMovimiento = ctl->y() || ctl->a() || ctl->b() || ctl->x();

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

        // Si apreto R1, entonces se activa el turbo y los motores de adelante aceleran en línea recta a todo lo que da el PWM
        bool turbo = ctl->r1();
        
        // Serial.printf("Botones: %2X:%2X:%2X\n", ctl->brake(), ctl->throttle(), ctl->r1());
        
        if (!botonMovimiento) { 
            movimiento(ctl, turbo); 
        }
    }

    delay(1);
}