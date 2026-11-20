// Test 2 - 5-channel flame sensor
// Open the Serial Monitor (9600 baud). Light a candle ~50 cm away and move it
// from the robot's LEFT to its RIGHT. The strongest channel should go 0 -> 4.
// If it goes 4 -> 0, reverse FLAME_PINS in config.h.
// If values go UP near the flame, set FLAME_LOWER_MEANS_MORE = false.
// Use the numbers you see to tune FLAME_DETECT/NEAR_STRENGTH in config.h.

const uint8_t FLAME_PINS[5] = {A0, A1, A2, A3, A4};

void setup() {
  Serial.begin(9600);
  Serial.println(F("raw values: ch0(left) ch1 ch2(center) ch3 ch4(right)"));
}

void loop() {
  int lowest = 0;
  int values[5];
  for (uint8_t i = 0; i < 5; i++) {
    values[i] = analogRead(FLAME_PINS[i]);
    if (values[i] < values[lowest]) lowest = i;
    Serial.print(values[i]);
    Serial.print('\t');
  }
  Serial.print(F("lowest(=strongest?): ch"));
  Serial.println(lowest);
  delay(200);
}
