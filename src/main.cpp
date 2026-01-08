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
FMState *state = new FMState(9990, 6);
FMStationList *list = new FMStationList();

Screen *screen = new MainScreen(&lcd, state);

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

void setScreen(int screenid){
  Screen *newScr = nullptr;
  switch(screenid){
    case MAIN_SCREEN:
      newScr = new MainScreen(&lcd, state);
      break;
    case VOL_SCREEN:
      newScr = new VolumeScreen(&lcd, state);
      break;
    case LIST_SCREEN:
      newScr = new StationListScreen(&lcd, state, list);
      break;
  }
  if (newScr != nullptr){
    delete screen;
    screen = newScr;
  }
}

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

  // add test stations
  list->add(new FMStationItem(8860, "Station1"));
  list->add(new FMStationItem(8900, "Station2"));
  list->add(new FMStationItem(8940, "Station3"));
}

void setStreen(int screenid);

void loop() {
  // put your main code here, to run repeatedly:
  delay(75);

  if(digitalRead(MENU_UP_PIN) == LOW){
    if (screen->getType() != LIST_SCREEN){
      state->frequency += 10;
      if (state->frequency > 10800)
        state->frequency = 8750;

      screen->setFrequency(state->frequency);
    }else{
      screen->moveUp();
    }
    
    delay(25);

    // set frequency on SI4703 chip
  }
  if (digitalRead(MENU_DOWN_PIN) == LOW){
    if (screen->getType() != LIST_SCREEN){
      state->frequency -= 10;
      if (state->frequency < 8750)
        state->frequency = 10800;
      screen->setFrequency(state->frequency);
    }else{
      screen->moveDown();
    }

    delay(25);
    // set frequency on SI4703 chip
  }
  if (digitalRead(MENU_PIN) == LOW){
    //currentSignal = (currentSignal+1)%4;
    //screen->setSignalStrength(currentSignal);
    if (screen->getType() != LIST_SCREEN){
      setScreen(LIST_SCREEN);
      ticker = 0;
    }else if (screen->getType() == LIST_SCREEN){
      FMStationItem *selected = list->get(screen->select());
      state->reset();
      state->frequency = selected->getFrequency();
      state->hasRds = selected->hasRds();
      state->ps = selected->getRdsPS();
      setScreen(MAIN_SCREEN);
    }
    delay(25);
  }
  if (digitalRead(VOLUME_UP_PIN) == LOW){
    // set volume on SI4703 chip
    if (screen->getType() != VOL_SCREEN) setScreen(VOL_SCREEN);
    screen->moveUp();
    ticker = 0;
    delay(25);
  }
  if (digitalRead(VOLUME_DN_PIN) == LOW){
    if (screen->getType() != VOL_SCREEN) setScreen(VOL_SCREEN);
    screen->moveDown();
    ticker = 0;
    delay(25);
  }

  // check for RDS data from SI4703 chip


  if (ticker%3 == 0) screen->tick();

  if(ticker++ > 40 && screen->getType() != LIST_SCREEN){
    ticker = 0;
    if (screen->getType() != MAIN_SCREEN) setScreen(MAIN_SCREEN);
  }
}