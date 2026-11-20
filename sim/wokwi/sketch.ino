// ===== Generated for the Wokwi simulator from firmware/fire_fighting_robot =====
// ===== (config.h + fire_fighting_robot.ino). Edit those files, not this one. =====
// ============================================================================
//  config.h - every pin number and every tunable number lives here.
//  When the real robot behaves differently from what you expect, you should
//  only need to change values in this file (not the logic in the .ino file).
// ============================================================================
#include <Arduino.h>

// ---------------------------------------------------------------------------
//  Pins
// ---------------------------------------------------------------------------
// 5-channel flame sensor, ordered from the robot's LEFT to its RIGHT.
// (module outputs A1..A5 -> Arduino A0..A4). If the robot turns the wrong
// way towards a flame, reverse this list.
const uint8_t FLAME_PINS[5] = {A0, A1, A2, A3, A4};
const uint8_t FLAME_COUNT   = 5;

// L298N motor driver
const uint8_t PIN_ENA = 5;   // left motor speed (PWM)
const uint8_t PIN_IN1 = 7;   // left motor direction
const uint8_t PIN_IN2 = 8;
const uint8_t PIN_ENB = 6;   // right motor speed (PWM)
const uint8_t PIN_IN3 = 11;  // right motor direction
const uint8_t PIN_IN4 = 12;

const uint8_t PIN_SERVO  = 9;   // SG90 nozzle servo
const uint8_t PIN_PUMP   = 10;  // relay module input (switches the pump)
const uint8_t PIN_BUZZER = 4;   // buzzer (optional)
const unsigned int BUZZER_HZ = 2000;

// ---------------------------------------------------------------------------
//  Hardware quirks (check these with the test sketches in firmware/tests/)
// ---------------------------------------------------------------------------
// Most cheap 1-channel relay modules switch ON when their input is LOW.
// If the pump runs when it should be off, set this to false.
const bool RELAY_ACTIVE_LOW = true;

// If a wheel spins backwards when the robot should drive forward,
// flip the matching flag instead of swapping wires.
const bool LEFT_MOTOR_REVERSED  = false;
const bool RIGHT_MOTOR_REVERSED = false;

// Typical IR flame sensor modules give a LOWER analog value when they see
// MORE flame. If yours does the opposite, set this to false.
const bool FLAME_LOWER_MEANS_MORE = true;

// ---------------------------------------------------------------------------
//  Flame detection thresholds
//  "strength" = how much a channel moved away from its no-fire baseline
//  that the robot measures by itself at start-up (0 = nothing, ~1000 = max).
// ---------------------------------------------------------------------------
const int FLAME_DETECT_STRENGTH = 120;  // above this: "I can see a flame"
const int FLAME_LOST_STRENGTH   = 80;   // below this: "the flame is gone"
const int FLAME_NEAR_STRENGTH   = 500;  // above this: "close enough, spray" (~20 cm)

// ---------------------------------------------------------------------------
//  Motor speeds (0..255). TT motors usually need at least ~110 to move.
// ---------------------------------------------------------------------------
const int SPEED_SEARCH_TURN = 150;  // spinning on the spot while searching
const int SPEED_FORWARD     = 140;  // driving straight at the flame
const int SPEED_STEER_FAST  = 165;  // outer wheel while steering
const int SPEED_STEER_SLOW  = 60;   // inner wheel while steering
const int SPEED_BACK_OFF    = 140;  // reversing after the fire is out

// ---------------------------------------------------------------------------
//  Timings (milliseconds)
// ---------------------------------------------------------------------------
const unsigned long START_DELAY_MS    = 3000;  // time to put the robot down
const unsigned long SEARCH_SPIN_MS    = 160;   // spin a little...
const unsigned long SEARCH_LOOK_MS    = 120;   // ...then stop and look
const unsigned long LOST_TIMEOUT_MS   = 800;   // flame unseen this long -> search
const unsigned long SPRAY_MS          = 2500;  // one spray burst
const unsigned long PUMP_MAX_ON_MS    = 4000;  // safety: pump never runs longer
const unsigned long CHECK_WAIT_MS     = 1200;  // wait after spraying, then look
const unsigned long BACK_OFF_MS       = 500;   // reverse away from the candle
const unsigned long SENSOR_PERIOD_MS  = 20;    // read the sensors 50x a second
const unsigned long DEBUG_PERIOD_MS   = 250;   // print status 4x a second

const uint8_t MAX_SPRAY_ATTEMPTS = 4;  // give up (and alarm) after this many

// ---------------------------------------------------------------------------
//  Nozzle servo angles (degrees)
// ---------------------------------------------------------------------------
const int SERVO_CENTER   = 90;
const int SERVO_SWEEP    = 30;   // sweeps CENTER-30 .. CENTER+30
const int SERVO_STEP_DEG = 3;    // degrees per sweep step
const unsigned long SERVO_STEP_MS = 25;

// Print sensor values and state on the Serial Monitor (9600 baud).
const bool DEBUG = true;

// ============================================================================
//  Autonomous Fire-Fighting Robot
//  Arduino UNO + 5-channel IR flame sensor + L298N + 2 DC motors
//  + relay-switched water pump + SG90 nozzle servo + buzzer
//
//  Behaviour (a "state machine"):
//
//    STARTING -> SEARCH -> APPROACH -> EXTINGUISH -> CHECK -+-> BACK_OFF -> SEARCH
//                  ^          ^                       |     |   (fire is out)
//                  |          +------ still burning --+     |
//                  +-------------- flame lost --------------+
//                                     too many attempts -> ALARM
//
//  All pins and tunable numbers are in config.h.
// ============================================================================
#include <Servo.h>

// ---------------------------------------------------------------------------
//  State machine
// ---------------------------------------------------------------------------
enum State { STARTING, SEARCH, APPROACH, EXTINGUISH, CHECK, BACK_OFF, ALARM };
const char *const STATE_NAMES[] = {"STARTING", "SEARCH",   "APPROACH", "EXTINGUISH",
                                   "CHECK",    "BACK_OFF", "ALARM"};

State state = STARTING;
unsigned long stateStart = 0;  // millis() when we entered the current state

// ---------------------------------------------------------------------------
//  Flame sensor data
// ---------------------------------------------------------------------------
int baseline[FLAME_COUNT];  // raw values with no fire (measured at start-up)
int strength[FLAME_COUNT];  // how strongly each channel sees a flame (0 = none)
int maxStrength = 0;        // strongest channel
float flamePosition = 0;    // -2 = far left, 0 = straight ahead, +2 = far right
unsigned long lastSeen = 0; // last time a flame was visible

// ---------------------------------------------------------------------------
//  Actuators
// ---------------------------------------------------------------------------
Servo nozzle;
int servoAngle = SERVO_CENTER;
int servoDirection = 1;
unsigned long lastServoStep = 0;

bool pumpOn = false;
unsigned long pumpOnSince = 0;


uint8_t sprayAttempts = 0;
unsigned long lastSensorRead = 0;
unsigned long lastDebug = 0;
unsigned long lastAlarmBeep = 0;

// ===========================================================================
//  Small helpers
// ===========================================================================
unsigned long timeInState() { return millis() - stateStart; }

// --- Motors ----------------------------------------------------------------
// speed: -255 (full reverse) .. 0 (stop) .. +255 (full forward)
void driveMotor(uint8_t pinEnable, uint8_t pinA, uint8_t pinB, int speed, bool reversed) {
  if (reversed) speed = -speed;
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    digitalWrite(pinA, HIGH);
    digitalWrite(pinB, LOW);
  } else if (speed < 0) {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, HIGH);
  } else {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, LOW);
  }
  analogWrite(pinEnable, abs(speed));
}

void setMotors(int left, int right) {
  driveMotor(PIN_ENA, PIN_IN1, PIN_IN2, left, LEFT_MOTOR_REVERSED);
  driveMotor(PIN_ENB, PIN_IN3, PIN_IN4, right, RIGHT_MOTOR_REVERSED);
}

void stopMotors() { setMotors(0, 0); }

// --- Pump (through the relay) ----------------------------------------------
void setPump(bool on) {
  if (on && !pumpOn) pumpOnSince = millis();
  pumpOn = on;
  bool level = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(PIN_PUMP, level ? HIGH : LOW);
}

// --- Buzzer (non-blocking) -------------------------------------------------
// tone() plays in the background and stops by itself after `ms`.
// It works with an active buzzer too (and makes the Wokwi piezo audible).
void beep(unsigned long ms) { tone(PIN_BUZZER, BUZZER_HZ, ms); }

// --- Nozzle sweep (non-blocking) -------------------------------------------
void centerNozzle() {
  servoAngle = SERVO_CENTER;
  servoDirection = 1;
  nozzle.write(servoAngle);
}

void sweepNozzle() {
  if (millis() - lastServoStep < SERVO_STEP_MS) return;
  lastServoStep = millis();
  servoAngle += servoDirection * SERVO_STEP_DEG;
  if (servoAngle >= SERVO_CENTER + SERVO_SWEEP) servoDirection = -1;
  if (servoAngle <= SERVO_CENTER - SERVO_SWEEP) servoDirection = 1;
  nozzle.write(servoAngle);
}

// ===========================================================================
//  Flame sensor
// ===========================================================================
// Measure every channel with NO fire in view. Everything is compared to this,
// so the robot adapts to the room's light by itself.
void calibrateFlameSensor() {
  long sum[FLAME_COUNT] = {0};
  const int samples = 40;
  for (int s = 0; s < samples; s++) {
    for (uint8_t i = 0; i < FLAME_COUNT; i++) sum[i] += analogRead(FLAME_PINS[i]);
    delay(10);
  }
  for (uint8_t i = 0; i < FLAME_COUNT; i++) baseline[i] = sum[i] / samples;
}

void readFlameSensor() {
  long total = 0;
  long weighted = 0;
  maxStrength = 0;
  for (uint8_t i = 0; i < FLAME_COUNT; i++) {
    int raw = analogRead(FLAME_PINS[i]);
    int s = FLAME_LOWER_MEANS_MORE ? baseline[i] - raw : raw - baseline[i];
    if (s < 0) s = 0;
    strength[i] = s;
    if (s > maxStrength) maxStrength = s;
    total += s;
    weighted += (long)s * (i - 2);  // channel 0 -> -2 ... channel 4 -> +2
  }
  flamePosition = (total > 0) ? (float)weighted / total : 0;
  if (maxStrength >= FLAME_DETECT_STRENGTH) lastSeen = millis();
}

// ===========================================================================
//  State changes: everything that must happen ONCE when entering a state
// ===========================================================================
void changeState(State next) {
  state = next;
  stateStart = millis();

  switch (next) {
    case SEARCH:
      setPump(false);
      stopMotors();
      centerNozzle();
      break;
    case APPROACH:
      setPump(false);
      break;
    case EXTINGUISH:
      stopMotors();
      sprayAttempts++;
      centerNozzle();
      setPump(true);
      break;
    case CHECK:
      setPump(false);
      stopMotors();
      centerNozzle();
      break;
    case BACK_OFF:
      setPump(false);
      setMotors(-SPEED_BACK_OFF, -SPEED_BACK_OFF);
      break;
    case ALARM:
      setPump(false);
      stopMotors();
      break;
    default:
      break;
  }

  if (DEBUG) {
    Serial.print(F(">> state: "));
    Serial.println(STATE_NAMES[next]);
  }
}

void printStatus() {
  Serial.print(STATE_NAMES[state]);
  Serial.print(F("  flame:"));
  for (uint8_t i = 0; i < FLAME_COUNT; i++) {
    Serial.print(' ');
    Serial.print(strength[i]);
  }
  Serial.print(F("  max:"));
  Serial.print(maxStrength);
  Serial.print(F("  pos:"));
  Serial.print(flamePosition, 2);
  Serial.print(F("  pump:"));
  Serial.println(pumpOn ? F("ON") : F("off"));
}

// ===========================================================================
//  One function per state: what to do while we are IN that state
// ===========================================================================
void doStarting() {
  if (timeInState() < START_DELAY_MS) return;
  calibrateFlameSensor();
  if (DEBUG) {
    Serial.print(F("baseline:"));
    for (uint8_t i = 0; i < FLAME_COUNT; i++) {
      Serial.print(' ');
      Serial.print(baseline[i]);
    }
    Serial.println();
  }
  beep(150);
  changeState(SEARCH);
}

void doSearch() {
  if (maxStrength >= FLAME_DETECT_STRENGTH) {
    stopMotors();
    sprayAttempts = 0;
    beep(80);
    changeState(APPROACH);
    return;
  }
  // Spin right a little, stop and look, spin again... (stopping makes the
  // readings steadier than spinning non-stop)
  unsigned long t = timeInState() % (SEARCH_SPIN_MS + SEARCH_LOOK_MS);
  if (t < SEARCH_SPIN_MS) setMotors(SPEED_SEARCH_TURN, -SPEED_SEARCH_TURN);
  else stopMotors();
}

void doApproach() {
  if (maxStrength >= FLAME_NEAR_STRENGTH) {  // close enough
    changeState(EXTINGUISH);
    return;
  }
  if (millis() - lastSeen > LOST_TIMEOUT_MS) {  // flame disappeared
    changeState(SEARCH);
    return;
  }
  if (maxStrength < FLAME_LOST_STRENGTH) {  // flickered out of view: wait
    stopMotors();
    return;
  }

  if (flamePosition < -1.2) {         // far to the left: turn on the spot
    setMotors(-SPEED_SEARCH_TURN, SPEED_SEARCH_TURN);
  } else if (flamePosition > 1.2) {   // far to the right: turn on the spot
    setMotors(SPEED_SEARCH_TURN, -SPEED_SEARCH_TURN);
  } else if (flamePosition < -0.4) {  // a bit left: curve left
    setMotors(SPEED_STEER_SLOW, SPEED_STEER_FAST);
  } else if (flamePosition > 0.4) {   // a bit right: curve right
    setMotors(SPEED_STEER_FAST, SPEED_STEER_SLOW);
  } else {                            // straight ahead
    setMotors(SPEED_FORWARD, SPEED_FORWARD);
  }
}

void doExtinguish() {
  sweepNozzle();
  // setPump(false) may already have been forced by the safety timer
  if (timeInState() >= SPRAY_MS || !pumpOn) changeState(CHECK);
}

void doCheck() {
  if (timeInState() < CHECK_WAIT_MS) return;  // let the smoke/flicker settle

  if (maxStrength >= FLAME_DETECT_STRENGTH) {  // still burning
    if (sprayAttempts >= MAX_SPRAY_ATTEMPTS) changeState(ALARM);
    else if (maxStrength >= FLAME_NEAR_STRENGTH) changeState(EXTINGUISH);
    else changeState(APPROACH);
  } else {                                     // fire is out
    beep(400);
    changeState(BACK_OFF);
  }
}

void doBackOff() {
  if (timeInState() >= BACK_OFF_MS) changeState(SEARCH);
}

void doAlarm() {
  // The robot could not put the fire out: stop everything and call a human.
  if (millis() - lastAlarmBeep >= 1000) {
    lastAlarmBeep = millis();
    beep(500);
  }
}

// ===========================================================================
//  Arduino entry points
// ===========================================================================
void setup() {
  // Put the relay in its OFF position *before* making the pin an output,
  // so the pump never gives a short squirt at power-up.
  digitalWrite(PIN_PUMP, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(PIN_PUMP, OUTPUT);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  stopMotors();

  nozzle.attach(PIN_SERVO);
  centerNozzle();

  Serial.begin(9600);
  Serial.println(F("Fire-fighting robot: keep flames out of view, calibrating soon..."));
  changeState(STARTING);
}

void loop() {
  // Safety net: the pump never runs longer than PUMP_MAX_ON_MS in one go.
  if (pumpOn && millis() - pumpOnSince > PUMP_MAX_ON_MS) setPump(false);

  if (state != STARTING && millis() - lastSensorRead >= SENSOR_PERIOD_MS) {
    lastSensorRead = millis();
    readFlameSensor();
  }

  switch (state) {
    case STARTING:   doStarting();   break;
    case SEARCH:     doSearch();     break;
    case APPROACH:   doApproach();   break;
    case EXTINGUISH: doExtinguish(); break;
    case CHECK:      doCheck();      break;
    case BACK_OFF:   doBackOff();    break;
    case ALARM:      doAlarm();      break;
  }

  if (DEBUG && millis() - lastDebug >= DEBUG_PERIOD_MS) {
    lastDebug = millis();
    printStatus();
  }
}
