
#include "FMLCD.h"

Screen::Screen(LiquidCrystal_I2C* lcd){
  this->lcd = lcd;
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

MainScreen::MainScreen(LiquidCrystal_I2C* lcd, uint16_t currentFrequency) : Screen(lcd){
  this->lcd = lcd;

  this->frequency = currentFrequency;
  this->ps = "[No PS]";
  this->pty = 0;
  this->rt = "[No RadioText]";
}

void MainScreen::setFrequency(uint16_t currentFrequency){
  this->frequency = currentFrequency;
  this->refreshOnNextDraw();
}

void MainScreen::setRDS(bool rds){
  this->hasRds = rds;
  this->refreshOnNextDraw();
}

void MainScreen::setStereo(bool stereo){
  this->hasStereo = stereo;
  this->refreshOnNextDraw();
}

void MainScreen::setSignalStrength(int signalStrength){
  this->signalStrength = signalStrength;
  this->refreshOnNextDraw();
}

void MainScreen::setRdsPS(char * rds_ps){
  this->ps = String(rds_ps).substring(0, 8);
  this->hasRds = true;
  this->refreshOnNextDraw();
}

void MainScreen::setRdsRT(char * rds_rt){
  this->rt = String(rds_rt).substring(0, 128);
  this->currentWindow = LCD_WIDTH;
  this->wait = 0;
  this->hasRds = true;
  this->refreshOnNextDraw();
}

void MainScreen::setRdsPTY(int rds_pty){
  this->pty = rds_pty;
  this->hasRds = true;
  this->refreshOnNextDraw();
}

void MainScreen::init(){
  this->lcd->clear();
  this->lcd->setCursor(1, 0);

  this->lcd->print(((float)this->frequency / 100.0f));
  this->lcd->print(" MHz");

  if(this->hasStereo){
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
    if (LCD_HEIGHT > 2 || !this->hasRds){
      this->lcd->print(((float)this->frequency / 100.0f));
      this->lcd->print(" MHz");
    }

    // display the Stereo indicator
    if(this->hasStereo){
      this->lcd->setCursor(LCD_WIDTH - 4, 0);
      this->lcd->print("ST");
    }

    if(this->hasRds){
      // draw the program service text (either below the frequency, or if the display only has 2 lines, where the frequency is usually displayed.)
      this->lcd->setCursor(0, LCD_HEIGHT > 2 ? 1 : 0);
      this->lcd->print(this->ps);

      // draw the RDS icon on the same line as the program service label.
      if (LCD_HEIGHT > 2){
        this->lcd->setCursor(LCD_WIDTH - 3, LCD_HEIGHT > 2 ? 1 : 0);
        this->lcd->print("RDS");

        // draw the PTY only if the display is a 4 liner.
        if (LCD_HEIGHT > 3) {
          // draw the PTY text
        }
      }

      // display the RT on the lower line of the LCD.
      if (this->rt.length() <= LCD_WIDTH){
        this->lcd->setCursor(0, LCD_HEIGHT > 3 ? 3 : LCD_HEIGHT > 2 ? 2 : 1);
        this->lcd->print(this->rt.substring(currentWindow-LCD_WIDTH, currentWindow));
      }
    }else if (LCD_HEIGHT > 2){
      // display a no rds available message when there is no RDS.
      this->lcd->setCursor(LCD_WIDTH/2 - 8, 2);
      this->lcd->print("No RDS available");
    }

    // draw signal strength meter

    this->hasUpdatedScreen();
  }

  if (this->hasRds && this->rt.length() > LCD_WIDTH){
    this->lcd->setCursor(0, LCD_HEIGHT > 3 ? 3 : LCD_HEIGHT > 2 ? 2 : 1);

    if (++this->currentWindow > this->rt.length()){
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

    this->lcd->print(this->rt.substring(currentWindow - LCD_WIDTH, currentWindow));
  }
}


/*
 *  StationListScreen class impl.
 *  extends Screen
 *
 *  This manages the selection list functionality. Mainly for when the user wants to scroll through the available
 *  stations to select something fast. Allows the user to go up and down the list and select something.
*/

/**
 * @param lcd A pointer to the main LCD
 * @param size the size of the list. @todo infer this from the sList provided.
 * @param sList a list of options
 */
StationListScreen::StationListScreen(LiquidCrystal_I2C* lcd, int size, char** sList) : Screen(lcd){
  this->selectionList = sList;
  this->sizeofList = size;
}

/**
 * Moves down the list to the next element - or goes back to the top if the bottom has been reached.
 * @todo instantanously update the screen so that it feels responsive.
 */
void StationListScreen::moveDown(){
  this->selected++;
  if (this->selected > this->sizeofList - 1)
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
    this->selected = this->sizeofList-1;
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
    this->lcd->print("Station List");

    // checks to see if the selected component is pointing to the beginning/end and adjust the > < selectors, else
    // set the selected line on the lcd to the third line.
    int currentY = (this->selected == 0) ? 1 : (this->selected == this->sizeofList-1) ? 3 : 2;
    this->lcd->setCursor(0, currentY);
    this->lcd->print(">");
    this->lcd->setCursor(19, currentY);
    this->lcd->print("<");

    int index = this->selected - 1;
    if (index < 0)
      index = 0;
    else if (index+2 > this->sizeofList-1)
      index = this->sizeofList - 3;

    for (int i = 0; i < 3; i++){
      this->lcd->setCursor(2, i+1);
      this->lcd->print(this->selectionList[index + i]);
    }

    this->hasUpdatedScreen();
  }
}


/*
 *  VolumeScreen class impl.
 *  extends MainScreen, Screen
 *
 *  This is called when the radio needs to display the volume information - ie the user is adjusting the volume.
 *  The tick method can be adjusted to modify the user interface for this.
*/


/**
 * @param lcd A pointer to the main LCD
 * @param freq the current frequency expressed as a uint16 - 9990 = 99.9 MHz.
 * @param initVol the current volume of the radio
 */
VolumeScreen::VolumeScreen(LiquidCrystal_I2C* lcd, uint16_t freq, int initVol) : MainScreen(lcd, freq) {
    this->volume = initVol;
}

/**
 * Increments the volume by 1 up to a maximum of 15. (Maximum for the SI470x library)
 */
void VolumeScreen::moveUp() {
    if (++this->volume > 15)
      this->volume = 15;

    this->refreshOnNextDraw();
}

/**
 * Decrements the volume by 1 down to 0.
 */
void VolumeScreen::moveDown(){
    if (--this->volume < 0)
        this->volume = 0;
      
    this->refreshOnNextDraw();
}

/**
 * Returns the current volume that is being displayed.
 */
int VolumeScreen::getVolume(){
    return this->volume;
}

/** 
 * Part of the screen interface; is called when the screen is first drawn.
 */
void VolumeScreen::init(){
  MainScreen::init();
  
  this->lcd->setCursor(LCD_WIDTH/2 - 3, 1);
  this->lcd->print("Volume");
  this->lcd->setCursor(LCD_WIDTH/2 - 1, 2);
  this->lcd->printf("%02d", this->volume);
}

/**
 * Called in the main loop to update the screen if necessary.
 */
void VolumeScreen::tick(){
  if(this->needsUpdate()){
    this->lcd->clear();
    this->lcd->setCursor(1, 0);

    this->lcd->setCursor(1, 0);

    this->lcd->print(((float)this->frequency / 100.0f));
    this->lcd->print(" MHz");

    if(this->hasStereo){
      this->lcd->setCursor(LCD_WIDTH - 4, 0);
      this->lcd->print("ST");
    }
    
    this->lcd->setCursor(LCD_WIDTH/2 - 3, 1);
    this->lcd->print("Volume");
    this->lcd->setCursor(LCD_WIDTH/2 - 1, 2);
    this->lcd->printf("%02d", this->volume);

    this->hasUpdatedScreen();
  }
}