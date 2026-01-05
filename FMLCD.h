#include "WString.h"
#ifndef FMLCD_H
#define FMLCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define SCROLL_WAITING_TIME  10
#define LCD_WIDTH            20   // width for screen scrolling
#define LCD_HEIGHT            4   // determines what is displayed.

class Screen {
  protected:
    LiquidCrystal_I2C* lcd;
    bool update = false;

    void refreshOnNextDraw();
    bool needsUpdate();
    void hasUpdatedScreen();

    void clearLine(int y);
  public:
    Screen(LiquidCrystal_I2C*);
    virtual void init();
    virtual void tick();
};


class StationListScreen : public Screen {
  private:
    char** selectionList;
    int selected = 0;
    int sizeofList = 0;
  public:
    StationListScreen(LiquidCrystal_I2C*, int, char**);
    void moveDown();
    void moveUp();
    int select();

    void init() override;
    void tick() override;
};

class MainScreen : public Screen {
  private:
    uint16_t frequency;
    bool hasRds = false;
    bool hasStereo = false;

    String ps;
    String rt;
    int pty;
    int signalStrength;

    // scrolling variables
    int currentWindow = LCD_WIDTH;
    int wait = 0;

  public:
    /** Draws the main screen. First parameter is the LCD API, the second is the current frequency. */
    MainScreen(LiquidCrystal_I2C*, uint16_t);
    /* Sets the Frequency displayed on the LCD. This will update the display on the next tick. */
    void setFrequency(uint16_t);
    /* Sets whether there is RDS or not. If not then an 'No RDS Available' message is shown. This will update the display on the next tick. */
    void setRDS(bool);
    /* Sets whether Stereo is available. Purely controls the Stereo Icon. This will update the display on the next tick. */
    void setStereo(bool);
    /* Sets the RDS Program Service label. This will update the display on the next tick. */
    void setRdsPS(char*);
    /* Sets the RDS Radiotext. This will update the display on the next tick. */
    void setRdsRT(char*);
    /* Sets the RDS Program Type. This will update the display on the next tick. */
    void setRdsPTY(int pty);
    /* Sets the Signal Strength for the FM signal. Will update the signal meter on the display. */
    void setSignalStrength(int signalStrength);

    
    void init() override;
    void tick() override;
};


#endif