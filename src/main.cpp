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
#define UPDATE_DELAY      100
#define TMP_SCR_SHOWTIME   30

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);

FMState *state = new FMState(9990, 6);
FMStationList *list = new FMStationList();

Screen *screen = new MainScreen(&lcd, state);

// Custom signal characters. 0-2 are the bar characters, signal_char is the antenna char
const char signal_0[] = {B00000,B00000,B00000,B00000,B00000,B00000,B10000,B10000};
const char signal_1[] = {B00000,B00000,B00000,B00100,B00100,B00100,B10100,B10100};
const char signal_2[] = {B00001,B00001,B00001,B00101,B00101,B00101,B10101,B10101};
const char signal_char[] = {B00000,B11111,B10001,B01010,B00100,B00100,B00100,B00100};

// used to keep track of screen cycles.
// screen objects update every third loop. This is so that other functions such as checking for button presses
// are still able to be done.
int ticker = 0;

/**
 * Changes the shown screen based on the screen id. These are found in the FMLCD.h header file
 * @param screenid The screen to load.
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
  }
}

void setup() {
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


  /** @todo Remove all debug code here */
  screen->setRDS(true);
  screen->setStereo(true);
  screen->setRdsPS("Test");
  screen->setRdsRT("This is test radiotext for the ESP radio");
  screen->setSignalStrength(2);

  // add test stations
  list->add(new FMStationItem(8860, "Station1"));
  list->add(new FMStationItem(8900, "Station2"));
  list->add(new FMStationItem(8940, "Station3"));
  /** end of debug */

  // load the main screen
  screen->init();
}

void loop() {
  delay(UPDATE_DELAY);
  bool buttonPressed = false;

  // check if the menu/frequency up button is being pushed
  if(digitalRead(MENU_UP_PIN) == LOW){
    // change the frequency if the list screen isn't loaded
    if (screen->getType() != LIST_SCREEN){
      state->frequency += 10;
      if (state->frequency > 10800)
        state->frequency = 8750;

      screen->setFrequency(state->frequency);

      // set frequency on SI4703 chip

    }else screen->moveUp(); // if list screen is loaded; move the selected element up one.

    buttonPressed = true;
  }
  // check if the menu/frequency down button is being pushed
  else if (digitalRead(MENU_DOWN_PIN) == LOW){
    // change the frequency if the list screen isn't loaded
    if (screen->getType() != LIST_SCREEN){
      state->frequency -= 10;
      if (state->frequency < 8750)
        state->frequency = 10800;
      
      screen->setFrequency(state->frequency);

      // set frequency on SI4703 chip
    }else screen->moveDown(); // if list screen is loaded; move the selected element down one.

    buttonPressed = true;
  }
  // check if the menu/select button is being pushed
  else if (digitalRead(MENU_PIN) == LOW){
    if (screen->getType() != LIST_SCREEN){  // load the station list screen
      setScreen(LIST_SCREEN);
      ticker = 0;
    }else if (screen->getType() == LIST_SCREEN){  // get the selected element from the list screen and tune
      FMStationItem *selected = list->get(screen->select());
      state->reset();
      state->frequency = selected->getFrequency();
      state->hasRds = selected->hasRds();
      state->ps = selected->getRdsPS();
      setScreen(MAIN_SCREEN);

      // SI4703 tune
    }
    buttonPressed = true;
  }

  // volume stuff
  if (digitalRead(VOLUME_UP_PIN) == LOW){
    // set volume on SI4703 chip
    if (screen->getType() != VOL_SCREEN) setScreen(VOL_SCREEN);
    screen->moveUp();
    ticker = 0;
    buttonPressed = true;
  }else if (digitalRead(VOLUME_DN_PIN) == LOW){
    if (screen->getType() != VOL_SCREEN) setScreen(VOL_SCREEN);
    screen->moveDown();
    ticker = 0;
    buttonPressed = true;
  }


  // check for RDS data from SI4703 chip


  // check if the screen needs updating, and update it.
  if (ticker%3 == 0 || buttonPressed) screen->tick();

  // change the screen back to the main screen if it is on another screen (except the list screen) and
  // this has been active for x amount of time.
  if(ticker++ > TMP_SCR_SHOWTIME && screen->getType() != LIST_SCREEN){
    ticker = 0;
    if (screen->getType() != MAIN_SCREEN) setScreen(MAIN_SCREEN);
  }

  if (buttonPressed)
    delay(50);
}