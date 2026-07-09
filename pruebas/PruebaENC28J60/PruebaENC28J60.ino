/*
  Prueba inicial ESP32 + ENC28J60.

  Objetivo:
  - Verificar que la ESP32 se comunica por SPI con el modulo ENC28J60.
  - Intentar obtener IP por DHCP.
  - Si DHCP falla, usar una IP fija de prueba.
  - Levantar un servidor web simple para confirmar respuesta por RJ45.

  Conexion usada:
    ENC28J60 SCK     -> ESP32 GPIO14
    ENC28J60 ST/MOSI -> ESP32 GPIO27
    ENC28J60 SO/MISO -> ESP32 GPIO26
    ENC28J60 CS      -> ESP32 GPIO25
    ENC28J60 5V      -> 5V/VIN/VBUS o fuente externa de 5 V
    ENC28J60 GND     -> GND ESP32

  RST, INT/LNT, CLK, WOL y Q3/3.3V quedan sin conectar en esta prueba.
*/

#include <SPI.h>
#include <EthernetENC.h>

const int PIN_ETH_SCK = 14;
const int PIN_ETH_MOSI = 27;
const int PIN_ETH_MISO = 26;
const int PIN_ETH_CS = 25;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x28, 0x60};

IPAddress ipFija(192, 168, 1, 77);
IPAddress dnsFijo(192, 168, 1, 1);
IPAddress gatewayFijo(192, 168, 1, 1);
IPAddress mascaraFija(255, 255, 255, 0);

EthernetServer servidor(80);
bool redIniciada = false;

void imprimirPinout() {
  Serial.println();
  Serial.println("Pinout usado para ENC28J60");
  Serial.println("--------------------------");
  Serial.print("SCK      -> GPIO ");
  Serial.println(PIN_ETH_SCK);
  Serial.print("MOSI/ST  -> GPIO ");
  Serial.println(PIN_ETH_MOSI);
  Serial.print("MISO/SO  -> GPIO ");
  Serial.println(PIN_ETH_MISO);
  Serial.print("CS       -> GPIO ");
  Serial.println(PIN_ETH_CS);
  Serial.println("RST      -> No conectado");
  Serial.println("INT/LNT  -> No conectado");
  Serial.println("CLK/WOL  -> No conectados");
}

void imprimirDireccionIp(const char *etiqueta, IPAddress direccion) {
  Serial.print(etiqueta);
  Serial.print(": ");
  Serial.println(direccion);
}

void imprimirConfigRed() {
  Serial.println();
  Serial.println("Configuracion de red");
  Serial.println("--------------------");
  imprimirDireccionIp("IP local", Ethernet.localIP());
  imprimirDireccionIp("Gateway", Ethernet.gatewayIP());
  imprimirDireccionIp("Mascara", Ethernet.subnetMask());
  imprimirDireccionIp("DNS", Ethernet.dnsServerIP());
}

bool iniciarENC28J60() {
  Serial.println();
  Serial.println("Inicializando SPI...");
  SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI, PIN_ETH_CS);
  Ethernet.init(PIN_ETH_CS);

  Serial.println("Probando ENC28J60 por SPI...");

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("ERROR: no se pudo detectar/inicializar ENC28J60.");
    Serial.println("Revisar alimentacion, GND, SCK, MOSI/ST, MISO/SO y CS.");
    return false;
  }

  Serial.println("OK: ENC28J60 detectado por SPI.");
  return true;
}

void iniciarRed() {
  Serial.println();
  Serial.println("Intentando DHCP...");

  if (Ethernet.begin(mac) == 1) {
    Serial.println("OK: DHCP entrego una IP.");
    redIniciada = true;
  } else {
    Serial.println("AVISO: DHCP fallo. Esto no significa necesariamente que el modulo este malo.");
    Serial.println("Usando IP fija de prueba 192.168.1.77.");
    Ethernet.begin(mac, ipFija, dnsFijo, gatewayFijo, mascaraFija);
    redIniciada = true;
  }

  imprimirConfigRed();

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("AVISO: sin link Ethernet. Revisa cable RJ45, switch/router y LEDs del conector.");
  } else {
    Serial.println("OK: link Ethernet reportado por la libreria.");
  }

  servidor.begin();
  Serial.println();
  Serial.print("Servidor web listo: http://");
  Serial.println(Ethernet.localIP());
}

void responderClienteWeb(EthernetClient &cliente) {
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
      cliente.println("ENC28J60 OK");
      cliente.println("ESP32 Ethernet test");
      cliente.println();
      cliente.println("Pinout:");
      cliente.println("SCK GPIO14");
      cliente.println("MOSI/ST GPIO27");
      cliente.println("MISO/SO GPIO26");
      cliente.println("CS GPIO25");
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
  Serial.println("Cliente web atendido.");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("Prueba ESP32 + ENC28J60");
  Serial.println("=======================");
  imprimirPinout();

  if (!iniciarENC28J60()) {
    Serial.println();
    Serial.println("Prueba detenida: primero corregir conexion SPI/alimentacion.");
    return;
  }

  iniciarRed();
}

void loop() {
  if (!redIniciada) {
    delay(1000);
    return;
  }

  EthernetClient cliente = servidor.available();
  if (cliente) {
    responderClienteWeb(cliente);
  }
}
