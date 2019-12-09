/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * Software SPI functions originally from Arduino Sd2Card Library
 * Copyright (c) 2009 by William Greiman
 */

/**
 * For TARGET_LPC1768
 */

/**
 * Hardware SPI and a software SPI implementations are included in this file.
 * The hardware SPI runs faster and has higher throughput but is not compatible
 * with some LCD interfaces/adapters.
 *
 * Control of the slave select pin(s) is handled by the calling routines.
 *
 * Some of the LCD interfaces/adapters result in the LCD SPI and the SD card
 * SPI sharing pins. The SCK, MOSI & MISO pins can NOT be set/cleared with
 * WRITE nor digitalWrite when the hardware SPI module within the LPC17xx is
 * active.  If any of these pins are shared then the software SPI must be used.
 *
 * A more sophisticated hardware SPI can be found at the following link.  This
 * implementation has not been fully debugged.
 * https://github.com/MarlinFirmware/Marlin/tree/071c7a78f27078fd4aee9a3ef365fcf5e143531e
 */

#ifdef TARGET_LPC1768

#include "../../inc/MarlinConfig.h"
#include <SPI.h>

// ------------------------
// Public functions
// ------------------------
#if ENABLED(LPC_SOFTWARE_SPI)

  #include <SoftwareSPI.h>

  // Software SPI

  static uint8_t SPI_speed = 0;

  static uint8_t spiTransfer(uint8_t b) {
    return swSpiTransfer(b, SPI_speed, SCK_PIN, MISO_PIN, MOSI_PIN);
  }

  void spiBegin() {
    swSpiBegin(SCK_PIN, MISO_PIN, MOSI_PIN);
  }

  void spiInit(uint8_t spiRate) {
    SPI_speed = swSpiInit(spiRate, SCK_PIN, MOSI_PIN);
  }

  uint8_t spiRec() { return spiTransfer(0xFF); }

  void spiRead(uint8_t*buf, uint16_t nbyte) {
    for (int i = 0; i < nbyte; i++)
      buf[i] = spiTransfer(0xFF);
  }

  void spiSend(uint8_t b) { (void)spiTransfer(b); }

  void spiSend(const uint8_t* buf, size_t nbyte) {
    for (uint16_t i = 0; i < nbyte; i++)
      (void)spiTransfer(buf[i]);
  }

  void spiSendBlock(uint8_t token, const uint8_t* buf) {
    (void)spiTransfer(token);
    for (uint16_t i = 0; i < 512; i++)
      (void)spiTransfer(buf[i]);
  }

#else

#endif // ENABLED(LPC_SOFTWARE_SPI)

#endif // TARGET_LPC1768
