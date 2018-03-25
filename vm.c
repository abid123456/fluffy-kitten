#include <stdio.h>

#define mem_size 65536
/** #define debug /**/

#define oc_MOV  0x10
#define oc_MOVB 0x18
#define oc_SUB  0x30
#define oc_CMP  0x38

#define op28_R_R 0x00
#define op28_R_I 0x01
#define op28_M_I 0x02
#define op28_D_I 0x03
#define op28_M_R 0x04
#define op28_D_R 0x05
#define op28_R_M 0x06
#define op28_R_D 0x07

unsigned char memory[mem_size];
unsigned char regs[16];
unsigned char *ip;
unsigned short addr_1, addr_2, addr_r, addr_r_2, addr_j, addr_p;

#define r_ax 0
#define r_bx 2
#define r_cx 4
#define r_dx 6
#define r_al 0
#define r_ah 1
#define r_bl 2
#define r_bh 3
#define r_cl 4
#define r_ch 5
#define r_dl 6
#define r_dh 7
#define r_si 8
#define r_fl 10
#define r_sp 12
#define r_ip 14

#define flag_o 0x01
#define flag_z 0x02

void take_pointer()
{
    addr_p = (*++ip);
    addr_p += (*++ip) << 8;
    addr_1 = memory[addr_p];
    addr_1 += memory[addr_p + 1] << 8;
    return;
}

void take_address()
{
    addr_1 = (*++ip);
    addr_1 += (*++ip) << 8;
    return;
}

int main()
{
    FILE *fp;
    int i;
    
    fp = fopen("disk_file", "rb");
    for (i = 0; i < mem_size; i++)
        memory[i] = 0;
    for (i = 0; i < 16; i++)
        regs[i] = 0;
    ip = memory;
    while (-1) {
        fread(ip++, sizeof *memory, 1, fp);
        if (feof(fp)) {
            fclose(fp);
            fp = NULL;
            break;
        }
    }
    ip = memory;
    while (-1) {
        #ifdef debug
        /**/printf("%02x,%02x,%02x,(%2d %02x)",
        memory[4096], memory[4097], memory[4098], (ip - memory), (int) *ip);
        /**/
        #endif
        switch(*ip) {
          case 0x00:
            break;
          case 0x0a:
            take_address();
            memory[addr_1]++;
            break;
          case 0x0b:
            take_pointer();
            memory[addr_1]++;
            break;
            
          /* --- MOV operation --- */
          case oc_MOV | op28_R_R:
            addr_r = (*++ip);
            addr_r_2 = (*++ip);
            regs[addr_r++] = regs[addr_r_2++];
            regs[addr_r] = regs[addr_r_2];
            break;
          case oc_MOV | op28_R_I:
            addr_r = (*++ip);
            regs[addr_r++] = (*++ip);
            regs[addr_r] = (*++ip);
            break;
          case oc_MOV | op28_M_I:
            take_address();
            memory[addr_1] = (*++ip);
            memory[addr_1 + 1] = ((*++ip) << 8);
            break;
          case oc_MOV | op28_D_I:
            take_pointer();
            memory[addr_1] = (*++ip);
            memory[addr_1 + 1] = ((*++ip) << 8);
            break;
          case oc_MOV | op28_M_R:
            take_address();
            addr_r = (*++ip);
            memory[addr_1++] = regs[addr_r++];
            memory[addr_1] = regs[addr_r];
            break;
          case oc_MOV | op28_D_R:
            take_pointer();
            addr_r = (*++ip);
            memory[addr_1++] = regs[addr_r++];
            memory[addr_1] = regs[addr_r];
            break;
          case oc_MOV | op28_R_M:
            take_address();
            addr_r = (*++ip);
            regs[addr_r++] = memory[addr_1++];
            regs[addr_r] = memory[addr_1];
            break;
          case oc_MOV | op28_R_D:
            take_pointer();
            addr_r = (*++ip);
            regs[addr_r++] = memory[addr_1++];
            regs[addr_r] = memory[addr_1];
            break;
            
          /* --- MOVB operation --- */
          case oc_MOVB | op28_R_R:
            addr_r = (*++ip);
            addr_r_2 = (*++ip);
            regs[addr_r] = regs[addr_r_2];
            break;
          case oc_MOVB | op28_R_I:
            addr_r = (*++ip);
            regs[addr_r] = (*++ip);
            break;
          case oc_MOVB | op28_M_I:
            take_address();
            memory[addr_1] = (*++ip);
            break;
          case oc_MOVB | op28_D_I:
            take_pointer();
            memory[addr_1] = (*++ip);
            break;
          case oc_MOVB | op28_M_R:
            take_address();
            addr_r = (*++ip);
            memory[addr_1] = regs[addr_r];
            break;
          case oc_MOVB | op28_D_R:
            take_pointer();
            addr_r = (*++ip);
            memory[addr_1] = regs[addr_r];
            break;
          case oc_MOVB | op28_R_M:
            take_address();
            addr_r = (*++ip);
            regs[addr_r] = memory[addr_1];
            break;
          case oc_MOVB | op28_R_D:
            take_pointer();
            addr_r = (*++ip);
            regs[addr_r] = memory[addr_1];
            break;
            
          /* --- SUB operation --- */
          case oc_SUB | op28_R_R:
            addr_r = (*++ip);
            addr_r_2 = (*++ip);
            if (regs[addr_r] < regs[addr_r_2]) {
                regs[addr_r++] -= regs[addr_r_2++];
                if (regs[addr_r]-- <= regs[addr_r_2]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= regs[addr_r_2];
                regs[r_fl] &= (~flag_z);
            } else {
                regs[addr_r++] -= regs[addr_r_2++];
                if (regs[addr_r] < regs[addr_r_2]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= regs[addr_r_2];
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_R_I:
            addr_r = (*++ip);
            if (regs[addr_r] < (*++ip)) {
                regs[addr_r++] -= *(ip++);
                if (regs[addr_r]-- <= *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= *ip;
                regs[r_fl] &= (~flag_z);
            } else {
                regs[addr_r++] -= *(ip++);
                if (regs[addr_r] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= *ip;
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_M_I:
            take_address();
            if (memory[addr_1] < (*++ip)) {
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1]-- <= *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
                regs[r_fl] &= (~flag_z);
            } else {
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_D_I:
            take_pointer();
            if (memory[addr_1] < (*++ip)) {
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1]-- <= *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
                regs[r_fl] &= (~flag_z);
            } else {
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_M_R:
            take_address();
            addr_r = (*++ip);
            if (memory[addr_1] < regs[addr_r]) {
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1]-- <= regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
                regs[r_fl] &= (~flag_z);
            } else {
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1] < regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_D_R:
            take_pointer();
            addr_r = (*++ip);
            if (memory[addr_1] < regs[addr_r]) {
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1]-- <= regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
                regs[r_fl] &= (~flag_z);
            } else {
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1] < regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_R_M:
            take_address();
            addr_r = (*++ip);
            if (regs[addr_r] < memory[addr_1]) {
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r]-- <= memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
                regs[r_fl] &= (~flag_z);
            } else {
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r] < memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_SUB | op28_R_D:
            take_pointer();
            addr_r = (*++ip);
            if (regs[addr_r] < memory[addr_1]) {
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r]-- <= memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
                regs[r_fl] &= (~flag_z);
            } else {
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r] < memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
            
           /* --- CMP operation --- */
          case oc_CMP | op28_R_R:
            addr_r = (*++ip);
            addr_r_2 = (*++ip);
            if (regs[addr_r] < regs[addr_r_2]) {
                regs[r_fl] &= (~flag_z);
                
                if (regs[++addr_r] <= regs[++addr_r_2]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (regs[addr_r++] == regs[addr_r_2++]) {
                    if (regs[addr_r] == regs[addr_r_2]) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (regs[addr_r] < regs[addr_r_2]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_R_I:
            addr_r = (*++ip);
            if (regs[addr_r] < (*++ip)) {
                regs[r_fl] &= (~flag_z);
                
                regs[addr_r++] -= *(ip++);
                if (regs[addr_r]-- <= *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= *ip;
            } else {
                regs[addr_r++] -= *(ip++);
                if (regs[addr_r] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= *ip;
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_CMP | op28_M_I:
            take_address();
            if (memory[addr_1] < (*++ip)) {
                regs[r_fl] &= (~flag_z);
                
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1]-- <= *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
            } else {
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_CMP | op28_D_I:
            take_pointer();
            if (memory[addr_1] < (*++ip)) {
                regs[r_fl] &= (~flag_z);
                
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1]-- <= *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
            } else {
                memory[addr_1++] -= *(ip++);
                if (memory[addr_1] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= *ip;
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_CMP | op28_M_R:
            take_address();
            addr_r = (*++ip);
            if (memory[addr_1] < regs[addr_r]) {
                regs[r_fl] &= (~flag_z);
                
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1]-- <= regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
            } else {
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1] < regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_CMP | op28_D_R:
            take_pointer();
            addr_r = (*++ip);
            if (memory[addr_1] < regs[addr_r]) {
                regs[r_fl] &= (~flag_z);
                
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1]-- <= regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
            } else {
                memory[addr_1++] -= regs[addr_r++];
                if (memory[addr_1] < regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                memory[addr_1] -= regs[addr_r];
                if (memory[addr_1] == 0 && memory[addr_1 - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_CMP | op28_R_M:
            take_address();
            addr_r = (*++ip);
            if (regs[addr_r] < memory[addr_1]) {
                regs[r_fl] &= (~flag_z);
                
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r]-- <= memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
            } else {
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r] < memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_CMP | op28_R_D:
            take_pointer();
            addr_r = (*++ip);
            if (regs[addr_r] < memory[addr_1]) {
                regs[r_fl] &= (~flag_z);
                
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r]-- <= memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
            } else {
                regs[addr_r++] -= memory[addr_1++];
                if (regs[addr_r] < memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
                regs[addr_r] -= memory[addr_1];
                if (regs[addr_r] == 0 && regs[addr_r - 1] == 0)
                    regs[r_fl] |= flag_z;
                else regs[r_fl] &= (~flag_z);
            }
            break;
            
          /* --- Jump operations --- */
          case 0x20:
            take_address();
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (memory[addr_1] == regs[0] + (regs[1] << 8))
                ip = memory + addr_j - 1;
            break;
          case 0x21:
            take_pointer();
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (memory[addr_1] == regs[0] + (regs[1] << 8))
                ip = memory + addr_j - 1;
            break;
          case 0x22:
            take_address();
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (memory[addr_1] != regs[0] + (regs[1] << 8))
                ip = memory + addr_j - 1;
            break;
          case 0x23:
            take_pointer();
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (memory[addr_1] != regs[0] + (regs[1] << 8))
                ip = memory + addr_j - 1;
            break;
            
          case 0x40:
            take_address();
            printf("%c", memory[addr_1]);
            break;
          case 0x41:
            take_pointer();
            printf("%c", memory[addr_1]);
            break;
          case 0x42:
            addr_r = (*++ip);
            printf("%c", regs[addr_r]);
            break;
          case 0x43:
            printf("%c", (*++ip));
            break;
          case 0xff:
            return 0;
          default:
            puts("unassigned opcode");
            break;
        }
        ip++;
        #ifdef debug
        /**/printf(",%02x,%02x,%02x\n",
                    memory[4096], memory[4097], memory[4098]);/**/
        #endif
        continue;
    }
}
