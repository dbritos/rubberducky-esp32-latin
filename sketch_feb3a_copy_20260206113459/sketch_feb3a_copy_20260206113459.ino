#include "USB.h"
#include "USBHIDKeyboard.h"
#include "FS.h"
#include "SD_MMC.h"

// Instancia del teclado
USBHIDKeyboard Keyboard;

// Configuración de pines para la SD (Basado en modelos S3-CAM) [5, 6]
#define PIN_SD_CLK 39
#define PIN_SD_CMD 38
#define PIN_SD_D0  40
const int botonBoot = 0; // El botón BOOT es el GPIO0 [5, 7]

void setup() {
  Serial.begin(115200);
  pinMode(botonBoot, INPUT_PULLUP); // El botón BOOT tiene pull-up interno [7, 8]
  
  // Inicialización de USB y Teclado
  Keyboard.begin();
  USB.begin();
  
  delay(100); // Estabilización inicial

  // Configuración de pines y montaje de SD en modo 1-bit [9, 10]
  SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
  
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Error al montar la SD. Revisá el formato FAT32.");
    return;
  }
  
  Serial.println("Sistema listo. Presioná BOOT para 'tipear' el archivo.");
}

void loop() {
  // Verificamos si se presiona el botón BOOT [5, 11]
  if (digitalRead(botonBoot) == LOW) {
    Serial.println("Botón presionado. Leyendo archivo y enviando por teclado...");
    
    // Abrimos el archivo de la tarjeta SD [12]
    File file = SD_MMC.open("/texto.txt");
    
    if (file) {
      // Leemos el archivo y lo enviamos caracter por caracter como teclado
      while (file.available()) {
        char c = file.read();
        Keyboard.write(c); // Emula la pulsación de la tecla correspondiente
        
        // Un pequeño delay opcional para no saturar el buffer del receptor
        delay(2); 
      }
      file.close();
      
      // Enviamos un Enter al finalizar
      Keyboard.press(KEY_RETURN);
      Keyboard.releaseAll();
      
      Serial.println("Envío completado.");
    } else {
      Serial.println("Error: No se pudo encontrar 'texto.txt'.");
    }
    
    // Debounce para evitar múltiples envíos por una sola pulsación
    delay(1000); 
  }
}