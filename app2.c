#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define NOTHING_SPECIAL 0x00
#define SPC_RIGHT       0x01
#define SPC_DOWN        0x10
#define SPC_UP          0x11
#define SPC_LEFT        0x02
#define SPC_BACK        0x03
#define SPC_END         0x04
#define SPC_DEL         0x05
#define SPC_ENTER       0x06
#define SPC_HOME        0x07
#define SPC_ESC         0x08

#define SPC_VERTICAL    0x10

#define F_GREY          F_BLUE | F_GREEN | F_RED
#define B_WHITE         B_BLUE | B_GREEN | B_RED | B_INTENSITY

#define F_BLUE          FOREGROUND_BLUE
#define F_GREEN         FOREGROUND_GREEN
#define F_RED           FOREGROUND_RED
#define F_INTENSITY     FOREGROUND_INTENSITY
#define F_BLACK         0
#define B_BLUE          BACKGROUND_BLUE
#define B_GREEN         BACKGROUND_GREEN
#define B_RED           BACKGROUND_RED
#define B_INTENSITY     BACKGROUND_INTENSITY

typedef struct {
    short x;
    short y;
} coord;

typedef struct {
    char spc;
    char c;
} key;

typedef struct {
    char  c;
    short a;
} char_info;

struct tfield {
    short width;
    short lc;
    char **line;
    coord *linepos;
};

struct tfield *tfield(short width, short lc, coord *linepos);
void  ftfield(struct tfield *tf);
void  dltfield(struct tfield tf, short y);
void  dtfield(struct tfield tf);
void  shift_down(struct tfield *tf, short top, short bottom, short *len);
void  shift_up(struct tfield *tf, short top, short bottom, short *len);
coord c(short x, short y);

void s_prepare();
key  s_read_key();
void s_pstrat(const char *str, coord c);
void s_printcis(char_info *arr, int len, coord c);
void s_mvcur(coord c);

CHAR_INFO  s_ci(char_info ci);
SMALL_RECT s_sr(short left, short top, short right, short bottom);
COORD s_c(short x, short y);
COORD s_cfc(coord c);

HANDLE s_h_in, s_h_out;

int main(int argc, char *argv[])
{
    struct tfield *tf;
    int i;
    
    s_prepare();
    tf = tfield(10, 10, NULL);
    for (i = 0; i < tf -> lc; i++)
        tf -> linepos[i] = c(10, i + 10);
    dtfield(*tf);
    ftfield(tf);
    
    return 0;
}

struct tfield *tfield(short width, short lc, coord *linepos)
{
    struct tfield *tf;
    int i, i2;
    
    tf = malloc(sizeof *tf);
    tf -> width   = width;
    tf -> lc      = lc;
    tf -> linepos = malloc(lc * sizeof *tf -> linepos);
    tf -> line    = malloc(lc * sizeof *tf -> line);
    
    for (i = 0; i < lc; i++) {
        tf -> line[i] = malloc((width + 1) * sizeof **tf -> line);
        for (i2 = 0; i2 <= width; i2++) tf -> line[i][i2] = '\0';
    }
    if (linepos != NULL)
        for (i = 0; i < lc; i++) tf -> linepos[i] = linepos[i];
    
    return tf;
}

void ftfield(struct tfield *tf)
{
    short  rx, ry;  /* cursor coordinates in tf                 */
    short  eocp;    /* y-coordinate of end of current paragraph */
    short  n;       /* used to determine newline presence       */
    short  maxy;    /* highest used y-coordinate                */
    
    short *len;     /* lengths of lines                                */
    char  *changed; /* whether lines have changed and must be rendered */
    
    char  *cpybuf;  /* string buffer  */
    char  *pbuf;    /* pointer buffer */
    
    int i, i2, postrx;
    key k;
    
    /* init some variables */
    rx = ry = 0;
    eocp    = 0;
    n       = tf -> width;
    
    len     = calloc(tf -> lc, sizeof *len);
    changed = calloc(tf -> lc, sizeof *changed);
    
    /* init len members and maxy */
    for (i = 0; i < tf -> lc; i++) {
        len[i] = strlen(tf -> line[i]);
        if (!len[i] && !tf -> line[i][n]) {
            maxy = i ? i - 1 : 0;
            break;
        }
    }
    
    /* main loop */
    while (-1) {
        /*printf("%02x", k.spc);
        /* adjust eocp */
        
        s_mvcur(c(0, 4));
        printf("1, eocp == %d, maxy == %d, (%d, %d)",
                    eocp, maxy, rx, ry);
        while (eocp != maxy && !tf -> line[eocp][n]) eocp++;
        s_mvcur(c(0, 5));
        printf("1, eocp == %d, maxy == %d, (%d, %d)",
                    eocp, maxy, rx, ry);
        s_pstrat("8", c(0, 0));
        /* update display */
        for (i = 0; i < tf -> lc; i++) {
            /*if (changed[i]) {
                dltfield(*tf, i);
                changed[i] = 0;
            }*/
            dtfield(*tf);
            s_mvcur(c(0, 3));
            printf("f%d", i);
            s_mvcur(c(tf -> linepos[i].x + tf -> width, tf -> linepos[i].y));
            printf("%c%c%c%x",
                    tf -> line[i][n] ? 'n' : '_',
                    i == eocp ? 'e' : '_',
                    i == maxy ? 'm' : '_',
                    len[i]);
        }
        s_mvcur(c(tf -> linepos[ry].x + rx, tf -> linepos[ry].y));
        s_pstrat("9", c(0, 0));
        /* main switch */
        switch ((k = s_read_key()).spc) {
        case NOTHING_SPECIAL:
        s_pstrat("1", c(1, 0));
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
        s_pstrat("2", c(1, 0));
            if (rx < len[ry] && (ry == eocp || rx < tf -> width - 1)) {
                rx++;
                break;
            }
        case SPC_DOWN:
        s_pstrat("3", c(1, 0));
            if (ry == maxy) break;
            if (k.spc == SPC_RIGHT) rx = 0;
            if (tf -> line[ry++][n]) eocp++;
            goto check_x_coord;
        case SPC_LEFT:
        case SPC_BACK:
        s_pstrat("5", c(1, 0));
            if (rx) {
                rx--;
                goto check_key;
            }
        case SPC_UP:
        s_pstrat("5", c(1, 0));
            if (!ry) break;
            if (tf -> line[--ry][n]) eocp--;
        check_x_coord:
            if ((k.spc & SPC_VERTICAL) && rx <= len[ry])
                goto adjust_rx;
        case SPC_END:
        s_pstrat("6", c(1, 0));
            rx = len[ry];
        adjust_rx:
            if (rx == tf -> width && !tf -> line[ry][n] && ry != maxy) rx--;
        check_key:
            if (k.spc != SPC_BACK) break;
        case SPC_DEL:
        s_pstrat("7", c(1, 0));
            if (rx == len[ry] && ry == maxy) break;
            /* if a newline is deleted */
            if (ry == eocp && rx == len[ry]) {
                s_pstrat("1", c(0, 0));
                s_pstrat("1", c(1, 0));
                tf -> line[ry][n] = 0;
                while (eocp != maxy && !tf -> line[eocp][n]) eocp++;
                if (rx == tf -> width) {
                    ry++;
                    rx = 0;
                    break;
                }
                s_pstrat("2", c(1, 0));
                /* initial shifting */
                postrx = tf -> width - rx;
                for (i = ry + 1; i < eocp; i++) {
                    for (i2 = rx; i2 < tf -> width; i2++)
                        tf -> line[i - 1][i2] = tf -> line[i][i2 - rx];
                    for (i2 = postrx; i2 < len[i]; i2++)
                        tf -> line[i][i2 - postrx] = tf -> line[i][i2];
                }
                i--;
                s_pstrat("3", c(1, 0));
                /* in the end */
                if (len[eocp] > postrx) {
                    for (i2 = rx; i2 < tf -> width; i2++)
                        tf -> line[i][i2] = tf -> line[eocp][i2 - rx];
                    for (i2 = postrx; tf -> line[eocp][i2]; i2++)
                        tf -> line[eocp][i2 - postrx] = tf -> line[eocp][i2];
                    len[i] = tf -> width;
                    i2 = postrx;
                    while (i2--) tf -> line[eocp][--len[eocp]] = '\0';
                } else {
                    for (i2 = rx; tf -> line[eocp][i2]; i2++)
                        tf -> line[i][i2] = tf -> line[eocp][i2 - rx];
                    len[i] += len[eocp];
                    while (len[eocp]) tf -> line[eocp][--len[eocp]] = '\0';
                }
                s_pstrat("4", c(1, 0));
            } else { /* if a character is deleted */
                /* shift characters within the current paragraph */
                s_pstrat("2", c(0, 0));
                s_pstrat("1", c(1, 0));
                i = ry;
                if (ry != eocp) {
                    while (-1) {
                        for (i2 = rx + 1; i2 < tf -> width; i2++)
                            tf -> line[i][i2 - 1] = tf -> line[i][i2];
                        tf -> line[i][tf -> width - 1] = tf -> line[i + 1][0];
                        if (++i == eocp) break;
                        for (i2 = 0; i2 < rx; i2++)
                            tf -> line[i][i2] = tf -> line[i][i2 + 1];
                    }
                    i2 = 0;
                } else i2 = rx;
                s_pstrat("2", c(1, 0));
                len[eocp]--;
                for (; i2 < len[eocp]; i2++)
                    tf -> line[eocp][i2] = tf -> line[eocp][i2 + 1];
                tf -> line[eocp][len[eocp]] = '\0';
                s_pstrat("3", c(1, 0));
            }
            /* if the deletion left an empty line */
            s_pstrat("3", c(0, 0));
            if (!len[eocp] && maxy) {
                s_pstrat("4", c(0, 0));
                if (tf -> line[eocp - 1][n]) goto change1;
                if (eocp == ry) rx = len[--ry];
                if (eocp == maxy) {
                    changed[eocp--] = 1;
                    maxy--;
                    goto change1;
                }
                s_pstrat("5", c(0, 0));
                for (i = eocp; i < maxy; i++) changed[i] = 1;
                tf -> line[eocp][n] = 0;
                tf -> line[--eocp][n] = 1;
                s_pstrat("6", c(0, 0));
                shift_up(tf, eocp-- + 1, maxy--, len);
            }
        change1:
            s_pstrat("7", c(0, 0));
            for (i = ry; i <= eocp; i++) changed[i] = 1;
            s_mvcur(c(0, 6));
            printf("1, eocp == %d, maxy == %d, (%d, %d)",
                    eocp, maxy, rx, ry);
            s_pstrat(" ", c(1, 0));
            break;
        case SPC_ENTER:
        s_pstrat("8", c(1, 0));
            if (!rx) {
                if (!ry) i2 = 0x01; /* cursor at (0,0) */
                else if (tf -> line[ry - 1][n])
                    i2 = 0x03; /* immediately after a newline */
                else {
                    tf -> line[ry - 1][n] = 1;
                    break;
                }
            } else {
                if (len[eocp] > rx)
                    i2 = 0x08; /* too long to fit without a new line */
                else if (ry == eocp) i2 = (ry == maxy) ?
                0x02: /* cursor at last line */
                0x06; /* cursor in a line with a newline */
                else i2 = 0x00;
            }
            if (maxy == tf -> lc - 1) break;
            s_pstrat("1", c(0, 0));
            if (i2 && i2 != 0x02) { /* 1, 3, 6, or 8 */
                if (i2 & 0x01) /* 1 or 3 */
                    i = ry;
                else /* 6 or 8 */
                    i = eocp + 1;
                s_pstrat("2", c(0, 0));
                shift_down(tf, i, maxy, len);
                s_pstrat("3", c(0, 0));
            }
            s_pstrat("4", c(0, 0));
            tf -> line[i2 == 0x04 ? ry + 1 : ry][n] = 1;
            
            if (i2 & 0x01) /* 1 or 3 */
                goto change2;
            s_pstrat("5", c(0, 0));
            if (i2 & 0x02) { /* 2 or 6 */
                for (i = rx; i < len[rx]; i++)
                    tf -> line[ry + 1][i - rx] = tf -> line[ry][i];
                goto change2;
            }
            s_pstrat("6", c(0, 0));
            /* 0 or 8 */
            postrx = tf -> width - rx;
            if (i2) { /* 8 */
                tf -> line[eocp + 1][n] = 1;
                if (eocp != ry) tf -> line[eocp][n] = 0;
                for (i = rx; i < len[eocp]; i++) {
                    tf -> line[eocp + 1][i - rx] = tf -> line[eocp][i];
                    tf -> line[eocp][i] = '\0';
                }
                len[eocp] += len[eocp - 1] - rx;
                i = eocp;
                len[--eocp] = rx;
            } else { /* 0 */
                i = eocp - 1;
                for (i2 = len[eocp] + postrx - 1; i2 >= postrx; i2--)
                    tf -> line[eocp][i2] = tf -> line[eocp][i2 - postrx];
                for (i2 = rx; i2 < tf -> width; i2++)
                    tf -> line[eocp][i2 - rx] = tf -> line[i][i2];
            }
            /* shifting within current paragraph */
            while (i > ry) {
                for (i2 = tf -> width - 1; i2 >= postrx; i2--)
                    tf -> line[i][i2] = tf -> line[i][i2 - postrx];
                for (i2 = rx; i2 < tf -> width; i2++)
                    tf -> line[i][i2 - rx] = tf -> line[i - 1][i2];
                i--;
            }
            for (i = rx; i < tf -> width; i++) tf -> line[ry][i] = '\0';
            len[eocp] += postrx;
            len[ry] = rx;
        change2:
            for (i = maxy++; i >= ry; i--) changed[i] = 1;
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

void dltfield(struct tfield tf, short y)
{
    const char_info ci = {
        '_',
        F_GREY | B_WHITE
    };
    
    char_info *cia;
    int x;
    
    cia = malloc(tf.width * sizeof *cia);
    
    for (x = 0; x < tf.width; x++) {
        cia[x].c = tf.line[y][x];
        if (cia[x].c == ' ' || cia[x].c == '\0') {
            cia[x] = ci;
            continue;
        }
        cia[x].a = B_WHITE | F_BLACK;
    }
    s_printcis(cia, tf.width, tf.linepos[y]);
    
    return;
}

void dtfield(struct tfield tf)
{
    int i;
    
    for (i = 0; i < tf.lc; i++) dltfield(tf, i);
    
    return;
}

void shift_down(struct tfield *tf, short top, short bottom, short *len)
{
    char *linebuf;
    short lenbuf;
    int   i;
    
    s_pstrat("1", c(1, 0));
    linebuf = tf -> line[++bottom];
    lenbuf  = len[bottom];
    s_pstrat("2", c(1, 0));
    for (i = bottom; i > top; i--) {
        tf -> line[i] = tf -> line[i - 1];
        len[i] = len[i - 1];
    }
    s_pstrat("3", c(1, 0));
    tf -> line[top] = linebuf;
    len[top] = lenbuf;
    s_pstrat(" ", c(1, 0));
    
    return;
}

void shift_up(struct tfield *tf, short top, short bottom, short *len)
{
    char *linebuf;
    short lenbuf;
    int   i;
    
    linebuf = tf -> line[--top];
    lenbuf  = len[top];
    for (i = top; i < bottom; i++) {
        tf -> line[i] = tf -> line[i + 1];
        len[i] = len[i + 1];
    }
    tf -> line[bottom] = linebuf;
    len[bottom] = lenbuf;
    return;
}

coord c(short x, short y)
{
    coord c;
    
    c.x = x;
    c.y = y;
    
    return c;
}

void s_prepare()
{
    s_h_in  = GetStdHandle(STD_INPUT_HANDLE);
    s_h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    
    return;
}

key s_read_key()
{
    const short corr_arr[][2] = {
        {VK_RIGHT  , SPC_RIGHT},
        {VK_DOWN   , SPC_DOWN},
        {VK_UP     , SPC_UP},
        {VK_LEFT   , SPC_LEFT},
        {VK_BACK   , SPC_BACK},
        {VK_END    , SPC_END},
        {VK_DELETE , SPC_DEL},
        {VK_RETURN , SPC_ENTER},
        {VK_HOME   , SPC_HOME},
        {VK_ESCAPE , SPC_ESC},
    };
    INPUT_RECORD ir;
    DWORD d;
    key   k;
    int   i;
     
    while (-1) {
        ReadConsoleInput(s_h_in, &ir, (DWORD) 1, &d);
        if (ir.EventType != KEY_EVENT) continue;
        if (!ir.Event.KeyEvent.bKeyDown) continue;
        switch (ir.Event.KeyEvent.wVirtualKeyCode) {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            continue;
        }
        k.c = ir.Event.KeyEvent.uChar.AsciiChar;
        d = (DWORD) ir.Event.KeyEvent.wVirtualKeyCode;
        for (i = sizeof corr_arr / sizeof **corr_arr / 2 - 1; i >= 0; i--) {
            if (d == corr_arr[i][0]) {
                k.spc = corr_arr[i][1];
                return k;
            }
        }
        k.spc = NOTHING_SPECIAL;
        return k;
    }
}

void s_pstrat(const char *str, coord c)
{
    LPCTSTR sstr;
    DWORD d;
    
    WriteConsoleOutputCharacter(s_h_out, str, strlen(str), s_cfc(c), &d);
    
    return;
}

void s_printcis(char_info *arr, int len, coord c)
{
    SMALL_RECT ssr;
    CHAR_INFO *sci;
    DWORD d;
    int   i;
    
    sci = malloc(len * sizeof *sci);
    ssr = s_sr(c.x, c.y, c.x + i - 1, c.y);
    for (i = 0; i < len; i++) sci[i] = s_ci(arr[i]);
    WriteConsoleOutput(s_h_out, sci, s_c(len, 1), s_c(0, 0), &ssr);
    
    return;
}

void s_mvcur(coord c)
{
    SetConsoleCursorPosition(s_h_out, s_cfc(c));
    return;
}

CHAR_INFO s_ci(char_info ci)
{
    CHAR_INFO sci;
    
    sci.Char.AsciiChar = ci.c;
    sci.Attributes = ci.a;
    
    return sci;
}

SMALL_RECT s_sr(short left, short top, short right, short bottom)
{
    SMALL_RECT sr;
    
    sr.Left   = left;
    sr.Top    = top;
    sr.Right  = right;
    sr.Bottom = bottom;
    
    return sr;
}

COORD s_c(short x, short y)
{
    COORD c;
    
    c.X = x;
    c.Y = y;
    
    return c;
}

COORD s_cfc(coord c)
{
    COORD sc;
    
    sc.X = c.x;
    sc.Y = c.y;
    
    return sc;
}
