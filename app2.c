#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOTHING_SPECIAL 0
#define SPC_RIGHT       1
#define SPC_DOWN        2
#define SPC_UP          3
#define SPC_LEFT        4
#define SPC_BACK        5
#define SPC_END         6
#define SPC_DEL         7
#define SPC_ENTER       8
#define SPC_HOME        9
#define SPC_ESC        10

typedef struct coord {
    short x;
    short y;
} coord;

typedef struct key {
    
} key;

struct tfield {
    short width;
    short lc;
    char **line;
    coord *linepos;
};

void ftfield(struct tfield *tf);

void s_prepare();
key  s_read_key();

int main(int argc, char *argv[])
{
    
    return 0;
}

void ftfield(struct tfield *tf)
{
    short  rx, ry;  /* cursor coordinates in tf                 */
    short  eocp;    /* y-coordinate of end of current paragraph */
    short  n;       /* used to determine newline presence       */
    short  maxy;    /* highest used y-coordinate                */
    
    short *len;     /* lengths of lines                               */
    char  *changed; /* whether lines has changed and must be rendered */
    
    char  *cpybuf;  /* string buffer  */
    char  *pbuf;    /* pointer buffer */
    
    int i, i2, postrx;
    key k;
    
    /* init some variables */
    rx = ry = 0;
    eocp    = 0;
    n       = tf -> width + 1;
    
    len     = calloc(tf -> lc, sizeof *len);
    changed = calloc(tf -> lc, sizeof *changed);
    
    cpybuf  = calloc(tf -> width + 1, sizeof *cpybuf);
    pbuf    = NULL;
    
    /* init length array */
    for (i = 0; i < tf -> lc; i++) {
        len[i] = strlen(tf -> line[i]);
        if (!len[i] && !tf -> line[i][n]) {
            maxy = i ? i - 1 : 0;
            break;
        }
    }
    
    /* main loop */
    while (-1) {
        /* adjust eocp */
        while (eocp != maxy && !tf -> line[eocp][n]) eocp++;
        
        /* update display */
        s_mvcur(c(tf -> linepos[ry].x + rx, tf -> linepos[ry].y));
        for (i = 0; i < tf -> lc; i++) {
            if (changed[i]) {
                dltfield(*tf, i);
                changed[i] = 0;
            }
        }
        
        /* main switch */
        switch ((k = s_read_key()).spc) {
        case NOTHING_SPECIAL:
            /* insert line if needed */
            if (len[eocp] == tf -> width) {
                if (maxy == tf -> lc - 1) break;
                shift_down(tf, ++eocp, maxy++, len);
                if (eocp != maxy) {
                    tf -> line[eocp - 1][n] = 0;
                    tf -> line[eocp][n] = 1;
                }
                for (i = maxy; i > eocp; i--) changed[i] = 1;
                if (rx == tf -> width) {
                    rx = 0;
                    ry++;
                }
            }
            /* shift characters within current paragraph */
            if (ry != eocp) {
                for (i = len[eocp]; i; i--)
                    tf -> line[eocp][i] = tf -> line[eocp][i - 1];
                tf -> line[eocp][0] = tf -> line[eocp - 1][tf -> width - 1];
                for (i2 = eocp - 1; i2 > ry; i2--) {
                    for (i = tf -> width - 1; i; i--)
                        tf -> line[i2][i] = tf -> line[i2][i - 1];
                    tf -> line[i2][0] = tf -> line[i2 - 1][tf -> width - 1];
                }
                i = tf -> width - 1;
            } else i = len[ry];
            for (; i > rx; i--)
                tf -> line[ry][i] = tf -> line[ry][i - 1];
            /* insert character */
            tf -> line[ry][rx] = k.c;
            len[eocp]++;
            for (i = ry; i <= eocp; i++) changed[i] = 1;
        case SPC_RIGHT:
            if (rx < len[ry] && (ry == eocp || rx < tf -> width - 1)) {
                rx++;
                break;
            }
        case SPC_DOWN:
            if (ry == maxy) break;
            if (k.spc == SPC_RIGHT) rx = 0;
            if (tf -> line[ry++][n]) eocp++;
            if (rx > len[ry]) rx = len[ry];
            goto adjust_rx;
        case SPC_UP:
            if (!ry) break;
            if (rx > len[--ry]) rx = len[ry];
            goto adjust_rx;
        case SPC_LEFT:
        case SPC_BACK:
            if (rx) {
                rx--;
                goto check_key;
            }
            if (!ry) break;
            if (tf -> line[--ry][n]) eocp--;
        case SPC_END:
            rx = len[ry];
        adjust_rx:
            if (rx == tf -> width && !tf -> line[ry][n] && ry != maxy) rx--;
        check_key:
            if (k.spc != SPC_BACK) break;
        case SPC_DEL:
            if (rx == len[ry] && ry == maxy) break;
            /* if a newline is deleted */
            if (ry == eocp && rx == len[ry]) {
                tf -> line[ry][n] = 0;
                while (eocp < maxy && !tf -> line[eocp][n]) eocp++;
                if (rx == tf -> width) {
                    ry++;
                    rx = 0;
                    break;
                }
                postrx = tf -> width - rx;
                /* initial shifting */
                for (i = ry + 1; i < eocp; i++) {
                    for (i2 = rx; i2 < tf -> width; i2++)
                        tf -> line[i - 1][i2] = tf -> line[i][i2 - rx];
                    for (i2 = postrx; i2 < len[i]; i2++)
                        tf -> line[i][i2 - postrx] = tf -> line[i][i2];
                }
                tf -> line[eocp][len[eocp] - postrx] = '\0';
                /* in the end */
                if (len[eocp] > postrx) {
                    for (i = len[eocp] - postrx + 1;
                        i < len[eocp]; i++)
                        tf -> line[eocp][i] = '\0';
                } else {
                    strcpy(tf -> line[eocp - 1] + rx,
                            tf -> line[eocp]);
                    for (i = rx + len[eocp] + 1; i < tf -> width; i++)
                        tf -> line[eocp - 1][i] = '\0';
                    memset(tf -> line[eocp], (int) '\0', len[eocp]
                            * sizeof **tf -> line);
                }
                len[ry] = tf -> width;
                len[eocp] -= postrx;
                if (len[eocp] < 0) {
                    len[eocp - 1] += len[eocp];
                    len[eocp] = 0;
                }
                tf -> line[ry][n] = 0;
            } else { /* if a character is deleted */
                /* shift characters within the current paragraph */
                strcpy(cpybuf, tf -> line[ry] + rx + 1);
                strcpy(tf -> line[ry] + rx, cpybuf);
                for (i = ry + 1; i <= eocp; i++) {
                    tf -> line[i - 1][tf -> width - 1] = tf -> line[i][0];
                    strcpy(cpybuf, tf -> line[i] + 1);
                    strcpy(tf -> line[i], cpybuf);
                }
                tf -> line[eocp][--len[eocp]] = '\0';
            }
            /* if the deletion created an empty line */
            if (!len[eocp] && maxy) {
                if (eocp)
                    if (tf -> line[eocp - 1][n]) goto change1; else;
                else break;
                pbuf = tf -> line[eocp];
                *cpybuf = tf -> line[eocp][n];
                i2 = (int) len[eocp];
                for (i = eocp--; i < maxy; i++) {
                    tf -> line[i] = tf -> line[i + 1];
                    tf -> line[i][n] = tf -> line[i + 1][n];
                    len[i] = len[i + 1];
                    changed[i] = 1;
                }
                tf -> line[maxy] = pbuf;
                tf -> line[maxy][n] = *cpybuf;
                len[maxy] = (short) i2;
                changed[maxy--] = 1;
            }
            if (ry > maxy) rx = len[--ry];
        change1:
            for (i = ry; i <= eocp; i++) changed[i] = 1;
            break;
        case SPC_ENTER:
            i2 = 0;
            if (!rx) {
                if (!ry) i2 = 1; /* cursor at (0,0) */
                else if (tf -> line[ry - 1][n])
                    i2 = 3; /* immediately after a newline */
                else {
                    tf -> line[ry - 1][n] = 1;
                    break;
                }
            } else {
                if (len[eocp] > rx)
                    i2 = 8; /* too long to fit without a new line */
                else if (ry == eocp) i2 = (ry == maxy) ? 2 : 4;
            }
            if (maxy == tf -> lc - 1) break;
            
            if (i2 && i2 != 2) { 
                if (i2 & 0x01) i = ry;
                else i = eocp + 1;
                shift_down(tf, i, maxy, len);
            }
            tf -> line[i2 == 4 ? ry + 1 : ry][n] = 1;
            if (i2) maxy++;
            for (i = maxy; i >= ry; i--) changed[i] = 1;
            
            if (i2 & 0x07) goto end_enter;
            
            i = eocp;
            if (i2) {
                if (eocp != maxy - 1) {
                    tf -> line[eocp + 1][n] = 1;
                    if (eocp != ry) tf -> line[eocp][n] = 0;
                }
                strcpy(tf -> line[eocp + 1], tf -> line[eocp] + rx);
                tf -> line[eocp][rx] = '\0';
            }
            postrx = tf -> width - rx;
            /* shifting within current paragraph */
            while (i > ry) {
                strcpy(cpybuf, tf -> line[i]);
                strcpy(tf -> line[i] + postrx, cpybuf);
                strncpy(tf -> line[i], tf -> line[i - 1] + rx, postrx);
                tf -> line[--i][rx] = '\0';
            }
            
            for (i = rx + 1; i < tf -> width; i++) tf -> line[ry][i] = '\0';
            if (eocp == ry) eocp++;
            len[eocp] += len[ry] - rx;
            len[ry] = rx;
            if (len[eocp] > tf -> width) {
                len[eocp + 1] = len[eocp] - tf -> width;
                len[eocp] = tf -> width;
            }
        end_enter:
            eocp = ++ry;
        case SPC_HOME:
            rx = 0;
            break;
        case SPC_ESC:
            return;
        }
    }
    return;
}

key s_read_key()
{
    ReadConsoleInput();
}
