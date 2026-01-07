#include "WString.h"
#ifndef FMLCD_H
#define FMLCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define SCROLL_WAITING_TIME  10
#define LCD_WIDTH            20   // width for screen scrolling
#define LCD_HEIGHT            4   // determines what is displayed.

#define MAIN_SCREEN 0x13
#define LIST_SCREEN 0x14
#define VOL_SCREEN  0x15

class Screen {
  protected:
    int screenType;
    LiquidCrystal_I2C* lcd;
    bool update = false;
    
    uint16_t frequency;
    uint8_t signalStrength;
    bool hasStereo = false;

    bool hasRds = false;
    String ps;
    String rt;
    int pty;

    void refreshOnNextDraw();
    bool needsUpdate();
    void hasUpdatedScreen();

    void clearLine(int y);
  public:
    Screen(LiquidCrystal_I2C*, uint16_t);
    Screen(LiquidCrystal_I2C*);

    const int getType();

    virtual void init();
    virtual void tick();

    virtual void moveDown();
    virtual void moveUp();
    virtual int select();

    /* Sets whether there is RDS or not. If not then an 'No RDS Available' message is shown. This will update the display on the next tick. */
    void setRDS(bool);
    /* Sets the RDS Program Service label. This will update the display on the next tick. */
    void setRdsPS(char*);
    /* Sets the RDS Radiotext. This will update the display on the next tick. */
    virtual void setRdsRT(char*);
    /* Sets the RDS Program Type. This will update the display on the next tick. */
    void setRdsPTY(int pty);
    /* Sets whether Stereo is available. Purely controls the Stereo Icon. This will update the display on the next tick. */
    void setStereo(bool);
    /* Sets the Frequency displayed on the LCD. This will update the display on the next tick. */
    void setFrequency(uint16_t);
    /* Sets the Signal Strength for the FM signal. Will update the signal meter on the display. */
    void setSignalStrength(int signalStrength);

    void moveData(Screen *scr);
};


class StationListScreen : public Screen {
  private:
    char** selectionList;
    int selected = 0;
    int sizeofList = 0;
  public:
    StationListScreen(LiquidCrystal_I2C*, int, char**);
    void moveDown() override;
    void moveUp() override;
    int select() override;

    void init() override;
    void tick() override;
};

class MainScreen : public Screen {
  protected:
    // scrolling variables
    int currentWindow = LCD_WIDTH;
    int wait = 0;

  public:
    /** Draws the main screen. First parameter is the LCD API, the second is the current frequency. */
    MainScreen(LiquidCrystal_I2C*, uint16_t);

    void setRdsRT(char*);

    void init();
    void tick();
};


class VolumeScreen : public MainScreen {
    private:
        int volume;
    public:
        VolumeScreen(LiquidCrystal_I2C*, uint16_t, int);

        void moveUp() override;
        void moveDown() override;
        int select() override;

        void init() override;
        void tick() override;
};

#endif