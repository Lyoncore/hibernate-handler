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

#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define finit_module(fd, uargs, flags) syscall(__NR_finit_module, fd, uargs, flags)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

int main(void) {
    struct utsname utsname={0};
    char rsi_sdio_ko[128]={0};
    char rsi_91x_ko[128]={0};
    int fd_sdio, fd_91x;
    int ret=EXIT_SUCCESS;
    struct stat st;

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("Redpine-hibernate", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); 
	syslog (LOG_NOTICE, "Redpine-hibernate reload driver start.\n");

	//Remove driver
    if (delete_module("ven_rsi_sdio", O_NONBLOCK) != 0) {
        syslog (LOG_ERR, "Remove ven_rsi_sdio fail\n");
        perror("delete_module failed(ven_rsi_sdio)");
    }

    if (delete_module("ven_rsi_91x", O_NONBLOCK) != 0) {
        syslog (LOG_ERR, "Remove ven_rsi_91x fail\n");
        perror("delete_module failed(ven_rsi_91x)");
    }

    uname(&utsname);
    sprintf(rsi_sdio_ko,"/lib/modules/%s/kernel/ubuntu/rsi/ven_rsi_sdio.ko",utsname.release);
    sprintf(rsi_91x_ko,"/lib/modules/%s/kernel/ubuntu/rsi/ven_rsi_91x.ko",utsname.release);
    fd_sdio = open(rsi_sdio_ko, O_RDONLY);
    if (fd_sdio < 0)
    {
        syslog (LOG_ERR, "Open driver failed:%s\n", rsi_sdio_ko);
        perror("Open ven_rsi_sdio failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }

    fd_91x = open(rsi_91x_ko, O_RDONLY);
    if (fd_91x < 0)
    {
        syslog (LOG_ERR, "Open driver failed:%s\n", rsi_91x_ko);
        perror("Open ven_rsi_91x failed\n");
        ret = EXIT_FAILURE;
        goto out_close_sdio;
    }

    // init_module ven_rsi_91x
    if (finit_module(fd_91x, "", 0) != 0) {
        perror("init_module image_91x");
        return EXIT_FAILURE;
        goto out_err_close_91x;
    }

    // init_module ven_rsi_sdio
    if (finit_module(fd_sdio, "", 0) != 0) {
        perror("init_module image_sdio");
        return EXIT_FAILURE;
        goto out_err_close_91x;
    }

out_err_close_91x:
    close(fd_91x);
out_close_sdio:
    close(fd_sdio);
out:
    closelog();
    return ret;
}
