CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iarduino_shim -I../firmware/fire_fighting_robot -x c++
SRC = robot_sim.cpp
DEPS = $(SRC) ../firmware/fire_fighting_robot/fire_fighting_robot.ino ../firmware/fire_fighting_robot/config.h arduino_shim/Arduino.h arduino_shim/Servo.h

run: robot_sim
	@mkdir -p out
	./robot_sim

robot_sim: $(DEPS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

clean:
	rm -rf robot_sim out
.PHONY: run clean
