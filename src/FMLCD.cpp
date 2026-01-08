/**
 * ESP-32 Radio
 * 
 * FMLCD.cpp
 * 
 * These files contain all user interface related functions.
 * Each class' init() and tick() methods can be modified to change the layout of the ui
 */


#include "FMLCD.h"

const char *pty_values[] = {
  "None",
  "News",
  "Current Affairs",
  "Information",
  "Sport",
  "Education",
  "Drama",
  "Culture",
  "Science",
  "Varied",
  "Pop Music",
  "Rock Music",
  "Easy Listening",
  "Light Classical",
  "Classical",
  "Other Music",
  "Weather",
  "Finance",
  "Children's",
  "Social Affairs",
  "Religion",
  "Phone-in",
  "Travel",
  "Leisure",
  "Jazz Music",
  "Country Music",
  "National Music",
  "Oldies Music",
  "Folk Music",
  "Documentary",
  "Alarm Test",
  "Alarm"
};



Screen::Screen(LiquidCrystal_I2C* lcd, FMState *state){
  this->lcd = lcd;

  this->state = state;
}

void Screen::refreshOnNextDraw(){
  this->update = true;
}

bool Screen::needsUpdate(){
  return this->update;
}

void Screen::hasUpdatedScreen(){
  this->update = false;
}

void Screen::clearLine(int y){
  this->lcd->setCursor(0, y);
  for (int i = 0; i < LCD_WIDTH; i++)
    this->lcd->print(" ");
}

const int Screen::getType(){
  return this->screenType;
}

void Screen::init() {}
void Screen::tick() {}

void Screen::moveDown() {}
void Screen::moveUp() {}
int Screen::select() {}



MainScreen::MainScreen(LiquidCrystal_I2C* lcd, FMState *state) : Screen(lcd, state){
  this->screenType = MAIN_SCREEN;
}

void MainScreen::refreshOnNextDraw() {
  Screen::refreshOnNextDraw();

  this->currentWindow = LCD_WIDTH;
  this->wait = 0;
}

void MainScreen::init(){
  this->lcd->clear();
  this->lcd->setCursor(1, 0);

  this->lcd->print(((float)this->state->frequency / 100.0f));
  this->lcd->print(" MHz");

  if(this->state->hasStereo){
    this->lcd->setCursor(LCD_WIDTH - 4, 0);
    this->lcd->print("ST");
  }

  this->currentWindow = LCD_WIDTH;
  this->wait = 0;

  this->refreshOnNextDraw();
}

void MainScreen::tick() {
  if(this->needsUpdate()){
    this->lcd->clear();
    this->lcd->setCursor(1, 0);

    // display the frequency (only if the lcd has more than 2 lines, or there is no RDS.)
    if (LCD_HEIGHT > 2 || !this->state->hasRds){
      this->lcd->print(((float)this->state->frequency / 100.0f));
      this->lcd->print(" MHz");
    }

    // display the Stereo indicator
    if(this->state->hasStereo){
      this->lcd->setCursor(LCD_WIDTH - 5, 0);
      this->lcd->print("ST");
    }

    this->lcd->setCursor(LCD_WIDTH - 2, 0);
    this->lcd->write(byte(0));
    if (this->state->signalStrength > 0 && this->state->signalStrength <= 3)
      this->lcd->write(byte(this->state->signalStrength));

    if(this->state->hasRds){
      // draw the program service text (either below the frequency, or if the display only has 2 lines, where the frequency is usually displayed.)
      this->lcd->setCursor(0, LCD_HEIGHT > 2 ? 1 : 0);
      this->lcd->print(this->state->ps);

      // draw the RDS icon on the same line as the program service label.
      if (LCD_HEIGHT > 2){
        this->lcd->setCursor(LCD_WIDTH - 3, LCD_HEIGHT > 2 ? 1 : 0);
        this->lcd->print("RDS");

        // draw the PTY only if the display is a 4 liner.
        if (LCD_HEIGHT > 3) {
          // draw the PTY text
          this->lcd->setCursor(0, 2);
          if (this->state->pty != -1)
            this->lcd->print(pty_values[this->state->pty]);
          else this->lcd->print("[No PTY]");
        }
      }

      // display the RT on the lower line of the LCD.
      if (this->state->rt.length() <= LCD_WIDTH){
        this->lcd->setCursor(0, LCD_HEIGHT > 3 ? 3 : LCD_HEIGHT > 2 ? 2 : 1);
        this->lcd->print(this->state->rt.substring(currentWindow-LCD_WIDTH, currentWindow));
      }
    }else {
      // display a no rds available message when there is no RDS.
      this->lcd->setCursor(LCD_WIDTH/2 - 3, LCD_HEIGHT > 3 ? 2 : 1);
      this->lcd->print("No RDS");
    }

    // draw signal strength meter

    this->hasUpdatedScreen();
  }

  if (this->state->hasRds && this->state->rt.length() > LCD_WIDTH){
    this->lcd->setCursor(0, LCD_HEIGHT > 3 ? 3 : LCD_HEIGHT > 2 ? 2 : 1);

    if (++this->currentWindow > this->state->rt.length()){
      if (++this->wait > SCROLL_WAITING_TIME){
        this->currentWindow = LCD_WIDTH;
        this->wait = 0;
      }
      else this->currentWindow--;
    }
    if (this->currentWindow == LCD_WIDTH + 1){
      if (++this->wait < SCROLL_WAITING_TIME){
        --this->currentWindow;
      }else this->wait = 0;
    }

    this->lcd->print(this->state->rt.substring(currentWindow - LCD_WIDTH, currentWindow));
  }
}




/**
 * @param lcd A pointer to the main LCD
 * @param state the size of the list. @todo infer this from the sList provided.
 * @param list a list of options
 */
StationListScreen::StationListScreen(LiquidCrystal_I2C* lcd, FMState *state, FMStationList *list) : Screen(lcd, state){
  this->list = list;
  this->selected = 0;
  this->screenType = LIST_SCREEN;
}

/**
 * Moves down the list to the next element - or goes back to the top if the bottom has been reached.
 * @todo instantanously update the screen so that it feels responsive.
 */
void StationListScreen::moveDown(){
  this->selected++;
  if (this->selected > this->list->size() - 1)
    this->selected = 0;
  this->refreshOnNextDraw();
}

/**
 * Moves up the list to the previous element - or goes back to the bottom if the top has been reached.
 * @todo instantanously update the screen so that it feels responsive.
 */
void StationListScreen::moveUp() {
  this->selected--;
  if (this->selected < 0)
    this->selected = this->list->size()-1;
  this->refreshOnNextDraw();
}

/**
 * Returns the currently selected component.
 */
int StationListScreen::select(){
  return this->selected;
}

/** 
 * Part of the screen interface; is called when the screen is first drawn.
 */
void StationListScreen::init(){
  this->selected = 0;
  
  this->refreshOnNextDraw();
}

/**
 * Called in the main loop to update the screen if necessary.
 */
void StationListScreen::tick(){
  if (this->needsUpdate()){

    this->lcd->clear();
    this->lcd->setCursor(0, 0);
    this->lcd->print("Stations");

    this->lcd->setCursor(LCD_WIDTH-5, 0);
    this->lcd->printf("%02d/%02d", this->selected+1, this->list->size());

    if (LCD_HEIGHT == 4){
      // checks to see if the selected component is pointing to the beginning/end and adjust the > < selectors, else
      // set the selected line on the lcd to the third line.
      int currentY = (this->selected == 0) ? 1 : (this->selected == this->list->size()-1) ? 3 : 2;
      this->lcd->setCursor(0, currentY);
      this->lcd->print(">");
      this->lcd->setCursor(LCD_WIDTH-1, currentY);
      this->lcd->print("<");

      int index = this->selected - 1;
      if (index < 0)
        index = 0;
      else if (index+2 > this->list->size()-1)
        index = this->list->size() - 3;

      if (LCD_WIDTH >= 20){
        for (int i = 0; i < 3; i++){
          this->lcd->setCursor(2, i+1);
          FMStationItem *item = this->list->get(index + i);
          this->lcd->printf("% 3.1f: %s", ((float)item->getFrequency() / 100.0f), item->getRdsPS());
        }
      }else{
        for (int i = 0; i < 3; i++){
          this->lcd->setCursor(2, i+1);
          FMStationItem *item = this->list->get(index + i);
          if (item->hasRds()) this->lcd->print(item->getRdsPS()); 
          else this->lcd->printf("% 3.1f MHz", ((float)item->getFrequency() / 100.0f));
        }
      }
    }else{
      this->lcd->setCursor(0, LCD_HEIGHT - 1);
      this->lcd->print("<");
      this->lcd->setCursor(LCD_WIDTH-1, LCD_HEIGHT - 1);
      this->lcd->print(">");

      int index = this->selected;
      if (index < 0)
        index = this->list->size() - 1;
      else if (index >= this->list->size())
        index = 0;
      
      FMStationItem *item = this->list->get(index);
      this->lcd->setCursor(2, LCD_HEIGHT - 1);
      
      if (LCD_WIDTH >= 20){
          this->lcd->printf("% 3.1f: %s", ((float)item->getFrequency() / 100.0f), item->getRdsPS());
      }else {
          if (item->hasRds()) this->lcd->print(item->getRdsPS()); 
          else this->lcd->printf("% 3.1f MHz", ((float)item->getFrequency() / 100.0f));
      }
    }

    

    this->hasUpdatedScreen();
  }
}



/**
 * @param lcd A pointer to the main LCD
 * @param freq the current frequency expressed as a uint16 - 9990 = 99.9 MHz.
 * @param initVol the current volume of the radio
 */
VolumeScreen::VolumeScreen(LiquidCrystal_I2C* lcd, FMState *state) : MainScreen(lcd, state) {
    this->screenType = VOL_SCREEN;
}

/**
 * Increments the volume by 1 up to a maximum of 15. (Maximum for the SI470x library)
 */
void VolumeScreen::moveUp() {
    this->state->incrementVolume();

    this->refreshOnNextDraw();
}

/**
 * Decrements the volume by 1 down to 0.
 */
void VolumeScreen::moveDown(){
  this->state->decrementVolume();
      
  this->refreshOnNextDraw();
}

/**
 * Returns the current volume that is being displayed.
 */
int VolumeScreen::select(){
    return this->state->volume;
}

/** 
 * Part of the screen interface; is called when the screen is first drawn.
 */
void VolumeScreen::init(){
  MainScreen::init();
  
  this->lcd->setCursor(LCD_WIDTH/2 - 3, 1);
  this->lcd->print("Volume");
  this->lcd->setCursor(LCD_WIDTH/2 - 1, 2);
  this->lcd->printf("%02d", this->state->volume);
}

/**
 * Called in the main loop to update the screen if necessary.
 */
void VolumeScreen::tick(){
  if(this->needsUpdate()){
    this->lcd->clear();
    this->lcd->setCursor(0, 0);

    if (LCD_HEIGHT > 2){

      if (!this->state->hasRds || this->state->ps.isEmpty() || this->state->ps.equals("[No PS]")){
        this->lcd->setCursor(1, 0);
        this->lcd->print(((float)this->state->frequency / 100.0f));
        this->lcd->print(" MHz");
      }else{
        this->lcd->print(this->state->ps);
      }

      // display the Stereo indicator
      if(this->state->hasStereo){
        this->lcd->setCursor(LCD_WIDTH - 5, 0);
        this->lcd->print("ST");
      }

      this->lcd->setCursor(LCD_WIDTH - 2, 0);
      this->lcd->write(byte(0));
      if (this->state->signalStrength > 0 && this->state->signalStrength <= 3)
        this->lcd->write(byte(this->state->signalStrength));
      
      this->lcd->setCursor(LCD_WIDTH/2 - 3, 1);
      this->lcd->print("Volume");
      this->lcd->setCursor(LCD_WIDTH/2 - 8, 2);
      for (int i = 0; i < this->state->volume; i++)
        this->lcd->write(byte(5));
      for (int i = this->state->volume; i < 16; i++)
        this->lcd->write(byte(this->state->volume == 15 ? 5 : 4));

    }else if (LCD_HEIGHT == 2){
      this->lcd->setCursor(LCD_WIDTH/2 - 3, 0);
      this->lcd->print("Volume");
      this->lcd->setCursor(LCD_WIDTH/2 - 8, 1);
      for (int i = 0; i < this->state->volume; i++)
        this->lcd->write(byte(5));
      for (int i = this->state->volume; i < 16; i++)
        this->lcd->write(byte(this->state->volume == 15 ? 5 : 4));
    }

    this->hasUpdatedScreen();
  }
}