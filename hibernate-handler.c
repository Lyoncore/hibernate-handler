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

#define finit_module(fd, uargs, flags) syscall(__NR_finit_module, fd, uargs, flags)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

#define REDPINED_RELOAD_DISABLE_FILE "redpine_reload_disable" // located at $SNAP_COMMON/redpine_reload_disable

#define BUFFER_SZ 256

int Reload_redpine_driver(void) {
    int ret=EXIT_SUCCESS;
    char rsi_sdio_ko[BUFFER_SZ]={0};
    char rsi_91x_ko[BUFFER_SZ]={0};
    char buf[BUFFER_SZ]={0};
    struct utsname utsname={0};
    int fd_sdio, fd_91x;

    syslog (LOG_NOTICE, "Redpine-hibernate reload driver start.\n");

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

    uname(&utsname);

    snprintf(rsi_sdio_ko, BUFFER_SZ, "/lib/modules/%s/kernel/ubuntu/rsi/ven_rsi_sdio.ko", utsname.release);
    snprintf(rsi_91x_ko, BUFFER_SZ, "/lib/modules/%s/kernel/ubuntu/rsi/ven_rsi_91x.ko", utsname.release);
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

    // init_module ven_rsi_91x
    if (finit_module(fd_91x, "", 0) != 0) {
        syslog (LOG_ERR, "Load driver module ven_rsi_91x failed, errno:%d\n", errno);
        perror("Load module ven_rsi_91x failed");
        return EXIT_FAILURE;
        goto out_err_close_91x;
    }

    // init_module ven_rsi_sdio
    if (finit_module(fd_sdio, "", 0) != 0) {
        syslog (LOG_ERR, "Load driver module ven_rsi_sdio failed, errno:%d\n", errno);
        perror("Load module ven_rsi_sdio failed");
        return EXIT_FAILURE;
        goto out_err_close_91x;
    }

	syslog (LOG_NOTICE, "Redpine-hibernate reload driver complete.\n");

out_err_close_91x:
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
