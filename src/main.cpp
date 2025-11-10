#include <Arduino.h>
#include <PS4Controller.h>
#include <ManejoConBotones.hpp>
#include <Estrategias.hpp>
/*
Comentarios: (1) ajustar las variables "duracion", "intensidad", "niveles", "tiempoRebote", "tiempoGiro180", "tiempoCarga", "tiempoGolpe",
(2) pensar si cambiar los delay() de las 4 estrategias por millis()
*/

// Librerias


// Descomentar deb en platformio.ini para mostrar los serial
#ifdef DEB
  #define deb(x) x
#else 
  #define deb(x)
#endif

// Pines físicos conectados a los DRV8833
#define Atras_Izq 18    
#define Adelante_Izq 21    
#define Atras_Der 25
#define Adelante_Der 26
#define PIN_LED 13  // LED

// Configuración PWM
#define PWM_FREQ 20000  // Frecuencia en Hz
#define PWM_RES 8       // Resolución en bits (0 - 2^(PWM_RES)-1)

// Para la función de la zona muerta
#define ZONA_DRIFT 10 // Si el drift del control varía, se cambia la constante numérica

const int PWM_MIN = 180;
const int PWM_MAX = 232;

// Variables para control de velocidad
float factorVelocidad = 2.0;                  // Factor multiplicador de velocidad (inicia en velocidad máxima)
int nivelActual = 2;                          // Nivel actual (2 = velocidad máxima)

const int cantNiveles = 3;
const float niveles[cantNiveles] = { 1.5, 1.65, 2.0 };  // Factores de velocidad por nivel (bajo, medio, alto)

// Variables para tiempos
int tiempoGiro180 = 685;  // Tiempo que tarda en girar 180°
int tiempoCarga = 300;    // Duración del retroceso en ms
int tiempoGolpe = 300;    // Duración del golpe en ms

// Corrección de PWM al motor derecho (Adelante_Der y Atras_Der)
// Sale con Fritas tiene K = 10
// Chispitas tiene K = 0
uint8_t K = 0;
// TODO comprobar con módulo usado en seguidores

//-------------------------------------------------------------------------------------------------------------------------------------

// Función para ajustar el nivel de velocidad con flechas arriba y abajo (las estrategias no se ven afectadas por esta función)
unsigned long tiempoUltimoCambio = 0;    // Marca de tiempo del último ajuste
const unsigned long tiempoRebote = 200;  // Tiempo mínimo entre ajustes (ms)

void controlVelocidad(bool subir, bool bajar) {
  unsigned long ahora = millis();
  if (ahora - tiempoUltimoCambio >= tiempoRebote) {
    if (subir && nivelActual < cantNiveles) {
      nivelActual++;
      factorVelocidad = niveles[nivelActual];
      deb(Serial.printf("Nivel %d | Factor %.2f\n", nivelActual, factorVelocidad);)
      tiempoUltimoCambio = ahora;
    }
    if (bajar && nivelActual > 0) {
      nivelActual--;
      factorVelocidad = niveles[nivelActual];
      deb(Serial.printf("Nivel %d | Factor %.2f\n", nivelActual, factorVelocidad);)
      tiempoUltimoCambio = ahora;
    }
  }
}

// Función para detener los motores
void detenerMotores() {
  ledcWrite(Adelante_Der, 0);
  ledcWrite(Atras_Der, 0);
  ledcWrite(Adelante_Izq, 0);
  ledcWrite(Atras_Izq, 0);
}

// Función de desplazamiento vectorial y rotación simultánea
void movimiento() {
  int pwm = 0;
  int pwmDer = 0;
  int pwmIzq = 0;

  // Lectura del joystick derecho en X e Y
  int ejeX_Der = PS4.RStickX();  // -127 a 127
  int ejeY_Der = PS4.RStickY();  // -127 a 127
  int ejeX_Izq = PS4.LStickX();  // -127 a 127
  int ejeY_Izq = PS4.LStickY();  // -127 a 127

  // TODO: revisar que devuelven los ejes, si ya son flotantes luego no se pasa nuevamente

  // Zona muerta de las palancas
  // Sirve para que si las palancas se encuentran en una zona aproximadamente central, los motores se detengan.
  if (abs(ejeX_Der) < ZONA_DRIFT && abs(ejeY_Der) < ZONA_DRIFT && abs(ejeX_Izq) < ZONA_DRIFT && abs(ejeY_Izq) < ZONA_DRIFT ) {
    detenerMotores();
    return;
  }

  // Normalización
  // TODO: probar con normalización vectorial

  float normX = ejeX_Der / 127.0; // Se entiende que la normalización estará entre -1 y 1
  float normY = ejeY_Der / 127.0;

  // Ángulo y magnitud
  float magnitud = constrain(sqrt(normX * normX + normY * normY), 0.0, 1.0); // TODO cambiar a nombre más intuitivo y correcto  
  float angulo = atan2(normY, normX);  // La función determina el angulo en cualquier cuadrante, respecto del eje X; será de -π a π en radianes
  pwm = constrain(127 * magnitud * factorVelocidad, PWM_MIN, PWM_MAX);

  if (angulo < 0) angulo += 2.0 * PI; // convertimos al angulo entre [0;2pi)
  float sentido_frontales = sin(angulo); // Debe darnos entre -1 y 1, lo cual nos indicará para la función el sentido atras y adelante.
  float sentido_laterales = cos(angulo); // El coseno nos va a indicar en nuestro mapa de circunferencia unitaria, qué tan a la izq o der se moverá

  // Calculo un pwm para darle al motor si quiero girar a alguno de los lados
  if (sentido_laterales >= 0.0) { 
    pwmIzq = pwm * sentido_laterales * magnitud;
  } else{
    pwmDer = pwm * sentido_laterales * magnitud;
  }

  // Acá creo que vuelve al flotante a un entero, mediante int y el redondeo con round
  if (pwmIzq >= 0) {
    ledcWrite(Adelante_Izq, (int)round(constrain(abs(pwmIzq), 0, PWM_MAX)));
    ledcWrite(Atras_Izq, 0);
  } else {
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Atras_Izq, (int)round(constrain(abs(pwmIzq), 0, PWM_MAX))); 
  }

  if (pwmDer >= 0) {
    ledcWrite(Adelante_Der, (int)round(constrain(abs(pwmDer), 0, PWM_MAX)) + K);
    ledcWrite(Atras_Der, 0);
  } else {
    ledcWrite(Adelante_Der, 0);
    ledcWrite(Atras_Der, (int)round(constrain(abs(pwmDer), 0, PWM_MAX)) + K);
  }

  // Registro
  deb(Serial.printf("Ángulo:%.2f | PWM:%d | PWM_L:%d PWM_R:%d\n", angulo * 180 / PI, pwm, pwmIzq, pwmDer);)
}

// Adelante con 255
void maxima() {
  ledcWrite(Adelante_Izq, 255);
  ledcWrite(Adelante_Der, 255);
  ledcWrite(Atras_Izq, 0);
  ledcWrite(Atras_Der, 0);
}

// Función para representar el nivel de batería del mando PS4 mediante vibración proporcional
void bateriaControl() {
  int nivel = PS4.Battery();                    // Obtiene el nivel de batería (0 a 12)
  int duracion = map(nivel, 0, 12, 200, 1500);  // Escala duración de vibración según nivel
  int intensidad = 200;                         // Intensidad fija

  // Activa vibración con duración proporcional al nivel de batería
  PS4.setRumble(intensidad, intensidad);
  PS4.sendToController();
  delay(duracion);
  PS4.setRumble(0, 0);  // Detiene vibración
  PS4.sendToController();

  // Registro en consola para trazabilidad
  deb(Serial.printf("Nivel batería del mando: %d/12 | Vibración: %d ms\n", nivel, duracion);)
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Setup
void setup() {
  deb(Serial.begin(115200);)
  PS4.begin();  // Colocar la MAC del mando si es necesario

  // Pines PWM
  ledcAttach(Adelante_Der, PWM_FREQ, PWM_RES);  // PWM motor Derecho
  ledcAttach(Atras_Der, PWM_FREQ, PWM_RES);  
  ledcAttach(Adelante_Izq, PWM_FREQ, PWM_RES);  // PWM motor Izquierdo
  ledcAttach(Atras_Izq, PWM_FREQ, PWM_RES);
   
  pinMode(PIN_LED, OUTPUT);
}

// Loop
void loop() {
  if (PS4.isConnected()) {
    int r = 170, g = 0, b = 220;  // Variabes para color del control
    PS4.setLed(r, g, b);          // Color del control
    digitalWrite(PIN_LED, HIGH);

    bool subir = PS4.Up();
    bool bajar = PS4.Down();
  
    if (subir || bajar) {
      controlVelocidad(subir, bajar);
    }
    movimiento ();
    antiReboteTriang ();
    antiReboteCruz ();
    antiReboteCirc ();
    antiReboteCuad ();

    unsigned long ultimoIzquierda = 0;
    if (PS4.L2() && millis() - ultimoIzquierda > tiempoRebote) {
      giro180Izquierda();
      ultimoIzquierda = millis();
    }
    unsigned long ultimoDerecha = 0;
    if (PS4.R2() && millis() - ultimoDerecha > tiempoRebote) {
      giro180Derecha();
      ultimoDerecha = millis();
    }
    unsigned long ultimoMaxima = 0;
    if (PS4.R1() && millis() - ultimoMaxima > tiempoRebote) {
      maxima();
      ultimoMaxima = millis();
    }
    if (PS4.Touchpad()) {
      bateriaControl();  // Control con touchpad
    }
    delay(50);
  } else {
    detenerMotores();  // Seguridad ante desconexión
  }
}