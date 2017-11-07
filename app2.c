#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* -------------------------- macro definitions -------------------------- */

#define NOTHING_SPECIAL 0x20
#define SPC_RIGHT       0x21
#define SPC_DOWN        0x10
#define SPC_UP          0x11
#define SPC_LEFT        0x02
#define SPC_BACK        0x03
#define SPC_END         0x04
#define SPC_DEL         0x05
#define SPC_ENTER       0x06
#define SPC_HOME        0x07
#define SPC_ESC         0x08
#define SPC_CTRL        0x09

#define SPC_VERTICAL    0x10
#define SPC_RIGHT_OR_NS 0x20

#define F_BLUE          FOREGROUND_BLUE
#define F_GREEN         FOREGROUND_GREEN
#define F_RED           FOREGROUND_RED
#define F_INTENSITY     FOREGROUND_INTENSITY
#define F_BLACK         0
#define B_BLUE          BACKGROUND_BLUE
#define B_GREEN         BACKGROUND_GREEN
#define B_RED           BACKGROUND_RED
#define B_INTENSITY     BACKGROUND_INTENSITY

#define F_GREY          F_BLUE | F_GREEN | F_RED
#define B_WHITE         B_BLUE | B_GREEN | B_RED | B_INTENSITY
#define F_WHITE         F_BLUE | F_GREEN | F_RED | F_INTENSITY
#define B_DARK_GREY     B_INTENSITY

#define SCR_W           80
#define SCR_H           25

/* ------------------------- struct definitions -------------------------- */

typedef struct {
    short x;
    short y;
} coord;

coord c(short x, short y)
{
    coord c;
    
    c.x = x;
    c.y = y;
    
    return c;
}


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
    coord *linepos;
    char **line;
};

struct tf_handle {
    short eocp;
    short maxy;
    short *len;
    char *changed;
    coord r;
};

/* ------------------------ function declarations ------------------------ */

struct tfield *tfield(short width, short lc, coord *linepos);
struct tfield *read_from_file(char *filename, short width, short lc,
                              coord *linepos);
struct tfield *write_to_file(char *filename, struct tfield *tf);
void dltfield(struct tfield *tf, short y);
void dtfield(struct tfield *tf);

void ftfield(struct tfield *tf);
void delete(struct tfield *tf, struct tf_handle *h);
void add_newline(struct tfield *tf, struct tf_handle *h);
void shift_down(struct tfield *tf, short top, short bottom, short *len);
void shift_up(struct tfield *tf, short top, short bottom, short *len);

void s_prepare();
key  s_read_key();
void s_pstrat(const char *str, coord c);
void s_printcis(char_info *arr, int len, coord c);
void s_mvcur(coord c);

CHAR_INFO  s_ci(char_info ci);
SMALL_RECT s_sr(short left, short top, short right, short bottom);
COORD s_c(short x, short y);
COORD s_cfc(coord c);

/* -------------------------- global variables --------------------------- */

HANDLE s_h_in, s_h_out;
short n; /* used to determine newline presence */

/* ----------------------------------------------------------------------- */
/* ------------------------ function definitions ------------------------- */
/* ----------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    struct tfield *tf;
    int i;
    short width, lc;
    
    width = 35;
    lc = 36;
    if (argc > 1) {
        tf = read_from_file(argv[1], width, lc, NULL);
        /* if (argc != 3) {
            printf("Error %d: Invalid number of arguments\n", 1);
            return 1;
        }
        x = (int) strtol(argv[1], NULL, 10);
        y = (int) strtol(argv[2], NULL, 10); */
    } else {
        tf = tfield(width, lc, NULL);
        /* x = 10;
        y = 10; */
    }
    s_prepare();
    for (i = 0; i < tf -> lc / 2; i++) {
        tf -> linepos[i] = c(4, i + 3);
        tf -> linepos[i + tf -> lc / 2] = c(40, i + 3);
    }
    
    dtfield(tf);
    ftfield(tf);
    write_to_file(argv[1], tf);
    
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
        tf -> line[i] = malloc((width + 2) * sizeof **tf -> line);
        for (i2 = 0; i2 < width + 2; i2++) tf -> line[i][i2] = '\0';
    }
    if (linepos != NULL)
        for (i = 0; i < lc; i++) tf -> linepos[i] = linepos[i];
    
    return tf;
}

struct tfield *read_from_file
(char *filename, short width, short lc, coord *linepos)
{
    struct tfield *tf;
    short x, y;
    char c;
    FILE *file;
    
    tf = tfield(width, lc, linepos);
    file = fopen(filename, "r");
    if (file == NULL) return tf;
    n = tf -> width + 1;
    
    x = 0;
    y = 0;
    while (fread(&c, sizeof c, 1, file) == 1) {
        if (c == '\n') {
            tf -> line[y++][n] = 1;
            x = 0;
        } else {
            if (x == width) {
                if (++y == lc) break;
                x = 0;
            }
            tf -> line[y][x++] = c;
        }
    }
    
    return tf;
}

struct tfield *write_to_file(char *filename, struct tfield *tf)
{
    short x, y;
    char c;
    FILE *file;
    
    file = fopen(filename, "w");
    n = tf -> width + 1;
    
    x = 0;
    y = 0;
    while (y < tf -> lc) {
        if (tf -> line[y][n] &&
            (x == tf -> width || tf -> line[y][x] == '\0')) {
            c = '\n';
            y++;
            x = 0;
        } else if (x == tf -> width) {
            if (++y == tf -> lc) break;
            x = 0;
            c = tf -> line[y][x++];
        } else if (tf -> line[y][x] == '\0' && !tf -> line[y][n]) {
            break;
        } else c = tf -> line[y][x++];
        fwrite(&c, sizeof c, 1, file);
    }
    
    return tf;
}

void dltfield(struct tfield *tf, short y)
{
    char_info *cia;
    int x;
    char space;
    const short a_grey = B_WHITE | F_GREY;
    const short a_black = B_WHITE | F_BLACK;
    
    cia = malloc(tf -> width * sizeof *cia);
    
    space = '_';
    for (x = 0; x < tf -> width; x++) {
        if (tf -> line[y][x]  == ' ') {
            cia[x].c = space;
            cia[x].a = a_grey;
        } else if (tf -> line[y][x] == '\0') {
            cia[x].c = '_';
            cia[x].a = a_grey;
        } else {
            if (space == '_') space = ' ';
            cia[x].c = tf -> line[y][x];
            cia[x].a = a_black;
        }
    }
    s_printcis(cia, tf -> width, tf -> linepos[y]);
    
    return;
}

void dtfield(struct tfield *tf)
{
    int i;
    
    for (i = 0; i < tf -> lc; i++) dltfield(tf, i);
    
    return;
}

void ftfield(struct tfield *tf)
{
    struct tf_handle h;
    
    int i, i2, i3, postrx;
    key k;
    
    /* init some variables */
    n = tf -> width + 1;
    
    h.r.x = 0;
    h.r.y = 0;
    
    h.eocp = 0;
    h.changed = calloc(tf -> lc, sizeof *h.changed);
    h.len = calloc(tf -> lc, sizeof *h.len);
    
    /* init len members and maxy */
    h.maxy = -1;
    for (i = 0; i < tf -> lc; i++) {
        h.len[i] = strlen(tf -> line[i]);
        if (!h.len[i] && !tf -> line[i][n]) {
            if (!i) h.maxy = 0;
            else if (tf -> line[i - 1][n]) h.maxy = i;
            else h.maxy = i - 1;
            break;
        }
    }
    if (h.maxy == -1) h.maxy = tf -> lc - 1;
    
    /* main loop */
    while (-1) {
        /* adjust eocp */
        while (h.eocp != h.maxy && !tf -> line[h.eocp][n]) h.eocp++;
        
        /* update display */
        for (i = 0; i < tf -> lc; i++) {
            /** if (h.changed[i]) { /**/
                dltfield(tf, i);
                h.changed[i] = 0;
            /** } /**
            printf("%c%c%c%02x", tf -> line[i][n] ? 'n' : '_', i == h.eocp
                    ? 'e' : '_', i == h.maxy ? 'm' : '_', h.len[i]); /**/
        }
        s_mvcur(c(tf -> linepos[h.r.y].x + h.r.x, tf -> linepos[h.r.y].y));
        
        /* input and main switch */
        k = s_read_key();
        switch (k.spc) {
        case NOTHING_SPECIAL:
            /* insert line if needed */
            if (h.len[h.eocp] == tf -> width) {
                if (h.maxy == tf -> lc - 1) break;
                shift_down(tf, ++h.eocp, h.maxy++, h.len);
                if (h.eocp != h.maxy) {
                    tf -> line[h.eocp - 1][n] = 0;
                    tf -> line[h.eocp][n] = 1;
                }
                for (i = h.maxy; i > h.eocp; i--) h.changed[i] = 1;
                if (h.r.x == tf -> width) {
                    h.r.x = 0;
                    h.r.y++;
                }
            }
            /* shift characters within current paragraph */
            if (h.r.y != h.eocp) {
                for (i = h.len[h.eocp]; i; i--)
                    tf -> line[h.eocp][i] = tf -> line[h.eocp][i - 1];
                tf -> line[h.eocp][0]
                = tf -> line[h.eocp - 1][tf -> width - 1];
                for (i2 = h.eocp - 1; i2 > h.r.y; i2--) {
                    for (i = tf -> width - 1; i; i--)
                        tf -> line[i2][i] = tf -> line[i2][i - 1];
                    tf -> line[i2][0] = tf -> line[i2 - 1][tf -> width - 1];
                }
                i = tf -> width - 1;
            } else i = h.len[h.r.y];
            for (; i > h.r.x; i--)
                tf -> line[h.r.y][i] = tf -> line[h.r.y][i - 1];
            /* insert character */
            tf -> line[h.r.y][h.r.x] = k.c;
            h.len[h.eocp]++;
            for (i = h.r.y; i <= h.eocp; i++) h.changed[i] = 1;
        case SPC_RIGHT:
            if (h.r.x < h.len[h.r.y] &&
                (h.r.y == h.eocp || h.r.x < tf -> width - 1)) {
                h.r.x++;
                break;
            }
        case SPC_DOWN:
            if (h.r.y == h.maxy) break;
            if (tf -> line[h.r.y++][n]) h.eocp++;
            if (k.spc & SPC_RIGHT_OR_NS) {
                h.r.x = 0;
                break;
            }
            goto check_x_coord;
        case SPC_LEFT:
        case SPC_BACK:
            if (h.r.x) {
                h.r.x--;
                goto check_key;
            }
        case SPC_UP:
            if (!h.r.y) break;
            if (tf -> line[--h.r.y][n]) h.eocp = h.r.y;
        check_x_coord:
            if ((k.spc & SPC_VERTICAL) && h.r.x <= h.len[h.r.y])
                goto adjust_rx;
        case SPC_END:
            h.r.x = h.len[h.r.y];
        adjust_rx:
            if (h.r.x == tf -> width &&
                !(tf -> line[h.r.y][n] || h.r.y == h.maxy)) h.r.x--;
        check_key:
            if (k.spc != SPC_BACK) break;
        case SPC_DEL:
            delete(tf, &h);
            break;
        case SPC_ENTER:
            add_newline(tf, &h);
            break;
        case SPC_HOME:
            h.r.x = 0;
            break;
        case SPC_ESC:
            return;
        }
    }
    return;
}

/* ------------------------ ftfield subfunctions ------------------------- */

void delete(struct tfield *tf, struct tf_handle *h)
{
    int i, i2, i3;
    short postrx;
    
    if (h -> r.y == h -> maxy && h -> r.x == h -> len[h -> r.y]) return;
    if (!tf -> line[h -> r.y][n] || h -> r.x != h -> len[h -> r.y]) {
        /* deleting a character */
        i2 = h -> len[h -> r.y] - 1;
        for (i = h -> r.x;  i < i2; i++)
            tf -> line[h -> r.y][i] = tf -> line[h -> r.y][i + 1];
        if (h -> r.y == h -> eocp) {
            tf -> line[h -> r.y][--h -> len[h -> r.y]] = '\0';
            h -> changed[h -> r.y] = 1;
            return;
        }
        h -> len[h -> r.y]--;
        i3 = tf -> width - 1;
        postrx = 1;
    } else { /* deleting a \n */
        tf -> line[h -> r.y][n] = 0;
        i3 = h -> r.x;
        postrx = tf -> width - h -> r.x;
        while (h -> eocp < h -> maxy && !tf -> line[h -> eocp][n])
            h -> eocp++;
    }
    /* shifting */
    for (i2 = h -> r.y + 1; i2 < h -> eocp; i2++) {
        for (i = i3; i < tf -> width; i++)
            tf -> line[i2 - 1][i] = tf -> line[i2][i - i3];
        for (i = 0; i < i3; i++)
            tf -> line[i2][i] = tf -> line[i2][postrx + i];
    }
    if (h -> len[h -> eocp] <= postrx) { /* shift up */
        i2 = i3 + h -> len[h -> eocp--];
        for (i = i3; i < i2; i++) {
            tf -> line[h -> eocp][i] = tf -> line[h -> eocp + 1][i - i3];
            tf -> line[h -> eocp + 1][i - i3] = '\0';
        }
        while (i < tf -> width)
            tf -> line[h -> eocp][i++] = '\0';
        h -> len[h -> eocp] = h -> len[h -> r.y] + h -> len[h -> eocp + 1];
        if (h -> eocp != h -> r.y) h -> len[h -> r.y] = tf -> width;
        h -> len[h -> eocp + 1] = 0;
        if (tf -> line[h -> eocp + 1][n]) {
            tf -> line[h -> eocp + 1][n] = 0;
            tf -> line[h -> eocp][n] = 1;
        }
        shift_up(tf, h -> eocp + 2, h -> maxy--, h -> len);
    } else { /* no shift up */
        for (i = i3; i < tf -> width; i++)
            tf -> line[h -> eocp - 1][i] = tf -> line[h -> eocp][i - i3];
        i2 = h -> len[h -> eocp] - postrx;
        for (i = 0; i < i2; i++)
            tf -> line[h -> eocp][i] = tf -> line[h -> eocp][postrx + i];
        while (i < h -> len[h -> eocp])
            tf -> line[h -> eocp][i++] = '\0';
        h -> len[h -> r.y] = tf -> width;
        h -> len[h -> eocp] -= postrx;
    }
    for (i = h -> r.y; i <= h -> eocp; i++) h -> changed[i] = 1;
    return;
}

void add_newline(struct tfield *tf, struct tf_handle *h)
{
    int case_n;
    short ix, iy, postrx, shift_top;
    
    /* determine case number */
    if (!h -> r.x) {
        if (!h -> r.y) case_n = 1; /* cursor at (0,0) */
        else if (tf -> line[h -> r.y - 1][n])
            case_n = 3; /* immediately after a newline */
        else {
            tf -> line[h -> r.y - 1][n] = 1;
            return;
        }
    } else {
        if (h -> r.x < h -> len[h -> eocp])
            case_n = 8; /* too long to fit without a new line */
        else if (h -> r.y == h -> eocp) case_n = (h -> r.y == h -> maxy) ?
        2:    /* cursor at last line */
        6;   /* cursor in a line with a newline */
        else case_n = 0;
    }
    if (h -> maxy == tf -> lc - 1) return;
    
    /* using case_n */
    if (case_n && case_n != 2) { /* 1, 3, 6, or 8 */
        if (case_n & 1) /* 1 or 3 */
            shift_top = h -> r.y;
        else /* 6 or 8 */
            shift_top = h -> eocp + 1;
        shift_down(tf, shift_top, h -> maxy, h -> len);
    }
    if (case_n == 6 ||
        (case_n == 8 && h -> r.y == h -> eocp && h -> r.y != h -> maxy))
        tf -> line[h -> r.y + 1][n] = 1;
    else tf -> line[h -> r.y][n] = 1;
    
    if (case_n & 1) /* 1 or 3 */
        goto an_change_h;
    if (case_n & 2) { /* 2 or 6 */
        for (ix = h -> r.x; ix < h -> len[h -> r.y]; ix++) {
            tf -> line[h -> r.y + 1][ix - h -> r.x]
            = tf -> line[h -> r.y][ix];
        }
        goto an_change_h;
    }
    
    /* 0 or 8 */
    postrx = tf -> width - h -> r.x;
    if (case_n) { /* 8 */
        for (ix = h -> r.x; ix < h -> len[h -> eocp]; ix++) {
            tf -> line[h -> eocp + 1][ix - h -> r.x]
            = tf -> line[h -> eocp][ix];
            tf -> line[h -> eocp][ix] = '\0';
        }
        iy = h -> eocp;
    } else { /* 0 */
        for (ix = h -> len[h -> eocp] + postrx - 1; ix >= postrx; ix--)
            tf -> line[h -> eocp][ix] = tf -> line[h -> eocp][ix - postrx];
        for (ix = h -> r.x; ix < tf -> width; ix++)
            tf -> line[h -> eocp][ix - h -> r.x]
        = tf -> line[h -> eocp - 1][ix];
        iy = h -> eocp - 1;
    }
    /* shifting within current paragraph */
    while (iy > h -> r.y) {
        for (ix = tf -> width - 1; ix >= postrx; ix--)
            tf -> line[iy][ix] = tf -> line[iy][ix - postrx];
        for (ix = h -> r.x; ix < tf -> width; ix++)
            tf -> line[iy][ix - h -> r.x] = tf -> line[iy - 1][ix];
        iy--;
    }
    for (ix = h -> r.x; ix < tf -> width; ix++)
        tf -> line[h -> r.y][ix] = '\0';
    if (h -> r.y != h -> eocp) {
        h -> len[h -> eocp] += h -> len[h -> r.y] - h -> r.x;
        if (h -> len[h -> eocp] > tf -> width) {
            h -> len[h -> eocp + 1] = h -> len[h -> eocp] - tf -> width;
            h -> len[h -> eocp] = tf -> width;
        }
    } else h -> len[h -> r.y + 1] += h -> len[h -> r.y] - h -> r.x;
    h -> len[h -> r.y] = h -> r.x;
  an_change_h:
    if (case_n) h -> maxy++;
    for (iy = h -> maxy; iy >= h -> r.y; iy--) h -> changed[iy] = 1;
    h -> eocp = ++h -> r.y;
    h -> r.x = 0;
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

/* ----------------- system-dependent functions (s_...) ------------------ */

void s_prepare()
{
    CHAR_INFO *sci;
    SMALL_RECT ssr;
    int i;
    
    s_h_in  = GetStdHandle(STD_INPUT_HANDLE);
    s_h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    
    sci = malloc(SCR_W * SCR_H * sizeof *sci);
    for (i = 0; i < SCR_W * SCR_H; i++) {
        sci[i].Char.AsciiChar = '\0';
        sci[i].Attributes = B_DARK_GREY | F_BLACK;
    }
    ssr = s_sr(0, 0, SCR_W - 1, SCR_H - 1);
    WriteConsoleOutput(s_h_out, sci, s_c(SCR_W, SCR_H), s_c(0, 0), &ssr);
    
    for (i = 0; i < 1500; i++) {
        sci[i].Char.AsciiChar = 177;
        sci[i].Attributes = B_DARK_GREY | F_BLACK;
    }
    ssr = s_sr(5, 4, 75, 22);
    WriteConsoleOutput(s_h_out, sci, s_c(71, 19), s_c(0, 0), &ssr);
    
    for (i = 0; i < 100; i++) {
        sci[i].Char.AsciiChar = '\0';
        sci[i].Attributes = B_WHITE | F_WHITE;
    }
    ssr = s_sr(4, 21, 38, 21);
    WriteConsoleOutput(s_h_out, sci, s_c(35, 1), s_c(0, 0), &ssr);
    ssr = s_sr(40, 21, 74, 21);
    WriteConsoleOutput(s_h_out, sci, s_c(35, 1), s_c(0, 0), &ssr);
    
    for (i = 0; i < 50; i++) {
        sci[i].Char.AsciiChar = 232;
        sci[i].Attributes = B_WHITE | F_BLACK;
    }
    ssr = s_sr(39, 3, 39, 21);
    WriteConsoleOutput(s_h_out, sci, s_c(1, 19), s_c(0, 0), &ssr);
    
    return;
}

key s_read_key()
{
    const short corr_arr[][2] = {
        {VK_RIGHT   , SPC_RIGHT },
        {VK_DOWN    , SPC_DOWN  },
        {VK_UP      , SPC_UP    },
        {VK_LEFT    , SPC_LEFT  },
        {VK_BACK    , SPC_BACK  },
        {VK_END     , SPC_END   },
        {VK_DELETE  , SPC_DEL   },
        {VK_RETURN  , SPC_ENTER },
        {VK_HOME    , SPC_HOME  },
        {VK_ESCAPE  , SPC_ESC   },
        {VK_CONTROL , SPC_CTRL  }
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
            if (d == (DWORD) corr_arr[i][0]) {
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
    DWORD d;
    
    WriteConsoleOutputCharacter(s_h_out, str, strlen(str), s_cfc(c), &d);
    
    return;
}

void s_printcis(char_info *arr, int len, coord c)
{
    SMALL_RECT ssr;
    CHAR_INFO *sci;
    int   i;
    
    sci = malloc(len * sizeof *sci);
    ssr = s_sr(c.x, c.y, c.x + len - 1, c.y);
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
