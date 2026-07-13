/*
  Control de servo clasificador.

  Sirve para:
  - Manejar un servo SG90 desde funciones llamadas por FrutaSpectral.ino.
  - Evitar repetir write() al servo si la posicion solicitada no cambia.

  Posiciones:
  - Inicio: 120 grados.
  - Posicion 1: 75 grados.
  - Posicion 2: 160 grados.
*/

const int SERVO_PWM_PIN = 32;

const int ANGULO_SERVO_INICIO = 120;
const int ANGULO_SERVO_POSICION_1 = 75;
const int ANGULO_SERVO_POSICION_2 = 160;

const int SERVO_PWM_FRECUENCIA_HZ = 50;
const int SERVO_PWM_RESOLUCION_BITS = 16;
const int SERVO_PULSO_MIN_US = 500;
const int SERVO_PULSO_MAX_US = 2400;
const int SERVO_PERIODO_US = 20000;

// 0 = inicio, 1 = posicion 1, 2 = posicion 2.
int estadoServoActual = 0;
bool controlServoIniciado = false;

uint32_t convertirAnguloServoADuty(int angulo) {
  int anguloLimitado = constrain(angulo, 0, 180);
  uint32_t pulsoUs = map(anguloLimitado, 0, 180, SERVO_PULSO_MIN_US, SERVO_PULSO_MAX_US);
  uint32_t dutyMaximo = (1UL << SERVO_PWM_RESOLUCION_BITS) - 1;
  return (pulsoUs * dutyMaximo) / SERVO_PERIODO_US;
}

void escribirAnguloServo(int angulo) {
  ledcWrite(SERVO_PWM_PIN, convertirAnguloServoADuty(angulo));
}

void moverServoSiCambio(int nuevoEstado, int angulo, const char *etiqueta) {
  if (estadoServoActual == nuevoEstado) {
    return;
  }

  escribirAnguloServo(angulo);
  estadoServoActual = nuevoEstado;

  Serial.print("Servo movido a ");
  Serial.print(angulo);
  Serial.print(" grados [");
  Serial.print(etiqueta);
  Serial.println("]");
}

void iniciarControlServo() {
  ledcAttach(SERVO_PWM_PIN, SERVO_PWM_FRECUENCIA_HZ, SERVO_PWM_RESOLUCION_BITS);
  escribirAnguloServo(ANGULO_SERVO_INICIO);
  estadoServoActual = 0;
  controlServoIniciado = true;

  Serial.println();
  Serial.println("Control servo por promedioVisibleNm activo.");
  Serial.print("Servo PWM en GPIO");
  Serial.println(SERVO_PWM_PIN);
  Serial.print("Angulo inicial: ");
  Serial.println(ANGULO_SERVO_INICIO);
}

void asegurarControlServoIniciado() {
  if (!controlServoIniciado) {
    iniciarControlServo();
  }
}

void moverServoAPosicionInicio() {
  asegurarControlServoIniciado();
  moverServoSiCambio(0, ANGULO_SERVO_INICIO, "INICIO");
}

void moverServoAPosicion1() {
  asegurarControlServoIniciado();
  moverServoSiCambio(1, ANGULO_SERVO_POSICION_1, "POSICION 1");
}

void moverServoAPosicion2() {
  asegurarControlServoIniciado();
  moverServoSiCambio(2, ANGULO_SERVO_POSICION_2, "POSICION 2");
}
