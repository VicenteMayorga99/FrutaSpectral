/*
  Sketch de prueba para escanear el bus I2C.

  Este archivo pertenece a la carpeta pruebas y se compila como un sketch
  independiente. No forma parte del codigo principal de la carpeta raiz.
  Sirve para inicializar el puerto serial, configurar los pines I2C del ESP32
  y llamar periodicamente al escaner de direcciones I2C.
*/

#include <Wire.h>

// Pines I2C usados por el ESP32 para probar el bus.
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

void setup() {
  // Inicia el monitor serial para mostrar las direcciones encontradas.
  Serial.begin(115200);
  delay(1000);

  // Inicializa el bus I2C con los pines configurados.
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  Serial.println();
  Serial.println("Escaner I2C ESP32");
  Serial.print("SDA: GPIO ");
  Serial.println(I2C_SDA_PIN);
  Serial.print("SCL: GPIO ");
  Serial.println(I2C_SCL_PIN);
}

void loop() {
  // Recorre todas las direcciones I2C posibles y muestra las que responden.
  escanerI2C();

  // Espera antes de volver a escanear para que la salida sea legible.
  delay(5000);
}
