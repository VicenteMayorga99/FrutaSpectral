/*
  Comunicacion Ethernet por ENC28J60.

  Este archivo publica por RJ45 el dato procesado principal del sensor:
  promedioVisibleNm.

  Usa IP fija y el mismo conexionado probado en PruebaENC28J60:
    SCK      -> GPIO14
    MOSI/ST  -> GPIO27
    MISO/SO  -> GPIO26
    CS       -> GPIO25

  Servicios disponibles:
    - Modbus TCP puerto 502: entrega promedioVisibleNm como holding register.
    - HTTP puerto 80: entrega el mismo valor en texto plano desde navegador.
*/

#include <SPI.h>
#include <EthernetENC.h>

void enviarExcepcionModbus(EthernetClient &cliente, const uint8_t *solicitud,
                           uint8_t codigoFuncion, uint8_t codigoExcepcion);

const int ETH_SCK_PIN = 14;
const int ETH_MOSI_PIN = 27;
const int ETH_MISO_PIN = 26;
const int ETH_CS_PIN = 25;

byte ETH_MAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x28, 0x61};

IPAddress ETH_IP(192, 168, 1, 77);
IPAddress ETH_DNS(192, 168, 1, 1);
IPAddress ETH_GATEWAY(192, 168, 1, 1);
IPAddress ETH_SUBNET(255, 255, 255, 0);

const uint16_t MODBUS_PORT = 502;
const uint8_t MODBUS_UNIT_ID = 1;
const uint16_t REG_PROMEDIO_VISIBLE_NM_X100 = 0;
const uint16_t REG_DATO_VALIDO = 1;

EthernetServer servidorModbus(MODBUS_PORT);
EthernetServer servidorPromedioHttp(80);

bool comunicacionEthernetLista = false;
float ultimoPromedioVisibleNm = 0.0;
uint16_t ultimoPromedioVisibleNmX100 = 0;
bool hayPromedioVisibleValido = false;
unsigned long ultimoPromedioVisibleMs = 0;

void imprimirDireccionEthernet(const char *etiqueta, IPAddress direccion) {
  Serial.print(etiqueta);
  Serial.print(": ");
  Serial.println(direccion);
}

void iniciarComunicacionEthernet() {
  Serial.println();
  Serial.println("Iniciando comunicacion Ethernet ENC28J60...");
  Serial.print("SCK GPIO ");
  Serial.println(ETH_SCK_PIN);
  Serial.print("MOSI/ST GPIO ");
  Serial.println(ETH_MOSI_PIN);
  Serial.print("MISO/SO GPIO ");
  Serial.println(ETH_MISO_PIN);
  Serial.print("CS GPIO ");
  Serial.println(ETH_CS_PIN);

  SPI.begin(ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
  Ethernet.init(ETH_CS_PIN);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("ERROR: no se detecto ENC28J60 por SPI.");
    Serial.println("El sensor sigue funcionando, pero no habra salida por RJ45.");
    comunicacionEthernetLista = false;
    return;
  }

  Ethernet.begin(ETH_MAC, ETH_IP, ETH_DNS, ETH_GATEWAY, ETH_SUBNET);
  servidorModbus.begin();
  servidorPromedioHttp.begin();
  comunicacionEthernetLista = true;

  Serial.println("OK: ENC28J60 listo con IP fija.");
  imprimirDireccionEthernet("IP", Ethernet.localIP());
  imprimirDireccionEthernet("Gateway", Ethernet.gatewayIP());
  imprimirDireccionEthernet("Mascara", Ethernet.subnetMask());
  Serial.println("Modbus TCP: puerto 502, Unit ID 1");
  Serial.println("Holding register 40001: promedioVisibleNm x 100");
  Serial.println("Holding register 40002: dato valido (0/1)");
  Serial.print("HTTP prueba: http://");
  Serial.println(Ethernet.localIP());

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("AVISO: no hay link RJ45. Conecta cable Ethernet cuando quieras probar red.");
  }
}

void actualizarDatoEthernet(const DatosProcesadosVisibles *datos) {
  ultimoPromedioVisibleNm = datos->promedioVisibleNm;
  float valorEscalado = datos->promedioVisibleNm * 100.0;

  if (valorEscalado < 0.0) {
    valorEscalado = 0.0;
  } else if (valorEscalado > 65535.0) {
    valorEscalado = 65535.0;
  }

  ultimoPromedioVisibleNmX100 = (uint16_t)(valorEscalado + 0.5);
  hayPromedioVisibleValido = true;
  ultimoPromedioVisibleMs = millis();
}

void imprimirPromedioEthernet(Print &salida) {
  salida.print("promedioVisibleNm=");

  if (hayPromedioVisibleValido) {
    salida.println(ultimoPromedioVisibleNm, 2);
  } else {
    salida.println("sin_dato");
  }
}

uint16_t leerRegistroModbus(uint16_t direccion) {
  if (direccion == REG_PROMEDIO_VISIBLE_NM_X100) {
    return ultimoPromedioVisibleNmX100;
  }

  if (direccion == REG_DATO_VALIDO) {
    return hayPromedioVisibleValido ? 1 : 0;
  }

  return 0;
}

void enviarExcepcionModbus(EthernetClient &cliente, const uint8_t *solicitud,
                           uint8_t codigoFuncion, uint8_t codigoExcepcion) {
  cliente.write(solicitud[0]);
  cliente.write(solicitud[1]);
  cliente.write((uint8_t)0x00);
  cliente.write((uint8_t)0x00);
  cliente.write((uint8_t)0x00);
  cliente.write((uint8_t)0x03);
  cliente.write(solicitud[6]);
  cliente.write(codigoFuncion | 0x80);
  cliente.write(codigoExcepcion);
}

void atenderClienteModbus() {
  EthernetClient cliente = servidorModbus.available();

  if (!cliente) {
    return;
  }

  uint8_t solicitud[12];
  int recibidos = 0;
  unsigned long inicio = millis();

  while (cliente.connected() && recibidos < (int)sizeof(solicitud) &&
         millis() - inicio < 1000) {
    if (cliente.available()) {
      solicitud[recibidos] = cliente.read();
      recibidos++;
    }
  }

  if (recibidos < 12) {
    cliente.stop();
    return;
  }

  uint16_t protocolo = ((uint16_t)solicitud[2] << 8) | solicitud[3];
  uint8_t unitId = solicitud[6];
  uint8_t funcion = solicitud[7];
  uint16_t direccionInicial = ((uint16_t)solicitud[8] << 8) | solicitud[9];
  uint16_t cantidad = ((uint16_t)solicitud[10] << 8) | solicitud[11];

  if (protocolo != 0 || unitId != MODBUS_UNIT_ID) {
    cliente.stop();
    return;
  }

  if (funcion != 0x03 && funcion != 0x04) {
    enviarExcepcionModbus(cliente, solicitud, funcion, 0x01);
    cliente.stop();
    return;
  }

  if (cantidad < 1 || cantidad > 2 ||
      direccionInicial + cantidad > 2) {
    enviarExcepcionModbus(cliente, solicitud, funcion, 0x02);
    cliente.stop();
    return;
  }

  uint8_t bytesDatos = cantidad * 2;
  uint16_t longitudMbap = 3 + bytesDatos;

  cliente.write(solicitud[0]);
  cliente.write(solicitud[1]);
  cliente.write((uint8_t)0x00);
  cliente.write((uint8_t)0x00);
  cliente.write((uint8_t)(longitudMbap >> 8));
  cliente.write((uint8_t)(longitudMbap & 0xFF));
  cliente.write(unitId);
  cliente.write(funcion);
  cliente.write(bytesDatos);

  for (uint16_t i = 0; i < cantidad; i++) {
    uint16_t valor = leerRegistroModbus(direccionInicial + i);
    cliente.write((uint8_t)(valor >> 8));
    cliente.write((uint8_t)(valor & 0xFF));
  }

  cliente.stop();
  Serial.println("Dato promedioVisibleNm enviado por Modbus TCP.");
}

void atenderClientePromedioHttp() {
  EthernetClient cliente = servidorPromedioHttp.available();

  if (!cliente) {
    return;
  }

  bool lineaVacia = true;
  unsigned long inicio = millis();

  while (cliente.connected() && millis() - inicio < 1000) {
    if (!cliente.available()) {
      continue;
    }

    char c = cliente.read();

    if (c == '\n' && lineaVacia) {
      cliente.println("HTTP/1.1 200 OK");
      cliente.println("Content-Type: text/plain");
      cliente.println("Connection: close");
      cliente.println();
      cliente.println("FrutaSpectral Ethernet");
      imprimirPromedioEthernet(cliente);
      cliente.print("timestampMs=");
      cliente.println(ultimoPromedioVisibleMs);
      break;
    }

    if (c == '\n') {
      lineaVacia = true;
    } else if (c != '\r') {
      lineaVacia = false;
    }
  }

  delay(1);
  cliente.stop();
  Serial.println("Dato promedioVisibleNm enviado por HTTP.");
}

void atenderComunicacionEthernet() {
  if (!comunicacionEthernetLista) {
    return;
  }

  atenderClienteModbus();
  atenderClientePromedioHttp();
}
