/**
 * ESP-32 Radio
 * 
 * FMLCD.h
 * 
 * These files contain all user interface related functions.
 * Each class' init() and tick() methods can be modified to change the layout of the ui
 */


#ifndef FMLCD_H
#define FMLCD_H


#include "WString.h"

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "FMState.h"

#define SCROLL_WAITING_TIME  10
#define LCD_WIDTH            20   // width for screen scrolling
#define LCD_HEIGHT            4   // determines what is displayed.

/** Main Screen ID */
#define MAIN_SCREEN 0x13
/** List Screen ID */
#define LIST_SCREEN 0x14
/** Volume Screen ID */
#define VOL_SCREEN  0x15


class Screen {
  protected:
    int screenType;
    LiquidCrystal_I2C* lcd;
    bool update = true;
    
    FMState *state;

    bool needsUpdate();
    void hasUpdatedScreen();

    void clearLine(int y);
  public:
    Screen(LiquidCrystal_I2C*, FMState*);

    const int getType();

    virtual void init();
    virtual void tick();

    virtual void moveDown();
    virtual void moveUp();
    virtual int select();

    virtual void refreshOnNextDraw();
};

/*
 *  StationListScreen class
 *  extends Screen
 *
 *  This manages the selection list functionality. Mainly for when the user wants to scroll through the available
 *  stations to select something fast. Allows the user to go up and down the list and select something.
*/
class StationListScreen : public Screen {
  private:
    FMStationList *list;
    int selected = 0;
  public:
    StationListScreen(LiquidCrystal_I2C*, FMState*, FMStationList*);
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
    MainScreen(LiquidCrystal_I2C*, FMState*);

    void refreshOnNextDraw();

    void init();
    void tick();
};

/*
 *  VolumeScreen class
 *  extends MainScreen, Screen
 *
 *  This is called when the radio needs to display the volume information - ie the user is adjusting the volume.
 *  The tick method can be adjusted to modify the user interface for this.
*/
class VolumeScreen : public MainScreen {
    public:
        VolumeScreen(LiquidCrystal_I2C*, FMState*);

        void moveUp() override;
        void moveDown() override;
        int select() override;

        void init() override;
        void tick() override;
};

#endif