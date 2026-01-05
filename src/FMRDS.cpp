
#include "FMRDS.h"

Application::Application(){
  // setup devices
  this->rx.setup(RST_PIN, SDIO_PIN);

  this->currentFrequency = 9990;
}

void Application::initComponents(){
  this->lcd.clear();
  this->lcd.backlight();

  this->lcd.setCursor(0, 1);
  this->lcd.print("Welcome to FM Radio!");
  this->lcd.setCursor(7, 2);
  this->lcd.print("v1.0.0");

  this->rx.setVolume(6);
  this->rx.setMono(false);
  this->rx.setRDS(true);
  this->rx.setRdsMode(1);

  // read potentiometer to get frequency

  this->rx.setFrequency(currentFrequency);
}

void Application::firstTick(){
  this->lcd.clear();

  this->updateLCD();
}

void Application::updateLCD() {
  // update rds info and add to screen
}

void Application::tick(){
  this->lcd.setCursor(0, 1);
  this->lcd.print("Welcome to FM Radio!");
  this->lcd.setCursor(7, 2);
  this->lcd.print("v1.0.0");
}