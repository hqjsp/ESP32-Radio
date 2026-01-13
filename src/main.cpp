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
#include "SI470X.h"

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

#define SI4703_RST_PIN   15
#define SI4703_SDA_PIN   19
#define SI4703_SCL_PIN   18


/* Screen timing stuff */
#define UPDATE_DELAY       80
#define TMP_SCR_SHOWTIME   35
#define DEBOUNCE_DELAY  150

SI470X si470x;
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

    /**
     * Loads a new screen based on the screen id. These are found in FMLCD.h.
     * The default values are:
     *  - Main Screen = 0x13
     *  - Station List Screen = 0x14
     *  - Volume Screen = 0x15.
     * These can be any arbitary value - but must be defined in FMLCD.h.
     * @param screenid the screen id to load.
     */
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

    /**
     * Debounce wrapper for a button press. Returns true if the button press should be registered, else returns false.
     */
    bool debounce(){
      unsigned long now = millis();

      if (now - ESPRadio::lastPressedTime > DEBOUNCE_DELAY){
        ESPRadio::lastPressedTime = now;
        return true;
      }

      return false;
    }

    /**
     * Checks to see if the specified pin is reading at the specified level, and calls the `callback()` function if this is true.
     * @param pin the GPIO pin to check.
     * @param level the level the GPIO pin should read to call the function
     * @param callback the function to call when the GPIO pin is active.
     */
    void pollButton(uint8_t pin, int level, void(*callback)()){
      if (digitalRead(pin) == level){
        
        if (ESPRadio::debounce()){
          callback();
        }
        ESPRadio::buttonPressed = true;
      }
    }
};

void menu_up(){
  if (screen->getType() != LIST_SCREEN){
    state->incrementFrequency();

    si470x.clearRdsBuffer();
    si470x.setFrequency(state->getFrequency());
  }else screen->moveUp();
}

void menu_down(){
  if (screen->getType() != LIST_SCREEN){
    state->decrementFrequency();

    si470x.clearRdsBuffer();
    si470x.setFrequency(state->getFrequency());
  }else screen->moveDown();
}

void menu_select(){
  if (screen->getType() != LIST_SCREEN){  // load the station list screen
    ESPRadio::setScreen(LIST_SCREEN);
        ticker = 0;
  }else if (screen->getType() == LIST_SCREEN){  // get the selected element from the list screen and tune
    FMStationItem *selected = list->get(screen->select());
    state->reset();
    state->setFrequency(selected->getFrequency());
    if (selected->hasRds()){
      state->setRDS(selected->hasRds());
      state->setRdsPS(selected->getRdsPS());
    }
    
    ESPRadio::setScreen(MAIN_SCREEN);

    si470x.clearRdsBuffer();
    si470x.setFrequency(state->getFrequency());
    // SI4703 tune
  }
  screen->refreshOnNextDraw();
}

void vol_up(){
  if (screen->getType() != VOL_SCREEN) 
    ESPRadio::setScreen(VOL_SCREEN);
  screen->moveUp();

  si470x.setVolume(state->getVolume());
  ticker = 0;
}

void vol_down(){
  if (screen->getType() != VOL_SCREEN) 
    ESPRadio::setScreen(VOL_SCREEN);
  screen->moveDown();

  si470x.setVolume(state->getVolume());
  ticker = 0;
}

void seekingFunction(){
  state->setFrequency(si470x.getFrequency());
}

void findChannels(){
  si470x.setFrequency(8750);
  si470x.setMute(true);
  list->removeAll();
  while(si470x.getRealFrequency() < 10790){
    si470x.seek(1, 1);
    lcd.setCursor(LCD_WIDTH / 2 - 5, 1);
    /*if (si470x.getRdsReady()){
      char* ps = si470x.getRdsStationName();
      if (ps != NULL){
        list->add(new FMStationItem(si470x.getRealFrequency()));
        lcd.printf("%d channels", list->size());
        continue;
      }
    }*/
    
    list->add(new FMStationItem(si470x.getRealFrequency()));
    lcd.printf("%d channels", list->size());

    #if LCD_HEIGHT > 2
    lcd.setCursor(LCD_WIDTH / 2 - 5, 2);
    lcd.printf("%.2f MHz", ((float)si470x.getRealFrequency() / 100.0f));
    #endif
  }
  si470x.setMute(false);
  if (list->size() > 0)
    si470x.setFrequency(list->get(0)->getFrequency());
  else si470x.setFrequency(8750);

  state->setFrequency(si470x.getRealFrequency());
}

void setup() {
  Serial.begin(115200);
  Serial.write("Starting up");
  // initiate button pins. These should be connected between the pin and ground.
  /*pinMode(MENU_UP_PIN, INPUT_PULLUP);
  pinMode(MENU_DOWN_PIN, INPUT_PULLUP);
  pinMode(MENU_PIN, INPUT_PULLUP);
  pinMode(VOLUME_UP_PIN, INPUT_PULLUP);
  pinMode(VOLUME_DN_PIN, INPUT_PULLUP);*/

  si470x.setup(SI4703_RST_PIN, SI4703_SDA_PIN, SI4703_SCL_PIN, OSCILLATOR_TYPE_CRYSTAL);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, signal_char);
  lcd.createChar(1, signal_0);
  lcd.createChar(2, signal_1);
  lcd.createChar(3, signal_2);
  lcd.createChar(4, meter_empty);
  lcd.createChar(5, meter_full);
  lcd.clear();

  lcd.setCursor(LCD_WIDTH / 2 - 6, 0);
  lcd.print("ESP-32 Radio");

  si470x.setRDS(true);
  si470x.setRdsMode(1);
  si470x.setMono(false);
  si470x.setAgc(true);
  si470x.setBlendLevelAdjustment(2);
  si470x.setBand(0);
  si470x.setSeekThreshold(16);

  si470x.setFrequency(state->getFrequency());
  si470x.setVolume(state->getVolume());

  lcd.setCursor(LCD_WIDTH / 2 - 5, 1);
  lcd.println("0 channels");
  findChannels();

  // load the main screen
  screen->init();
}

void loop() {
  delay(UPDATE_DELAY);
  ESPRadio::buttonPressed = false;
  
  // check buttons
  /*ESPRadio::pollButton(MENU_UP_PIN, LOW, menu_up);
  ESPRadio::pollButton(MENU_DOWN_PIN, LOW, menu_down);
  ESPRadio::pollButton(MENU_PIN, LOW, menu_select);
  ESPRadio::pollButton(VOLUME_UP_PIN, LOW, vol_up);
  ESPRadio::pollButton(VOLUME_DN_PIN, LOW, vol_down);*/

  if (Serial.available() > 0){
    int command = Serial.read();

    if (command == 65){
      menu_up();
    }else if (command == 66){
      menu_down();
    }else if (command == 61){
      vol_up();
    }else if (command == 45){
      vol_down();
    }else if (command == 10){
      menu_select();
    }
  }

  // get info from SI4703 chip
  int signalStrength = si470x.getRssi();
  state->setRDSIndicator(si470x.getRdsSync());
  state->setStereo(si470x.isStereo());

  if (signalStrength < 9){
    state->setSignalStrength(0);
  }else if (signalStrength < 20){
    state->setSignalStrength(1);
  }else if (signalStrength < 32){
    state->setSignalStrength(2);
  }else {
    state->setSignalStrength(3);
  }

  // check for RDS data.
  if (si470x.getRdsReady()){
    char* ps = si470x.getRdsStationName();
    if (ps != NULL)
      state->setRdsPS(ps);
    
    char* rt;

    if (si470x.getRdsFlagAB() == 0){
      rt = si470x.getRdsText2A();
    }else rt = si470x.getRdsText2B();
    if (rt != NULL){
      state->setRdsRT(rt);
    }
    
    state->setRdsPTY(si470x.getRdsProgramType());
  }
  

  // check if the screen needs updating, and update it.
  if (ticker%4 == 0 || ESPRadio::buttonPressed) {
    screen->tick();

    // update the item in the station list if there was RDS received.
    if (state->hasRDS()){
      FMStationItem *fmitem = list->get(FMStationItem(state->getFrequency()));
      if (fmitem != nullptr)
        fmitem->setRdsPS(state->getRdsPS().c_str());
    }
  }

  // change the screen back to the main screen if it is on another screen (except the list screen) and
  // this has been active for x amount of time.
  if(++ticker >= TMP_SCR_SHOWTIME && screen->getType() != LIST_SCREEN){
    ticker = 0;
    if (screen->getType() != MAIN_SCREEN) 
      ESPRadio::setScreen(MAIN_SCREEN);
  }
}