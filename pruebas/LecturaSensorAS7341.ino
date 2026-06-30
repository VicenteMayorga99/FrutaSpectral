/*
  Lectura completa de diagnostico del AS7341.

  Este archivo esta dentro de la carpeta pruebas porque no forma parte del flujo
  principal del clasificador. Sirve como referencia para leer todos los canales
  del sensor con la funcion estandar readAllChannels().
*/

#include <Adafruit_AS7341.h>

const uint8_t AS7341_PRUEBA_I2C_ADDRESS = 0x39;

Adafruit_AS7341 as7341Prueba;

void imprimirCanalAS7341Prueba(const char *nombre, uint16_t valor) {
  Serial.print(nombre);
  Serial.print(": ");
  Serial.println(valor);
}

void imprimirFlickerAS7341Prueba(uint16_t frecuencia) {
  Serial.print("Flicker: ");

  if (frecuencia == 0) {
    Serial.println("No detectado");
  } else if (frecuencia == 1) {
    Serial.println("Detectado, frecuencia desconocida");
  } else {
    Serial.print(frecuencia);
    Serial.println(" Hz");
  }
}

bool iniciarSensorAS7341Prueba() {
  Serial.println();
  Serial.println("Iniciando sensor AS7341 para prueba completa...");

  if (!as7341Prueba.begin(AS7341_PRUEBA_I2C_ADDRESS, &Wire)) {
    Serial.println("No se encontro el AS7341 para la prueba completa.");
    return false;
  }

  as7341Prueba.setATIME(100);
  as7341Prueba.setASTEP(999);
  as7341Prueba.setGain(AS7341_GAIN_256X);

  Serial.println("AS7341 listo para prueba completa.");
  return true;
}

void imprimirDatosCompletosAS7341Prueba() {
  uint16_t canales[12];

  if (!as7341Prueba.readAllChannels(canales)) {
    Serial.println("Error al leer los canales del AS7341.");
    return;
  }

  Serial.println();
  Serial.println("Lectura completa AS7341");
  Serial.println("------------------------------");
  imprimirCanalAS7341Prueba("F1 415 nm", canales[0]);
  imprimirCanalAS7341Prueba("F2 445 nm", canales[1]);
  imprimirCanalAS7341Prueba("F3 480 nm", canales[2]);
  imprimirCanalAS7341Prueba("F4 515 nm", canales[3]);
  imprimirCanalAS7341Prueba("Clear ciclo 1", canales[4]);
  imprimirCanalAS7341Prueba("NIR ciclo 1", canales[5]);
  imprimirCanalAS7341Prueba("F5 555 nm", canales[6]);
  imprimirCanalAS7341Prueba("F6 590 nm", canales[7]);
  imprimirCanalAS7341Prueba("F7 630 nm", canales[8]);
  imprimirCanalAS7341Prueba("F8 680 nm", canales[9]);
  imprimirCanalAS7341Prueba("Clear ciclo 2", canales[10]);
  imprimirCanalAS7341Prueba("NIR ciclo 2", canales[11]);

  uint16_t flicker = as7341Prueba.detectFlickerHz();
  imprimirFlickerAS7341Prueba(flicker);
}
