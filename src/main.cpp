/**
 * ESP-32 Radio
 * 
 * main.cpp
 * 
 * This is a FM radio project for the ESP-32, 2004 LCD and SI4703 RDS FM chip.
 * 
 * All button inputs are connected via Ground.
 * 
 * Default Pinouts:
 *  PIN 2   - SI4703 GPIO
 *  PIN 4   - SI4703 RST PIN
 *  PIN 5   - Menu/Select button
 *  PIN 15  - SI4703 GPIO
 *  PIN 16  - Menu/Freq Up button
 *  PIN 17  - Menu/Freq Down button
 *  PIN 18  - Volume Up button
 *  PIN 19  - Volume Down button
 *  PIN 21  - I2C Data - to both LCD SDA and SI4703 SDA
 *  PIN 22  - I2C Clock - to both LCD SCL and SI4703 SCL
 * 
 * The LCD must be a 2004 by default. A 1602 LCD can be adapted by changing the LCD_WIDTH and LCD_HEIGHT constants in
 * the FMLCD.h header file which will automatically adjust the content displayed on the main screen and volume screen.
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "FMLCD.h"

/** The frequency/menu up button pin */
#define MENU_UP_PIN     16
/** The frequency/menu down button pin */
#define MENU_DOWN_PIN   17
/** The select/menu button pin */
#define MENU_PIN         5
/** The volume up button pin */
#define VOLUME_UP_PIN   18
/** The volume down button pin */
#define VOLUME_DN_PIN   19


/* SI4703 Control pins */

#define SI4703_RST_PIN    4
#define SI4703_RDS_PIN    2
#define SI4703_SEEK_PIN  15


/* Screen timing stuff */
#define UPDATE_DELAY       100
#define TMP_SCR_SHOWTIME   35
#define DEBOUNCE_DELAY  150


LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);

FMState *state = new FMState(FM_DEFAULT_FREQ, 6);
FMStationList *list = new FMStationList();

Screen *screen = new MainScreen(&lcd, state);

// Custom signal characters. 0-2 are the bar characters, signal_char is the antenna char
const char signal_0[] = {B00000,B00000,B00000,B00000,B00000,B00000,B10000,B10000};
const char signal_1[] = {B00000,B00000,B00000,B00100,B00100,B00100,B10100,B10100};
const char signal_2[] = {B00001,B00001,B00001,B00101,B00101,B00101,B10101,B10101};
const char signal_char[] = {B00000,B11111,B10001,B01010,B00100,B00100,B00100,B00100};

const char meter_empty[] = {B00000,B11111,B10001,B10001,B10001,B10001,B10001,B11111};
const char meter_full[] = {B00000,B11111,B11111,B11111,B11111,B11111,B11111,B11111};

// used to keep track of screen cycles.
// screen objects update every third loop. This is so that other functions such as checking for button presses
// are still able to be done.
int ticker = 0;

namespace ESPRadio {

    volatile unsigned long lastPressedTime = 0;
    volatile bool buttonPressed = false;

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
              screen->init();
          }
    }
    void pollButton(uint8_t pin, int level, void(*callback)()){
      if (digitalRead(pin) == level){
        callback();
        ESPRadio::buttonPressed = true;
      }
    }

    bool debounce(){
      unsigned long now = millis();

      if (now - ESPRadio::lastPressedTime > DEBOUNCE_DELAY){
        ESPRadio::lastPressedTime = now;
        return true;
      }

      return false;
    }
};

void menu_up(){
  if (ESPRadio::debounce()){
    if (screen->getType() != LIST_SCREEN){
      state->incrementFrequency();
    }else screen->moveUp();
  }
}

void menu_down(){
  if (ESPRadio::debounce()){
    if (screen->getType() != LIST_SCREEN){
      state->decrementFrequency();
    }else screen->moveDown();
  }
}

void menu_select(){
  if (ESPRadio::debounce()){
    if (screen->getType() != LIST_SCREEN){  // load the station list screen
      ESPRadio::setScreen(LIST_SCREEN);
      ticker = 0;
    }else if (screen->getType() == LIST_SCREEN){  // get the selected element from the list screen and tune
      FMStationItem *selected = list->get(screen->select());
      state->reset();
      state->setFrequency(selected->getFrequency());
      state->setRDS(selected->hasRds());
      state->setRdsPS(selected->getRdsPS());
      ESPRadio::setScreen(MAIN_SCREEN);

        // SI4703 tune
    }
    screen->refreshOnNextDraw();
  }
}

void vol_up(){
  if (ESPRadio::debounce()){
    if (screen->getType() != VOL_SCREEN) 
      ESPRadio::setScreen(VOL_SCREEN);
    screen->moveUp();
    ticker = 0;
  }
}

void vol_down(){
  if (ESPRadio::debounce()){
    if (screen->getType() != VOL_SCREEN) 
      ESPRadio::setScreen(VOL_SCREEN);
    screen->moveDown();
    ticker = 0;
  }
}

void setup() {
  Serial.begin(115200);
  // initiate button pins. These should be connected between the pin and ground.
  pinMode(MENU_UP_PIN, INPUT_PULLUP);
  pinMode(MENU_DOWN_PIN, INPUT_PULLUP);
  pinMode(MENU_PIN, INPUT_PULLUP);
  pinMode(VOLUME_UP_PIN, INPUT_PULLUP);
  pinMode(VOLUME_DN_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.createChar(0, signal_char);
  lcd.createChar(1, signal_0);
  lcd.createChar(2, signal_1);
  lcd.createChar(3, signal_2);
  lcd.createChar(4, meter_empty);
  lcd.createChar(5, meter_full);

  lcd.setCursor(LCD_WIDTH / 2 - 6, 1);
  lcd.print("ESP-32 Radio");


  /** @todo Remove all debug code here */
  state->setRDS(true);
  state->setStereo(true);
  state->setRdsPS("Test");
  state->setRdsRT("This is test radiotext for the ESP radio");
  state->setSignalStrength(2);

  screen->refreshOnNextDraw();

  // add test stations
  list->add(new FMStationItem(8860, "TEST FM"));
  list->add(new FMStationItem(8900, "NATIONAL"));
  list->add(new FMStationItem(8940, "Electro"));
  list->add(new FMStationItem(9000, "90 RADIO"));
  list->add(new FMStationItem(9990, "CLASSIC"));
  list->add(new FMStationItem(10450, " BEATS "));
  /** end of debug */

  delay(2000);
  // load the main screen
  screen->init();
}

void loop() {
  delay(UPDATE_DELAY);
  ESPRadio::buttonPressed = false;

  // check buttons
  ESPRadio::pollButton(MENU_UP_PIN, LOW, menu_up);
  ESPRadio::pollButton(MENU_DOWN_PIN, LOW, menu_down);
  ESPRadio::pollButton(MENU_PIN, LOW, menu_select);
  ESPRadio::pollButton(VOLUME_UP_PIN, LOW, vol_up);
  ESPRadio::pollButton(VOLUME_DN_PIN, LOW, vol_down);

  // check for RDS data from SI4703 chip
  if (ticker %26 == 0){
    switch(state->getFrequency()){
      case 10450:
        state->setRdsPTY(15);
        state->setRdsPS(" BEATS ");
        state->setStereo(true);
        state->setRdsRT("BEATS RADIO 104.5 - The Hottest Beats are on Beats Radio");
        state->setSignalStrength(2);
        break;
      case 9990:
        state->setRdsPTY(14);
        state->setRdsPS("CLASSIC");
        state->setRdsRT("Home of Mozart - 99.9 Classic FM");
        state->setSignalStrength(1);
        break;
      case 9000:
        state->setRdsPTY(12);
        state->setRdsPS("90 RADIO");
        state->setStereo(true);
        state->setRdsRT("Your Home of 90's");
        state->setSignalStrength(3);
        break;
      case 8940:
        state->setRdsPTY(15);
        state->setRdsPS("Electro");
        state->setStereo(true);
        state->setRdsRT("Love Electronic Music");
        state->setSignalStrength(3);
        break;
      case 8900:
        state->setRdsPTY(2);
        state->setRdsPS("NATIONAL");
        state->setSignalStrength(1);
        break;
      case 8860:
        state->setRdsPTY(0);
        state->setRdsPS("TEST FM");
        state->setStereo(true);
        state->setRdsRT("This is test radiotext for the ESP radio");
        state->setSignalStrength(2);
        break;
    }
  }


  // check if the screen needs updating, and update it.
  if (ticker%3 == 0 || ESPRadio::buttonPressed) 
    screen->tick();

  // change the screen back to the main screen if it is on another screen (except the list screen) and
  // this has been active for x amount of time.
  if(++ticker >= TMP_SCR_SHOWTIME && screen->getType() != LIST_SCREEN){
    ticker = 0;
    if (screen->getType() != MAIN_SCREEN) 
      ESPRadio::setScreen(MAIN_SCREEN);
  }
}