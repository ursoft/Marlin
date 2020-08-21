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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * Fast I/O Routines for LPC1768/9
 * Use direct port manipulation to save scads of processor time.
 * Contributed by Triffid_Hunter and modified by Kliment, thinkyhead, Bob-the-Kuhn, et.al.
 */

/**
 * Description: Fast IO functions LPC1768
 *
 * For TARGET LPC1768
 */

#include "../shared/Marduino.h"

#define PWM_PIN(P)            true // all pins are PWM capable

#ifdef IVI_PWM_EXT_1_0
#include "../../../feature/twibus.h"
extern TWIBus i2c;

#define IS_IVI_PWM_EXT(IO)    (((IO) & IVI_PWM_EXT) == IVI_PWM_EXT)
#define NA_IVI_PWM_EXT(IO, W) if(IS_IVI_PWM_EXT(IO)) { W++; } else 
#define LPC_PIN(pin)          do{ NA_IVI_PWM_EXT(IO, LPC_PIN) LPC176x::gpio_pin(pin); } while(0)
#define LPC_GPIO(port)        do{ NA_IVI_PWM_EXT(IO, LPC_GPIO) LPC176x::gpio_port(port) } while(0)

#define SET_DIR_INPUT(IO)     do{ if(!IS_IVI_PWM_EXT(IO)) LPC176x::gpio_set_input(IO); } while(0)
#define SET_DIR_OUTPUT(IO)    do{ if(!IS_IVI_PWM_EXT(IO)) LPC176x::gpio_set_output(IO); } while(0)

#define SET_MODE(IO, mode)    do{ if(!IS_IVI_PWM_EXT(IO)) pinMode(IO, mode); } while(0) //not required

#define WRITE_PIN_SET(IO)     do{ NA_IVI_PWM_EXT(IO, WRITE_PIN_SET) LPC176x::gpio_set(IO); } while(0)
#define WRITE_PIN_CLR(IO)     do{ NA_IVI_PWM_EXT(IO, WRITE_PIN_CLR) LPC176x::gpio_clear(IO); } while(0)

template <uint16_t sel> 
struct PinManager {
    static uint8_t Read(uint16_t io);
    static void Write(uint16_t io, uint8_t v);
};
template<> inline uint8_t PinManager<IVI_PWM_EXT>::Read(uint16_t) { return 0 /* reading not implemented yet */; }
template<> inline uint8_t PinManager<0>::Read(uint16_t io) { return LPC176x::gpio_get(io); }
template<> inline void PinManager<IVI_PWM_EXT>::Write(uint16_t io, uint8_t v) { i2c.WritePwmExt((int8_t)io, v); }
template<> inline void PinManager<0>::Write(uint16_t io, uint8_t v) { LPC176x::gpio_set(io, v); }

#define READ_PIN(IO)          PinManager<IO & IVI_PWM_EXT>::Read(IO)
#define WRITE_PIN(IO, V)      PinManager<IO & IVI_PWM_EXT>::Write(IO, V)
#else
#define LPC_PIN(pin)          LPC176x::gpio_pin(pin)
#define LPC_GPIO(port)        LPC176x::gpio_port(port)

#define SET_DIR_INPUT(IO)     LPC176x::gpio_set_input(IO)
#define SET_DIR_OUTPUT(IO)    LPC176x::gpio_set_output(IO)

#define SET_MODE(IO, mode)    pinMode(IO, mode)

#define WRITE_PIN_SET(IO)     LPC176x::gpio_set(IO)
#define WRITE_PIN_CLR(IO)     LPC176x::gpio_clear(IO)

#define READ_PIN(IO)          LPC176x::gpio_get(IO)
#define WRITE_PIN(IO,V)       LPC176x::gpio_set(IO, V)

#define IS_IVI_PWM_EXT(IO)    false
#endif

/**
 * Magic I/O routines
 *
 * Now you can simply SET_OUTPUT(STEP); WRITE(STEP, HIGH); WRITE(STEP, LOW);
 *
 * Why double up on these macros? see https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
 */

/// Read a pin
#define _READ(IO)             READ_PIN(IO)

/// Write to a pin
#define _WRITE(IO,V)          WRITE_PIN(IO,V)

/// toggle a pin
#define _TOGGLE(IO)           _WRITE(IO, !READ(IO))

/// set pin as input
#define _SET_INPUT(IO)        SET_DIR_INPUT(IO)

/// set pin as output
#define _SET_OUTPUT(IO)       SET_DIR_OUTPUT(IO)

/// set pin as input with pullup mode
#define _PULLUP(IO,V)         do{ if(!IS_IVI_PWM_EXT(IO)) pinMode(IO, (V) ? INPUT_PULLUP : INPUT); } while(0)

/// set pin as input with pulldown mode
#define _PULLDOWN(IO,V)       do{ if(!IS_IVI_PWM_EXT(IO)) pinMode(IO, (V) ? INPUT_PULLDOWN : INPUT); } while(0)

/// check if pin is an input
#define _IS_INPUT(IO)         (IS_IVI_PWM_EXT(IO) ? false : !LPC176x::gpio_get_dir(IO))

/// check if pin is an output
#define _IS_OUTPUT(IO)        (IS_IVI_PWM_EXT(IO) ? true : LPC176x::gpio_get_dir(IO))

/// Read a pin wrapper
#define READ(IO)              _READ(IO)

/// Write to a pin wrapper
#define WRITE(IO,V)           _WRITE(IO,V)

/// toggle a pin wrapper
#define TOGGLE(IO)            _TOGGLE(IO)

/// set pin as input wrapper
#define SET_INPUT(IO)         _SET_INPUT(IO)
/// set pin as input with pullup wrapper
#define SET_INPUT_PULLUP(IO)  do{ _SET_INPUT(IO); _PULLUP(IO, HIGH); }while(0)
/// set pin as input with pulldown wrapper
#define SET_INPUT_PULLDOWN(IO) do{ _SET_INPUT(IO); _PULLDOWN(IO, HIGH); }while(0)
/// set pin as output wrapper  -  reads the pin and sets the output to that value
#define SET_OUTPUT(IO)        do{ _WRITE(IO, _READ(IO)); _SET_OUTPUT(IO); }while(0)
// set pin as PWM
#define SET_PWM               SET_OUTPUT

/// check if pin is an input wrapper
#define IS_INPUT(IO)          _IS_INPUT(IO)
/// check if pin is an output wrapper
#define IS_OUTPUT(IO)         _IS_OUTPUT(IO)

// Shorthand
#define OUT_WRITE(IO,V)       do{ SET_OUTPUT(IO); WRITE(IO,V); }while(0)

// digitalRead/Write wrappers
#define extDigitalRead(IO)    (IS_IVI_PWM_EXT(IO) ? 0 /* reading not implemented yet */ : digitalRead(IO))
#ifdef IVI_PWM_EXT_1_0
#define extDigitalWrite(IO,V) (IS_IVI_PWM_EXT(IO) ? i2c.WritePwmExt((int8_t)(IO), V) : digitalWrite(IO,V))
#define extAnalogWrite(IO,V)  (IS_IVI_PWM_EXT(IO) ? i2c.WritePwmExt((int8_t)(IO), V) : analogWrite(IO,V))
#else
#define extDigitalWrite(IO,V) (digitalWrite(IO,V))
#define extAnalogWrite(IO,V)  (analogWrite(IO,V))
#endif