#include <stdio.h>
#include <conio.h>
#include "operation.h"

#define mem_size 65536
/** #define debug /**/

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
    int i, i2;
    
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
    #ifdef debug
    puts("memory:");
    for (i = 0; i < 10; i++) {
        for (i2 = 0; i2 < 16; i2++) {
            printf("%s%02x", i2 ? " " : "", (int) ip[16 * i + i2]);
        }
        puts("");
    }
    puts("");
    #endif
    while (-1) {
        #ifdef debug
        /**/printf("%02x,%02x,%02x,(%2d:%02x %02x %02x %02x %02x)",
        memory[4096], memory[4097], memory[4098], (ip - memory), (int) *ip,
        (int) ip[1], (int) ip[2], (int) ip[3], (int) ip[4]);
        if (getch() == '\e') {
            puts("");
            return 0;
        }
        /**/
        #endif
        /* ==================== Start of main switch ===================== */
        switch(*ip) {
          case 0x00:
            break;
          /* ----------------------- INC operation ----------------------- */
          case oc_INC | op12_M:
            take_address();
            memory[addr_1]++;
            if (memory[addr_1] == 0) {
                memory[++addr_1]++;
                if (memory[addr_1] == 0) {
                    regs[r_fl] |= flag_o;
                    regs[r_fl] |= flag_z;
                } else {
                    regs[r_fl] &= (~flag_o);
                    regs[r_fl] &= (~flag_z);
                }
            } else {
                regs[r_fl] &= (~flag_o);
                regs[r_fl] &= (~flag_z);
            }
            break;
          case oc_INC | op12_D:
            take_pointer();
            memory[addr_1]++;
            if (memory[addr_1] == 0) {
                memory[++addr_1]++;
                if (memory[addr_1] == 0) {
                    regs[r_fl] |= flag_o;
                    regs[r_fl] |= flag_z;
                } else {
                    regs[r_fl] &= (~flag_o);
                    regs[r_fl] &= (~flag_z);
                }
            } else {
                regs[r_fl] &= (~flag_o);
                regs[r_fl] &= (~flag_z);
            }
            break;
            
          /* ----------------------- MOV operation ----------------------- */
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
            
          /* ---------------------- MOVB operation ----------------------- */
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
            
          /* ----------------------- SUB operation ----------------------- */
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
            
           /* ---------------------- CMP operation ----------------------- */
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
                
                if (regs[++addr_r] <= (*++ip)) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (regs[addr_r++] == *(ip++))  {
                    if (regs[addr_r] == *ip) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (regs[addr_r] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_M_I:
            take_address();
            if (memory[addr_1] < (*++ip)) {
                regs[r_fl] &= (~flag_z);
                
                if (memory[++addr_1] <= (*++ip)) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (memory[addr_1++] == *(ip++)) {
                    if (memory[addr_1] == *ip) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (memory[addr_1] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_D_I:
            take_pointer();
            if (memory[addr_1] < (*++ip)) {
                regs[r_fl] &= (~flag_z);
                
                if (memory[++addr_1] <= (*++ip)) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (memory[addr_1++] == *(ip++)) {
                    if (memory[addr_1] == *ip) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (memory[addr_1] < *ip) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_M_R:
            take_address();
            addr_r = (*++ip);
            if (memory[addr_1] < regs[addr_r]) {
                regs[r_fl] &= (~flag_z);
                
                if (memory[++addr_1] <= regs[++addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (memory[addr_1++] == regs[addr_r++]) {
                    if (memory[addr_1] == regs[addr_r]) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (memory[addr_1] < regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_D_R:
            take_pointer();
            addr_r = (*++ip);
            if (memory[addr_1] < regs[addr_r]) {
                regs[r_fl] &= (~flag_z);
                
                if (memory[++addr_1] <= regs[++addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (memory[addr_1++] == regs[addr_r++]) {
                    if (memory[addr_1] == regs[addr_r]) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (memory[addr_1] < regs[addr_r]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_R_M:
            take_address();
            addr_r = (*++ip);
            if (regs[addr_r] < memory[addr_1]) {
                regs[r_fl] &= (~flag_z);
                
                if (regs[++addr_r] <= memory[++addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (regs[addr_r++] == memory[addr_1++]) {
                    if (regs[addr_r] == memory[addr_1]) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (regs[addr_r] < memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
          case oc_CMP | op28_R_D:
            take_pointer();
            addr_r = (*++ip);
            if (regs[addr_r] < memory[addr_1]) {
                regs[r_fl] &= (~flag_z);
                
                if (regs[++addr_r] <= memory[++addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            } else {
                if (regs[addr_r++] == memory[addr_1++]) {
                    if (regs[addr_r] == memory[addr_1]) regs[r_fl] |= flag_z;
                    else regs[r_fl] &= (~flag_z);
                } else regs[r_fl] &= (~flag_z);
                
                if (regs[addr_r] < memory[addr_1]) regs[r_fl] |= flag_o;
                else regs[r_fl] &= (~flag_o);
            }
            break;
            
          /* ---------------------- Jump operations ---------------------- */
          case oc_JE | op_jmp_A:
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (regs[r_fl] & flag_z)
                ip = memory + addr_j - 1;
            
            break;
          case oc_JE | op_jmp_P:
            addr_p = (*++ip);
            addr_p += (*++ip) << 8;
            addr_j = memory[addr_p];
            addr_j += memory[addr_p + 1] << 8;
            if (regs[r_fl] & flag_z)
                ip = memory + addr_j - 1;
            
            break;
          case oc_JNE | op_jmp_A:
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (!(regs[r_fl] & flag_z))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JNE | op_jmp_P:
            addr_p = (*++ip);
            addr_p += (*++ip) << 8;
            addr_j = memory[addr_p];
            addr_j += memory[addr_p + 1] << 8;
            if (!(regs[r_fl] & flag_z))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JL | op_jmp_A:
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (regs[r_fl] & flag_o)
                ip = memory + addr_j - 1;
            
            break;
          case oc_JL | op_jmp_P:
            addr_p = (*++ip);
            addr_p += (*++ip) << 8;
            addr_j = memory[addr_p];
            addr_j += memory[addr_p + 1] << 8;
            if (regs[r_fl] & flag_o)
                ip = memory + addr_j - 1;
            
            break;
          case oc_JG | op_jmp_A:
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (!(regs[r_fl] & (flag_z | flag_o)))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JG | op_jmp_P:
            addr_p = (*++ip);
            addr_p += (*++ip) << 8;
            addr_j = memory[addr_p];
            addr_j += memory[addr_p + 1] << 8;
            if (!(regs[r_fl] & (flag_z | flag_o)))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JLE | op_jmp_A:
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (regs[r_fl] & (flag_z | flag_o))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JLE | op_jmp_P:
            addr_p = (*++ip);
            addr_p += (*++ip) << 8;
            addr_j = memory[addr_p];
            addr_j += memory[addr_p + 1] << 8;
            if (regs[r_fl] & (flag_z | flag_o))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JGE | op_jmp_A:
            addr_j = (*++ip);
            addr_j += (*++ip) << 8;
            if (!(regs[r_fl] & flag_o))
                ip = memory + addr_j - 1;
            
            break;
          case oc_JGE | op_jmp_P:
            addr_p = (*++ip);
            addr_p += (*++ip) << 8;
            addr_j = memory[addr_p];
            addr_j += memory[addr_p + 1] << 8;
            if (!(regs[r_fl] & flag_o))
                ip = memory + addr_j - 1;
            
            break;
            
          /* --------------------- Output operations --------------------- */
          case oc_OUT | op14_M:
            take_address();
            printf("%c", memory[addr_1]);
            break;
          case oc_OUT | op14_D:
            take_pointer();
            printf("%c", memory[addr_1]);
            break;
          case oc_OUT | op14_R:
            addr_r = (*++ip);
            printf("%c", regs[addr_r]);
            break;
          case oc_OUT | op14_I:
            printf("%c", (*++ip));
            break;
            
          /* --------------------- Exit and default ---------------------- */
          case 0xff:
            return 0;
          default:
            puts("unassigned opcode");
            break;
        }
        /* ===================== End of main switch ====================== */
        ip++;
        #ifdef debug
        /**/printf(",%02x,%02x,%02x\n",
                    memory[4096], memory[4097], memory[4098]);/**/
        #endif
        continue;
    }
}
