/*
  Control del LED integrado del modulo AS7341.

  Sirve para:
  - Leer un pulsador conectado entre D12/GPIO12 y GND.
  - Encender o apagar el LED integrado del modulo como interruptor.
  - Aplicar antirrebote para evitar multiples cambios por una sola pulsacion.

*/

#include <Adafruit_AS7341.h>

// Objeto global creado en FrutaSpectral.ino; se usa aqui para controlar el LED.
extern Adafruit_AS7341 as7341;

// Pin del pulsador. Se conecta entre D12 y GND.
const int PULSADOR_LED_SENSOR_PIN = 12;

// Corriente del LED integrado del modulo AS7341, en mA.
const uint16_t LED_SENSOR_CURRENT_MA = 20;

// Tiempo minimo para ignorar rebotes mecanicos del pulsador.
const unsigned long ANTIRREBOTE_MS = 50;

bool ledSensorEncendido = false;
bool lecturaAnteriorPulsador = HIGH;
unsigned long ultimoCambioPulsador = 0;
bool controlLedSensorIniciado = false;

void iniciarControlLedSensor() {
  // INPUT_PULLUP deja el pin en HIGH cuando el pulsador esta suelto.
  // Al presionar, el pulsador conecta D12 a GND y el pin pasa a LOW.
  pinMode(PULSADOR_LED_SENSOR_PIN, INPUT_PULLUP);

  // Configura una corriente moderada y deja el LED apagado al iniciar.
  as7341.setLEDCurrent(LED_SENSOR_CURRENT_MA);
  as7341.enableLED(false);
  ledSensorEncendido = false;
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
    ultimoCambioPulsador = ahora;
  }

  if (lecturaActualPulsador != lecturaAnteriorPulsador) {
    ultimoCambioPulsador = ahora;
    lecturaAnteriorPulsador = lecturaActualPulsador;
  }
}
