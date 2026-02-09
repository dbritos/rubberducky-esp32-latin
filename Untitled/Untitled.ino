#include "USB.h"
#include "USBHIDKeyboard.h"
#include "FS.h"
#include "SD_MMC.h"

USBHIDKeyboard Keyboard;

// Pines para ESP32-S3 AI Cam (DFRobot / Freenove) [4]
#define PIN_SD_CLK 39
#define PIN_SD_CMD 38
#define PIN_SD_D0  40
const int botonBoot = 0; 

void enviarTextoTraduciendo(String texto) {
  for (int i = 0; i < texto.length(); i++) {
    unsigned char c = (unsigned char)texto[i];
    Keyboard.releaseAll(); 

    // --- BLOQUE 1: LLAVES Y CORCHETES (Mapeo Físico Directo) [2, 3] ---
    if (c == '{')      { Keyboard.write(39); } // Tecla física ' en US -> { en tu Latam
    else if (c == '}') { Keyboard.write(92); } // Tecla física \ en US -> } en tu Latam
    else if (c == '[') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(39); Keyboard.releaseAll(); } 
    else if (c == ']') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(92); Keyboard.releaseAll(); }

    // --- BLOQUE 2: ARREGLOS PREVIOS (@, :, ;) [3, 5] ---
    else if (c == '@') { 
      delay(40); // Tiempo de guarda para AltGr
      Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); 
      Keyboard.write(20); // Tecla 'q'
      Keyboard.releaseAll(); 
    }
    else if (c == ':') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(46); Keyboard.releaseAll(); }
    else if (c == ';') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(44); Keyboard.releaseAll(); }
    
    // --- BLOQUE 3: POWERSHELL Y RUTAS [3, 5] ---
    else if (c == '$') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(52); Keyboard.releaseAll(); }
    else if (c == '=') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(48); Keyboard.releaseAll(); }
    else if (c == '(') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(56); Keyboard.releaseAll(); }
    else if (c == ')') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(57); Keyboard.releaseAll(); }
    else if (c == '/') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(55); Keyboard.releaseAll(); }
    else if (c == '_') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(47); Keyboard.releaseAll(); }
    else if (c == '|') { Keyboard.write(96); } // Código físico para pipe en tu log [2]

    // --- BLOQUE 4: CARACTERES ESPECIALES UTF-8 ---
    else if (c == 0xC2) { 
      unsigned char sig = (unsigned char)texto[i+1];
      if(sig == 0xA1) { Keyboard.write(43); i++; } // ¡ [3]
      else if(sig == 0xBF) { Keyboard.write(61); i++; } // ¿ [5]
    }
    else { Keyboard.write(c); }
    
    delay(25); // Sincronización para bus USB nativo del S3 [6]
    Keyboard.releaseAll();
  }
}

// ... Resto de la lógica de procesarLinea y setup ...
void procesarLinea(String linea) {
  linea.trim();
  if (linea.length() == 0 || linea.startsWith("REM")) return;
  if (linea.startsWith("DELAY")) delay(linea.substring(6).toInt());
  else if (linea.startsWith("STRING")) enviarTextoTraduciendo(linea.substring(7));
  else if (linea.startsWith("ENTER")) Keyboard.write(KEY_RETURN);
  else if (linea.startsWith("GUI r")) { Keyboard.press(KEY_LEFT_GUI); Keyboard.write('r'); Keyboard.releaseAll(); }
  else if (linea.startsWith("ALT F4")) { Keyboard.press(KEY_LEFT_ALT); Keyboard.write(KEY_F4); Keyboard.releaseAll(); }
}

void setup() {
  Keyboard.begin();
  USB.begin();
  delay(2000);
  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
  SD_MMC.begin("/sdcard", true);
  pinMode(botonBoot, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(botonBoot) == LOW) {
    File file = SD_MMC.open("/script.txt");
    while (file && file.available()) {
      String l = file.readStringUntil('\n');
      procesarLinea(l);
    }
    if(file) file.close();
    delay(1000);
  }
}
