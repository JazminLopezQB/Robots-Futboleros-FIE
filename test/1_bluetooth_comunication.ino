/*
//reconocimiento bluetooth

Código #: 1
Nombre del archivo: 1_bluetooth_comunication.cpp  
Fecha: 23/08/2025  
Propósito: Comunicación bidireccional entre ESP32 y celular vía Bluetooth, evitando eco duplicado.  
Hardware: ESP32  
Notas técnicas:  
- Mensajes desde USB se muestran y se envían al celular.  
- Mensajes desde el celular se muestran pero no se reenvían.  
- Ideal para pruebas de controladores Bluetooth como el mando PS4.
*/



#include <Arduino.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  // Inicia la consola por USB
  Serial.begin(115200);
  while(!Serial);

  // Inicia el Bluetooth Classic con nombre
  if(!SerialBT.begin("ESP32_BT")) {
    Serial.println("Error al iniciar Bluetooth");
    return;
  }
  Serial.println("Bluetooth iniciado como ESP32_BT");
}

void loop() {
  // Si llega dato por Bluetooth, lo envío a la consola USB y lo devuelvo
  if (SerialBT.available()) {
    char c = SerialBT.read();
    Serial.write(c);
    //SerialBT.write(c); // Eco del mensaje (aparece dos veces en el celular)
  }

  // Si escribo en la consola USB, lo envío por Bluetooth
  if (Serial.available()) {
    char c = Serial.read();
    Serial.write(c); // Aparece en el monitor serial (computadora) lo que estoy escribiendo
    SerialBT.write(c);
  }

  delay(20);
}
