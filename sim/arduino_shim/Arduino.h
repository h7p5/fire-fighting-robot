// Minimal stand-in for the Arduino core so the robot firmware can be compiled
// and run on a PC. The functions are implemented by sim/robot_sim.cpp.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
enum { A0 = 14, A1, A2, A3, A4, A5 };

#define F(x) (x)
#define constrain(amt, lo, hi) ((amt) < (lo) ? (lo) : ((amt) > (hi) ? (hi) : (amt)))

unsigned long millis();
void delay(unsigned long ms);
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);
inline void tone(uint8_t, unsigned int, unsigned long = 0) {}
inline void noTone(uint8_t) {}

extern bool g_serialEcho;  // print the robot's Serial output to the terminal?

struct SimSerial {
  void begin(long) {}
  void print(const char *s) { if (g_serialEcho) std::printf("%s", s); }
  void print(char c) { if (g_serialEcho) std::printf("%c", c); }
  void print(int v) { if (g_serialEcho) std::printf("%d", v); }
  void print(long v) { if (g_serialEcho) std::printf("%ld", v); }
  void print(unsigned long v) { if (g_serialEcho) std::printf("%lu", v); }
  void print(double v, int digits = 2) { if (g_serialEcho) std::printf("%.*f", digits, v); }
  void println() { print("\n"); }
  template <class T> void println(T v) { print(v); println(); }
  void println(double v, int digits) { print(v, digits); println(); }
};
extern SimSerial Serial;
