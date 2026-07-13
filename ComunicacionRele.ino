/*
  Comunicacion por reles hacia PLC.

  Sirve para:
  - Mantener estable el dato de color hacia el PLC.
  - Avisar al PLC cuando se detecta una fruta nueva mediante un pulso de clock.

  Senales:
  - D26/GPIO26: clock, pulso de 0.5 s cuando hay nueva fruta detectada.
  - D25/GPIO25: dato estable de color, LOW = verde, HIGH = roja.
*/

const int RELE_CLOCK_PIN = 26;
const int RELE_DATO_COLOR_PIN = 25;
const unsigned long RELE_CLOCK_PULSO_MS = 500;

bool comunicacionReleIniciada = false;
bool pulsoClockReleActivo = false;
unsigned long inicioPulsoClockReleMs = 0;

void iniciarComunicacionRele() {
  pinMode(RELE_CLOCK_PIN, OUTPUT);
  pinMode(RELE_DATO_COLOR_PIN, OUTPUT);

  digitalWrite(RELE_CLOCK_PIN, LOW);
  digitalWrite(RELE_DATO_COLOR_PIN, LOW);

  comunicacionReleIniciada = true;
  pulsoClockReleActivo = false;

  Serial.println();
  Serial.println("Comunicacion por reles hacia PLC activa.");
  Serial.print("Clock fruta: GPIO");
  Serial.println(RELE_CLOCK_PIN);
  Serial.print("Dato color: GPIO");
  Serial.println(RELE_DATO_COLOR_PIN);
  Serial.println("Dato color LOW=verde, HIGH=roja.");
  Serial.print("Pulso clock ms: ");
  Serial.println(RELE_CLOCK_PULSO_MS);
}

void asegurarComunicacionReleIniciada() {
  if (!comunicacionReleIniciada) {
    iniciarComunicacionRele();
  }
}

void actualizarColorRele(bool frutaRoja) {
  asegurarComunicacionReleIniciada();

  digitalWrite(RELE_DATO_COLOR_PIN, frutaRoja ? HIGH : LOW);

  Serial.print("PLC rele: dato color estable = ");
  Serial.println(frutaRoja ? "roja" : "verde");
}

void enviarEventoFrutaPorRele() {
  asegurarComunicacionReleIniciada();

  digitalWrite(RELE_CLOCK_PIN, HIGH);

  pulsoClockReleActivo = true;
  inicioPulsoClockReleMs = millis();

  Serial.println("PLC rele: evento fruta, clock activo.");
}

void atenderComunicacionRele() {
  if (!pulsoClockReleActivo) {
    return;
  }

  if (millis() - inicioPulsoClockReleMs >= RELE_CLOCK_PULSO_MS) {
    digitalWrite(RELE_CLOCK_PIN, LOW);
    pulsoClockReleActivo = false;
    Serial.println("PLC rele: clock apagado.");
  }
}
