#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "parseline.h"
#include "hist.h"
#include "def.h"
#include "env.h"

static int fd;
static struct termios config;
static tcflag_t o_stty;
static tcflag_t m_stty;

char *in_m;
static int prompt_length;
int idx;
int length;
char buf[INPUT_MAX_LEN];

void prompt()
{
        char prompt[200];
        if (strcpy(getenv("USER"), "root") == 0) {
                sprintf(prompt, "\e[1m%s#\e[0m ", getenv("PWD"));
        } else {
                sprintf(prompt, "\e[1m%s$\e[0m ", getenv("PWD"));
        }
        prompt_length = strlen(getenv("PWD")) + 1;
        printf("%s", prompt);
}

void free_cmd()
{
        config.c_lflag = o_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
        }

        close (fd);
        hist_free();
        free (in_m);        
}

void buf_mv(char in)
{
        if (in == 'C') { // left
                if (idx < length) {
                        idx++;
                        printf("[C");
                }
        } else if (in == 'D') { // right
                if (idx > 0) {
                        idx--;
                        printf("[D");
                }
        }
}

// TODO: fix printing for long strings
void reprint()
{
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        int i;
        for (i = 0; i < idx; i++) printf("[D");
//      non-colored output
        for (i = 0; i < length; i++) printf("%c", buf[i]);
        int num_spaces = w.ws_col - (length + prompt_length);
        for (i = 0; i < num_spaces-2; i++) printf(" ");
        for (i = length+num_spaces-2; i > idx; i--) printf("[D");
}

void buffer(char in)
{
        if (idx < length) {
                int i;
                for (i = length; i > idx; i--)
                        buf[i] = buf[i-1];
        }

        buf[idx++] = in;
        length++;

        printf("[C");
        reprint();
}

void rebuffer(char *in)
{
        int inlen = strlen(in);

        strcpy(buf, in);
        length = inlen;
        int i;
        for (i = 0; i < idx; i++) printf("[D\n");
        idx = 0;

        reprint();
}

void sbuffer(char *in)
{
        int inlen = strlen(in);
        int i;

        if (idx < length) {
                for (i = length; i > (idx + inlen); i--)
                buf[i] = buf[i-inlen];
        }

        for (i = 0; i < inlen; i++) {
                buf[idx++] = in[i];
        }
        length += inlen;

        for (i = 0; i < inlen; i++) printf("[C");
        reprint();
}

void ibuffer(char in, int lind) {
        int oind = idx;
        idx = lind;
        buffer(in);
        idx = oind;
}

void buffer_rm(int bksp)
{
        if (bksp == 1 && idx == 0) return;
        if (length == 0) return;

        if (bksp) {
                idx--;
                printf("[D");
        }

        int i;
        for (i = idx; i < length; i++)
                buf[i] = buf[i+1];

        if (idx == length) {
                idx--;
                printf("[D");
        }
        length--;

        reprint();
}


void init()
{
        default_jpsh_env();

        // set canonical mode off
        const char *device = "/dev/tty";
        fd = open(device, O_RDWR);
        if (fd == -1) {
                printf("Failed to get fd\n");
                exit(1);
        }

        if (tcgetattr(fd, &config) < 0) {
                printf("tcgetattr failed\n");
                exit(1);
        }

        o_stty = config.c_lflag;
        config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
        m_stty = config.c_lflag;

        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
                exit(1);
        }
}

int interp(char in, int pre)
{
        if (in == '\n') {
                printf("\n");
                return -1;
        } else if (pre == 3) { // after [3 (or perhaps later /[\d/)
                if (in == '~') buffer_rm(0);
                else buffer(in);
                return 0;
        } else if(pre == 2) { // after [
                switch (in) {
                        case 'D':
                        case 'C':
                                buf_mv(in);
                                return 0;
                        case 'A':
                        case 'B':
                                rebuffer(hist_mv(in));
                                return 0;
                        case '3':
                                return 3;
                        default:
                                buffer(in);
                                return 0;
                }
        } else if (pre == 1) { // after 
                switch (in) {
                        case '[':
                                return 2;
                        default:
                                buffer(in);
                                return 0;
                }
        } else { // pre == 0, regular chars
                switch (in) {
                        case '':
                                buffer_rm(1);
                                return 0;
                        case '\t':
                                // tab-handling
                                sbuffer("TAB");
                                return 0;
                        case '':
                                return 1;
                        case '':
                                while (idx > 0) {
                                        idx--;
                                        printf("[D");
                                }
                                return 0;
                        case '':
                                while (idx < length) {
                                        idx++;
                                        printf("[C");
                                }
                                return 0;
                        default:
                                buffer(in);
                                return 0;
                }
        }
}

void line_loop()
{
        char in[2];
        idx = 0;
        length = 0;
        memset(buf, 0, 300 * sizeof(char));

        //  => status=1
        // [ => status=2
        // 3 => status=3 (for delete)
        int pre = 0;

        while (1) {
                fgets(in, 2, stdin);
                pre = interp(*in, pre);
                if (pre == -1) return;
        }
}

int prep_eval()
{
        if ((in_m = calloc(2 + strlen(buf), sizeof(char))) == NULL) {
                printf("malloc failed\n");
                return -1;
        }
        strcpy (in_m, buf);

        hist_add(in_m);

        in_m[strlen(in_m)] = ' ';

        config.c_lflag = o_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
                return -1;
        }

        return 0;
}
void unprep_eval()
{
        config.c_lflag = m_stty;
        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
        }

        free (in_m);
}

int main (void)
{
        init();

        while (1) {
                prompt();
                line_loop();

                if (!prep_eval())
                        eval (in_m);
                unprep_eval();
        }
        return 0;
}
