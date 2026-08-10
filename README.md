# Robots Futboleros de la FIE

---
## Instrucciones de Uso

### Conectar Mando Bluetooth a Robot

#### 1) Conseguir la MAC Adress de la ESP usada:
  1. Activar el Bluetooth de la ESP.
  2. Descargar la siguiente ![aplicación](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal) en el celular.
  3. Vincularse desde el celular a la ESP con Bluetooth.
  4. Abrir la aplicación y dirigirse a la sección de dispositivos.
  5. En dicha sección debería aparecer la ESP con su MAC Adress.

#### 2) Grabar la MAC Adress correcta al mando:
  1. Descargar e instalar ![SixaxisPairTool](https://www.filehorse.com/es/download-sixaxispairtool/).
  2. Abrir la aplicación y conectar el mando a la computadora mediante un cable USB.
  3. Si es la primera vez que se conecta el mando, se descargarán e instalarán los drivers adecuados.
  4. Una vez finalizado el proceso, en la aplicación meter la MAC Adress de la ESP y darle a actualizar.
  5. Corroborar.

### Mantenimiento de Baterias

- Para controlar el nivel de las baterías:
  - Medir con multimetro en los pines VCC y GND de los puentes H o en la bornera en caso de tener una.
  - La tensión minima es de 6,5V y la máxima de 8,4V

- Para mantener:
  - En caso de que la tensión sea mínima, retirar las baterias y cargar adecuadamente.
  - Al reponer las baterias, reforzar y homologar.

### Tips Generales

- En caso de que el robot sufra una contusión y se reinicie, presionar el boton de conexión de el mando para reestablecer el enlace entre ambos.
- Presionando el panel tactil de los mandos de PS4 se puede determinar su nivel de bateria según cuanto vibren.

---
---
# Índice Importante:
**Links:**
- [[#^43a2ae|Links útiles]]

**Tips e Info:**
- [[#^23ebe2|Placa V3 usada en la Bestia 2.0 y la Bella]]
- [[#^925762|Comentarios y Cambios por futbolero en el Código]]
- [[#^1c89af|Funciones de la Página WEB]]
- [[#^0a90d6|Manejo del ROBOT]]
- [[#^6af2b9|Baterías]]
- [[#^a27400|Conexión del mando]]
- [[#^53dff0|Requerimientos para cargar el código]]
***
***
## Bluepad32 ESP32

**Links Útiles:** ^43a2ae
- Mandos soportados: https://bluepad32.readthedocs.io/en/latest/supported_gamepads/
- Emparejar un solo mando: https://bluepad32.readthedocs.io/en/latest/FAQ/#how-to-pair-just-one-controller-to-one-particular-board

> [!Check] Ventajas sobre el PS4 Controller. 
>
|                                   |         Bluepad32          |  PS4 Controller   |
| :-------------------------------: | :------------------------: | :---------------: |
|        **Compatibilidad**         |          Genérico          |   Sólo para PS4   |
| **Complejidad de emparejamiento** | Fácil y de muchos recursos |     Complejo      |
|         **Mantenimiento**         | Actualizaciones constantes | Librería obsoleta |

> Debemos agregarle verificación / autenticación. Se explica en el último link, pero se debe adicionar la mejora de la selección del mismo posiblemente con web server.

***
***
## TIPS e INFO:

**Placa V3 usada en la Bestia 2.0 y la Bella:** ^23ebe2
> La placa posee diversos arreglos, entre ellos posee la conexión por puente del ENABLE con 3v3; y otro puente que va del regulador de 5V a Vin, además, el regulador se cambió ya que el conseguido fue de 3v3 por error y se consiguió uno de inserción a ultimo momento de pinaje distinto al SMD. 

> [!Important] La ESP es de 36 pines por confusión, lo que cambia el pinaje del código. 

> [!Check] Mejoras y comodidades observadas:
> - Los conectores facilitan la conexión y desconexión rápida y fácil de los motores, la batería y el LED. 
> - Los sockets son cruciales para cambio de componentes. 
> - El capacitor de carga permitió menos desconexión por choques pese a tener que homologar el portapilas.

**Comentarios y Cambios por futbolero en el Código:** ^925762
1. **Pinaje de las Placas y motores:**

| Pines        | Bestia 1.0 | Bestia 2.0 | Bella |
| ------------ | ---------- | ---------- | ----- |
| Atras_Izq    | 25         | 25         | 26    |
| Adelante_Izq | 26         | 26         | 25    |
| Atras_Der    | 18         | 14         | 12    |
| Adelante_Der | 21         | 12         | 14    |
| PIN_LED 2    | 13         | 2          | 2     |
2. **Funciones:** para la Bestia 1.0, los pwm del movimiento se invierte en suma y resta; como tambien la funcion de manejo de botones en izquierda y derecha. 
3. **WIFI e IP:** cambiar en el código los nombres y direcciones por robot para evitar confusión.

| Robot      | IP           | SSID            |
| ---------- | ------------ | --------------- |
| Bestia 1.0 | 192.168.23.1 | Robot-Bestia1.0 |
| Bestia 2.0 | 192.168.24.1 | Robot-Bestia2.0 |
| Bella      | 192.168.25.1 | Robot-Bella     |
> [!Important] La contraseña para todos es "Fulbo123".

**Funciones de la Página WEB:** ^1c89af
> Permite visualizar y autorizar la conexión de mandos de forma remota, y uno a la vez; como así el cambio de variables como el ángulo de giro, PWM y constantes de ajuste de los motores durante los dos minutos que se crea el access point tra prender la ESP32.

**Manejo del ROBOT:** ^0a90d6

| Botón                  | Función                              |
| ---------------------- | ------------------------------------ |
| Palanca Derecha        | Dirección                            |
| Palanca Izquierda      | -                                    |
| Flechas arriba y abajo | Aumento y disminución de velocidad   |
| Botones A Y B X        | Manejo de dirección precisa          |
| R1                     | Turbo                                |
| R2                     | Giro Derecha                         |
| L2                     | Giro Izquierda                       |
| Menú (+)               | Prende el AP para calibrar variables |
| Select (-)             | Apaga el AP para calibrar variables  |

**Baterías:** ^6af2b9
> Se observó que las mismas ofrecían duración de 2 partidas contra robots de poca potencia; sin embargo para la batalla contra robots del estilo de PhobosyDeimos baterías con 2 o 3 partidas ya eran insuficientes para empujar la potencia de los robots. 

> [!Check] Mejora con las Baterías:
> - Conseguir baterías LiPo e investigar la carga, duración y conexión en una nueva versión de placa.
> - Chequear el portapilas para:
> 	- Evitar homologación constante por desconexión de mando.
> 	- Fácil acceso para cargar baterías o recambio de las pilas. 

**Conexión del mando:** ^a27400
1. Prender el robot con la ESP32. 
2. Abrir desde el dispositivo deseado la conexión WIFI y seleccionar aquel cuyo SSID sea el del robot correspondiente. 
3. Establecida la conexión del Access Point (que dura 2minutos), ingresar a la IP del robot.
4. Prender el mando Bluethoot genérico.
5. Una vez que la ESP lo detecta, la pagina web muestra la mac del mando conectado, pero no permite el manejo del robot hasta autorizarlo.
6. El mando autorizado queda guardado en una lista, y en caso de querer olvidarlo para conectar otro se debe acceder nuevamente al access point y borrar la whislist. 

> [!Info] Para mandos específicos chequear el link de compatibilidad y conexión. 

**Requerimientos para cargar el código:** ^53dff0
1. Instalar Arduino IDE V2. 
2. Abrir uno de los archivos del código dentro de la misma carpeta ya que de esa manera se compilan juntos y se abren en distintas pestañas.
3. Descargar la placa en Boards de *esp32_bluepad32 by Ricardo Quesada* - V4.1.0.
   https://gitlab.com/ricardoquesada/bluepad32
4. Descargar la librería:
	- BLE-Gamepad-Client by Tomasz Bekas - V0.12.1. 
	- ESP32-BLE-GAMEPAD by IemmingDev - V0.7.3.
5. Conectar la esp vía USB a la PC/Notebook. 
6. Seleccionar la placa DOIT ESP32 DEVKIT V1 - esp32_bluepad32 en el puerto adecuado y detectado en la conexión.
7. Cargar el código (con la flechita).


