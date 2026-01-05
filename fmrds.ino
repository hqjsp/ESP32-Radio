
#include "FMRDS.h"

Application app = Application();

void setup() {
  // put your setup code here, to run once:
  app.initComponents();
  delay(500);

  app.firstTick();
}

void loop() {
  // put your main code here, to run repeatedly:
  app.tick();
}
