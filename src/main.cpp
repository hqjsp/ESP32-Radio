#include <Arduino.h>

#include <LiquidCrystal_I2C.h>
#include "FMLCD.h"

#define MENU_UP_PIN     16
#define MENU_DOWN_PIN   17
#define MENU_PIN         5
#define VOLUME_UP_PIN   18
#define VOLUME_DN_PIN   19

#define SI4703_RST_PIN    4
#define SI4703_RDS_PIN    2
#define SI4703_SEEK_PIN  15

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);
Screen *screen = new MainScreen(&lcd, 9990);
Screen *currentScreen = screen;

uint16_t frequency = 9990;
int currentVolume = 4;
int currentSignal = 0;

const char signal_0[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B10000,
  B10000
};

const char signal_1[] = {
  B00000,
  B00000,
  B00000,
  B00100,
  B00100,
  B00100,
  B10100,
  B10100
};

const char signal_2[] = {
  B00001,
  B00001,
  B00001,
  B00101,
  B00101,
  B00101,
  B10101,
  B10101
};

const char signal_char[] = {
  B00000,
  B11111,
  B10001,
  B01010,
  B00100,
  B00100,
  B00100,
  B00100
};

int ticker = 0;

void setup() {
  pinMode(MENU_UP_PIN, INPUT_PULLUP);
  pinMode(MENU_DOWN_PIN, INPUT_PULLUP);
  pinMode(MENU_PIN, INPUT_PULLUP);
  pinMode(VOLUME_UP_PIN, INPUT_PULLUP);
  pinMode(VOLUME_DN_PIN, INPUT_PULLUP);
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("Hello World!");

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.createChar(0, signal_char);
  lcd.createChar(1, signal_0);
  lcd.createChar(2, signal_1);
  lcd.createChar(3, signal_2);

  screen->setRDS(true);
  screen->setStereo(true);
  screen->setRdsPS("Test");
  screen->setRdsRT("This is test radiotext for the ESP radio");
  screen->setSignalStrength(2);

  screen->init();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(75);

  if(digitalRead(MENU_UP_PIN) == LOW){
    frequency += 10;
    if (frequency > 10800)
      frequency = 8750;

    currentScreen->setFrequency(frequency);
    
    delay(25);

    // set frequency on SI4703 chip
  }
  if (digitalRead(MENU_DOWN_PIN) == LOW){
    frequency -= 10;
    if (frequency < 8750)
      frequency = 10800;
    currentScreen->setFrequency(frequency);

    delay(25);
    // set frequency on SI4703 chip
  }
  if (digitalRead(MENU_PIN) == LOW){
    currentSignal = (currentSignal+1)%4;
    currentScreen->setSignalStrength(currentSignal);
    delay(25);
  }
  if (digitalRead(VOLUME_UP_PIN) == LOW){
    // set volume on SI4703 chip
    if (currentScreen->getType() != VOL_SCREEN){
      Screen *scr = new VolumeScreen(&lcd, frequency, currentVolume);
      scr->moveData(screen);
      currentScreen = scr;
    }
    currentScreen->moveUp();
    currentVolume++;
    ticker = 0;
    delay(25);
  }
  if (digitalRead(VOLUME_DN_PIN) == LOW){
    if (currentScreen->getType() != VOL_SCREEN){
      Screen *scr = new VolumeScreen(&lcd, frequency, currentVolume);
      scr->moveData(screen);
      currentScreen = scr;
      ticker = 0;
    }
    currentScreen->moveDown();
    currentVolume--;
    ticker = 0;
    delay(25);
  }

  // check for RDS data from SI4703 chip


  if (ticker%3 == 0) currentScreen->tick();

  if(ticker++ > 40){
     ticker = 0;
    if (currentScreen->getType() != MAIN_SCREEN){
      screen->moveData(currentScreen);
      if (currentScreen->getType() == VOL_SCREEN)
        currentVolume = currentScreen->select();
      Screen *old = currentScreen;
      currentScreen = screen;
      delete old;
    }
  }
}
