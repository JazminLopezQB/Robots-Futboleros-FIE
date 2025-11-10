/*
Comentarios: (1) ajustar las variables "duracion", "intensidad", "niveles", "tiempoRebote", "tiempoGiro180", "tiempoCarga", "tiempoGolpe",
(2) pensar si cambiar los delay() de las 4 estrategias por millis()
*/

//#define DEB   // Descomentar deb para mostrar los serial
#ifdef DEB
  #define deb(x) x
#else 
  #define deb(x)
#endif

// Librerias
#include <PS4Controller.h>
#include <Arduino.h>

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

#define PWM_MIN 180
#define PWM_MAX PWM_MAX

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

  // HASTA ACÁ SE LEYÓ Y COMPRENDIÓ EL CÓDIGO, SE PENSÓ UNA NUEVA IDEA PARA EL CÁLCULO VECTORIAL DE LA MAGNITUD Y Y ÁNGULO 
  // CON EL FIN DE QUE SEA SENCILLO DE ENTENDER Y CAMBIAR EN EL CÓDIGO.
  // SE PENSÓ EL USO DE SENOS Y COSENOS ÚNICAMENTE Y ELIMINAR LA PARTE DE LÓBULOS O REESCRIBIRLA

  float normX = ejeX_Der / 127.0; // Se entiende que la normalización estará entre -1 y 1
  float normY = ejeY_Der / 127.0;

  // Ángulo y magnitud
  float magnitud = constrain(sqrt(normX * normX + normY * normY), 0.0, 1.0); // TODO cambiar a nombre más intuitivo y correcto
  float angulo = atan2(normY, normX);  // -π a π en radianes

  pwm = constrain(127 * magnitud * factorVelocidad, PWM_MIN, PWM_MAX);
  // --- entrada: angulo en radianes (-PI..PI), pwm ya calculado ---
  // Queremos picos en:  0° (izq), 90° (ambos), 180° (der), 270° (ambos negativos)
  // Usamos cos como base y combinaciones para tener los efectos deseados.

  // convertir theta a [0,2PI)
  float theta = angulo;
  if (theta < 0) theta += 2.0f * PI;

  // usar versiones float de cos y max
  float pico0 = fmaxf(0.0f, cosf(theta - 0.0f));
  float pico90 = fmaxf(0.0f, cosf(theta - PI / 2.0f));
  float pico180 = fmaxf(0.0f, cosf(theta - PI));
  float pico270 = fmaxf(0.0f, cosf(theta - 3.0f * PI / 2.0f));

  // Esto crea lóbulos positivos alrededor de cada punto.
  // Queremos que en 90° y 270° ambos motores tengan magnitud, en 0° sólo izq, en 180 sólo der.
  // Construimos magnitudes (positivo) y luego aplicamos signo según adelante/atrás.

  float magIzq = 0.0f;
  float magDer = 0.0f;

  // combinar lóbulos para obtener la forma deseada
  magIzq = pico90 + pico0 + pico270;
  magDer = pico90 + pico180 + pico270;

  // ajustar pesos relativos: en 90 queremos ambos fuertes (ya suman pico90); en 270 queremos ambos negativos:
  // para 270 detectamos retroceso por el seno (o por theta) y aplicamos signo negativo
  // normalizamos magnitudes para que la mayor valga 1
  float fmax = max(magIzq, magDer);
  if (fmax <= 0.0f) fmax = 1.0f;
  magIzq /= fmax;
  magDer /= fmax;

  // definir signo (adelante/atrás) según componente Y original (normY) o según ángulo
  // mejor usar sin(angulo) para dirección continua: sin(theta) >0 => hacia adelante (90), <0 => atrás (270)
  float sentido = sin(angulo);  // >0 adelante, <0 atrás

  // convertir mag a valores con signo entre -1 y 1; si querés permitir giro en sitio (motores opuestos),
  // entonces para ángulos cercanos a 0 o 180 el otro motor debe ser 0 (ya lo logramos con combinación).
  float outIzq = magIzq * (sentido >= 0 ? 1.0f : -1.0f);
  float outDer = magDer * (sentido >= 0 ? 1.0f : -1.0f);

  // escalar al PWM seguro
  float pwmIzqF = outIzq * pwm;
  float pwmDerF = outDer * pwm;

  // aplicar a los H-bridge (manteniendo tu lógica de sentido con normY si preferís)
  if (pwmIzqF >= 0) {
    ledcWrite(Adelante_Izq, (int)round(constrain(pwmIzqF, 0, PWM_MAX)));
    ledcWrite(Atras_Izq, 0);
  } else {
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Atras_Izq, (int)round(constrain(-pwmIzqF, 0, PWM_MAX)));
  }
  if (pwmDerF >= 0) {
    ledcWrite(Adelante_Der, (int)round(constrain(pwmDerF, 0, PWM_MAX)) + K);
    ledcWrite(Atras_Der, 0);
  } else {
    ledcWrite(Adelante_Der, 0);
    ledcWrite(Atras_Der, (int)round(constrain(-pwmDerF, 0, PWM_MAX)) + K);
  }

  // Registro
  deb(Serial.printf("Ángulo:%.2f | PWM:%d | PWM_L:%d PWM_R:%d\n", angulo * 180 / PI, pwm, pwmIzq, pwmDer);)
}

// Estrategia 1 - giro brusco de 180° a la izquierda
void giro180Izquierda() {
  // Motor izquierdo apagado
  ledcWrite(Adelante_Izq, 0);
  ledcWrite(Atras_Izq, 0);
  ledcWrite(Atras_Der, 0);

  // Motor derecho gira hacia adelante
  ledcWrite(Adelante_Der, PWM_MAX + K);  // PWM calibrado para giro
  delay(tiempoGiro180);   // Tiempo estimado para giro completo
  detenerMotores();       // Detener ambos motores
}

// Estrategia 2 - giro brusco de 180° a la derecha
void giro180Derecha() {
  // Motor derecho apagado
  ledcWrite(Adelante_Der, 0);
  ledcWrite(Atras_Der, 0);
  ledcWrite(Atras_Izq, 0);

  // Motor izquierdo gira hacia adelante
  ledcWrite(Adelante_Izq, PWM_MAX);  // PWM calibrado para giro
  delay(tiempoGiro180);   // Tiempo estimado para giro completo
  detenerMotores();       // Detener ambos motores
}

// Estrategia 3 - patada con la derecha
void patadaDerecha() {
  int fuerzaR2 = PS4.R2Value();                    // 0 a 255
  if (fuerzaR2 < 10) return;                       // Zona muerta
  int pwmGolpe = map(fuerzaR2, 0, 255, 180, PWM_MAX);  // PWM proporcional al gatillo

  // Paso 1: Carga (retroceso leve del motor derecho)
  ledcWrite(Adelante_Der, 0);    // Sentido atrás
  ledcWrite(Atras_Der, 180 + K);  // PWM moderado para carga
  ledcWrite(Atras_Izq, 0);    // Motor izquierdo apagado
  ledcWrite(Adelante_Izq, 0);
  delay(tiempoCarga);

  // Paso 2: Golpe (avance fuerte del motor derecho)
  ledcWrite(Adelante_Der, pwmGolpe + K);  // Sentido adelante
  ledcWrite(Atras_Der, 0);         // PWM proporcional
  delay(tiempoGolpe);
  detenerMotores();  // Detener ambos motores

  deb(Serial.printf("Patada derecha | PWM:%d | Carga:%dms | Golpe:%dms | Fuerza R2:%d\n", pwmGolpe, tiempoCarga, tiempoGolpe, fuerzaR2);)
  }

// Estrategia 4 - patada con la izquierda
void patadaIzquierda() {
  int fuerzaL2 = PS4.L2Value();                    // 0 a 255
  if (fuerzaL2 < 10) return;                       // Zona muerta
  int pwmGolpe = map(fuerzaL2, 0, 255, 180, PWM_MAX);  // PWM proporcional al gatillo

  // Paso 1: Carga (retroceso leve del motor izquierdo)
  ledcWrite(Adelante_Izq, 0);    // Sentido atrás
  ledcWrite(Atras_Izq, 180);  // PWM moderado para carga
  ledcWrite(Atras_Der, 0);    // Motor derecho apagado
  ledcWrite(Adelante_Der, 0);
  delay(tiempoCarga);

  // Paso 2: Golpe (avance fuerte del motor izquierdo)
  ledcWrite(Atras_Izq, 0);         // Sentido adelante
  ledcWrite(Adelante_Izq, pwmGolpe);  // PWM proporcional
  delay(tiempoGolpe);
  detenerMotores();  // Detener ambos motores

  deb(Serial.printf("Patada izquierda | PWM:%d | Carga:%dms | Golpe:%dms | Fuerza L2:%d\n", pwmGolpe, tiempoCarga, tiempoGolpe, fuerzaL2);)
}

// Adelante con triangulo
void adelante() {
  if (nivelActual == 0) {
    ledcWrite(Adelante_Izq, 180);
    ledcWrite(Adelante_Der, 180 + K);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else if (nivelActual == 1) {
    ledcWrite(Adelante_Izq, 206);
    ledcWrite(Adelante_Der, 206 + K);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else if (nivelActual == 2) {
    ledcWrite(Adelante_Izq, PWM_MAX);
    ledcWrite(Adelante_Der, PWM_MAX + K);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else {
    return;
  }
}

// Atrás con cruz
void atras() {
  if (nivelActual == 0) {
    ledcWrite(Atras_Izq, 180);
    ledcWrite(Atras_Der, 180 + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Adelante_Der, 0);
  } else if (nivelActual == 1) {
    ledcWrite(Atras_Izq, 206);
    ledcWrite(Atras_Der, 206 + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Adelante_Der, 0);
  } else if (nivelActual == 2) {
    ledcWrite(Atras_Izq, PWM_MAX);
    ledcWrite(Atras_Der, PWM_MAX + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Adelante_Der, 0);
  } else {
    return;
  }
}

// Izquierda con círculo
void izquierda() {
  if (nivelActual == 0) {
    ledcWrite(Adelante_Izq, 180);
    ledcWrite(Adelante_Der, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else if (nivelActual == 1) {
    ledcWrite(Adelante_Izq, 206);
    ledcWrite(Adelante_Der, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else if (nivelActual == 2) {
    ledcWrite(Adelante_Izq, PWM_MAX);
    ledcWrite(Adelante_Der, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else {
    return;
  }
}

// Derecha con cuadrado
void derecha() {
  if (nivelActual == 0) {
    ledcWrite(Adelante_Der, 180 + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else if (nivelActual == 1) {
    ledcWrite(Adelante_Der, 206 + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else if (nivelActual == 2) {
    ledcWrite(Adelante_Der, PWM_MAX + K);
    ledcWrite(Adelante_Izq, 0);
    ledcWrite(Atras_Izq, 0);
    ledcWrite(Atras_Der, 0);
  } else {
    return;
  }
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
  ledcAttach(Adelante_Der, PWM_FREQ, PWM_RES);  // PWM Izquierdo
  ledcAttach(Atras_Der, PWM_FREQ, PWM_RES);  // PWM Derecho
  ledcAttach(Adelante_Izq, PWM_FREQ, PWM_RES);  // PWM Izquierdo
  ledcAttach(Atras_Izq, PWM_FREQ, PWM_RES);  // PWM Derecho
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

    movimiento();

    unsigned long ultimoTriangulo = 0;
    const unsigned long tiempoRebote = 200;

    if (PS4.Triangle() && millis() - ultimoTriangulo > tiempoRebote) {
      adelante();
      ultimoTriangulo = millis();
    }
    unsigned long ultimoCruz = 0;
    if (PS4.Cross() && millis() - ultimoCruz > tiempoRebote) {
      atras();
      ultimoCruz = millis();
    }
    unsigned long ultimoCirculo = 0;
    if (PS4.Circle() && millis() - ultimoCirculo > tiempoRebote) {
      izquierda();
      ultimoCirculo = millis();
    }
    unsigned long ultimoCuadrado = 0;
    if (PS4.Square() && millis() - ultimoCuadrado > tiempoRebote) {
      derecha();
      ultimoCuadrado = millis();
    }
    unsigned long ultimoIzquierda = 0;
    if (PS4.Left() && millis() - ultimoIzquierda > tiempoRebote) {
      giro180Izquierda();
      ultimoIzquierda = millis();
    }
    unsigned long ultimoDerecha = 0;
    if (PS4.Right() && millis() - ultimoDerecha > tiempoRebote) {
      giro180Derecha();
      ultimoDerecha = millis();
    }
    unsigned long ultimoMaxima = 0;
    if (PS4.R1() && millis() - ultimoMaxima > tiempoRebote) {
      maxima();
      ultimoMaxima = millis();
    }
    if (PS4.R2()) {
      patadaDerecha();  // Control con R2
    }
    if (PS4.L2()) {
      patadaIzquierda();  // Control con L2
    }
    if (PS4.Touchpad()) {
      bateriaControl();  // Control con touchpad
    }
    delay(50);
  } else {
    detenerMotores();  // Seguridad ante desconexión
  }
}