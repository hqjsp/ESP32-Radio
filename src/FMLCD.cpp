/**
 * ESP-32 Radio
 * 
 * FMLCD.cpp
 * 
 * These files contain all user interface related functions.
 * Each class' init() and tick() methods can be modified to change the layout of the ui
 */


#include "FMLCD.h"

/**
 * RDS Program Type values. Taken from https://en.wikipedia.org/wiki/Radio_Data_System
 * Modify these if you would like to change what is displayed on the screen for each of these values.
 */
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

/**
 * Call this method when there is a substantial change to information on the display. This will tell the screen to redraw the whole
 * screen. This is to reduce LCD flicker as LCD's are slow to redraw - normally only essential information is drawn.
 */
void Screen::refreshOnNextDraw(){
  this->update = true;
}

/**
 * This should never be called outside of the children of this class. This will return true if the `refreshOnNextDraw()` method was called.
 */
bool Screen::needsUpdate(){
  return this->update;
}

/**
 * This should never be called outside of the children of this class. This will reset the update flag after the `refreshOnNextDraw()` method is called.
 * Use this when the whole screen has been updated.
 */
void Screen::hasUpdatedScreen(){
  this->update = false;
}

/**
 * @return Returns the type of screen. This is used like `instanceof` in other OOP languages to determine what screen is currently being displayed.
 */
const int Screen::getType(){
  return this->screenType;
}

void Screen::init() {}
void Screen::tick() {}

void Screen::moveDown() {}
void Screen::moveUp() {}
int Screen::select() {}

/**
 * This should be used in the `tick()` method. Clears a line by writing a line of space characters to the LCD. The idea of this method is to clear the screen
 * line by line to update it. This is used instead of `lcd.clear()` when `CLEAR_SCREEN_INSTEAD_OF_WRITE` is not defined.
 * @param line the line to clear on the LCD.
 */
void Screen::clearLine(int line){
  this->lcd->setCursor(0, line);
  this->lcd->print("                    ");
}


/**
 * This screen displays all information about the radio station, RDS, etc. This looks different depending on screen dimensions (defined in FMLCD.h).
 * 
 * - 20x4 LCD:
 *    `[ [FREQ] MHZ    ST SS]`
 *    `[[RDS PS]         RDS]`
 *    `[[RDS PTY]           ]`
 *    `[[RDS RT (< scrolls)]]`
 * - 16x3 LCD:
 *    `[ [FREQ] MHZ   SS]`
 *    `[[RDS PS]  ST RDS]`
 *    `[[RDS RT (< scr)]]`
 * - 16x2 LCD:
 *    `[[RDS PS]   ST SS]`
 *    `[[RDS RT (< scr)]]`
 * 
 * Where ST is the stereo indicator, RDS is the RDS indicator and SS is the signal strength indicator.
 */
MainScreen::MainScreen(LiquidCrystal_I2C* lcd, FMState *state) : Screen(lcd, state){
  this->screenType = MAIN_SCREEN;
}

/**
 * Call this method when there is a substantial change to information on the display. This will tell the screen to redraw the whole
 * screen. This is to reduce LCD flicker as LCD's are slow to redraw - normally only essential information is drawn.
 */
void MainScreen::refreshOnNextDraw() {
  Screen::refreshOnNextDraw();

  this->currentWindow = LCD_WIDTH;
  this->wait = 0;
}

void MainScreen::init(){
  this->lcd->clear();
  this->lcd->setCursor(1, 0);

  this->lcd->print(((float)this->state->getFrequency() / 100.0f));
  this->lcd->print(" MHz");

  if(this->state->hasStereo()){
    this->lcd->setCursor(LCD_WIDTH - 4, 0);
    this->lcd->print("ST");
  }

  this->currentWindow = LCD_WIDTH;
  this->wait = 0;

  this->refreshOnNextDraw();
}

void MainScreen::tick() {
  bool refreshWholeScreen = this->needsUpdate(); 

  
  if(refreshWholeScreen || this->state->getStateChanged()){
    #ifndef CLEAR_SCREEN_INSTEAD_OF_WRITE
    // clear the screen
    this->lcd->setCursor(LCD_WIDTH - 5, 0);
    this->lcd->print("   ");
    this->lcd->setCursor(LCD_WIDTH - 1, 0);
    this->lcd->print(" ");
    #else
    this->lcd->clear();
    #endif

    // print the frequency. If the LCD is a 2 line, then it prints the frequency IF the rds ps is not available.
    this->lcd->setCursor(1, 0);

    #if LCD_HEIGHT == 2
      if (!this->state->hasRDS()){
    #endif
      this->lcd->printf("%.2f MHz  ", ((float)this->state->getFrequency() / 100.0f));
    #if LCD_HEIGHT == 2
      }
    #endif
    
    #ifndef CLEAR_SCREEN_INSTEAD_OF_WRITE
    // clear the rest of the screen
    this->clearLine(1);

    #if LCD_HEIGHT > 2
    this->clearLine(2);
      #if LCD_HEIGHT > 3
    this->clearLine(3);
      #endif
    #endif
    #endif

    // Print the RDS information if it is available; else print 'No RDS'
    if (this->state->hasRDS()){
      #if LCD_HEIGHT > 2
        this->lcd->setCursor(0, 1);
      #else
        this->lcd->setCursor(0, 0);
      #endif
      this->lcd->print(this->state->getRdsPS());

      #if LCD_HEIGHT > 2
        // print the RDS indicator
        this->lcd->setCursor(LCD_WIDTH - 4, 1);
        this->lcd->printf("%04X", this->state->getRdsPI());

        // draw the PTY only if the display is a 4 liner.
        #if LCD_HEIGHT > 3
          this->lcd->setCursor(0, 2);
          this->lcd->print(this->state->getRdsPTY() != -1 ? pty_values[this->state->getRdsPTY()] : "[No PTY]");

          if (this->state->getRdsTP()){
            this->lcd->setCursor(LCD_WIDTH-2, 2);
            this->lcd->print("TP");
          }
        #endif
      #endif

      // print the radio text
      this->lcd->setCursor(0, LCD_HEIGHT - 1);
      if (this->state->getRdsRT().length() < LCD_WIDTH)
        currentWindow = LCD_WIDTH;
      this->lcd->print(this->state->getRdsRT().substring(currentWindow-LCD_WIDTH, currentWindow));
    }else{
      // display a no rds available message when there is no RDS.
      this->lcd->setCursor(LCD_WIDTH/2 - 3, LCD_HEIGHT > 3 ? 2 : 1);
      this->lcd->print("No RDS");
    }

    #ifdef CLEAR_SCREEN_INSTEAD_OF_WRITE
    this->state->allStatesChanged();
    #endif
    this->hasUpdatedScreen();
  }

  // update and draw the stereo indicator
  if (refreshWholeScreen ||  this->state->getStereoStateChanged()){
    #if LCD_WIDTH < 17 && LCD_HEIGHT > 2
    if(this->state->hasRDS()) this->lcd->setCursor(LCD_WIDTH - 6, 1);
    else this->lcd->setCursor(LCD_WIDTH - 4, 0);
    #else
    this->lcd->setCursor(LCD_WIDTH - 5, 0);
    #endif
    this->lcd->print(this->state->hasStereo() ? "ST" : "  ");
  }

  // update and draw the signal indicator
  if (refreshWholeScreen || this->state->getSignalStateChanged()){
    this->lcd->setCursor(LCD_WIDTH - 2, 0);
    this->lcd->write(byte(0));
    if (this->state->getSignalStrength() > 0) 
      this->lcd->write(byte(this->state->getSignalStrength()));

  }

  // scroll the radio text if it is longer than the width of the screen.
  if (this->state->hasRDS() && this->state->getRdsRT().length() > LCD_WIDTH){
    this->lcd->setCursor(0, LCD_HEIGHT > 3 ? 3 : LCD_HEIGHT > 2 ? 2 : 1);

    if (this->currentWindow > this->state->getRdsRT().length()){
      this->currentWindow = this->state->getRdsRT().length() < LCD_WIDTH ? LCD_WIDTH : this->state->getRdsRT().length();
    }

    if (this->currentWindow == LCD_WIDTH || this->currentWindow >= this->state->getRdsRT().length()){
      this->wait++;
      if (this->wait >= SCROLL_WAITING_TIME){
        if (this->currentWindow == LCD_WIDTH)
          this->currentWindow++;
        else this->currentWindow = LCD_WIDTH;
        this->wait = 0;
      }
    }else this->currentWindow++;

    this->lcd->print(this->state->getRdsRT().substring(currentWindow - LCD_WIDTH, currentWindow));
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

    #if LCD_HEIGHT == 4
      
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

      for (int i = 0; i < 3; i++){
        this->lcd->setCursor(1, i+1);
        FMStationItem *item = this->list->get(index + i);
        #if LCD_WIDTH >= 20 
          this->lcd->printf("% 6.1f  %s", ((float)item->getFrequency() / 100.0f), item->getRdsPS());
        #else 
          this->lcd->printf("% 6.1f MHz", ((float)item->getFrequency() / 100.0f));
        #endif
      }
    #else
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
      this->lcd->setCursor(1, LCD_HEIGHT - 1);
      
      #if LCD_WIDTH >= 20
          this->lcd->printf("% 6.1f  %s", ((float)item->getFrequency() / 100.0f), item->getRdsPS());
      #else
          if (item->hasRds()) this->lcd->printf(" %s", item->getRdsPS()); 
          else this->lcd->printf("% 6.1f MHz", ((float)item->getFrequency() / 100.0f));
      #endif
    #endif

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
    return this->state->getVolume();
}

/** 
 * Part of the screen interface; is called when the screen is first drawn.
 */
void VolumeScreen::init(){
  MainScreen::init();
  this->lcd->clear();
  
  this->lcd->setCursor(LCD_WIDTH/2 - 3, 1);
  this->lcd->print("Volume");
}

/**
 * Called in the main loop to update the screen if necessary.
 */
void VolumeScreen::tick(){
  if(this->needsUpdate() || this->state->getStateChanged()){
    this->lcd->setCursor(0, 0);

    #if LCD_HEIGHT > 2

      if (!this->state->hasRDS() || this->state->getRdsPS().isEmpty() || this->state->getRdsPS().equals("[No PS]")){
        this->lcd->setCursor(0, 0);
        this->lcd->printf(" %.2f MHz", ((float)this->state->getFrequency() / 100.0f));
      }else{
        this->lcd->printf("%- 8s    ", this->state->getRdsPS());
      }

      this->lcd->setCursor(LCD_WIDTH - 5, 0);
      // display the Stereo indicator
      if(this->state->hasStereo()){
        this->lcd->print("ST");
      }else
        this->lcd->print("  ");

      this->lcd->setCursor(LCD_WIDTH - 2, 0);
      this->lcd->write(byte(0));
      if (this->state->getSignalStrength() > 0 && this->state->getSignalStrength() <= 3)
        this->lcd->write(byte(this->state->getSignalStrength()));
      else this->lcd->print(" ");
      
      this->lcd->setCursor(LCD_WIDTH/2 - 3, 1);
      this->lcd->print("Volume");
      this->lcd->setCursor(LCD_WIDTH/2 - 8, 2);
      for (int i = 0; i < this->state->getVolume(); i++)
        this->lcd->write(byte(5));
      for (int i = this->state->getVolume(); i < 16; i++)
        this->lcd->write(byte(this->state->getVolume() == 15 ? 5 : 4));

    #elif LCD_HEIGHT == 2
      this->lcd->setCursor(LCD_WIDTH/2 - 3, 0);
      this->lcd->print("Volume");
      this->lcd->setCursor(LCD_WIDTH/2 - 8, 1);
      for (int i = 0; i < this->state->getVolume(); i++)
        this->lcd->write(byte(5));
      for (int i = this->state->getVolume(); i < 16; i++)
        this->lcd->write(byte(this->state->getVolume() == 15 ? 5 : 4));
    #endif

    this->hasUpdatedScreen();
  }
}