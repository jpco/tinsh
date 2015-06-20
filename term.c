#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

// local includes
#include "debug.h"

// self-include
#include "term.h"

static int fd;
static struct termios config;
static tcflag_t o_stty;
static tcflag_t m_stty;

char rd (void)
{
        unsigned char buffer[4];
        ssize_t n;
        n = read(fd, buffer, 1);
        if (n > 0) return buffer[0];
        else return 0;
}

int term_width (void)
{
        struct winsize w;
        if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
                print_err_wno ("Error getting term width", errno);
                return -1;
        }
        return w.ws_col;
}

int cursor_pos (int *row, int *col)
{
        write (fd, "[6n", 4);    // printf ("[6n"); 
        char cbuf[20] = {0};
        int idx = 0;
        char cval = '\0';
        while ((cval = rd()) != 'R' && idx < 20) {
                if (cval == 0) return 1;
                cbuf[idx++] = cval;
        }

        int l_row = 0;
        int l_col = 0;
        int success = sscanf(cbuf+2, "%d;%dR", &l_row, &l_col);
        if (success) {
                *row = l_row;
                *col = l_col;
                return 0;
        } else return 1;
}

int term_prep (void)
{
        config.c_lflag = o_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                print_err_wno ("tcsetattr failed.", errno);
                return -1;
        }

        return 0;
}

void term_unprep (void)
{
        config.c_lflag = m_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                print_err_wno("tcsetattr failed.", errno);
        }
}

void term_init (void)
{
        // turn off canonical mode
        const char *device = "/dev/tty";
        fd = open (device, O_RDWR);
        int err = errno;
        if (fd == -1) {
                print_err_wno ("Failed to set shell attributes (1).",
                                err);
                exit(1);
        }
        int retcode = tcgetattr(fd, &config);
        err = errno;
        if (retcode < 0) {
                print_err_wno ("Failed to set shell attributes (2).",
                                err);
                exit(1);
        }

        o_stty = config.c_lflag;
        config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
        m_stty = config.c_lflag;

        retcode = tcsetattr(fd, TCSAFLUSH, &config);
        err = errno;
        if (retcode < 0) {
                print_err_wno ("Failed to set shell attributes (3).",
                                err);
                exit (1);
        }
}

void term_exit (void)
{
        config.c_lflag = o_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0)
                print_err_wno("tcsetattr failed.", errno);

        do {
                close (fd);
        } while (errno == EINTR);
}
