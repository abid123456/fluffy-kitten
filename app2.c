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
#define SPC_PRESSED     0x40
#define CTRL            0x01

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

#define undo_limit      300

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

typedef struct {
    int op_id;
    coord c;
} operation;


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
    int u_counter;
    int r_counter;
    operation u_stack[undo_limit];
    int u_first;
    int u_limiter;
};

/* ------------------------ function declarations ------------------------ */

struct tfield *tfield(short width, short lc, coord *linepos);
struct tfield *read_file(char *fname, short width, short lc, coord *linepos);
int  write_to_file(char *fname, struct tfield *tf);
void save(struct tfield *tf);
int  exit_prompt();
void mark_unsaved();

void dltfield(struct tfield *tf, short y);
void dtfield(struct tfield *tf);

void ftfield(struct tfield *tf);
int  operate(struct tfield *tf, struct tf_handle *h, operation o);

int  insert_char(struct tfield *tf, struct tf_handle *h, char c);
int  delete(struct tfield *tf, struct tf_handle *h);
int  add_newline(struct tfield *tf, struct tf_handle *h);
void shift_down(struct tfield *tf, short top, short bottom, short *len);
void shift_up(struct tfield *tf, short top, short bottom, short *len);
int  move_cursor(struct tfield *tf, struct tf_handle *h, char d);
void adjust_eocp(struct tfield *tf, struct tf_handle *h);

void scan_line(char *output, int alloc_size, coord c);
void await_e(coord c);

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

int file_read = 0;
char *current_fname;

int saved = 0;
char save_prompt[2];
const char str_saved[2] = {(char) 196, '\0'};
const char str_unsaved[2] = {(char) 193, '\0'};

const char right_arrow = 26;
const char left_arrow = 27;
int undoing = 0;

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
        current_fname = argv[1];
        tf = read_file(argv[1], width, lc, NULL);
        if (file_read == -1) saved = -1;
        else saved = 1;
        /* if (argc != 3) {
            printf("Error %d: Invalid number of arguments\n", 1);
            return 1;
        }
        x = (int) strtol(argv[1], NULL, 10);
        y = (int) strtol(argv[2], NULL, 10); */
    } else {
        tf = tfield(width, lc, NULL);
        saved = -1;
        /* x = 10;
        y = 10; */
    }
    s_prepare();
    for (i = 0; i < tf -> lc / 2; i++) {
        tf -> linepos[i] = c(4, i + 3);
        tf -> linepos[i + tf -> lc / 2] = c(40, i + 3);
    }
    
    dtfield(tf);
    if (file_read == 1) s_pstrat(current_fname, c(6, 2));
    ftfield(tf);
    
    return 0;
}

struct tfield *tfield(short width, short lc, coord *linepos)
{
    struct tfield *tf;
    int ix, iy;
    
    tf = malloc(sizeof *tf);
    tf -> width   = width;
    tf -> lc      = lc;
    tf -> linepos = malloc(lc * sizeof *tf -> linepos);
    tf -> line    = malloc(lc * sizeof *tf -> line);
    
    for (iy = 0; iy < lc; iy++) {
        tf -> line[iy] = malloc((width + 2) * sizeof **tf -> line);
        for (ix = 0; ix < width + 1; ix++) tf -> line[iy][ix] = '\0';
        tf -> line[iy][width + 1] = 0;
    }
    if (linepos != NULL)
        for (iy = 0; iy < lc; iy++) tf -> linepos[iy] = linepos[iy];
    
    return tf;
}

struct tfield *read_file(char *fname, short width, short lc, coord *linepos)
{
    struct tfield *tf;
    short x, y;
    char c;
    FILE *file;
    
    tf = tfield(width, lc, linepos);
    file = fopen(fname, "r");
    if (file == NULL) {
        file_read = -1;
        return tf;
    }
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
    fclose(file);
    
    file_read = 1;
    return tf;
}

int write_to_file(char *fname, struct tfield *tf)
{
    short x, y;
    char c;
    FILE *file;
    
    file = fopen(fname, "w");
    if (file == NULL) return -1;
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
    fclose(file);
    
    return 0;
}

void save(struct tfield *tf)
{
    int i;
    
    if (saved == -1) {
        while (-1) {
            current_fname = malloc(60 * sizeof *current_fname);
            s_pstrat("File name :          ", c(4, 1));
            scan_line(current_fname, 60, c(16 ,1));
            for (i = 0; i < 71; i++) s_pstrat(" ", c(4 + i, 1));
            
            if (current_fname[0] == '\0') {
                free(current_fname);
                break;
            }
            if (write_to_file(current_fname, tf) != 0) {
                s_pstrat("File cannot be saved.", c(4, 1));
                await_e(c(25, 1));
                free(current_fname);
                continue;
            }
            saved = 1;
            s_pstrat(current_fname, c(6, 2));
            break;
        }
        return;
    }
    
    if (write_to_file(current_fname, tf) != 0) {
        s_pstrat("File cannot be saved.", c(4, 1));
        await_e(c(25, 1));
    } else if (saved == 0) saved = 1;
    
    return;
}

int exit_prompt()
{
    int i;
    
    s_pstrat("Exit without saving (y/n)?  ", c(4, 1));
    while (-1) {
        scan_line(save_prompt, 2, c(31, 1));
        if (*save_prompt == 'y' || *save_prompt == 'Y')
            return 1;
        if (*save_prompt == 'n' || *save_prompt == 'N') {
            for (i = 0; i < 28; i++) s_pstrat(" ", c(4 + i, 1));
            return 0;
        }
        s_pstrat(" ", c(31, 1));
    }
    return 0;
}

void mark_unsaved()
{
    if (saved != -1) saved = 0;
    return;
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
    int iy;
    
    for (iy = 0; iy < tf -> lc; iy++) dltfield(tf, iy);
    
    return;
}

void ftfield(struct tfield *tf)
{
    struct tf_handle h;
    
    int iy;
    operation o;
    key k;
    char u_display[6];
    int k_c_ns, h_r_counter_temp;
    
    /* init some variables */
    n = tf -> width + 1;
    
    k_c_ns = 0;
    
    h.r.x = 0;
    h.r.y = 0;
    
    h.eocp = 0;
    h.changed = calloc(tf -> lc, sizeof *h.changed);
    h.len = calloc(tf -> lc, sizeof *h.len);
    
    h.u_counter = 0;
    h.r_counter = 0;
    h.u_first = 0;
    h.u_limiter = 0;
    
    /* init len members and maxy */
    h.maxy = -1;
    for (iy = 0; iy < tf -> lc; iy++) {
        h.len[iy] = strlen(tf -> line[iy]);
        if (!h.len[iy] && !tf -> line[iy][n]) {
            if (!iy) h.maxy = 0;
            else if (tf -> line[iy - 1][n]) h.maxy = iy;
            else h.maxy = iy - 1;
            break;
        }
    }
    if (h.maxy == -1) h.maxy = tf -> lc - 1;
    
    /* main loop */
    while (-1) {
        adjust_eocp(tf, &h);
        
        /* update display */
        for (iy = 0; iy < tf -> lc; iy++) {
            if (h.changed[iy]) {
                dltfield(tf, iy);
                h.changed[iy] = 0;
            } /**
            printf("%c%c%c%02x", tf -> line[iy][n] ? 'n' : '_', iy == h.eocp
                    ? 'e' : '_', iy == h.maxy ? 'm' : '_', h.len[iy]); /**/
        }
        
        s_mvcur(c(tf -> linepos[h.r.y].x + h.r.x, tf -> linepos[h.r.y].y));
        
        if (saved != 1)  s_pstrat(str_unsaved, c(4, 2));
        else s_pstrat(str_saved, c(4, 2));
        sprintf(u_display, "%c %d  ", left_arrow, h.u_counter);
        s_pstrat(u_display, c(4, 0));
        sprintf(u_display, "%c %d  ", right_arrow, h.r_counter);
        s_pstrat(u_display, c(10, 0));
        
        /* input and main switch */
        k = s_read_key();
        switch (k.spc) {
          case NOTHING_SPECIAL:
            k_c_ns = 0;
            switch ((int) k.c) {
              case 19: /* if saving */
                save(tf);
                break;
              case 26: /* if undoing */
                if (!h.u_counter) break;
                if (--h.u_limiter < 0) h.u_limiter = undo_limit - 1;
                undoing = 1;
                operate(tf, &h, h.u_stack[h.u_limiter]);
                undoing = 0;
                h.u_counter--;
                h.r_counter++;
                break;
              case 25: /* if redoing */
                if (!h.r_counter) break;
                h_r_counter_temp = h.r_counter - 1;
                if (h.u_stack[h.u_limiter].op_id & 0xff == NOTHING_SPECIAL)
                    k_c_ns = -1;
                operate(tf, &h, h.u_stack[h.u_limiter]);
                h.r_counter = h_r_counter_temp;
                break;
              default:
                k_c_ns = 1;
                break;
            }
            if (!k_c_ns) break;
            if (k_c_ns == -1) goto move_right;
            o.op_id = k.spc + 0x100 * (unsigned char) k.c;
            o.c = h.r;
            if (operate(tf, &h, o) != 0) break;
            mark_unsaved();
           move_right:
            k.spc = SPC_RIGHT;
          case SPC_RIGHT:
          case SPC_DOWN:
          case SPC_LEFT:
          case SPC_BACK:
          case SPC_UP:
          case SPC_HOME:
          case SPC_END:
            if (move_cursor(tf, &h, k.spc) != 0) break;
            if (k.spc != SPC_BACK) break;
            k.spc = SPC_DEL;
          case SPC_DEL:
          case SPC_ENTER:
            o.op_id = k.spc;
            o.c = h.r;
            operate(tf, &h, o);
            mark_unsaved();
            break;
          case SPC_ESC:
            switch (saved) {
              case -1:
                if (h.maxy == 0 && h.len[0] == 0) return;
              case 0:
                if (!exit_prompt()) break;
              case 1:
                return;
            }
        }
    }
    return;
}

/* ------------------------ ftfield subfunctions ------------------------- */

int operate(struct tfield *tf, struct tf_handle *h, operation o)
{
    int r;
    operation o_inverse;
    /**/ char str[100]; /**/
    
    h -> r = o.c;
    h -> eocp = h -> r.y;
    adjust_eocp(tf, h);
    /**/
    sprintf(str, "(%d,%d),e=%d          ",
    h -> r.x, h -> r.y, h -> eocp);
    s_pstrat(str, c(15,0)); /**/
    switch (o.op_id & 0xff) {
      case NOTHING_SPECIAL:
        o.op_id >>= 8;
        /**/ sprintf(str, "%d   ", o.op_id);
        s_pstrat(str, c(0,0)); /**/
        if ((r = insert_char(tf, h, (char) o.op_id)) < 0) return r;
        o_inverse.c = o.c;
        o_inverse.op_id = SPC_DEL;
        break;
      case SPC_DEL:
        if ((r = delete(tf, h)) < 0) return r;
        o_inverse.c = h -> r;
        o_inverse.op_id = r == 0xffff ?
                        SPC_ENTER : (NOTHING_SPECIAL + 0x100 * r);
        break;
      case SPC_ENTER:
        if ((r = add_newline(tf, h)) < 0) return r;
        o_inverse.c = o.c;
        o_inverse.op_id = SPC_DEL;
        break;
    }
    
    h -> u_stack[h -> u_limiter] = o_inverse;
    if (!undoing) {
        if (++h -> u_limiter == undo_limit) h -> u_limiter = 0;
        if (h -> u_counter == undo_limit) h -> u_first = h -> u_limiter;
        else h -> u_counter++;
        h -> r_counter = 0;
    }
    
    return 0;
}

int insert_char(struct tfield *tf, struct tf_handle *h, char c)
{
    int ix, iy;
    
    /* insert line if needed */
    if (h -> len[h -> eocp] == tf -> width) {
        if (h -> maxy == tf -> lc - 1) return -1;
        shift_down(tf, ++h -> eocp, h -> maxy++, h -> len);
        if (h -> eocp != h -> maxy) {
            tf -> line[h -> eocp - 1][n] = 0;
            tf -> line[h -> eocp][n] = 1;
        }
        for (iy = h -> maxy; iy > h -> eocp; iy--) h -> changed[iy] = 1;
        if (h -> r.x == tf -> width) {
            h -> r.x = 0;
            h -> r.y++;
        }
    }
    /* shift characters within current paragraph */
    if (h -> r.y != h -> eocp) {
        for (ix = h -> len[h -> eocp]; ix; ix--)
            tf -> line[h -> eocp][ix] = tf -> line[h -> eocp][ix - 1];
        tf -> line[h -> eocp][0] =
          tf -> line[h -> eocp - 1][tf -> width - 1];
        for (iy = h -> eocp - 1; iy > h -> r.y; iy--) {
            for (ix = tf -> width - 1; ix; ix--)
                tf -> line[iy][ix] = tf -> line[iy][ix - 1];
            tf -> line[iy][0] = tf -> line[iy - 1][tf -> width - 1];
        }
        ix = tf -> width - 1;
    } else ix = h -> len[h -> r.y];
    for (; ix > h -> r.x; ix--)
        tf -> line[h -> r.y][ix] = tf -> line[h -> r.y][ix - 1];
    /* insert character */
    tf -> line[h -> r.y][h -> r.x] = c;
    h -> len[h -> eocp]++;
    for (iy = h -> r.y; iy <= h -> eocp; iy++) h -> changed[iy] = 1;
    
    return 0;
}

int delete(struct tfield *tf, struct tf_handle *h)
{
    int ix, iy, ix_limiter, lshift_length, r;
    short postrx;
    
    if (h -> r.y == h -> maxy && h -> r.x == h -> len[h -> r.y]) return -1;
    if (!tf -> line[h -> r.y][n] || h -> r.x != h -> len[h -> r.y]) {
        /* deleting a character */
        r = (unsigned char) tf -> line[h -> r.y][h -> r.x];
        ix_limiter = h -> len[h -> r.y] - 1;
        for (ix = h -> r.x;  ix < ix_limiter; ix++)
            tf -> line[h -> r.y][ix] = tf -> line[h -> r.y][ix + 1];
        if (h -> r.y == h -> eocp) {
            tf -> line[h -> r.y][--h -> len[h -> r.y]] = '\0';
            h -> changed[h -> r.y] = 1;
            return r;
        }
        h -> len[h -> r.y]--;
        lshift_length = tf -> width - 1;
        postrx = 1;
        r = 0;
    } else { /* deleting a \n */
        r = 0xffff;
        tf -> line[h -> r.y][n] = 0;
        if (h -> r.x == tf -> width) {
            h -> r.x = 0;
            h -> r.y++;
            return r;
        }
        lshift_length = h -> r.x;
        postrx = tf -> width - h -> r.x;
        adjust_eocp(tf, h);
    }
    /* shifting */
    for (iy = h -> r.y + 1; iy < h -> eocp; iy++) {
        for (ix = lshift_length; ix < tf -> width; ix++)
            tf -> line[iy - 1][ix] = tf -> line[iy][ix - lshift_length];
        for (ix = 0; ix < lshift_length; ix++)
            tf -> line[iy][ix] = tf -> line[iy][postrx + ix];
    }
    if (h -> len[h -> eocp] <= postrx) { /* shift up */
        ix_limiter = lshift_length + h -> len[h -> eocp--];
        for (ix = lshift_length; ix < ix_limiter; ix++) {
            tf -> line[h -> eocp][ix] = tf -> line[h -> eocp + 1][ix - lshift_length];
            tf -> line[h -> eocp + 1][ix - lshift_length] = '\0';
        }
        while (ix < tf -> width)
            tf -> line[h -> eocp][ix++] = '\0';
        h -> len[h -> eocp] = h -> len[h -> r.y] + h -> len[h -> eocp + 1];
        if (h -> eocp != h -> r.y) h -> len[h -> r.y] = tf -> width;
        h -> len[h -> eocp + 1] = 0;
        if (tf -> line[h -> eocp + 1][n]) {
            tf -> line[h -> eocp + 1][n] = 0;
            tf -> line[h -> eocp][n] = 1;
        }
        shift_up(tf, h -> eocp + 2, h -> maxy, h -> len);
        for (iy = h -> eocp + 1; iy <= h -> maxy; iy++) h -> changed[iy] = 1;
        h -> maxy--;
    } else { /* no shift up */
        for (ix = lshift_length; ix < tf -> width; ix++)
            tf -> line[h -> eocp - 1][ix] = tf -> line[h -> eocp][ix - lshift_length];
        ix_limiter = h -> len[h -> eocp] - postrx;
        for (ix = 0; ix < ix_limiter; ix++)
            tf -> line[h -> eocp][ix] = tf -> line[h -> eocp][postrx + ix];
        while (ix < h -> len[h -> eocp])
            tf -> line[h -> eocp][ix++] = '\0';
        h -> len[h -> r.y] = tf -> width;
        h -> len[h -> eocp] -= postrx;
    }
    for (iy = h -> r.y; iy <= h -> eocp; iy++) h -> changed[iy] = 1;
    return r;
}

int add_newline(struct tfield *tf, struct tf_handle *h)
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
            return 0;
        }
    } else {
        if (h -> r.x < h -> len[h -> eocp])
            case_n = 8; /* too long to fit without a new line */
        else if (h -> r.y == h -> eocp) case_n = (h -> r.y == h -> maxy) ?
        2:    /* cursor at last line */
        6;   /* cursor in a line with a newline */
        else case_n = 0;
    }
    if (case_n && h -> maxy == tf -> lc - 1) return -1;
    
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
        for (ix = h -> r.x; ix < tf -> width; ix++) {
            tf -> line[h -> eocp][ix - h -> r.x] =
              tf -> line[h -> eocp - 1][ix];
        }
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
    for (iy = h -> r.y; iy <= h -> maxy; iy++) h -> changed[iy] = 1;
    h -> eocp = ++h -> r.y;
    h -> r.x = 0;
    return 0;
}

void shift_down(struct tfield *tf, short top, short bottom, short *len)
{
    char *linebuf;
    short lenbuf;
    int   iy;
    
    linebuf = tf -> line[++bottom];
    lenbuf  = len[bottom];
    for (iy = bottom; iy > top; iy--) {
        tf -> line[iy] = tf -> line[iy - 1];
        len[iy] = len[iy - 1];
    }
    tf -> line[top] = linebuf;
    len[top] = lenbuf;
    
    return;
}

void shift_up(struct tfield *tf, short top, short bottom, short *len)
{
    char *linebuf;
    short lenbuf;
    int   iy;
    
    linebuf = tf -> line[--top];
    lenbuf  = len[top];
    for (iy = top; iy < bottom; iy++) {
        tf -> line[iy] = tf -> line[iy + 1];
        len[iy] = len[iy + 1];
    }
    tf -> line[bottom] = linebuf;
    len[bottom] = lenbuf;
    return;
}

int move_cursor(struct tfield *tf, struct tf_handle *h, char d)
{
    switch (d) {
      case SPC_RIGHT:
        if (h -> r.x < h -> len[h -> r.y] &&
            (h -> r.y == h -> eocp || h -> r.x < tf -> width - 1)) {
            h -> r.x++;
            return 0;
        }
      case SPC_DOWN:
        if (h -> r.y == h -> maxy) return -1;
        if (tf -> line[h -> r.y++][n]) h -> eocp++;
        if (d & SPC_RIGHT_OR_NS) goto cr;
        goto check_x_coord;
      case SPC_LEFT:
      case SPC_BACK:
        if (h -> r.x) {
            h -> r.x--;
            return 0;
        }
      case SPC_UP:
        if (!h -> r.y) return -2;
        if (tf -> line[--h -> r.y][n]) h -> eocp = h -> r.y;
       check_x_coord:
        if ((d & SPC_VERTICAL) && h -> r.x <= h -> len[h -> r.y])
            goto adjust_rx;
      case SPC_END:
        h -> r.x = h -> len[h -> r.y];
       adjust_rx:
        if (h -> r.x == tf -> width &&
            !(tf -> line[h -> r.y][n] || h -> r.y == h -> maxy)) h -> r.x--;
        return 0;
      case SPC_HOME:
       cr:
        h -> r.x = 0;
        return 0;
    }
}

void adjust_eocp(struct tfield *tf, struct tf_handle *h)
{
    while (h -> eocp != h -> maxy && !tf -> line[h -> eocp][n]) h -> eocp++;
    return;
}

/* -------------------------- support functions -------------------------- */

void scan_line(char *output, int alloc_size, coord c)
{
    key k;
    coord c_out;
    int i, i_del, done;
    
    for (i = 0; i < alloc_size; i++) output[i] = '\0';
    i = 0;
    done = 0;
    c_out.y = c.y;
    while (-1) {
        c_out.x = c.x + i;
        s_mvcur(c_out);
        s_pstrat(output, c);
        
        k = s_read_key();
        switch (k.spc) {
          case NOTHING_SPECIAL:
            if (i == alloc_size - 1) break;
            output[i++] = k.c;
            break;
          case SPC_ENTER:
            done = 1;
            break;
          case SPC_ESC:
            done = -1;
            break;
          case SPC_LEFT:
          case SPC_BACK:
            if (!i) break;
            i--;
            if (k.spc == SPC_LEFT) break;
            goto del_a;
          case SPC_DEL:
            if (output[i] == '\0') break;
           del_a:
            for (i_del = i; output[i_del + 1] != '\0'; i_del++)
                output[i_del] = output[i_del + 1];
            output[i_del] = '\0';
            c_out.x = c.x + i_del;
            s_pstrat(" ", c_out);
            break;
          case SPC_RIGHT:
            if (output[i] == '\0') break;
            i++;
            break;
          case SPC_HOME:
            i = 0;
            break;
          case SPC_END:
            while (output[i] != '\0') i++;
            break;
        }
        if (!done) continue;
        if (done == -1) {
            for (i = 0; output[i] != '\0'; i++) {
                c_out.x = c.x + i;
                s_pstrat(" ", c_out);
            }
            output[0] = '\0';
        }
        return;
    }
}

void await_e(coord c)
{
    char buffer[1];
    
    scan_line(buffer, 1, c);
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
    SetConsoleMode(s_h_in, ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
    
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
        /*if (ir.Event.KeyEvent.dwControlKeyState &
            (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
            k.spc = SPC_PRESSED|CTRL;
        }*/
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
    int i;
    
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
