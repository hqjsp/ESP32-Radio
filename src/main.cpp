#include <Arduino.h>

#include <LiquidCrystal_I2C.h>
#include "FMLCD.h"

//Application app = Application();

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);
VolumeScreen screen = VolumeScreen(&lcd, 9990, 6);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("Hello World!");

  lcd.init();
  lcd.backlight();
  lcd.clear();

  //screen.setRDS(true);
  //screen.setRdsPS("Test");
  //screen.setRdsRT("This is test radiotext for the ESP radio");

  screen.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(250);

  screen.moveUp();

  screen.tick();

}
