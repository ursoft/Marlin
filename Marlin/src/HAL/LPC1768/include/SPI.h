/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
#pragma once

#include "../../shared/HAL_SPI.h"

#include <stdint.h>

#define MSBFIRST 1
#define SPI_MODE3 0

class SPISettings {
  public:
    SPISettings(uint32_t speed, int, int) : spi_speed(speed) {};
    uint32_t spiRate() const { return spi_speed; }
  private:
    uint32_t spi_speed;
};

#ifdef NEED_SPI_TEMPLATE_CLASS

#include <SoftwareSPI.h>

template<int NT_SCK_PIN = SCK_PIN, int NT_MISO_PIN = MISO_PIN, int NT_MOSI_PIN = MOSI_PIN>
class SoftwareSPI {
  public:
    uint8_t SPI_speed;

    static void spiBegin() {
      swSpiBegin(NT_SCK_PIN, NT_MISO_PIN, NT_MOSI_PIN);
    }

    void spiInit(uint8_t spiRate) {
      SPI_speed = swSpiInit(spiRate, NT_SCK_PIN, NT_MOSI_PIN);
    }

    uint8_t spiTransfer(uint8_t b) {
      return swSpiTransfer(b, SPI_speed, NT_SCK_PIN, NT_MISO_PIN, NT_MOSI_PIN);
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

  public:
    void begin() { spiBegin(); }
    void beginTransaction(SPISettings cfg) {
      uint8_t spiRate;
      switch (cfg.spiRate()) {
        case 8000000: spiRate = 0; break;
        case 4000000: spiRate = 1; break;
        case 2000000: spiRate = 2; break;
        case 1000000: spiRate = 3; break;
        case  500000: spiRate = 4; break;
        case  250000: spiRate = 5; break;
        case  125000: spiRate = 6; break;
        default: spiRate = 2; break;
      }
      spiInit(spiRate);
    }
    void endTransaction() {}
    uint8_t transfer(uint8_t B)  { return spiTransfer(B); }
    uint16_t transfer16(uint16_t data) {
      return (transfer((data >> 8) & 0xFF) << 8)
       | (transfer(data & 0xFF) & 0xFF);
    }
};

// decide which HW SPI device to use
#ifndef LPC_HW_SPI_DEV
  #if (SCK_PIN == P0_07 && MISO_PIN == P0_08 && MOSI_PIN == P0_09)
    #define _LPC_HW_SPI_DEV 1
  #else
    #if (SCK_PIN == P0_15 && MISO_PIN == P0_17 && MOSI_PIN == P0_18)
      #define _LPC_HW_SPI_DEV 0
    #else
      #error "Invalid pins selected for hardware SPI"
    #endif
  #endif
#endif
#if (LPC_HW_SPI_DEV == 0)
  #define _LPC_SSPn LPC_SSP0
#else
  #define _LPC_SSPn LPC_SSP1
#endif

// Hardware SPI

#include <lpc17xx_pinsel.h>
#include <lpc17xx_ssp.h>
#include <lpc17xx_clkpwr.h>

template<int NT_SCK_PIN = SCK_PIN, int NT_MISO_PIN = MISO_PIN, int NT_MOSI_PIN = MOSI_PIN, int NT_LPC_HW_SPI_DEV = _LPC_HW_SPI_DEV>
class HardwareSPI {
  public:
    static void spiBegin() {  // setup SCK, MOSI & MISO pins for SSP0
      PINSEL_CFG_Type PinCfg;  // data structure to hold init values
      PinCfg.Funcnum = 2;
      PinCfg.OpenDrain = 0;
      PinCfg.Pinmode = 0;
      PinCfg.Pinnum = LPC176x::pin_bit(NT_SCK_PIN);
      PinCfg.Portnum = LPC176x::pin_port(NT_SCK_PIN);
      PINSEL_ConfigPin(&PinCfg);
      SET_OUTPUT(NT_SCK_PIN);

      PinCfg.Pinnum = LPC176x::pin_bit(NT_MISO_PIN);
      PinCfg.Portnum = LPC176x::pin_port(NT_MISO_PIN);
      PINSEL_ConfigPin(&PinCfg);
      SET_INPUT(NT_MISO_PIN);

      PinCfg.Pinnum = LPC176x::pin_bit(NT_MOSI_PIN);
      PinCfg.Portnum = LPC176x::pin_port(NT_MOSI_PIN);
      PINSEL_ConfigPin(&PinCfg);
      SET_OUTPUT(NT_MOSI_PIN);
      // divide PCLK by 2 for SSP0
      CLKPWR_SetPCLKDiv(NT_LPC_HW_SPI_DEV ? CLKPWR_PCLKSEL_SSP1 : CLKPWR_PCLKSEL_SSP0, CLKPWR_PCLKSEL_CCLK_DIV_2);
      spiInit(0);
      SSP_Cmd(NT_LPC_HW_SPI_DEV ? LPC_SSP1 : LPC_SSP0, ENABLE);  // start SSP running
    }

    static void spiInit(uint8_t spiRate) {
      // table to convert Marlin spiRates (0-5 plus default) into bit rates
      uint32_t Marlin_speed[7]; // CPSR is always 2
      Marlin_speed[0] = 8333333; //(SCR:  2)  desired: 8,000,000  actual: 8,333,333  +4.2%  SPI_FULL_SPEED
      Marlin_speed[1] = 4166667; //(SCR:  5)  desired: 4,000,000  actual: 4,166,667  +4.2%  SPI_HALF_SPEED
      Marlin_speed[2] = 2083333; //(SCR: 11)  desired: 2,000,000  actual: 2,083,333  +4.2%  SPI_QUARTER_SPEED
      Marlin_speed[3] = 1000000; //(SCR: 24)  desired: 1,000,000  actual: 1,000,000         SPI_EIGHTH_SPEED
      Marlin_speed[4] =  500000; //(SCR: 49)  desired:   500,000  actual:   500,000         SPI_SPEED_5
      Marlin_speed[5] =  250000; //(SCR: 99)  desired:   250,000  actual:   250,000         SPI_SPEED_6
      Marlin_speed[6] =  125000; //(SCR:199)  desired:   125,000  actual:   125,000         Default from HAL.h
      // setup for SPI mode
      SSP_CFG_Type HW_SPI_init; // data structure to hold init values
      SSP_ConfigStructInit(&HW_SPI_init);  // set values for SPI mode
      HW_SPI_init.ClockRate = Marlin_speed[_MIN(spiRate, 6)]; // put in the specified bit rate
      HW_SPI_init.Mode |= SSP_CR1_SSP_EN;
      SSP_Init(NT_LPC_HW_SPI_DEV ? LPC_SSP1 : LPC_SSP0, &HW_SPI_init);  // puts the values into the proper bits in the SSP0 registers
    }

    static uint8_t doio(uint8_t b) {
      /* send and receive a single byte */
      SSP_SendData(NT_LPC_HW_SPI_DEV ? LPC_SSP1 : LPC_SSP0, b & 0x00FF);
      while (SSP_GetStatus(NT_LPC_HW_SPI_DEV ? LPC_SSP1 : LPC_SSP0, SSP_STAT_BUSY));  // wait for it to finish
      return SSP_ReceiveData(NT_LPC_HW_SPI_DEV ? LPC_SSP1 : LPC_SSP0) & 0x00FF;
    }

    static void spiSend(uint8_t b) { doio(b); }

    static void spiSend(const uint8_t* buf, size_t nbyte) {
      for (uint16_t i = 0; i < nbyte; i++) doio(buf[i]);
    }

    static void spiSend(uint32_t chan, byte b) {}

    static void spiSend(uint32_t chan, const uint8_t* buf, size_t nbyte) {}

    // Read single byte from SPI
    static uint8_t spiRec() { return doio(0xFF); }

    static uint8_t spiRec(uint32_t chan) { return 0; }

    // Read from SPI into buffer
    static void spiRead(uint8_t *buf, uint16_t nbyte) {
      for (uint16_t i = 0; i < nbyte; i++) buf[i] = doio(0xFF);
    }

    static uint8_t spiTransfer(uint8_t b) {
      return doio(b);
    }

    // Write from buffer to SPI
    static void spiSendBlock(uint8_t token, const uint8_t* buf) {
      (void)spiTransfer(token);
      for (uint16_t i = 0; i < 512; i++)
        (void)spiTransfer(buf[i]);
    }

    /** Begin SPI transaction, set clock, bit order, data mode */
    static void spiBeginTransaction(uint32_t spiClock, uint8_t bitOrder, uint8_t dataMode) {
      // TODO: to be implemented
    }

  public:
    void begin() { spiBegin(); }
    void beginTransaction(const SPISettings &cfg) {
      uint8_t spiRate;
      switch (cfg.spiRate()) {
        case 8000000: spiRate = 0; break;
        case 4000000: spiRate = 1; break;
        case 2000000: spiRate = 2; break;
        case 1000000: spiRate = 3; break;
        case  500000: spiRate = 4; break;
        case  250000: spiRate = 5; break;
        case  125000: spiRate = 6; break;
        default: spiRate = 2; break;
      }
      spiInit(spiRate);
    }
    void endTransaction() {}
    uint8_t transfer(uint8_t B)  { return spiTransfer(B); }
    uint16_t transfer16(uint16_t data) {
      return (transfer((data >> 8) & 0xFF) << 8)
       | (transfer(data & 0xFF) & 0xFF);
    }
};

#else
class SPIClass {
public:
    void begin();
    void beginTransaction(const SPISettings &);
    void endTransaction() {};
    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
};
extern SPIClass SPI;
#endif