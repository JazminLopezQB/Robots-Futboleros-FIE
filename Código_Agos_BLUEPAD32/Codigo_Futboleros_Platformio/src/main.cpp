#include <Bluepad32.h>
#include <Arduino.h>
#include "config.h"
#include "DeclaracionDeFunciones.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;
// ========================================================================
// ============================== SETUP ==============================
// ========================================================================

void setup() {
    Serial.begin(115200);

    // ================= ADC BATERÍA =================
    pinMode(PIN_BATERIA, INPUT);
    analogReadResolution(12);
    inicioHistorial = millis();

    for (int i = 0; i < MAX_HISTORIAL; i++) {
        historialVoltaje[i] = 0;
        historialTiempo[i] = 0;
    }

    cargarConfiguracionMotores();

    // Cargar último perfil activo al iniciar
    Preferences prefs;
    prefs.begin("perfiles", true);
    int ultimoPerfil = prefs.getInt("activo", 0);
    prefs.end();
    cargarPerfil(ultimoPerfil);

    // Pines PWM
    ledcSetup(CH_ADELANTE_DER, PWM_FREQ, PWM_RES);
    ledcSetup(CH_ATRAS_DER, PWM_FREQ, PWM_RES);
    ledcSetup(CH_ADELANTE_IZQ, PWM_FREQ, PWM_RES);
    ledcSetup(CH_ATRAS_IZQ, PWM_FREQ, PWM_RES);

    ledcAttachPin(Adelante_Der, CH_ADELANTE_DER);
    ledcAttachPin(Atras_Der, CH_ATRAS_DER);
    ledcAttachPin(Adelante_Izq, CH_ADELANTE_IZQ);
    ledcAttachPin(Atras_Izq, CH_ATRAS_IZQ);

    pinMode(PIN_LED, OUTPUT);

    // ================= WIFI AP =================
    iniciarAccessPoint(true);

    // ================= WHITELIST =================
    prefs.begin("gamepad", true);
    if (prefs.isKey("mac")) {
        prefs.getBytes("mac", allowedController, 6);
        whitelistEnabled = true;
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                allowedController[0], allowedController[1], allowedController[2], 
                allowedController[3], allowedController[4], allowedController[5]);
        authorizedGamepad = String(macStr);
    }
    prefs.end();

    // ================= SERVIDOR WEB =================
    server.on("/", []() {
        String html = R"rawliteral(
        <!DOCTYPE html>
        <html lang="es">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Robot Futbolero</title>
            <style>
                * { box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
                body { margin: 0; padding: 20px 10px; background: #0b0e14; color: #e1e7ec; display: flex; justify-content: center; }
                .container { width: 100%; max-width: 520px; }
                
                .header-title { text-align: center; font-size: 28px; font-weight: 800; color: #00d4ff; text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 20px; text-shadow: 0 0 10px rgba(0,212,255,0.3); }
                
                .card { background: #161b22; border-radius: 16px; padding: 20px; margin-bottom: 20px; border: 1px solid #21262d; box-shadow: 0 8px 24px rgba(0,0,0,0.5); }
                .card-title { font-size: 16px; font-weight: 700; color: #58a6ff; text-transform: uppercase; letter-spacing: 1px; margin-top: 0; margin-bottom: 15px; border-bottom: 1px solid #30363d; padding-bottom: 8px; }
                
                .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
                .grid-3 { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 8px; }
                
                .config-item { margin-top: 12px; }
                .config-item label { display: block; font-size: 13px; color: #8b949e; margin-bottom: 6px; font-weight: 600; }
                .config-item input, .config-item select { width: 100%; border: 1px solid #30363d; border-radius: 8px; padding: 12px; font-size: 14px; background: #0d1117; color: #c9d1d9; outline: none; transition: 0.2s; }
                .config-item input:focus, .config-item select:focus { border-color: #58a6ff; box-shadow: 0 0 0 3px rgba(88,166,255,0.15); }
                
                .checkbox-item { display: flex; align-items: center; gap: 10px; margin-top: 12px; cursor: pointer; }
                .checkbox-item input { width: 18px; height: 18px; accent-color: #00d4ff; }
                .checkbox-item label { font-size: 14px; color: #c9d1d9; cursor: pointer; }

                button { width: 100%; border: none; border-radius: 10px; padding: 12px 8px; font-size: 13px; font-weight: 700; margin-top: 12px; cursor: pointer; transition: all 0.2s; text-transform: uppercase; letter-spacing: 0.5px; }
                button:active { transform: scale(0.97); }
                
                .btn-primary { background: #238636; color: #fff; }
                .btn-primary:hover { background: #2ea043; }
                .btn-sec { background: #1f6feb; color: #fff; }
                .btn-danger { background: #da3633; color: #fff; }
                .btn-purple { background: #8957e5; color: #fff; }
                .btn-teal { background: #117ca6; color: #fff; }

                .voltage-num { text-align: center; font-size: 42px; font-weight: 800; color: #3fb950; margin: 10px 0; font-family: monospace; }
                .message { text-align: center; margin-top: 12px; font-size: 13px; font-weight: 600; color: #58a6ff; min-height: 18px; }
                .footer { text-align: center; font-size: 12px; color: #484f58; margin-top: 20px; }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header-title">Robot Futbolero</div>

                <!-- BATERÍA -->
                <div class="card">
                    <div class="card-title">Estado de Batería</div>
                    <div class="voltage-num" id="voltajeBateria">-- V</div>
                    <canvas id="graficoBateria" width="400" height="160" style="width:100%;"></canvas>
                </div>

                <!-- GESTIÓN DE PERFILES -->
                <div class="card">
                    <div class="card-title">Gestión de Perfiles</div>
                    <div class="config-item">
                        <label>Seleccionar Perfil Activo</label>
                        <select id="selectPerfil" onchange="cambiarPerfilWeb(this.value)">
                        </select>
                    </div>
                    
                    <div class="config-item" id="seccionNuevoNombre" style="display:none;">
                        <label>Nombre del Nuevo Perfil</label>
                        <input type="text" id="nombrePerfilNuevo" placeholder="Ej: Ataque Rapido">
                    </div>

                    <div class="grid-3">
                        <button class="btn-sec" onclick="mostrarCrearPerfil()">+ Nuevo</button>
                        <button class="btn-primary" onclick="guardarPerfilWeb()">Guardar</button>
                        <button class="btn-danger" onclick="eliminarPerfilWeb()">Eliminar</button>
                    </div>
                    <div id="mensajeConfig" class="message"></div>
                </div>

                <!-- CONFIGURACIÓN DE STICK Y BOTONES -->
                <div class="card">
                    <div class="card-title">Ajustes de Control</div>
                    <div class="grid-2">
                        <div class="config-item">
                            <label>Factor Corrección Giro (kGiro)</label>
                            <input type="number" id="kGiro" step="0.05" min="0" max="3" value="1.0">
                        </div>
                        <div class="config-item">
                            <label>Stick para Corrección Giro</label>
                            <select id="stickGiro">
                                <option value="0">Stick Izquierdo (X)</option>
                                <option value="1">Stick Derecho (RX)</option>
                            </select>
                        </div>
                    </div>
                    <div class="checkbox-item">
                        <input type="checkbox" id="invX">
                        <label for="invX">Invertir Giro Stick (Eje X)</label>
                    </div>
                    <div class="checkbox-item">
                        <input type="checkbox" id="invY">
                        <label for="invY">Invertir Avance Stick (Eje Y)</label>
                    </div>
                    <div class="config-item">
                        <label>Zona Muerta Stick (1 a 50)</label>
                        <input type="number" id="zonaMuerta" min="1" max="50" value="10">
                    </div>

                    <div class="card-title" style="margin-top:20px;">Mapeo de Botones</div>
                    <div class="grid-2">
                        <div class="config-item">
                            <label>⬆️ Adelante</label>
                            <select id="btnF">
                                <option value="1">Y / Triángulo</option>
                                <option value="2">A / Cruz</option>
                                <option value="3">B / Círculo</option>
                                <option value="4">X / Cuadrado</option>
                                <option value="5">L1</option>
                                <option value="6">R1</option>
                            </select>
                        </div>
                        <div class="config-item">
                            <label>⬇️ Atrás</label>
                            <select id="btnB">
                                <option value="1">Y / Triángulo</option>
                                <option value="2">A / Cruz</option>
                                <option value="3">B / Círculo</option>
                                <option value="4">X / Cuadrado</option>
                                <option value="5">L1</option>
                                <option value="6">R1</option>
                            </select>
                        </div>
                        <div class="config-item">
                            <label>⬅️ Izquierda</label>
                            <select id="btnL">
                                <option value="1">Y / Triángulo</option>
                                <option value="2">A / Cruz</option>
                                <option value="3">B / Círculo</option>
                                <option value="4">X / Cuadrado</option>
                                <option value="5">L1</option>
                                <option value="6">R1</option>
                            </select>
                        </div>
                        <div class="config-item">
                            <label>➡️ Derecha</label>
                            <select id="btnR">
                                <option value="1">Y / Triángulo</option>
                                <option value="2">A / Cruz</option>
                                <option value="3">B / Círculo</option>
                                <option value="4">X / Cuadrado</option>
                                <option value="5">L1</option>
                                <option value="6">R1</option>
                            </select>
                        </div>
                    </div>
                </div>

                <!-- CALIBRACIÓN Y TEST -->
                <div class="card">
                    <div class="card-title">Calibración de Motores</div>
                    <div class="config-item">
                        <label>PWM Máximo (0 - 255)</label>
                        <input type="number" id="pwmMax" min="0" max="255" value="220">
                    </div>
                    <div class="grid-2">
                        <div class="config-item">
                            <label>K Izquierdo</label>
                            <input type="number" id="kIzq" min="0" max="255" value="0">
                        </div>
                        <div class="config-item">
                            <label>K Derecho</label>
                            <input type="number" id="kDer" min="0" max="255" value="0">
                        </div>
                    </div>

                    <div class="card-title" style="margin-top:20px;">Prueba de Giro</div>
                    <div class="config-item">
                        <label>Ángulo de Giro</label>
                        <select id="anguloGiro">
                            <option value="45">45°</option>
                            <option value="90">90°</option>
                            <option value="180" selected>180°</option>
                            <option value="270">270°</option>
                            <option value="360">360°</option>
                        </select>
                    </div>
                    <div class="grid-2">
                        <button class="btn-purple" onclick="girar('izquierda')">Girar Izq</button>
                        <button class="btn-teal" onclick="girar('derecha')">Girar Der</button>
                    </div>
                    <div id="mensajeGiro" class="message"></div>
                </div>

                <!-- MANDOS DETECTADOS Y RENOMBRAR -->
                <div class="card">
                    <div class="card-title">Mandos Conectados</div>
                    <div id="contenedorMandos">
                        <p style="color:#8b949e; font-size:13px;">Cargando mandos...</p>
                    </div>
                    
                    <div class="grid-2" style="margin-top:15px;">
                        <button class="btn-danger" onclick="fetch('/clear').then(()=>location.reload())">Borrar Whitelist</button>
                        <button class="btn-sec" onclick="cargarMandosWeb()">Actualizar</button>
                    </div>
                </div>

                <div class="footer">ESP32 + Bluepad32 Control System</div>
            </div>

            <script>
                let creandoNuevo = false;

                window.onload = function() {
                    cargarListaPerfiles();
                    actualizarBateria();
                    cargarMandosWeb();
                    setInterval(actualizarBateria, 2000);
                    setInterval(cargarMandosWeb, 4000);
                };

                function cargarMandosWeb() {
                    fetch('/getJoysticks')
                    .then(response => response.json())
                    .then(mandos => {
                        const contenedor = document.getElementById("contenedorMandos");
                        if (!mandos || mandos.length === 0) {
                            contenedor.innerHTML = "<p style='color:#8b949e; font-size:13px;'>No hay joysticks conectados.</p>";
                            return;
                        }

                        // Si el usuario está escribiendo en algún input, NO redibujamos para no interrumpir
                        if (document.activeElement && document.activeElement.tagName === "INPUT" && document.activeElement.id.startsWith("input-alias-")) {
                            return;
                        }

                        let html = "";
                        mandos.forEach(m => {
                            html += `
                            <div style="background:#0d1117; border:1px solid #30363d; border-radius:10px; padding:12px; margin-bottom:10px;">
                                <div style="font-size:12px; color:#8b949e; margin-bottom:4px;">MAC: <span style="font-family:monospace; color:#58a6ff;">${m.mac}</span> (Slot ${m.slot})</div>
                                <div class="config-item" style="margin-top:0;">
                                    <input type="text" id="input-alias-${m.slot}" value="${m.nombre}" placeholder="Ej: Mando Titular Agos" style="padding:8px 10px; font-size:13px;">
                                </div>
                                <div class="grid-3" style="margin-top:8px;">
                                    <button class="btn-primary" onclick="guardarNombreMando('${m.mac}', ${m.slot})">💾 Nombre</button>
                                    <button class="btn-teal" onclick="fetch('/authorize?id=${m.slot}').then(()=>location.reload())">Autorizar</button>
                                    <button class="btn-danger" onclick="fetch('/disconnect?id=${m.slot}').then(()=>cargarMandosWeb())">Desconectar</button>
                                </div>
                            </div>`;
                        });
                        contenedor.innerHTML = html;
                    })
                    .catch(err => console.error("Error al obtener mandos:", err));
                }

                function guardarNombreMando(mac, slot) {
                    const input = document.getElementById(`input-alias-${slot}`);
                    if (!input) return;
                    
                    const nuevoNombre = input.value.trim();
                    if (!nuevoNombre) return;

                    fetch(`/renombrarJoystick?mac=${encodeURIComponent(mac)}&nombre=${encodeURIComponent(nuevoNombre)}`)
                        .then(res => res.text())
                        .then(data => {
                            console.log("Respuesta servidor:", data);
                            cargarMandosWeb(); // Recarga la lista para verificar el cambio
                        })
                        .catch(err => console.error("Error al renombrar:", err));
                }

                function cargarListaPerfiles() {
                    fetch('/listarPerfiles')
                    .then(r => r.json())
                    .then(data => {
                        const select = document.getElementById("selectPerfil");
                        select.innerHTML = "";
                        data.perfiles.forEach(p => {
                            const opt = document.createElement("option");
                            opt.value = p.id;
                            opt.text = "⭐ " + p.nombre;
                            select.add(opt);
                        });
                        select.value = data.activo;
                        cambiarPerfilWeb(data.activo);
                    });
                }

                function mostrarCrearPerfil() {
                    creandoNuevo = true;
                    document.getElementById("seccionNuevoNombre").style.display = "block";
                    document.getElementById("nombrePerfilNuevo").value = "";
                    document.getElementById("nombrePerfilNuevo").focus();
                    document.getElementById("mensajeConfig").innerText = "Ingresa el nombre y presiona Guardar";
                }

                function cambiarPerfilWeb(idPerfil) {
                    creandoNuevo = false;
                    document.getElementById("seccionNuevoNombre").style.display = "none";
                    fetch('/getPerfil?id=' + idPerfil)
                    .then(response => response.json())
                    .then(data => {
                        document.getElementById("pwmMax").value = data.pwmMax;
                        document.getElementById("kIzq").value = data.kIzq;
                        document.getElementById("kDer").value = data.kDer;
                        document.getElementById("anguloGiro").value = data.anguloGiro;

                        document.getElementById("invX").checked = data.invX === 1;
                        document.getElementById("invY").checked = data.invY === 1;
                        document.getElementById("zonaMuerta").value = data.zm;

                        document.getElementById("btnF").value = data.btnF;
                        document.getElementById("btnB").value = data.btnB;
                        document.getElementById("btnL").value = data.btnL;
                        document.getElementById("btnR").value = data.btnR;

                        document.getElementById("kGiro").value = data.kGiro;
                        document.getElementById("stickGiro").value = data.stickGiro;

                        document.getElementById("mensajeConfig").innerText = "✓ Perfil activo";
                    });
                }

                function guardarPerfilWeb() {
                    const select = document.getElementById("selectPerfil");
                    let idPerfil = select.value;
                    let nombrePerfil = "";

                    if (creandoNuevo) {
                        nombrePerfil = document.getElementById("nombrePerfilNuevo").value.trim();
                        if (!nombrePerfil) {
                            document.getElementById("mensajeConfig").innerText = "❌ Ingresa un nombre válido";
                            return;
                        }
                        idPerfil = select.options.length;
                    } else {
                        nombrePerfil = select.options[select.selectedIndex].text.replace("⭐ ", "");
                    }

                    const pwmMax = document.getElementById("pwmMax").value;
                    const kIzq = document.getElementById("kIzq").value;
                    const kDer = document.getElementById("kDer").value;
                    const anguloGiro = document.getElementById("anguloGiro").value;

                    const invX = document.getElementById("invX").checked ? 1 : 0;
                    const invY = document.getElementById("invY").checked ? 1 : 0;
                    const zm = document.getElementById("zonaMuerta").value;

                    const btnF = document.getElementById("btnF").value;
                    const btnB = document.getElementById("btnB").value;
                    const btnL = document.getElementById("btnL").value;
                    const btnR = document.getElementById("btnR").value;

                    const kGiro = document.getElementById("kGiro").value;
                    const stickGiro = document.getElementById("stickGiro").value;

                    const url = `/guardarPerfil?id=${idPerfil}&nombre=${encodeURIComponent(nombrePerfil)}&pwmMax=${pwmMax}&kIzq=${kIzq}&kDer=${kDer}&anguloGiro=${anguloGiro}&invX=${invX}&invY=${invY}&zm=${zm}&btnF=${btnF}&btnB=${btnB}&btnL=${btnL}&btnR=${btnR}&kGiro=${kGiro}&stickGiro=${stickGiro}`;
                    
                    fetch(url)
                    .then(response => response.text())
                    .then(data => {
                        document.getElementById("mensajeConfig").innerText = "✓ Guardado con éxito";
                        creandoNuevo = false;
                        document.getElementById("seccionNuevoNombre").style.display = "none";
                        cargarListaPerfiles();
                    });
                }

                function eliminarPerfilWeb() {
                    const select = document.getElementById("selectPerfil");
                    if (select.options.length <= 1) {
                        document.getElementById("mensajeConfig").innerText = "❌ No puedes borrar el único perfil";
                        return;
                    }

                    const idPerfil = select.value;
                    if (confirm("¿Estás seguro de eliminar este perfil?")) {
                        fetch('/eliminarPerfil?id=' + idPerfil)
                        .then(response => response.text())
                        .then(data => {
                            document.getElementById("mensajeConfig").innerText = "✓ Perfil eliminado";
                            cargarListaPerfiles();
                        });
                    }
                }

                function girar(direccion) {
                    const angulo = document.getElementById("anguloGiro").value;
                    fetch(`/girar?direccion=${direccion}&angulo=${angulo}`)
                    .then(response => response.text())
                    .then(data => { document.getElementById("mensajeGiro").innerText = "✓ " + data; });
                }

                function actualizarBateria() {
                    fetch('/bateria')
                    .then(response => response.json())
                    .then(data => {
                        document.getElementById("voltajeBateria").innerText = data.voltaje.toFixed(2) + " V";
                        dibujarGrafico(data.historial);
                    });
                }

                function dibujarGrafico(datos) {
                    const canvas = document.getElementById("graficoBateria");
                    const ctx = canvas.getContext("2d");
                    const ancho = canvas.width;
                    const alto = canvas.height;

                    ctx.clearRect(0, 0, ancho, alto);
                    if (!datos || datos.length < 2) return;

                    let minV = datos[0].v, maxV = datos[0].v;
                    datos.forEach(p => {
                        if (p.v < minV) minV = p.v;
                        if (p.v > maxV) maxV = p.v;
                    });

                    minV -= 0.1; maxV += 0.1;
                    if (maxV - minV < 0.5) {
                        const centro = (maxV + minV) / 2;
                        minV = centro - 0.25;
                        maxV = centro + 0.25;
                    }

                    ctx.beginPath();
                    ctx.strokeStyle = "#30363d";
                    ctx.moveTo(35, 10); ctx.lineTo(35, alto - 25); ctx.lineTo(ancho - 10, alto - 25);
                    ctx.stroke();

                    ctx.fillStyle = "#8b949e";
                    ctx.font = "11px Segoe UI";
                    ctx.fillText(maxV.toFixed(1) + "V", 2, 15);
                    ctx.fillText(minV.toFixed(1) + "V", 2, alto - 28);

                    ctx.beginPath();
                    datos.forEach((p, i) => {
                        let x = 35 + (i / (datos.length - 1)) * (ancho - 45);
                        let y = (alto - 25) - ((p.v - minV) / (maxV - minV)) * (alto - 35);
                        if (i === 0) ctx.moveTo(x, y);
                        else ctx.lineTo(x, y);
                    });

                    ctx.strokeStyle = "#3fb950";
                    ctx.lineWidth = 2;
                    ctx.stroke();
                }
            </script>
        </body>
        </html>
        )rawliteral";

        server.send(200, "text/html", html);
    });

    // === RUTAS HTTP APIS ===
    server.on("/authorize", []() {
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            if (gamepads[id].connected) {
                memcpy(allowedController, gamepads[id].ctl->getProperties().btaddr, 6);
                Preferences prefs;
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

server.on("/getJoysticks", HTTP_GET, []() {
        Preferences prefs;

    String json = "[";
    
    prefs.begin("joysticks", true);

    bool primero = true;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        ControllerPtr ctl = myControllers[i];
        if (ctl && ctl->isConnected()) {
            if (!primero) json += ",";
            primero = false;

            String mac = obtenerMACJoystick(ctl);
            
            // Generar exactamente la misma clave recortada
            String clave = mac;
            clave.replace(":", "");
            if (clave.length() > 8) {
                clave = clave.substring(clave.length() - 8);
            }

            // Si no encuentra nada guardado, asigna la MAC original como fallback
            String nombreGuardado = prefs.getString(clave.c_str(), mac);

            json += "{";
            json += "\"slot\":" + String(i) + ",";
            json += "\"mac\":\"" + mac + "\",";
            json += "\"nombre\":\"" + nombreGuardado + "\"";
            json += "}";
        }
    }
    prefs.end();
    json += "]";

    server.send(200, "application/json", json);
});

// Endpoint 2: Recibe la MAC y el nuevo apodo desde la Web
server.on("/renombrarJoystick", HTTP_GET, []() {
        Preferences prefs;

    if (server.hasArg("mac") && server.hasArg("nombre")) {
        String mac = server.arg("mac");
        String nombre = server.arg("nombre");

        // Crear una clave de 8 caracteres (ej: "AABBCCDD") sin superar los 15 permitidos por NVS
        String clave = mac;
        clave.replace(":", "");
        if (clave.length() > 8) {
            clave = clave.substring(clave.length() - 8);
        }

        prefs.begin("joysticks", false);
        prefs.putString(clave.c_str(), nombre);
        prefs.end();

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Parametros faltantes");
    }
});

server.on("/clear", []() {
        Preferences prefs;
        prefs.begin("gamepad", false);
        prefs.clear();
        prefs.end();
        whitelistEnabled = false;
        authorizedGamepad = "Ninguno";
        server.send(200, "text/plain", "Whitelist borrada");
    });

    server.on("/disconnect", []() {
        if (server.hasArg("id")) {
            int id = server.arg("id").toInt();
            if (gamepads[id].connected) {
                gamepads[id].ctl->disconnect();
                server.send(200, "text/plain", "Desconectado");
                return;
            }
        }
        server.send(400, "text/plain", "Error");
    });

    server.on("/listarPerfiles", HTTP_GET, []() {
        Preferences prefs;
        prefs.begin("perfiles", true);
        int total = prefs.getInt("total", 1);
        int activo = prefs.getInt("activo", 0);

        String json = "{\"activo\":" + String(activo) + ",\"perfiles\":[";
        for (int i = 0; i < total; i++) {
            String key = "p_" + String(i) + "_nom";
            String defNom = "Perfil " + String(i + 1);
            String nom = prefs.getString(key.c_str(), defNom.c_str());

            json += "{\"id\":" + String(i) + ",\"nombre\":\"" + nom + "\"}";
            if (i < total - 1) json += ",";
        }
        json += "]}";
        prefs.end();

        server.send(200, "application/json", json);
    });

    server.on("/getPerfil", HTTP_GET, []() {
        int id = server.hasArg("id") ? server.arg("id").toInt() : perfilActualID;
        cargarPerfil(id);

        Preferences prefs;
        prefs.begin("perfiles", false);
        prefs.putInt("activo", id);
        prefs.end();

        String json = "{";
        json += "\"id\":" + String(perfilActualID);
        json += ",\"pwmMax\":" + String(perfilActivo.pwmMax);
        json += ",\"kIzq\":" + String(perfilActivo.kIzq);
        json += ",\"kDer\":" + String(perfilActivo.kDer);
        json += ",\"anguloGiro\":" + String(perfilActivo.anguloGiro);
        json += ",\"invX\":" + String(perfilActivo.invertirEjeX ? 1 : 0);
        json += ",\"invY\":" + String(perfilActivo.invertirEjeY ? 1 : 0);
        json += ",\"zm\":" + String(perfilActivo.zonaMuerta);
        json += ",\"btnF\":" + String(perfilActivo.btnAdelante);
        json += ",\"btnB\":" + String(perfilActivo.btnAtras);
        json += ",\"btnL\":" + String(perfilActivo.btnIzq);
        json += ",\"btnR\":" + String(perfilActivo.btnDer);
        
        // 📍 NUEVOS CAMPOS EN JSON
        json += ",\"kGiro\":" + String(perfilActivo.kGiro, 2);
        json += ",\"stickGiro\":" + String(perfilActivo.stickGiro);
        json += "}";

        server.send(200, "application/json", json);
    });

    server.on("/guardarPerfil", HTTP_GET, []() {
        if (!server.hasArg("id")) {
            server.send(400, "text/plain", "Falta ID");
            return;
        }

        PerfilConfig p;
        int id = server.arg("id").toInt();

        if (server.hasArg("nombre")) {
            String nom = server.arg("nombre");
            snprintf(p.nombre, sizeof(p.nombre), "%s", nom.c_str());
        } else {
            snprintf(p.nombre, sizeof(p.nombre), "Perfil %d", id + 1);
        }

        p.pwmMax = server.arg("pwmMax").toInt();
        p.kIzq = server.arg("kIzq").toInt();
        p.kDer = server.arg("kDer").toInt();
        p.anguloGiro = server.arg("anguloGiro").toInt();

        p.invertirEjeX = server.arg("invX") == "1";
        p.invertirEjeY = server.arg("invY") == "1";
        p.zonaMuerta = server.arg("zm").toInt();

        p.btnAdelante = server.arg("btnF").toInt();
        p.btnAtras = server.arg("btnB").toInt();
        p.btnIzq = server.arg("btnL").toInt();
        p.btnDer = server.arg("btnR").toInt();

        p.kGiro = server.arg("kGiro").toFloat();
        p.stickGiro = server.arg("stickGiro").toInt();

        guardarPerfil(id, p);
        server.send(200, "text/plain", "Perfil guardado");
    });

    server.on("/eliminarPerfil", HTTP_GET, []() {
        if (!server.hasArg("id")) {
            server.send(400, "text/plain", "Falta ID");
            return;
        }

        int idABorrar = server.arg("id").toInt();
        
        Preferences prefs;
        prefs.begin("perfiles", false);
        int total = prefs.getInt("total", 1);

        if (total <= 1) {
            prefs.end();
            server.send(400, "text/plain", "No se puede eliminar el único perfil");
            return;
        }

        // Reorganizar los índices desplazando los posteriores hacia la izquierda
        for (int i = idABorrar; i < total - 1; i++) {
            String keyCurrent = "p_" + String(i) + "_";
            String keyNext = "p_" + String(i + 1) + "_";

            prefs.putString((keyCurrent + "nom").c_str(), prefs.getString((keyNext + "nom").c_str(), ""));
            prefs.putInt((keyCurrent + "pwm").c_str(), prefs.getInt((keyNext + "pwm").c_str(), 220));
            prefs.putInt((keyCurrent + "ki").c_str(), prefs.getInt((keyNext + "ki").c_str(), 0));
            prefs.putInt((keyCurrent + "kd").c_str(), prefs.getInt((keyNext + "kd").c_str(), 0));
            prefs.putInt((keyCurrent + "ang").c_str(), prefs.getInt((keyNext + "ang").c_str(), 180));
            prefs.putBool((keyCurrent + "ix").c_str(), prefs.getBool((keyNext + "ix").c_str(), false));
            prefs.putBool((keyCurrent + "iy").c_str(), prefs.getBool((keyNext + "iy").c_str(), false));
            prefs.putInt((keyCurrent + "zm").c_str(), prefs.getInt((keyNext + "zm").c_str(), 10));
            prefs.putInt((keyCurrent + "ba").c_str(), prefs.getInt((keyNext + "ba").c_str(), 1));
            prefs.putInt((keyCurrent + "bb").c_str(), prefs.getInt((keyNext + "bb").c_str(), 2));
            prefs.putInt((keyCurrent + "bi").c_str(), prefs.getInt((keyNext + "bi").c_str(), 3));
            prefs.putInt((keyCurrent + "bd").c_str(), prefs.getInt((keyNext + "bd").c_str(), 4));
        }

        // Eliminar el último que quedó duplicado tras la reestructuración
        String lastKey = "p_" + String(total - 1) + "_";
        prefs.remove((lastKey + "nom").c_str());
        prefs.remove((lastKey + "pwm").c_str());
        
        prefs.putInt("total", total - 1);
        prefs.putInt("activo", 0); // Regresar al perfil 0 por seguridad
        prefs.end();

        cargarPerfil(0);
        server.send(200, "text/plain", "Perfil eliminado correctamente");
    });

    server.on("/bateria", HTTP_GET, []() {
        String json = "{";
        json += "\"voltaje\":" + String(voltajeBateria, 2);
        json += ",\"historial\":" + obtenerHistorialJSON();
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/girar", HTTP_GET, []() {
        if (!server.hasArg("direccion") || !server.hasArg("angulo")) {
            server.send(400, "text/plain", "Error");
            return;
        }
        String direccion = server.arg("direccion");
        int grados = server.arg("angulo").toInt();

        if (direccion == "izquierda") giroIzquierda(grados);
        else if (direccion == "derecha") giroDerecha(grados);

        server.send(200, "text/plain", "Giro ejecutado");
    });

    server.begin();
        
        // Lanza la tarea del Servidor Web en el NÚCLEO 0
        xTaskCreatePinnedToCore(
            TaskWebServer,    // Función de la tarea
            "TaskWebServer",  // Nombre
            10000,            // Tamaño de stack
            NULL,             // Parámetros
            1,                // Prioridad
            NULL,             // Handle
            0                 // Core 0 (El control de motores correrá libre en Core 1)
        );

        BP32.setup(&onConnectedController, &onDisconnectedController);
    }

// ========================================================================
// ============================== LOOP ==============================
// ========================================================================

void loop() {
    if (millis() - ultimaLecturaBateria >= INTERVALO_BATERIA) {
        ultimaLecturaBateria = millis();
        leerBateria();
    }

    bool dataUpdated = BP32.update();
    if (dataUpdated) processControllers();

    for (auto ctl : myControllers) {
        if (!ctl || !ctl->isConnected() || !ctl->hasData()) continue;
        if (!isControllerAllowed(ctl)) continue;            

        bool botonStart = ctl->miscStart();
        if (botonStart && !botonStartAnterior) iniciarAccessPoint(false);
        botonStartAnterior = botonStart;

        bool botonSelect = ctl->miscSelect();
        if (botonSelect && !botonSelectAnterior) apagarAccessPoint();
        botonSelectAnterior = botonSelect;

        static bool prevY = false, prevA = false;
        bool y = ctl->dpad() & DPAD_UP;
        bool a = ctl->dpad() & DPAD_DOWN;

        if (y && !prevY) controlVelocidad(true, false);
        if (a && !prevA) controlVelocidad(false, true);

        prevY = y; prevA = a;

        procesarBotonesDinamicos(ctl);

        bool botonMovimiento = estaBotonPresionado(ctl, perfilActivo.btnAdelante) ||
                               estaBotonPresionado(ctl, perfilActivo.btnAtras) ||
                               estaBotonPresionado(ctl, perfilActivo.btnIzq) ||
                               estaBotonPresionado(ctl, perfilActivo.btnDer);

        static unsigned long ultimoIzquierda = 0;
        if (ctl->brake() && millis() - ultimoIzquierda > tiempoRebote) {
            giroIzquierda(perfilActivo.anguloGiro);
            ultimoIzquierda = millis();
        }
        
        static unsigned long ultimoDerecha = 0;
        if (ctl->throttle() && millis() - ultimoDerecha > tiempoRebote) {
            giroDerecha(perfilActivo.anguloGiro);
            ultimoDerecha = millis();
        }

        bool turbo = ctl->r1();

        if (!botonMovimiento) { 
            movimiento(ctl, turbo); 
        }
    }

    delay(1);
}