/*
  Funcion de escaneo I2C para el sketch de pruebas.

  Recorre las direcciones I2C posibles, intenta comunicarse con cada una y
  muestra por Serial las direcciones que responden. Es util para verificar si
  el AS7341 u otro modulo esta conectado y responde en el bus.
*/

void escanerI2C() {
  // Contador de dispositivos que responden durante el recorrido del bus.
  int dispositivosEncontrados = 0;

  Serial.println();
  Serial.println("Buscando dispositivos I2C...");

  // Las direcciones I2C validas de 7 bits van de 1 a 126.
  for (uint8_t direccion = 1; direccion < 127; direccion++) {
    // Si endTransmission devuelve 0, hay un dispositivo en esa direccion.
    Wire.beginTransmission(direccion);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Dispositivo encontrado en 0x");
      if (direccion < 16) {
        Serial.print("0");
      }
      Serial.println(direccion, HEX);
      dispositivosEncontrados++;
    } else if (error == 4) {
      // Codigo 4: error desconocido reportado por la libreria Wire.
      Serial.print("Error desconocido en 0x");
      if (direccion < 16) {
        Serial.print("0");
      }
      Serial.println(direccion, HEX);
    }
  }

  // Resume el resultado del escaneo completo.
  if (dispositivosEncontrados == 0) {
    Serial.println("No se encontraron dispositivos I2C.");
  } else {
    Serial.print("Total encontrados: ");
    Serial.println(dispositivosEncontrados);
  }
}
