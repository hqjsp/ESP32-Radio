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

#define ENABLE_WEB_SERVER // remove this to disable all the web server and web control stuff.

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "SI470X.h"

#ifdef ENABLE_WEB_SERVER
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define WIFI_SSID   "ESP Radio"   // network ssid for the wifi hotspot
#define WIFI_PASSWD "letmelisten" // network password

#endif

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
#define UPDATE_DELAY       20
#define TMP_SCR_SHOWTIME   40
#define DEBOUNCE_DELAY  300

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

#ifdef ENABLE_WEB_SERVER
AsyncWebServer server(80);
#endif

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
    screen->refreshOnNextDraw();
  }else screen->moveDown();
}

void menu_down(){
  if (screen->getType() != LIST_SCREEN){
    state->decrementFrequency();

    si470x.clearRdsBuffer();
    si470x.setFrequency(state->getFrequency());
    screen->refreshOnNextDraw();
  }else screen->moveUp();
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
      state->rds.set_ps((char *)selected->getRdsPS().c_str());
    }
    
    ESPRadio::setScreen(MAIN_SCREEN);

    si470x.clearRdsBuffer();
    si470x.setFrequency(state->getFrequency());
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
  Serial.printf("Starting up\n");
  // initiate button pins. These should be connected between the pin and ground.
  /*pinMode(MENU_UP_PIN, INPUT_PULLUP);
  pinMode(MENU_DOWN_PIN, INPUT_PULLUP);
  pinMode(MENU_PIN, INPUT_PULLUP);
  pinMode(VOLUME_UP_PIN, INPUT_PULLUP);
  pinMode(VOLUME_DN_PIN, INPUT_PULLUP);*/

  si470x.setup(SI4703_RST_PIN, SI4703_SDA_PIN, SI4703_SCL_PIN, OSCILLATOR_TYPE_REFCLK);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, signal_char);
  lcd.createChar(1, signal_0);
  lcd.createChar(2, signal_1);
  lcd.createChar(3, signal_2);
  lcd.createChar(4, meter_empty);
  lcd.createChar(5, meter_full);
  lcd.clear();

  #ifdef ENABLE_WEB_SERVER
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWD);
  
  IPAddress myIP = WiFi.softAPIP();

  server.on("/info", HTTP_GET, [](AsyncWebServerRequest * request) {
    char resp[256];
    snprintf(resp, 256, "{\"frequency\": %d, \"stereo\": %s, \"tp\": %s, \"pi\": %d, \"pty\": %d, \"ps\": \"%s\", \"rt\": \"%s\"}", 
      state->getFrequency(), state->hasStereo() ? "true" : "false", state->rds.get_tp() ? "true" : "false", state->rds.get_pi(), state->rds.get_pty(), 
      state->rds.get_ps(), state->rds.get_rt());
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", resp);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {request->send(200, "text/html", "<html><head><meta charset='utf-8' /><meta name='viewport' content='width=device-width, initial-scale=0.4, maximum-scale=0.4, user-scalable=0'/><title>ESP32 Radio Web Interface</title><style>html, body, .info-panel {width: 100vw; height: 100vh; margin: 0; background-color: #f4f4f4;font-size: 32pt;font-family: Verdana, sans-serif;overflow: hidden;}.info { width: 60vw; margin: auto; padding-top: 15vh; }.info div div { width: 50%; display: inline; }.info-panel .info .right p {display: inline;float: right;text-align: right;padding: 0 10px;}.info-panel .info .info-freq {font-size: 100pt;text-align: center;font-weight: bold;line-height: 185px;}.info-panel .info .info-rds { width: 60vw; text-align: center; }.info-panel .info .info-rds p { margin: 15px 0; }.info-panel .info .info-rds .rds-ps { font-size: 54pt; }.info-panel .info .info-rds .rds-pty { color: #777; }.info-panel .info .info-rds .rds-rt {width: 60vw;font-size: 40pt;padding-top: 20pt;margin: 0 auto;white-space: nowrap;overflow: hidden;position: absolute;}.info-panel .info .info-rds .rds-rt span {display: inline-block;padding-left: 100%;animation: rds-rt 10s linear(0.25 0%, 0.75 100%) infinite;}@keyframes rds-rt { 0% { transform: translate(0, 0); } 100% { transform: translate(-100%, 0); }}.button-panel {width: 100vw;padding-top: 40px;border: 1px solid #888;background-color: #fff;position: absolute;bottom: 0;}.button-panel div {width: 90%;margin-left: 5%;text-align: center;margin-bottom: 50px;}.relative-controls {display: grid;grid-template: 'a a a a a';}.relative-controls a, .preset-controls a {width: 90%;height: 150px;line-height: 150px;text-decoration: none;background-color: #f4f4f4;border: 1px solid #333;color: #000;}.preset-controls {display: grid;grid-template: 'a a a';row-gap: 50px;}</style><script>const pty_values = ['None','News','Current Affairs','Information','Sport','Education','Drama','Culture','Science','Varied','Pop Music','Rock Music','Easy Listening','Light Classical','Classical','Other Music','Weather','Finance','Children\\\'s','Social Affairs','Religion','Phone-in','Travel','Leisure','Jazz Music','Country Music','National Music','Oldies Music','Folk Music','Documentary','Alarm Test','Alarm'];let callServer = function(){return new Promise(function (resolve, reject) {let xml = new XMLHttpRequest();xml.open('GET', '/info');xml.onreadystatechange = function(ev){resolve(JSON.parse(xml.responseText));};xml.send();});};let getServerContent = async function(){let json = await callServer();document.getElementById('frequency').innerText = (json.frequency/100).toFixed(2);document.getElementById('stereo').innerText = json.stereo === true ? 'Stereo' : '';document.getElementById('traffic-program').innerText = json.tp === true ? 'TP' : '';document.getElementById('rds-pi').innerText = json.pi.toString(16);let pty = '';if (json.pty === 255)pty = 'None';else pty = pty_values[json.pty];document.getElementById('pty').innerText = pty;document.getElementById('ps').innerText = json.ps;document.getElementById('rt').innerText = json.rt;};let sendCommand = function(cmd){let req = new XMLHttpRequest();req.open('GET', '/' + cmd);req.send();getServerContent();};document.addEventListener('DOMContentLoaded', function(){setInterval(() => {getServerContent();}, 1000);});</script></head><body><div class='info-panel'><div class='info'><div style='display: flex;'><div class='left'><p class='rds-pi' id='rds-pi'></p></div><div class='right'><p class='rds-tp' id='traffic-program'></p><p class='fm-stereo' id='stereo'></p></div></div><div class='info-freq' id='frequency'>000.00</div><div class='info-rds'><p class='rds-pty' id='pty'></p><p class='rds-ps' id='ps'>Test FM</p><p class='rds-rt'><span id='rt'>The best test beats are on 99.9 - Test FM.</span></p></div></div></div><div class='button-panel'><div class='relative-controls'><a href='#' onclick='sendCommand(\"fdn\")'>Freq -</a><a href='#' onclick='sendCommand(\"fup\")'>Freq +</a><a href='#' onclick='sendCommand(\"menu\")'>Menu</a><a href='#' onclick='sendCommand(\"voldn\")'>Vol -</a><a href='#' onclick='sendCommand(\"volup\")'>Vol +</a></div><div class='preset-controls'><a href='#'>Preset 1</a><a href='#'>Preset 2</a><a href='#'>Preset 3</a><a href='#'>Preset 4</a><a href='#'>Preset 5</a><a href='#'>Preset 6</a></div></div></body></html>");});
  server.on("/fup", HTTP_GET, [](AsyncWebServerRequest * request) {if (ESPRadio::debounce()) menu_up();});
  server.on("/fdn", HTTP_GET, [](AsyncWebServerRequest * request) {if (ESPRadio::debounce()) menu_down();});
  server.on("/menu", HTTP_GET, [](AsyncWebServerRequest * request) {if (ESPRadio::debounce()) menu_select();});
  server.on("/volup", HTTP_GET, [](AsyncWebServerRequest * request) {if (ESPRadio::debounce()) vol_up();});
  server.on("/voldn", HTTP_GET, [](AsyncWebServerRequest * request) {if (ESPRadio::debounce()) vol_down();});
  server.begin();
  #endif

  lcd.setCursor(LCD_WIDTH / 2 - 6, 0);
  lcd.print("ESP-32 Radio");

  si470x.setRDS(true);
  si470x.setRdsMode(0);
  si470x.setMono(false);
  si470x.setFmDeemphasis(1);
  si470x.setAgc(true);
  si470x.setBlendLevelAdjustment(2);
  si470x.setBand(0);
  si470x.setSeekThreshold(20);

  si470x.setSoftmute(true);
  si470x.setSoftmuteAttenuation(1);

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
  state->setStereo(si470x.isStereo());

  if (signalStrength < 10){
    state->setSignalStrength(0);
  }else if (signalStrength < 25){
    state->setSignalStrength(1);
  }else if (signalStrength < 55){
    state->setSignalStrength(2);
  }else {
    state->setSignalStrength(3);
  }

  if (si470x.getRdsReady()){
    char *ps = si470x.getRdsStationName();
    char *rt = si470x.getRdsText2A();

    // check for RDS data.
    if (si470x.getRDSErrors() < 3){

      if (ps != NULL) 
        state->rds.set_ps(ps);
      
      if (rt != NULL)
        state->rds.set_rt(rt);
      
      state->rds.set_pty(si470x.getRdsProgramType());
      state->rds.set_pi(si470x.getRdsPI());
      state->rds.set_tp(si470x.getRdsTP());
    }
  }
  

  // check if the screen needs updating, and update it.
  if (ticker%10 == 0 || ESPRadio::buttonPressed) {
    screen->tick();

    // update the item in the station list if there was RDS received.
    if (state->rds.has_rds()){
      FMStationItem *fmitem = list->get(FMStationItem(state->getFrequency()));
      if (fmitem != nullptr)
        fmitem->setRdsPS(state->rds.get_ps());
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