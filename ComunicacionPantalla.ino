/*
  Comunicacion con pantalla OLED I2C SSD1306 de 0.96".

  - Busca la pantalla en las direcciones I2C mas comunes: 0x3C y 0x3D.
  - Inicializa el controlador SSD1306.
  - Usa la franja amarilla superior para mostrar el promedio visible en nm.
  - Dibuja una barra con zoom entre F4 515 nm y F8 680 nm.
  - Usa la zona azul inferior para graficar F4-F8 y Clear.
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PANTALLA_ANCHO = 128;
const int PANTALLA_ALTO = 64;
const int PANTALLA_RESET_PIN = -1;

const uint8_t PANTALLA_DIRECCION_1 = 0x3C;
const uint8_t PANTALLA_DIRECCION_2 = 0x3D;
const int FRANJA_AMARILLA_ALTO = 16;
const int ZONA_AZUL_Y = FRANJA_AMARILLA_ALTO;
const int ZONA_AZUL_ALTO = PANTALLA_ALTO - FRANJA_AMARILLA_ALTO;
const float LONGITUD_ONDA_MIN_NM = 515.0;
const float LONGITUD_ONDA_MAX_NM = 680.0;

Adafruit_SSD1306 pantalla(PANTALLA_ANCHO, PANTALLA_ALTO, &Wire, PANTALLA_RESET_PIN);
bool pantallaIniciada = false;

bool probarDireccionI2C(uint8_t direccion) {
  Wire.beginTransmission(direccion);
  return Wire.endTransmission() == 0;
}

int calcularPosicionOnda(float longitudOndaNm) {
  float ondaLimitada = constrain(longitudOndaNm, LONGITUD_ONDA_MIN_NM, LONGITUD_ONDA_MAX_NM);
  float proporcion = (ondaLimitada - LONGITUD_ONDA_MIN_NM) /
                     (LONGITUD_ONDA_MAX_NM - LONGITUD_ONDA_MIN_NM);

  return 1 + round(proporcion * (PANTALLA_ANCHO - 2));
}

uint16_t obtenerMaximoCanales(const MuestraDatosSensor *muestra) {
  uint16_t maximo = muestra->f4;
  maximo = max(maximo, muestra->f5);
  maximo = max(maximo, muestra->f6);
  maximo = max(maximo, muestra->f7);
  maximo = max(maximo, muestra->f8);
  maximo = max(maximo, muestra->clear);
  return maximo;
}

void dibujarBarraCanal(int x, const char *etiqueta, uint16_t valor, uint16_t maximo) {
  const int anchoBarra = 13;
  const int yBase = PANTALLA_ALTO - 1;
  const int altoMaximoBarra = 30;
  int altoBarra = 0;

  if (maximo > 0) {
    altoBarra = map(valor, 0, maximo, 0, altoMaximoBarra);
  }

  pantalla.drawRect(x, yBase - altoMaximoBarra, anchoBarra, altoMaximoBarra + 1, SSD1306_WHITE);
  pantalla.fillRect(x + 1, yBase - altoBarra + 1, anchoBarra - 2, altoBarra, SSD1306_WHITE);
  pantalla.setCursor(x, ZONA_AZUL_Y + 1);
  pantalla.print(etiqueta);
}

void actualizarPantallaDatosVisibles(const MuestraDatosSensor *muestra,
                                     const DatosProcesadosVisibles *datos) {
  if (!pantallaIniciada) {
    return;
  }

  int posicionOnda = calcularPosicionOnda(datos->promedioVisibleNm);
  uint16_t maximoCanales = obtenerMaximoCanales(muestra);

  // En las OLED amarillo/azul, los pixeles blancos de esta zona se ven amarillos.
  pantalla.fillRect(0, 0, PANTALLA_ANCHO, FRANJA_AMARILLA_ALTO, SSD1306_BLACK);

  pantalla.setTextColor(SSD1306_WHITE);
  pantalla.setTextSize(1);
  pantalla.setCursor(0, 0);
  pantalla.print("Nm ");
  pantalla.print(datos->promedioVisibleNm, 0);

  pantalla.setCursor(70, 0);
  pantalla.print((int)LONGITUD_ONDA_MIN_NM);
  pantalla.print("-");
  pantalla.print((int)LONGITUD_ONDA_MAX_NM);

  pantalla.drawRect(0, 9, PANTALLA_ANCHO, 7, SSD1306_WHITE);
  pantalla.fillRect(1, 10, max(1, posicionOnda - 1), 5, SSD1306_WHITE);
  pantalla.drawFastVLine(posicionOnda, 8, 8, SSD1306_WHITE);

  // La zona inferior azul muestra la respuesta relativa de cada canal leido.
  pantalla.fillRect(0, ZONA_AZUL_Y, PANTALLA_ANCHO, ZONA_AZUL_ALTO, SSD1306_BLACK);
  dibujarBarraCanal(0, "F4", muestra->f4, maximoCanales);
  dibujarBarraCanal(21, "F5", muestra->f5, maximoCanales);
  dibujarBarraCanal(42, "F6", muestra->f6, maximoCanales);
  dibujarBarraCanal(63, "F7", muestra->f7, maximoCanales);
  dibujarBarraCanal(84, "F8", muestra->f8, maximoCanales);
  dibujarBarraCanal(105, "CL", muestra->clear, maximoCanales);

  pantalla.display();
}

void iniciarPantalla() {
  Serial.println();
  Serial.println("Probando pantalla OLED SSD1306...");

  uint8_t direccionPantalla = 0;
  if (probarDireccionI2C(PANTALLA_DIRECCION_1)) {
    direccionPantalla = PANTALLA_DIRECCION_1;
  } else if (probarDireccionI2C(PANTALLA_DIRECCION_2)) {
    direccionPantalla = PANTALLA_DIRECCION_2;
  }

  if (direccionPantalla == 0) {
    Serial.println("No se encontro OLED en 0x3C ni en 0x3D.");
    Serial.println("Revisa VCC, GND, SDA y SCL.");
    pantallaIniciada = false;
    return;
  }

  Serial.print("OLED encontrada en I2C 0x");
  Serial.println(direccionPantalla, HEX);

  if (!pantalla.begin(SSD1306_SWITCHCAPVCC, direccionPantalla)) {
    Serial.println("La OLED respondio por I2C, pero SSD1306 no inicio.");
    pantallaIniciada = false;
    return;
  }

  pantallaIniciada = true;
  pantalla.clearDisplay();
  pantalla.display();
  Serial.println("Pantalla OLED lista.");
}
