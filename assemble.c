#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define s_prog 1
#define s_data 2
#define s_var  3
#define s_code 4

#define t_string 0
#define t_byte   8
#define t_short  16

#define a_reg 0
#define a_adr 1
#define a_ptr 2
#define a_cst 3

#define c_nop  0
#define c_mov  1
#define c_movb 2
#define c_inc  3
#define c_out  4
#define c_je   5
#define c_jne  6
#define c_exit 7

#define oc_MOV  0x10
#define oc_MOVB 0x18

#define op28_R_R 0x00
#define op28_R_I 0x01
#define op28_M_I 0x02
#define op28_D_I 0x03
#define op28_M_R 0x04
#define op28_D_R 0x05
#define op28_R_M 0x06
#define op28_R_D 0x07

char line[102];
int line_number;

typedef struct {
    char *id;
    int addr;
} reg_i_a;

const reg_i_a const registers[] = {
    {.id = "ax", .addr = 0},
    {.id = "bx", .addr = 2},
    {.id = "cx", .addr = 4},
    {.id = "dx", .addr = 6},
    {.id = "al", .addr = 0},
    {.id = "ah", .addr = 1},
    {.id = "bl", .addr = 2},
    {.id = "bh", .addr = 3},
    {.id = "cl", .addr = 4},
    {.id = "ch", .addr = 5},
    {.id = "dl", .addr = 6},
    {.id = "dh", .addr = 7},
    {.id = "si", .addr = 8},
    {.id = "fl", .addr = 10},
    {.id = "sp", .addr = 12},
    {.id = "ip", .addr = 14},
};

const int num_registers = sizeof (registers) / sizeof *registers;

void error(const char *str)
{
    printf("Error : %s (at line %d).\n", str, line_number);
    exit(-1);
}

char *find_token(int *start_point)
{
    char *r;
    
    while (isspace(line[*start_point])) {
        ++*start_point;
        if (line[*start_point] == '\0') return NULL;
    }
    r = line + *start_point;
    while (!isspace(line[*start_point])) ++*start_point;
    line[*start_point] = '\0';
    ++*start_point;
    
    return r;
}

typedef struct {
    char name[21];
    unsigned short address;
} location;

union data {
    unsigned short constant;
    char string[30];
};

typedef struct {
    unsigned short size;
    int type;
    union data data;
} data_entry;

int main(int argc, char *argv[])
{
    char *filename;
    char section_name[10];
    char *token;
    int i, i_sn, i_string;
    int section, s_temp;
    FILE *fp, *fp_output;
    int code_length;
    location locs[100];
    data_entry data_entries[30];
    int n_locs, i_de, i_locs;
    unsigned char disk_file[65536];
    unsigned short curr_addr;
    unsigned short curr_data_addr;
    unsigned short curr_var_addr;
    int type, atoi_buf;
    int arg_1, arg_2, arg_1_type, arg_2_type, i_arg, n_arg;
    int op_name;
    int end, mwc;
    int reg_addr;
    int i_registers;
    
    for (i = 0; i < 65536; i++)
        disk_file[i] = 0;
    curr_addr = 0;
    curr_data_addr = 0;
    curr_var_addr = 4096;
    if (argc >= 2) filename = argv[1];
    else {
        filename = malloc(256 * sizeof *filename);
        printf("Enter name of file to assemble: ");
        fgets(filename, 256, stdin);
    }
    fp = fopen(filename, "r");
    if (argc == 1) free(filename);
    
    section = 0;
    n_locs = 0;
    i_de = 0;
    line_number = 0;
    end = 0;
    while (-1) {
        fgets(line, 102, fp);
        line_number++;
        printf("line %d: length %d", line_number, strlen(line));
        if (feof(fp)) printf(", feof(fp)");
        puts("");
        printf("line: \"%s\"\n", line);
        printf("processing %dth line\n", line_number);
        for (i = 0; i < 102; i++) {
            if (line[i] == '\n') {
                line[i] = ' ';
                break;
            }
            if (line[i] == '\0') break;
            if (i == 100)
                error("line too long");
        }
        
        printf("checkpoint 1");
        i = 0;
        mwc = 0;
        while (isspace(line[i])) {
            i++;
            if (line[i] == '\0') {
                mwc = 1;
                break;
            }
        }
        if (mwc) continue;
        printf("checkpoint 2\n");
        if (line[i] == '.') {
            i++;
            for (i_sn = 0; (line[i] != ':' && line[i] != '.'); i_sn++) {
                if (i_sn == 9) error("section name too long");
                if (line[i] == '\0') error("unterminated section name");
                section_name[i_sn] = line[i];
                i++;
            }
            if (line[i] == '.') {
                section_name[i_sn] = '\0';
                printf("section_name : \"%s\"\n", section_name);
                if (strcmp(section_name, "end"))
                    error("incorrectly terminated section name");
                end = 1;
                break;
            }
            section_name[i_sn] = '\0';
            
            s_temp = 0;
            if (!strcmp(section_name, "prog")) s_temp = s_prog;
            if (!strcmp(section_name, "data")) s_temp = s_data;
            if (!strcmp(section_name, "var"))  s_temp = s_var;
            if (!strcmp(section_name, "code")) s_temp = s_code;
            if (s_temp - section != 1)
                error("incorrect section order or "
                        "unrecognised section name");
            section = s_temp;
            continue;
        }
        
        if (line[i] == ';') continue; /* comment */
        
        printf("checkpoint 3\n");
        switch (section) {
          case s_prog: /* ========== prog section ========== */
            printf("processing prog section line\n");
            if ((token = find_token(&i)) == NULL) continue;
            if (!strcmp(token, "code_length")) {
                if ((token = find_token(&i)) == NULL) {
                    error("argument of code_length unspecified");
                }
                code_length = atoi(token);
                if (code_length <= 0) error("code_length value invalid");
                curr_data_addr = code_length;
            }
            break;
          case s_data:/* ========== data section ========== */
            printf("processing data section line\n");
            printf("value of i: %d\n", i);
            printf("the line: \"%s\"\n", line);
            if ((token = find_token(&i)) == NULL) continue;
            printf("type token: \"%s\"\n", token);
            if (!strcmp(token, "string"))     type = t_string;
            else if (!strcmp(token, "byte"))  type = t_byte;
            else if (!strcmp(token, "short")) type = t_short;
            else error("invalid type");
            locs[n_locs].address = curr_data_addr;
            if ((token = find_token(&i)) == NULL)
                error("data identifier unspecified");
            for (i_string = 0; i_string < 21; i_string++)
                locs[n_locs].name[i_string] = '\0';
            strncpy(locs[n_locs].name, token, 20);
            if (locs[n_locs].name[0] == '_') {
                if (locs[n_locs].name[1] == 'r')
                    error("invalid identifier(1)");
            }
            if (locs[n_locs].name[0] == '[') {
                error("invalid identifier(2)");
            }
            if (isdigit(locs[n_locs].name[0])) {
                error("invalid identifier(3)");
            }
            for (i_registers = 0; i_registers < num_registers;
             i_registers++) {
                if (!strcmp(locs[n_locs].name,
                 registers[i_registers].id)) {
                    error("invalid identifier (4)");
                }
            }
            n_locs++;
            
            switch (type) {
              case t_string:
                puts("processing string data");
                while (isspace(line[i])) {
                    i++;
                    if (line[i] == '\0') error("data value unspecified");
                }
                
                if (line[i++] != '"')
                    error("starting quotation mark not found");
                while (line[i] != '"') {
                    if (line[i] == '\\') {
                        i++;
                        disk_file[curr_data_addr++] = line[i++];
                        if (line[i] == '\0')
                            error("terminating quotation mark not found");
                    } else disk_file[curr_data_addr++] = line[i++];
                }
                disk_file[curr_data_addr++] = '\0';
                break;
              case t_byte:
                puts("processing byte data");
                if ((token = find_token(&i)) == NULL)
                    error("data value unspecified");
                
                atoi_buf = atoi(line + i);
                atoi_buf -= (atoi_buf >> 8) << 8;
                disk_file[curr_data_addr++] = atoi_buf;
                break;
              case t_short:
                puts("processing short data");
                if ((token = find_token(&i)) == NULL)
                    error("data value unspecified");
                
                atoi_buf = atoi(token);
                printf("short data, atoi_buf = %d\n", atoi_buf);
                atoi_buf -= (atoi_buf >> 16) << 16;
                disk_file[curr_data_addr + 1] = atoi_buf >> 8;
                atoi_buf -= (atoi_buf >> 8) << 8;
                disk_file[curr_data_addr] = atoi_buf;
                curr_data_addr += 2;
                break;
            }
            break;
          case s_var: /* ========== var section ========== */
            printf("processing var section line\n");
            if ((token = find_token(&i)) == NULL) continue;
            if (strcmp(token, "var")) error("\"var\" keyword not found");
            if ((token = find_token(&i)) == NULL)
                error("variable size not specified");
            if ((atoi_buf = atoi(token)) <= 0)
                error("invalid variable size");
            if ((token = find_token(&i)) == NULL)
                error("variable name not specified");
            
            locs[n_locs].address = curr_var_addr;
            curr_var_addr += atoi_buf;
            for (i_string = 0; i_string < 21; i_string++)
                locs[n_locs].name[i_string] = '\0';
            strncpy(locs[n_locs].name, token, 20);
            if (locs[n_locs].name[0] == '_') {
                if (locs[n_locs].name[1] == 'r')
                    error("invalid identifier(1)");
            }
            if (locs[n_locs].name[0] == '[') {
                error("invalid identifier(2)");
            }
            if (isdigit(locs[n_locs].name[0])) {
                error("invalid identifier(3)");
            }
            for (i_registers = 0; i_registers < num_registers;
             i_registers++) {
                if (!strcmp(locs[n_locs].name,
                 registers[i_registers].id)) {
                    error("invalid identifier (4)");
                }
            }
            n_locs++;
            break;
          case s_code: /* ========== code section ========== */
            printf("processing code section line\n");
            if ((token = find_token(&i)) == NULL) continue;
            /* ---------- process jump label ---------- */
            if (token[strlen(token) - 1] == ':') {
                printf("assigned address names (n_locs = %d):\n", n_locs);
                for (i_locs = 0; i_locs < n_locs; i_locs++) {
                    printf("locs[%d] : name %s, address %04x\n", i_locs,
                            locs[i_locs].name, locs[i_locs].address);
                }
                puts("processing jump label");
                locs[n_locs].address = curr_addr;
                token[strlen(token) - 1] = '\0';
                for (i_string = 0; i_string < 21; i_string++)
                    locs[n_locs].name[i_string] = '\0';
                strncpy(locs[n_locs].name, token, 20);
                if (locs[n_locs].name[0] == '_') {
                    if (locs[n_locs].name[1] == 'r')
                        error("invalid identifier(1)");
                }
                if (locs[n_locs].name[0] == '[') {
                    error("invalid identifier(2)");
                }
                if (isdigit(locs[n_locs].name[0])) {
                    error("invalid identifier(3)");
                }
                for (i_registers = 0; i_registers < num_registers;
                 i_registers++) {
                    if (!strcmp(locs[n_locs].name,
                     registers[i_registers].id)) {
                        error("invalid identifier (4)");
                    }
                }
                n_locs++;
                printf("assigned address names (n_locs = %d):\n", n_locs);
                for (i_locs = 0; i_locs < n_locs; i_locs++) {
                    printf("locs[%d] : name %s, address %04x\n", i_locs,
                            locs[i_locs].name, locs[i_locs].address);
                }
                break;
            }
            
            /* ---------- process operation ---------- */
            if (!strcmp(token, "nop"))  {op_name = c_nop , n_arg = 0;}
            if (!strcmp(token, "mov"))  {op_name = c_mov , n_arg = 2;}
            if (!strcmp(token, "movb")) {op_name = c_movb, n_arg = 2;}
            if (!strcmp(token, "inc"))  {op_name = c_inc , n_arg = 1;}
            if (!strcmp(token, "out"))  {op_name = c_out , n_arg = 1;}
            if (!strcmp(token, "je"))   {op_name = c_je  , n_arg = 2;}
            if (!strcmp(token, "jne"))  {op_name = c_jne , n_arg = 2;}
            if (!strcmp(token, "exit")) {op_name = c_exit, n_arg = 0;}
            
            /* ---------- get arguments from token ---------- */
            for (i_arg = 0; i_arg < n_arg; i_arg++) {
                if ((token = find_token(&i)) == NULL) {
                    if (i_arg == 0) error("argument not found");
                    error("second argument not found");
                }
                /* ---------- get argument and argument type ---------- */
                reg_addr = -1;
                for (i_registers = 0; i_registers < num_registers;
                 i_registers++) {
                    if (!strcmp(token, registers[i_registers].id)) {
                        reg_addr = registers[i_registers].addr;
                    }
                }
                if (reg_addr != -1) { /* register (specified with name) */
                    if (i_arg == 0){
                        arg_1 = reg_addr;
                        arg_1_type = a_reg;
                    } else {
                        arg_2 = reg_addr;
                        arg_2_type = a_reg;
                    }
                } else if (token[0] == '_') { /* reg. (spec. with number) */
                    if (token[1] == 'r') {
                        if (token[2] == '\0')
                            error("register number not found");
                        atoi_buf = atoi(token + 2);
                        if (atoi_buf <= 0 || atoi_buf > 16)
                            error("invalid register number");
                        if (i_arg == 0){
                            arg_1 = atoi_buf - 1;
                            arg_1_type = a_reg;
                        } else {
                            arg_2 = atoi_buf - 1;
                            arg_2_type = a_reg;
                        }
                    } else error("invalid argument identifier");
                } else if (token[0] == '[') { /* pointer */
                    if (token[strlen(token) - 1] != ']')
                        error("closing ']' not found");
                    token[strlen(token) - 1] = '\0';
                    token++;
                    for (i_locs = 0; i_locs < n_locs; i_locs++) {
                        if (!strcmp(locs[i_locs].name, token)) break;
                        if (i_locs == n_locs - 1)
                            error("pointer name undeclared");
                    }
                    if (i_arg == 0){
                        arg_1 = locs[i_locs].address;
                        arg_1_type = a_ptr;
                    } else {
                        arg_2 = locs[i_locs].address;
                        arg_2_type = a_ptr;
                    }
                } else if (isdigit(token[0])) { /* constant */
                    for (i_string = strlen(token) - 1;
                      i_string >= 0; i_string--) {
                        if (!isdigit(token[i_string]))
                            error("invalid constant value");
                    }
                    atoi_buf = atoi(token);
                    if (i_arg == 0){
                        arg_1 = atoi_buf;
                        arg_1_type = a_cst;
                    } else {
                        arg_2 = atoi_buf;
                        arg_2_type = a_cst;
                    }
                } else if (token[0] == '<') { /* location address */
                    if (token[strlen(token) - 1] != '>')
                        error("closing '>' not found");
                    token[strlen(token) - 1] = '\0';
                    token++;
                    for (i_locs = 0; i_locs < n_locs; i_locs++) {
                        if (!strcmp(locs[i_locs].name, token)) break;
                        if (i_locs == n_locs - 1)
                            error("pointer name undeclared");
                    }
                    if (i_arg == 0){
                        arg_1 = locs[i_locs].address;
                        arg_1_type = a_cst;
                    } else {
                        arg_2 = locs[i_locs].address;
                        arg_2_type = a_cst;
                    }
                } else { /* variable/address */
                    printf("assigned address names (n_locs = %d):\n",
                            n_locs);
                    for (i_locs = 0; i_locs < n_locs; i_locs++) {
                        printf("locs[%d] : name %s, address %04x\n", i_locs,
                                locs[i_locs].name, locs[i_locs].address);
                        if (!strcmp(locs[i_locs].name, token)) break;
                        if (i_locs == n_locs - 1)
                            error("variable name undeclared");
                    }
                    if (i_arg == 0){
                        arg_1 = locs[i_locs].address;
                        arg_1_type = a_adr;
                    } else {
                        arg_2 = locs[i_locs].address;
                        arg_2_type = a_adr;
                    }
                }
            }
            /* --------------- handle operations --------------- */
            /* --- handle nop --- */
            if (op_name == c_nop) {
                disk_file[curr_addr++] = 0x00;
            }
            /* --- mov operation --- */
            else if (op_name == c_mov) {
                disk_file[curr_addr] = oc_MOV;
                switch (arg_1_type) {
                  case a_reg:
                    switch (arg_2_type) {
                      case a_reg:
                        disk_file[curr_addr++] |= op28_R_R;
                        disk_file[curr_addr++] = arg_1;
                        disk_file[curr_addr++] = arg_2;
                        break;
                      case a_adr:
                      case a_ptr:
                        disk_file[curr_addr++] |=
                            (arg_2_type == a_adr) ? op28_R_M : op28_R_D;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        disk_file[curr_addr++] = arg_2 >> 8;
                        disk_file[curr_addr++] = arg_1;
                        break;
                      case a_cst:
                        disk_file[curr_addr++] |= op28_R_I;
                        disk_file[curr_addr++] = arg_1;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        disk_file[curr_addr++] = arg_2 >> 8;
                        break;
                    }
                    break;
                  case a_adr:
                    switch (arg_2_type) {
                      case a_reg:
                        disk_file[curr_addr++] |= op28_M_R;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] = arg_2;
                        break;
                      case a_cst:
                        disk_file[curr_addr++] |= op28_M_I;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        disk_file[curr_addr++] = arg_2 >> 8;
                        break;
                      default:
                        error("invalid second argument");
                    }
                    break;
                  case a_ptr:
                    switch (arg_2_type) {
                      case a_reg:
                        disk_file[curr_addr++] |= op28_D_R;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] = arg_2;
                        break;
                      case a_cst:
                        disk_file[curr_addr++] |= op28_D_I;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        disk_file[curr_addr++] = arg_2 >> 8;
                        break;
                      default:
                        error("invalid second argument");
                    }
                    break;
                  case a_cst:
                    error("constant as first argument of mov");
                }
            }
            /* --- movb operation --- */
            else if (op_name == c_movb) {
                disk_file[curr_addr] = oc_MOVB;
                switch (arg_1_type) {
                  case a_reg:
                    switch (arg_2_type) {
                      case a_reg:
                        disk_file[curr_addr++] |= op28_R_R;
                        disk_file[curr_addr++] = arg_1;
                        disk_file[curr_addr++] = arg_2;
                        break;
                      case a_adr:
                      case a_ptr:
                        disk_file[curr_addr++] |=
                            (arg_2_type == a_adr) ? op28_R_M : op28_R_D;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        disk_file[curr_addr++] = arg_2 >> 8;
                        disk_file[curr_addr++] = arg_1;
                        break;
                      case a_cst:
                        disk_file[curr_addr++] |= op28_R_I;
                        disk_file[curr_addr++] = arg_1;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        break;
                    }
                    break;
                  case a_adr:
                    switch (arg_2_type) {
                      case a_reg:
                        disk_file[curr_addr++] |= op28_M_R;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] = arg_2;
                        break;
                      case a_cst:
                        disk_file[curr_addr++] |= op28_M_I;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        break;
                      default:
                        error("invalid second argument");
                    }
                    break;
                  case a_ptr:
                    switch (arg_2_type) {
                      case a_reg:
                        disk_file[curr_addr++] |= op28_D_R;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] = arg_2;
                        break;
                      case a_cst:
                        disk_file[curr_addr++] |= op28_D_I;
                        disk_file[curr_addr++] =
                            arg_1 - ((arg_1 >> 8) << 8);
                        disk_file[curr_addr++] = arg_1 >> 8;
                        disk_file[curr_addr++] =
                            arg_2 - ((arg_2 >> 8) << 8);
                        break;
                      default:
                        error("invalid second argument");
                    }
                    break;
                  case a_cst:
                    error("constant as first argument of movb");
                }
            }
            /* --- inc operation --- */
            else if (op_name == c_inc) {
                switch (arg_1_type) {
                  case a_reg:
                    error("invalid argument");
                  case a_adr:
                  case a_ptr:
                    disk_file[curr_addr++] =
                        (arg_1_type == a_adr) ? 0x0a : 0x0b;
                    disk_file[curr_addr++] =
                        arg_1 - ((arg_1 >> 8) << 8);
                    disk_file[curr_addr++] = arg_1 >> 8;
                    break;
                  case a_cst:
                    error("constant as argument of inc");
                }
            }
            /* --- out operation --- */
            else if (op_name == c_out) {
                switch (arg_1_type) {
                  case a_reg:
                    disk_file[curr_addr++] = 0x42;
                    disk_file[curr_addr++] = arg_1;
                    break;
                  case a_adr:
                  case a_ptr:
                    disk_file[curr_addr++] =
                        (arg_1_type == a_adr) ? 0x40 : 0x41;
                    disk_file[curr_addr++] =
                        arg_1 - ((arg_1 >> 8) << 8);
                    disk_file[curr_addr++] = arg_1 >> 8;
                    break;
                  case a_cst:
                    disk_file[curr_addr++] = 0x43;
                    disk_file[curr_addr++] = arg_1;
                    break;
                }
            }
            /* --- je operation --- */
            else if (op_name == c_je) {
                switch (arg_1_type) {
                  case a_reg:
                  case a_cst:
                    error("invalid argument");
                  case a_adr:
                  case a_ptr:
                    disk_file[curr_addr++] =
                        (arg_1_type == a_adr) ? 0x20 : 0x21;
                    disk_file[curr_addr++] =
                        arg_1 - ((arg_1 >> 8) << 8);
                    disk_file[curr_addr++] = arg_1 >> 8;
                    disk_file[curr_addr++] =
                        arg_2 - ((arg_2 >> 8) << 8);
                    disk_file[curr_addr++] = arg_2 >> 8;
                    break;
                }
            }
            /* --- jne operation --- */
            else if (op_name == c_jne) {
                switch (arg_1_type) {
                  case a_reg:
                  case a_cst:
                    error("invalid argument");
                  case a_adr:
                  case a_ptr:
                    disk_file[curr_addr++] =
                        (arg_1_type == a_adr) ? 0x22 : 0x23;
                    disk_file[curr_addr++] =
                        arg_1 - ((arg_1 >> 8) << 8);
                    disk_file[curr_addr++] = arg_1 >> 8;
                    disk_file[curr_addr++] =
                        arg_2 - ((arg_2 >> 8) << 8);
                    disk_file[curr_addr++] = arg_2 >> 8;
                    break;
                }
            }
            /* --- exit operation --- */
            else if (op_name == c_exit) {
                disk_file[curr_addr++] = 0xff;
            }
            /* handle unrecognised operation */
            else error("unrecognised operation");
            break;
          default:
            break;
        }
        if (feof(fp)) break;
    }
    if (!end) error("\".end.\" not found");
    fclose(fp);
    fp = fopen("disk_file", "wb");
    for (i = 0; i < curr_data_addr; i++) {
        fwrite(disk_file + i, sizeof *disk_file, 1, fp);
    }
    fclose(fp);
    return 0;
}
