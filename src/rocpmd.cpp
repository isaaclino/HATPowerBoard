/*
 * File name: rocpmd.cpp
 * Date:	  2006-08-08 16:05
 * Author:	Kristján Rúnarsson
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <wiringPi.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h> // Strange things happen to POSIX threads if this is missing.
#include <errno.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/time.h>

#include "tdaemon.h"
#include "command-line.h"
#include "config.h"
#include "verbose.h"
#include "gpio.h"
#include "ud_server.h"
#include "ud_client.h"
#include "logger.h"

#define ROCPMD_LOCKFILE_MISSING       1
#define ROCPMD_LOCKFILE_NOT_READABLE  2
#define ROCPMD_PROCESS_LIVES          3
#define ROCPMD_PROCESS_DEAD           4


using namespace std;

// -----------------------------
// Globals
// -----------------------------

// -----------------------------
// Functions
// -----------------------------

void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        //FIXME: unlink the bloody *.pid file.
        exit(EXIT_SUCCESS);
    }
}


int rocpmd_instance_lives(string lockfile_path)
{
    // If the lock file does not exist we assume there is no living rocpmd instance.
    if(access(lockfile_path.c_str(), F_OK) < 0)
    {
        return ROCPMD_LOCKFILE_MISSING;
    }

    // If the lock file exists and is not readable we have a problem.
    if(access(lockfile_path.c_str(), R_OK) < 0)
    {
        return ROCPMD_LOCKFILE_NOT_READABLE;
    }

    // ... else we check if the pid in the lockfile is for a living process.

    string pid = "";

    ifstream lockfile(lockfile_path.c_str());

    getline(lockfile, pid);

    lockfile.close();

    /* pid file exists and process lives */ 
    if(kill(atoi(pid.c_str()), 0) == 0)
    {
        return ROCPMD_PROCESS_LIVES;
    }

    return ROCPMD_PROCESS_DEAD;
}


// -----------------------------
// Classes and structures
// -----------------------------

class rocpmd: public tdaemon
{
    
    public:
    	rocpmd(string daemon_name, string lock_file_name, int daemon_flags);
        ~rocpmd();
    
    protected:
        bool power_log;
        ud_server *ud;
        logger *battery_level_log;
        struct timeval last_read;
    	int daemon_main(config *conf);
        bool init_battery_level_timer();
        int battery_level_raw, battery_level_percent;
        void halt_system();
        bool battery_level_read_interval_expired(config *conf);
};

rocpmd::rocpmd(string daemon_name, string lock_file_name, int daemon_flags)
: tdaemon(daemon_name, lock_file_name, daemon_flags)
{
    gettimeofday(&last_read , NULL);
}

rocpmd::~rocpmd()
{
    delete battery_level_log;
}

int rocpmd::daemon_main(config *conf)
{
    string wd = "";
	check_work_dir(wd);
	
    ud = new ud_server();

    bool create_power_log = conf->get_battery_level_reader().battery_level_log;

    if(create_power_log)
    {
        try
        {
            battery_level_log = new logger("/var/log", "rocpmd-power-level.log");
        }
        catch(runtime_error re)
        {
    	    syslog(LOG_CRIT, "%s", re.what());
    	    syslog(LOG_CRIT, "Skipping power level log creation: %s", re.what());
            create_power_log = false;
        }
    }

	stringstream msg;
	msg<<"Starting rocpmd (pid: " << get_pid() << ")";

    syslog(LOG_CRIT,msg.str().c_str());

	while(true)
    {
		usleep(100000);

        if(!gpio_read_req_off(conf))
        {
		    syslog(LOG_CRIT, "Caught a REQ_OFF signal");
            halt_system();
        }

        if(battery_level_read_interval_expired(conf))
        {
            battery_level_raw = gpio_read_battery_level_raw(conf);
    
            // The battery level reading is not reliable when the wall wart is connected
            // in which case the gpio_read_battery_level_raw() function will return -1
            // so in the interest of uniformity the percentage power level is also set to
            // -1 to indicate charging status.
            if(battery_level_raw >= 0)
            {
                battery_level_percent = conf->get_powermap_element_at(battery_level_raw);
            }
            else
            {
                battery_level_percent = -1;
            }
    
            ud->set_battery_level_percent(battery_level_percent);
            ud->set_battery_level_raw(battery_level_raw);

            if(create_power_log)
            {
                try
                {
                    stringstream msg;
                    msg << battery_level_raw << "," << battery_level_percent << "%";
                    battery_level_log->log(msg.str());
                }
                catch(runtime_error re)
                {
                    syslog(LOG_CRIT, "%s", re.what());
                }
            }
    
            try
            {
                ud->handle_requests();
            }
            catch(runtime_error re)
            {
    		    syslog(LOG_CRIT, "%s", re.what());
            }
    
            if(battery_level_percent == 1)
            {
                halt_system();
            }
        }
	}

	return 0;
}

void rocpmd::halt_system()
{
    system("/sbin/shutdown -h now");
    exit(EXIT_SUCCESS);
}

#define SEC_TO_MSEC(x) x*1000.0

bool rocpmd::battery_level_read_interval_expired(config *conf)
{
    struct timeval now;

    double interval = SEC_TO_MSEC(conf->get_battery_level_reader().battery_level_read_interval);

    double diff, seconds, useconds;

    gettimeofday(&now , NULL);

    seconds  = now.tv_sec  - last_read.tv_sec;
    useconds = now.tv_usec - last_read.tv_usec;

    diff = ((seconds) * 1000 + useconds/1000.0);

    if(diff < interval)
    {
        return false;
    }

    last_read = now;

    return true;
}

// -----------------------------
// Main
// -----------------------------

int main(int argc, char **argv)
{
    bool opt_exit = false;

    signal(SIGINT, sig_handler);

    string daemon_name = "rocpmd";
    string daemon_lockfile_name = daemon_name;
    daemon_lockfile_name.append(".pid");

    cmdline_options opts(argc, argv);

    if(opts.is_help())
    {
        cmdline_options::usage();
        exit(0);
    }

    rocpmd *ROCPMD = NULL;
    try
    {
		ROCPMD = new rocpmd(daemon_name, daemon_lockfile_name, SINGLETON);
	}
	catch(tdaemon_exception e)
    {
		cout << "exiting: " << e.what()<<endl;
        exit(EXIT_FAILURE);
	}
	catch(...)
    {
		unexpected();
	}

    if(geteuid() != 0)
    {
        cerr << "You must have root privileges to run the '" << daemon_name << "' daemon" << endl;
        exit (EXIT_FAILURE);
    }

    wiringPiSetup () ;

    config *conf = NULL;
    string config_path;

    if(opts.get_config_path().length())
    {
        config_path = opts.get_config_path();
    }
    
    try
    {        
        conf = new config(config_path);
    }
    catch(runtime_error e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    catch(...)
    {
        unexpected();
    }

    if(opts.is_verbose())
    {
        verbose_print_options(opts);
        verbose_print_config(conf);
    }

    gpio_init(conf);

    if(opts.is_power_off() || 
       opts.is_battery_level() ||
       opts.is_battery_level_raw())
    {
        opt_exit = true;
    }

    if(opts.is_battery_level() || opts.is_battery_level_raw())
    {
        int raw_power = 0;

        int rocpmd_status = rocpmd_instance_lives(ROCPMD->get_lockfile_path());

        if(rocpmd_status == ROCPMD_LOCKFILE_NOT_READABLE)
        {
            cerr << "Unable to read the lockfile: " << ROCPMD->get_lockfile_path() << endl;
            exit(EXIT_FAILURE);
        }

        if(rocpmd_status == ROCPMD_LOCKFILE_MISSING || rocpmd_status == ROCPMD_PROCESS_LIVES)
        {
            if(opts.is_battery_level_raw())
            {
                int raw_power = ud_client_send_command(ROCPMD_SEND_POWER_LEVEL_RAW);

                if(raw_power >= 0)
                {
                    cout << "Raw power level:        " << raw_power << endl;
                }
                else
                {
                     cout << "Charging..." << endl;
                }
            }

            if(opts.is_battery_level())
            {
                int percent_power = ud_client_send_command(ROCPMD_SEND_POWER_LEVEL_PERCENT);
                
                if(percent_power >= 0)
                {
                    cout << "Percentage power level: " << percent_power << "%" << endl;
                }
                else
                {
                    cout << "Charging..." << endl;
                }
            }
        }
        else
        {
            raw_power = gpio_read_battery_level_raw(conf);

            if(raw_power >= 0)
            {
                if(opts.is_battery_level_raw())
                {
                    cout << "Raw power level: " << raw_power << endl;
                }

                if(opts.is_battery_level())
                {
                    int percent_power = conf->get_powermap_element_at(raw_power);
                    cout << "Percentage power level: " << percent_power << "%" << endl;
                }
            }
            else
            {
                cout << "Charging..." << endl;
            }
        }
    }

    if(opts.is_power_off())
    {
		syslog(LOG_CRIT, "Power off...");
        gpio_write_off(conf);
    }

    if(opt_exit)
    {
        exit(EXIT_SUCCESS);
    }

    try
    {
		ROCPMD->run_daemon(conf);
	}
	catch(tdaemon_exception e)
    {
		cout << "exiting: " << e.what()<<endl;
	}
	catch(runtime_error e)
    {
		cout << "exiting: " << e.what()<<endl;
	}
	catch(...)
    {
		unexpected();
	}

    delete conf;

	return 0;
} // end main()
