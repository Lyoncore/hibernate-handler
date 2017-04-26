/*
 * Copyright (C) 2017        Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "modprobe_file_parse.h"
#include "hibernate-handler.h"

#define finit_module(fd, uargs, flags) syscall(__NR_finit_module, fd, uargs, flags)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

int Reload_redpine_driver(void) {
    int ret=EXIT_SUCCESS;
    char rsi_sdio_ko[BUFFER_SZ]={0};
    char rsi_91x_ko[BUFFER_SZ]={0};
    char mac80211_ko[BUFFER_SZ]={0};
    char cfg80211_ko[BUFFER_SZ]={0};
    char buf[BUFFER_SZ]={0};
    struct utsname utsname={0};
    int fd_sdio, fd_91x, fd_mac80211, fd_cfg80211, fd;
    char options[BUFFER_SZ]={0};

    syslog (LOG_NOTICE, "Redpine-hibernate reload driver start.\n");

    //Parse the /etc/modprobe.d/rs9113.conf
    fd = open(RS9113_SYSFS_DEV_OPER_MODE, O_RDONLY);
    if (fd < 0)
    {
        syslog (LOG_ERR, "Read dev_oper_mode from rs9113 failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    else
    {
        char mode[10];
        if( read(fd, mode, 10) >= 0)
        {
            syslog (LOG_NOTICE, "Last dev_oper_mode set to %s\n", mode);
            snprintf(options, BUFFER_SZ, "dev_oper_mode=%s",mode);
        }
    }

    //Remove driver
    if (delete_module("ven_rsi_sdio", O_NONBLOCK) != 0) {
        syslog (LOG_ERR, "Remove driver vmodule en_rsi_sdio module failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (delete_module("ven_rsi_91x", O_NONBLOCK) != 0) {
        syslog (LOG_ERR, "Remove driver module ven_rsi_91x failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (delete_module("mac80211", O_NONBLOCK) != 0) {
        syslog (LOG_ERR, "Remove driver module mac80211 failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    if (delete_module("cfg80211", O_NONBLOCK) != 0) {
        syslog (LOG_ERR, "Remove driver module cfg80211 failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    uname(&utsname);

    snprintf(rsi_sdio_ko, BUFFER_SZ, "/lib/modules/%s/kernel/ubuntu/rsi/ven_rsi_sdio.ko", utsname.release);
    snprintf(rsi_91x_ko, BUFFER_SZ, "/lib/modules/%s/kernel/ubuntu/rsi/ven_rsi_91x.ko", utsname.release);
    snprintf(mac80211_ko, BUFFER_SZ, "/lib/modules/%s/kernel/net/mac80211/mac80211.ko", utsname.release);
    snprintf(cfg80211_ko, BUFFER_SZ, "/lib/modules/%s/kernel/net/wireless/cfg80211.ko", utsname.release);
    fd_sdio = open(rsi_sdio_ko, O_RDONLY);
    if (fd_sdio < 0)
    {
        syslog (LOG_ERR, "Open driver module %s failed, errno:%d\n", rsi_sdio_ko, errno);
        perror("Open driver module ven_rsi_sdio.ko failed");
        ret = EXIT_FAILURE;
        goto out;
    }

    fd_91x = open(rsi_91x_ko, O_RDONLY);
    if (fd_91x < 0)
    {
        syslog (LOG_ERR, "Open driver module %s failed, errno:%d\n", rsi_91x_ko, errno);
        perror("Open driver module ven_rsi_91x.ko failed");
        ret = EXIT_FAILURE;
        goto out_close_sdio;
    }

    fd_mac80211 = open(mac80211_ko, O_RDONLY);
    if (fd_mac80211 < 0)
    {
        syslog (LOG_ERR, "Open driver module %s failed, errno:%d\n", mac80211_ko, errno);
        perror("Open driver module mac80211.ko failed");
        ret = EXIT_FAILURE;
        goto out_close_91x;
    }

    fd_cfg80211 = open(cfg80211_ko, O_RDONLY);
    if (fd_cfg80211 < 0)
    {
        syslog (LOG_ERR, "Open driver module %s failed, errno:%d\n", cfg80211_ko, errno);
        perror("Open driver module cfg80211.ko failed");
        ret = EXIT_FAILURE;
        goto out_close_mac80211;
    }

    // init_module cfg80211
    if (finit_module(fd_cfg80211, "", 0) != 0) {
        syslog (LOG_ERR, "Load driver module cfg80211 failed, errno:%d\n", errno);
        perror("Load module cfg80211 failed");
        return EXIT_FAILURE;
        goto out_close_cfg80211;
    }

    // init_module mac80211
    if (finit_module(fd_mac80211, "", 0) != 0) {
        syslog (LOG_ERR, "Load driver module mac80211 failed, errno:%d\n", errno);
        perror("Load module mac80211 failed");
        return EXIT_FAILURE;
        goto out_close_cfg80211;
    }

    // init_module ven_rsi_91x
    if (finit_module(fd_91x, "", 0) != 0) {
        syslog (LOG_ERR, "Load driver module ven_rsi_91x failed, errno:%d\n", errno);
        perror("Load module ven_rsi_91x failed");
        return EXIT_FAILURE;
        goto out_close_cfg80211;
    }

    // init_module ven_rsi_sdio
    if (finit_module(fd_sdio, options, 0) != 0) {
        syslog (LOG_ERR, "Load driver module ven_rsi_sdio failed, errno:%d\n", errno);
        perror("Load module ven_rsi_sdio failed");
        return EXIT_FAILURE;
        goto out_close_cfg80211;
    }


    syslog (LOG_NOTICE, "Redpine-hibernate reload driver complete.\n");

out_close_cfg80211:
    close(fd_cfg80211);
out_close_mac80211:
    close(fd_mac80211);
out_close_91x:
    close(fd_91x);
out_close_sdio:
    close(fd_sdio);

out:
    return ret;
}

int main(void) {
    int ret=EXIT_SUCCESS;
    struct stat st;
    char *s, redpine_reload_disable_file[BUFFER_SZ]={0};

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("hibernate-handler", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    s = getenv("SNAP_COMMON");
    if ( s != NULL )
        snprintf(redpine_reload_disable_file, BUFFER_SZ, "%s/%s", s, REDPINED_RELOAD_DISABLE_FILE);
    else
        snprintf(redpine_reload_disable_file, BUFFER_SZ, "%s", REDPINED_RELOAD_DISABLE_FILE);
        
    if( access( redpine_reload_disable_file , F_OK ) != -1 ) {
        syslog (LOG_NOTICE, "Redpine Reload is disabled\n");
        return ret;
    }
    else
        ret = Reload_redpine_driver();

    closelog();
    return ret;
}
