/*
  FrutaSpectral: orquestador del clasificador de fruta.

  Sirve para:
  - Inicializar el ESP32, el bus I2C y el sensor espectral AS7341.
  - Ejecutar el ciclo principal de lectura de datos seleccionados.
  - Mantener actualizado el control del LED integrado del modulo AS7341.

*/

#include <Wire.h>
#include <Adafruit_AS7341.h>

// Direccion I2C por defecto del sensor AS7341.
const uint8_t AS7341_I2C_ADDRESS = 0x39;

// Pines I2C usados en el ESP32.
const int I2C_SDA_PIN = 5;
const int I2C_SCL_PIN = 18
;

// Objeto de la libreria Adafruit que controla el sensor.
Adafruit_AS7341 as7341;

// Prototipos de las funciones definidas en otros archivos .ino del sketch.
// Se declaran antes de setup() y loop() para avisarle al compilador que
// estas funciones existen, aunque su codigo este escrito en otro archivo .ino.
// Asi se pueden llamar dentro de setup() y loop() sin errores de compilacion.
// void imprimirDatosSensorEnSerial();  // Lectura completa del AS7341.
struct MuestraDatosSensor {
  uint16_t f4;
  uint16_t f5;
  uint16_t f6;
  uint16_t f7;
  uint16_t f8;
  uint16_t clear;
};

struct DatosProcesadosVisibles {
  float f4Normalizado;
  float f5Normalizado;
  float f6Normalizado;
  float f7Normalizado;
  float f8Normalizado;
  uint32_t sumaVisible;
  float promedioVisibleNm;
  bool saturado;
};

bool obtenerDatosSeleccionados(MuestraDatosSensor *muestra);
bool procesarDatosVisibles(const MuestraDatosSensor *muestra, DatosProcesadosVisibles *datos);
void imprimirDatosProcesadosEnSerial(const DatosProcesadosVisibles *datos);
void controlarLedSensor();
void iniciarPantalla();
void actualizarPantallaDatosVisibles(const MuestraDatosSensor *muestra,
                                     const DatosProcesadosVisibles *datos);

void iniciarSensorAS7341() {
  Serial.println();
  Serial.println("Iniciando sensor AS7341...");
  Serial.print("Direccion I2C: 0x");
  Serial.println(AS7341_I2C_ADDRESS, HEX);
  Serial.print("SDA: GPIO ");
  Serial.println(I2C_SDA_PIN);
  Serial.print("SCL: GPIO ");
  Serial.println(I2C_SCL_PIN);

  // Si el sensor no responde, el programa se detiene para evitar lecturas falsas.
  if (!as7341.begin(AS7341_I2C_ADDRESS, &Wire)) {
    Serial.println("No se encontro el AS7341. Revisa alimentacion, SDA/SCL y direccion I2C.");
    while (true) {
      delay(1000);
    }
  }

  // Configuracion de integracion y ganancia.
  // Mayor tiempo/ganancia aumenta la senal, pero hace mas lenta la lectura.
  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_256X);

  Serial.println("AS7341 listo.");
}

void setup() {
  // Inicia el monitor serial para mostrar las lecturas del sensor.
  Serial.begin(115200);
  delay(1000);

  // Inicializa el bus I2C compartido por la pantalla y el sensor.
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Primero prueba la pantalla, para confirmar que la OLED responde.
  iniciarPantalla();

  // Verifica que el sensor AS7341 responda.
  iniciarSensorAS7341();

  // Configura el pulsador que controla el LED integrado del modulo AS7341.
  controlarLedSensor();
}

void loop() {
  // Actualiza el interruptor del LED del modulo con el pulsador en D12.
  controlarLedSensor();

  // Toma los datos visibles, los procesa y los imprime por Serial.
  MuestraDatosSensor muestra;
  DatosProcesadosVisibles datos;
  if (obtenerDatosSeleccionados(&muestra) && procesarDatosVisibles(&muestra, &datos)) {
    imprimirDatosProcesadosEnSerial(&datos);
    actualizarPantallaDatosVisibles(&muestra, &datos);
  }

  // Lectura completa comentada. Descomentar para ver todos los canales.
  // imprimirDatosSensorEnSerial();

  // Vuelve a actualizar el LED despues de la lectura, porque leer el sensor toma tiempo.
  controlarLedSensor();

  // Pausa entre lecturas para que la salida serial sea fÃ¡cil de revisar.
  unsigned long inicioPausa = millis();
  while (millis() - inicioPausa < 1000) {
    controlarLedSensor();
    delay(10);
  }
}
