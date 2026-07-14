/*
  Control del LED integrado del modulo AS7341.

  Sirve para:
  - Leer un pulsador conectado entre D13/GPIO13 y GND.
  - Encender o apagar el LED integrado del modulo como interruptor.
  - Aplicar antirrebote para evitar multiples cambios por una sola pulsacion.

*/
#include <Adafruit_AS7341.h>

// Objeto global creado en FrutaSpectral.ino; se usa aqui para controlar el LED.
extern Adafruit_AS7341 as7341;

void reiniciarUmbralServo();

// Pin del pulsador. Se conecta entre D13 y GND.
const int PULSADOR_LED_SENSOR_PIN = 13;

// Corriente del LED integrado del modulo AS7341, en mA.
const uint16_t LED_SENSOR_CURRENT_MA = 20;

// LED integrado usual de la ESP32 DevKit.
const int LED_DEVKIT_PIN = 2;
const unsigned long LED_DEVKIT_PARPADEO_MS = 120;

// Tiempo minimo para ignorar rebotes mecanicos del pulsador.
const unsigned long ANTIRREBOTE_MS = 50;

bool ledSensorEncendido = false;
bool lecturaAnteriorPulsador = HIGH;
unsigned long ultimoCambioPulsador = 0;
bool controlLedSensorIniciado = false;

void iniciarLedDevkit() {
  pinMode(LED_DEVKIT_PIN, OUTPUT);
  digitalWrite(LED_DEVKIT_PIN, LOW);
}

void parpadearLedDevkitCalibracion() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_DEVKIT_PIN, HIGH);
    delay(LED_DEVKIT_PARPADEO_MS);
    digitalWrite(LED_DEVKIT_PIN, LOW);
    delay(LED_DEVKIT_PARPADEO_MS);
  }
}

void iniciarControlLedSensor() {
  // INPUT_PULLUP deja el pin en HIGH cuando el pulsador esta suelto.
  // Al presionar, el pulsador conecta D13 a GND y el pin pasa a LOW.
  pinMode(PULSADOR_LED_SENSOR_PIN, INPUT_PULLUP);

  // Configura una corriente moderada y deja el LED encendido al iniciar.
  as7341.setLEDCurrent(LED_SENSOR_CURRENT_MA);
  as7341.enableLED(true);
  ledSensorEncendido = true;
  lecturaAnteriorPulsador = digitalRead(PULSADOR_LED_SENSOR_PIN);
  ultimoCambioPulsador = millis();
  controlLedSensorIniciado = true;
}

void controlarLedSensor() {
  if (!controlLedSensorIniciado) {
    iniciarControlLedSensor();
  }

  bool lecturaActualPulsador = digitalRead(PULSADOR_LED_SENSOR_PIN);
  unsigned long ahora = millis();

  // Solo cuenta una pulsacion nueva cuando el pin pasa de HIGH a LOW.
  if (lecturaAnteriorPulsador == HIGH &&
      lecturaActualPulsador == LOW &&
      ahora - ultimoCambioPulsador >= ANTIRREBOTE_MS) {
    ledSensorEncendido = !ledSensorEncendido;
    as7341.enableLED(ledSensorEncendido);
    reiniciarUmbralServo();
    ultimoCambioPulsador = ahora;
  }

  if (lecturaActualPulsador != lecturaAnteriorPulsador) {
    ultimoCambioPulsador = ahora;
    lecturaAnteriorPulsador = lecturaActualPulsador;
  }
}
