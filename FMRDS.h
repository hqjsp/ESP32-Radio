#ifndef FMRDS_H
#define FMRDS_H


#include <LiquidCrystal_I2C.h>
#include <SI470X.h>

#define RST_PIN     15
#define SDIO_PIN    A4
#define TUNING_PIN  A1

class Application {
  private:
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);
    SI470X rx;

    uint16_t currentFrequency;
    bool hasSt = true;
    bool hasRds = false;

  public:
    Application();

    void initComponents();
    void firstTick();
    void tick();


    void updateLCD();

};

#endif