/*
  Procesamiento de datos visibles.

  Sirve para:
  - Tomar la muestra cruda obtenida desde ObtencionDeDatos.ino.
  - Calcular F4-F8 normalizados por suma visible.
  - Calcular suma visible, promedio visible en nm y estado de saturacion.
  - Imprimir todos los datos procesados desde una sola funcion Serial.
*/

const uint16_t UMBRAL_SATURACION_VISIBLE = 60000;

bool procesarDatosVisibles(const MuestraDatosSensor *muestra, DatosProcesadosVisibles *datos) {
  datos->sumaVisible = (uint32_t)muestra->f4 + muestra->f5 + muestra->f6 +
                       muestra->f7 + muestra->f8;
  datos->saturado = muestra->f4 >= UMBRAL_SATURACION_VISIBLE ||
                    muestra->f5 >= UMBRAL_SATURACION_VISIBLE ||
                    muestra->f6 >= UMBRAL_SATURACION_VISIBLE ||
                    muestra->f7 >= UMBRAL_SATURACION_VISIBLE ||
                    muestra->f8 >= UMBRAL_SATURACION_VISIBLE;

  if (datos->sumaVisible == 0) {
    datos->f4Normalizado = 0.0;
    datos->f5Normalizado = 0.0;
    datos->f6Normalizado = 0.0;
    datos->f7Normalizado = 0.0;
    datos->f8Normalizado = 0.0;
    datos->promedioVisibleNm = 0.0;
    return true;
  }

  float sumaVisible = datos->sumaVisible;
  datos->f4Normalizado = muestra->f4 / sumaVisible;
  datos->f5Normalizado = muestra->f5 / sumaVisible;
  datos->f6Normalizado = muestra->f6 / sumaVisible;
  datos->f7Normalizado = muestra->f7 / sumaVisible;
  datos->f8Normalizado = muestra->f8 / sumaVisible;
  datos->promedioVisibleNm = (515.0 * muestra->f4 + 555.0 * muestra->f5 +
                              590.0 * muestra->f6 + 630.0 * muestra->f7 +
                              680.0 * muestra->f8) / sumaVisible;

  return true;
}

void imprimirDatosProcesadosEnSerial(const DatosProcesadosVisibles *datos) {
  Serial.println();
  Serial.println("Datos visibles procesados");
  Serial.println("------------------------------");
  Serial.print("F4 515 nm normalizado: ");
  Serial.println(datos->f4Normalizado, 6);
  Serial.print("F5 555 nm normalizado: ");
  Serial.println(datos->f5Normalizado, 6);
  Serial.print("F6 590 nm normalizado: ");
  Serial.println(datos->f6Normalizado, 6);
  Serial.print("F7 630 nm normalizado: ");
  Serial.println(datos->f7Normalizado, 6);
  Serial.print("F8 680 nm normalizado: ");
  Serial.println(datos->f8Normalizado, 6);
  Serial.print("Suma visible: ");
  Serial.println(datos->sumaVisible);
  Serial.print("Promedio visible nm: ");
  Serial.println(datos->promedioVisibleNm, 2);
  Serial.print("Estado saturacion: ");
  Serial.println(datos->saturado ? "Saturado" : "OK");
}

