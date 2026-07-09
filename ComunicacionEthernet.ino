/*
  Comunicacion Ethernet por ENC28J60.

  Este archivo publica por RJ45 el dato procesado principal del sensor:
  promedioVisibleNm.

  Usa IP fija y el mismo conexionado probado en PruebaENC28J60:
    SCK      -> GPIO14
    MOSI/ST  -> GPIO27
    MISO/SO  -> GPIO26
    CS       -> GPIO25

  Servidores disponibles:
    - TCP puerto 5000: entrega una linea simple para pruebas con PLC/PC.
    - HTTP puerto 80: entrega el mismo valor en texto plano desde navegador.
*/

#include <SPI.h>
#include <EthernetENC.h>

const int ETH_SCK_PIN = 14;
const int ETH_MOSI_PIN = 27;
const int ETH_MISO_PIN = 26;
const int ETH_CS_PIN = 25;

byte ETH_MAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x28, 0x61};

IPAddress ETH_IP(192, 168, 1, 77);
IPAddress ETH_DNS(192, 168, 1, 1);
IPAddress ETH_GATEWAY(192, 168, 1, 1);
IPAddress ETH_SUBNET(255, 255, 255, 0);

EthernetServer servidorPromedioTcp(5000);
EthernetServer servidorPromedioHttp(80);

bool comunicacionEthernetLista = false;
float ultimoPromedioVisibleNm = 0.0;
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
    Serial.println("El clasificador sigue funcionando, pero no habra salida por RJ45.");
    comunicacionEthernetLista = false;
    return;
  }

  Ethernet.begin(ETH_MAC, ETH_IP, ETH_DNS, ETH_GATEWAY, ETH_SUBNET);
  servidorPromedioTcp.begin();
  servidorPromedioHttp.begin();
  comunicacionEthernetLista = true;

  Serial.println("OK: ENC28J60 listo con IP fija.");
  imprimirDireccionEthernet("IP", Ethernet.localIP());
  imprimirDireccionEthernet("Gateway", Ethernet.gatewayIP());
  imprimirDireccionEthernet("Mascara", Ethernet.subnetMask());
  Serial.println("TCP promedioVisibleNm: puerto 5000");
  Serial.print("HTTP prueba: http://");
  Serial.println(Ethernet.localIP());

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("AVISO: no hay link RJ45. Conecta cable Ethernet cuando quieras probar red.");
  }
}

void actualizarDatoEthernet(const DatosProcesadosVisibles *datos) {
  ultimoPromedioVisibleNm = datos->promedioVisibleNm;
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

void atenderClientePromedioTcp() {
  EthernetClient cliente = servidorPromedioTcp.available();

  if (!cliente) {
    return;
  }

  imprimirPromedioEthernet(cliente);
  cliente.print("timestampMs=");
  cliente.println(ultimoPromedioVisibleMs);
  cliente.stop();
  Serial.println("Dato promedioVisibleNm enviado por TCP.");
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

  atenderClientePromedioTcp();
  atenderClientePromedioHttp();
}
