// ============================================================================
//  config.h - every pin number and every tunable number lives here.
//  When the real robot behaves differently from what you expect, you should
//  only need to change values in this file (not the logic in the .ino file).
// ============================================================================
#pragma once
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
