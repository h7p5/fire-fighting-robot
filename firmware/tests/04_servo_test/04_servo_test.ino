// Test 4 - nozzle servo (SG90)
// The servo goes to the center, then sweeps left/right like when spraying.
// Fix the nozzle so it points straight ahead when the message says CENTER.

#include <Servo.h>

const uint8_t PIN_SERVO = 9;
const int CENTER = 90, SWEEP = 30;
Servo nozzle;

void setup() {
  nozzle.attach(PIN_SERVO);
  Serial.begin(9600);
}

void loop() {
  Serial.println(F("CENTER (90)"));
  nozzle.write(CENTER);
  delay(2000);
  Serial.println(F("sweeping..."));
  for (int i = 0; i < 3; i++) {
    for (int a = CENTER - SWEEP; a <= CENTER + SWEEP; a += 3) { nozzle.write(a); delay(25); }
    for (int a = CENTER + SWEEP; a >= CENTER - SWEEP; a -= 3) { nozzle.write(a); delay(25); }
  }
}
