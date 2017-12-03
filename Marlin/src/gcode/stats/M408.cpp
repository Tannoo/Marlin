/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
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

#include "../gcode.h"

#include "../../sd/cardreader.h"
#include "../../module/temperature.h"
#include "../../module/planner.h"
#include "../../module/printcounter.h"

/**
 * M408: Report JSON-style response to serial
 */

#if ENABLED(REPORT_M408_JSON)

  void floatkey(const char *pstr, const float &val, const bool integer = false, const bool comma = true) {
    SERIAL_CHAR('"');
    SERIAL_PROTOCOLCHAR(pstr);
    SERIAL_ECHO("\": ");
    if (comma) {
      if (integer)
        SERIAL_ECHO((int)val);
      else
        SERIAL_ECHO(val);
      SERIAL_CHAR(',');
    }
    else {
      SERIAL_ECHO(val);
    }
  }

  void xyzkey(const char *pstr, const bool homed = false, const bool fan = false, const bool comma = true) {
    SERIAL_CHAR('"');
    SERIAL_PROTOCOLCHAR(pstr);
    SERIAL_CHAR('"');
    SERIAL_ECHO(": [");
    if (fan)
      for (uint8_t i = 0; i < FAN_COUNT; i++) {
        if (i)
          SERIAL_PROTOCOLCHAR(',');
        SERIAL_PROTOCOL_F(fanSpeeds[i] / 2.5, 1);
      }
    else
      LOOP_XYZ(i) {
        if ( i != X_AXIS )
          SERIAL_CHAR(',');
          if (homed)
            SERIAL_PROTOCOLCHAR(axis_homed[i]?'1':'0');
          else
            SERIAL_PROTOCOL_F(current_position[i], 2);
      }
    SERIAL_CHAR(']');
    if (comma)
      SERIAL_CHAR(',');
  }

  void heatkey(const char *pstr, const bool active = false, const bool comma = true) {
    SERIAL_CHAR('"');
    SERIAL_PROTOCOLCHAR(pstr);
    SERIAL_CHAR('"');
    SERIAL_ECHO(": [");
    #if HAS_TEMP_BED
      if (active)
        SERIAL_PROTOCOL(thermalManager.degTargetBed());
      else
        SERIAL_PROTOCOL_F(thermalManager.degBed(), 1);
    #else
      SERIAL_PROTOCOL(-1);
    #endif
    #if HAS_TEMP_HOTEND
      HOTEND_LOOP() {
        SERIAL_PROTOCOLCHAR(',');
        if (active)
          SERIAL_PROTOCOL(thermalManager.degTargetHotend(e));
        else
          SERIAL_PROTOCOL_F(thermalManager.degHotend(e), 1);
      }
    #endif
    SERIAL_CHAR(']');
    if (comma)
      SERIAL_CHAR(',');
  }

  void e_stepperkey(const char *pstr, const bool flow = false, const bool comma = true) {
    static int16_t flow_percentage[EXTRUDERS];
    SERIAL_CHAR('"');
    SERIAL_PROTOCOLCHAR(pstr);
    SERIAL_CHAR('"');
    SERIAL_ECHO(": [");
    for (uint8_t i = 0; i < E_STEPPERS; i++) {
      if (i)
        SERIAL_PROTOCOLCHAR(',');
        if (flow)
          SERIAL_PROTOCOL(flow_percentage[i]);
        else
          SERIAL_PROTOCOL_F(current_position[E_AXIS + i], 1);
    }
    SERIAL_CHAR(']');
    if (comma)
      SERIAL_CHAR(',');
  }

  void GcodeSuite::M408() {

    // Status
    SERIAL_PROTOCOLPGM("{\"status\": \"");
    SERIAL_PROTOCOLCHAR(
      !planner.blocks_queued()                        ? 'I' : // IDLING
      (IS_SD_PRINTING || print_job_timer.isRunning()) ? 'P' : // SD Printing
      print_job_timer.isPaused()                      ? 'A' : // PAUSED
                                                        'B'   // SOMETHING ELSE
    );
    SERIAL_ECHO("\",");

    // Current Temps
    heatkey("heaters");

    // Setpoints
    heatkey("active", true);

    // Current XYZ positions
    xyzkey("pos");

    // Current Extruder positions
    e_stepperkey("extr");

    // sfactor
    floatkey("sfactor", feedrate_percentage, true);

    // Flow
    e_stepperkey("efactor", true);

    // Tool
    floatkey("tool", active_extruder, true);

    // Fan
    xyzkey("fanPercent", false, true);

    // Homed
    xyzkey("homed", true);

    // SD Printing
    #if ENABLED(SDSUPPORT)
      floatkey("fraction_printed", (card.percentDone() * 0.01), false, false);
    #else
      SERIAL_PROTOCOLCHAR("]");
    #endif

    SERIAL_CHAR('}');
    SERIAL_EOL();
  }

#endif // REPORT_M408_JSON
