// Test 3 - relay + pump (do it over a sink!)
// The pump should run 1 s, then stop 3 s, again and again.
// If it is ON when the message says OFF, your relay is active-HIGH:
// set RELAY_ACTIVE_LOW = false in config.h (and here).

const uint8_t PIN_PUMP = 10;
const uint8_t PIN_BUZZER = 4;
const bool RELAY_ACTIVE_LOW = true;

void pump(bool on) {
  bool level = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(PIN_PUMP, level ? HIGH : LOW);
}

void setup() {
  pump(false);
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.begin(9600);
  Serial.println(F("Pump test starts in 2 s"));
  delay(2000);
}

void loop() {
  Serial.println(F("pump ON (and beep)"));
  pump(true);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(1000);
  Serial.println(F("pump OFF"));
  pump(false);
  digitalWrite(PIN_BUZZER, LOW);
  delay(3000);
}
