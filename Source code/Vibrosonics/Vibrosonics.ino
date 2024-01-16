#include "Vibrosonics.h"

Vibrosonics v = Vibrosonics();

void setup() {
  v.init();
}

void loop() {
  v.update();
}
