/*
 * File name: ud_client.cpp
 * Date:      2015-08-25 05:36
 * Author:    
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <syslog.h>

#include <iostream>

#include "ud_client.h"

using namespace std;

int ud_client_send_command(int command)
{
    struct sockaddr_un address;
    int  socket_fd;

    if(command != ROCPMD_SEND_OFF && 
       command != ROCPMD_SEND_POWER_LEVEL_PERCENT && 
       command != ROCPMD_SEND_POWER_LEVEL_RAW)
    {
        syslog(LOG_CRIT, "Invalid rocpmd command %d in function %s\n", command, __FUNCTION__);
        return ROCPMD_COMM_ERROR;
    }

    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);

    if(socket_fd < 0)
    {
        syslog(LOG_CRIT, "socket() failed in function %s(): %s\n", __FUNCTION__, strerror(errno));
        return ROCPMD_COMM_ERROR;
    }

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;

    string power_off_uds_path = "/tmp/";
    power_off_uds_path.append(ROCPMD_POWER_OFF_UDS_NAME);

    string power_level_uds_path = "/tmp/";
    power_level_uds_path.append(ROCPMD_POWER_LEVEL_UDS_NAME);

    if(command == 1)
    {
        snprintf(address.sun_path, PATH_MAX, power_off_uds_path.c_str());
    }
    else if(command == 2 || command==3)
    {
        snprintf(address.sun_path, PATH_MAX, power_level_uds_path.c_str());
    }
    else
    {
        return ROCPMD_COMM_ERROR;
    }

    if(connect(socket_fd,
               (struct sockaddr *) &address,
               sizeof(struct sockaddr_un)) != 0)
    {
        syslog(LOG_CRIT, "connect() failed in function %s(): %s\n", __FUNCTION__, strerror(errno));
        return ROCPMD_COMM_ERROR;
    }

    int bytes_written =  write(socket_fd, &command, sizeof(command));

    if(bytes_written < 0)
    {
        syslog(LOG_CRIT, "write() failed in function %s(): %s\n", __FUNCTION__, strerror(errno));
        close(socket_fd);
        return ROCPMD_COMM_ERROR;
    }

    int power_level;
    int bytes_read = read(socket_fd, &power_level, sizeof(power_level));

    if(bytes_written < 0)
    {
        syslog(LOG_CRIT, "write() failed in function %s(): %s\n", __FUNCTION__, strerror(errno));
        close(socket_fd);
        return ROCPMD_COMM_ERROR;
    }

    if(bytes_read < 0)
    {
        syslog(LOG_CRIT, "read() failed in function %s(): %s\n", __FUNCTION__, strerror(errno));
        close(socket_fd);
        return ROCPMD_COMM_ERROR;
    }

    syslog(LOG_CRIT, "MESSAGE FROM SERVER: %d\n", power_level);

    close(socket_fd);

    return power_level;
}
