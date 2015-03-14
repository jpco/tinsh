#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>

int idx;
int length;
char buf[300];

void prompt()
{
        printf(" > ");
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

void hist_mv(char in)
{
        // move through history sometime later
}

void reprint()
{
        int i;
        for (i = 0; i < idx; i++) printf("[D");
        for (i = 0; i < length; i++) printf("%c", buf[i]);
        printf(" ");
        for (i = length+1; i > idx; i--) printf("[D");
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

        if (idx < length) {
                printf("[C");
                reprint();
        }
        else printf("%c", in);
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

        if (idx < length) {
                for (i = 0; i < inlen; i++) printf("[C");
                reprint();
        }
        else printf("%s", in);
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
        struct termios config;
        int fd;

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

        config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);

        if (tcsetattr(fd, TCSAFLUSH, &config) < 0) {
                printf("tcsetattr failed\n");
                exit(1);
        }
        close(fd);
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
                                hist_mv(in);
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

int main()
{
        init();

        // input loop
        while (1) {
                prompt();
                line_loop();
                // line-processing things with buf goes here
        }
        
        return 0;
}
