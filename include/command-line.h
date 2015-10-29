#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include <string>

class cmdline_options
{
    private:
        int power_off_flag;
        int websocket_flag;
        int berkleysocket_flag;
        int battery_level_flag;
        int battery_level_raw_flag;
        int verbose_flag;
        int help_flag;
        std::string config_path;

    public:
        cmdline_options(int argc, char **argv);

        bool is_power_off();
        bool is_websocket();
        bool is_berkleysocket();
        bool is_battery_level();
        bool is_battery_level_raw();
        bool is_verbose();
        bool is_help();
        std::string get_config_path();
        static void usage();
};

#endif
