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

#include "../../../inc/MarlinConfig.h"

#if HAS_COLOR_LEDS

#include "../../gcode.h"
#include "../../../feature/leds/leds.h"

/**
 * M150: Set Status LED Color - Use R-U-B-W for R-G-B-W
 *       and Brightness       - Use P (for NEOPIXEL only)
 *
 * Always sets all 3 or 4 components. If a component is left out, set to 0.
 *                                    If brightness is left out, no value changed
 *
 * With NEOPIXEL_LED:
 *  I<index>  Set the Neopixel index to affect. Default: All
 *
 * Examples:
 *
 *   M150 R255       ; Turn LED red
 *   M150 R255 U127  ; Turn LED orange (PWM only)
 *   M150            ; Turn LED off
 *   M150 R U B      ; Turn LED white
 *   M150 W          ; Turn LED white using a white LED
 *   M150 P127       ; Set LED 50% brightness
 *   M150 P          ; Set LED full brightness
 *   M150 I0         ; Work with pixel #0 only 
 *   M150 I          ; Work with all pixels (including 0)
 *   M150 I1 R       ; Set NEOPIXEL index 1 to red
 */
#ifdef CASE_LIGHT_NATURAL_BRIGHTNESS
extern const uint8_t cvt_brightness[] = {0, 16, 32, 34, 36, 38, 40, 43, 45, 47, 49, 51, 54, 56, 58, 60, 62, 
  65, 67, 69, 71, 73, 75, 77, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 99, 101, 103, 105, 107, 108, 110, 
  112, 113, 115, 117, 118, 120, 121, 123, 124, 125, 127, 128, 129, 130, 131, 133, 134, 135, 136, 138, 139, 
  140, 141, 142, 143, 145, 146, 147, 148, 149, 150, 151, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 
  164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 183, 
  184, 185, 186, 187, 188, 189, 190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200, 201, 202, 
  203, 203, 204, 205, 206, 206, 207, 208, 209, 209, 210, 211, 212, 212, 213, 214, 215, 215, 216, 217, 217, 
  218, 219, 219, 220, 221, 221, 222, 223, 223, 224, 224, 225, 226, 226, 227, 227, 228, 228, 229, 230, 230, 
  231, 231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 239, 239, 240, 240, 
  241, 241, 242, 242, 242, 243, 243, 243, 244, 244, 245, 245, 245, 246, 246, 246, 247, 247, 247, 247, 248, 
  248, 248, 249, 249, 249, 249, 250, 250, 250, 250, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 
  253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 
  254, 254, 254, 254, 255 };
#endif
void GcodeSuite::M150() {
  #ifdef IVI_PWM_EXT_1_0
    if(parser.seen('I') && parser.has_value() && parser.value_byte() == NEOPIXEL_PIXELS /* 0-3 for display */) {
      uint8_t brightness = parser.byteval('P', 255);
      i2c.WritePwmExt(0, 
#ifdef CASE_LIGHT_NATURAL_BRIGHTNESS
        cvt_brightness[brightness]
#else
        brightness
#endif
      );
      return;
    }
  #endif
  const LEDColor c((MakeLEDColor(
    parser.seen('R') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
    parser.seen('U') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
    parser.seen('B') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
    parser.seen('W') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
    parser.seen('P') ? (parser.has_value() ? parser.value_byte() : 255) : neo.brightness()
  )));
  if(parser.seen('S')) {
    uint8_t idx = parser.byteval('S', 0xFF);
    if(idx < NEOPIXEL_PIXELS || idx == 0xFF) {
      uint8_t real_idx = (idx == 0xFF) ? 0 : idx;
      if(real_idx == 0) LEDLights::defaultLEDColor = c;
      neo.set_pixel_color(real_idx, neo.Color(c.r, c.g, c.b, c.w));
      neo.set_brightness(c.i);
      neo.show();
      if(idx != 0xFF) return;
    } else {
      SERIAL_ECHOLNPAIR("pixel index out of range: ", idx);
      SERIAL_EOL();
      return;
    }
  }
  leds.set_color(c);
}

#endif // HAS_COLOR_LEDS
