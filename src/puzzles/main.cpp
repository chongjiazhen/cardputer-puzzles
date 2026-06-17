#include "cardputer_hw.h"

void setup() {
  cardputer::begin();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.print("puzzles boot");
  Serial.begin(115200);
  Serial.println("puzzles boot ok");
}

void loop() {
  cardputer::update();
  delay(16);
}
