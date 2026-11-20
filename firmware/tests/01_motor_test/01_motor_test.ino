// Test 1 - motors + L298N
// Lift the robot so the wheels spin in the air, open the Serial Monitor
// (9600 baud) and check each wheel does what the message says.
// Wrong direction? Flip LEFT/RIGHT_MOTOR_REVERSED in config.h of the main sketch.

const uint8_t PIN_ENA = 5, PIN_IN1 = 7, PIN_IN2 = 8;    // left motor
const uint8_t PIN_ENB = 6, PIN_IN3 = 11, PIN_IN4 = 12;  // right motor
const int SPEED = 150;

void motor(uint8_t en, uint8_t a, uint8_t b, int speed) {
  digitalWrite(a, speed > 0 ? HIGH : LOW);
  digitalWrite(b, speed < 0 ? HIGH : LOW);
  analogWrite(en, abs(speed));
}

void drive(const char *what, int left, int right) {
  Serial.println(what);
  motor(PIN_ENA, PIN_IN1, PIN_IN2, left);
  motor(PIN_ENB, PIN_IN3, PIN_IN4, right);
  delay(1500);
  motor(PIN_ENA, PIN_IN1, PIN_IN2, 0);
  motor(PIN_ENB, PIN_IN3, PIN_IN4, 0);
  delay(1000);
}

void setup() {
  uint8_t pins[] = {PIN_ENA, PIN_IN1, PIN_IN2, PIN_ENB, PIN_IN3, PIN_IN4};
  for (uint8_t p : pins) pinMode(p, OUTPUT);
  Serial.begin(9600);
  Serial.println(F("Motor test starts in 2 s"));
  delay(2000);
}

void loop() {
  drive("LEFT wheel only, FORWARD", SPEED, 0);
  drive("RIGHT wheel only, FORWARD", 0, SPEED);
  drive("BOTH forward", SPEED, SPEED);
  drive("BOTH backward", -SPEED, -SPEED);
  drive("Spin RIGHT (left fwd, right back)", SPEED, -SPEED);
  delay(2000);
}
