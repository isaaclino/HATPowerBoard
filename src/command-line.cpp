/*
   -----------------------------------------------------------------------------

   Copyright (C) 2015  Kristjan Runarsson, Terry Garyet, Red oak Canyon LLC.

   This file is part of the Red Oak Canyon Power Management Daemon (rocpmd).

   rocpmd is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   rocpmd is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with rocpmd.  If not, see <http:  www.gnu.org/licenses/>.

   -----------------------------------------------------------------------------

   File name: command-line.cpp
   Date:      2015-08-07 21:56
   Author:    Kristjan Runarsson

   -----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <getopt.h>
#include <string>

#include <iostream>

#include "command-line.h"

using namespace std;

cmdline_options::cmdline_options(int argc, char **argv) :
    power_off_flag(0),
    websocket_flag(0),
    berkleysocket_flag(0),
    battery_level_flag(0),
    battery_level_raw_flag(0),
    verbose_flag(0),
    help_flag(0),
    config_path("/etc/rocpmd.conf")
{
    int c;

    while (1)
    {

      static struct option long_options[] =
        {
          {"config-path:",      required_argument,   0,                       'c'},
          {"power-off",         no_argument,         &power_off_flag,         'p'},
          //{"websocket",         no_argument,         &websocket_flag,         'w'},
          //{"berkleysocket",     no_argument,         &berkleysocket_flag,     'b'},
          {"battery-level",     no_argument,         &battery_level_flag,     'l'},
          {"battery-level-raw", no_argument,         &battery_level_raw_flag, 'r'},
          {"verbose",           no_argument,         &verbose_flag,           'v'},
          {"help",              no_argument,         &help_flag,              'h'},
          {0, 0, 0, 0}
        };

      int option_index = 0;

      //c = getopt_long (argc, argv, "c:pwblrvh",
      c = getopt_long (argc, argv, "c:plrvh",
                       long_options, &option_index);

      // Detect the end of the options.

      if (c == '?' || c == ':')
      {
          exit(1);
      }

      if (c == -1)
      {
          break;
      }

      switch (c)
        {
        case 0:
          // If this option set a flag, do nothing else now.
          if (long_options[option_index].flag != 0)
            break;
          break;

        case 'c':
          config_path = optarg;

          break;

        case 'p':
          power_off_flag = true;
          break;

        case 'h':
          help_flag = true;
          break;

        /*
        case 'w':
          websocket_flag = true;
          break;

        case 'b':
          berkleysocket_flag = true;
          break;
         */

        case 'l':
          battery_level_flag = true;
          break;

        case 'r':
          battery_level_raw_flag = true;
          break;

        case 'v':
          verbose_flag = true;
          break;

        case '?':
          // getopt_long already printed an error message.
          break;

        default:
          abort ();
        }
    }
}

bool cmdline_options::is_power_off()
{
    return power_off_flag;
}

bool cmdline_options::is_websocket()
{
    return websocket_flag;
}

bool cmdline_options::is_berkleysocket()
{
    return berkleysocket_flag;
}

bool cmdline_options::is_battery_level()
{
    return battery_level_flag;
}

bool cmdline_options::is_battery_level_raw()
{
    return battery_level_raw_flag;
}

bool cmdline_options::is_verbose()
{
    return verbose_flag;
}

bool cmdline_options::is_help()
{
    return help_flag;
}

string cmdline_options::get_config_path()
{
    return config_path;
}

void cmdline_options::usage()
{
    cout << "  -c, config-path CONFIG_PATH     Set path to an alternative configuration file." << endl;
    cout << "  -p, power-off                   Brutally powers off the PI." << endl; 
    //cout << "  -w, websocket                   Start start server to provide battery level." << endl;
    //cout << "  -b, berkleysocket               Start server to provde battery level." << endl;
    cout << "  -l, battery-level               Battery level as a percentage." << endl;
    cout << "  -r, battery-level-raw           Raw output of the battery level comparator." << endl;
    cout << "  -v, verbose                     Debug output during startup." << endl;
    cout << "  -h, help                        This text." << endl;
}
