/*
 * Clasificador por Color con AS7341 + SG90
 * ─────────────────────────────────────────────────────────────────────────────
 * Comportamiento:
 *   - Arranque        → Servo en 0°  (posición de espera)
 *   - Detecta ROJO    → Servo se mueve a 45°  y SE QUEDA ahí
 *   - Detecta VERDE   → Servo se mueve a 135° y SE QUEDA ahí
 *   - Sin color claro → Servo no se mueve (mantiene posición actual)
 *
 * El servo solo se mueve cuando detecta un color nuevo distinto al actual.
 *
 * Conexiones:
 *   sensor : VIN→3.3V | GND→GND | SCL→A5 | SDA→A4
 *   SG90   : VCC→5V externo | GND→GND común | Señal→Pin 9
 *
 * Librería requerida: "Adafruit AS7341" (instalar desde Library Manager)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <Adafruit_AS7341.h>
#include <Servo.h>

// ── Objetos ───────────────────────────────────────────────────────────────────
Adafruit_AS7341 as7341;
Servo miServo;

// ── Pines y ángulos ───────────────────────────────────────────────────────────
const int PIN_SERVO     = 9;
const int ANGULO_INICIO = 120;    // Posición inicial de espera
const int ANGULO_ROJO   = 75;   // Clasificación rojo
const int ANGULO_VERDE  = 160;  // Clasificación verde

// ── Parámetros de detección ───────────────────────────────────────────────────
// UMBRAL_DIFERENCIA: diferencia mínima entre canal rojo y verde para
//   considerar que hay un color claro. Súbelo si hay falsos positivos
//   con luz ambiente; bájalo si no detecta bien en condiciones de poca luz.
const int UMBRAL_DIFERENCIA = 500;

// LECTURAS_CONFIRMACION: cuántas lecturas consecutivas del mismo color
//   se requieren antes de mover el servo. Evita movimientos por destellos.
const int LECTURAS_CONFIRMACION = 3;

// ── Estado del sistema ────────────────────────────────────────────────────────
// 0 = inicio/neutro | 1 = rojo confirmado | 2 = verde confirmado
int estadoActual       = 0;
int colorCandidato     = 0;  // color detectado en la lectura actual
int contadorConfirm    = 0;  // contador de lecturas consecutivas del mismo color

// ─────────────────────────────────────────────────────────────────────────────
void moverServo(int angulo, const char* etiqueta) {
  miServo.write(angulo);
  Serial.print(">>> SERVO movido a ");
  Serial.print(angulo);
  Serial.print("°  [");
  Serial.print(etiqueta);
  Serial.println("]");
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("════════════════════════════════════════");
  Serial.println("  Clasificador por Color  AS7341 + SG90");
  Serial.println("════════════════════════════════════════");

  // Servo: posición inicial 0°
  miServo.attach(PIN_SERVO);
  moverServo(ANGULO_INICIO, "INICIO");
  delay(600);

  // Sensor AS7341
  if (!as7341.begin()) {
    Serial.println("ERROR: Sensor AS7341 no encontrado.");
    Serial.println("Verifica conexiones SDA/SCL y alimentación.");
    while (1) delay(100);
  }

  // Configuración del sensor
  as7341.setATIME(100);              // \ tiempo de integración:
  as7341.setASTEP(999);              // /  ~100 ms por lectura
  as7341.setGain(AS7341_GAIN_256X);  // Ganancia alta (bajar si hay saturación)

  Serial.println("Sensor listo. Esperando objeto...");
  Serial.println("────────────────────────────────────────");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {

  // 1. Leer todos los canales del sensor
  if (!as7341.readAllChannels()) {
    Serial.println("Error de lectura. Reintentando...");
    delay(200);
    return;
  }

  // 2. Obtener canales representativos de rojo y verde
  //    F8 (680 nm) y F7 (630 nm) → rojo
  //    F4 (515 nm) y F5 (555 nm) → verde
  uint16_t r1 = as7341.getChannel(AS7341_CHANNEL_680nm_F8);
  uint16_t r2 = as7341.getChannel(AS7341_CHANNEL_630nm_F7);
  uint16_t g1 = as7341.getChannel(AS7341_CHANNEL_515nm_F4);
  uint16_t g2 = as7341.getChannel(AS7341_CHANNEL_555nm_F5);

  uint16_t rojoFinal  = (r1 + r2) / 2;
  uint16_t verdeFinal = (g1 + g2) / 2;
  int diferencia      = (int)rojoFinal - (int)verdeFinal;

  // 3. Determinar color detectado en esta lectura
  int colorDetectado = 0;  // 0 = ambiguo
  if (diferencia >  UMBRAL_DIFERENCIA) colorDetectado = 1;  // ROJO
  if (diferencia < -UMBRAL_DIFERENCIA) colorDetectado = 2;  // VERDE

  // 4. Lógica de confirmación: acumular lecturas del mismo color
  if (colorDetectado != 0 && colorDetectado == colorCandidato) {
    contadorConfirm++;
  } else {
    // Cambió el color detectado → reiniciar contador
    colorCandidato  = colorDetectado;
    contadorConfirm = (colorDetectado != 0) ? 1 : 0;
  }

  // 5. Mover servo solo cuando se confirma el color Y es distinto al estado actual
  if (contadorConfirm >= LECTURAS_CONFIRMACION) {

    if (colorCandidato == 1 && estadoActual != 1) {
      // Nuevo color confirmado: ROJO
      moverServo(ANGULO_ROJO, "ROJO");
      estadoActual = 1;

    } else if (colorCandidato == 2 && estadoActual != 2) {
      // Nuevo color confirmado: VERDE
      moverServo(ANGULO_VERDE, "VERDE");
      estadoActual = 2;
    }
    // Si el mismo color ya estaba activo, no se hace nada (servo fijo)
  }

  // 6. Log en monitor serie para calibración
  Serial.print("R:");
  Serial.print(rojoFinal);
  Serial.print(" G:");
  Serial.print(verdeFinal);
  Serial.print(" Dif:");
  Serial.print(diferencia);
  Serial.print("  Detectado:");
  if (colorDetectado == 1) Serial.print("ROJO ");
  else if (colorDetectado == 2) Serial.print("VERDE");
  else Serial.print("---  ");
  Serial.print("  Confirm:");
  Serial.print(contadorConfirm);
  Serial.print("/");
  Serial.print(LECTURAS_CONFIRMACION);
  Serial.print("  ServoEstado:");
  if (estadoActual == 0) Serial.println("INICIO(0°)");
  else if (estadoActual == 1) Serial.println("ROJO(45°)");
  else Serial.println("VERDE(135°)");

  delay(200);  // ~200 ms entre lecturas
}