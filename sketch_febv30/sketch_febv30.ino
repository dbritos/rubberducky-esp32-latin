#include "USB.h"
#include "USBHIDKeyboard.h"
#include "FS.h"
#include "SD_MMC.h"

// ==========================================
// SELECCIÓN DE DISTRIBUCIÓN DE TECLADO
// ==========================================
#define LAYOUT_ES_LATAM 

USBHIDKeyboard Keyboard;

// Configuración de pines para SD_MMC (Basado en ESP32-S3-CAM) [3]
#define PIN_SD_CLK 39
#define PIN_SD_CMD 38
#define PIN_SD_D0  40
const int botonBoot = 0; // Pin BOOT (GPIO0) [3, 4]

String ultimaLinea = ""; 
int jitterAmount = 0; // Característica de DuckyScript 3.0 [2]

// Función de traducción avanzada para Layout Latinoamericano
void enviarTextoTraduciendo(String texto) {
  for (int i = 0; i < texto.length(); i++) {
    unsigned char c = (unsigned char)texto[i];
    Keyboard.releaseAll(); // Limpia el bus USB entre caracteres

    #ifdef LAYOUT_ES_LATAM
      // Símbolos de Rutas y Red
      if (c == ':') { delay(15); Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('.'); Keyboard.releaseAll(); delay(15); }
      else if (c == '-') { Keyboard.write('/'); } 
      else if (c == '_') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('/'); Keyboard.releaseAll(); }
      else if (c == '/') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('7'); Keyboard.releaseAll(); }
      else if (c == '\\') { 
        delay(15); Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); 
        Keyboard.write('-'); // AltGr + ? produce \ en Latam
        Keyboard.releaseAll(); delay(15); 
      }
      else if (c == '@') { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); Keyboard.write('q'); Keyboard.releaseAll(); }

      // Operadores de PowerShell y Sintaxis
      else if (c == '$') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('4'); Keyboard.releaseAll(); }
      else if (c == '=') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('0'); Keyboard.releaseAll(); }
      else if (c == '(') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('8'); Keyboard.releaseAll(); }
      else if (c == ')') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('9'); Keyboard.releaseAll(); }
      else if (c == ';') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(','); Keyboard.releaseAll(); }
      else if (c == '|') { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); Keyboard.write('1'); Keyboard.releaseAll(); }
      
      // Llaves y Corchetes
      else if (c == '{') { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); Keyboard.write('\''); Keyboard.releaseAll(); }
      else if (c == '}') { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); Keyboard.write(']'); Keyboard.releaseAll(); }
      else if (c == '[') { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); Keyboard.write('8'); Keyboard.releaseAll(); }
      else if (c == ']') { Keyboard.press(KEY_LEFT_CTRL); Keyboard.press(KEY_LEFT_ALT); Keyboard.write('9'); Keyboard.releaseAll(); }
      
      // Comillas y Caracteres Especiales
      else if (c == '\'') { Keyboard.write('-'); } 
      else if (c == '"') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('2'); Keyboard.releaseAll(); }
      else if (c == '*') { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write(']'); Keyboard.releaseAll(); }
      else if (c == '+') { Keyboard.write(']'); }
      
      // Manejo de caracteres UTF-8 (¡ y ¿)
      else if (c == 0xC2) { 
          unsigned char nextC = (unsigned char)texto[i+1];
          if(nextC == 0xA1) { Keyboard.write('\''); i++; } // ¡
          else if(nextC == 0xBF) { Keyboard.press(KEY_LEFT_SHIFT); Keyboard.write('='); Keyboard.releaseAll(); i++; } // ¿
      }
      else {
        Keyboard.write(c);
      }
      delay(15); // Sincronización para el bus USB nativo del S3
    #endif
  }
}

void enviarEspecial(String tecla) {
  tecla.toUpperCase();
  if (tecla == "ENTER") Keyboard.write(KEY_RETURN);
  else if (tecla == "TAB") Keyboard.write(KEY_TAB);
  else if (tecla == "SPACE") Keyboard.write(' ');
  else if (tecla == "GUI" || tecla == "WINDOWS") Keyboard.press(KEY_LEFT_GUI);
  else if (tecla == "ALT") Keyboard.press(KEY_LEFT_ALT);
  else if (tecla == "CTRL") Keyboard.press(KEY_LEFT_CTRL);
  else if (tecla == "SHIFT") Keyboard.press(KEY_LEFT_SHIFT);
  else if (tecla == "F4") Keyboard.write(KEY_F4);
  else if (tecla == "LEFTARROW") Keyboard.write(KEY_LEFT_ARROW);
  else if (tecla == "RIGHTARROW") Keyboard.write(KEY_RIGHT_ARROW);
  else if (tecla == "UPARROW") Keyboard.write(KEY_UP_ARROW);
  else if (tecla == "DOWNARROW") Keyboard.write(KEY_DOWN_ARROW);
  
  delay(100);
  Keyboard.releaseAll();
}

void procesarLinea(String linea) {
  linea.trim();
  if (linea.length() == 0 || linea.startsWith("REM")) return;
  ultimaLinea = linea;

  if (linea.startsWith("DELAY")) {
    delay(linea.substring(6).toInt());
  }
  else if (linea.startsWith("STRING")) {
    enviarTextoTraduciendo(linea.substring(7));
  }
  else if (linea.startsWith("REPEAT")) {
    int veces = linea.substring(7).toInt();
    for (int i = 0; i < veces; i++) procesarLinea(ultimaLinea);
  }
  else if (linea.startsWith("JITTER")) {
    jitterAmount = linea.substring(7).toInt();
  }
  else if (linea.startsWith("ALT ") || linea.startsWith("CTRL ") || linea.startsWith("GUI ")) {
    int spaceIdx = linea.indexOf(" ");
    String cmd = linea.substring(0, spaceIdx);
    String key = linea.substring(spaceIdx + 1);
    
    if (cmd == "ALT") Keyboard.press(KEY_LEFT_ALT);
    else if (cmd == "CTRL") Keyboard.press(KEY_LEFT_CTRL);
    else if (cmd == "GUI") Keyboard.press(KEY_LEFT_GUI);
    
    if (key.length() == 1) Keyboard.print(key);
    else enviarEspecial(key);
    
    delay(100);
    Keyboard.releaseAll();
  } else {
    enviarEspecial(linea);
  }
}

void ejecutarScript() {
  File file = SD_MMC.open("/script.txt");
  if (!file) return;
  while (file.available()) {
    String l = file.readStringUntil('\n');
    procesarLinea(l);
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  pinMode(botonBoot, INPUT_PULLUP);
  Keyboard.begin();
  USB.begin();
  delay(2000); 

  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Error SD.");
    return;
  }
  Serial.println("Sistema listo. Presioná el botón BOOT.");
}

void loop() {
  if (digitalRead(botonBoot) == LOW) {
    ejecutarScript();
    delay(1000); // Debounce
  }
}