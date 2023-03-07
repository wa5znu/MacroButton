/*
 * based on http://www.arduino.cc/en/Tutorial/KeyboardMessage
 * https://digistump.com/wiki/digispark/tutorials/button
 * Copyright 2020 Leigh Klotz WA5ZNU <leigh@wa5znu.org>
 */

#include "DigiKeyboard.h"
#include "config.h"

#define PRESSED (HIGH)
#define RELEASED (LOW)

// Rev4 marking: The on-board LED is connected to P1.
// sadly I connected both to pin 1.  The multiplex works though.
const int PIN = 1;

const long TIMEOUT = 1500;
const long DEBOUNCE_DELAY = 25;
const long DIT_DAH_THRESHOLD = 200;  // todo: should be auto-threshold
const long ELEMENT_END_DELAY = 400;

int button_state = RELEASED;
int last_button_state = RELEASED;
long last_pressed_time = 0;
long last_released_time = 0;
long last_debounce_time = 0;
int state_index = 0;

const int MAX_RX_LEN = MAX_KEY_LEN + 1;
const char KEYS[N_KEYS][MAX_KEY_LEN] = MESSAGE_KEYS;
const char* MESSAGES[N_KEYS] = MESSAGE_VALUES;
char RX[MAX_RX_LEN];  // +1 for null terminator

void clearState();
void handleButton(int button_state, long delta);
void send();
void blink(int d);

void setup() {
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  DigiKeyboard.sendKeyStroke(0);
  digitalWrite(PIN, LOW);
  clearState();
  blink(100);
}

void loop() {
  pinMode(PIN, INPUT);
  int reading = digitalRead(PIN);

  long now = millis();
  if (reading != last_button_state) {
    last_debounce_time = now;
  }

  if ((now - last_debounce_time) > DEBOUNCE_DELAY) {
    if (reading != button_state) {
      button_state = reading;
      if (button_state == PRESSED) {
        last_pressed_time = now;
      } else if (button_state == RELEASED) {
        long delta_keydown = now - last_pressed_time;
        last_pressed_time = 0;
        handleButton(button_state, delta_keydown);
        last_released_time = now;
      } else {
        DigiKeyboard.println(F("BAD BUTTON STATE"));
      }
    }
  }

  if (state_index > 0 && button_state == RELEASED) {
    long delta = (now - last_released_time);
    if (delta > TIMEOUT) {
      handleTimeout();
    } else if (delta > ELEMENT_END_DELAY) {
      int k = key_matching_rx();
      if (k >= 0) {
        send(k);
        clearState();
      }
    }
  }

  last_button_state = reading;
}

void handleButton(int button_state, long delta) {
  if (state_index + 1 < MAX_RX_LEN) {
    char elt = ((delta <= DIT_DAH_THRESHOLD) ? '*' : '-');
    RX[state_index++] = elt;
    RX[state_index] = '\0';
  } else {
    blink(250);
  }
}

void clearState() {
  state_index = 0;
  last_debounce_time = 0;
  last_pressed_time = 0;
  last_released_time = 0;
  last_button_state = RELEASED;
  for (int i = 0; i < MAX_RX_LEN; i++) {
    RX[i] = '\0';
  }
}

void handleTimeout() {
  clearState();
  blink(50);
}

int key_matching_rx() {
  if (state_index > 0) {
    for (int k = 0; k < N_KEYS; k++) {
      if (strcmp(RX, KEYS[k]) == 0) {
        return k;
      }
    }
  }
  return -1;
}

void blink(int d) {
  for (int i = 0; i < 5; i++) {
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, HIGH);
    DigiKeyboard.delay(d);
    digitalWrite(PIN, LOW);
    DigiKeyboard.delay(d);
  }
}

void send(int k) {
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  DigiKeyboard.println(MESSAGES[k]);
  digitalWrite(PIN, LOW);
}
