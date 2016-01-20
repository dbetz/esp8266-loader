/*

MySoftwareSerial.cpp - Implementation of the Arduino software serial for ESP8266.
Copyright (c) 2015 Peter Lerup. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <Arduino.h>

// The Arduino standard GPIO routines are not enough,
// must use some from the Espressif SDK as well
extern "C" {
#include "gpio.h"
}

#include "MySoftwareSerial.h"

#define MAX_PIN 15

// List of SoftSerial object for each possible Rx pin
MySoftwareSerial *InterruptList[MAX_PIN+1];
bool InterruptsEnabled = false;

MySoftwareSerial::MySoftwareSerial(int receivePin, int transmitPin, bool inverse_logic, unsigned int buffSize) {
   m_rxValid = m_txValid = false;
   m_buffer = NULL;
   m_invert = inverse_logic;
   if (isValidGPIOpin(receivePin)) {
      m_rxPin = receivePin;
      m_buffSize = buffSize;
      m_buffer = (uint8_t*)malloc(m_buffSize);
      if (m_buffer != NULL) {
         m_rxValid = true;
         m_inPos = m_outPos = 0;
         pinMode(m_rxPin, INPUT);
         if (!InterruptsEnabled) {
            ETS_GPIO_INTR_ATTACH(handle_interrupt, 0);
            InterruptsEnabled = true;
         }
         InterruptList[m_rxPin] = this;
         GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(m_rxPin));
         enableRx(true);
      }
   }
   if (isValidGPIOpin(transmitPin)) {
      m_txValid = true;
      m_txPin = transmitPin;
      pinMode(m_txPin, OUTPUT);
      digitalWrite(m_txPin, m_invert ? LOW : HIGH);
   }
   // Default speed
   begin(9600);
}

MySoftwareSerial::~MySoftwareSerial() {
   enableRx(false);
   if (m_rxValid)
      InterruptList[m_rxPin] = NULL;
   if (m_buffer)
      free(m_buffer);
}

bool MySoftwareSerial::isValidGPIOpin(int pin) {
   // Some GPIO pins are reserved by the system
   return (pin >= 0 && pin <= 5) || (pin >= 12 && pin <= MAX_PIN);
}

void MySoftwareSerial::begin(long speed) {
   // Use getCycleCount() loop to get as exact timing as possible
   m_bitTime = ESP.getCpuFreqMHz()*1000000/speed;
}

void MySoftwareSerial::enableRx(bool on) {
   if (m_rxValid) {
      GPIO_INT_TYPE type;
      if (!on)
         type = GPIO_PIN_INTR_DISABLE;
      else if (m_invert)
         type = GPIO_PIN_INTR_POSEDGE;
      else
         type = GPIO_PIN_INTR_NEGEDGE;
      gpio_pin_intr_state_set(GPIO_ID_PIN(m_rxPin), type);
   }
}

int MySoftwareSerial::read() {
   if (!m_rxValid || (m_inPos == m_outPos)) return -1;
   uint8_t ch = m_buffer[m_outPos];
   m_outPos = (m_outPos+1) % m_buffSize;
   return ch;
}

int MySoftwareSerial::available() {
   if (!m_rxValid) return 0;
   int avail = m_inPos - m_outPos;
   if (avail < 0) avail += m_buffSize;
   return avail;
}

#define WAIT { while (ESP.getCycleCount()-start < wait); wait += m_bitTime; }

size_t MySoftwareSerial::write(uint8_t b) {
   if (!m_txValid) return 0;

   if (m_invert) b = ~b;
   // Disable interrupts in order to get a clean transmit
   cli();
   unsigned long wait = m_bitTime;
   digitalWrite(m_txPin, HIGH);
   unsigned long start = ESP.getCycleCount();
    // Start bit;
   digitalWrite(m_txPin, m_invert ? HIGH : LOW);
   WAIT;
   for (int i = 0; i < 8; i++) {
     digitalWrite(m_txPin, (b & 1) ? HIGH : LOW);
     WAIT;
     b >>= 1;
   }
   // Stop bit
   digitalWrite(m_txPin, m_invert ? LOW : HIGH);
   WAIT;
   sei();
   return 1;
}

void MySoftwareSerial::flush() {
   m_inPos = m_outPos = 0;
}

int MySoftwareSerial::peek() {
   if (!m_rxValid || (m_inPos == m_outPos)) return -1;
   return m_buffer[m_outPos];
}

void ICACHE_RAM_ATTR MySoftwareSerial::rxRead() {
   unsigned long wait = m_bitTime;
   unsigned long start = ESP.getCycleCount();
   uint8_t rec = 0;
   for (int i = 0; i < 8; i++) {
     WAIT;
     rec >>= 1;
     if (digitalRead(m_rxPin))
       rec |= 0x80;
   }
   if (m_invert) rec = ~rec;
   // Stop bit
   WAIT;
   // Store the received value in the buffer unless we have an overflow
   int next = (m_inPos+1) % m_buffSize;
   if (next != m_inPos) {
      m_buffer[m_inPos] = rec;
      m_inPos = next;
   }
}

void ICACHE_RAM_ATTR MySoftwareSerial::handle_interrupt(void *arg) {
   uint32_t gpioStatus = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
   // Clear the interrupt(s) otherwise we get called again
   GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpioStatus);
   ETS_GPIO_INTR_DISABLE();
   for (uint8_t pin = 0; pin <= MAX_PIN; pin++) {
      if ((gpioStatus & BIT(pin)) && InterruptList[pin]) {
         // Seems like the interrupt is delivered on all flanks in regardless
         // of what edge that has been set. Hence ignore unless we have a start bit
         if (digitalRead(pin) == InterruptList[pin]->m_invert)
            InterruptList[pin]->rxRead();
      }
   }
   ETS_GPIO_INTR_ENABLE();
}

