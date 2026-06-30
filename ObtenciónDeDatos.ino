/*
  Obtencion de datos seleccionados del AS7341.

  Sirve para:
  - Configurar manualmente el SMUX del sensor para leer F4, F5, F6, F7, F8 y Clear.
  - Obtener esos seis datos en un solo ciclo de integracion.
  - Entregar la muestra cruda para que otro archivo realice el procesamiento.

*/

#include <Adafruit_AS7341.h>

// Objeto global creado en FrutaSpectral.ino; se usa aqui para leer el sensor.
extern Adafruit_AS7341 as7341;

bool escribirRegistroAS7341(uint8_t registro, uint8_t valor) {
  Wire.beginTransmission(AS7341_I2CADDR_DEFAULT);
  Wire.write(registro);
  Wire.write(valor);
  return Wire.endTransmission() == 0;
}

uint8_t leerRegistroAS7341(uint8_t registro) {
  Wire.beginTransmission(AS7341_I2CADDR_DEFAULT);
  Wire.write(registro);
  Wire.endTransmission(false);
  Wire.requestFrom(AS7341_I2CADDR_DEFAULT, 1);
  return Wire.available() ? Wire.read() : 0;
}

bool leerRegistrosAS7341(uint8_t registroInicial, uint8_t *datos, uint8_t cantidad) {
  Wire.beginTransmission(AS7341_I2CADDR_DEFAULT);
  Wire.write(registroInicial);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  uint8_t recibidos = Wire.requestFrom(AS7341_I2CADDR_DEFAULT, cantidad);

  if (recibidos != cantidad) {
    return false;
  }

  for (uint8_t i = 0; i < cantidad; i++) {
    datos[i] = Wire.read();
  }

  return true;
}

bool aplicarSmuxF4F8Clear() {
  // Detiene la medicion espectral antes de cambiar el SMUX.
  as7341.enableSpectralMeasurement(false);

  // CFG6[4:3] = 2: comando WRITE, copia la configuracion RAM del SMUX al sensor.
  uint8_t cfg6 = leerRegistroAS7341(AS7341_CFG6);
  escribirRegistroAS7341(AS7341_CFG6, (cfg6 & 0xE7) | 0x10);

  // Configuracion SMUX personalizada:
  // CH0=F5, CH1=F6, CH2=F7, CH3=F8, CH4=Clear, CH5=F4.
  escribirRegistroAS7341(0x00, 0x00);
  escribirRegistroAS7341(0x01, 0x00);
  escribirRegistroAS7341(0x02, 0x00);
  escribirRegistroAS7341(0x03, 0x40); // F8 izquierdo -> ADC3.
  escribirRegistroAS7341(0x04, 0x02); // F6 izquierdo -> ADC1.
  escribirRegistroAS7341(0x05, 0x60); // F4 izquierdo -> ADC5.
  escribirRegistroAS7341(0x06, 0x10); // F5 izquierdo -> ADC0.
  escribirRegistroAS7341(0x07, 0x03); // F7 izquierdo -> ADC2.
  escribirRegistroAS7341(0x08, 0x50); // Clear izquierdo -> ADC4.
  escribirRegistroAS7341(0x09, 0x10); // F5 derecho -> ADC0.
  escribirRegistroAS7341(0x0A, 0x03); // F7 derecho -> ADC2.
  escribirRegistroAS7341(0x0B, 0x00);
  escribirRegistroAS7341(0x0C, 0x00);
  escribirRegistroAS7341(0x0D, 0x06); // F4 derecho -> ADC5.
  escribirRegistroAS7341(0x0E, 0x24); // F6 derecho -> ADC1, F8 derecho -> ADC3.
  escribirRegistroAS7341(0x0F, 0x00);
  escribirRegistroAS7341(0x10, 0x00);
  escribirRegistroAS7341(0x11, 0x50); // Clear derecho -> ADC4.
  escribirRegistroAS7341(0x12, 0x00);
  escribirRegistroAS7341(0x13, 0x00); // NIR deshabilitado.

  // ENABLE[4] = SMUXEN. El bit vuelve a 0 cuando termina de aplicar el SMUX.
  uint8_t enable = leerRegistroAS7341(AS7341_ENABLE);
  escribirRegistroAS7341(AS7341_ENABLE, enable | 0x10);

  unsigned long inicio = millis();
  while ((leerRegistroAS7341(AS7341_ENABLE) & 0x10) != 0) {
    if (millis() - inicio > 1000) {
      return false;
    }
    delay(1);
  }

  return true;
}

bool leerF4F8ClearUnCiclo(uint16_t *f4, uint16_t *f5, uint16_t *f6,
                          uint16_t *f7, uint16_t *f8, uint16_t *clear) {
  if (!aplicarSmuxF4F8Clear()) {
    return false;
  }

  as7341.enableSpectralMeasurement(true);
  as7341.delayForData(0);

  uint8_t datos[12];
  if (!leerRegistrosAS7341(AS7341_CH0_DATA_L, datos, sizeof(datos))) {
    return false;
  }

  uint16_t adc0 = datos[0] | (datos[1] << 8);
  uint16_t adc1 = datos[2] | (datos[3] << 8);
  uint16_t adc2 = datos[4] | (datos[5] << 8);
  uint16_t adc3 = datos[6] | (datos[7] << 8);
  uint16_t adc4 = datos[8] | (datos[9] << 8);
  uint16_t adc5 = datos[10] | (datos[11] << 8);

  *f5 = adc0;
  *f6 = adc1;
  *f7 = adc2;
  *f8 = adc3;
  *clear = adc4;
  *f4 = adc5;

  return true;
}

bool obtenerDatosSeleccionados(MuestraDatosSensor *muestra) {
  return leerF4F8ClearUnCiclo(&muestra->f4, &muestra->f5, &muestra->f6,
                              &muestra->f7, &muestra->f8, &muestra->clear);
}
