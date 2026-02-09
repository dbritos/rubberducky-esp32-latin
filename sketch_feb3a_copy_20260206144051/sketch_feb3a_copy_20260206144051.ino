#include "USB.h"
#include "USBHIDKeyboard.h"
#include "FS.h"
#include "SD_MMC.h"

// Instancia del teclado USB nativo
USBHIDKeyboard Keyboard;

// Configuración de pines para SD_MMC (Basado en modelos S3-CAM) [4, 5]
#define PIN_SD_CLK 39
#define PIN_SD_CMD 38
#define PIN_SD_D0  40
const int botonBoot = 0; // Botón BOOT es el GPIO0 [6]

String ultimaLinea = ""; // Para el comando REPEAT [7]

// Función para ejecutar teclas especiales y de función
void ejecutarTeclaEspecial(String tecla) {
  tecla.toUpperCase();
  if (tecla == "ENTER") Keyboard.write(KEY_RETURN);
  else if (tecla == "TAB") Keyboard.write(KEY_TAB);
  else if (tecla == "SPACE") Keyboard.write(' ');
  else if (tecla == "ESC") Keyboard.write(KEY_ESC);
  else if (tecla == "BACKSPACE") Keyboard.write(KEY_BACKSPACE);
  else if (tecla == "DELETE") Keyboard.write(KEY_DELETE);
  else if (tecla == "UPARROW" || tecla == "UP") Keyboard.write(KEY_UP_ARROW);
  else if (tecla == "DOWNARROW" || tecla == "DOWN") Keyboard.write(KEY_DOWN_ARROW);
  else if (tecla == "LEFTARROW" || tecla == "LEFT") Keyboard.write(KEY_LEFT_ARROW);
  else if (tecla == "RIGHTARROW" || tecla == "RIGHT") Keyboard.write(KEY_RIGHT_ARROW);
  else if (tecla == "PRINTSCREEN") Keyboard.write(0xCE); 
  else if (tecla == "MENU" || tecla == "APP") Keyboard.write(0xED);
  else if (tecla.startsWith("F")) {
    int fnum = tecla.substring(1).toInt();
    if (fnum >= 1 && fnum <= 12) Keyboard.write(0xC1 + fnum - 1);
  }
}

// Intérprete de comandos DuckyScript [8, 9]
void procesarLinea(String linea) {
  linea.trim();
  if (linea.length() == 0 || linea.startsWith("REM")) return; // Ignora comentarios [10]

  if (linea.startsWith("REPEAT")) {
    int veces = linea.substring(7).toInt();
    for (int i = 0; i < veces; i++) procesarLinea(ultimaLinea);
    return;
  }
  ultimaLinea = linea; // Guardar para REPEAT [7]

  if (linea.startsWith("DELAY")) {
    delay(linea.substring(6).toInt());
  } 
  else if (linea.startsWith("STRING")) {
    Keyboard.print(linea.substring(7));
  } 
  else if (linea.startsWith("CTRL") || linea.startsWith("ALT") || linea.startsWith("SHIFT") || linea.startsWith("GUI") || linea.startsWith("WINDOWS")) {
    int espacioIdx = linea.indexOf(" ");
    uint8_t mod;
    if (linea.startsWith("CTRL")) mod = KEY_LEFT_CTRL;
    else if (linea.startsWith("ALT")) mod = KEY_LEFT_ALT;
    else if (linea.startsWith("SHIFT")) mod = KEY_LEFT_SHIFT;
    else mod = KEY_LEFT_GUI;

    Keyboard.press(mod);
    if (espacioIdx != -1) {
      String resto = linea.substring(espacioIdx + 1);
      if (resto.length() == 1) Keyboard.press(resto.charAt(0));
      else ejecutarTeclaEspecial(resto);
    }
    delay(100);
    Keyboard.releaseAll();
  } 
  else {
    ejecutarTeclaEspecial(linea);
  }
}

void ejecutarScript() {
  File file = SD_MMC.open("/script.txt");
  if (!file) {
    Serial.println("Error: No se encontró script.txt");
    return;
  }
  Serial.println("Ejecutando script...");
  while (file.available()) {
    String linea = file.readStringUntil('\n');
    procesarLinea(linea);
  }
  file.close();
  Serial.println("Script finalizado.");
}

// Función setup() - Obligatoria
void setup() {
  Serial.begin(115200);
  pinMode(botonBoot, INPUT_PULLUP);
  Keyboard.begin();
  USB.begin();
  delay(500); // Estabilidad inicial [11]

  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Error al montar la SD");
    return;
  }
  Serial.println("Sistema listo. Presioná el botón BOOT.");
}

// Función loop() - Obligatoria
void loop() {
  if (digitalRead(botonBoot) == LOW) {
    ejecutarScript();
    delay(1000); // Evitar múltiples disparos
  }
}
