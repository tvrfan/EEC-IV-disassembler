
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * * 3.06
 ******************************************************

 * This code assumes INT is at least 32 bits, on some older compilers this would require
 * the use of LONG instead of INT.  Some code here also uses a LONG for a VOID *.

 * On Win32 builds (Codeblocks mingw)      int =long = void* = 32 bits.
 * On Linux builds (CodeBlocks gcc amd64)  int = 32 bits, long = void* = 64 bits.

 ******************************************************
 *  Declarations and includes
 *******************************************************/

#include  "core.h"             // this calls shared.h
#include  "sign.h"

#define DBGPRT  wnprt
#define DBGPRTN wnprtn


/**********************************************************************************
external subroutine and variable declarations
***********************************************************************************/

extern PAT intign;                // sig 'ignore interrupt'  (for naming)
extern PAT hdr;                   // sig 'start jump' (and vects);
extern PAT fnlp;
extern PAT tblp;
extern PAT tblx;
 
void  show_prog    (int);
SIG*  do_sig       (PAT *, int);
SIG*  scan_sigs    (int);
SIG*  scan_asigs   (int);
void  prescan_sigs (void);

/*****************************************************************************
* declarations for subrs in structs declarations
***************************************************************************/

int pp_dft   (int, int);
int pp_wdbt  (int, int);
int pp_text  (int, int);
int pp_vect  (int, int);
int pp_code  (int, int);
int pp_stct  (int, int);
int pp_timer (int, int);

int set_vect (CPS*);               // command processors
int set_code (CPS*);
int set_scan (CPS*);
int set_cdih (CPS*);
int set_args (CPS*);
int set_data (CPS*);
int set_opts (CPS*);
int set_rbas (CPS*);
int set_sym  (CPS*);
int set_subr (CPS*);
int set_bnk  (CPS*);
int set_time (CPS*);
int set_ordr (CPS*);

// CHAIN related subrs

void fsym(void *);         // free block procs - symbol
void fscn(void *);         // scan
void fsub(void *);         // subr
void fcmd(void *);         // cmd (and auxcmd)
void fblk(void *);         // alloced blocks chain

int cpfjp  (int, void *);    // binary chop compares
int cptjp  (int, void *);
int cpsym  (int, void *);
int cpbase (int, void *);
int cpsig  (int, void *);
int cpcmd  (int, void *);
int cpacmd (int, void *);
int cpscan (int, void *);
int cpsub  (int, void *);
int cpddt  (int, void *);
int eqcmd  (int, void *);
int eqacmd (int, void *);

// CHAIN items here - which use common find and insert mechanisms
// num entries, num ptrs, allocsize, entry size (bytes), max ptrs, nallocs,ptr array,free,compare,equals

CHAIN jpfch  = {0,0,500,sizeof(JLT),   0,0,0,0    ,cpfjp  , cpfjp };      // jump from
CHAIN jptch  = {0,0,500,0          ,   0,0,0,0    ,cptjp  , cptjp };      // jump to REMOTE ptrs to jpfch
CHAIN symch  = {0,0,400,sizeof(SYT),   0,0,0,fsym ,cpsym  , cpsym };      // syms read
CHAIN basech = {0,0, 20,sizeof(RBT),   0,0,0,0    ,cpbase , cpbase};      // rbase
CHAIN sigch  = {0,0, 40,sizeof(SIG),   0,0,0,0    ,cpsig  , cpsig };      // signature
CHAIN auxch  = {0,0, 10,sizeof(LBK),   0,0,0,fcmd ,cpacmd , eqacmd};      // aux cmd (xcode, args)
CHAIN cmdch  = {0,0,200,sizeof(LBK),   0,0,0,fcmd ,cpcmd  , eqcmd };      // commands
CHAIN scanch = {0,0,750,sizeof(SBK),   0,0,0,fscn ,cpscan , cpscan};      // scan  bring back eqscan ?
CHAIN subch  = {0,0,300,sizeof(SXT),   0,0,0,fsub ,cpsub  , cpsub };      // subr
CHAIN datch  = {0,0,500,sizeof(DDT),   0,0,0,0    ,cpddt  , cpddt };      // data addresses detected


CHAIN mblk   = {0,0,100,sizeof(MBL),   0,0,0,fblk ,0      , 0    };        // track mallocs for cleanup

// for neat start and tidyup loops.

CHAIN *chlist[] = { &jpfch, &jptch, &symch, &basech, &sigch, &auxch, &cmdch, &scanch,
                    &subch, &datch, &mblk };



/********** COMMAND Option letters -

      as opt            in cmd extension
 A
 B
 C    C code            (character print if DATA ?)
 D                      Fixed offset for param (like rbase) (word or byte) 2nd BANK   (vect)
 E                      ENCODED  type, rbase
 F                      special func type  - type, addr, cols   (func - if data cmd)
 G
 H    set 8065
 I
 J
 K
 L    label names       int
 M    manual mode       mask ?(byte or word pair) and flags for syms ?
 N    Int proc names    Name lookup
 O                      Count (cols etc)
 P    proc names        Print fieldwidth
 Q                      Quit (terminator) Byte (or Word)
 R                      Indirect/pointer (vector)
 S    sig detect        Signed    (unsigned otherwise)
 T                      BiT (with symbol)           (table if data cmd?)
 U
 V                      Divisor (Scaler) - Float 
 W                      Word
 X    Decimal print     Decimal print
 Y                      Byte
 Z
 ***************************************************************/


/*********************
main command structure -
params are -
command string, command processor, command printer, max addnl levels, max addresses/numbers expected,
par which is 'start' address, name expected, overrides (bit mask), merge allowed, options string (scanf)

merge is with same cmd, override is bitmask

******/


DIRS dirs[] = {
{"fill"    , set_data,  pp_dft,   0,  2, 1, 0, 0, 1, 0},                      // fill - default -> MUST be zero ix
{"byte"    , set_data,  pp_wdbt,  2,  2, 1, 0, 3, 1, "%[:PSX]%n" },          // pfw, signed, hex, divisor
{"word"    , set_data,  pp_wdbt,  2,  2, 1, 0, 1, 1, "%[:LPSX]%n" },         // + int can be ovr by byte !
{"text"    , set_data,  pp_text,  0,  2, 1, 0, 7, 1, 0  } ,

{"table"   , set_data,  pp_stct,  2,  2, 1, 1, 7, 0, "%[:OPSVWXY]%n" },
{"func"    , set_data,  pp_stct,  3,  2, 1, 1, 7, 0, "%[:LPSVWXY]%n" } ,
{"struct"  , set_data,  pp_stct,  15, 2, 1, 1, 7, 0, "%[:|DLMNOPRSVWXYQ]%n" },

{"timerl"  , set_time, pp_timer, 2,  2, 1, 1, 7, 0, "%[:YWNT]%n" },          // timer list
{"vect"    , set_vect, pp_vect,  2,  2, 1, 1, 7, 1, "%[:DQ]%n"} ,            // bank, quit byte (always hex) N ? names
{"args"    , set_args, pp_dft,   15, 2, 1, 0, 0, 0, "%[:DELNOPSWXY]%n" },    // call address, not subr

{"subr"    , set_subr, pp_dft,   15, 1, 1, 1, 0, 0, "%[:DEFLNOPSUWXY]%n" },
{"xcode"   , set_cdih, pp_dft,   0,  2, 1, 0, 0, 1, 0  } ,
{"code"    , set_code, pp_code,  0,  2, 1, 0, 7, 1, 0  } ,             // may be more than 7 for ovrdes

{"scan"    , set_scan, pp_dft ,  0,  1, 1, 0, 0, 0, 0   },
{"opts"    , set_opts, pp_dft,   2,  0, 0, 0, 0, 0, "%[:CDHLGMNPS]%n" },       // add more here....
{"rbase"   , set_rbas, pp_dft,   0,  2, 2, 0, 0, 0, 0   },

{"sym"     , set_sym,  pp_dft,   2,  1, 1, 1, 0, 0, "%[:FTW]%n"    },       // flags,bit,write // flags auto on bit ? // Scaler ? (as V)

{"banks"   , set_bnk,  pp_dft,   0,  4, 2, 0, 0, 0, 0 },              //  file map/offsets
{"order"   , set_ordr, pp_dft,   2,  4, 0, 0, 0, 0, 0 }
};


// scan type  = 1 init jump, 2 static jump, 3 cond jump  4 intr subr, 5 vect subr, 6 called subr.
// remove init jump and DEFINE scan types ?

const char *empty = "";
const char *jptxt[] = {empty, "ijmp","sjmp","cjmp","isub","vsub","csub","jsub","vect" };

/**********************
 *  opcode definition table
 **********************/

/*********************************************
jump swopovers......
'9'   => 'goto 1'   normally,
'9'   => '{' when swopped sense with jump
'8'   => 'if' normally
95 items with invalid for 8061, 99 for 8065

could consider offset for strings as 4 byte pointers are big....
also can crunch source code for all the spaces ?

**********************************************/

// index for opcodes points into main opcode table - 0 is invalid 


uchar opcind[256] =                // rordered to put main types together.
{
// 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f 

  88, 10, 11, 12,  0, 13, 14, 15,  4,  1,  5,  0,  6,  3,  7, 16,    // 00 - 0f  
  111,17, 18, 19,  0, 20, 21, 22,  8,  2,  9,  0,  0,  0,  0,  0,    // 10 - 1f  
  84, 84, 84, 84, 84, 84, 84, 84, 85, 85, 85, 85, 85, 85, 85, 85,    // 20 - 2f  
  86, 86, 86, 86, 86, 86, 86, 86, 87, 87, 87, 87, 87, 87, 87, 87,    // 30 - 3f  
  23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26,    // 40 - 4f  
  27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30,    // 50 - 5f 
  31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34,    // 60 - 6f 
  35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38,    // 70 - 7f 
  39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42,    // 80 - 8f 
  43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46,    // 90 - 9f 
  47, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50,    // a0 - af 
  51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54,    // b0 - bf 
  55,  0, 55, 55, 56,  0, 56, 56, 57, 57, 57, 57, 58,  0, 58, 58,    // c0 - cf 
  67, 68, 70, 69, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,    // d0 - df 
  83,  0,  0,  0,  0,  0,  0, 89,  0,  0,  0,  0,  0,  0,  0, 90,    // e0 - ef 
  91, 92, 97, 98,107,108,109,  0,101,102,103,104,105,110,112,106     // f0 - ff 
};


/* instruction types - letters DEFINEd in core.h
                     (immediate, indirect,indexed....int)
A     bitwise AND
B     short jump with J  could use rsz2 for this (rsz is for jump destination, always 2)
C     conditional jump (with J)
D     DJNZ instruction (with J)
E     this opcode ends code block (or scan), subject to other conditions
F     Ret instruction (with J)
G
H     8065 opcode only
I
J     jumps & subr calls, RET by default,  short with B
K     SKIP instruction (with J)
L     int Jump with J
M     multimode opcode, word or byte
N
O     bitwise OR   XOR with X
P     pushw
Q
R     RAM bank swop   [RET with J? ]
S     shift
T     bit jump, djnz (with J)
U
V
W
X     Subroutine call with J   XOR with O
Y     Special marker for CARRY operations, for signature.
Z     Read operand can be dropped for printout if zero

 Signature index value for like instructions - MAX 63 (new sign code)
 0  special   1  clr    2  neg/cpl     3  shift l   4  shift R   5 and
 6  add       7  subt   8  mult    9  or       10 cmp       11 divide
 12 load      13 sto    14 push    15 pop      16 jumps     17 subr
 18 djnz      19 condjmp 20 ret     21 carry   22 other

 23 jnb/jb - can cross match to 19 with extra par

 24 dec     25 inc
 12 and 13 (ldx, stx) can be regarded as equal with params swopped over...
  
  
 * 
 * 
 * 
 40+ are optional and skipped, but detectable with -

 40 sig      41 popp     42 pushp    43 di       44 ei       45 (RAM)bnk
 46 (ROM)bnk 47 nop      48 others
*/

// code emulation procs only 17 - moved to later struct by do_code 

int   clr  (SBK *);
int   ldx  (SBK *);
int   stx  (SBK *);
int   orx  (SBK *);
int   add  (SBK *);
int   xand (SBK *);
int   neg  (SBK *);
int   cpl  (SBK *);
int   sub  (SBK *);
int   cmp  (SBK *);
int   mlx  (SBK *);
int   dvx  (SBK *);
int   shl  (SBK *);
int   shr  (SBK *);
int   popw (SBK *);
int   pshw (SBK *);
int   inc  (SBK *);
int   dec  (SBK *);



// reordered to put main types together
// sig ix, alt ix, alters psw, numops,write op, wsz,rsz1,sz2,sz3/flag,  emuflags,name,scecode

OPC opctbl[] = {                                                               // 114 cells of 24
{ 0 ,  0,   0, 0,0, 0,0,0,0, E         ,0     ,"!INV!" ,  "" },              // zero entry is for invalid opcode

{ 3 ,  0,   1, 2,2, 2,1,2,0, S         ,shl   ,"shlw"  ,  "\x2 <<= \x1;" },  // 1
{ 3 ,  0,   1, 2,2, 1,1,1,0, S         ,shl   ,"shlb"  ,  "\x2 <<= \x1;" },  // 2 
{ 3 ,  0,   1, 2,2, 3,1,3,0, S         ,shl   ,"shldw" ,  "\x7\x2 <<= \x1;" },   // 3

{ 4 ,  0,   1, 2,2, 2,1,2,0, S         ,shr   ,"shrw"  ,  "\x2 >>= \x1;" },  // 4
{ 4 ,  0,   1, 2,2, 2,1,6,0, S         ,shr   ,"asrw"  ,  "\x7\x2 >>= \x1;" },  // 5
{ 4 ,  0,   1, 2,2, 3,1,3,0, S         ,shr   ,"shrdw" ,  "\x7\x2 >>= \x1;" },   // 6
{ 4 ,  0,   1, 2,2, 7,1,7,0, S         ,shr   ,"asrdw" ,  "\x7\x2 >>= \x7\x1;" },  // 7
{ 4 ,  0,   1, 2,2, 1,1,1,0, S         ,shr   ,"shrb"  ,  "\x2 >>= \x1;" }, // 8
{ 4 ,  0,   1, 2,2, 5,1,5,0, S         ,shr   ,"asrb"  ,  "\x7\x2 >>= \x1;" },  // 9

{ 1 ,  0,   0, 1,1, 2,2,0,0, 0         ,clr   ,"clrw"  ,  "\x1 = 0;" },            // 10
{ 2 ,  0,   1, 1,1, 2,2,0,0, 0         ,cpl   ,"cplw"  ,  "\x1 = ~\x1;" },
{ 2 ,  0,   1, 1,1, 2,2,0,0, 0         ,neg   ,"negw"  ,  "\x1 = -\x1;" },
{ 24,  0,   1, 1,1, 2,2,0,0, 0         ,dec   ,"decw"  ,  "\x1--;"  },
{ 22,  0,   0, 1,1, 7,2,0,0, 0         ,0   ,"sexw"  ,  "\x7\x1 = \x7\x1;" },
{ 25,  0,   1, 1,1, 2,2,0,0, 0         ,inc   ,"incw"  ,  "\x1++;" },
{ 22,  0,   1, 2,1, 1,1,3,0, 0         ,0   ,"norm"  ,  "\x7\x1 = nrml(\x7\x2);" },    // x2 is LONG
{ 1 ,  0,   0, 1,1, 1,1,0,0, 0         ,clr    ,"clrb"  ,  "\x1 = 0;" },
{ 2 ,  0,   1, 1,1, 1,1,0,0, 0         ,cpl    ,"cplb"  ,  "\x1 = ~\x1;" },
{ 2 ,  0,   1, 1,1, 1,1,0,0, 0         ,neg    ,"negb"  ,  "\x1 = -\x1;" },      

{ 24,  0,   1, 1,1, 1,1,0,0, 0         ,dec   ,"decb"  ,  "\x1--;" },                // 20
{ 22,  0,   1, 1,1, 6,1,0,0, 0         ,0   ,"sexb"  ,  "\x7\x1 = \x7\x1;" },
{ 25,  0,   1, 1,1, 1,1,0,0, 0         ,inc   ,"incb"  ,  "\x1++;"  },

{ 5 ,  47,  1, 3,3, 2,2,2,2, M|A       ,xand   ,"an3w"  ,  "\x3 = \x2 & \x1;" },      // 23  LDW for zeroes
{ 6 ,  47,  1, 3,3, 2,2,2,2, M         ,add    ,"ad3w"  ,  "\x3 = \x2 + \x1;" },
{ 7 ,  0,   1, 3,3, 2,2,2,2, M         ,sub    ,"sb3w"  ,  "\x3 = \x2 - \x1;" },
{ 8 ,  59,  1, 3,3, 3,2,2,2, M         ,mlx    ,"ml3w"  ,  "\x7\x3 = \x2 * \x1;"  },
{ 5 ,  0,   1, 3,3, 1,1,1,1, M|A       ,xand   ,"an3b"  ,  "\x3 = \x2 & \x1;"  },
{ 6 ,  0,   1, 3,3, 1,1,1,1, M         ,add    ,"ad3b"  ,  "\x3 = \x2 + \x1;"  },
{ 7 ,  0,   1, 3,3, 1,1,1,1, M         ,sub    ,"sb3b"  ,  "\x3 = \x2 - \x1;"  },


{ 8 ,  60,  1, 3,3, 2,1,1,1, M         ,mlx    ,"ml3b"  ,  "\x7\x3 = \x2 * \x1;"  },  //30
{ 5 ,  0,   1, 2,2, 2,2,2,0, M|A       ,xand   ,"an2w"  ,  "\x2 &= \x1;" },
{ 6 ,  0,   1, 2,2, 2,2,2,0, M         ,add    ,"ad2w"  ,  "\x2 += \x1;" },
{ 7 ,  0,   1, 2,2, 2,2,2,0, M         ,sub    ,"sb2w"  ,  "\x2 -= \x1;" },
{ 8 ,  61,  1, 2,2, 3,2,2,0, M         ,mlx    ,"ml2w"  ,  "\x7\x2 = \x2 * \x1;"  },
{ 5 ,  0,   1, 2,2, 1,1,1,0, M|A       ,xand   ,"an2b"  ,  "\x2 &= \x1;"  },
{ 6 ,  0,   1, 2,2, 1,1,1,0, M         ,add    ,"ad2b"  ,  "\x2 += \x1;" },
{ 7 ,  0,   1, 2,2, 1,1,1,0, M         ,sub    ,"sb2b"  ,  "\x2 -= \x1;" },
{ 8 ,  62,  1, 2,2, 2,1,1,0, M         ,mlx    ,"ml2b"  ,  "\x7\x2 = \x2 * \x1;" },
{ 9 ,  0,   1, 2,2, 2,2,2,0, M|O       ,orx    ,"orw"   ,  "\x2 |= \x1;" },


{ 9 ,  0,   1, 2,2, 2,2,2,0, M|O|X     ,orx    ,"xrw"   ,  "\x2 ^= \x1;" },       //40
{ 10,  0,   1, 2,0, 0,2,2,0, M|V       ,cmp    ,"cmpw"  ,  "" },
{ 11,  63,  1, 2,2, 3,2,3,0, M         ,dvx    ,"divw"  ,  "\x2 = L\x2 / \x1;" },
{ 9 ,  0,   1, 2,2, 1,1,1,0, M|O       ,orx    ,"orb"   ,  "\x2 |= \x1;" },
{ 9 ,  0,   1, 2,2, 1,1,1,0, M|O|X     ,orx    ,"xorb"  ,  "\x2 ^= \x1;" },
{ 10,  0,   1, 2,0, 1,1,1,0, M|V       ,cmp    ,"cmpb"  ,  "" },

{ 11,  64,  1, 2,2, 2,1,2,0, M         ,dvx    ,"divb"  ,  "\x2 = \x7\x2 / \x1;" },
{ 12,  0,   0, 2,2, 2,2,2,0, M         ,ldx    ,"ldw"   ,  "\x2 = \x1;" },
{ 6 ,  0,   1, 2,2, 2,2,2,0, M|Z       ,add    ,"adcw"  ,  "\x2 += \x1 + CY;" },
{ 7 ,  0,   1, 2,2, 2,2,2,0, M|Z       ,sub    ,"sbbw"  ,  "\x2 -= \x1 - CY;" },


{ 12,  0,   0, 2,2, 2,1,1,0, M         ,ldx    ,"ldzbw" ,  "\x7\x2 = \x7\x1;" },  // 50
{ 12,  0,   0, 2,2, 1,1,1,0, M         ,ldx    ,"ldb"   ,  "\x2 = \x1;" },
{ 6 ,  0,   1, 2,2, 1,1,1,0, M|Z       ,add    ,"adcb"  ,  "\x2 += \x1 + CY;" },
{ 7 ,  0,   1, 2,2, 1,1,1,0, M|Z       ,sub    ,"sbbb"  ,  "\x2 -= \x1 - CY;" },
{ 12,  0,   0, 2,2, 6,1,1,0, M         ,ldx    ,"ldsbw" ,  "\x7\x2 = \x7\x1;" },
{ 13,  0,   0, 2,1, 2,2,2,0, M         ,stx    ,"stw"   ,  "\x1 = \x2;" },

{ 13,  0,   0, 2,1, 1,1,1,0, M         ,stx    ,"stb"   ,  "\x1 = \x2;" },
{ 14,  65,  0, 1,0, 0,2,0,0, M|P       ,pshw   ,"push"  ,  "push(\x1);" },        // or [STACK++] = \x1
{ 15,  66,  0, 1,1, 2,2,0,0, M         ,popw   ,"pop"   ,  "\x1 = pop();" },    // or \x1 = [STACK--];
{ 8 ,  0,   1, 3,3, 7,6,6,2, M         ,mlx    ,"sml3w" ,  "\x7\x3 = \x2 * \x1;" }, 
 
 
{ 8 ,  0,   1, 3,3, 6,5,5,2, M         ,mlx    ,"sml3b" , "\x7\x3 = \x2 * \x1;" },    // 60
{ 8 ,  0,   1, 2,2, 7,6,6,0, M         ,mlx    ,"sml2w" , "\x7\x2 = \x2 * \x1;"},
{ 8 ,  0,   1, 2,2, 6,5,5,0, M         ,mlx    ,"sml2b" , "\x7\x2 = \x2 * \x1;"},
{ 11,  0,   1, 2,2, 7,6,7,0, M         ,dvx    ,"sdivw" , "\x2 = \x7\x2 / \x1;"},
{ 11,  0,   1, 2,2, 6,5,6,0, M         ,dvx    ,"sdivb" , "\x2 = \x7\x2 / \x1;"},
{ 14,  0,   0, 1,0, 0,2,0,0, H|M|P     ,pshw   ,"pusha" , "pusha(\x1);" },
{ 15,  0,   0, 1,1, 2,2,0,0, H|M       ,popw   ,"popa"  , "\x1 = popa();" },

{ 19,  75,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jnst"  ,  "\x8STC = 0) \x9" },
{ 19,  76,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jleu"  ,  "\x8(uns) \x6 <= \x5) \x9" },
{ 19,  78,  0, 1,0, 0,2,0,0, J|C|Y     ,0   ,"jnc"   ,  "if (CY = 0) \x9" },

{ 19,  77,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jgt"   ,  "\x8\x6 > \x5) \x9" },  // 70
{ 19,  79,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jnvt"  ,  "\x8OVT = 0) \x9" },
{ 19,  80,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jnv"   ,  "\x8OVF = 0) \x9" },
{ 19,  81,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jge"   ,  "\x8\x6 >= \x5) \x9" },
{ 19,  82,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jne"   ,  "\x8\x6 != \x5) \x9" },
{ 19,  67,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jst"   ,  "\x8STC = 1) \x9" },
{ 19,  68,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jgtu"  ,  "\x8(uns) \x6 > \x5 ) \x9" },
{ 19,  70,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jle"   ,  "\x8\x6 <= \x5) \x9" },
{ 19,  69,  0, 1,0, 0,2,0,1, J|C|Y     ,0   ,"jc"    ,  "if (CY = 1) \x9" },
{ 19,  71,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jvt"   ,  "\x8OVT = 1) \x9" },
 
{ 19,  72,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jv"    ,  "\x8OVF = 1) \x9" },         // 80
{ 19,  73,  0, 1,0, 0,2,0,0, J|C       ,0   ,"jlt"   ,  "\x8\x6 < \x5) \x9" },
{ 19,  74,  0, 1,0, 0,2,0,0, J|C       ,0   ,"je"    ,  "\x8\x6 = \x5) \x9" },
{ 18,  0,   0, 2,2, 2,2,1,0, J|D       ,0   ,"djnz"  ,  "\x2--;\xb\x8\x2 != 0) \x9" }, 

{ 16,  0,   0, 1,0, 0,2,1,0, J|B|E     ,0   ,"sjmp"  ,  "\x9" },
{ 17,  93,  0, 1,0, 0,2,1,0, J|B|X     ,0   ,"scall" ,  "\x1\xc" },
{ 23,  87,  0, 2,0, 0,2,1,1, J|T       ,0   ,"jnb"   ,  "\x8\x3\x2 = 0) \x9" }, 
{ 23,  86,  0, 2,0, 0,2,1,1, J|T       ,0   ,"jb"    ,  "\x8\x3\x2 = 1) \x9" },
{ 16,  0,   0, 0,0, 0,2,0,0, J|K|E     ,0   ,"skip"  ,  "\x9" },           // skip = sjmp [pc+2]
{ 16,  0,   0, 1,0, 0,2,0,0, J|E|L     ,0   ,"jump"  ,  "\x9" },              

 17,  94,  0,  1,0, 0,2,0,0, J|X|L     ,0   ,"call"  ,  "\x1\xc" ,               // 90
 20,  95,  0,  0,0, 0,2,0,0, J|E|F     ,0   ,"ret"   ,  "return;", 
 20,  96,  0,  0,0, 0,2,0,0, J|E|F     ,0   ,"reti"  ,  "return;" ,
 17,  0,   0,  1,0, 0,0,0,0, J|H|B|X   ,0   ,"sclla"  , "\x1\xc",
 17,  0,   0,  2,0, 0,0,0,0, J|H|L|X   ,0   ,"calla"  , "\x1\xc" ,
 20,  0,   0,  0,0, 0,0,0,0, J|H|E|F   ,0   ,"reta"   , "return;" ,
 20,  0,   0,  0,0, 0,0,0,0, J|H|E|F   ,0   ,"retia"  , "return;" ,
 41,  99,  0,  0,0, 0,0,0,0, 0         ,0   ,"pushp" ,  "push(PSW);" ,
 42,  100, 1,  0,0, 0,0,0,0, 0         ,0   ,"popp"  ,  "PSW = pop();" ,
 40,  0,   0,  0,0, 0,0,0,0, H         ,0   ,"pshpa"  , "pusha(PSW);" ,
 
 41,  0,   1,  0,0, 0,0,0,0, H         ,0   ,"poppa"  , "PSW = popa(););",          // 100
 21,  0,   1,  0,0, 0,0,0,0, Y         ,0   ,"clc"   ,  "CY = 0;" ,
 21,  0,   1,  0,0, 0,0,0,0, Y         ,0   ,"stc"   ,  "CY = 1;" ,
 43,  0,   0,  0,0, 0,0,0,0, 0         ,0   ,"di"    ,  "disable ints;" ,
 44,  0,   0,  0,0, 0,0,0,0, 0         ,0   ,"ei"    ,  "enable ints;" , 
 48,  0,   0,  0,0, 0,0,0,0, 0         ,0   ,"clrvt" ,  "OVT = 0" ,
 47,  0,   0,  0,0, 0,0,0,0, 0         ,0   ,"nop"   ,  "" ,
 45,  0,   0,  0,0, 0,0,0,0, H|R       ,0   ,"bnk0"  ,  "" ,
 45,  0,   0,  0,0, 0,0,0,0, H|R       ,0   ,"bnk1"  ,  "" ,
 45,  0,   0,  0,0, 0,0,0,0, H|R       ,0   ,"bnk2"  ,  "" ,
 
 45,  0,   0,  0,0, 0,0,0,0, H|R       ,0   ,"bnk3"  ,  "" ,    // 110
 
 46,  0,   0,  1,0, 0,1,0,0, H|B       ,0   ,"rbnk"  ,  "" ,  // 111 bank prefix.
 40,  0,   0,  0,0, 0,0,0,0, 0         ,0   ,"!PRE!" ,  ""    // 112 signed prefix op

 };



// special modded opcode sources
const char *scespec[] = {
"\x2 += CY;", 
"\x2 -= CY;"

 };
 
// interrupt function names in array for auto naming
// "HSI_" and default name is output as prefix in code
// first flag 0 is 8061, 1 is 8065
// second flag is whether to add number to string or not
// switched off with PINTF   (opts & N )

NAMES inames[] = {
 0,   0, 0, 0, "HSO_2",            //  8061 ints - actual at 2010
 1,   1, 0, 0, "Timer_OVF",
 2,   2, 0, 0, "AD_Rdy",
 3,   3, 0, 0, "HSI_Data",
 4,   4, 0, 0, "External",
 5,   5, 0, 0, "HSO_1",
 6,   6, 0, 0, "HSI_1",
 7,   7, 0, 0, "HSI_0",            // 201f  - end 8061 list

 0,  15, 1, 1, "HSO_",           // 8065 ints - 2010-201f (plus num)
 16, 16, 1, 0, "HSI_FIFO",
 17, 17, 1, 0, "External",
 18, 18, 1, 0, "HSI_0",
 19, 19, 1, 0, "HSI_Data",
 20, 20, 1, 0, "HSI_1",
 21, 21, 1, 0, "AD_Imm_Rdy",
 22, 22, 1, 0, "AD_Timed_Rdy",
 23, 23, 1, 0, "ATimer_OVF",
 24, 24, 1, 0, "AD_Timed_Start",
 25, 25, 1, 0, "ATimer_reset",
 26, 29, 1, 1, "Counter_",            // add num
 30, 39, 1, 1, "Software_"           // add num 2040-205f    end 8065
};


// Auto names, which have numbered suffix.
// current num, mask, base string

AUTON anames [] = {
 {1, 0, "Timer"     },        // used as a SPECIAL for timers.
 {1, P, "Sub"     },          // 1   PPROC   proc names (xfunc, vfunc, intfunc)
 {1, L, "Lab"     },          // 2   PLABEL  label names (lab)
 {1, F, "Func"    },          // 3   PFUNC   function data names
 {1, F, "Table"   },          // 4   PFUNC   table data names

 {1, G, "UUBFunLU" },          // 5 signs   auto subroutine names for function and table lookup procs
 {1, G, "USBFunLU" },  
 {1, G, "SUBFunLU" },  
 {1, G, "SSBFunLU" },  
 
 {1, G, "UUWFunLU" },         // 9
 {1, G, "USWFunLU" },  
 {1, G, "SUWFunLU" },  
 {1, G, "SSWFunLU" },  
 
 {1, G, "UTabLU"  },          // was 7, now 13
 {1, G, "STabLU"  }            // 14
 

 };


/***** default symbols *******/

   // name, addr, bitno, flag (8065 = 2,8061 = 1), write flag

DFSYM defsyms [] = {
{ "CPU_OK"        ,    2,   6, 3,0 },     //both 61 and 65
{ "LSO_Port"      ,    2,  -1, 3,0 },
{ "LIO_Port"      ,    3,  -1, 3,0 },
{ "AD_Low"        ,    4,  -1, 3,0 },
{ "AD_High"       ,    5,  -1, 3,0 },
{ "IO_Timer"      ,    6,  -1, 3,0 },
{ "IO_Timer_Hi"   ,    7,  -1, 1,0 },     // 61 only
{ "IO_Timer_Mid"  ,    7,  -1, 2,0 },     // 65 only
{ "INT_Mask"      ,    8,  -1, 3,0 },
{ "INT_Pend"      ,    9,  -1, 3,0 },
 {"HSO_OVF"       ,  0xA ,  0, 3,0 },
 {"HSI_OVF"       ,  0xA ,  1, 3,0 },
 {"HSI_Ready"     ,  0xA ,  2, 3,0 },
 {"AD_Ready"      ,  0xA ,  3, 3,0 },
 {"INT_Service"   ,  0xA ,  4, 1,0 },           // 8061 only
 {"INT_Priority"  ,  0xA ,  5, 1,0 },
 {"MEM_Expand"    ,  0xA ,  4, 2,0 },           // 8065 only
 {"HSO_PCNT_OVF"  ,  0xA ,  5, 2,0 },
 {"AD_TMD_RDY"    ,  0xA ,  6, 2,0 },
 {"HSO_PORT_OVF"  ,  0xA ,  7, 2,0 },
 {"IO_Status"     ,  0xA , -1, 3,0 },
 {"HSI_Sample"    ,  0xB , -1, 3,0 },
 {"HSI_Mask"      ,  0xC , -1, 3,0 },
 {"HSI_Data"      ,  0xD , -1, 3,0 },
 {"HSI_Time"      ,  0xE , -1, 3,0 },
 {"HSI_Time_Hi"   ,  0xF , -1, 1,0 },
 {"AD_TMD_HI"     ,  0xF , -1, 2,0 },

 {"STACK"         ,  0x10, -1, 1,0 },        // 8061 stack
 {"HSO_IntPend1"  ,  0x10, -1, 2,0 },        // 8065 only
 {"HSO_IntMask1"  ,  0x12, -1, 2,0 },
 {"IO_Timer_Hi"   ,  0x13, -1, 2,0 },
 {"HSO_IntPend2"  ,  0x14, -1, 2,0 },
 {"LSSI_A"        ,  0x15, -1, 2,0 },
 {"HSO_IntMask2"  ,  0x16, -1, 2,0 },
 {"LSSI_B"        ,  0x17, -1, 2,0 },
 {"HSO_Status"    ,  0x18, -1, 2,0 },
 {"LSSI_C"        ,  0x19, -1, 2,0 },
 {"HSI_MODE"      ,  0x1A, -1, 2,0 },
 {"HSO_UCNT"      ,  0x1B, -1, 2,0 },
 {"HSO_TIMEC"     ,  0x1C, -1, 2,0 },
 {"LSSI_D"        ,  0x1D, -1, 2,0 },
 {"HSO_Last"      ,  0x1E, -1, 2,0 },
 {"HSO_Select"    ,  0x1F, -1, 2,0 },
 {"STACK"         ,  0x20, -1, 2,0 },        // 8065 stacks
 {"ALTSTACK"      ,  0x22, -1, 2,0 },

 {"AD_Cmd"        , 4    , -1, 3,1 },           // Write symbols from here
 {"WDG_Timer"     , 5    , -1, 3,1 },
 {"AD_TMD_CMD"    , 7    , -1, 2,1 },
 {"HSO_Cmd"       , 0xd  , -1, 3,1 },
 {"HSO_Time"      , 0xe  , -1, 3,1 },
 {"HSO_Time_Hi"   , 0xf  , -1, 3,1 },
 {"BANK_SEL"      , 0x11,  -1, 2,1 },
 {"LSSO_A"        , 0x15 , -1, 2,1 },
 {"LSSO_B"        , 0x17 , -1, 2,1 },
 {"LSSO_C"        , 0x19 , -1, 2,1 },
 {"LSSO_D"        , 0x1D , -1, 2,1 },
};

/**************************************************/

HDATA fldata;                 // files data holder
char pathchar;

// IBUF - FIRST 8192 (0x2000) reserved for RAM and reg data, then 4*0x10000 banks,
// ref'ed by nbuf pointers in bkmap  (with PCORG offset).  bkmap[BMAX] is used for RAM [0-1fff]
// opbit is used for opcode start flags

uchar ibuf[0x42000];              // 4 banks of 0xffff + 2000 ram at front
uint  opbit[0x2000];              // 4 banks of 32 flags (0x800 each = 0x10000 flags)

BANK bkmap[BMAX+1];               // bank map - allow for full range of 16 banks PLUS 1 for register data.

BANK *lastbmap;                   // test for speedup of mapping


// size, fieldwidth, mask (for -ve). 

int  casz[8] = {0,1,2,4,0,1,2,4};       // data size, byte,short,long, unsigned, signed
int  cafw[8] = {0,3,5,7,0,4,6,8};       // field width, byte,short,long, unsigned, unsigned
uint camk[8] = {0,0xff,0xffff,0xffffffff,0,0xff,0xffff,0xffffffff};


/* positions of columns in printout
28      where opcode mnemonic starts (after header) 26 no bank prefix
34      where opcode data starts (+6)
51      where pseudo source code starts  (+17)
85      where comment starts      (+34)
180     max width of page - wrap here
*/

int cposn[5] = {26, 32, 49, 83, 180};

int lastpad;                // last padded column
//int xcalls  = 0;            // progress bar updates to screen - ref'ed externally
//int mcalls  = 0;            // malloc calls
//int maxholds = 0;           // for debug.
//int holdsp  = 0;
//int mrels   = 0;            // malloc releases
//int wtcnt   = 0;            // watch count

int maxalloc = 0;           //  TEMP for malloc tracing
int curalloc = 0;
int tscans = 0;             // total scan count, for safety

int  anlpass    = 0;        // number of passes done so far
int  cmdopts    = 0;        // command options

int  numbanks;              // number of 8065 banks-1

SYT  *snam[16];             // sym names temp holder for bitwise flags
char flbuf[256];            // for directives and comment file reads
char nm [128];              // for temp name construction, etc.

int  gcol;                  // where print is at (no of chars = colno)

int cmntofst;               // last addr for comment
char *cmtt;                 // last comment text posn(in flbuf)
char autocmt [128];         // auto added comment(s)
INST *cmntinst;             // auto added comment for operands;

int psw;
//struct ?
int datbank;                // currently selected data bank - always 8 if single bank
int codbank;                // currently selected code bank
int rambank;                // currently selected RAM bank - always 0 for 8061

// opcode and operands pile - 256 deep, plus a dummy entry [256] for compare print

#define PILESZ  128

INST instpile[PILESZ];        // last opcodes list
OPS  *curops;                // pointer to current operands
INST *curinst;               // current instruction

int lpix;                    // index for opcode array

int minwreg, maxwreg;                 // min ram update register (i.e less than stack reg)
int stkreg1, stkreg2;        // stack registers, 2 for 8065

int schblk[30];              // search block, used for find - overlayed with whatever struct
ushort lram[NPARS];           // (16) last updated ram for HOLD (address & size)

void *shuffle[16];           // for chain reorganises

/********** file open flags for linux and Win32 ********************/

#ifdef __linux__
   HOP fd[] ={{".bin","r"},    {"_lst.txt","w"}, {"_msg.txt","w"},
              {"_dir.txt","r"},{"_cmt.txt","r"}, {"sad.ini","r"}  };

#else

   HOP fd[] ={{".bin","rb"},    {"_lst.txt","wt"}, {"_msg.txt","wt"},
              {"_dir.txt","rt"},{"_cmt.txt","rt"}, {"sad.ini","rt"}  };
#endif


/************************
 * chaining and admin stuff
 **************************/

int cmdaddr(int add)                // command addr option for multibanks
{
 if (!numbanks) add &= 0xffff;
 return add;
}

int wnprt (const char *fmt, ...)   // warning file prt
{
  va_list args;

  if (!fmt ) return 0;
  va_start (args, fmt);
  vfprintf (fldata.fl[2], fmt, args);
  va_end (args);
  return 1;             // useful for if statements
}

int wnprtn (const char *fmt, ...)   // warning file prt with newline
{
  va_list args;

  if (fmt)
   {
    va_start (args, fmt);
    vfprintf (fldata.fl[2], fmt, args);
    va_end (args);
   }


  fprintf(fldata.fl[2], "\n");
  va_end (args);
  if (fmt) return 1;             // useful for if statements
  return 0;
}


int wflprt(const char *fmt, float fv)
 {           // separate float print
  char *tx;
  int cs;

  wnprt(fmt);
  cs = sprintf(nm+32,"%.3f",fv);
  tx = nm+31+cs;
  while (*tx == '0') tx--;
  if (*tx == '.') tx--;
  *(tx+1) = '\0';
  wnprt(nm+32);
  wnprt(" ");
 return tx-nm-32;
}


void DBGPBK(LBK *b, const char *fmt, ...)       // debug print warn blk
{
  va_list args;

  if (!b) return;

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[2], fmt, args);
     va_end (args);
    }

  wnprt(" %s %x %x ",  dirs[b->fcom].com, b->start, b->end);
  va_end (args);
}

void DBGPBKN(LBK *b, const char *fmt, ...)
{
  va_list args;
  if (!b) return;
  va_start (args, fmt);
  DBGPBK(b,fmt,args);
  va_end (args);
  DBGPRTN(0);
}


void p_pad(int c)
{
// list file. pad out to required column number, if not already there

int i;

if  (c <= gcol) c = gcol+1;                // always add at least one space...
for (i = gcol; i < c; i++)  fprintf(fldata.fl[1], " ");
gcol = c;
lastpad = c;                         // last padded column
}

void newl (int c)
{                  // add return(s) and reset colno
  if (c <= 0) c = 1;
  while (c > 0) {fprintf(fldata.fl[1], "\n"); c--;}
  gcol = 0;
}

void wrap()
 {
  // newline and wrap back to comments column if already greater,
  // or lastpad column if not
  fprintf(fldata.fl[1], "\n");
  gcol = 0;
  if (lastpad > cposn[3])  lastpad = cposn[3];
  p_pad(lastpad);
 }

void pchar(char z)
{
 fputc(z,fldata.fl[1]);
 gcol++;
 if (gcol >= cposn[4]) wrap();
}

void pstr (const char *fmt, ...)
{   // output file print
  va_list argptr;

  if (!fmt || !*fmt ) return;
  va_start (argptr, fmt);
  gcol += vfprintf (fldata.fl[1], fmt, argptr);
  va_end (argptr);
  if (gcol >= cposn[4]) wrap();
}


/************************
 * master exit and cleanup routines
  **************************/


 void closefiles(void)
{
  int i;

  for (i=0; i<5; i++)         // include sad.ini
   {
    if (fldata.fl[i])         // close file
       {
         fflush (fldata.fl[i]);
         fclose (fldata.fl[i]);
         fldata.fl[i] = NULL;
       }
   }
  fldata.fillen = 0;             //flag marker.
}


void mfree(void *addr, size_t size)
 {
  if (!addr) return;     // safety

  curalloc -= size;    // keep track of mallocs
//  mrels++;
  free (addr);
  addr = 0;
 }


void* mem(void *ptr, size_t osize, size_t nsize)
{  // use realloc, as is same as malloc with zero osize ptr
  void *ans;
  if (nsize < 1) wnprtn ("Invalid malloc size");

  if (ptr)  curalloc -= osize;
//  mcalls++;
  curalloc += nsize;
  if (curalloc > maxalloc) maxalloc = curalloc;
  ans = realloc(ptr, nsize);

  if (!ans) wnprtn("Run out of malloc space");
  return ans;
}


short chk_cadt(CADT *a)
{

 int safe, safe2;
 CADT *b;

 safe = 0;

 while (a && safe < 50 )
  {
   b = a->next;
   safe2 = 0;
   while (b && safe2 < 50)
     {
      if (b == a) return 1;       //loop found
      b = b->next;                // next item to check
      safe2++;
     }
   if (safe2 >=50) return 2;
   a = a->next;
   safe++;
  }
 if (safe >=50) return 2;
 return 0;
 }

int freecadt(CADT **z)
 {
  CADT *n,*x;
  int a;

  x = *z;
  a = chk_cadt(x);
  if (a)
    {
//     DBGPRTN("CADT chain err %d",a);
     return 1;
    }

  while (x)
   {
    n = x->next;
    mfree (x, sizeof(CADT));
    x = n;
   }
 *z = NULL;
 return 0;
 }

int freesigc(CSIG **z)
 {

  CSIG *n, *x;

  x = *z;
  while (x)
   {
    n = x->next;
    mfree (x, sizeof(CSIG));
    x = n;
   }
 *z = NULL;
 return 0;
 }


void fsym(void *x)             // free symbol
{
 SYT *s;
 int z;
 s = (SYT*) x;
 z = 0;
 if (s->name) z = strlen(s->name) + 1;    // string plus 0 byte
 if (z)  mfree (s->name, z);     // symbol name string
 s->name = NULL;
}

void fsub(void *x)             // subr
{
  SXT *p;

  p = (SXT*) x;

  if (p->sdat)
   {
    freecadt (&(p->sdat->cadt));                   //cadt and sigs first
    freesigc (&(p->sdat->sigc));
    mfree    (p->sdat, sizeof(SXDT));              // then ptr stct
    p->sdat = NULL;
   }
 }

void fcmd(void *x)       // cmd
{
  LBK *k;
  k = (LBK*) x;
  freecadt (&(k->adnl));
  }

void fscn(void *x)       // Scan
{
  SBK *k;
  k = (SBK *) x;
  mfree(k->pars, sizeof(SPARS));
  k->pars = 0;
 
}

void fblk(void *x)       // main  malloced blocks tracker (done last)
{
  MBL *k;
  k = (MBL *) x;
  mfree(k->blk, k->size);
}

void clearchain(CHAIN *x)
{             // clear pointer list contents only
 int i;

 //  count DOWN so that for mblks first entry (points to itself) is done last.
 
 for (i = x->num-1; i >=0; i--)    
    {
     if (x->cfree)  x->cfree(x->ptrs[i]);
    }
    
 x->num = 0;
 x->max = 0;
 
 }


void freeptrs(CHAIN *x)
{
    // free main pointer block of each chain

 if (x->pnum)
   {
    mfree(x->ptrs, x->pnum * sizeof(void*));
    x->ptrs = NULL;
    x->pnum = 0;
   }
}

void freechain(CHAIN *x)
{
  int size;

  size =  x->pnum * sizeof(void *);     // num ptrs of void*
  size += x->pnum * x->esize;          // + structs pointed to
//  DBGPRTN("max=%4d tot=%4d allocs=%2d esize=%2d tsize=%d)", x->max, x->pnum, x->allocs, x->esize, size);
  clearchain(x);
  freeptrs(x);
}

void  free_structs ()
{                        // complete tidyup for closing or restart
 uint i;

 for (i = 0; i < sizeof(chlist)/sizeof(chlist[0]); i++)
  {
   freechain(chlist[i]);           // this includes mblock 
  }

 //     DBGPRTN ("max alloc = %d", maxalloc);
//      DBGPRTN ("cur alloc = %d", curalloc);
//      DBGPRTN ("mcalls = %d", mcalls);
//      DBGPRTN ("mrels  = %d", mrels);
//      DBGPRTN ("prog bar updates = %d", xcalls);
//      DBGPRTN ("watch reg updates = %d", wtcnt);
 //     DBGPRTN ("max holds = %d", maxholds);

   fflush (fldata.fl[1]);
   fflush (fldata.fl[2]);
}


void adpch(CHAIN *);    // declare for logblk  

void logblk(void *blk, size_t size)
{
 MBL *b;

 if (!blk) return;

 if (mblk.num >= mblk.pnum-1) adpch(&mblk);      // need more pointers     

 b = (MBL*) mblk.ptrs[mblk.num];  // no point in using chmem
 b->blk = blk;
 b->size = size;
 mblk.num ++;
 if (mblk.num > mblk.max) mblk.max = mblk.num;      // for debug really
}

 //----------------------------------------------------------------
 // add pointers for chains. malloced entries
 // pointer block is realloced, so is continuous...
 // but data blocks are NOT....
 //----------------------------------------------------------------

void adpch(CHAIN *x)      // add pointers for malloced entries
 {              // pointer IS realloced, so IS continuous...
 int old, nw, msz,i;
 char *z, *n;
                       // grow pointer array - always as single block for arrays
    old = x->pnum;
    x->pnum += x->asize;
    x->asize += x->asize/4;      // add 25% for next alloc
    x->allocs++;

    nw = x->pnum;            // new size
    x->ptrs = (void**) mem(x->ptrs, old*sizeof(void*), nw*sizeof(void*));

    // allocate new entries if not a remote chain, and keep them in mbl chain
    
    n = NULL;

    if (x->esize)
     {
      msz = x->esize*x->asize;           // size of new data block to go with ptrs
      n =  (char *) mem(0,0,msz);        // null check reqd
 //     if (!n) DBGPRT("MALLOC ERROR ");
     }

    z = n;
    for (i = old;  i < nw; i++)
     {
      x->ptrs[i] = z;
      z += x->esize;
     }
     logblk(n, msz); 
 
 }


void* chmem(CHAIN *x)
 {
    // get a free entry of the chain FOR INSERT.
  void *ptr;

  if (!x->esize) return NULL;         // can't have memory on a remote chain...

  if (x->num >= x->pnum-1) adpch(x);     // need more pointers

  ptr = x->ptrs[x->num];              // use next free block (at end of list)
  memset(ptr,0, x->esize);            // and clear it
  return ptr;
 }


void* schmem(void)
 {
    // get a free block for chain searches, from reserved block.
  memset(schblk,0, sizeof(int)*30);           // clear block
  return schblk;
 }
 
 
void chinsert(CHAIN *x, int ix, void *blk)
{
  // insert new pointer (= block) at ix
  // relies on main pointer block being contiguous

 if (!blk) return ;

 if (x->num >= x->pnum-1) adpch(x);      // need more pointers

 // move entries up one from ix to make a space.
 memmove(x->ptrs+ix+1,x->ptrs+ix, sizeof(void*) * (x->num-ix));  // fastest for contiguous

 x->ptrs[ix] = blk;                         // insert new block at ix
 x->num ++;
 if (x->num > x->max) x->max = x->num;      // for debug really
}


void chrelease(CHAIN *x, int ix, int num)
{
 // release num block(s) from ix, shuffle down and restore ptr to top after num
 // relies on main pointer block being contiguous
 // shuffle array to hold pointers (max 16)
 int i;

 if (ix < 0 || ix >= x->num) return;  //// + num?
 
 if (x->cfree)
   {
    for (i = ix;  i < ix+num; i++) x->cfree(x->ptrs[i]);
   }
 
 memcpy(shuffle, x->ptrs+ix, sizeof(void*) * num);                           // save 'num' pointers

 // move entries down by 'num' from ix+num and then restore pointers to top of pointers, after (new) num.
 memmove(x->ptrs+ix,x->ptrs+ix+num, sizeof(void*) * (x->num-num-ix));  // fastest for contiguous
 memcpy(x->ptrs + x->num - num, shuffle, sizeof(void*) * num);
 x->num -= num;
}



int minadd(int addr)
{
 return bkmap[(addr >> 16) & 0xf].minpc;
}                                 // min address is bank|PCORG

int maxadd (int addr)
{
 return bkmap[(addr >> 16) & 0xf].maxpc;
}                                 // max address for this bank

bool val_code_addr(int addr)
 {
 // return 1 if valid 0 if invalid.
 // CODE addresses - no data fixup

 //val jump addr with list of OK (for console)
 
 int bank;
 BANK *b;

 bank = (addr >> 16) & 0xf;        // NOT dbank call
 b = bkmap+bank;

 if (!b->bok) return 0;           // invalid

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // now where fill starts

 return 1;        // within valid range, OK
 }

int dbank(int addr)
{
  // get data bank. has below PCORG fixup  
 int bank;
 
 if ((addr & 0xffff) < PCORG) return BMAX;  // register and ram fixup.
 bank = (addr >> 16) & 0xf;
 return bank;                             // and return as an int 0-16
}


bool val_jmp_addr(int addr)
 {
 // return 1 if valid 0 if invalid.
 // JUMP addresses

 //val jump addr with list of extra OK (for console)
 
 int x;
 BANK *b;

 x = (addr >> 16) & 0xf;        // NOT dbank call
 b = bkmap+x;

 if (addr >= 0x8d000 && addr <= 0x8d010) return 1; // cal console
 if (addr == 0x1f1c) return 1; // DUCE chip ??
 
 if (!b->bok) return 0;           // invalid

 if (addr < b->minpc) return 0;   // minpc = PCORG
 if (addr > b->maxpc) return 0;   // now where fill starts

 return 1;        // within valid range, OK
 }


bool val_data_addr(int addr)

{
 // return 1 if valid 0 if invalid.
 // DATA addresses

 int bank;
 BANK *b;

 bank = dbank(addr);
 b = bkmap+bank;                 // handles < PCORG

 if (!b->bok) return 0;          // invalid

 if (addr <= b->minpc) return 0;  // PCORG (and zero) is invalid
 if (addr > b->maxpc) return 0;

 return 1;        // within valid range, OK
 }


uchar* map_addr(int addr)
 {
  // only used by g_ subrs so allow maxbk as limit (so fill can be read)

  int bank;
  BANK *b;

  // do these first as speedup to save remapping - may be an even faster trick ?

  if ((addr & 0xffff) < PCORG) return bkmap[BMAX].fbuf;
  if (addr >= lastbmap->minpc && addr <= lastbmap->maxbk) return lastbmap->fbuf;

  bank = (addr >> 16) & 0xf;
  b = bkmap+bank;

  if (!b->bok || addr > b->maxbk) return NULL;  // invalid

  lastbmap = b;
  return b->fbuf;         // valid address, return pointer

 }

int icaddrse(int *start, int *end)
{                                      // for start and end checks of COMMANDS - not below PCORG
 int addr;

 if (*end <= 0xffff) *end |= (*start & 0xf0000);     // use bank from start addr

 if ((*end & 0xf0000) != (*start & 0xf0000))
    {
 //     DBGPRTN ("Invalid - diff banks %x-%x", *start, *end);
      return 1;
    }


 if (val_code_addr(*start) && val_code_addr(*end))
    {
     addr = maxadd(*start);
     if (*end > addr)   *end = addr;
     if (*end < *start) *end = *start;
     return 0;
    }
// if (*start >= PCORG)  DBGPRTN ("Invalid address %x-%x", *start, *end);
 return 1;
}


//----------------------------------------------------------


int opbank(int xbank)
 {
   // simple way to get BANK opcode to override default  
  if (curinst->bnk) return curinst->bank <<16;
 return xbank;
 }


/*---------------------------------------------------------------------------------------*/

// cp procs used mainly by bfindix (binary chop)
// NB.  straight subtract works for chain ordering, even for pointers
// < 0 less than, 0 equals, > 0 greater than  

int cpfjp (int ix, void *d)
{
    // 'from' jump chain
  int ans;
  JLT *j, *t;
 
  if (ix >= jpfch.num) return -1; 
  
  j = (JLT*)jpfch.ptrs[ix];
  t = (JLT*) d;

  ans = j->from - t->from;

  if (ans) return ans;

  if (t->type) ans = j->type - t->type;

  return ans;
}


int cptjp (int ix, void *d)
{
  // 'to' jump chain  
  int ans;
  JLT *j, *t;
  
  if (ix >= jptch.num) return -1;
  
  j = (JLT*)jptch.ptrs[ix];
  t = (JLT*) d;

  ans = j->to - t->to;
  
  if (ans) return ans;

  if (t->type) ans = j->type - t->type;

  return ans;
}


int cpsym (int ix, void *d)
{
 SYT *s, *t;
 int ans;
 if (ix >= symch.num) return -1;

 s = (SYT*) symch.ptrs[ix];
 t = (SYT*) d;
 ans = s->add - t->add;
 
 if (ans) return ans;

 ans = s->bitno - t->bitno;
 if (ans) return ans;

// mod this ?    if write set, can still return READ....same for bitno

 ans = s->writ - t->writ;
 return ans;
}


int cpbase (int ix, void *d)
{
 RBT *b, *t;

 if (ix >= basech.num) return -1;
 b = (RBT*)basech.ptrs[ix];
 t = (RBT*) d;
 
 return b->radd - t->radd;
}


int cpsig (int ix, void *d)
{
 SIG *g, *t;
 int ans;

 if (ix >= sigch.num) return -1;
 g = (SIG*) sigch.ptrs[ix];
 t = (SIG*) d;
 ans =  g->ofst - t->ofst;
 if (ans) return ans;

 if (t->ptn) ans = g->ptn - t->ptn;
 return ans;
}


int cpddt (int ix, void *d)
{
 DDT *g, *t;
 int ans;

 if (ix >= datch.num) return -1;

 g = (DDT*) datch.ptrs[ix];
 t = (DDT*) d;
 ans = g->start - t->start;

// if (ans) return ans;
// if (t->from) ans = g->from - t->from;   // disable for now, and increments.
 
 return ans;
}



int cpscan (int ix, void *d)
{
  // need to check FROM (retaddr) so that alternative scan params (i.e. tables etc) get picked
  // blk is what you would insert (or find match to)

 SBK *s, *t;
 int ans;

 if (ix >= scanch.num) return -1;

 s = (SBK*) scanch.ptrs[ix];
 t = (SBK*) d;
 
 ans = s->start - t->start;

 if (ans) return ans;

 if (t->retaddr)  ans = s->retaddr - t->retaddr;        // return address

 if (ans) return ans;

 if (t->sub)  ans = s->sub - t->sub;               // and subr
 
 return ans;
 
} 


int cpsub (int ix, void *d)
{
 // add a check here for pushp subrs.
 SXT *s, *t;

 if (ix >= subch.num) return -1;

 s = (SXT*) subch.ptrs[ix];
 t = (SXT*) d;

 if (s->psp && t->addr == s->addr+1)  return 0;     // pushp match

 return s->addr - t->addr;
}



int cpcmd (int ix, void *d)
{
 LBK *b, *t;     // commands check END address, not start
 int ans;
 
 if (ix >= cmdch.num) return -1;
 b = (LBK*) cmdch.ptrs[ix];
 t = (LBK*) d;

 ans = b->end - t->start;

// if (pc >= b->start && pc <= b->end )
//    {
//     if (!par1 || par1 == b->fcom) return 0;
//    }
    
 return ans;
 
}

int cpacmd (int ix, void *d)             //int addr, ulong dummy)
{
 LBK *b, *t;     // commands check END address, not start
 int ans;
 
 if (ix >= auxch.num) return -1;
 b = (LBK*) auxch.ptrs[ix];
 t = (LBK*) d;
 
 ans = b->end - t->start;

// if (pc >= b->start && pc <= b->end )
//    {
//     if (!par1 || par1 == b->fcom) return 0;
//    }
    
 return ans;
 
}

int eqcmd (int ix, void *d)
{
 LBK *b, *t;

 if (ix >= cmdch.num) return -1;

 b = (LBK*) cmdch.ptrs[ix];
 t = (LBK*) d;
 // is one of next checks redundant after cpcmd ? should this be ix-1 ?

 if (t->start >= b->start && t->start <= b->end )
    {
     if (!t->fcom || t->fcom == b->fcom) return 0;
    }
 return 1;
}

int eqacmd (int ix,  void *d)          //int pc, ulong par1)
{
 LBK *b, *t;

 if (ix >= auxch.num) return -1;

 b = (LBK*) auxch.ptrs[ix];
 t = (LBK*) d;
 // is one of next checks redundant after cpcmd ? should this be ix-1 ?

 if (t->start >= b->start && t->start <= b->end )
    {
     if (!t->fcom || t->fcom == b->fcom) return 0;
    }
 return 1;
}


int bfindix(CHAIN *x, void *blk)
{
  // generic binary chop subr
  // send back INDEX value to allow access to adjacent blocks.
  // use a struct match specified chain, so is void *
  // answers so that min-1 is always smaller than blk, and min is blk match (or larger).

  int mid, min, max;

  min = 0;
  max = x->num;

  while (min < max)
    {
      mid = min + ((max - min) / 2);      // safest way to do midpoint (imin, imax);
      if(x->comp(mid,blk) < 0)            // blk < mid, so this becomes new 'bottom'
         min = mid + 1; else  max = mid;  // otherwise becomes new 'top'
    }

  return min;
}


// special checking for LBK

int olchk(CHAIN *x, int i, int ix)
 {

   /***** overlap and overwrite check if - commands can be merged/overwrite etc
   * b is 'master' (new) block at ix , t is existing (test) block at i
   * set answer as bit mask -
     L   overlaps - this is flag then used to do something
     S   block is spanned - t within blk
     W   overwrite (command) allowed
     V   overwrite (other way) allowed
     C   contains - blk within t (reverse of spanned)
     M   merge allowed
     Q   equal
   ************/

   LBK *t, *b;
   int ans;
   
   if (i < 0 || ix < 0) return 0;
   if (i > x->num || ix > x->num) return 0;

   t = (LBK*) x->ptrs[i];
   b = (LBK*) x->ptrs[ix];     // block just inserted

   ans = 0;   //clear all flags;

   if (t == b) return 0;                     // safety

   if (t->start == b->start && t->end == b->end && t->fcom == b->fcom) return Q;                      // deemed equal


// now check for overlaps

   if (b->start >= t->start && b->end <= t->end)  {ans |= L; ans |= C;}  // contains
   else
   if (b->start < t->start && b->end > t->end)  {ans |= L; ans |= S;}  // spans
   else
   if (t->start >= b->start && t->start <= b->end)  ans |= L;        // overlap (start)
   if (t->end   >= b->start && t->end   <= b->end)  ans |= L;        //overlap (end)

   if (ans & L)
    {
     if (!t->cmd && (dirs[b->fcom].ovrde & (1 << t->fcom))) ans |= W;  // if overwrite flag set
     if (!b->cmd && (dirs[t->fcom].ovrde & (1 << b->fcom))) ans |= V;
    }

   if (dirs[t->fcom].merge && t->fcom == b->fcom)
    {
      // check there are no cadt extras first.
      if (!t->adnl && !b->adnl)  
       {
        if (ans & L) ans |= M;          // overlap set, merge allowed
        if (b->end == t->start-1 || b->start == t->end+1) {ans |= (L|M);}       //adjacent
       } 
    }

 return ans;
}

void merge(LBK *t, LBK *s)
 { // merge two blocks, t = s update BOTH
  if (s->start < t->start) t->start = s->start;
  if (s->end   > t->end)   t->end   = s->end;

  if (t->start < s->start) s->start = t->start;
  if (t->end   > s->end)   s->end   = t->end;
 }

int split(LBK *m, LBK *s)
{
  // m is kept original and s is trimmed
  // s is either higher or lower, trim as reqd

  if (s->start >= m->start && s->start <= m->end)
    {
      s->start = m->end+1;
      return 1;
    } 
  if (s->end  >= m->start && s->end   <= m->end)
    {
     s->end   = m->start-1;

    }
  return 0;  
}


void DBGPRTFGS(int x)
{
    // temp for testing
  if (x & U) DBGPRT( "U");
  if (x & D) DBGPRT( "D");
  if (x & L) DBGPRT( "L");
  if (x & C) DBGPRT( "C");
  if (x & M) DBGPRT( "M");
  if (x & Q) DBGPRT( "Q");
  if (x & S) DBGPRT( "S");
  if (x & W) DBGPRT( "W");
  if (x & V) DBGPRT( "V");
  DBGPRT(" ");

}

     /* L   overlaps - this is flag then used to do something
      * P must split to keep new blk ?
      * S   block is spanned - t within blk
      * W   overwrite (command) allowed
      * C   contains - blk within t (reverse of spanned)
      * M   merge allowed
      * Q   equal
      */



int inschk(CHAIN *x, int *ix, LBK *b)
{

   // check if insert is allowed, fix any overlaps later.
   // b is insert candidate,   ix is where it would go

  LBK *l, *n;               // last, block for insert, next
  
  int chkn, chkl, min, max, ans;           // TEST


  // for olchk by pointer x->num is insert candidate....

  ans = 1;            // insert set

  min = (*ix)-1;
 // max = *ix;

 // chkn = olchk(x, max, x->num);             // check with next block
  
  //change to check upwards FIRST, as backwards should only ever have overlap ?   
 
 

  max = *ix;
  chkn = 1;
  
  while (chkn)
    {
     chkn = olchk(x, max, x->num);
     if  (!chkn) break;
 
  
     n = (LBK*) x->ptrs[max];   
     
     if (chkn & Q)                          // chkl won't be a Q
      {
       DBGPBKN(n,"Dupl");
       ans = 0;                           // exact match, don't insert
      }
     else
     if (chkn & L)
      {
      // overlaps with next block
       if (chkn & C)
         {
         // b already contained in l - do nothing
         DBGPBK(b,"Ins Chk N");  DBGPRTFGS(chkn);      DBGPBKN(n,"contained in");
          ans = 0;
         }
       else
       if (chkn & M)
        {
         // merge allowed
         DBGPBK(b,"Ins Chk N");    DBGPRTFGS(chkn);     DBGPBKN(n,"Merge with");
         merge(n,b);
         ans = 0;
        }
       else
        if (chkn & W)
        {
         // overwrite allowed
          DBGPBK(b,"Ins Chk N");   DBGPRTFGS(chkn);   DBGPBKN(n,"Split with");
         split(b,n);
         (*ix)++;                //  but insert now one up
         // and can still insert
        }
       else
        {
          // default catch
          DBGPBK(b,"Ins Chk N");   DBGPRTFGS(chkn);   DBGPBKN (n, "REJECTn");
          ans = 0;           // temp stop insert
        } 
      }
    
     max++;
     if (!ans) return ans;
    }

  if (! ans) return ans;
  
  chkl = 1;
  while (chkl)
   {  
     chkl = olchk(x, min, x->num);
     if  (!chkl) break;
      
     l = (LBK*) x->ptrs[min];   
     if (chkl & L)
        {
         // overlaps with last block
          if (chkl & M)
          {
           // merge allowed BUT NOT IF UPWARD OVERLAP !!!
           DBGPBK(b,"Ins Chk L"); DBGPRTFGS(chkl);  DBGPBKN(l,"Merge with");
           merge(l,b);
           ans = 0;
          }
        else {   ans = 0;     DBGPBK(b,"Ins Chk L"); DBGPRTFGS(chkl); DBGPBKN(l, "REJECT l!");}
        }
     min--;    // loop back for all overlaps
    }




 return ans;                    // insert 
}



LBK* inscmd(CHAIN *x, LBK *blk)
 {
   /* change to insert THEN fixup ?

   
    * at this call 'blk is always x->ptrs[x->num] ....
    * do a simple check,   THEN do more complex merging as necessary

   this works but does not merge anything yet.  THis assumes you never get an END overlap ?
   probably true for current scan setup, but not safe.....

   eqcmd only checks that start falls between start&end of chained block....

   NB.   Probably don't care about overlaps right now, can do later, but must capture
   any extended END markers.

   so have to check for exact match, and non_allowed overlaps/overrides ?

    */

  int ans, ix;      //, s;
   
  ans = 1;

  // blk is actually at ptr x->num...

  ix = bfindix(x, blk);                 // where to insert

  ans = inschk(x,&ix,blk);              // found - is insert allowed and can mod ix

  if (ans)  chinsert(x, ix, blk);            // do insert at ix

  if (ans) return blk;

  return (LBK*) x->ptrs[ix];

}

// match for signature blocks

SIG* inssig(SIG *blk)
 {

  int ans, ix;
  SIG *s;
  ans = 1;                  // insert by default

  ix = bfindix(&sigch, blk);

  ans = cpsig(ix,blk);

  if (!ans) return 0;             // straight duplicate
  
// check for duplicates with different skips at front...
// and check for overlap with next sig - but sign sig may be OK ?
// may need better check here for overlaps
  
  if (ix < sigch.num)
   {
    s = (SIG*) sigch.ptrs[ix];
   
   if (s->ptn == blk->ptn && s->ofst+s->skp == blk->ofst+blk->skp)
     {      // duplicate, only skips differ...
       return 0;
     }
   }


  if (ix > 0)
  {

   s = (SIG*) sigch.ptrs[ix-1];
   
   if (s->ptn == blk->ptn && s->ofst+s->skp == blk->ofst+blk->skp)
     {      // duplicate, only skips differ...
       return 0;
     }


   if (s->ofst <= blk->ofst && s->xend > blk->ofst)
     {
 //     DBGPRT("SIG OLP %x %s with %x ", blk->ofst, blk->ptn->name, s->ofst);
      if (!s->ptn->olap) return 0;
     }
     
  }

  chinsert(&sigch, ix, blk);      // do insert
  return blk;

}



DDT* add_ddt(int addr, int size, int from)
 {
  DDT *x;
  int ans,ix;

  if (anlpass >= ANLPRT)  return 0;

  ans = addr & 0xffff;
  if (ans < PCORG)  addr = ans;     // strip bank for < PCORG
  
  if (addr <= maxwreg) return NULL;


  
  if (!val_data_addr(addr))
   { 
 
    return NULL;
   }  



  x = (DDT *) chmem(&datch);      // next free block
  x->start  = addr;
  if (curinst->inr) x->from = from;   


  ix = bfindix(&datch, x);
  ans = cpddt(ix, x);

  if (ans)
    {          // no match

     x->from = from;
     x->imd = curinst->imd;
     if( curinst->inx)
       { if (curops[1].addr) x->inx = 1; else x->imd = 1; } 
     x->inr = curinst->inr;

     x->inc = curinst->inc;       // this is accumulated for "++"
     x->ssize = size;             // will not override tab or func....
     chinsert(&datch, ix, x);
     return x;
    }
 
   // Match - return matched block (for extra flags)
 return (DDT *) datch.ptrs[ix];
}

// used in lots of places....general chain finder - move to void * as matching struct

void *find(CHAIN *x, void *b)
{
 // find item with optional extra pars in b

 int ix;
 ix = bfindix(x, b);
 if (x->equal(ix,b)) return NULL;
 return x->ptrs[ix];

}


int g_byte (int addr)
{                    // addr is  encoded bank | addr
  uchar *b;

  b = map_addr(addr);
  if (!b) return 0;        // safety

  return (int) b[(addr & 0xffff)];
}


int g_word (int addr)                       //  unsigned word
{                                           // addr is  encoded bank | addr
  int val;
  uchar *b;

  b = map_addr(addr);                      // map to required bank
  if (!b)  return 0;

  addr &= 0xffff;

  val =  b[addr+1];
  val =  val << 8;
  val |= b[addr];
  return val;

 }

/*************************************
Multiple scale generic get value.
scale  1=byte, 2=word, 3=long. unsigned
       5=byte, 6=word, 7=long. signed
Auto negates via bit mask if required
***********************************/

int g_val (int addr, int size)
{                                           // addr is  encoded bank | addr
  int val, mask;                            // scale is -ve for no sign
  uchar *b;

  b = map_addr(addr);

  if (!b)  return 0;                        // map to required bank

  addr &= 0xffff;

  val = 0;
  mask = 0;

    switch(size)
     {

      case 5:                    //signed byte
        mask = 0x80;
      case 1 :                  // uns byte

       val = b[addr];
       break;

      case 6:                  // word
        mask = 0x8000;
      case 2:
        val =  b[addr+1];
        val =  val << 8;
        val |= b[addr];
        break;


      case 7:                    //int, or long in 806x terms
      //  mask = 0x8000;        // not reqd !!
      case 3:
        val =  b[addr+3];
        val =  val << 8;
        val |= b[addr+2];
        val =  val << 8;

        val |= b[addr+1];
        val =  val << 8;
        val |= b[addr];
        break;

      default:
        val = 0;
        mask = 0;
  //      if (size) DBGPRTN ("g_val inv size %d", size);       // allow size 0
        break;
    }

   if (mask & val) val |= ~(mask - 1);

  return val;

}


int negval (int val, int mask)
{  // negate value according to sign bit position - used for some jumps

  if (mask & val) val |= ~(mask - 1);        // negate if sign bit set
  return val;
}


/*
int find_wtram(int addr)
 {
  int i,ans;

  // does register exists in list ?
  // i.e. was it recently updated ?
  // for signed funcs and tabs.

  ans = 0;        // not found
  for (i = 0; i < NPARS; i++)
     if ((lram[i] & 0x1fff) == addr) 
       { 
        if (lram[i] & 0x8000) ans = 2;
        else ans = 1;      // found 
        break;
       }
return ans;
 
 }

*/









void upd_wtram(int addr, int size)
 {
  int i,j;

  // straight memory move down list,
  // with check for duplicate.
  // [0] = newest entry

  j = NPARS-1;           // move 15 entries if not found

  for (i = 0; i < NPARS-1; i++)
     if ((lram[i] & 0x1fff) == addr) 
       {           // no point checking last entry....
        j = i; 
        break;
       }

    memmove (lram+1, lram, j * sizeof(ushort));
    lram[0] = addr;
    lram[0] |= (size << 14);
 }


void upd_wtlist(int add, short size)
{ 

  if (add <= minwreg || add >= PCORG) return;        // only keep from general regs to PCORG

  size &=3;                                          // no longs (yet)
  upd_wtram(add, size);

}


RBT *add_rbase(int reg, int add)
{
  RBT *x;     // add includes bank

  int ans,ix;

  if (reg & 1) return 0;            // can't have odd ones
  if (reg <= stkreg1) return 0;   // can't have special regs or stack
  if (add <= maxwreg) return 0;   // can't have register to register adds

  if (add < 0xffff && add > PCORG) add |= opbank(datbank);      // safety check for imd values

  if (!val_data_addr(add)) return 0;
 
  x = (RBT *) chmem(&basech);
  x->radd = reg;
  x->rval = add;
 
  ix = bfindix(&basech, x);
  ans = cpbase(ix, x);

  if (ans)
   {
 
    chinsert(&basech, ix, x);
  //  DBGPRTN("add rbase %x = %x", reg, add);
    return x;
   }

  return (RBT *) basech.ptrs[ix];

}



int upd_ram(int add, uint val, int size)
 {

  add &= 0xffff;
  if (add <= 0 )  return 0;

   switch(size)
    {

     case 1 :
     case 5 :
        if (add > PCORG) return 0;

        ibuf[add] = (val & 0xff);
        break;

     case 6:
     case 2:
        if (add > (PCORG-1)) return 0;

        ibuf[add] = val & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        break;

     case 7:
     case 3:
        if (add > (PCORG-3)) return 0;
        ibuf[add] = val & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        ibuf[++add] = (val >> 8) & 0xff;
        break;

     default:
//       DBGPRTN ("Upd_reg inv size %d", size);
       return 0;
    }
   return 1;        // updated OK
 }


void upd_watch(SBK *s)         // s is not used
 {
  OPS *o;
  int w ;
  RBT *b;
  DDT *d;

if (anlpass >= ANLPRT) return;       // don't do in print phase !!

  w = opctbl[curinst->opcix].wop;
  o = curinst->opr + w;                    // destination entry

  if (o->rbs && o->rbs->cmd)
     {
 //     DBGPRTN ("Rbase bans update %x", o->addr);
      return;
     }


   if (w)
     {
      if (curinst->imd) b = add_rbase(o->addr,o->val);
      else 
       {
          b = (RBT*) schmem();                // get temp block 
          b->radd = o->addr;
          b = (RBT*) find(&basech, b);

          if (b && !b->cmd)
           {
            if (b->cnt < 15) b->cnt++;             // extra trick !!
            if (b->rval != o->val && !b->inv)
             {
               b->inv = 1;      // not same value
             //  DBGPRTN ("Inv rbase %x", b->radd);
             }  
            if (!curinst->imd)
               {
                b->inv = 1;    // not immediate value - this seems to work
   //             DBGPRTN ("RInv rbase %x", b->radd);
               }   
           }
       }

     if (upd_ram(o->addr,o->val,o->wsize))  upd_wtlist(o->addr,o->wsize);

  //   if (anlpass < ANLPRT) DBGPRTN("%x RAM %x = %x (%d)",s->curaddr,  o->addr, o->val, o->wsize);

    }

// check for autoinc (can be on the read register....always [1])


  o = curinst->opr+1;

  if (w && curinst->inc)
    {           // autoinc set - update value, check for subr params
     
     //check for DDT here
       d = (DDT*) chmem(&datch);
       d->start = o->addr;
       d->from = curinst->ofst;
        
       d = (DDT*) find(&datch,d);
       if (d) d->inc =1; 
 
    } 

//   wtcnt++;

 // return o->addr;
}



//------------------------------------------------------------------------

void set_opstart (int ofst)
{
  short bit;
  uint *z;
  BANK *x;
  int bank;

  bank = (ofst >> 16) & 0xf;
  ofst &= 0xffff;

  x = bkmap+bank;
  z = x->opbt;
  ofst -= PCORG;
  bit = ofst & 0x1F;       // 32 bits
  ofst = ofst / 32;
  z[ofst] |= (1 << bit);
}

int get_opstart (int ofst)
{
  short bit;
  int bank;
  BANK *x;
  uint *z;

  bank = ofst >> 16;
  x = bkmap+bank;
  z = x->opbt;
  ofst &= 0xffff;
  ofst -= PCORG;
  bit = ofst & 0x1F;
  ofst = ofst / 32;

  return z[ofst] & (1 << bit);
}

int get_prev_opstart (int ofst)

{
  short bit;
  int bank,xofst;
  BANK *b;
  uint *z;

  bank = (ofst >> 16) & 0xf;
  b = bkmap + bank;
  z = b->opbt;
  xofst = ofst & 0xffff;
  xofst -= PCORG;

  while (xofst-- > 0)
   {
    bit = xofst & 0x1F;
    bank = xofst / 32;
    if (z[bank] & (1 << bit)) break;
   }

  xofst += PCORG;
  xofst |= (ofst & 0xf0000);  // bank
  return xofst;

}


int outside (int addr, int low, int high)
 {
   if (addr < low)  return 1;
   if (addr > high) return 1;

   return 0;
 }

void do_jumps (void)
{

/******* jumps checklist ***************
 set conditions flags for various things
 whether jumps overlap, multiple end points etc etc.
*****************************************/

 int ix, zx, start, end;     // have to use indexes here as interleaved scan

 JLT *f, *t, *l;

 for (ix = 0; ix < jpfch.num; ix++)
    {
     f = (JLT*) jpfch.ptrs[ix];                // down full list of forward jumps

     show_prog (anlpass);

     if (f->to == f->from)
       {
        if (f->type == 0) f->retn = 1;            // return
        else f->lstp = 1;                         // loopstop
        continue;                                 // no more to do
        }

     if (f->type != 3) continue;                 // CHECK -type 2 jumps can still be relevant ?

     if (f->back)
      {
       start = f->to;
       end   = f->from;
      }
     else
      {
       start = f->from;
       end   = f->to;
      }

     // do any jumps IN, from destination of jump in f

     zx = bfindix(&jptch,f);
     t = (JLT*) jptch.ptrs[zx]; 

     l = 0;

     while (t->to < end && zx < jptch.num)
      {
       if (f != t && t->type && t->type < 4)
       {

       if (outside(t->from, start, end)) {
         if (t->to > start)   f->uci = 1; }    // jumps outside in to same start is OK
       }
       zx++;

       l = t;                        // keep last;
       t = (JLT*) jptch.ptrs[zx];
       if (l!=f && l->to == f->to) {f->mult = 1; l->mult = 1;}   // multiple 'to'
      }

      // check jumps OUT from inside this jump

     zx = ix+1;
     t = (JLT*) jpfch.ptrs[zx];
     while (t->from < end && zx < jpfch.num)
      {
       if (f != t && t->type && t->type < 4)
         {
          if (t->type == 2 && t->from+t->cnt == end && t->from > f->from+f->cnt)
           { // t last statement skipped from f, but not straight after f 
            t->elif = 1;  // check for ELSE here....
           // t->cbkt = 1;    // ?
           }
          else
          if (outside(t->to, start, end)) f->uco = 1;      // jumps inside to out
         }
       zx++;
       t = (JLT*) jpfch.ptrs[zx];
      }


      if (!f->back && !f->uci && !f->uco)    // ucio ?
        {
        f->cbkt = 1;  	// conditional, forwards, clean.  Ok to use   seems to work here
        }
    }

}

/*
void DBG_CSIG(CSIG *s)
 {                // prt single csig
  if (!s) return;
  wnprt(s->sig->ptn->name);
  if (s->disb) wnprt(" !");
  wnprt(" L1=%d", s->lev1);
  if (s->l1d) wnprt("*");
  wnprt(" L2=%d", s->lev2);
  if (s->l2d) wnprt("*");
  wnprt(" P=%x",  s->par1);
  wnprt(" D=%x",  s->dat1);
  wnprt(" A=%x",s->caddr);
 }
*/



void prt_csig_list(CSIG *s)
 {                   // special funcs for subrs - COMMAND output
  short levs;        // safety count


// check for manual sig in here....for the F: option, and print it as a CADT fake....

  levs = 0;
  if (!s) return;

  while (s && levs < MAXLEVS)
   {
    //  wnprt(" :");
    //DBG_CSIG(s);
    // just print the 'special sig, if there is one.....


if (s->sig->ptn == &tblp || s->sig->ptn == &tblx) 
   {
     wnprt(" : F 2");
     wnprt (" %x %x", s->sig->v[4],s->sig->v[3]);
     if (s->sig->v[2] < 2) wnprt (" Y");  else wnprt(" W");
   }  

if (s->sig->ptn == &fnlp) 
   {
     wnprt(" : F 1");
     wnprt (" %x", s->sig->v[4]);
     if (s->sig->v[2] == 2) wnprt (" Y");  else wnprt(" W");
     if ((s->dat1 & 3) == 1) wnprt (" U");  else wnprt(" S");
     if ((s->dat1 &0x300) == 0x100) wnprt (" U");  else wnprt(" S");
 
   }

    if (s == s->next) {
       // DBGPRT(" __LOOP__");
        break; }         // safety
    s = s->next;
    levs++;
   }
 }


int dsize(CADT *s)
{            // size of a single CADT entry
 if (!s) return 0;
 if (s->disb) return 0;        // disabled entry
 return s->cnt * casz[s->ssize];
}


void prt_cadt(CADT *s)
{                  // additional params in LBK (commands) or SXT  (subs)

ushort flg, levs;

levs = 0;

while (s && levs < MAXLEVS)
  {
   if (!s->nodis && !s->disb)
   {
   if (s->split) wnprt("| "); else wnprt(": ");
   if (s->ssize > 4) wnprt("S ");
   flg = s->ssize & 3;

    switch (flg)
     {
      case 1:
        wnprt("Y ");
        break;
      case 2:
        if (s->enc) wnprt("E %d %x ", s->enc, s->addr);
        else        wnprt("W ");
        break;
      case 3:
        wnprt("L ");
        break;
      default:
        break;
        
     }
    

  // only print fieldwidth if it's not standard and specified

   if (s->pfw)
    {
      if (s->pfw != cafw[s->ssize])  wnprt("P %x ",s->pfw);
    }

   if (s->dcm) wnprt("X ");
   if (s->cnt > 1) wnprt("O %d ", s->cnt);
   if (s->foff) wnprt("D %x ", s->addr);
   if (s->divd) wflprt("V ", s->divd);
   if (s->vect) wnprt("R ");
   if (s->name) wnprt("N ");
 //  if (s->disb) wnprt("! ");
  // DBGPRT ("[A%x D%x] ", s->addr, s->dreg);
 //  }
   
   if (s == s->next)
      {
     //   DBGPRT("__LOOP__");
        return;
      }        // safety
   
}
   s = s->next;
   levs++;
  }
}

void prt_bnks(void)
{
int i;
BANK *x;
for (i = 0; i < BMAX; i++)
 {
  x = bkmap+i;

  if (x->bok)
    {
     wnprt("bank %d  %5x  %5x", i, x->filstrt, x->filend);
  //   DBGPRT("  # %5x fill %5x-%5x, type %d",x->maxpc,x->minpc,x->maxbk,x->btype);
     wnprtn(0);
    }
 }
}

void prtflgs(int flgs)
{

 int fl;
 short i;

 fl = flgs;

 for (i = 0; i < 26; i++)
   {
   if (!fl) break;
   if (fl & 1) wnprt("%c ", i+'A');
   fl >>=1;
  }

 }

void prt_opts(void)
{
// print opt flags and banks
 wnprt("opts   :" );
 prtflgs(cmdopts);
 wnprtn(0);
}

/*
void DBG_scanpars(SBK *z)
{
 int i;
 SPARS *k;
 
 k = z->pars;
 
 if (!k) return;
 
 for (i = 0; i < NPARS; i++)
   {
     if (k->preg[i]) 
         {
             wnprt(" %d|", k->preg[i] & 0x8000 ? 2 : 1); 
             wnprt("%x=%x",k->preg[i] & 0x1fff, k->pval[i]);
         }
    
   }
 wnprtn(0);
}*/

/*
void DBG_sbk(SBK *s)
{
 DBGPRT("sc %5x ", s->start);
 DBGPRT("end %5x  ret %5x t%d ",s->end, s->retaddr, s->type);

 if (s->sub)  DBGPRT(" sub %5x", s->sub->addr);else DBGPRT("         ");
 if (s->caller) DBGPRT(" callst %5x r %5x", s->caller->start, s->caller->retaddr);
 if (s->caller && s->caller->sub) DBGPRT(" csub %5x", s->caller->sub->addr);
 if (s->cmd)    DBGPRT(" cmd");
 if (s->inv)     DBGPRT(" INV");
 else if (!s->scanned) DBGPRT(" NoSCAN");
 if (s->hblk)    DBGPRT(" HOLD to %5x", s->hblk->start);
 if (s->holder)  DBGPRT(" Holder");
 //    prt_pars(s);
 DBGPRTN(0);   
}


void DBG_scans(short dbgall)
{
  SBK *s;
  short ix;

 // wnprtn("# scans");

   for (ix = 0; ix < scanch.num; ix++)
    {
    s = (SBK*) scanch.ptrs[ix];

    if (s->cmd)
       {                // scan command for inclusion
        wnprtn("scan    %x", cmdaddr(s->start));
       }
    }

    if (!dbgall) return;

 //   DBGPRTN("# dbg scans num = %d    tscans = %d", scanch.num, tscans);

 //   for (ix = 0; ix < scanch.num; ix++)
 //     {
 //      prt_sbk((SBK*) scanch.ptrs[ix]);
 //   }
 wnprtn(0);
} */


SYT* get_sym(int rw, int add, int bit)
{
  SYT *s, *t;
  int a;

  if (bit > 7)
   {                // always do on a byte basis
    add++;
    bit -= 8;
   }
  
  a = add & 0xffff;
  if (a < PCORG)  add = a;     // fiddle for low syms

   if (!val_data_addr(add)) return 0;

  s = (SYT*) schmem();      // dummy block for search
  s->add = add;
  s->bitno = bit;
  if (rw) s->writ = 1;
  
  t = (SYT*) find(&symch,s);

  if (rw && !t)
    {
     s->writ = 0;
     t = (SYT*) find(&symch,s);       // check for read symbol
    } 

  if (t) t->used =1;               // for printout & debug

  return t;
}



char* get_sym_name(int rw, int pc)
 {
  SYT* x;    // whole 'read' symbols only

  x = get_sym(rw,pc,-1);
  if (x) return x->name;
  return NULL;
 }


void prt_cmds(CHAIN *z)
{                // sub command printer for both cmd and auxcmd
 short ix;
 LBK *c;
 SYT *x;

 for (ix = 0; ix < z->num; ix++)
    {
     c = (LBK*) z->ptrs[ix];
     wnprt("%-7s %x %x", dirs[c->fcom].com, cmdaddr(c->start), cmdaddr(c->end));

     if (dirs[c->fcom].namee)         // name allowed
      {
       x = get_sym(0,c->start,-1);
       if (x)
        {
         wnprt("  %c%s%c  ", '\"', x->name, '\"');
         x->prtd = 1;        // printed this symbol
        }
      }

     prt_cadt(c->adnl);

     if (c->cmd) wnprt("         # cmd");
     wnprtn(0);
    }
   wnprtn(0);
}



void prt_subs (void)
{
  SXT *s;
  SYT *x;
  SXDT *d;
  short ix;

  wnprtn("# ------------ Subroutine list");
//  DBGPRTN("# num subs = %d", subch.num);
//  DBGPRTN("# addr name [ psize cmd psp ]");

  for (ix = 0; ix < subch.num; ix++)
      {
       s = (SXT*) subch.ptrs[ix];
       d = s->sdat;       

       wnprt("sub %x ", cmdaddr(s->addr));
       x = get_sym(0,s->addr,-1);    
       if (x)
         {
          wnprt("%c%s%c  ", '\"', x->name, '\"');
          x->prtd = 1;        // printed sym, don't repeat
         }
       if (d)
         {       
           prt_cadt(d->cadt);
           prt_csig_list(d->sigc);
     //      if (s->nargs1) DBGPRT (" #sz %d", s->nargs1);
     //       if (s->nargs2) DBGPRT (" #sz2 %d", s->nargs2);
           if (s->cmd)     wnprt       ("            # by cmd");
         }
          wnprtn(0);  
      }
      
    wnprtn(0);  
  }


void prt_syms (void)
{
 SYT *t;
 short ix;
 int pc;

 wnprtn("# ------------ Symbol list ----");

 //DBGPRTN("# num syms = %d", symch.num);

 for (ix = 0; ix < symch.num; ix++)
   {
   t = (SYT*) symch.ptrs[ix];
   if (!t->prtd)     // not printed as subr or struct
     {
     wnprt("sym");

     pc = t->add;

     if (pc < PCORG || !numbanks)  pc &= 0xffff;

     wnprt(" %x ", pc);

     wnprt(" \"%s\"",t->name);

    if (t->bitno >= 0 || t->writ) wnprt(" :");  
    if (t->bitno >= 0) wnprt("T %d " , t->bitno);
    if (t->writ) wnprt("W");

  //  if (!t->cmd)  DBGPRT("    # auto-added");
  //  if (!t->used) DBGPRT("      # Not used");
  //   DBGPRT ("           ## A %x", t->add);

     wnprtn(0) ;
     }
   }

 }

/*
void DBG_ddt (void)
{
  DDT *r ;
  short ix;

  wnprtn("DDT num=%d", datch.num);
  for (ix = 0; ix < datch.num; ix++)
   {
    r = (DDT*) datch.ptrs[ix];

 //  if (!r->inv)
 //    {
    wnprt("ddt %x ", r->start);
    wnprt("sz %d ", r->ssize);
    wnprt("FROM %x", r->from);
    if (r->inx)  wnprt(" inx");
    if (r->inr)  wnprt(" inr");
    if (r->imd)  wnprt(" imd");
    if (r->inc)  wnprt(" inc %d", r->inc);       //not quite useful, YET.
    if (r->rbt)  wnprt(" rbs");
    if (r->vect) wnprt(" vec");
    if (r->psh)  wnprt(" psh");
    
    if (r->tab)
    {
      wnprt(" tab");
      wnprt(" C%d R%d" ,r->p1, r->p2);
    }
    else
    if (r->func)
      {
        wnprt(" func %d", r->p2);
      }
    else
     { 
      if (r->inx)
      {if (r->p1 > -32 && r->p1 < 32)    
      wnprt(" A %x V %d T%x" ,r->p2, r->p1, r->p3); 
      else
      wnprt(" A %x V %x t%x" ,r->p2, r->p1, r->p3); }
     }
  //    if (r->p1)  wnprt(" p1 = %x", r->p1) ;
  //    if (r->p2)  wnprt(" p2 = %x", r->p2) ;
     
//   wnprtP(r->inv, " INV");
 //   wnprtP(r->cmd, "  # CMD");
    wnprtn(0);
 //   }

   }
} */

void prt_rbt (void)
{
  RBT *r ;
  short ix;

  for (ix = 0; ix < basech.num; ix++)
   {
    r = (RBT*) basech.ptrs[ix];

   if (!r->inv)
     {
    wnprt("rbase %x %x" , r->radd , r->rval);
    if (r->cmd) wnprt("       # cmd");
    wnprt("\n");
    }

   }
}


void prt_dirs(int x)
{
// print all directives for possible re-use



//  DBGPRTN("# aux = %d, cmds = %d", auxch.num,cmdch.num);
  prt_opts();
  wnprtn(0);
  prt_bnks();
  wnprtn(0);
  prt_rbt();
  wnprtn(0);
//  prt_scans(0);
  wnprtn(0);
  prt_cmds(&cmdch);
  prt_cmds(&auxch);
  wnprtn(0);
  prt_subs();
  wnprtn(0);
  if (x) prt_syms(); 
}

/*

void DBG_jumps (CHAIN *x)
{
 JLT *j;
 short ix;

  DBGPRT("------------ Jump list \n");
  DBGPRT("num jumps = %d\n", x->num);

  for (ix = 0; ix < x->num; ix++)
    {
      j = (JLT*) x->ptrs[ix];

      DBGPRT("%5x (%5x)-> %5x t%d %s",j->from, j->from+j->cnt,j->to, j->type, jptxt[j->type]);
      if (j->bswp) DBGPRT(" bswp");
      if (j->back) DBGPRT(" back");
      if (j->uci)  DBGPRT(" uci");
      if (j->uco)  DBGPRT(" uco");
      if (j->retn) DBGPRT(" retn");
      if (j->mult) DBGPRT(" mult");
      if (j->lstp) DBGPRT(" lstp");
      if (j->elif) DBGPRT(" elif");
      
      if (j->cbkt) DBGPRT(" }");
      DBGPRT("\n");
    }
    DBGPRT("\n");
}
*/


SXDT *get_sxdt(SXT *s, int create)
{
    // get sxdt from subr, create if not exist
  SXDT *d;

  if (!s) return NULL;               // safety, no subr....

  d = s->sdat;

  if (!d && create)
    {
     d = (SXDT*) mem (0,0,sizeof(SXDT));
     memset(d,0, sizeof(SXDT));             //clear block
     s->sdat = d;
    }

  return d;
}

CSIG *get_csig(SXT *s)
{
  SXDT *d;

  if (!s) return NULL;
  d = s->sdat;
  if (!d) return NULL;
  return d->sigc;
}

CADT *get_cadt(SXT *s)
{
  SXDT *d;

  if (!s) return NULL;
  d = s->sdat;
  if (!d) return NULL;
  return d->cadt;
}


CADT* add_cadt (CADT **x)
  {
   // add cadt block to CADT chain at end

  CADT *ans, *z;

  ans = (CADT *) mem (0,0,sizeof(CADT));
  memset(ans,0,sizeof(CADT));                // clear block
  
  ans->cnt = 1;
  ans->ssize = 1;                            // single byte unsigned is default

  z = *x;                                    // chain ptr (can be zero)

  if (!z) *x = ans;      // new chain
  else
   {    
    while(z->next) z = z->next;   // end of chain
    z->next = ans;                // add to end
   }

  return ans;
}


CSIG *add_csig(SXT *s)           //, int forb)
 {
  CSIG *f, *x;
  SXDT *d;

  if (!s) return NULL;               // safety, no subr....

  d = get_sxdt(s,1);

  x = (CSIG*) mem (0,0,sizeof(CSIG));
  memset(x,0,sizeof(CSIG));            //new zero block

  if (!d->sigc)
   {
     d->sigc = x;            // front of chain
   }
  else
    {                 // end of chain
      f = d->sigc; 
      while(f->next) f = f->next;
      f->next = x;
    }

return x;
 }



// may not need both s and patn here ...

CSIG* find_csig(SXT *sub, SIG *s, PAT *patn, int addr)
{
 CSIG *f;                     // return block which matches or zero if no match
 int ans;
 SIG *z;

 ans = 0;

 if (!sub) return 0;

 f = get_csig(sub);

 while(f)
   {
    ans = 0;
    z = f->sig;
    if (!s    || s == z) ans ++;
    if (!addr || addr == f->caddr) ans++;
    if (!patn || patn == z->ptn) ans++;
    if (ans > 2) break;
    f = f->next;
   }

  return f;
 }

CSIG* find_csigp(SXT *s)
{
   // find any sig which is a param getter (for encodes and grnf)
   // used for func and tab too !! (and address match ?)

 CSIG *f;

 if (!s)  return 0;

 f = get_csig(s);

  while(f)
   {
    if (f->gpar && f->disb == 0 && f->par1 > 1)  break;
    f = f->next;
   }

  return f;
 }

CSIG* find_csige(CSIG *z, int reg)
{
   // find any sig which is an encoder

 CSIG *f;

 if (!z)  return 0;

 f = z->next;              //all csigs AFTER z

  while(f)
   {
    if (f->enc && f->disb == 0 && f->dat1 == reg)  break;
    f = f->next;
   }

  return f;
 }

void get_scanpars(SBK *s)
{
  int i,j;
  SPARS *k;

  if (!s->pars)
   {
    s->pars = (SPARS*) mem(0,0,sizeof(SPARS));     // space for addr and its value
   }
  
  k = s->pars;
  
  for (i = 0; i < NPARS; i++)
   {
    k->preg[i] = lram[i];
    j = lram[i] & 0x1fff;
    if (j) {k->pval[i] = ibuf[j];
    if (lram[i] & 0x8000) k->pval[i] |= (ibuf[j+1] << 8);}
   }
 
 //  DBGPRT("\nGET ");
 //  prt_scanpars(s);

}

void put_scanpars(SBK *s)     // restore saved values from hold
{
 int i,j;
 SPARS *k;
 
 k = s->pars;

 if (!k) return;
 
 for (i = 0; i < NPARS; i++)
   {
    if (k->preg[i])
       {
        j = k->preg[i] & 0x1fff;
        ibuf[j] = k->pval[i] & 0xff;
        if (k->preg[i] & 0x8000) ibuf[j+1] = (k->pval[i] >> 8);
       }
   }
//     wnprt("\nPUT ");
 //    prt_scanpars(s);
}















LBK *set_auxcmd (int start, int end, int com)
{
   LBK *b, *z;

   if (icaddrse(&start,&end)) return NULL;

   b = (LBK *) chmem(&auxch);
   b->start = start;
   b->end   = end;
   b->fcom  = com & 0x1f;    // max 31 as set
   if (com & 32) b->cmd = 1;

   z = inscmd(&auxch,b);
   DBGPBKN (b,"set auxcmd");
   if (z != b) return 0;                       // fail

   return z;
}

int check_xcode(int start)
{
  LBK n;                  // local block
  
  n.start = start;
  n.fcom  = C_XCODE;                  // search for xcodes
  if (find(&auxch,&n)) return 1;
  return 0;
}


LBK* set_cmd (int start, int end, int com)
 {          // this for user commands only, no called params
  LBK *n, *z;

  if (anlpass >= ANLPRT)    return NULL;
  if (icaddrse(&start,&end)) return NULL;

  n = (LBK *) chmem(&cmdch);
  n->start = start;
  n->end   = end;
  n->fcom  = com & 0x1f;    // max 31 as set
  if (com & C_CMD) n->cmd = 1;


  if (n->fcom == C_CODE && check_xcode(start))
     {
 //     DBGPRTN ("XCODE bans set_cmd %x", start);
      return NULL;
     }

  DBGPBK (n,"try cmd");
  z = inscmd(&cmdch,n);  
      
  if (z != n) return 0;                        // fail
  DBGPRTN ("OK!");   
  return n;   // successful
 }


/************************************************
 *   add symbol
 * addr       address to assoc. with name
 * bitno      bit number PLUS 1  (0 is no bits)
 * fnam       name to add
 ************************************************/

int do_symname (SYT *xsym, const char *fnam)
 {
  int z, s;

  if (!fnam || !*fnam)  return 0;
  z = strlen (fnam) + 1;  // include null, strlen does not...
  if (xsym->name)
  s = strlen (xsym->name) + 1;  // include null, strlen does not...
  else s = 0;
  
  if (z > 31) z = 31;    // max storage size

  xsym->name = (char *) mem (xsym->name,s,z);
  if (xsym->name)  strncpy (xsym->name, fnam,z);
  return z;

 }


SYT *new_sym (int rw, const char *fnam, int add, short bitno)
{
   // rw is 0 for read sym, 1 for write, |=2 for cmd set.
   // Changed this so that all bits are 0-7 and addressed by byte (-1 is full word)

  SYT *s;
  int ans,ix;

  if (anlpass >= ANLPRT) return 0;
  if (!fnam || !(*fnam)) return 0;              // safety

  if (bitno > 7)
   {
    add++;
    bitno -= 8;
   }

   s = (SYT *) chmem(&symch);
   s->add = add;
   s->bitno = bitno;
   s->writ = rw & 1;

  ix = bfindix(&symch, s);                //add, bitno);
  ans = cpsym(ix, s);            //add, bitno);   // zero if matches

  if (ans)
   {
 
     // if (bitno >=0) xsym->flags = 1;       // at least one flags bit.
     if ((rw & 2)) s->cmd = 1;
     if (do_symname (s, fnam)) chinsert(&symch, ix, s);
     
    // DBGPRT("add sym %6x %s",add,fnam);
    // if (bitno >= 0) DBGPRT (" B%d", bitno);
    // DBGPRT("\n");
    return s;
   }   // do insert

 return (SYT*) symch.ptrs[ix];

}


// add numbered auto sym and increment save value

SYT *add_nsym (uint ix, int addr)
{
  SYT *xsym;
  AUTON *n;
  int z;

  if (anlpass >= ANLPRT) return 0;
  if (ix >= NC(anames)) return NULL;    // safety check

  if (!(cmdopts & anames[ix].mask)) return NULL;    // autoname option not set

  xsym = get_sym(0,addr,-1);

  if (xsym)          // symbol found
   {
    if (xsym->cmd) return xsym;              // set by cmd
    if (xsym->anum) return xsym;       // already autonumbered
   }

  // now get symbol name

  n = anames + ix;

  z = sprintf(nm, "%s", n->pname);
  z += sprintf(nm+z, "%d", n->nval++);

   // name now set in nm

  if (xsym) do_symname (xsym, nm);   //update name
  else xsym = new_sym(0,nm,addr,-1);     // add new (read) sym

  if (!xsym)
   {
    if (anames[ix].nval > 1) anames[ix].nval--;
   }
  else
   {
    xsym->anum = 1;                 // remember autonumber
   }
  return xsym;
}


void do_subnames(void)
{
  // add names as required - do at end of dissas from
  // subr chain

 int ix;
 SXT  *sub;
 SYT  *z;

 for (ix = 0; ix < subch.num; ix++)
    {
     sub = (SXT*) subch.ptrs[ix];

     if (sub->cmd)  continue;       // can't change if via command
     z = (SYT*) schmem(); 
     z->add = sub->addr;
     z->bitno = -1;

     z = (SYT*) find(&symch,z);
     if (z)         continue;                // check if sym exists alreaady....

     if (sub->xnam && !sub->cname) add_nsym(sub->xnam,sub->addr);             // add subr name acc. to type
    }
 //  DBGPRTN("end dosubnames");
 }


void set_size(SXT *s)
{
 CADT *x;

 if (!s) return;
 s->nargs1 = 0;

 x = get_cadt(s);

 while(x)
   {
    s->nargs1 += dsize(x);
    x = x->next;
   }

  // DBGPRTN("set size = %d for %x",s->nargs1, s->addr);
}

void fix_scans(SXT* sub)
 {
   // after a subr added, check if any jumps to that subr, if so, mark
   // scans to be redone, so that any subr special funcs get done

   // still doesn't work for A9l - callers TO the subr which jumps ?

   // BUT what about everything which then calls the fixed up jump-to-subr ?
  //     must also rescan callers of rescan block ?


   int ix;
   SBK  *s, *x;

   x = (SBK*) schmem();
   x-> start = sub->addr; 

   ix = bfindix(&scanch, x);            // any scans for addr ?
   if (ix >= scanch.num) return;        // none found

   while (ix < scanch.num)
    {
      s = (SBK*) scanch.ptrs[ix];
      if (s->start != sub->addr) break;      // not found

    //  if (s->type == 2)
        {

         s = s->caller;        // go to caller (of this block)
         if (s && s->scanned)
          {
           s->scanned = 0;     // rescan caller
  //         DBGPRT("Set rescan");
           if (s->inv)
              {
               s->inv = 0;
   //            DBGPRT(" and INV");
              }
   //        DBGPRTN(" %x (sub %x)", s->start,sub->addr);  // rescan caller");
          }
         // and do one level up, too - perhaps more....


        }
      ix++;
       }


   //if (n->start == s->start && n->retaddr == s->retaddr) return 0;

   //return 1;
 }



void cpy_sub_csigs (SXT *tsub, SXT* ssub)        //, SBK* blk)
  {
   // copy subr signatures tsub = ssub only copies POINTERS
   // need blk to process sigs - NO LEVEL CHANGES

  CSIG *s, *n;

  int c;              //changed, front/back ?

  if (tsub->cmd) return;
  if (!tsub || !ssub || tsub == ssub) return;
             // don't update if set by command

 // DBGPRTN("Copy sub csigs %x to %x", ssub->addr, tsub->addr);

  // now add any attrs not found (cadt and sig)

   c = 0;
   s = get_csig(ssub);

   while (s)
    {
     if (!find_csig(tsub,s->sig,0,0))
        {
  //       DBGPRTN("csig %s", s->sig->ptn->name);
         if (s->gpar && !s->lev1)
          {
  //           DBGPRT("SL=0!!");       // if lev=0 then copy the related CADT ?
             //n->lev1 = 1;
          }
         else
          {
            n = add_csig(tsub);
           *n = *s;              // copy sig to new blk
            n->next = 0;          // clear next ptr
            n->l1d = 0;               // need to redo it ?
            c++;
          }
        }
     s = s->next;
    }

   if(c)
     {
      fix_scans(tsub);
      }
 }


CSIG* init_csig(SIG *x, SBK *b)
{
   CSIG *c;

   c = add_csig(b->sub);               // sub from scan block
   c->sig = x;                         // this sig add to chain
   c->caddr = b->curaddr;              // and current addr  = curinst->ofst
 //  DBGPRT (" Sub %x init csig ", b->sub->addr);
   //   PRT_CSIG(c);
   // DBGPRTN(0);  
     
 return c;
}


void nldf (SIG *x, SBK *b)
{
  // default preprocess - no levels
  // CSIG *c;
  
   if (!b) return;
   
//   c =
 init_csig(x,b); 
//   PRT_CSIG(c);
//   DBGPRTN(0);
}

void plvdf (SIG *x, SBK *b)
{
  // default preprocess - 'pop' type levels
   CSIG *c;
  
   if (!b) return;
   
   c = init_csig(x,b); 
         
   c->lev1 = x->v[1];
   
   c->lev2 = x->v[2];
   c->lev1 += c->lev2;

//   PRT_CSIG(c);
 //  DBGPRTN(0);
}



void process_sig(SIG *x, SBK *b)
 {
  // does this check AFTER the relevant do-code
  // so that curinst, codebank, etc are sorted
  // assumes only one sig can exist for one ofst
 
  SXT *s;
  CSIG *c;
  PAT *p;

  s = b->sub;
  p = x->ptn;

  if (s && s->cmd && !p->spf)
     {
 //     DBGPRTN("Ignore sig %s subr %x cmd",x->ptn->name, s->addr);
      return;      // subr specified by command, no overrides (EXCEPT an F!)
     }

  if (!s)
     {          // no sub, set zero  scanblk
      p->sig_prep(x,0);
      return;
     }

   // correct that sigs are chained at the back (i.e. in order),

     c = find_csig(s,x,0,0);

     if (!c)                            // not a duplicate....
         {
 //         DBGPRT ("(PRCS) ");
          p->sig_prep(x,b);     // add csig to sub and preprocess it
   //       DBGPRT ("%x (PRC) ADD csig to sub %x ",b->curaddr, s->addr);
  //        DBGPRTN(0);
          fix_scans(s);            // rescan any blocks for this addr
         }
    }

/*
void do_data_pat(int ix)
{
  int i, j, start, addr,gap, val;
  int score [32];                  // = max gap
  int mtype [32];                  // temp for now whilst testing
  uchar vl  [32];               // value difference
  uchar a, b;
  // uint c,d; ? or ushort ?
  LBK *s; //, *n;
  s = (LBK*) cmdch.ptrs[ix];
//  n = (LBK*) cmdch.ptrs[ix+1];


// data struct may scan beyond end.....

if (s->fcom == C_CODE) start = s->end+1;
else start = s->start;

DBGPRTN ("start gap data analysis at %x", start);

 for (gap =1 ; gap < 32; gap++)
 {
   score[gap] = 0;
   for (i = 0; i < gap; i++)       // establish base row of 'gap' elements
     {
      addr = start+i;
      a = g_byte(addr);
      b = g_byte(addr+gap);
      vl[i] = a-b;
      mtype[i] = 0;
                             // compile value difference.
      if (a == b)  mtype[i] |= M;         // match
      else if (__builtin_popcount(a) == 1 && __builtin_popcount(b) == 1) mtype[i] = B;  // bit mask ?
      if (a < b )  mtype [i] |= I;        // increment
      if (a > b )  mtype [i] |= D;        // decrement
 //     j = __builtin_popcount(a);          // no of bits
     }

// now compare niew rows with 'base' row

   for (j = start+gap; j < (start+127)-gap; j+=gap)       // row by row until 128 max
     {
         for (i = 0; i < gap; i++)       // compare this row to the 'base'
          {
           addr = j+i;
           a = g_byte(addr);
           b = g_byte(addr+gap);
           val = a-b;                              // value difference.
           if (val == vl[i])  score[gap] +=3;

           if (mtype[i] & M)
           {
             if (a == b)  score[gap] += 2; else score[gap]--;        // match
           }
           if (mtype[i] & I)
           {
           if (a < b)  score[gap] += 2; else score[gap]--;       // decrement
           }
           if (mtype[i] & D)
           {
           if (a > b)  score[gap] += 2; else score[gap]--;       // increment
           }
           if (mtype[i] & B)
           {
            if (a != b && __builtin_popcount(a) == 1 && __builtin_popcount(b) == 1 ) score[gap] +=2;
           }

           }
     }

DBGPRTN("gap %d, score %d", gap, score[gap]);

 }

 addr = 0;                         // use for highest score
 gap = 0;                          // where it is
 for (i=1 ; i < 32; i++)
 {
   if (score[i] > addr)
   { addr = score[i];
      gap = i;
   }
 }

DBGPRTN("end analysis at %x", s->end+1);
DBGPRTN("highest score is %d for gap %d", addr, gap);

}
*/













void code_scans (void)
 {

  // turn scan list into code, if scans complete and valid
  // needs a DONE flag if called more then once....
  int ix;
  SBK *s;
 // LBK *k;          //, *n;

  for (ix = 0; ix < scanch.num; ix++)
   {
    s = (SBK*) scanch.ptrs[ix];
    if (s->inv)
       {
  //      DBGPRTN("INV scan %x - %x",s->start, s->end);
   //     pstr("\n ** this block may be invalid **\n")
        //continue;                // ignore invalid blocks !!
       }

    if (s->scanned && s->end >= s->start)  // 2nd check redundant ?
     {
         
         
      //k = (LBK*) find(&cmdch,s->start, C_CODE);
      //if (!k || s->start < k->start || s->end > k->end)
      //k = 
      set_cmd(s->start,s->end,C_CODE);
     }
   }
 }


int unhold_scans (SBK *blk)             //old one
 {
  SBK *s;
  int ix, ans;

  ans = scanch.num;                   //lowest index unheld
  
  // NB  this searches WHOLE CHAIN !!

  if (!blk->holder) return 0;     // was holds flag

  for (ix = 0; ix < scanch.num; ix++)
   {
    s = (SBK*) scanch.ptrs[ix];
    if (s->hblk == blk)
    {
 //     DBGPRT("UNHOLD %x cur %x h %d t %d", s->start, s->curaddr, s->htype, s->type);
 //     if (blk->sub) DBGPRT(" csub %x", blk->sub->addr);
      s->hblk = 0;

      // Do NOT update params here !!!
 //     DBGPRTN(0);
      if (ix < ans) ans = ix;             // keep track of lowest ix
    }
  blk->holder = 0;   // holds against this block now clear
//  if (holdsp > 0) holdsp--;
 }
 return ans;        // lowest index updated
}

 
 
 
 
 
 
 
 

void end_scan (SBK *s, int end)         // CANNOT use ix (adding scan changes ix)
{
  // allow zero end value
  if (anlpass >= ANLPRT || PMANL ) return;



//  if (s->hblk) DBGPRT("HOLD "); 
//  if (s->inv)  DBGPRT (" Invalid ");

  if (!s->hblk && end) s->end = end;
   
//  DBGPRTN("END scan t=%d, start %x, end %x ",s->type, s->start, s->end);
 
  if (!s->hblk)
    {
      s->stop = 1;
      if (s->end >= PCORG && s->end <= maxadd(s->end) && s->end >= s->start)
        {
         s->scanned = 1;
        }
    }
}





SXT* add_subr (int addr)
{
  // type is 1 add name, 0 no name

  int ans, ix;
  SXT *x;

  if (anlpass >= ANLPRT) return NULL;
  if (!val_code_addr(addr)) return NULL;

  if (check_xcode(addr))
     {
//      DBGPRTN("XCODE bans add_subr %x", addr);
      return NULL;
     }

  ans = 1;

// insert into subroutine chain
  x = (SXT *) chmem(&subch);
  x->addr = addr;

  ix = bfindix(&subch, x);
  ans = cpsub (ix,x);

  if (ans)
   {

    x->xnam = 1;                   //default subname ('subnnn')
    chinsert(&subch, ix, x);       // do insert

    //  DBGPRT("Add sub %x\n",  addr);

    if (g_byte(addr) == 0xf2) {x->psp = 1; addr++;}   // subr begins with pushflags

    return x;
   }

   // match - do not insert, free block. Duplicate.
  //   DBGPRT("Dup sub %x\n",addr);
  // return 0;
  return (SXT*) subch.ptrs[ix];

  }


// only sj_subr uses the answer from add_scan..and sets caller.


SBK *add_scan (int add, int type, SBK *caller)
{

 /* add code scan.  Will autocreate a subr for scans > 3 so that sub ptr set, and comparisons work.
  * types 0-15. +16 indicates a command.....+32 for return zero if not added (for vects)
  * 
  * scan type  =
   1 init jump    -- is a static jump without a hold
   2 static jump
   3 cond jump
   4 int subr
   5 vect subr
   6 called subr
   7 converted jmp to subr
 */

  SBK *s;             //, *t;
  int ans, ix;

  if (anlpass >= ANLPRT)   return 0;

// TEMP add a safety barrier ...
if (scanch.num > 12000) return 0;

  if (!val_code_addr(add))
    {
 //    if (val_jmp_addr(add)) DBGPRT ("Ignore scan %x", add);
 //    else        DBGPRT ("Invalid scan %x", add);
 //    if (caller) DBGPRT (" from %x", caller->curaddr);
 //    DBGPRTN(0);
     return 0;
    }

  if (check_xcode(add))
     {
  //    DBGPRT("XCODE bans scan %x", add);
   //   if (caller) DBGPRT (" from %x", caller->curaddr);
  //    DBGPRTN(0);
      return 0;
     }

  if (PMANL)
      {
  //     DBGPRTN("no scan %x, manual mode", add);
       return 0;
      }

  s = (SBK *) chmem(&scanch);             // new block
  s->start = add;
  s->end = maxadd(add);                   // test ?
  s->curaddr = s->start;                  // important !! 
  if (type & C_CMD) s->cmd = 1;           // added by user command
//  if (type & C_TST) s->tst = 1;         // keep test marker                 

  s->type = type & 0x1f;                  // scan type 0-31

  // retaddr is actually the START of calling opcode, need to add cnt to get true return addr
  // need to keep it, as caller->curaddr may CHANGE, so this is record of calling address
  // this means HOLD resumes at the start of the call or jump to redo subr params (if reqd)
  
  // xdt2 shows we must keep subr for sigs.....including conditionals

  if (caller)
   {
    s->caller   = caller;
    s->retaddr  = caller->curaddr;                      // return addr for tracking
    s->sub = caller->sub;
    if (s->retaddr == s->start) return 0;               // a loopstop jump
 //   s->test = caller->test;                             // keep test marker
   }

   if (s->type > 3)   s->sub = add_subr(s->start);           // returns 'sub' even if a duplicate

   ix = bfindix(&scanch, s); 
   ans = cpscan(ix,s);

   if (!ans) return (SBK*) scanch.ptrs[ix];                 // duplicate scan

   chinsert(&scanch, ix, s);            // do insert
//   DBGPRT ("Add Scan %x t=%d", s->start, s->type);
//   if (s->type > 3) DBGPRT(" +sub");
//   if (s->cmd)  DBGPRT(" +cmd");
  // if (s->test) DBGPRT(" +tst");

   if (caller) 
    {
     get_scanpars(s);           //only if NEW scanblk
  //   if (caller->sub) DBGPRT(" sub %x", caller->sub->addr);
//     DBGPRT (" from %x", s->retaddr);
    }
// DBGPRTN(0);
 return s;
}

/*
void scan_all (void);

/
void scan_gaps(void)
{
  int ix, i, ofst;
  LBK *t, *n, *l;    // this, next, last

  for (ix = 0; ix < cmdch.num-1; ix++)
   {
    if (ix <= 0) l = 0; else l = (LBK*) cmdch.ptrs[ix-1];
    
    t = (LBK*) cmdch.ptrs[ix];
    i = ix+1;
    n = (LBK*) cmdch.ptrs[i];

    if ((t->start & 0xffff) <= 0x201e) continue;

    // check for filler (can be anywhere !!)
    ofst = t->end+1;
    while (ofst < n->start)
     {  
      if (g_byte(ofst) != 0xff)  break;
      ofst++;
     } 

     if (ofst < n->start)
      {     
       DBGPRTN("GAP detected %x->%x type %s and %s", t->end+1, n->start-1, cstr[t->fcom], cstr[n->fcom]);
       
       if (l && l->end == t->start-1 && t->fcom == C_WORD && n->fcom == C_CODE && l->fcom == C_CODE &&
           t->end - t->start == 1)
             {
                 DBGPRT("*A ");
              add_scan(t->start,S_SJMP|C_TST,0);       // add test marker
              scan_all(); 
             }
                  
       if (t->fcom == C_CODE && n->fcom == C_CODE)
          {
            DBGPRT("*B ");
            add_scan(t->end+1,S_SJMP|C_TST,0);       // add test marker
            scan_all(); 
          }
     // or if only one word right at front (a push...) and then code on both sides, this also for test scan. 


      }
  //  }
   }
 // justprint at the moment

} */




int decode_addr(CADT *a, int ofst)
  {
  int rb,val,off;
  RBT *r;

  val = g_val (ofst, a->ssize);   // offsets may be SIGNED
  rb  = val;

  if (a->foff)
    {            // fixed offset (as address)
     val += a->addr;
     val &= 0xFFFF;         // bank ?
     return val;
    }

  switch (a->enc)
     {
      case 1:           // only if top bit set
      case 3:
        if (! (rb & 0x8000)) return val;
        off = val & 0xfff;
        rb >>= 12;            // top nibble
        if (a->enc == 3) rb &= 7;
        rb *=2;
        break;

      case 2:       // Always decode
        off = val & 0xfff;
        rb >>= 12;     // top nibble
        rb &= 0xf;
        break;

      case 4:
        off = val & 0x1fff;
        rb >>= 12;     // top nibble
        rb &= 0xe;
        break;

      default:
        break;
     }

  if (a->enc)
   {
    r = (RBT*) schmem();
    r->radd = a->addr & 0xff;
    r->radd += rb;   
    r = (RBT*) find(&basech, r);

    if (r)    val = off + r->rval;        // reset op
//    else
 //   DBGPRTN("No rbase for address decode! %x (at %x)", a->addr+rb, ofst);
   }
   
  if (a->vect)
  {    // pointer to subr - may do this elsewhere too... 
   val |= (ofst & 0xf0000);   // add bank from start addr....not necessarily right
  } 
  return val;

}

void chg_subpars (SXT* sub, int reg, int size)
{
 // or merge reg and size ?
 //subr (bytes to words)  here and other places too. (reg, ssize) ?

 int tsz, ls;   // total size, local size
 CADT *a;

 if (sub && anlpass < ANLPRT) a = get_cadt(sub);
 else  a = 0;
 
 //DBGPRT("In chg_subpars sub %x  p%x to %d", sub->addr,reg, size);

 while (a)
  {
   ls = casz[a->ssize];   // saves some lookups
   if (a->dreg == reg && ls != size)
     {
      tsz = ls * a->cnt;      // size of entry
      a->ssize = size;
      a->cnt = tsz/ls;         // reset count
   //   DBGPRT (" changed ");
      break;                             // found
     }
   a = a->next;
  }

// DBGPRTN(" exit chg");
}



int get_subpars(SXT* sub, SBK* blk, int pno)
{
  // pno is which param gets returned in answer, used for tab and funcs
  // or is register with param in it (marked by flag) 
  // This gets whole chain at the moment.....put break back after debug.
  // blk is caller

 int i,n, size, reg, addr, val, ans;
 CADT *a;


 if (!sub) return 0;
 if (anlpass >= ANLPRT) return 0;

 a = get_cadt(sub);
 if (!a) return 0;   // no additional
 
 addr = blk->nextaddr;

// DBGPRT("In get_subpars sub %x at %x", sub->addr,addr);

 reg = 0;
 
 if (pno & 0x100)
   {
     reg = pno & 0xff;
     pno = 0;
//     DBGPRTN(" Reg %x", reg);
   }
// else
 //  DBGPRTN(" pno %d",  pno);  
 
 n   = 1;
 ans = 0;

 while (a)
  {
   for (i = 0; i < a->cnt; i++)
     {
       size = casz[a->ssize];              // TEST set up data correctly...
       val = decode_addr(a, addr);

       if (val_data_addr(val))
        {
          add_ddt(val,size, curinst->ofst);            //ddt is there, it's the cmd merge....
        }
   
       if (pno && n == pno)  {
           ans = val;
 //DBGPRT("*"); 
 }
       
       if (reg && a->dreg == reg) {
           ans = val; 
       //    DBGPRT("*");
           }
   
 //      DBGPRT ("P%d = %x, ",n, val);
       n++;
       addr += size;
     }

  a = a->next;
}
// DBGPRTN(0);
 return ans;
}


/*
void DBGPSUB(SXT *csub, SBK *blk)
{
  DBGPRT("Resume at %x ", blk->nextaddr + csub->nargs1);
  if (csub->cmd) DBGPRT("(cmd) ");
  if (csub->nargs1 > 0) DBGPRT("%d numargs ",csub->nargs1);
}

*/

CSIG* cpy_csig_levels(CSIG *z, SBK *cblk)
{
// copy any new sigs to calling subr, if addr matches and levels high enough....

CSIG *a;

if (!cblk->sub) return 0;

if (z->disb) return 0;                // DON'T copy disabled csigs.

//DBGPRTN("Cpy csig lvs [%s %d %d] to %x ",z->sig->ptn->name, z->lev1, z->lev2,cblk->sub->addr); 

a = find_csig(cblk->sub,0,z->sig->ptn,cblk->curaddr);

if (!a && (z->lev1 > 1 || z->lev2 > 1))           // not match addr or pattern and > lev 1
    {

     a = add_csig(cblk->sub);               //,0);            //z->sig->ptn->forb);
     *a = *z;                           // copy to new block
     a->next = 0;                       // clear next
     if (a->lev1 > 0) a->lev1--;        // safety so it don't wrap
     if (a->lev1 > 0) a->l1d = 0;       // > 0, mark as active again
     if (a->lev2 > 0) a->lev2--;        // safety so it don't wrap
     if (a->lev2 > 0) a->l2d = 0;
    
     a->caddr = cblk->curaddr;
 //    PRT_CSIG(a);
 //    DBGPRTN(0);
     return a;
    }

 return 0;
  }

void sdumx (CSIG *z, SXT *c, SBK *b)
{
  // dummy handler
 // DBGPRTN(" SKIP %x (%s)",b->curaddr, z->sig->ptn->name);
}


void fnplu (CSIG *z, SXT *csub, SBK *blk)
 {

    // func lookup - don't do anything to SUB itself
    // added check for signed sigs at front, if not done already....

   int adr, size;      //, val, sign;       //, ans, val2;
   SIG   *s;        //, *p;
   DDT   *d;
   CSIG  *x;

   s = z->sig;

   size = s->v[2]/2;                 // size from sig (unsigned)
   
//   DBGPRT("In FNPLU %d (%x)", size, blk->curaddr);

   if (size < 1 || size > 4)
       {
 //       DBGPRTN("FUNC Invalid size - set default =1");
        size = 1;
       }

 

   x = find_csigp(csub);

   if (x)
     {
       // this subr has an inline par getter
       adr = get_subpars(csub,blk, s->v[4] | 0x100);      // reqd register
     }

   else
     {
      adr = g_word(s->v[4]);        // address from sig
  //    DBGPRTN("Direct addr R%x = %x", s->v[4], adr); 

     }
   if (adr < 256 && adr > minwreg)
     {
  //    DBGPRT("FUNC addr is reg %x ", adr);
      adr = g_word(adr);
 //     DBGPRTN("= %x", adr);
     }

  if (adr == 0)
     {
  //   DBGPRTN("FUNC Invalid address ZERO (%x)",blk->curaddr );
      return;
     }
  
  if (size > 1 && (adr & 1))
       {
    //    DBGPRTN("FUNC odd start addr for word func - adjust PC+1");
        adr+= 1;
       }

   adr &= 0xffff;
   adr |= datbank;       // need this as get_reg only does 16 bit
  
      if (!val_data_addr(adr))
     {
  //    DBGPRTN("FUNC Invalid addr %x", adr);
      return;
     }

    d = add_ddt(adr, size, csub->addr);
    if (d)
      {
       d->func = 1;
       d->ssize = size;                // both cols in func are same size....
       d->p2 = size;

         if (s->v[0] == 0x100000)
          { 
           // by user command
           d->cmd = 1;             // no changes
           if (s->v[14] == 2) d->ssize |= 4;  // signed out
           if (s->v[15] == 2) d->p2 |= 4;  // signed in
          }
            //      DBGPRTN("ADD FUNC %x isize=%d osize=%d", adr, d->ssize, d->p2);
      }

    s->done = 1;                           // for sig tracking only

}



void tbplu (CSIG *z, SXT *csub, SBK *blk)

{
   int adr, cols;    //, val, sign;

   SIG *s;          //, *p, *t;
   DDT *d;
   CSIG *x;

   s = z->sig;

//  DBGPRT("In TBPLU ");
 
   x = find_csigp(csub);
   
   if (x)
   {
       // this subr has a par getter
       chg_subpars(csub, s->v[4], 2);                    // change reg to a WORD
       adr = get_subpars(csub,blk,s->v[9] | 0x100);      // first param

   }

   else
    adr = g_word(s->v[4]);

   if (adr < 255 && adr > minwreg)
       {
  //      DBGPRT("TABLE addr embedded = %x ", adr);
        adr = g_word(adr);
   //     DBGPRTN("= %x", adr);
       }

   // abandon scan ? add reg as a monitor ?

   adr &= 0xffff;
   adr |= datbank;                            //wbnk(datbank));

     // get extended block for tab or func commands

   if (!val_data_addr(adr))
      {
   //    DBGPRTN("TABLE Invaddr %x", adr);
        return;
      }


   cols = g_byte(s->v[3]);

//   DBGPRT("1 add %x cols (%x)=%x ", adr, s->v[3],cols);
   if (s->v[3] < 0) cols = - s->v[3];          // fixed size

/*
   if (!z->l1d)
    {
      // look for tabinterp subr first - tabinterp at main z->v[0] (jump addr)
      t = (SIG*) schmem();   
      t->ofst = s->v[0]; 
      
      t = (SIG*) find(&sigch,t);     // but could be AFTER start (sign flag)
      if (!t) t = scan_asigs (s->v[0]);
 
      if (t) 
       {
 
        if (!strcmp(t->ptn->name, "iu"))
         {
          // unsigned tabinterp - nothing else to do
           z->dat1 = 1;              
         }
        else
         {   
 
  //      DBGPRT(" found %s %x", t->ptn->name, s->v[0]);  
  //      s->v[3] = p->v[3];                       // location of signed flag
  //      s->v[14] = p->v[15];                     // and its val/mask
         }
       }
        
        
     // now look for sign       
 
  //    int find_wtram(int addr)   !!    was flag updated ?       
      
      if (csub->addr != s->ofst)
       {                           // not main subr, look for pre-sig
        p = (SIG*) schmem();   
        p->ofst = csub->addr;            
         
        p = (SIG*) find(&sigch,p);          // find sign sig ? but could be scaled sig also
        if (!p) p = scan_asigs (csub->addr);
 
        if (p) 
         {      // pre-sig
   //     DBGPRT(" found %s %x", p->ptn->name, csub->addr);
  // s sign t scale i interp
   
          if (*p->ptn->name == 'z')     z->par1 = 1;               // temp !!
         }
       }
      else p = 0;
  
      val = 0;
      if (p && t && p->v[3] == t->v[3])         // [3] is flag marker (prefix and tabinterp)
       {
        val = p->v[4];                              // val from this sig
    
        sign = t->v[15] & 0x1000;                // JNB or not (reversed from func lookups) 
        p->v[15] = 2;                                      // unsigned  4 & 5
        if ((val & t->v[15]) && !sign)   p->v[15] = 1;       // signed from interp
        if (!(val & t->v[15]) && sign)   p->v[15] = 1;
        z->dat1 = p->v[15];
     
  //     if (sign) s->v[15] = 1; else s->v[15] = 2;
       }

// bare tab lu - default to uns as can't tell without interp.....

  //   if (z->dat1) DBGPRTN(" SO"); else DBGPRTN(" UO");
     z->l1d = 1;       //mark as done.

     if (csub->xnam == 1)
       { 
         csub->xnam = 13;  
         if (!(s->v[15] & 0x1000)) 
          { csub->xnam ++;              // SU (signed in);
            z->dat1 = 2;
          }
         else z->dat1 = 1;      
     
      } 
    }

*/

















 //  DBGPRT("add %x cols %d ", adr, cols);


   if (cols <= 0 || cols > 64)
       {
 //       DBGPRT(" inv colsize %x. set 1",cols);
        cols = 1;
       }

   
    // tables are BYTE so far, do an UPD_CMD, to replace directly



   d = add_ddt(adr, cols, csub->addr);
   if (d)
     {                          // multiple updates ?
      d->p1 = cols;
      d->p2 = 1;
      d->ssize = 1;
      if (z->dat1) d->ssize |= 4;          // signed marker
   
      d->tab = 1;
     }

   s->done = 1;     // for display only
 //  DBGPRTN(0);
}



void pencdx (SIG *x, SBK *b)
{
  CSIG *y, *z;                       // sub's sig chain


  if (!b) return;                     // safety - proc expected

 // DBGPRT ("In pencdx");
  
  y = init_csig(x,b);

  if (x->v[9]) y->dat1 = x->v[9];
  else y->dat1 = x->v[8];           // where data is (for decoding)
  y->enc = 1;

//  PRT_CSIG(y);
//  DBGPRTN(" + Set dat %x, recalc", y->dat1);
  b->sub->sgclc = 1;                // set recalc....


// can't just stop here as we need a LEVEL from somewhere.....

z = find_csigp(b->sub);                // find prev param getter - may need more checks

 if (z)
   {
    y->lev1 = z->lev1;            // set encode level from param getter
//    DBGPRTN(" lev=%d (<- %s)", y->lev1, z->sig->ptn->name);
    if (x->v[9])               // has indirect assign at front....
    {
      y->lev1++;                // so assume one level extra
 //     DBGPRT(" ++");
    }
  //  DBGPRTN(0); 
   }
}




void encdx (CSIG *z, SXT *csub, SBK *blk)
{


  CSIG *c;
  SIG *s;

  s = z->sig;

//  DBGPRTN("in encdx");

  if (z->disb) {
      //DBGPRTN ("encdx disb");
 return;}


  // if direct, sig gets replaced in preprocess
    
   if (z->lev1 > 1)                 // not yet, copy down a level, but get data dest if indirect
      {
       c = find_csigp(csub);
       if (c && s->v[9]) 
        {  
   //      DBGPRTN(" rpar %x",c->dat2);
         z->dat1 = c->dat2;
        }
       cpy_csig_levels(z,blk);
       return;
      }

}



void pfparp(SIG *s, SBK *b)
{
    //preprocess for fparp -- add multiple fparp here ?

    CSIG *c;

    if (!b) return;

//    DBGPRT("In pfparp");

    c = init_csig(s,b);  

    c->dat1 = s->v[4];     // first data register move from 4 to 16
    c->par1 = s->v[3];     // no of pars
    c->gpar = 1;
    c->lev1 = s->v[1];
    c->lev2 = 0;
    
    if (s->v[12] == 2)  // variable patt match...
      {
       c->dat2 = g_byte(s->v[14]);
      }
    
    
//    PRT_CSIG(c);
//    DBGPRTN(0);
  }



void pfp5z(SIG *x, SBK *b)
{
     CSIG *c;

     if (!b) return;
     // only for 8065 ..... levels are actually FIXED by the   ldw   R40,[R20+2] style instead of popw
     
 //    DBGPRT("In pfp5Z ");

     c = init_csig(x,b);           //,0);

     c->par1 = x->v[3];
     c->dat1 = x->v[4];
     c->gpar = 1;                  // this is a param getter
     c->lev1 = 2;                   // fiddle !!
  //   c->lev2 = 0;
 //    PRT_CSIG(c);
//  DBGPRTN(0);     
}



void pfp52 (SIG *x, SBK *b)
  {
     CSIG *c;

     if (!b) return;
     // only for 8065 ..... levels are actually FIXED by the   ldw   R40,[R20+2] style instead of popw
     
 //    DBGPRT("In pfp52 ");

     c = init_csig(x,b);           //,0);

     c->par1 = x->v[3];
     c->dat1 = x->v[4];
     c->gpar = 1;                  // this is a param getter
     
     c->lev1 = x->v[15]/2;                   // temp fiddle !!
     
 //    PRT_CSIG(c);
//  DBGPRTN(0);  
  }




void do_pgcd(CSIG *z, SXT *s)
{
  // add cadt for par getters, fpars and vpars, with check for encodes 
  // do in sets of 2, so that any subsequent encdx can be resolved into it.
// assume that sequential registers will form WORDS  

  int i,j;
  CADT *a;
  SXDT *d;
  CSIG *t;
  SIG *x;  

     d = get_sxdt(s,0);
     
     i = z->par1;       // no of bytes
     j = z->dat1;       // start register
  //   y = z->sig;

     while (i > 0)
      {
       a = add_cadt(&(d->cadt));            // at end of subr chain

       if (a)
        {  
         // check for following encoder first !!
         t = find_csige(z,j);       // find any encoders after THIS csig for j (data reg)

         if (t)
          {
          // add an encoded cadt instead  
           x = t->sig;
           a->enc = 3;                                   // ENC13 (default)

           if (*x->ptn->name == 0x32) a->enc = 4;        // ENC24 from name
   
           if (a->enc == 4 && x->v[3] == 0xf)  a->enc = 2;            // type 2, not 4
           if (a->enc == 3 && x->v[3] == 0xfe) a->enc = 1;             // type 1, not 3

           a->addr  = x->v[5];                            // 'start of data' base reg
           a->dreg  = j; 
           a->ssize = 2;                  // word based
           a->cnt   = 1;                    // zero words
           a->name  = 1;                    // find symbol names
           x->done = 1;
   
  //         DBGPRT(" enc cadt %d (%x) ",a->enc, z->caddr);
          } 
       else
        {
          // cnt = 1 and ssize = 1 by default
          if (i >=2 && (j & 1) == 0) {a->ssize = 2;}    // even start and at least 2 left    
         //else a->cnt = 1;                          // otherwise 1 byte
         a->dreg = j;                             // data destination
         a->name = 1;                              // find sym names
         a->pget = 1;                              // this is a par getter
    //     DBGPRT("pget cadt (%x)",z->caddr);
        }
       }  
       i-= a->cnt*a->ssize;           // doesn't work if SIGNED
       j+= a->cnt*a->ssize;
      }

}


void fparp (CSIG *z, SXT *csub, SBK *blk)
   {
    // fixed pars subr - THIS IS SUBSET OF VPAR ?   ONLY NEED ONE PROCESSOR ?
    /* VPAR IS
    * [15] first pop level
    * [14] first num pars (bytes)
    * [1] second pop level        [1-3] to match fpar
    * [2] second num pars
    * [3] param list (incremented) */

    // this is -
    // s->v[1] is level (1)
    // s->v[2] is no of bytes
    // s->v[3] is addr/reg which then may be used by encx

    // if levels > 1 then copy this sig to caller subr (via blk->caller)
    
 



    SIG *s;
//    SXDT *d;

    s = z->sig;
 //   d = get_sxdt(csub,0);

 //   DBGPRTN("In fparp %x (%d)", blk->curaddr, z->lev1);

    if (z->lev1 > 1)                 // not yet, copy down a level
      {
       cpy_csig_levels(z,blk);
       return;
      }


    // split into sets of 2 bytes for a possible encode to follow
    // dat1 is param list start, par1 is number. (dreg is start in each CADT)

    if (!z->l1d)
    {
        
     if (s->v[12] > 1) 
      {
        int m,v;
        
        csub->nargs2 = s->v[12];
        
        if (s->v[13] == 2)  // bit jump
        {
        if (s->v[11] & 0x100) v = 0; else v = 1;      // mask value
        m = s->v[11] & 0xf;                    // mask

        if ((z->dat2 & m) == v)  z->par1 += s->v[12];
        }
     
  //     DBGPRT("z");
    

      }        
        
        
        
     do_pgcd(z,csub);

     if (csub->nargs2) z->par1 -= csub->nargs2; 

 //    DBGPRT("subr %x add cadt (%x)",csub->addr,blk->curaddr);
  //   PRT_CADT(d->cadt);
  //   DBGPRTN(0);
     set_size(csub);
     s->done = 1;
     z->l1d = 1;
 //    DBGPRTN ("fparp done");
    }

 //   get_subpars(csub,blk,0);

   }


void feparp (CSIG *z, SXT *csub, SBK *blk)
   {
    // fixed pars subr with embedded encode
 
    // s->v[1] is level (1)
    // s->v[2] is no of bytes
    // s->v[3] is addr/reg which then may be used by encx

    // if levels > 1 then copy this sig to caller subr (via blk->caller)
    
    SIG *s;
    SXDT *d;
    CADT *a;
    int i;
    
    s = z->sig;
    d = get_sxdt(csub,0);

 //   DBGPRTN("In feparp %x (%d)", blk->curaddr, z->lev1);

    if (z->lev1 > 1)                 // not yet, copy down a level
      {
       cpy_csig_levels(z,blk);
       return;
      }


    // split into sets of 2 bytes for a possible encode to follow
    // dat1 is param list start, par1 is number. (dreg is start in each CADT)

    if (!z->l1d)
    {
     // encoded par is FIRST, then ordinary par.
   
      d = get_sxdt(csub,0);
     
      a = add_cadt(&(d->cadt));            // at end of subr chain
      if (a)
       {  // encode
        a->enc = 4;   
        if (s->v[15] == 0xf)  a->enc = 2;            // type 2, not 4
        a->addr  = s->v[11];                         // 'start of data' base reg
        a->dreg  = s->v[4]; 
        a->ssize = 2;                  // word based
        a->cnt   = 1;                    // zero words
        a->name  = 1;                    // find symbol names
       }
 
      i = s->v[3];         // total par count  // cnt = 1 and ssize = 1 by default
      i -= 2;               // encode from above;
      
      a = add_cadt(&(d->cadt));            // at end of subr chain
      if (a)
        {           // standard par
         a->dreg = s->v[6];                        // data destination
//         a->name = 1;                              // find sym names
         a->pget = 1;                              // this is a par getter
//         if (i & 1) 
//           {
//            a->ssize = 2;
//            a->cnt = i/2;
//           }
//         else
          { 
            a->ssize = 1;
            a->cnt = i;
          }    // even start and at least 2 left   
 
        }  // if (a) 
    } 
        
 //    DBGPRT("subr %x add cadt (%x)",csub->addr,blk->curaddr);
  //   PRT_CADT(d->cadt);
  //   DBGPRTN(0);
     set_size(csub);
     s->done = 1;
     z->l1d = 1;
  //   DBGPRTN ("feparp done");
    }


void pvparp (SIG *x, SBK *b) // SXT *p)
{
  CSIG *c;

  if (!b) return;

//  DBGPRT("In pvparp ");

  c = init_csig(x,b);

  c->dat1 = g_word(x->v[9]);             // address of 2nd level data (assumes some emulation...)
  c->gpar = 1;                     // x->v[3]
  c->lev1 = x->v[1];
   
  c->lev2 = x->v[2];              //p->lv2];
  c->lev1 += c->lev2;

//  PRT_CSIG(c);
//  DBGPRTN(0);
  // num pars not available yet.....
  
}

void pvpar5 (SIG *x, SBK *b) // SXT *p)
{
  CSIG *c;

  if (!b) return;

//  DBGPRT("In pvpar5 ");

  c = init_csig(x,b);

  c->dat1 = g_word(x->v[9]);             // address of 2nd level data (assumes some emulation...)
  c->gpar = x->v[3];
  c->lev1 = 2;
   
  c->lev2 = 1; 
 // c->lev1 += c->lev2;

//  PRT_CSIG(c);
//  DBGPRTN(0);
  // num pars not available yet.....
  
}



void vparp (CSIG *z, SXT *csub, SBK *blk)
  {
     /* variable args at various levels
      * a double level param getter
     [15] first pop level
     [14] first num pars (bytes)
     [1] second pop level        [1-3] to match fpar
     [2] second num pars
     [3] param list (incremented) */

   // current value of v[9] done in preprocess
   // safe to use addr here (used with offset and encode rbases)

   // fix this first and then .......

   CADT *a;      //, *n;
   SIG *s;
   SXDT *d;
 //  CSIG *c;
 //  int i,j;

   d = get_sxdt(csub,1);
   s = z->sig;

 //  DBGPRT("In Vparp %x L1=%d L2=%d P=%x", blk->curaddr, z->lev1, z->lev2, z->par1);
 //  if (z->l1d ) DBGPRT (" L1 Done");
 //  if (z->l2d ) DBGPRT (" L2 Done");
 //  DBGPRTN(0);


   if (z->lev2 > 1)          // >1 not this proc yet, copy to caller
      {
       cpy_csig_levels(z,blk); 
       return;
      }
 
   if (z->lev2 == 1)
     {                  // process at this level (numof pars reqd)
       if (!z->l2d)
        {                 // not processed yet - add CADT    
          a = add_cadt(&(d->cadt));
          if (a)                                 // safety
           {                                     // add LOCAL param as a CADT    (this is number of pars to get from caller)
            a->ssize = 1;                       // NOT redundant (as rewrite)
            a->cnt = s->v[4];                  // num of pars first level
            a->dreg = s->v[5];                  // FIRST data location (for first level)
            set_size(csub);
      //      DBGPRTN("sub %x add cadt L=%d", csub->addr, csub->nargs1);
      //      PRT_CADT(d->cadt);
            z->l2d = 1;                // local level done  ->done =1;
            s->done = 1;
           }
        }
       z->par1 = get_subpars(csub, blk, 1);     // get any subpars when l2 = 1 - this gets TARGET values !!
    //   DBGPRTN(" VPARP Varg = %x [%x]", z->par1, blk->nextaddr);
    }

   if (z->lev1 > 1)         // >1 not pars for this proc yet, copy to caller
      { 
       cpy_csig_levels(z,blk);        // change this for multiple fpars ?
       
       return;
      }



   if (z->lev1 == 1)
    {
    if (!z->l1d)
    {
     do_pgcd(z,csub);               // must add an 'assume consecutive' !
     set_size(csub);

  //     DBGPRT("subr %x add cadt R (%x)",csub->addr,blk->curaddr);
  //     PRT_CADT(d->cadt);
   //    DBGPRTN(0);
       s->done = 1;
       z->l1d = 1;
    } 
   z->dat2 = get_subpars(csub, blk, 1);        //TEST - first param. seems to work for encdx replace (later on)
    }
    // get subpars ?
 }  


void grnfp(CSIG *z, SXT *csub, SBK *blk)
{
  CSIG *c;
 // SIG *s;

  if (z->l1d) {
      //DBGPRTN("GRNF done");
      return;}

//  DBGPRTN("In GRNF subr %x (%d) for %x", csub->addr, csub->nargs1, blk->curaddr );

  //  extra POP before param getter.  Increase level1 by no of pops
  // NB. this currently pops but then calls getpar LOCALLY so only ups l1, NOT l2

  c = get_csig(csub);

  while (c)
  {
    if (c != z)
      {      // skip GRNF sig itself
       c->lev1 += z->lev1;
 //      DBGPRTN("Sig %s +lev = %d and %d",c->sig->ptn->name, c->lev1, c->lev2);
    }
    c = c->next;
  }

z->l1d = 1;
z->sig->done = 1;

}


void ptimep(SIG *x, SBK *b)
{
    // don't have to have a subr for this to work, so can do as
    // a preprocess, but logic works as main process too (but only once)

 //SXT *s;
 //CSIG *c;
 LBK *k;
 CADT *a;
 int start,ofst, val,z;

 if (x->done) return;
 
// DBGPRTN("in ptimep");          //move this later
  
 if (b)                        // may be sub, but possibly not....
  { 
 //  s = b->sub;
//   DBGPRT ("%x Add csig to sub %x ",b->curaddr, s->addr); 
   //c = 
   init_csig(x, b);
//   PRT_CSIG(c);
//  DBGPRTN(0);  
  }
//  else s = 0;

  x->v[0] = g_word(x->v[1]);

 // size in v[14].  do a 'find end' for the zero terminator
 // cmd size (Y or W) is (2 or 4)=byte (3 or 5)=word

   start = x->v[0]; 
   start |= datbank;
   z = x->v[14];
   
 //   DBGPRT("Timer list %x size %d", x->v[0]-1, z+3);
 //   if (s) DBGPRT(" Sub %x", s->addr);
  //  DBGPRTN(0);

    val = 1;
    a = 0;
    
 // should also check for next cmd....and put names in here....
    
    ofst = start;
    
    while (ofst < maxadd(ofst))
    { 
      val = g_byte(ofst);
      if (!val) break;
      ofst += z;
      if (val & 1) ofst+=3; else ofst+=1;
    }

    k = set_cmd(start, ofst, C_TIMR);    // this basic works, need to add symbols ?....
    if (k) a = add_cadt(&k->adnl);
    if (a) {a->ssize = z; a->name = 1;}

  x->done = 1;
}



 /* 
 * 
 * void set_time(CPS *cmnd)
 {
  LBK *blk;
  int val,type,bank;
  int xofst;
  short b, m, sn;
  SYT *s;
  CADT *a;
  char *z;

  blk = set_cmd (cmnd->start,cmnd->end,cmnd->fcom,1);
  set_adnl(cmnd,blk);
  DBGPRTN(0);
  new_sym(2,cmnd->symname,cmnd->start, -1);  // safe for null name

  // up to here is same as set_prm...now add names

  if (!blk) return;          // command didn't stick
  a = blk->addnl;
  if (!a) return;                     //  nothing extra to do
  sn = 0;                             // temp flag after SIGN removed !!
  if (a->addr) sn = 1;
  if (!a->name && !sn) return;       // nothing extra to do

  if (a->ssize < 1) a->ssize = 1;  // safety

  xofst = cmnd->start;
  bank = xofst &= 0xf0000;                  //cmnd->start >> 16;
  while (xofst < maxadd(xofst))
   {
    type = g_byte (xofst++);                  // first byte - flags of time size
    if (!type) break;                         // end of list;

    val = g_val (xofst, a->ssize);            // address of counter
    val |= bank;

    if (a->name) // && (cmdopts & T)
    {
     z = nm;
     z += sprintf(z, "%s", anames[0].pname);
     z += sprintf(z, "%d", anames[0].nval++);
     if (type& 0x4)  z += sprintf(z,"D"); else z+=sprintf(z,"U");
     if (type&0x20) z += sprintf(z,"_mS");
     else
     if (type&0x40) z += sprintf(z,"_8thS");
     else
     if (type&0x80) z += sprintf(z,"_S");
     s = new_sym(0,nm,a->addr,type);     // add new (read) sym
    }


    xofst+= casz[a->ssize];                           // jump over address (word or byte)

    z = nm;
    if (type&1)      // int entry
     {
      if (s && sn)
        {                      // add flags word name too
        m = g_byte (xofst++); // bit mask
        b = 0;
        while (!(m&1)) {m /= 2; b++;}    // bit number
        val = g_byte (xofst++);    // address of bit mask
        val |= bank;
        if (type & 8) z+= sprintf(z,"Stop_"); else z += sprintf(z,"Go_");
        sprintf(z,"%s",s->name);
        new_sym(0,nm, val, b);
        }
      else     xofst+=2;                // jump over extras
     }
   }


      // upd sym to add flags and times
 }



*/



void do_substructs (SBK* nsc, SBK * caller)             // was int
  {
   // nsc is new block, for sub 
   // caller is caller of nsc (and sub), which is where args are

   SXT *sub; 
   CSIG *z;                  // for sub's sig chain

  if (anlpass >= ANLPRT) return;

 // DBGPRT("substructs ");
 
 if (!nsc)
   {
 //    DBGPRTN("nsc Null");         // no called sub - should never happen
     return;
    }
 
 sub = nsc->sub;
 
 if (!sub)
   {
 //   DBGPRTN("Sub Null");         // no called sub - should never happen
    return;
   }

 //if (sub->cmd) DBGPRTN("Sub CMD");
  
 // DBGPRTN("at %x for sub %x", caller->curaddr, sub->addr);

  z = get_csig(sub);                // any sigs in called sub ?

  // nsc->sub is target sub.  Do_sigs may add sigs at different levels for fpar,vpar, GRNF, etc
  // will be found when sub scanned

 if (sub->sgclc && sub->sdat) freecadt(&sub->sdat->cadt);         //recalc set - clear ALL cadts so far

  while (z)                          // process each sig in order along chain
   {
    if (!sub->cmd || z->sig->ptn->spf)    // not cmd or specific override
      {  
       if (sub->sgclc) z->l1d = 0;      // reset 
       z->sig->ptn->sig_pr(z,sub,caller);
      } 
    z = z->next;
   }

   sub->sgclc = 0;
 //  DBGPSUB(sub,caller);
 //  DBGPRTN("Exit substr");
   get_subpars(sub,caller,0);                // for data markers...
   caller->nextaddr += sub->nargs1;
   
  // check for invalid opcode here !! but isn't always spotted....
}



JLT *add_jump (SBK *b)
{
  JLT *j,*z;
  OPS *o;               // assumes destination in [1].addr
  int ix;

  if (anlpass >= ANLPRT) return NULL;

  o = curops + 1;

  if (!val_jmp_addr(o->addr))
    {
  //   DBGPRTN("Invalid jump to %x", o->addr);
     return NULL;
    }

  j = (JLT *) chmem(&jpfch);
  j->from  = b->curaddr;
  j->cnt   = b->nextaddr - b->curaddr;
  j->to    = o->addr;

  j->type  = b->htype;

  if ((j->to & 0xf0000) != (j->from & 0xf0000))  j->bswp = 1;        // bank change
  
  else  if (j->type > 0 && j->type < 4 && j->from >= j->to) j->back = 1;  // not for sub calls or vects or rets

  if (!j->type) j->retn = 1;          // return jump (flag can be inherited to other jumps)

  // ***** now INSERT into the two jump chains
  
  ix = bfindix(&jpfch, j);      // FROM chain

  if (ix < jpfch.num)
    {                            // check for duplicate jump
     z = (JLT *) jpfch.ptrs[ix];
     if (j->to == z->to && j->from == z->from && j->type == z->type) return NULL;
    }

  chinsert(&jpfch, ix, j);                    // do insert in FROM chain

  ix = bfindix(&jptch, j);        // add to TO chain
  chinsert(&jptch, ix, j);

  return j;
}





INST *get_prev_opcode(INST *x)
 {

  int i, ofst;

  ofst = get_prev_opstart (x->ofst);        // previous opcode

  i = lpix;                                // current index

  while (1)
    {
     i--; 
     if (i < 0) i = PILESZ-1;       // loop

     if (i == lpix)             // not found
       {

        x = 0;
        break;
       }

     x = instpile + i;
     if (x->ofst == ofst) return x;       // found
  }

  return 0;

 }



int check_dead(SBK *n, SBK *s)
  {
   // check for deadlock in hold chain
   // make sure 'n' does not have 's' in its chain.
   //  s->hblk = n;   from below, so check if  blk s called blk n

    int ans;
    ans = 0;

    if (n == s) return 1;

    while (n)
      {
       // block not scanned yet and is on hold to s, so can't hold on it
       // may need to check beyond find pointer......

       if (!n->scanned &&  n->hblk == s)
           {
            ans = 1;
      //      DBGPRT("DDLK ");
            break;
           }
       n = n->hblk;
      }
    return ans;
  }
  
  
int do_hold(SBK *nsc, SBK *cblk)
 {
  // return 1 if on hold, zero otherwise.

  cblk->hblk = 0;         // nsc not valid or in hold chain

  if (cblk->type == 1) return 0;            // inital jump, no hold

  if (!nsc || nsc->scanned) return 0;        // scan block invalid, or scanned already
  if (check_dead(nsc,cblk))  return 0;        // check blk is not in hold chain of n

  get_scanpars(cblk);     // doesn't work without this...fixes scanned (reused) subr for new params
  cblk->hblk = nsc;       // hold blk, it has called nsc
  nsc->holder = 1;        // nsc now has holds waiting on it
//  holdsp ++;
 // if (holdsp > maxholds) maxholds = holdsp;

 // DBGPRTN("HOLD at %x -> %x (t=%d)", cblk->curaddr,nsc->start, cblk->htype);
  return 1;

 }



int do_sjsubr(SBK *cblk, const OPC *opl)
{
  /* do subroutine or jump
  * new scanned block (or found one) may become holder of this block
  * cblk is therefore caller
  * for subroutine special funcs (and any args)
  */


  SBK *nsc;        // new scan, or dupl scan ptr
  int addr, ans;

  if (PMANL) return (opl->opt & E);           // no scans, use default 

  addr = curops[1].addr;

  switch (cblk->htype)                                // requested scan type
    {

     default:
 //       DBGPRT("Error at %x unknown htype =%d", cblk->curaddr,cblk->htype);

     case 0:                             // return, do nothing (end scan)
        return 1;


        
     case S_CJMP:                        // conditional jump

      /* don't necessarily need to add another scan here as code CAN continue, but
       * do it to get any alt. addresses (e.g. table.....)
       */

      // not sure if this best fix, but seeems to work....
      if (addr <= cblk->curaddr && addr > cblk->start) return 0;  //backwards jump, ignore
      if (addr == cblk->nextaddr) return 0;                          // loostop jump

      add_scan (addr,cblk->htype, cblk);  // need sub for signatures, including conditionals
      return 0;

 
     case S_ISUB:                        // interrupt subr (subs already added) - no caller.
        add_scan (addr,cblk->htype, 0);
      return 0;


     case S_SJMP:                       // static jump
        ans = 1;
        
        if (addr <= cblk->curaddr && addr >= cblk->start) ans = 0;   // local loop - ignore.    
         if (addr == cblk->nextaddr) ans = 0;
   
        if (cblk->type == S_CJMP)      // don't hold statics out from conditional ? TEST
        {
         add_scan (addr,cblk->htype,cblk);
         
         return ans;
        } 
                     
        // check for jump to already existing subroutine
        if (cblk->type > 5) 
          {         // THIS block (caller) is itself a subroutine call
           SXT *sub;
           sub = (SXT*)schmem();
           sub->addr = addr;
           sub = (SXT *) find(&subch, sub);
           
           if (sub && get_csig(sub) && cblk->sub && cblk->sub->addr != addr )
           {
              // found a static jump to an already created subr.
              // copy its attributes to this sub - MAY NOT BE CORRECT
  //            DBGPRT("Jump to subr-");
              cblk->htype = S_JSUB;                        // change to modded subr call.
              cpy_sub_csigs(cblk->sub,sub);                 // need blk for sig process
           }
          }
        else
        {   
         if (addr <= cblk->curaddr && addr >= cblk->start) return 0;     // local loop - temp ignore.    
         if (addr == cblk->nextaddr) return 0;                           // dummy/loopstop jump
        } 
     
     // fall through

     case S_VSUB:                       // vect (list) subr
     case S_CSUB:                       // std subr
     case S_JSUB:                       // modded jump to subr

           ans = 0;
           nsc = add_scan (addr,cblk->htype,cblk);     // nsc = new or zero if dupl/invalid
           
           if (do_hold(nsc,cblk)) break;              // this can fall through.

    }        // end switch           




    if (cblk->htype == 6 && !cblk->hblk)  do_substructs(nsc,cblk);       // always check any params for subr 
    
  
  return ans;
}


/***********************************************
 *  get label name for pc and assign operand parameter
 *  returns:    str ptr if found     0 otherwise
 *  get op value too
 ************************************************/

void op_val (int pc, int ix)
  // PC is SIGNED - so can be NEGATIVE
  // negative ix signifies use of CODE bank
  // assumes readsize already set in OPS cell
  // names and fixups moved to print phase

  {
  int bank;
  OPS *o;
  RBT* r;

  if (ix < 0)
   {
    bank = opbank(codbank);        // codebank for jumps
    ix = -ix;
   }
  else bank = opbank(datbank);   // data bank otherwise

  o = curops+ix;
  o->stk = 0;
  o->addr = pc;
  o->rbs = 0;

  if (o->addr < 0x100)   o->addr += rambank;     // always zero if !8065
  if (o->addr >= PCORG)  o->addr |= bank;        // add bank

  if (o->rgf)               // if register
     {
      o->addr &= 0xff;                  // safety;
      o->addr += rambank;               // rambank always zero if !8065
      if (o->addr == stkreg1)  o->stk = 1;
      if (o->addr == stkreg2)  o->stk = 1;        // stack marker

      r = (RBT*) schmem();
      r->radd = o->addr;
      o->rbs = (RBT*) find(&basech, r);          //o->addr,0);   // rbase register. base includes bank
      if (o->rbs && o->rbs->inv) o->rbs = 0;      // marked as invalid
     }

  if (o->imd || o->bit) o->val = pc;          // immediate or [0], val is pc
  else o->val = g_val(o->addr,o->ssize);      // get value from address, sized by op

  if (o->rbs) o->val = o->rbs->rval;                // safety for now - may only need this for PRINT

}

void op_imd (void)
{
  // convert op[1] to imd
  OPS *o;

  o = curops+1;
  o->imd  = 1;
  o->rgf  = 0;
  o->rbs  = 0;
  if (rambank) o->addr -=  rambank;               // remove any register bank
  o->val  = o->addr;
}

void do_vect_list(int start, int end)
 {
  int pc, vcall;
  LBK  *s;
  CADT *c;
  DDT  *d;

  if (anlpass >= ANLPRT) return;

  pc = start;

  d = add_ddt(pc,2,curinst->ofst);
  if (d) {d->vect = 1; d->dis = 1;}

  // would need to do PC+subpars for A9L proper display

  for (pc = start; pc < end; pc +=2)
    {
     vcall = g_word (pc);                            // address from list
     vcall |= opbank(codbank);                       // assume always in CODE not databank
//  ?     add_ddt(vcall);

     if (val_jmp_addr(vcall)) add_scan (vcall, S_VSUB,0);     // vect subr
    }


  s = set_cmd (start, end, C_VECT);
  if (s)
    {
     // check banks here......
     if (datbank != codbank)
      {
       c = s->adnl;
       if (!c) c = add_cadt(&s->adnl);
       c->foff = 1;
       c->ssize = 0;         // no size
       c->addr = codbank >> 16;   // add current codbank
      }
    }

 }

/******************************************
attempt to decode vector or vector list from push word
probably need to split off the vect list (for multibanks)
****************************************/

int check_vect_list(int start, int sz, int retaddr)
{
   int addr, i, score, lowcadd, end;
   SBK *s, *t;
   LBK *l, *k;

   // assume bank is correct in start
   if ((start & 0xffff) < PCORG) return 0;


   end = start + 0x300; 
   score = 0;                     // max score
   lowcadd = maxadd(curinst->ofst);              // bank max for call addresses - multibank ?

   // check start against curinst+cnt to see if an extra offset needs to be added....
   //this needs more checks as well

   //allow for sz = 0 !!
 //  DBGPRTN("in check_vect %x sz=%x R=%x lc=%x", start, sz, retaddr, lowcadd);

   if (!sz) sz = 0x300;
   t = (SBK*) chmem(&scanch);
   k = (LBK*) chmem(&auxch);


   for(i = start; i < end; i+=2)       // always word addresses  (score > 0?)
   {

     if (i >= start+sz && score < 10) {end=i-1;
// DBGPRTN("end %x", end);
  break;}

     if (i >= lowcadd) {
//         DBGPRTN("hit lc_add %x", i);
 break;}
     
     t->start = i;  // for check of SBK
     k->start = i;  // for check of auxcommands
     
     s = (SBK*) find(&scanch,t);
     if (s) {
  //          DBGPRTN("scan %x t%d", i, s->type);

            end = i-1;
            break;
            }

     l = (LBK *) find(&cmdch,k); 
     if (l) {
         if (l->fcom > C_WORD) {
    //     DBGPRTN("cmd %x %d",i,l->fcom);
 end = i-1; break;
         }}

     addr = g_word (i);
     

     addr |= (retaddr & 0xf0000);    // bank from return addr

     if (!val_jmp_addr(addr)) {
         score +=2;
  //       DBGPRTN("iadd %x (%x)",i, addr);
         addr = 0;
         }

     if (addr)
     {
       // lowest call address - but this doesn't work for multibanks !!
       if (addr > start && addr < lowcadd) {lowcadd = addr; }


       l = (LBK *) find(&auxch, k);
       if (l)
        {
         score +=2;
 //        DBGPRTN("Aux %x=%x",i, addr);
         }

       if ((addr & 0xff) == 0)     // suspicious, but possible
          {
         score ++;
   //      DBGPRTN("Suspicious %x=%x",i, addr);
         }
       // more scores ??
     }
    else
      if (i == start) start +=2;
   }

 //  DBGPRTN("Check_vect %x-%x (%d)|lowc %x|score %d", start, i, i-start, lowcadd,score);

   if (lowcadd < end) end = lowcadd-1;

   addr = i-start;
   if (addr >0)
   {
     score = (score*100)/addr;
 //    DBGPRT ("FINAL SCORE %d",score);
 //     if (score < 10)   DBGPRT ("PASS"); else DBGPRT("FAIL");
  //   DBGPRT("\n");

     if (score < 10) do_vect_list(start,end);
     return 1;
   }
  return 0;
}



int popw(SBK *s)
{


//  if (anlpass < ANLPRT) DBGPRT("POPW %x %d r%x ",s->curaddr,s->type,curops[curinst->opl->wop].addr);

curops[opctbl[curinst->opcix].wop].val = 0;         // test
 
	  return 0;
}


 

 void vct1  (SIG *x, SBK *b)
  {

   int start, end;

   if (anlpass >= ANLPRT)  return;
   // assume bank now included in all addresses

     // a LIST, from indirect pushw

 //    DBGPRTN("In vct1 from %x for %s (%x)",x->ofst, x->ptn->name, x->v[4]);

     start = x->v[4] ;

     if (start > PCORG)
     {                                // ignore register offsets

     if (start > x->ofst && start <= x->xend) start = x->xend+1;        //overlaps , so list must start at [1] or more
     if (start & 1) { start++;}                     // odd address not allowed

     end = start + x->v[1];

     // but size may be unknown !!

 //    DBGPRTN("Do LIST %x-%x",start, end);

    if (x->v[6]) end = x->v[6];
    else
    if (x->v[8]) end = x->v[8]; 
    else   
    end = start;    
        
        
     check_vect_list(start,x->v[1], end);
     }

     // Immediate, return addr pushed before list

     if (x->v[6])
     {
      start = x->v[6];
      if (val_code_addr(start))
       {
  //      DBGPRTN("Do IMD1 %x",start);
//      add_subr(start);
        add_scan (start,S_VSUB,0);          // treat as vect subr
       }
     }
     
      if (x->v[8])
     {
      start = x->v[8];
      if (val_code_addr(start))
       {
  //      DBGPRTN("Do IMD2 %x",start);
        add_scan (start,S_VSUB,0);          // treat as vect subr
       }
     }

     x->done = 1;
  }


 int pshw (SBK *s)
{

  OPS *z;
  int addr;
  DDT *d;

  if (anlpass >= ANLPRT) return 0;

  // now, check for single vect or vect list
  // immediate may be a vect list (but not always)
  // MUST REWORK THIS TO CHECK FOR A RET.. otherwise can give invalid addresses
// but PE has a push (30de) which does not get  used till much later....
// check the RET ?

  z = curinst->opr + 1;


 // DBGPRT("PUSH R%x (%x) ", z->addr, curinst->ofst);
 // if (s->sub && s->type > 3) s->sub->popl--;

  addr = z->addr & 0xffff;

  //if (curinst->inx)  addr = curinst->opr->val;      // [0]

  //if (curinst->inr)  addr = z->val;

  addr |= opbank(datbank);               // if not in vect sig, assume it's DATA - NO THIS DOESN'T WORK !!
  d = add_ddt(addr,2,curinst->ofst);
  if (d) d->psh = 1;
  return 0;
}





short find_fjump (int ofst)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 4 for 'else', 3 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JLT * j;

  if (anlpass < ANLPRT) return 0;


  j = (JLT*) chmem(&jpfch);
  j->from = ofst;
  j = (JLT*) find(&jpfch,j);         // jump from

  if (!j) return 0;

 // if (j->elif) pstr(" *E* ");  maybe later
  if (j->retn) return 2;    // end of subr, check this FIRST
  if (j->cbkt) return 1;     // bracket - reverse condition in source
 

  return 0;
}


void find_tjump (int ofst)
{
  // find trailing jump, for brackets, matches fjump
  // prints close bracket for each jump found

  // need to put loop detection here ?


 JLT * j;
 short ix;

 if (anlpass < ANLPRT) return;
 if (!PSSRC) return;

 // not find, as needs to check more than one
  j = (JLT*) chmem(&jpfch);
  j->to = ofst;

 ix = bfindix(&jptch, j);     // a jump TO here

 while (ix >=0 && ix < jptch.num)
  {
   j = (JLT*) jptch.ptrs[ix];
   if (j->to == ofst)
    {
     if (j->cbkt)
       {
       p_pad(cposn[2]);
       pstr ("}");
      }
     ix++;
    }
   else break;    //stop as soon as different address
  }
}



void pp_hdr (int ofst, const char *com, short cnt, short pad)
{
  char *tt;
  if (!cnt) return;

  if (gcol) newl(1);                                // always start at beginning of line

  if (pad < 0) tt = NULL;
  else  tt = get_sym_name(0,ofst);                  // is there a symbol here ?

  if (tt)
    {
     pstr ("\n  %s:", tt);
     newl(1);                   // output newline and reset column after newline
    }

  if (!numbanks) pstr ("%4x: ", ofst & 0xffff);
  else pstr ("%05x: ", ofst);

  while (--cnt) pstr ("%02x,", g_byte(ofst++));
  pstr ("%02x", g_byte(ofst++));

  p_pad(pad >0 ? pad : cposn[0]);                      //INSTART);
  if (com) pstr ("%s",com);
  p_pad(cposn[1]);                                     //MNSTART);

}

/*************************************
get next comment line from file
move to combined bank+addr (as single hex)
**************************************/

short getcmnt (int *ofst, char **cmnt)
{
  short ans;
  int maxlen, rlen;
  int pcz;

  ans = 0;
  rlen = 0;
  maxlen = 0;
  pcz = 0;
  
  *cmnt = flbuf;               // safety

  if (fldata.fl[4] == NULL || feof(fldata.fl[4]))
   {
    *ofst = 0xfffff;          // max possible address
    **cmnt = '\0';
    return 0;                 // no file, or end of file
   }

  // bank,pc

  while (ans < 1 && fgets (flbuf, 255, fldata.fl[4]))
    {
     maxlen = strlen(flbuf);   // safety check
     ans = sscanf (flbuf, "%x%n", &pcz, &rlen);    // n is no of chars read to %n
    }

  if (rlen > maxlen) rlen = maxlen;

  if (ans)
    {
      if (!numbanks  && pcz < 0xffff)  pcz += 0x80000;  // force bank 8

      *ofst = pcz;
      *cmnt = flbuf+rlen;
      while (**cmnt == 0x20) (*cmnt)++;     // consume any spaces after number
      return 1;
    }

  **cmnt = '\0';    //anything else is dead and ignored...
  return 0;

}

// incorrectly stacked comments cause filled spaces (cos of print...)


char* symfromcmt(char *s, int *c)
 {
   int ans,add, bit;
   SYT *z;

  if (!isxdigit(*s))
   {
    *c = 0;
    return 0;   // anything other than digit after special char not allowed
   }
   bit = -1;
   z = 0;

   ans = sscanf(s,"%x%n:%x%n",&add,c,&bit,c);

   if (!ans) return NULL;

   // add,bit  with add or add,bit allowed


   if (add < PCORG) {}                //add |= RAMBNK;
   else
   if (!numbanks && add <= 0xffff)
     {
      add |= 0x80000;
     }

   if (ans > 1) z =  get_sym (0,add, bit);

   if (!z)      z = get_sym(0,add,-1);

   if (z) return z->name;
   return NULL;
 }


 // could allow a wrap() here for multiline comments by design also.



// print indent line for extra psuedo code lines

void p_indl (void)
{
  if (gcol) newl(1);
  p_pad(cposn[2]);

}

/***************************
signature pattern detection
**************************/

// will only print max of 'size' bytes
// add CHECK for incorrect table or struct overlapping next cmd.

CADT *pp_lev (int ofst, CADT *a, int pad, int max)
{

 int pfw, val, end, i, lev;
 float fval;
 SYT* sym;

 if (max)
   end = ofst + max;
 else
   end = ofst + 64;      /* safety stop 64 bytes */
 
 lev = 0;
 while (a && lev < MAXLEVS)
  {
   if (ofst >= end) break;                 // safety - force exit   
   if (a->disb) { a = a->next; continue; } // skip this entry

   for (i = 0; i < a->cnt; i++)      // count within each level
    {
     if (i) pchar(',');
     sym = NULL;
     pfw = 1;                               // min field width
     if (ofst > end) break;                // safety - force exit
     if (pad)
       {                                   // for table layouts etc, pad entries
        pfw = a->pfw;
        if (!pfw) pfw = cafw[a->ssize];          // default to preset
        if (a->divd)   pfw+=4;                   // 4 extra for ".000"
       }

     val = decode_addr(a, ofst);               // val with decode if nec.  and sign
     if (a->name)
       {
         sym = get_sym(0,val,-1);    // syname AFTER any decodes - was xval
       }

     if (sym) pstr("%s",sym->name);
     else
     if (a->divd)
       {
        char *tx;
        int cs;
        fval = val/a->divd;
        cs = sprintf(nm,"%*.3f", pfw, fval);
        tx = nm+cs-1;
        while (*tx == '0') *tx-- = ' ';
        if (*tx == '.') *tx = ' ';
        pstr("%s", nm);
       }
     else
     if (a->dcm)
      pstr("%*d", pfw, val);   // decimal print
     else
        { // hex prt
         val &= 0xFFFF;
         pstr("%*x", pfw, val);
        }
     ofst += casz[a->ssize];
     }
   lev++;
   a = a->next;
   if (a) pchar(',');
  }
return a;

}

//*********************************

int pp_text (int ofst, int ix)
{
  int cnt;
  short i, v;
  int xofst;
  LBK *x;
  x = (LBK*) cmdch.ptrs[ix];

  xofst = x->end;
  cnt = xofst-ofst+1;

  if (cnt > 16) cnt = 16;
  if (cnt < 1)  cnt = 1;

  pp_hdr (ofst, dirs[x->fcom].com, cnt,0);

  xofst = ofst;

  p_pad(cposn[3]);
  pstr("\"");
  for (i = cnt; i > 0; i--)
     {
      v = g_byte (xofst++);
      if (v >=0x20 && v <= 0x7f) pchar (v); else  pchar('.');
     }
  pchar('\"');

  return ofst+cnt;
}

//***********************************
// word and byte only - fiddle name position

int pp_wdbt (int ofst, int ix)
{
  CADT *a;
  char *tt;

  LBK *x;
  
  x = (LBK *) cmdch.ptrs[ix];

  a = x->adnl;

  if (!a)
   {       // add  block so can use pp_lev
    a = add_cadt(&(x->adnl));         // size 1 by default
    a->nodis = 1;           // don't print cadt in command prt
   }
  // always addnl now...
  if (x->fcom != C_BYTE) a->ssize = 2; 

  if (ofst + casz[a->ssize] > x->end+1)
    {
     // can't be a word if overlaps end, KLUDGE to byte
     a->ssize = 1;
     x->fcom = C_BYTE; 
  } 

  pp_hdr (ofst, dirs[x->fcom].com, casz[a->ssize],-1);
  pp_lev(ofst,x->adnl,1,0);

  // add symbol name in source code position

  tt = get_sym_name(0,ofst);

  if (tt)
    {
     p_pad(cposn[2]);
     pstr ("%s", tt);
    }

  return ofst+casz[a->ssize];
}


//***********************************

int pp_vect(int ofst, int ix)
{
  /*************************
  * A9l has param data in its vector list
  * do a find subr to decide what to print
  ***************/

  int val;
  char *n;
  CADT *a;
  LBK  *x;
  SXT * sub;
  int bank, size;

  x = (LBK*) cmdch.ptrs[ix];
  
  a = x->adnl;

  size = 0;
  val = g_word (ofst);

  if (a && a->foff && bkmap[a->addr].bok)
      bank = a->addr << 16;         //different bank specified in cadt
  else
     bank = ofst & 0xf0000;         // use bank from ofst
     
  val |= bank;

  sub = (SXT*) schmem();
  sub->addr = val;
  
  if (!find(&subch, sub)) size = 2;  // not correct for cal console etc
  
  if (ofst & 1)  size = 1;

  if (size)
      {
      pp_hdr (ofst, dirs[size].com, size,0);         // replace with WORD
      }
   else
    { pp_hdr (ofst, dirs[x->fcom].com, 2,0);
    size = 2;
    }
  n = get_sym_name (0,val);

  pstr ("%x,", cmdaddr(val));

  if (n)
     {
     p_pad(cposn[2]);                          //OPSSTART);
     pstr ("%s", n);
     }
  return ofst+size;
}

//  print a line as default

int pp_dft (int ofst, int ix)
{
  int i, cnt, w, sval;       // for big blocks of fill.....


  LBK *x;
  x = (LBK*) cmdch.ptrs[ix];

 if (!numbanks) w = 4; else w = 5;
 
 
  cnt = x->end-ofst+1;

  sval = g_byte(ofst);            // start val for repeats
  for (i = 0; i < cnt; ++i)
    {
      if (g_byte(ofst+i) != sval) break;
    }

  if (i > 32)   // more than 2 rows of fill     PUT THIS BACK FOR MULTIBANKS (LARGE BLKS OF FILL)
     {
      newl(2);
      pstr("## %*x to %*x = 0x%x  (filler ?) ## ", w,cmdaddr(ofst),w,cmdaddr(ofst+i-1), sval );
      newl(2);
      return ofst+i;
    }

  if (cnt <= 0 ) cnt = 1;
  if (cnt > 16) cnt = 16;      // 16 max in a line


  if (ofst + cnt > x->end) cnt = x->end - ofst +1;

  if (i >= cnt)
     {
      pp_hdr (ofst, 0, cnt,0);
      return ofst+cnt;
    }

  if (cnt > 8) cnt = 8;        // data up to 8 for UNKNOWN

  pp_hdr (ofst, "??", cnt,0);

  return ofst+cnt;
}

// this works for actual structure - need to sort up/down etc
// looks like may be standard set...

int pp_timer(int ofst, int ix)
{
  int val,cnt;
  int xofst;
//
//  int bank;
  SYT *sym;
  short bit, i;
  CADT *a;
  LBK *x;
  x = (LBK*) cmdch.ptrs[ix];



  sym = 0;
  a = x->adnl;
  if (!a)  a = add_cadt(&(x->adnl));       // just in case


  xofst = ofst;


  val = g_byte (xofst++);  // first byte
  if (!val)
    {
      pp_hdr (ofst,dirs[C_BYTE].com,1,0);    // end of list
 
      return xofst; 
     }


  cnt = (val & 1) ? 3 : 1;     // int or short entry
  pp_hdr (ofst, dirs[C_STCT].com, cnt+casz[a->ssize],0);
  pstr("%2x, ", val);
  val = g_val (xofst, a->ssize);
  xofst += casz[a->ssize];

  if (a->name) sym = get_sym(0,val,-1);           // syname AFTER any decodes
  if (sym) pstr("%s, ",sym->name);
  else pstr("%5x, ",val);

  if (cnt == 3)
   {                // int entry
    i = g_byte (xofst++);
    bit = 0;
    while (!(i&1) && bit < 16) {i /= 2; bit++;}          // LOOP without bit check !!
    val = g_byte (xofst++);

     if (a->name) {sym = get_sym(0,val,bit);
     if (!sym) sym = get_sym(0,val,-1); }
    if (sym) pstr("%s, ",sym->name);
    else  pstr(" B%d_%x",bit, val);
   }


  return ofst+cnt+casz[a->ssize];
}



void p_op (OPS *o)
{
 // bare single operand print

 if (o->imd) pstr ("%x", o->val & camk[o->ssize]);      // immediate (with size)
 if (o->rgf) pstr ("R%x",o->addr);                      // register, already 0xff | rambank
 if (o->inc) pstr ("++");                               // autoincrement
 if (o->bit) pstr ("B%d",o->val);                       // bit value 
 if (o->adr) pstr ("%x", cmdaddr(o->addr));             // address - jumps
 if (o->off) pstr ("%x", o->addr & camk[o->ssize]);     // address, as offset (inx)

}


void p_oper (int ix)
{
  // bare op print - top level

  OPS *o;

  ix &= 0x3;                      // safety check
  
  o = curops+ix;

  if (o->bkt)  pchar ('[');
  p_op (o);

  if (curinst->inx && ix == 1)        // indexed op - use op [0] as well.
    {                                // indexed ops are SIGNED offset, in [0]
      pchar ('+');
      p_op (curops);
    }
  if (o->bkt)  pchar (']');
  return;
}


int p_sc (OPS *o)           //INST *x, int ix)
{
// single op print for source code from supplied instance
// ans = 1 if printed something. 0 otherwise

int v;

 if (o->sym)  {pstr (o->sym->name); return 1; }

 if (o->imd)                               // immediate operand
    {
     pstr("%x", o->val & camk[o->ssize]);
     return 1;
    }

 if (o->rgf)
    {
     if (o->addr || o->wsize) pstr ("R");           // non zero reg, or write op
     pstr ("%x", o->addr);
     if (o->inc) pstr ("++");
     return 1;
    }

 if (o->bit) {pstr ("B%d_", o->val); return 1;}

 if (o->adr)
    {
     pstr ("%x", cmdaddr(o->addr));
     return 1;
    }
    
if (o->off)
    {
     v = o->addr;
     if (v < 0)  v = -v;
     pstr ("%x", v & camk[2]);  
     return 1;
    }
    
    
return 0;
}


int find_last_psw(INST **z)
 {
   // finds last PSW changing opcode for printout
    int ans, cnt;
    INST *x;
    const OPC *opl;

    x = curinst;
    cnt = 0;
    ans = -1;           // not found
    *z = 0;             // not found
 
    while (x && cnt < 16)
     {
      x = get_prev_opcode(x);
      cnt++;                         // test max 16 opcodes

       // need more checks in here.....opt & E  must be a stop , and probably CALL.
 
      if (!x) break;
      opl = opctbl + x->opcix;
      //   if (opl->opt & E)  break;                      // end instruction
      if (opl->sigix == 17)  break;              // call
      if (opl->sigix == 21)  break;              // set or clear carry
      
      if (opl->pswc)                  // sets PSW, allow target
         {
           *z = x;                   // instance found
           ans = opl->wop;           // return wop (zero for compare)
           break;
         }
     }

    return ans;
 }




void p_opsc (INST *x, int ix)
{
 // source code printout

  OPS *o, *b;

   if (!x)
   {
       pchar('0');
       return;
   } 



 o = x->opr + ix;                            // set operand

  if (x->inx && ix == 1)
    {                                        // indexed ops are SIGNED offset
      b = x->opr;
      if (o->bkt) pchar ('[');
      if (p_sc (o))
        {
         pchar( (b->addr < 0) ? '-' : '+');      // if first op printed
        }
      p_sc (b);
      if (o->bkt) pchar (']');
      return;
    }                     // end indexed

  if (o->bkt)  pchar ('[');
  p_sc (o);
  if (o->bkt)  pchar (']');
  return;
}

void do_comment(char *c, INST *l)
{
  char *x;
  int z, col;

   if (!c)  return;                               // safety
   if (*c != '|' ) p_pad(cposn[3]);               // '|' = newline at front, so no pad

     while (*c)
      {
       switch (*c)
        {
          // autocomment operands - a fiddle really ....

          case 1:
	      case 2:
	      case 3:
              if (l) p_opsc (l,*c);
    	      break;


         case '|' :                                // newline
           newl(1);
           break;

         case '~' :             // runout 80
           if (!gcol)
             {                // newline just issued ( '|')
              x = c+1;        // next char is the runout one
              for (z = 0; z < 80; z++) pchar(*x);
              c++;
             }
           else
            pchar(*c);
           break;

         case '@' :                                // '@' symname with padding, drop through
         case '$' :                                // '$' symname with no padding
           x = symfromcmt(c+1, &z);                // c+z+1 seems to work
           col = gcol;                             // remember this column
           if (x) pstr("%s",x);                                // if (x) pstr(x);
           if (*c == '@') p_pad(col+21);           // pad 21 chars from col (or not)
           c = c+z;                                // skip the sym numbers
           break;

         case '\n' :                               // '\n',  ignore it
           break;

         case '^' :                                // '^' wrap comment, or pad to comment column
           if (lastpad >= cposn[3]) wrap();
           else p_pad(cposn[3]);
           break;

         default:
           pchar(*c);                              // print anything else
           break;
        }
       c++;
      }
}



void pp_comment (int ofst)
{
 
  // ofst , for convenience, is for the START of a statement so comments
  // up to ofst -1 should be printed....


  if (anlpass < ANLPRT) return;
  if (ofst < 0) ofst = 1;
  if (ofst > maxadd(ofst)) ofst = maxadd(ofst)+1;         // safety checks

  while (cmntofst < ofst)
    {
     do_comment(cmtt,0); 
     if (!getcmnt(&cmntofst, &cmtt)) break;
    }
 }



void auto_comment (void)
{
// put auto generated in HERE
    if (*autocmt) { 
    do_comment(autocmt, cmntinst);
    *autocmt = 0;
    cmntinst = 0;             //safety
    }

}

// print subroutine params
int pp_stct (int ofst, int ix)
{
 int size, indent, end;
 LBK *x;
 CADT *a, *c;

 x = (LBK*) cmdch.ptrs[ix]; 
 a = x->adnl;

 // must safety check size versus end !!

 if (ofst+x->size > x->end) size = x->end-ofst+1;        // overlap !!   
 else size = x->size;
 
 if (!x->split)
  {
   // single line printout	
   indent = size*3+8;
   pp_hdr (ofst, dirs[x->fcom].com,size,indent);
   pp_lev(ofst,a,1, size);
   ofst += size;
   pp_comment (ofst);     
  } 
 else
 {
	 // multiline printout, look for split flag(s) - this will 
	end = ofst + size;
	indent = 0;
    while (a && ofst < end)
	 {
	  size = 0;
      c = a; 
	  while(a)
	   {
	    if (size && a->split) break; 
	    size += dsize(a);
	    a = a->next;
	   }
      if ((size*3+8) > indent) indent = size*3+8;      // temp lashup 
      pp_hdr (ofst, dirs[x->fcom].com, size,indent);
      pp_lev(ofst,c,1,size);
      ofst += size;
      pp_comment (ofst);     
	 }
 }	 

 if (x->term && ofst == x->end)     // terminator byte(s) ?
  {
   pp_hdr (ofst, "byte", x->term, indent);
   pstr("##  terminator");
   return ofst+x->term;           // only if term byte printed
  }
  return ofst;
}


int pp_subpars (int ofst, short cnt)
{
  SXT  *xsub;
  CADT *add;
  LBK  *sblk;
  short size;
  OPS *t;
  //short xpc,xbnk;

  add = NULL;
  size = 0;
  t = curops+1;

  xsub = (SXT*) chmem(&subch);
  xsub->addr = t->addr;

  xsub = (SXT*) find(&subch, xsub);

  if (xsub)
   {
    add = get_cadt(xsub);
    size = xsub->nargs1;
   }
 
  sblk = (LBK*) chmem(&auxch);
  sblk->start = ofst;
  sblk->fcom = C_ARGS;

  sblk = (LBK *) find(&auxch,sblk);
  if (sblk)
     {
     add =  sblk->adnl;      // ARGS command overrides proc settings
     size = sblk->size;
     }

   if (!add)
     {                 // no additional params
      if (PSSRC) pstr("();");
      return 0;
     }

     // 'addnl' must be set from here
   if (PSSRC)
    {
     pchar('(');

     if (size)
       {
        pp_lev(ofst+cnt,add,0,size);     // do params at min width, drop trail comma
       }
     pstr(");");
     }

     if (size)
       {
        pp_comment (ofst+cnt);                          // do any comment before args
        pp_hdr (ofst+cnt, "#args ", size,0);           // do args as sep line
       }
  return size;
}


//const char *

void fixup_sym_names(const char **tx)
{
  // adjust ops for correct SOURCE code printout, rbases, indexes etc.

  // sort of OK so far ....

  OPS *b, *o;
  SYT *s;
  int i, addr;
  const OPC* opl;

  opl = opctbl + curinst->opcix;
  //  BIT JUMP
  
  
  if (curops[3].bit)            // bit jump - if bitname, drop register name
     {
        o = curops+2;
        b = curops+3;               // b = bit number entry, get bit/flag name
        b->sym = get_sym (0,o->addr, b->val);
        if (b->sym)   {o->rgf = 0; o->sym = 0; return; }        // tx;}
     }

// get sym names FIRST, then can drop them later

  for (i = 1; i <= opl->nops; i++)
   {
    o = curops+i;
    if (i == opl->wop)
     o->sym = get_sym (1,o->addr,-1);
    else
     o->sym = get_sym (0,o->addr,-1);              // READ symbols
   }


  if (opl->opt & S)
   { // shift  
     if (curops[1].val < 16)  curops[1].sym = 0;
   } 





// indexed - check if RBS, or zero base (equiv of =[imd])

  if (curinst->inx)                             //indexed, use addr in cell [0] first
     {
     b = curops;        // probably redundant, but safety - cell [0]
     o = curops+1;

     if (!o->addr || o->rbs)
      {
        // drop register part and merge two terms into one as fixed addr
        // can't rely on o->val always being correct (pop screws it up...) so reset here
        if (!o->addr)  addr = 0;
        if (o->rbs) addr = o->rbs->rval; 
        
        addr += curops->addr;                   // should give true address....
        
        
        if (addr >= PCORG) addr |= opbank(datbank);                        // do need this for inx additions !!

        s = get_sym (o->wsize, addr,-1);           // and sym for true inx address

        curops->val = addr;                    // temp !!

        o->rgf = 0;
        o->sym = 0;         // drop any sym and register print

       if (s)               //symbol found at (true) addr
        {
          o->bkt = 0;       // drop brackets (from [1])
          curops->sym = s;   
        }

       else
        {  // update for printout as a single address
         b->addr = addr;

        }

      } // end merge

    else
      {  
        // this is an [Rx + off], or [Rx - off], just add [0] symbol if not a small value

        if (curops->addr > 0xff) curops->sym = get_sym (o->wsize, curops->addr,-1);           // and sym for true inx address


      }

    }         // end inx

       if (curinst->imd)       // clear symbol if register
     {
      if (curops[1].val < 0xff)  curops[1].sym = 0;      // not (normally) names for registers
      if (opl->sigix == 10) curops[1].sym = 0;  // must be abs value for imd compare !!
     }

    // 3 operands, look for rbase or zero in [1] or [2] convert or drop

 if (opl->nops > 2)
 {
     if (curops[1].rbs && curops[2].addr == 0)
        {       // if true, RBS address and sym already sorted
         curops[2] = curops[3];                // op [2] becomes wop and dropped 
         *tx = opctbl[47].sce;                  // use ldx string TEMP  ([2] = [1] )
        }
    //   need to use ALT for this for full solution !!
     

     if (curops[2].rbs && !curinst->inx)        // RBASE by addition, not indexed
      {
       b = curops+1;
       b->val = b->val + curops[2].val;                   // should give true address....
       b->sym = get_sym (0, b->val,-1);     // redo symbol check

       curops[2] = curops[3];                // drop op [2]
       *tx = opctbl[47].sce;        // use ldx string TEMP  ([2] = [1] )

      }

    if (curops[2].addr == 0)         // 0+x,   drop x
     {
      curops[2] = curops[3];                // drop op [2]
      *tx = opctbl[47].sce;        // use ldx string TEMP  ([2] = [1] )
     } 

    if (curops[1].addr == 0)      // 0+x drop x
     {
      curops[1] = curops[2];
      curops[2] = curops[3];              // shuffle ops up

      *tx = opctbl[47].sce;        // use ldx string TEMP  ([2] = [1] )

     }

 }
else
 {
  // can sometimes get 0+x in 2 op adds as well for carry
 // "\x2 += \x1 + CY;"
  if (curops[1].addr == 0 &&  (opl->opt & Z))
  {
   *tx = scespec[opl->sigix-6];   // drop op [1] via array of alts
  }  

}

//return tx;

}



short get_bitsyms (SYT **snam)
 {

 // for mask bit conversion to multiple sym names
 // ans no of syms found for given address and mask
 // this search assumes syms are in strictly correct order, bits then -1 (0->7)
 // and size is now known !!

   int i,ix,hsym,add, maxbit, maxadd;
   SYT *xsym;

   add =  curops[2].addr;
   maxbit = 7;                  // default to byte size
   maxadd = add;

   hsym = -1;  // highest bit number found
   for (i = 0; i < 16; i++) snam[i] = NULL;   // clear names

   xsym = (SYT*) chmem(&symch);
   xsym->add = add;
   xsym->bitno = -1;

   ix = bfindix(&symch, xsym);   // first entry for add
   if (ix >= symch.num) return 0;

   if (casz[curops[2].ssize] > 1)
   {
       maxbit = 15;       // this is a word op
       maxadd++;
   }

    // now ix is at first xsym which matches pc (probably whole sym)

   xsym = (SYT *) symch.ptrs[ix];    // could be NULL ?

   while (ix < symch.num && xsym->add <= maxadd)
     {
      i = xsym->bitno;

      if (i >= 0)
       {
        if (xsym->add > add) i += 8;    // for 2nd byte of a word

        if (i <= maxbit)
          {
           snam[i] = xsym;
           if (i > hsym)  hsym =  i;      // keep highest bitno
          }
       }
      xsym = (SYT *) symch.ptrs[++ix];    // could be NULL ?
     }
  return hsym;
}

//const char *
void bitwise_replace(const char **tx)

/* check for bitwise operations - not for 3 operand instructions or where no symname ?*/
{

  short lg, i, k;
  short mask;
  char eq[3];
  const OPC * opl;

  opl = opctbl + curinst->opcix;

  if (anlpass < ANLPRT) return;       // *tx;                // don't care
  if (opl->nops > 2)    return;       // *tx;                  // not 3 op instructions
  if (!curops[1].imd)   return;       // *tx;                // only allow immediate vals

  lg = -1;
  if (opl->opt & A) lg = 0;
  if (opl->opt & O) lg = 1;

  if (lg < 0) return;      // tx;

  mask = curops[1].val;

  if (get_bitsyms (snam) < 0) return;     // tx;   // no bit names found

  // names are in bit order (1 << offset);

  k = 0;		                    // first output flag
  if (opl->opt & X)
     strcpy(eq,"^=");
  else
     strcpy(eq,"=");
   // xor or standard

  if (opl->opt & A) mask = ~mask;      // reverse for AND

  if (!mask) return;          // tx;                           // no flags, so print orig

  for (i = 0; i < casz[curops[1].ssize]* 8; i++)
   {
    if (mask & 1)
      {
       if (k++)  p_indl ();    // new line and indent if not first
       if (snam[i])
          {
           pstr ("%s",snam[i]->name);   //print sym name
          }
       else
         {
          pstr ("B%d_", i);         //no name match
          p_opsc (curinst, 2);
         }
       pstr (" %s %d;",eq, lg);
      }
    mask  = mask >> 1;
   }
  *tx = empty;         //return empty;          // at least one name substituted  so return a ptr to null
 }



void shift_comment(void)
 {
  /*  do shift replace here - must be arith shift and not register */
  int j, ps;
  const OPC *opl;      // opr
  OPS *ops;

  if (anlpass < ANLPRT) return ;      // s;         // redundant ?

  ops = curops+1;
  opl = opctbl + curinst->opcix;
  
  j = 1 << ops->val;         // equiv maths divide or multiply 

  if (opl->opt & S && ops->imd)
   {
     cmntinst = curinst;  
     ps = sprintf(autocmt, "## \x2 "); 
     if (curinst->opcix < 4)
      ps += sprintf(autocmt+ps,"*");
     else  
      ps += sprintf(autocmt+ps,"/");
     ps += sprintf(autocmt+ps, "= %d ", j); 
   }
}

 /*
void do_std_ops(const char **tx)
{ // use 0x10 ?
  const OPC* opl;
  
  opl = curinst->opl;

   if (**tx == 15)  // 0xf
   {
     // replace with std type A = B + C;
     if (opl->nops == 3) p_opsc(curinst,3);
     else p_opsc (curinst,2);
     pstr(" = ");

     p_opsc (curinst,2);
     pstr(" %s ",*tx+1);
     p_opsc (curinst,1);
     pstr(";");
     *tx = empty;  
   }  
    
}
*/




void carry_src(const char **tx, int jf)
{
   int jc;                // last write op
   INST *last;            // in last instance
   int ps;                //  position
   const OPC *opc, *opl;
   
   opc = opctbl + curinst->opcix;            // mapped to curinst
 
   if (opc->sigix == 21)  return;       // not CLC, STC
   if (!(opc->opt & Y)) return;         // not carry op
 
   jc = opc->rsz3;                      // 1 if jc zero if jnc
 
   // JC or JNC to get here
 
   find_last_psw(&last);                // get last psw changer (last is always valid)
      
   if (!last)  return;              // valid one not found, or subr call etc.
         
   opl = opctbl + last->opcix;      // mapped to last inst

      if (opl->sigix == 10 || opl->sigix == 7)
        {
           pstr ("if (");          // compare or sub borrow cleared if a=b
           pstr("(uns) ");
           p_opsc(last,2); 
           if (jc != jf) pstr(" >= "); else
           pstr(" < ");               // right for JNC if no bkts            // this to swop ?
           p_opsc(last, 1);
         
        
           if (jf == 1)
             {
              pstr(" )  {");
             }
           else
             { 
              pstr(") goto ");
              p_opsc(curinst,1); 
              pstr(";  ");
             }
       //    pstr("x-x");
           *tx = empty;            // nothing to follow
        }

// these two don't really work in a maths sense....so embed a comment instead.

      if (opl->sigix == 6  || opl->sigix == 3)
        {                 //  add
     
 
           if (opl->sigix == 6)
            {  
             ps = sprintf(autocmt, "## jump if (\x2+\x1");
             if (opl->opt & Z) ps+= sprintf(autocmt+ps,"+CY");
             ps += sprintf(autocmt+ps, ")");
            } 
           
           else
           ps = sprintf(autocmt, "## jump if (\x2<<\x1)"); 
           
           cmntinst = last;
           
           ps += sprintf(autocmt+ps, jc ? " > " : " <= ");
           
           
           if (casz[last->opr[1].ssize] == 1 )
           sprintf(autocmt + ps, "0xff " );   else
           sprintf(autocmt + ps, "0xffff ");
        }
    
      if (opl->sigix == 25)
      {       // inc
          cmntinst = last;
         ps = sprintf(autocmt, "## jump if \x1"); 
          ps += sprintf(autocmt+ps, jc ? " > " : " <= ");
             if (casz[last->opr[1].ssize] == 1 )
           sprintf(autocmt + ps, "0xff " );   else
           sprintf(autocmt + ps, "0xffff ");  
          
      } 

    if (opl->sigix == 24)
      {       // dec
          cmntinst = last;
         ps = sprintf(autocmt, "## jump if \x1");
   ps += sprintf(autocmt+ps, jc ? " >= " : " < ");
           sprintf(autocmt + ps, "0 ");  


 
      } 



      

} 

int  clr (SBK *s)
  {
    curops[opctbl[curinst->opcix].wop].val = 0;
		  return 0;
  }

int neg  (SBK *s)
{
 curops[1].val = -curops[1].val;
 	  return 0;
}

int  cpl  (SBK *s)
{
 curops[1].val = (~curops[1].val) & 0xffff;
 	  return 0;
}

int  stx (SBK *s)
  {
   /* NB. this can WRITE to indexed and indirect addrs
    * but addresses not calc'ed in print phase
    */
   if (anlpass >= ANLPRT) return 0;

   curops[1].val = curops[2].val;
	  return 0;
  }

int ldx (SBK *s)
  {
 
   int ix, w;
   ix = 1;
   w = opctbl[curinst->opcix].wop;

 //  if (curinst->inr) ix = 0;

   if (w) curops[w].val = curops[ix].val;

   // SPECIAL for bank change via LDB

    if (anlpass < ANLPRT && P8065 && curops[w].addr == 0x11 && curinst->imd)
      { // This is BANK SWOP    via LDB
        // top nibble is STACK (we don't care) bottom is DATA BANK
        // may need to ADD REGISTER load
        int i;        //,j;
        i = curops[1].val & 0xf;
        //j = curops[1].val >> 4;

        //nned to check if this is register or immediate
    //    DBGPRT("%x LDX R11 Data Bank = %d, Stack Bank =%d" ,curinst->ofst, i, j);
        if (bkmap[i].bok) datbank = i << 16;
   //     else  DBGPRT("- Invalid data");
  //      if (!bkmap[j].bok) DBGPRT("- Invalid stack bank");
   //     DBGPRTN(0);

      }
  /*

  // SPECIAL for STACK move
   for (i = opl->nops; i > 0; i--)
      {
       if (curops[i].stk)        // && curops[i].reg)
          {
          if (curops[i].wsize)  DBGPRT( "*STACK WRITE at %x\n",ofst);
          else DBGPRT( "*STACK READ at %x\n",ofst);
          }
    }
 }
        */
	  return 0;
  }

int orx(SBK *s)
 {
  curops[opctbl[curinst->opcix].wop].val |=  curops[1].val;
  	  return 0;
 }

int add(SBK *s)
 {

   curops[curinst->numops].val =  curops[2].val + curops[1].val;

   	  return 0;
 }


int xand(SBK *s)
 {
   curops[curinst->numops].val =  curops[2].val & curops[1].val;
   	  return 0;
 }

 
int sub(SBK *s)
 {
  curops[curinst->numops].val =  curops[2].val - curops[1].val;
  return 0;
 }


int cmp(SBK *s)
 {
  
//   int x;
   
 //  s->nextaddr = calc_mops(xofst);
   //x =  curops[2].val - curops[1].val;      // 2 - 1 for psw set
 //  DBGPRTN("cmp %x-%x",curops[2].val,curops[1].val);
 //  set_psw(0x3e, x);         // special call here (no writes)
   
//   if (x >=0) psw |= 2; else psw &= 0x3d;      // set carry if positive clear if not
   return 0;
   
 }

int mlx(SBK *s)
 {
   curops[curinst->numops].val =  curops[2].val * curops[1].val;  // 2 and 3 ops
   return 0;
 }


 int dvx(SBK *s)
 {
  if (curops[1].val != 0)
    curops[opctbl[curinst->opcix].wop].val /=  curops[1].val;    // if 2 ops (wop = 2)
		  return 0;
 }

int shl(SBK *s)       // shifts may need more....
{
 curops[opctbl[curinst->opcix].wop].val <<=  curops[1].val;
 	  return 0;
 }

 int shr(SBK *s)
 {
  curops[opctbl[curinst->opcix].wop].val >>=  curops[1].val;
  	  return 0;
 }

int inc(SBK *s)
 {
// NOT ssize, but ALWAYS 1 (only autoinc is one or two)
//  curops[curinst->opl->wop].val++;
  //curinst->inc = 1;                  // trick as autoinc ?
	  return 0;
 }

int dec(SBK *s)
 {
 // curops[curinst->opl->wop].val--;
  	  return 0;
 }


int calc_mult_ops(int xofst)
{
  /******************* instructions M *****************
    * multiple address multiple op opcodes
    * opcsub values   0 direct  1 immediate  2 indirect  3 indexed
    * order of ops    3 ops       2 ops      1 op
    *                 dest reg    dest reg   sce reg
    *                 sce  reg    sce  reg
    *                 sce2 reg
    * NB indexed mode is SIGNED offset
    *
    *******************************************
    * Adjust cnt of opcode bytes for 'M' opcodes
    * opcsub 3 - +1 byte if int index flag set
    * Word op - opcsub 1 and 3 - +1 byte
    * Byte op - opcsub 3 - +1 byte
    ******************************************************/

     int opcsub, firstb,i;
     OPS *o;
     DDT *d;                             // TEMP

     opcsub = g_byte(xofst);
     opcsub &= 3;                           // op subtypes

     firstb = g_byte(xofst+1);             // first data byte after (true) opcode
     op_val (firstb, 1);        

     o = curops+1;                             // most action is for [1]
     
     
     switch (opcsub)
	  {
	   case 1:                                          // first op is immediate value, byte or word
         curinst->imd = 1;
         op_imd();                                      // op[1] is immediate

	     if (casz[o->ssize] == 2)
          {
           xofst++;                                    // word,  recalc
           op_val (g_word(xofst), 1);                  // recalc op1 as word
          }

          
    //     if (val_code_addr(o->addr)) 
    //     add_ddt(o->addr, o->ssize);                          // may be an address of struct...

         break;

       case 2:     // indirect, optionally with increment - add data cmd if valid
                   //not here, but indirects are ALWAYS WORD.
         curinst->inr = 1;
         if (firstb & 1)
	      {                                         // auto increment
           firstb &= 0xFE;                          // drop flag marker
           curinst->inc = 1;
           o->inc = 1;
	      }
         op_val (firstb, 1);                       // recalc [1] for inr
         o->bkt = 1;

         if (anlpass < ANLPRT)
          { // get actual values for emulation
           o->addr = g_word(o->addr);                // get address from register, always word
           if (o->addr >= PCORG) o->addr |= opbank(datbank);
           o->val = g_val(o->addr,o->ssize);  // get value from address, sized by op

           d = add_ddt(o->addr, o->ssize, xofst);
          }
	     break;


	   case 3:                                          // indexed, op0 is a fixed SIGNED offset

	     curinst->inx = 1;                             // mark op1 as indexed
         if (firstb & 1)
            {
             curops->ssize = 2;                        // offset is long (TRY unsigned word)
             firstb &= 0xFE;                             // and drop flag
            }
          else
            {
             curops->ssize = 5;                         // short, = byte offset signed
            }

         curops->off = 1;                               // op [0] is an address offset
         o->bkt = 1;
         op_val (firstb, 1); 
         
         curops->addr = g_val(xofst+2,curops->ssize);
         if (curops->addr >= PCORG) curops->addr |= opbank(datbank); 

        if (anlpass < ANLPRT)
          { // get actual values for emulation
           opcsub = o->val;
           o->addr = curops->addr+o->val;                 // get address from register, always word
           o->val = g_val(o->addr,o->ssize);  // get value from address, sized by op
           d = add_ddt(o->addr, o->ssize, xofst);
           
           if (d)
            {
            d->p1 = curops->addr;     // add_ddtx( ); with inx offset
            d->p2 = opcsub;
          }  

          }         
         opcsub = casz[curops[0].ssize];                  // size of op zero
         xofst += opcsub;
    
         break;

       default:      // direct, all ops are registers
         break;
      }

     //calc rest of ops - always register
     for (i = 2; i <= curinst->numops; i++) op_val (g_byte(xofst + i), i);

      return xofst + curinst->numops + 1; 
  }


void calc_jump_ops(int xofst, SBK *s)
  {
   /******* jumps and calls ******************
   * assume all jumps honour bnk commands , use CODE bank for jump calc
   * hold type (htype)= jump type
   *******************/
     int pcj, firstb, i;
     SBK *c;
     const OPC *opl;

     firstb = g_byte(xofst+1);             // first data byte after (true) opcode
     op_val (firstb, 1);        

     opl = opctbl +curinst->opcix;
     
     s->htype = 2;                             // default to static jump

     switch (opl->opt & (B|C|T|D|K|L))
      {
	   case B:                                     //short jump
         i = g_byte(xofst) & 7;
         i <<= 8;
         i |= firstb;
         i =  negval(i,0x400);                     // signed 11 bit value
	     break;

	   case K:                                     // skip, = pc+2 forwards
	     i = 1;
	     break;

 	   case T:                                     // bit conditional jump JB, JNB
         curops[3].bit = 1;
         curops[3].rgf = 0;
         curinst->numops++;                        // add op3 as bit number, but NO change to size.
	     op_val (g_byte(xofst) & 7, 3);
         // fall through
       case D:                                     // djnz here
         s->htype = 3;                             // conditional jump
         op_val (firstb, 2);
	     i =  g_val(xofst+2,5);                    //  signed byte
         break;

       case C:                                     // short, conditional
         s->htype = 3;                             // conditional jump
	     i = negval(firstb,0x80);
	     break;

	   case L:                                     // long jump, int
	     i = g_val(xofst+1,6);                     // signed WORD
	     s->nextaddr++;
         break;

       default:                          // ret
         s->htype = 0;                   // return
         i = -1;                         // track return to and from opcode address
         break;

	  }  // end jump type switch

     curops[1].adr = 1;
     curops[1].rgf = 0;
     pcj = s->nextaddr + i;              // offset as calc'ed from NEXT
     pcj &= 0xffff;                      // pcj allow for wrap in 16 bits
     op_val (pcj, -1);                   // jump destination in [1] + CODE bank

     // check if jump destination is a ret, or another static jump, which can shorten the scan paths
     // but ONLY if a scan exists already !!
     pcj = curops[1].addr;
     i = g_byte(pcj);
     if (i == 0xf0 || i == 0xf1)
       {
         c = (SBK*) schmem();
         c->start = pcj;

         if (find(&scanch, c))  s->htype = 0;  
       }

     curinst->nextaddr = s->nextaddr;
     if (opl->opt & X)
     {                                            // subroutine call
      curops[2].val = 0;                          // stop any other values if JLT/JLE etc
      s->htype = 6;
     }

     if (anlpass < ANLPRT)
       {
          if (!val_jmp_addr(pcj))
          {
           s->inv = 1;
  //         DBGPRTN("Invalid jump (%x) %x", pcj, cmdaddr(s->curaddr));
           end_scan (s, s->nextaddr);
          }   
        else   
         { 
          add_jump (s);
          do_sjsubr(s,opl);
          if (opl->opt & E) end_scan (s, s->nextaddr-1);
         }
      }
   }        // end of jump handling




void do_code (SBK *s)

{
     /* analyse next code statement from ofst and print operands
      * s is a scan blk, pp_code calls this with a dummy scan block in prt phase
      */

  int i, j, xofst, indx;              // used for SIGNED jump values and indexed ops
//  short firstb;                            // first first byte after operand, UNSIGNED
  OPC *opl;
  SIG *g;

  codbank = s->curaddr & 0xf0000;                // current code bank from ofst - safety

   if (s->scanned) 
  { 
 //   DBGPRTN("Already Scanned s %x c %x e %x", s->start, s->curaddr, s->end);  
    return;
  }  

  if (s->stop) 
  { 
//    DBGPRTN("Stop set s %x c %x e %x", s->start, s->curaddr, s->end);  
    return;
  }  

    if (anlpass < ANLPRT && s->curaddr >= s->end)
    {
 //      DBGPRTN("Scan = End, STOP");
       end_scan (s, 0); 
       return; 
    }


  if (!val_code_addr(s->curaddr))
    {
  //     DBGPRTN("Invalid address %x", cmdaddr(s->curaddr));
     s->inv = 1;
     s->stop = 1;
    }

 xofst = s->curaddr;   
 
   /***************************************************************
  * one instruction at a time.
  * this is both opcode analysis and print routine
  ***************************************************************/

  xofst = s->curaddr;                        // don't modify original ofst, use this instead
  s->nextaddr = s->curaddr + 1;
  i = -1;                              // used for temp bank holder

  j = g_byte(xofst);
  indx = opcind[j];                    // opcode table index
  opl = opctbl + indx;


 // add 1 plus numops.....book implies can have rombank+bank+fe as prefix !! (= 3 bytes)

   if (indx == 111  && P8065)   //   BANK SWOP
     {
      xofst++;
      i = g_byte(xofst++);              // bank number
      i &= 0xf;
      j = g_byte(xofst);                // opcode
      indx = opcind[j];
      opl = opctbl + indx;              // this could be SIGNED prefix (do next)
      s->nextaddr +=2;
     }

  if (indx == 112)
     {  // this is the SIGNED prefix index, get alt opcode if exists
      xofst++;
      j = g_byte(xofst);
      indx = opcind[j];
      opl = opctbl + indx;
      if (opl->alx)
       { indx = opl->alx;             // get prefix opcode index instead
         s->nextaddr++ ;                      // and add a byte to opcode size
       }
     }

  opl = opctbl + indx;
  
  s->nextaddr += opl->nops;                    // set opcode count (may be modded later)

  if ((opl->opt & H) && !P8065) indx = 0;

  if (!indx)
     {
       s->inv = 1;
       if (anlpass < ANLPRT)
         {

        //  wnprtn("Invalid opcode (%x) %x", j, s->curaddr);
          end_scan (s, s->nextaddr);
         }
       else  
            wnprtn("Invalid opcode at %x", s->curaddr); // report only once
         
      return;
     }

  lpix++;
  if (lpix >=PILESZ) lpix = 0;        //loop around
  curinst = instpile + lpix;
  memset(curinst,0,sizeof(INST));      // clear next entry, loop around pile if nec.

  curinst->opcix = indx;
  curinst->ofst = s->curaddr;                          // original start of opcode
  curops = curinst->opr;
  curinst->numops = opl->nops;                         // can change for bit ops etc
  curops[opl->wop].wsize = opl->wsz;                   // set write size (or zero)
  curops[1].ssize = opl->rsz1;                         // these may be changed later
  curops[2].ssize = opl->rsz2;
  curops[3].ssize = opl->rsz3;

  curops[1].rgf  = 1;                                   // all registers by default
  curops[2].rgf  = 1;
  curops[3].rgf  = 1;

  curinst->bank = codbank>>16;                             //(ofst >> 16) & 0xf;                          // codebank

  if (i >=0)
  {  // bank opcode
    if (bkmap[i].bok)
     {
      curinst->bnk  = 1;               // mark bank instruction
      curinst->bank = i;               // and keep it for this opcode 
     }
    else
     {
 //     if (anlpass < ANLPRT) DBGPRTN("bankOPC - Invalid bank %d", i);
     }
  }

// instead of signatures here, need an emulation approach to get number of arguments...

  i = get_opstart(s->curaddr);           // has this been scanned already ?

  if (i)
    { g = (SIG*) chmem(&sigch);
      g->ofst = s->curaddr;
      g = (SIG*) find(&sigch,g);         // find any sig at this address
    }  
  else
    {
     g = scan_sigs(s->curaddr);        // scan sig if not code scanned before (faster)
     set_opstart(s->curaddr);          // include prefixes in opcode flags
    }

  if (g) process_sig(g, s);              // process sig if reqd

  /* now analyse operands */

  /****** shifts (all are 2 op) *********************
  * immediate if < 16 or from register otherwise
  *******************************************/
  if (opl->opt & S)
    {
     short b;    
     b = g_byte(xofst+1);             // first data byte after (true) opcode
     op_val (b, 1);                   // 5 lowest bits only from firstb, from hbook
     if (b < 16) op_imd();            // convert op 1 to absolute (= immediate) shift value
     op_val (g_byte(xofst+2),2);
    }
  else
  if (opl->opt & R)
    { // bank swopper opcodes
              /* NOTE - according to Ford book, this only sets PSW so would be cancelled by a POPP
              (or PUSHP/POPP save/restore....) so may need a PREV_BANK */

         i = indx - 95;                   // new RAM bank
    //     rambank = i*0x100;             // establish new offset
//         DBGPRTN("New Rambank = %x", rambank);

      op_imd();     // mark operand 1 as immediate
    }
   else
   if (opl->opt & M) s->nextaddr = calc_mult_ops(xofst);
   else
   if (opl->opt & J) calc_jump_ops(xofst,s);
   else
   {  // do default opcodes as registers  
    for (i = 1; i <= opl->nops; i++)
       {
        op_val (g_byte(xofst + i), i);
       } 
   }
  

/*********************************
emulation and any opcode specials in here
*********************************/

if  (anlpass < ANLPRT)
{
  if (opl->eml) opl->eml(s); 
  upd_watch(s);
}

return;
}



/**************************************************************************
* opcode printout - calls do_code
***************************************************************************/





int pp_code (int ofst, int ix)
{
  int i, j, lwop, cnt;
  SBK s;                          // DON'T use a temp one, this is safer....
  const char  *tx ;
  INST *last;
  const OPC* opl;
 
  memset(&s,0, sizeof(SBK));     // clear it first
  s.curaddr = ofst;
  do_code(&s);
  
  if (s.inv)                            // cnt < 0
    {
     pp_hdr (ofst, opctbl->name, 1,0);   // single invalid opcode
     return s.nextaddr;
     }

// add an extra line for bank swop prefix, as there is no other way to neatly show
// indirect references correctly (i.e. [x]) with bank number

  if (curinst->bnk)
    {
     pp_hdr (ofst, "bank", 2,0);
     pstr ("%d", curinst->bank);
     s.curaddr +=2;
    } 

// NB. keep 'ofst' at original address so that symbols, vects etc work correctly
  opl = opctbl + curinst->opcix;

  cnt = s.nextaddr - s.curaddr;
  pp_hdr (s.curaddr, opl->name, cnt,0);

  i = curinst->numops;
  while (i)
     {
      p_oper (i);              // code printout
      if (--i) pchar (',');
     }

  /**********************************************
  * source code generation from here
  * check and do substitutes and cleanups, name resolution etc.
  *********************************************/

  if (PSSRC)
    {
      if (opl->sce)
       {               // skip if no pseudo source
        p_pad(cposn[2]); 

        tx = opl->sce;                    // source code, before any mods

        // any mods to source code go here

        j = find_fjump (ofst);
        
        fixup_sym_names(&tx);         // for rbases, 3 ops, names, indexed fixes
        shift_comment();              // add comment to shifts with mutiply/divide
        bitwise_replace(&tx);         // replace bit masks with flag names
        carry_src(&tx, j);            // sort out carry cases
        
     //   do_std_ops(&tx);              // shortcut std opcode


    
        if (j == 1 && *tx) 
         {
         if (opl->alx) tx = opctbl[opl->alx].sce;  // 'opposite case' source listing
         else j = 0;
        }

       //*** do psuedo source output

       while (*tx)
	    {
            
         switch (*tx)   // 1-12 used  1-31 available
          {
           case 1:
	       case 2:
	       case 3:
           	    p_opsc (curinst,*tx);
    	        break;
            

//           case 4:  not used ???

	       case 5:
            lwop = find_last_psw(&last);                 // get last psw changer 
            if (lwop > 0) p_opsc(0,1);                   // zero if not a compare
            else p_opsc(last, 1);                        // operand 1 of compare (or zero op)
            break;

	       case 6:
            lwop = find_last_psw(&last);                // get last psw changer
            if (lwop > 0) p_opsc(last,lwop);            // write op if not a compare
            else p_opsc(last,2);                        // operand 2 of compare
            break;
            
          case 7:                                       // add size or SIGN value
            i = *(tx+1);                                // get operand
            if (curops[i].sym) break;
            if (!curops[i].rgf) break;
            if (i == 1 && (curinst->inx || curinst->inr)) break;     // not for indexes or indirect
            
            if (curops[i].wsize) i = curops[i].wsize; else i = curops[i].ssize;
            if (i & 4)  pstr("S");
            i &= 3;
            if (i == 1)  pstr("Y");
            if (i == 2)  pstr("W");
            if (i == 3)  pstr("L");
            break;

         case 8:
            if (j<3) pstr ("if (");
            break;

	   case 9:

  	    switch (j)  //jump handling
             {
              case 1 :
	            pstr (" {");   // [if] cases
                break;
              case 2 :
                pstr ("return;");
                break;
              case 3:
                pchar(';');
                // drop through
              case 4:
                pstr("} else {");
                break;
              default:
                pstr ("goto ");
	            p_opsc (curinst,1);
	            pchar (';');
                break;

             }
	    break;

	   case 11:
	      p_indl();             // print new line with indent
	    break;

       case 12:
	        s.nextaddr += pp_subpars(ofst,cnt);
            break;

   //    case '=' :
   //        pstr(" = "); // not work for += etc
   //        break;
	   default:
            pchar(*tx);
	  }
      
	 tx++;
	}
      }
     }
     else      // this is for embedded arguments without 'C' option - must skip operand bytes
      if (!PSSRC && (opl->opt & J) && (opl->opt & X))
           s.nextaddr += pp_subpars(ofst, cnt);

     find_tjump (s.nextaddr);   // nextaddr ? for close bracket(s)

/*
  {
    OPS *s;                            // debug

  //  pp_comment(ofst+cnt);              // do comment first
    if (curinst->opl->nops)
     {
      p_pad(cposn[3]);                          //CDOSTART);
      pstr(" SVA ");

      for (i=curinst->numops;  i>=0; i--)
        {
         s = curops+i;
         if (s->addr + s->val)
          {
   //        if(curinst->opl->wop == i && i)  pstr("*");
           pstr("%c[",i+0x30);
           if (s->sym) pstr("%s ",s->sym->name);
           pstr("%x %x",s->val, s->addr);
//           if (!i) pstr (" T%x", curinst->tval);          //new temp value
           pstr("]%x, ",s->ssize);    
          }
        }
     }
  }                 // end dubug         */


  auto_comment();                   // print auto comment if one created

  if (opl->opt & E)    // add an extra blank line after END opcodes ...
   {
     pp_comment (s.nextaddr);           // do comment before newlines
     newl(1);

   }

   if (opl->sigix == 20) newl(1);  // add another after a ret

  return s.nextaddr;
}

int do_error(const char *err,int posn)
  {                                 // cmnd string in flbuf....
   int i; 

   wnprtn(flbuf);                 // put newline on end.
   for (i = 0; i < posn; i++) fputc(0x20, fldata.fl[2]);
   wnprt("^ ");
   wnprtn(err);
   return 1;
}



int set_rbas (CPS *c)
{
 RBT *x;
 x = add_rbase(c->p[0]&0xFF, c->p[1]);
 if (x) x->cmd = 1;              // by command
 return 0;
}



int set_opts (CPS *c)
{
  // check first, for single bank 8065
  int x;
  if (P8065) x = 1; else x = 0;
  
  cmdopts = c->p[0];
  
  if (numbanks || x)  cmdopts |= H;
  
//  DBGPRT("\nOpts = ");
//  prtflgs(cmdopts);
//  DBGPRTN(0);
//  DBGPRTN(0);
return 0;
}



void set_adnl(CPS *cmnd, LBK *z)
{           // move cadt blocks from cmnd to new LBK 

  if (!z) return;

  if (cmnd->levels <= 0 || !cmnd->adnl)
    {
 //    DBGPRTN(0);
     return;       // no addnl block
    }

  freecadt(&(z->adnl));            // free any existing CADT, probably redundant ?

  z->size = cmnd->size;

  z->adnl = cmnd->adnl;          // add chain to command blk
  cmnd->adnl = NULL;             // and drop from cmnd structure

//  #ifdef XDBG
//     DBGPRT("      with addnl params");
//     prt_cadt(z->adnl);
//     DBGPRTN(0);
//  #endif

}

void set_data_vect(LBK *blk)
{
 int ofst, end,pc;
 int bank;
 short i;
 CADT *addnl;

 if (blk && blk->fcom == C_STCT)
  {
   ofst = blk->start;
   end =  blk->end;
   bank = ofst & 0xf0000;

   while (ofst < end)
     {
      i = 0;
      addnl = blk->adnl;
 
     while (addnl && i < 32 )
       {
        if (addnl->vect && !PMANL)
         {
          pc = g_word(ofst);
          pc |= bank;                 // this assumes vect is always in same bank
          add_scan (pc, S_VSUB, 0);   // adds subr
         }
        ofst += dsize(addnl);
        i++;
        addnl = addnl->next;
       }
     }
  }

}

int set_data (CPS *c)
{
  LBK *blk;
  blk = set_cmd (c->p[0], c->p[1], c->fcom|C_CMD);  // start,end, cmnd. by cmd

  set_adnl(c,blk);
 // DBGPRTN(0);
  if (blk) 
	  {
       blk->term = c->term;                    // terminating byte flag
       blk->split = c->split;                  // split printout
	  } 
  new_sym(2,c->symname,c->p[0], -1);           // +cmd. safe if symname null
  set_data_vect(blk);
  return 0;
}


int set_sym (CPS *c)
{
  int bitno,w;
  CADT *a;

  a = c->adnl;
  bitno = -1;

  if (c->levels)
   {
    if (a->addr >= 0 && a->addr < 16) bitno = a->addr;
   }

  w = (a && a->ssize & 2);                       // W option, write symbol (via size = word)
  w +=2;                                         // with cmd flag

  new_sym(w,c->symname, c->p[0], bitno);
 return 0;
}


int set_subr (CPS *c)
{
  SXT *xsub;
  SXDT *d;
  xsub = add_subr(c->p[0]);

  if (xsub)
   {
     if (*c->symname)
        {
         new_sym(2,c->symname,c->p[0], -1);     // cmd set if sym specified
        }
     add_scan (c->p[0], S_CSUB|C_CMD, 0);

     if (c->levels <= 0 || !c->adnl) return 0;    // no addnl block

     d = get_sxdt(xsub,1);

     if (d)
      {
       xsub->cmd = 1;              // only set if extra cmds
       d->cadt = c->adnl;          // add chain to subroutine
       c->adnl = NULL;             // and drop from cmnd structure
      }
     xsub->nargs1 = c->size;   // and set sub size 
   }
  
  return 0;
 }

void set_banks(void);
void cpy_banks(void);


 int check_bank(int bk)
{
  if (bk == 0 || bk == 1 || bk == 8 || bk == 9) return 1;
  return 0;  
}

int set_ordr(CPS *cmnd)
 {
   int i,ans;
  // not allowed if single bank !!
  
  if (!numbanks)
    {
     do_error("Order IGNORED for 1 bank", cmnd->posn);
     return 1;
    }

  if (cmnd->npars != numbanks+1)
     {do_error("Num params not equal to banks detected", cmnd->posn); 
     return 1; }

  ans = 1;
  for (i = 0; i < numbanks+1; i++)
   {
    if (!check_bank(cmnd->p[i]))
       {
        do_error("Illegal Bank Number", cmnd->posn);        
        ans = 0;
       }
   }    

 if (!ans )  return 1;
            
// check bank nums first.....

//  DBGPRT("set order = %d,%d,%d,%d\n", cmnd->p[0],cmnd->p[1],cmnd->p[2],cmnd->p[3]);

  bkmap[2].dbnk = cmnd->p[0];
  bkmap[3].dbnk = cmnd->p[1];
  bkmap[4].dbnk = cmnd->p[2];
  bkmap[5].dbnk = cmnd->p[3];
  
  // and reset bok flags for copy....
  bkmap[2].bok = 1;
  bkmap[2].bok = 1;
  bkmap[2].bok = 1;
  bkmap[2].bok = 1;
  
  cpy_banks();
  set_banks();
  return 0;
 }

int set_time(CPS *c)
 {
  LBK *blk;
  int val,type,bank;
  int xofst;
  short b, m, sn;
  SYT *s;
  CADT *a;
  char *z;

  blk = set_cmd (c->p[0],c->p[1],c->fcom|C_CMD);
  set_adnl(c,blk);
 // DBGPRTN(0);
  new_sym(2,c->symname,c->p[0], -1);  // safe for null name

  // up to here is same as set_prm...now add names

  if (!blk) return 1;          // command didn't stick
  a = blk->adnl;
  if (!a) return 0;                     //  nothing extra to do
  sn = 0;                             // temp flag after SIGN removed
  if (a->addr) sn = 1;
  if (!a->name && !sn) return 0;       // nothing extra to do

  if (a->ssize < 1) a->ssize = 1;  // safety

  xofst = c->p[0];
  bank = xofst &= 0xf0000;                  //cmnd->start >> 16;
  while (xofst < maxadd(xofst))
   {
    type = g_byte (xofst++);                  // first byte - flags of time size
    if (!type) break;                         // end of list;

    val = g_val (xofst, a->ssize);            // address of counter
    val |= bank;

    if (a->name) // && (cmdopts & T)
    {
     z = nm;
     z += sprintf(z, "%s", anames[0].pname);
     z += sprintf(z, "%d", anames[0].nval++);
     if (type& 0x4)  z += sprintf(z,"D"); else z+=sprintf(z,"U");
     if (type&0x20) z += sprintf(z,"_mS");
     else
     if (type&0x40) z += sprintf(z,"_8thS");
     else
     if (type&0x80) z += sprintf(z,"_S");
     s = new_sym(0,nm,a->addr,type);     // add new (read) sym
    }

    xofst+= casz[a->ssize];                           // jump over address (word or byte)

    z = nm;
    if (type&1)      // int entry
     {
      if (s && sn)
        {                      // add flags word name too
        m = g_byte (xofst++); // bit mask
        b = 0;
        while (!(m&1)) {m /= 2; b++;}    // bit number
        val = g_byte (xofst++);    // address of bit mask
        val |= bank;
        if (type & 8) z+= sprintf(z,"Stop_"); else z += sprintf(z,"Go_");
        sprintf(z,"%s",s->name);
        new_sym(0,nm, val, b);
        }
      else     xofst+=2;                // jump over extras
     }
   }

return 0;
      // upd sym to add flags and times
 }


int set_bnk (CPS *cmnd)
{
  int bk;
  BANK *b;

   
   bk = cmnd->p[0];          // first param
   if (!check_bank(bk)) {do_error("Illegal Bank Number", cmnd->posn); return 1;} 
   
   if (bkmap[bk].cmd)
   {
     do_error("Bank already defined",cmnd->posn);
     return 1;
   }

   if (cmnd->p[1] < 0 || cmnd->p[1] > fldata.fillen) {do_error("Illegal File Offset", cmnd->posn); return 1;}
   
   if (cmnd->p[2] - cmnd->p[1] > 0xdfff) {do_error("start->end gap too large", cmnd->posn); return 1;}
   
   
   if (bkmap[2].btype)
   { 
   // kill all banks ready for new commands
   memset(bkmap,0,sizeof(bkmap));
   bkmap[0].opbt = opbit;                   // bit flag array reset
   bkmap[1].opbt = opbit + 0x800;           // = 32 bit   0x1000 = 16 bit ints
   bkmap[8].opbt = opbit + 0x1000;
   bkmap[9].opbt = opbit + 0x1800;
   numbanks = -1;          
   }
   
   // now add one bank

   b = bkmap+bk;
    
   b->fbuf = ibuf + cmnd->p[1];          // file offset
   b->filstrt = cmnd->p[1];
   b->minpc = PCORG;
   b->maxbk = cmnd->p[2]-PCORG;                  //
   if (cmnd->p[3])  b->maxpc = cmnd->p[3];

   if (cmnd->p[3] < PCORG || cmnd->p[3] > b->maxbk)
   {  do_error("fill start outside start and end", cmnd->posn); return 1;}
   
   b->cmd = 1;
   b->bok = 1;

   numbanks++; 


   set_banks();              // on final bank layout, not 2,3,4,5  no harm called for each bank.....
 

//   DBGPRT("Bank Set %d %x %x ",cmnd->p[1], cmnd->p[2],cmnd->p[3]);
 //  DBGPRTN(" #( = %x - %x fill from %x)",b->minpc, b->maxbk, b->maxpc);
  
  return 0;
 }


int set_vect (CPS *c)
{                          // assumes vect subrs in same bank as pointers
  int i, ofst,bank;
  LBK *blk;
  CADT *a;

  a = c->adnl;
  blk = set_cmd (c->p[0],c->p[1], C_VECT|C_CMD);   // by cmd
//  DBGPRTN(0);
  if (PMANL) return 0;
  set_adnl(c,blk);

  bank = c->p[0] & 0xf0000;

  if (a && a->foff && bkmap[a->addr].bok) bank = a->addr << 16;

  for (i = c->p[0]; i < c->p[1]; i += 2)
    {
      ofst = g_word (i);
      ofst |= bank;
      add_scan (ofst, S_VSUB, 0);      // adds subr
    }
    
    return 0;
}

int set_code (CPS *c)
{
  set_cmd (c->p[0], c->p[1], c->fcom|C_CMD);   // by cmd
 // DBGPRTN(0);
  return 0;
}

int set_scan (CPS *c)
{
  add_scan (c->p[0], S_SJMP|C_CMD, 0);
  return 0;
}


int set_args(CPS *c)
{
  LBK *blk;
  blk = set_auxcmd (c->p[0], c->p[1], c->fcom|C_CMD);  // by cmd
  set_adnl(c,blk);
  set_data_vect(blk);
//  DBGPRTN(0);
  return 0;
}

int set_cdih (CPS *c)
{
   set_auxcmd(c->p[0],c->p[1],c->fcom|C_CMD);
   return 0;
}



/***************************************************************
 *  do last phase listing
 *
 *  loop within loop for faster speed
 ***************************************************************/


 void do_listing (void)
{
  int ofst;
  short i, ix;
  LBK *x, *y;
  BANK *b;
  anlpass = ANLPRT;                   // safety
  getcmnt(&cmntofst, &cmtt);          // initialise comments
  *autocmt = 0;
  cmntinst = 0;

  // print full bank profile first ??
  newl(1);
  fprintf (fldata.fl[1], "####################################################################################################");
  newl(1);
  fprintf (fldata.fl[1],"#\n# Disassembly listing of binary '%s' ",fldata.bare); 
  if (P8065) pstr("(8065, %d banks)",numbanks+1 ); else pstr("(8061, 1 bank)");
  fprintf (fldata.fl[1], " by SAD Version %s", SADVERSION);
  fprintf (fldata.fl[1],"\n# See '%s%s' for warnings, results other info", fldata.bare, fd[2].suffix);
  fprintf (fldata.fl[1],"\n# Key  R general register, L long (4 bytes), W word (2 bytes), Y byte");
  fprintf (fldata.fl[1],"\n#      S signed, (Unsigned is default)");
  fprintf (fldata.fl[1],"\n#      extras shown where opcode has mixed sizes (e.g DIV"); 
  fprintf (fldata.fl[1],"\n#      [ ] = address of (word), '++' means increment register after operation" );
  fprintf (fldata.fl[1],"\n# Processor Status flags (for conditional jumps)" );
  fprintf (fldata.fl[1],"\n#     CY = carry, STC = sticky, OVF = overflow, OVT = overflow trap\n");
  fprintf (fldata.fl[1],"####################################################################################################");
  newl(2);



 for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;

     if (!b->bok) continue;

     if (numbanks)
      {
       newl(2);
       pstr ("########################################################################");
       newl(1);
       pstr ("# Bank %d  file offset %x-%x, (%x - %x)", i, b->filstrt, b->filend, b->minpc, b->maxbk);
       newl(1);
       pstr ("########################################################################");
       newl(2);
      }

     for (ofst = b->minpc; ofst < b->maxbk; )
      {
                // first, find command for this offset
       y = (LBK*) schmem();            //x = NULL;
       y->start = ofst;
        
       ix = bfindix(&cmdch,y); 

       x = (LBK*) cmdch.ptrs[ix];

       if (ix >= cmdch.num || cmdch.equal(ix,y))
         {                            // NOT found, set up a dummy block
         x = (LBK*) chmem(&cmdch);
         x->start = ofst;
         x->end   = b->maxbk;   // safety catch
         x->fcom  = C_DFLT;

         if (ix < cmdch.num)
             {                     // set end up to next block - should be redundant....
              y = (LBK*) cmdch.ptrs[ix];
              if (x->end >= y->start) x->end = y->start -1;
             }
          ix = cmdch.num;
         }

       if (x->start > x->end)          // safety check
         {
   //       PBKN(x, "Invalid cmd/order - set 1");
          x->end = x->start+1;
          ofst++;
          continue;
         }

        memset(curops,0, sizeof(OPS)*4);               // clear all ops

       while (ofst <= x->end)
         {
          show_prog (anlpass);
          pp_comment(ofst);                            // before, and any auto ones
          ofst = dirs[x->fcom].prtz (ofst, ix);
          if (!val_data_addr(ofst)) break;             // safety - probably redundant
          pp_comment (ofst);                           // after
         }
       newl(2);                  // add blank lines after each command

      }      // ofst
    }        // bank
}




void scan_blk(SBK *blk)
 {

  blk->stop = 0;                           // just in case...
  put_scanpars(blk);                       // restore saved values

 while (!blk->stop)
  {
   do_code (blk);                         // do op decode first to get curinst, codebank, etc
   if (blk->hblk) break;                  // scan block is on HOLD (in do_code)
   blk->curaddr = blk->nextaddr;              // next opcode
  }


}


void scan_all (void)
{
  SBK *s;
  int ix, nx;   // nx to check if scans changed

  ix = 0;

  while (ix < scanch.num)
   {
    nx = scanch.num;             // save no of scans now

    s = (SBK*) scanch.ptrs[ix];

    if (!s->scanned && !s->hblk)
      {
       if (anlpass >= ANLPRT) newl(1);                // new line for new block print
       show_prog (anlpass);

 //      DBGPRTN(0);
    //   if (s->tst) DBGPRT("TEST ");
//       wnprt("SCAN %x st=%x %d=%s r=%x",s->curaddr, s->start,
//               s->type,jptxt[s->type],s->retaddr);
   //       prt_scanpars(s);
 //      DBGPRTN(0);

     
       if (PLABEL)
        {          // static or cond jump, add auto label name (goto)
        if (s->type == 2 || s->type == 3)  add_nsym (2, s->start);
        }
        
       scan_blk(s);
       
       if (scanch.num > nx) ix = -1;               // scan added somewhere, restart loop
       if (!s->hblk) nx = unhold_scans (s);        // unlock any held scan blocks, return lowest
       if (nx < ix)    ix = nx-1;                  // reset to lowest unheld block
       
       tscans++;

     if (tscans > scanch.num * 16)               // loop safety check
       {
        wnprtn("\n *********");   
        wnprtn(" abandoned - too many scans - a loop somewhere ?");
        wnprtn("\n *********"); 
        printf("FAILED - too many scan loops\n");
        break;
       }
    }
   ix++;
   }
 }


/* find functions */

void incpc (int *ofst, short inc)
{
  *ofst += inc;
  if (*ofst >= maxadd(*ofst))
    {
   //  DBGPRTN("increment outside ROM [%x] ignored", *ofst);
    }
}



void set_tab(int ix)
{

 /* sort out table to full size.
 * doesnt work if table abuts large FILL area !!
 * must check next entry starts on table row boundary....
 * row size in adnl->cnt
 * single word skip was for errant addresses which appear in multiplies etc...
 */

  int cols, ofst, rows, xend;
  DDT *d, *n;
  LBK *blk;
  CADT *a;
  SXT *sub;
  
  //int min, max, mode;                  // average change value
 
 // char *fvals;                 // first row;
//  int fcol[32];                  // first column
 // int 
  int sval, eval, dif, sign, cmax;
//  int dir, dif, cnt;
  
// probably need a maxend safety here....., and a sign check....


  d = (DDT*) datch.ptrs[ix];

  cols = d->p1;                   // cols size;
  ofst = d->start;
  rows = 0;
  ix++;

  n = (DDT*) datch.ptrs[ix];    // next data entry
   
  while (ofst < maxadd(ofst))
    {
     ofst+=cols;                // row by row
     rows++;                    // row count
     
     if (ofst >= n->start)
      {
       if (n->tab || n->func || n->p1 > 1)
         {
           if (ofst > n->start)  rows--;
           break;
           }     // fits to next struct
       else
       {
        // decide if this can be skipped
        if (!n->tab && !n->func) 
        {        // ignore - MORE PROBABLY REQUIRED 
         ix++;
         n = (DDT*) datch.ptrs[ix];    // next data entry
        }
       } 
      }
    }  

// need to fix ofst up to whole rows and cols in case of a filler byte...

xend = d->start;
xend += (rows*cols) -1;


// now try to work out if its signed.....................
// unlike funcs, tables are 2D SURFACES so the half scale check probably works
// simple check for now....


sign = 0;                // set for signed
ofst = d->start;

while (ofst <= xend)
 {
  cmax = ofst+cols;             // row end 
  sval = g_byte(ofst++);       // first byte of row;  
  while (ofst <  cmax)
    {   // per row
     eval = g_byte(ofst++);          // next byte of row;
     dif = eval-sval;
     if (dif > 200 || dif < -200) sign = 1;
     sval = eval; 
    }
 }


//  DBGPRT("DDT TAB %x, cols %x rows %x ", d->start, d->p1, rows);
//  if (d->ssize & 4) DBGPRT(" TSO ");
//  DBGPRTN(0);


if (!d->cmd)
  {  
   if (!sign) d->ssize &= 3;
   if (sign)  d->ssize |= 4;
  } 


sub = (SXT*) schblk;
sub->addr = d->from;

sub = (SXT*) find(&subch, sub);

sval = 13;   // main name (U)
if (sign) sval += 1;                       // U to S

if (sub && !sub->cmd)
{
  if (sval > sub->xnam)
     {      // may need more checks than this 
       sub->xnam = sval;
     }

}

 

 
  blk = set_cmd(d->start, xend, C_TABLE);


     if (blk && !blk->cmd)                // in case tab by user cmd....
       {
        add_nsym(4,d->start);                  // auto table name, can fail
        a = add_cadt(&(blk->adnl)); //,0);    // add at front
        blk->size = d->p1;                // size for one line ( = cols)
        a->cnt = d->p1;
        a->dcm = 1;                       // default to decimal (PDCM)? 1: 0;
        a->ssize = d->ssize;
       }

       // probably need some more checks, but simple version works for now...
 
}


int set_stval(int size, int *sval, int *nval)
{

  if (size == 1)
    {                      // unsigned byte
      *sval = 0xff;
      *nval = 0;
     return 1;          // ok   
    }

  if (size == 2)
    {  
      *sval = 0xffff;
      *nval = 0;
      return 1;          //ok
    }
    
  if (size == 5)
    {  
     *sval = 0x7f;        // signed byte
     *nval = 0x80;  
     return 1;
    }
    
  if (size == 6) 
    {      
     *sval = 0x7fff;
     *nval = 0x8000;
     return 1;
    }

  return 0;
  }   


void set_func(int ix)
{
 /*  extend functions to true 'end'.  Funcs should be
 *  unsigned in - start 0xffff or 0xff,  end 0    (word,byte)
 *  signed in   - start 0x7fff or 0x7f,  end 0x8000 or 0x80
 *  Output - check sign crossover with up or down change.
 *  not perfect. Less hassle to do this all unsigned...
 * 
 * 
 * 
 * CLOSEST FIT ?   add differences for signed/unsigned and compare ?
 * 
 */


  int sval, nval, val3, xofst, xend, maxend, err;
 
  int cnt, osze, isze, odir;        // start and end values, and sign bit
  int incr, esize;          // increment, entry (read) size for g_val

  CADT *a;
  LBK *blk;
  DDT *d, *n;
  SXT *sub;


  d = (DDT*) datch.ptrs[ix];
 // DBGPRT("DDT FUNC %x, sz %x %x", d->start, d->ssize, d->p1);
 
  maxend = maxadd(d->start);

 
  // get right sign first.....

  xofst = d->start;
  isze = d->ssize & 3;                     // start as unsigned
 
  val3 = g_val(xofst, isze);         // read first input value (as unsigned)


  if (!set_stval(isze, &sval, &nval)) { wnprt("invalid size in func"); return ;}

  if (val3 != sval)  // try alternate sign....
    {
     isze ^= 4;          // swop sign flag
 //    wnprtn("Swop sign in - val %x (%x)", val3, sval);
     set_stval(isze, &sval, &nval);  // and try again
          
     if (val3 != sval)
      {
        wnprtn("Func %x Invalid start value %x", d->start, val3);
        return;
      }
    }
    
  // val3 = first row

 esize = isze & 3;
  n = (DDT*) datch.ptrs[ix+1];  // next cmd

 // esize &= 3;             // drop sign
  xend  = 0;
  cnt = 0;
  err = 0;
  incr = casz[esize] *2;                  // whole row increment

  while (xofst += incr)                   // check input value of each row (UNSIGNED)
    {
     if (xofst > maxend) err = 3;
     val3 = g_val(xofst, esize);          // next input value

     if (xend)
       {           // track WHOLE ROW from end value
        val3 = g_val(xofst, esize+1);     // get whole row
        if (val3 != sval) {err=4; break;}
        cnt++;
        if (cnt > 32) {err=4; break;}          // safety
        if (xofst >= n->start)
           { if (!val3 || n->func || n->tab){err=5; break;}}  // and check for next block (any if zero)
        xend = xofst+incr-1;                   // update end  
       }
     else
     if (val3 == nval)
       { 
        xend = xofst+incr-1;                        // hit end value, set end
        sval = g_val(xofst, esize+1);               // get whole row for next check
       }
     else
       {
        if (xend) break;                            // have run over struct end
        if (val3 == sval) cnt++; else cnt = 0;      // count any matching rows
        if (cnt > 16) err = 2;                       // 8 identical data rows - STOP
        sval = val3;                                // save this value for next row check
       }

     if (err)
       {
  //      DBGPRT(" err=%d ", err); 
        if (!xend) xend = xofst-1;
        break;}
    }          // while

  // check for SIGN in output
  
  
  // can do a 'consistent direction' check ? signed would have to go through zero

 osze = esize;                       // uns size for output
 odir = 0;
 xofst = d->start + casz[esize];     // start at first OUTPUT value
 nval = g_val(xofst, esize);         // read first input value (as unsigned)

 val3 = g_val(xend-esize+1, esize);          // last value


 if (esize == 1) sval = 0x80 ; else sval = 0x8000;    // sign bit...
 
 while (xofst += incr)                   // check input value of each row (UNSIGNED)
    {
      if (xofst > xend) break;           // end of struct  
     val3 = g_val(xofst, esize);          // next output value
     if (!odir) 
       { 
         if (nval > val3) odir = 1;         // down
         if (nval < val3) odir = 2;         // up
       }  
   
     if ((nval ^ val3) & sval)          // differ by sign bit (ignore zero...)
      {
        if (!val3)
         {       // check if zero crossed !! definite signed if so....
           while (!val3)
               {
                   xofst += incr;
                   if (xofst > xend) {sval = 0; break;}  // no sign flip
                   val3 = g_val(xofst, esize);    // skip any more zeroes !!
               }
               
           if ((nval ^ val3) & sval)
            {
             osze |=4;        // DEFINITELY signed 
             break;
            }
         } 
        else
         {                   // exclude zero
           if ((odir == 1 &&  val3 > nval) ||
               (odir == 2 &&  val3 < nval)) 
                 {          // reversed direction with sign bit (no zero)
                        // check if direction consistent with sign set ??
 
       //   wnprt("z");
                  osze |=4;        // signed ? is this enough ? 
                  break;
                 }
             
         }
      }
     nval = val3;                    // update last value 
    }
    
//  if (xend > n->start && (n->func || n->tab) ) DBGPRT("olap next %x", n->start);

//  wnprtn("start %x, end %x, err %d : si %x so %x", d->start, xend, err, d->ssize, osze);


if (d->cmd)
{  // user defined - restore originals
    isze = d->ssize;
    osze = d->p2;
}

sub = (SXT*) schblk;
sub->addr = d->from;

sub = (SXT*) find(&subch, sub);


if ((isze &3) < 2) val3 = 5; else val3 = 9;    // main name (UU size)
if (isze & 4) val3 += 2;                       // UU to SU
if (osze & 4) val3 += 1;                       // UU to SU


if (sub && !sub->cmd)
{
  if (val3 > sub->xnam)
     {      // may need more checks than this 
       sub->xnam = val3;
     }

}


// OK now add function cmd

    blk = set_cmd (d->start, xend, C_FUNC);

    if (blk && !blk->cmd)
      {                                 // cmd OK, add size etc (2 sets for func)

       add_nsym(3,d->start);            // auto func name, can fail
       blk->size = incr;               // size of one row

       a = add_cadt(&(blk->adnl)); //,0);                       // at front
       if (a) add_cadt(&(blk->adnl)); //,1);                    // at end
       a->cnt = 1;
       a->ssize = isze;                         // size, word or byte
       a->dcm = 1;                              // default to decimal print

       a->next->cnt = 1;
       a->next->ssize = osze;                    // size out
       a->next->dcm = 1;                        // default to decimal
      }

}

void set_ddt(int ix)
{

DDT *d;
//LBK *blk;

int sz;

d = (DDT*) datch.ptrs[ix];

if (d->psh) return;                       //ignore pshed addresses (code)

sz = casz[d->ssize];

if (sz == 1)
{

set_cmd (d->start, d->start, C_BYTE);


}

if (sz == 2)
{

set_cmd (d->start, d->start+1, C_WORD);

}


/*

if (d->ssize  & 3)


blk = set_cmd (d->start, xend, C_FUNC, 0);

    if (blk && !blk->cmd)
      {                                 // cmd OK, add size etc (2 sets for func)

       add_nsym(3,d->start);            // auto func name, can fail
       blk->size1 = incr;               // size of one row

       a = add_cadt(&(blk->addnl),0);                       // at front
       if (a) add_cadt(&(blk->addnl),1);                    // at end
       a->cnt = 1;
       a->ssize = d->ssize;                    // size, word or byte
       a->dcm = 1;                              // default to decimal print

       a->next->cnt = 1;
       a->next->ssize = d->sz2;              // size
       a->next->dcm = 1;                        // default to decimal
      }

}


*/


}









 void do_ddt(void)
 {
  int ix;
  DDT *d;

  if (PMANL) return;

 // DBGPRTN("do_ddt");
  for (ix = 0; ix < datch.num; ix++)
   {
    d = (DDT*) datch.ptrs[ix];  // this func
    if (!d->dis)
    {  

    if (d->tab) set_tab (ix);
    else
    if (d->func)  set_func(ix);
    else set_ddt(ix);
    }
   }
 }




//********** command processing section **************







void DBG_sigs(CHAIN *x)
{
 int i, ix;
 SIG *y;

 wnprtn("\n-----------Signatures---------[%d]--", x->num);
 wnprtn("d ofst  end    k    name      pars" );

 for (ix = 0; ix < x->num; ix++)
   {
     y = (SIG*) x->ptrs[ix];
     wnprt("%d %5x %5x %2d %7s  ", y->done, y->ofst, y->xend, y->skp, y->ptn->name);
     for (i = 0; i < NSGV; i++) wnprt("%5x," , y->v[i]);
     wnprtn(0);
    }

 wnprtn(0);
}





//*********************************************************************************


void add_iname(short ix, int ofst)
{
uint i, x;
char *z;

 // ignore_int can be at the end of a jump in multibanks....

 if (!PINTF) return;

 ix -= 0x10;
 ix /= 2;          // get correct index for table lookup


 x  = (P8065) ? 1 : 0;
 z = nm;

 if (numbanks)               // sort out name
   {
    i = ofst >> 16;             // add bank number after the I
    z += sprintf(nm, "I%d_",i);
   }
 else
  z += sprintf(nm, "I_");

 if (do_sig(&intign, ofst))  // this is 'ignore int' signature, use 'ignore' name
    {
      sprintf(z,"Ignore");
      new_sym(0,nm, ofst, -1);
      return; // nm;
    }

 for (i = 0; i < NC(inames); i++)
    {
    if (ix <= inames[i].end && ix >= inames[i].start && inames[i].p5 == x)
       {
       z += sprintf(z,"%s",inames[i].name);                      // number flag set
       if (inames[i].num) z+=sprintf(z, "%d", ix-inames[i].start);
       new_sym(0,nm, ofst, -1);
       break;    // stop loop once found for single calls
       }
    }
return;
 }




/*******************command stuff ***************************************************/





int readpunc(int posn, int end)
{
 while (posn <= end && (flbuf[posn] == ' ' || flbuf[posn] == ',')) posn++;
 return posn;
 }



int getp(int *v, int *posn)
 {
  int ans, rlen;   
  ans = sscanf(flbuf+*posn, "%5x%n %5x%n %5x%n %5x%n", v, &rlen, v+1, &rlen, v+2, &rlen, v+3, &rlen);
  if (ans) *posn += rlen;
  return ans;
 }




int chk_startaddr(CPS *c, int ix)
 {
   // check ans fixup any address issues (bank etc) 
  int start, end;

  if (ix <= 0) return 0;
  
  start = c->p[ix-1];
  end   = c->p[ix];
     
  // autofix single banks to bank 8 
  if (!numbanks && (start <= 0xffff && start >= PCORG)) start |= 0x80000;

  if (!end)  end = start;

  if (end <= 0xffff) end |= (start & 0xf0000);      // add start bank

  if ((end & 0xf0000) != (start & 0xf0000))  {do_error("Banks don't match",c->posn);return 8;}   // banks don't match
  
  if ( end < start) {do_error("End less than Start",c->posn);return 8;}   // banks don't match
    
  c->p[ix-1] = start;
  c->p[ix]   = end;  

 return 0;      
 }



int do_adnl_letter (CPS* cmnd, char optl)

{
     CADT *c;
     int ans, rlen;
     int v[4];
     float f1;
     SIG * z;     
     // switch by command letter - moved out of parse loop for clarity
 
     c = cmnd->adnl;
     z = 0;                // safety....

     while(c && c->next) c = c->next;          // make sure at end of chain
    
	 if (!c && optl != ':') return do_error("Missing Colon", cmnd->posn);
	
     switch(optl)
          {
           // case 'A':                    // AD voltage ?? / RPM/ IOtime....
           //    cadd->volt = 1;
           //    break;

           // case 'B' :
           //    cadd->f.foff = 1;
           //    cadd->divisor  = ans;    // Byte + fixed address offset (funcs)
           //    break;

           // case 'C' :
           //    break;
		   
           case '|' :
		      c = add_cadt(&(cmnd->adnl));
	          cmnd->levels++;
              c->split = 1;                    // mark split printout here
			  cmnd->split = 1;
              break;		   
		   
		   case ':' :
 
              // colon add a new CADT struct (= level) at end of chain
              c = add_cadt(&(cmnd->adnl));
	          cmnd->levels++;
              break;
   
			  
           case 'D' :

             // 1 hex value (address offset)
             ans = sscanf(flbuf+cmnd->posn, "%5x%n ", v, &rlen);
             if (!ans) return do_error("Param reqd", cmnd->posn);
             cmnd->posn+=rlen;
             c->foff = 1;
             c->addr  = v[0];    // Word + fixed address offset (funcs) - bank swop vect
             break;


           case 'E' :
             // ENCODED 2 hex vals
             ans = sscanf(flbuf+cmnd->posn, "%5x%n %5x%n", v, &rlen, v+1, &rlen);
             if (ans != 2 )  return do_error("2 Params reqd", cmnd->posn);
              
             cmnd->posn+=rlen;
             c->enc  = v[0]; 
             c->addr = v[1];
             c->ssize = 2;   // encoded, implies word
              
             break;

           case 'F' :
               // special subr function read but ignored - need to put back
               // f 1 <reg1>   <size+sign> <sign>    func
               // f 2 <reg1> <reg2>  <size+sign>    tab

               // f 3 <reg1> <size>           variable ?
               // f 4 <?>     <size1> < size2>  switch between 2 sizes args ?
               
               // cmnd.fcom to check ..

               ans = sscanf(flbuf+cmnd->posn, "%5x%n", v, &rlen);
                  
               if (ans != 1)   return do_error("Invalid Func type", cmnd->posn);
               cmnd->posn+=rlen;
               if (v[0] < 1 || v[0] > 2) return do_error("Invalid Func type", cmnd->posn);
                { 
                        //    add a SIG here to do the job
                  c->disb = 1;                          // disable this cadt entry
                  cmnd->z = (SIG *) chmem(&sigch);
                  z =  cmnd->z;
                  z->ofst = cmnd->p[0];
                  z->xend = cmnd->p[0];        // start and end address of subr
                  z->v[0] = 0x100000;          // special address as marker
                  z->v[14] = 1;
                  z->v[15] = 1;               // unsigned
               
               if (v[0] == 1)
                {
                 ans = sscanf(flbuf+cmnd->posn, "%5x%n", v, &rlen);
                 if (ans != 1)   return do_error("Invalid address register", cmnd->posn);
                 cmnd->posn+=rlen;   
                    
                 z->ptn = &fnlp ;           // function
                 z->v[2] = 1;                // default to byte
                 z->v[4] = v[0];            // register for address
                } 
               else
                {                           // table  
                 ans = sscanf(flbuf+cmnd->posn, "%5x%n %5x%n", v, &rlen, v+1, &rlen);
                 // error ?
                 cmnd->posn+=rlen;
                 z->ptn = &tblp;
                 z->v[4] = v[0];             // address register
                 z->v[3] = v[1];             // tab column register

             
                }
              inssig(z);            // insert sig into chain
  
             }
             break;

  // G H I J K

           case 'L':                // long int
             c->ssize &= 4;        // keep sign
             c->ssize |= 3;
             break;

 // case 'M':                // mask pair ?, use esize to decide word or byte
 //    cadd->mask = 1;
 //    break;

           case 'N':
             c->name = 1;       // Name lookup
             break;

           case 'O':                // Cols or count
             // 1 decimal value

             ans = sscanf(flbuf+cmnd->posn, "%d%n ", v, &rlen);
             if (ans>0)
               {
                 cmnd->posn+=rlen;
                 c->cnt = v[0];            // must be positive ?
               }     else return do_error("Param reqd", cmnd->posn);  
             break;

           case 'P':               // Print fieldwidth   1 decimal
             ans = sscanf(flbuf+cmnd->posn, "%5d%n ", v, &rlen);
             if (ans>0){ cmnd->posn+=rlen;
             c->pfw = v[0];
               }     else return do_error("Param reqd", cmnd->posn);
              break;

           case 'Q' :              // terminator byte
             cmnd->term = 1;
             break;

           case 'R':               // indirect pointer to subroutine (= vect)
             c->ssize = 2;        // safety, lock to WORD
             c->vect = 1;
             c->name = 1;         // with name by default
             break;

           case 'S':              // Signed
             c->ssize |= 4;
             if (cmnd->z)
                {
                  z = cmnd->z;  
                  if (z->v[1]) z->v[14] = 2;   // signed out
                  else
                    { z->v[15] = 2; // signed in
                     if (z->ptn == &fnlp) z->v[1] = 1;
                    }
  
                } 
             break;

           case 'T':                 // biT
               // read one decimal val
             ans = sscanf(flbuf+cmnd->posn, "%5d%n ", v, &rlen);
             if (ans > 0) {
               cmnd->posn+=rlen;
               c->addr = v[0];
               }    else return do_error("Param reqd", cmnd->posn);
             break;

           case  'U' :  //not used , except for funcs (default uns)
              c->ssize &= 3;         // clear sign
              if (cmnd->z)
               {
                 z = cmnd->z;  
                 if (z->v[1])  z->v[14] = 1;   // unsigned out
                 else
                     {z->v[15] = 1; // unsigned out
                      if (z->ptn == &fnlp) z->v[1] = 1;
                     }
                }   
                     
              break;

           case 'V':                  // diVisor
             // read one FLOAT val
             ans = sscanf(flbuf+cmnd->posn, "%f%n ", &f1, &rlen);       // FLOAT
             if (ans > 0)
             {cmnd->posn+=rlen;
               c->divd = f1;                                       // now go FULL FLOAT
             }     else return do_error("Param reqd", cmnd->posn);
             break;

           case 'W':                 // Word   - or write for syms     /
             c->ssize &= 4;          // keep sign
             c->ssize |= 2;
             if (cmnd->z)  cmnd->z->v[2] = 4;      // set size of func to word
             break;

           case 'X':                 // Print in decimal /
             c->dcm = 1;
             break;

           case 'Y':                   // bYte/
             c->ssize &= 4;           // keep sign, but ditch rest
             c->ssize |= 1;
             if (cmnd->z)  cmnd->z->v[2] = 2;      // set size of func to byte
             break;

         // Z
 
           default:
              return do_error("Illegal Option", cmnd->posn);

          }          // end switch
 return 0;
    }   



void  do_opt_letter (CPS* cmnd, char optl)
{
    // this is a  cmd: ABCD type (no params)
    
    int j;
    
    if (optl >= 'A' && optl <= 'Z')
	  {
       j = optl - 'A';                      // convert letter to bit offset
       cmnd->p[0] |= (1 << j) ;
	 }
 }



int parse_com(char *flbuf)
{
  // parse (and check) cmnd return error(s) as bit flags;

  uint j;
  int maxlen,rlen,ans;

  char rdst[32];
  char *t, *e;                                  // where we are up to

  DIRS* crdir;
  CPS  cmnd;                                            // for parse command
  CADT *c;

  memset(&cmnd, 0, sizeof(CPS));                       // clear struct
  c = 0;
  
  crdir = NULL;
  maxlen = strlen(flbuf);    // safety check


// ans = sscanf(flbuf, "[\n\r#]");      // alternate method ?
// if (ans)  flbuf[end] = '\0';

  e = flbuf + maxlen;

  t = strchr(flbuf, '\n');            // check for end of line
  if (t && t < e) e = t;                // remove it

  t = strchr(flbuf, '\r');            // check for end of line
  if (t && t < e) e = t;                 // remove it

  t = strchr(flbuf, '#');            // stop at comment
  if (t && t < e) e = t;                  // remove any comment

  maxlen = e - flbuf;
  
  if (maxlen <=0) return 0;                // null line;

  cmnd.posn = 0;

  cmnd.posn = readpunc(cmnd.posn,maxlen);
  ans = sscanf (flbuf, "%7s%n", rdst, &rlen);    // n is no of chars read, to %n

  if (ans <= 0) return 0;                // no command - null line;

  // get command

  for (j = 0; j < NC(dirs); j++)
   {
    if (!strncasecmp (dirs[j].com, rdst, 3) )      // 3 chars min
      {
       cmnd.fcom = j;                 // found
       crdir = dirs+j;                // ptr to command entry
       break;
      }
   }
  cmnd.posn += rlen;
  if (!crdir)  return do_error("Invalid Command",cmnd.posn);             // cmnd not found


  cmnd.posn = readpunc(cmnd.posn, maxlen);


  if (crdir->maxps)       // read up to 4 following addresses into p (if any)
    {
     ans = getp(cmnd.p, &cmnd.posn);

     if (ans < 1) return do_error("Parameters Required",cmnd.posn);
     if (ans > crdir->maxps) return do_error("Too many Parameters",cmnd.posn);
     cmnd.npars = ans;
     
     // verify addresses here - change to allow single start for everything....

     ans = chk_startaddr(&cmnd, crdir->spos);
 
     if (ans) return ans;
    }
    
  // possible name here

  cmnd.posn = readpunc(cmnd.posn, maxlen);

  if (crdir->namee && cmnd.posn < maxlen &&  flbuf[cmnd.posn] == '\"')
   {
    cmnd.posn++;   // skip quote
    ans = sscanf(flbuf+cmnd.posn," %32s%n", cmnd.symname,&rlen);     //max 32 char names.
    if (ans) cmnd.posn += rlen;
    
    t = strchr(cmnd.symname, '\"');                 // drop trailing quote from syname
    if (t) *t = 0;
	else return do_error("Missing close quote", cmnd.posn);
   }

  // now read levels


  if (crdir->opts) 
   {        // if no opts, skip this
      
     // read option chars and level markers whilst data left

      while (cmnd.posn < maxlen)
       {
        cmnd.posn = readpunc(cmnd.posn,maxlen);
        if (cmnd.posn >= maxlen) break;         // safety
     
        ans = sscanf(flbuf+cmnd.posn,crdir->opts, rdst, &rlen);
        t = flbuf+cmnd.posn;                                               // remember char
	
        if (ans <= 0) return do_error("Illegal Option", cmnd.posn);        // illegal option char (4)
   
	    cmnd.posn += 1;                                                    // Not rlen, only single char here
    
        if (!crdir->maxps) do_opt_letter(&cmnd, rdst[0]);                  // cmd : ABC  style
        else
         {
          if (do_adnl_letter (&cmnd, rdst[0]))  return 1;
         } 
         
      }      //  end get more options (colon levels

   }  // end if options/adnl

 
  c = cmnd.adnl;

  while(c)
   {
    cmnd.size += dsize(c);   // set final size
    if (cmnd.fcom >= C_TABLE && cmnd.fcom <= C_FUNC) c->dcm = 1;  // decimal default
    c = c->next;
   }

  c = cmnd.adnl;

   if (cmnd.fcom == C_FUNC &&  cmnd.levels  < 2)    // func must always have 2 levels
   { c->next = (CADT *) mem (0,0,sizeof(CADT));
     *(c->next) = *c;        // copy entry
     c->next->next = NULL;
     cmnd.levels ++;
     cmnd.size *= 2;             //csize
   }


dirs[cmnd.fcom].setz (&cmnd);

// DBGPRTN("%s - OK e=%d", flbuf, error);
freecadt(&(cmnd.adnl));                                 // safety
show_prog(0);
return 0;
}




/***************************************************************
 *  read directives file
 ***************************************************************/
void getudir (void)
{
  int addr,addr2;
  uint i;
  int  j,bank;
  BANK *b;
  DFSYM *z;

  cmdopts = PDEFLT;           // always reset to default

  if (fldata.fl[3] == NULL)
    {
     wnprt("\n --- No directive file. Set default opts = C N P S");
      if (numbanks)
          {
           cmdopts |= H;        // 8065 multibank
           wnprt(" H");
          }
      wnprtn(0);
    }
  else
    {
      wnprtn("\n\n --- Read commands from directive file = %s", fldata.fn[3]);

     while (1)
	  {
	    if (!fgets (flbuf, 255, fldata.fl[3])) break;

        addr = parse_com(flbuf);
        show_prog(0);
	  }
    }

   j = (P8065)? 2:1;

  for (i = 0; i < NC(defsyms); i++)
    {
     z = defsyms+i;
     if (j & z->pcx) new_sym (z->wrt+2,z->symn, z->addr, z->bit1);
    }

  if (PMANL) return;


  // now setup each bank's first scans and interrupt subr cmds

  for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;
     if (!b->bok) continue;
 //    DBGPRTN("--- auto setup for bank %d ---",i);
     bank = i << 16;
     addr = PCORG;
     addr |= bank;

     add_scan (addr,S_IJMP|C_CMD,0);                    // inital scan at PCORG (cmd)
  
    // set_cmd (addr+0xa , addr+0xe, C_WORD,1);    // PCORG+0xa, cal pars and cmd
     j = (P8065) ? 0x5f : 0x1f;
     
     set_cmd (addr+0x10, addr+j, C_VECT|C_CMD);    // interrupt vecs, by cmd
     
     
 //    DBGPRTN(0);

     for (j -=1; j >= 0xf; j-=2)      // counting DOWN
	    {
	     addr2 = g_word (addr+j);
         addr2 |= bank;
         add_iname(j,addr2); 
         add_scan (addr2, S_ISUB|C_CMD,0);         // interrupt handling subrs
        }
     }
 }


short init_structs(void)
 {
  uint i;                                              // NO file stuff
  for (i = 0; i < NC(anames); i++) anames[i].nval = 1;
 
  memset(lram,0,NPARS*2);

  memset(bkmap,0,sizeof(bkmap));
  bkmap[2].opbt = opbit;                   // bit flag array start
  bkmap[3].opbt = opbit + 0x800;           // = 32 bit   0x1000 = 16 bit ints
  bkmap[4].opbt = opbit + 0x1000;
  bkmap[5].opbt = opbit + 0x1800; 
 
  memset(opbit,0, 0x8000);                 // clear all opcode markers
  memset(ibuf, 0, PCORG);                  // clear RAM and regs at front of ibuf 
  anlpass  = 0;                             // number of passes done so far
  gcol     = 0;
  lastpad  = 0;
  cmdopts  = 0;
  
  /*
  maxalloc = 0;          // malloc tracing
  curalloc = 0;
  mcalls   = 0;
  mrels    = 0;
  xcalls   = 0;
  */

  tscans = 0;
  cmntofst   = 0;
  lpix = 0;

  curinst = instpile;
  curops = curinst->opr;
  rambank = 0;

  return 0;
}


void openfiles(void)
{
  int i;

  fldata.error = 0;

  for (i = 0; i < 5; i++)
   {
    fldata.fl[i] = fopen(fldata.fn[i], fd[i].fpars);           // open precalc'ed names
    if(i < 3 && fldata.fl[i] == NULL) {fldata.error = i+1; return;}  // failed to open a file
   }

}




 /* first jump should start at 0 or 2000 within file for each bank.
  * fiddle banks ptrs temporarily to get sigs to scan the whole file
  * use a nominal bank 2,3,4,5 and fiddle the endpoint -

  * If we assume that bytes may be missing or extra then scan around the start
  * first, then scan whole front

  * 1 - at zero
  * 2 - at 0x2000          (1 & 2 are official starts)
  * 3 - -8 to +8           (missing or extra bytes)
  * 4 - full 0-2000        (last ditch effort)
  */
  

int scan_start (int ix, int faddr)
{
     // file is read in to ibuf[0x2000] onwards - look for start jump and interrupt vectors
 
    BANK *b;    
    SIG* s;
    int max, addr, bank, val, pass, x;

    b = bkmap+ix;
    bank = b->minpc & 0xf0000;      // FAKE bank from sig
    b->btype = 0;
    
    pass = 0;

 //   DBGPRTN("scan for bank start");
    while (pass < 4 && b->btype == 0)
     {
       switch(pass)
       {
          case 0:
            addr = faddr;                     // try at zero offset in this block, once
//            DBGPRTN("try0 = %x",addr);
            max = faddr;
            break;
            
          case 1:
            addr = faddr + 0x2000;            // try a block along, once (0x2000 of front fill)
//            DBGPRTN("try1 = %x",addr);
            max = addr;
            break;
            
          case 2: 
            addr = faddr-8;                   // try +-8 around a zero pointer
            max = faddr + 8;
//            DBGPRTN("try2 = %d to %d", addr,max);     // so minus works...

            break;
          
          case 3:
            addr = faddr;
            max = addr + 0x2000;                        // search whole block ?
//            DBGPRTN("try3 = %x to %x", addr,max);
           
       } 


       while (addr <= max && b->btype == 0)
        {
         clearchain(&sigch);                    // clear out any sigs so far
         b->fbuf = ibuf + addr;
         s = do_sig(&hdr,0x2000|bank);             //hdr is pattern ref - but this is just a jump, really.... 

        if (s)
         {
          bank = s->ofst & 0xf0000;               // FAKE bank from sig
          if (s->v[1] <= s->ofst+s->skp)
           {
             b->btype = 2;                          // loopstop 
             b->filstrt = addr;                  // actual bank start (buffer offset) for 0x2000
             s->v[3] = 2;
           }
          if ((s->v[1] & 0xffff) > 0x201f)
           {
             b->btype = 1;                         // jump over int regs.
             b->filstrt = addr;                  // actual bank start (buffer offset) for 0x2000
             s->v[3] = 1;
           }

          // now check that the int vects look OK.....if 8065 this can be longer...
          // possibly check for ignint too ??


          if (s->skp > 2) b->btype = 0;                                    // TEST !! for extra front chars
          for (x = (bank|0x2010);  x < (bank|0x201f);  x+=2)
           {               // 8061 ints
            val = g_word(x);
            if (val < 0x2020) b->btype = 0;              //    need more here !!  
           
   //         val = g_byte(bank|val); 
 
//            if (val && (val < 0xf0 || val > 0xf2) && val != 0x10) 
  //            {     
    //           b->btype = 0;                 // 0,ret, retei, pushp, bnk allowed  CARD !!
    //          }
           }
           
          // if it's a bnk, then the jump should go to one of the other existing int subrs....

         b->dbnk = 1;                        // temp flag
         for (x = (bank|0x2020);  x < (bank|0x205f);  x+=2)
           {
            val = g_word(x);
            if (val < 0x2060) b->dbnk = 0;                 // need more here !!  
           }

           if (b->btype)
             {
              faddr = addr; 
  //            DBGPRTN("match at = %x", faddr);    
             } 
         }           // end of if (s)
         addr++;   
        
      }          // for cnt
     pass++; 
    }             // for pass
      
   return faddr;  

 }




void find_banks(void)
 {
  // file is in ibuf[0x2000] onwards
  // set TEMP banks [2]-[5] as these are not used in main decode
     
  int faddr, i, end, bank;
  BANK *b;

  lastbmap = bkmap;                         // for later on...
  numbanks = -1;   
  
  i = 2;                                    // use 2,3,4,5 as temp holders 
  end = fldata.fillen;
  faddr = 0;
 
  while (faddr < end && i < 6)           // 4 banks max for now
    {
     bank = i << 16;   
     b = bkmap+i;
     b->maxbk = bank | 0xffff;          // temp bank setup so that g_val (sigs) etc. work
     b->maxpc = b->maxbk;               // need for signature scan
     b->minpc = bank | PCORG;
     b->bok = 1;

     faddr = scan_start(i, faddr);             // test for start jump at or around around faddr

     if (b->btype > 0)
      {                                        // found !!
       faddr += 0xe000;                        // assume end of this bank, as a file offset
       b->filend = faddr-1;                 // this may be wrong, but this is default for now (0x2000 - 0xffff).
           
       if (faddr >= end) b->filend = end-1;                  // end of file
       
       b->maxbk = b->filend - b->filstrt + PCORG;
       b->maxbk |= bank;
       numbanks++;

  //     DBGPRTN("temp Bank %5x-%5x, type %d", b->filstrt, b->filend, b->btype);
  //     DBGPRTN(0);
       if (faddr >= end) break;
       i++;
     }
    else break;
    }
}
 
int find_text(BANK *b)
{
  // check for text in end filler block
  // text blocks have either "Copyright at ff63" or ".HEX* ff06"  in them...
  // check address above ff00 for copyright and then check blocks....
  // works for now, more or less
 
  int ans, val, start, ofst, bk, cnt;
 
  if ((b->maxbk & 0xffff) < 0xdfff)  return 0;          // not full bank
  
  if (strncmp("Copyright", (char *) b->fbuf + 0xff63, 9)) return 0;  // not found
  
  // matched Copyright string
  bk = b->minpc & 0xf0000;

  start = (0xff63 | bk);
  ofst = start;

  ans = 0;  
  val = g_byte(ofst);   
  
  while (!val || (val >=0x20 && val <= 0x7f))
   {
      ofst++;
      val = g_byte(ofst);
   }
  set_cmd (start,ofst, C_TEXT |C_CMD);
  set_cmd (ofst+1,b->maxbk, C_DFLT|C_CMD);    // treat as cmd (cannot override)   
 
  ans = start - 1;

  ofst = (0xff06 | bk);
  cnt = 0;
  val = g_byte(ofst);
  
  while (ofst < (0xff63 | bk)) 
   {            // allow a couple of non printers...use 6....
    // if (!strncmp(".HEX", (char *) b->fbuf + (ofst & 0xffff), 4)) start = (0xff06 | bk);  // found in blk somewhere
     if (val < 0x20 || val > 0x7f) 
      {
        if (cnt > 5) set_cmd (ofst-cnt,ofst-1, C_TEXT |C_CMD);
        cnt = 0;
      }   
     else 
      {
       cnt ++;
       start = ofst;
      } 
     ofst++;
     val = g_byte(ofst);
   }

    ans = start-1;
 
  return ans;   
 }


void find_filler(BANK *b)
  {

// look for filler at end of bank.

   int ofst, cnt, val, last;
   
   last = b->maxbk;
   b->maxpc = b->maxbk;             // default and safety

   ofst = find_text(b);             // any text found ?
   
   if (!ofst) ofst = b->maxbk; else last = ofst;
   
   cnt = 0;
   for (; ofst > b->minpc; ofst--)
     {
      val = g_byte(ofst);
      
      if (val != 0xff) 
         {
          cnt++;                  // count non 0xff
         }
      if (cnt > 3) break;         // 2 non 0xff and no text match
     }
     
     
   ofst += 4;                     // 2 count plus one - may need more for a terminator byte ?

   if (ofst < b->maxbk)
      {
        // fill at end
       set_cmd (ofst,last, C_DFLT|C_CMD);    // treat as cmd (cannot override)
       set_auxcmd (ofst,b->maxpc,C_XCODE);       // and xcode to match
       b->maxpc = ofst;
  //     DBGPRTN(0);
      }
  }  

void mkbanks(int num, ...)
{
 va_list ixs;
 int i;
 va_start(ixs,num);

 for (i = 2; i < 2+num; i++) bkmap[i].dbnk = va_arg(ixs,int);

 va_end(ixs);

}

void cpy_banks(void)
 {
  int i, bk;
  BANK *b;
  
 for (i = 2; i < 3+numbanks; i++)
 {
   b = bkmap + bkmap[i].dbnk;                // preset destination
   bk = (bkmap[i].dbnk << 16);
   *b = *(bkmap+i); 
   b->minpc &= 0xffff;
   b->minpc |= bk;    // substitute bank no
   b->maxpc = 0;       // do fill check
   b->maxbk &= 0xffff;
   b->maxbk |= bk;
   bkmap[i].bok = 0;
 }

}

void set_banks(void)
{
 int i;
 BANK *b;
 
  codbank = 0x80000;           // code start always bank 8
  rambank = 0;                 // start at zero for RAM locations
  if (numbanks) datbank = 0x10000;           // data bank default to 1 for multibanks
  else   datbank = 0x80000;         // databank 8 for single

  for (i = 0; i < BMAX; i++)
     {
         // maxpc used as flag to check if user entered....
         
      b = bkmap + i;
      if (b->bok && !b->maxpc) find_filler(b);
     }

// this is a temp fiddle for single bank 8065...

    if (!numbanks)
     {  
      i = g_word(0x82060);
      if  (i == 0x108) cmdopts |= H;        // single bank 8065, rbase at 2060
     }  


  if (P8065)
     {
      minwreg = 0x1f;    // min allowed update RAM - allow stack
      maxwreg = 0x3ff;   // max register
      stkreg1 = 0x20;    // stack
      stkreg2 = 0x22;    // altstack
     }
  else
     {
      minwreg = 0xf;    // same for 8061, no altstack
      maxwreg = 0xff;   
      stkreg1 = 0x10;
      stkreg2 = -1;
     }
 }



int do_banks(void)
 {
  // set up banks after find_banks, or user command.
    
  // set up RAM bank [16] first 0x2000 of ibuf for ram and regs
  // NOTE - not technically safe if file has missing front bytes, but probably OK

  int end,ofst,bank,i;      //, ans;
  BANK *b;

  b = bkmap+BMAX;
  b->maxbk = PCORG-1;          //0-0x1fff in bank 16 for registers
  b->maxpc = PCORG-1;
  b->minpc = 0;
  b->fbuf  = ibuf;                  // first 0x2000  in bank 16 for regs and RAM
  b->bok = 1;

  // now do the checks and bank ordering.....
 
  if (numbanks)  cmdopts |= H;          // multibank bin, mark as 8065
  else
   {  
     if (bkmap[2].dbnk) cmdopts |= H;   // single bank 8065 detected from int.vects
   }

  bank = -1;                           // temp bank with the start jump (=code)
  end = 0;                             // temp number of banks with start jumps
  ofst = 0;                            // temp number of loopback jumps

  for (i = 0; i <= numbanks; i++)
      {
       b = bkmap+i+2;                // 2 to 5
       b->dbnk = 0;                  // clear any flags 
       if (b->btype == 1) { end++; bank = i; }              // a real jump
       if (b->btype == 2) ofst++;                           // loopback jump
      }

  if (!end)
     {
      wnprtn("Cannot find Code start bank [8]");
      return -4;
     }

  if (end > 1)
     {
      wnprtn("Found multiple Code start banks [8]");
      return -5;
     }

 // DBGPRT("%d Banks, start %d\n", numbanks+1, bank+1);

  // this is a bit ugly, but it allows commands to override autosetup easily...
  // just set which bank is which 
 
  switch (numbanks)
    {

     default:
       wnprtn("No banks, or >4 banks found. File corrupt?");
       return -2;
       break;

     case 0:                      // copybanks autostarts at [2] for source
       mkbanks(1,8);   
       break;

     case 1:                      // 2 bank;
       if (bank == 1) mkbanks(2,1,8); else mkbanks(2,8,1);    
       break;

     // no 3 bank bins so far .....

     case 3:

       switch(bank)
        {
         // Alternate bank orders - best guess

         case 0:
           mkbanks(4,8,9,0,1);  
           break;

         case 2:
         default:
           mkbanks(4,0,1,8,9);
           break;

         case 3:
           mkbanks(4,0,1,9,8);
           break;
        }

       break;
      }

   // final copy and fill detection. this subr recalled if user command
  cpy_banks();
  set_banks();  

  return 0;  
}


int readbin(void)
{
  /* Read binary into main block 0x2000 onwards (ibuf[0x2000])
  *  ibuf  [0]-[2000]     RAM  (faked and used in this disassembly)
  *  ibuf  [2000]-[42000] up to 4 banks ROM, at 0x10000 each
  */

 // int end;

  fldata.fillen = fseek (fldata.fl[0], 0, SEEK_END);
  fldata.fillen = ftell (fldata.fl[0]);
  fseek (fldata.fl[0], 0, SEEK_SET);	            // get file size & restore to start

  wnprtn(" --- Input file = '%s'. It is %d (0x%x) bytes",fldata.fn[0], fldata.fillen, fldata.fillen);

//  fldata.error = 0;

  if (fldata.fillen > 0x40000)
   {
    wnprtn("BIN file > 256k. Truncated at 256k");  // is this always right ??
    fldata.fillen = 0x40000;
   }

  //end = 
  fread (ibuf+PCORG, 1, fldata.fillen, fldata.fl[0]);
 // DBGPRTN("Read input file (0x%x bytes)", end);

// check for file error ?
//  end = fldata.fillen;

  find_banks();
  do_banks();               // auto, may be modded later by command

  return 0;
}

void rbasp(SIG *sx, SBK *b)
{

  //add rbase entries from signature match
 
  int add, reg, i, cnt,xcnt,rcnt;
  RBT *r,*n;
  int  pc;

  if (b) return;                 // no process if subr set

  rcnt = basech.num;              // may have rbases already start at 'NEW' point
  reg = (sx->v[1] & 0xff);
  add = sx->v[3];
  cnt = g_byte(add);          // no of entries
  
//  DBGPRTN("In rbasp - begin reg %x at %x, cnt %d", reg, add, cnt);

  if (!set_cmd (add, add+1,C_BYTE))
   {
 //   DBGPRTN("Rbase Already Processed");
    return ;
   }
  set_cmd (add+2, (add+cnt*2)+1, C_WORD);

  pc= add+2;

  for (i = 0; i < cnt; i++)
   {
    add = g_word(pc);                       // register pointer
    add |= datbank;                         // add + default data bank
    r = add_rbase(reg,add);                     // message in here....
    if (r) r->cmd = 1;
    reg+=2;
    pc+=2;
   }

 // now check pointers for XCODE directives for newly added entries

 if (rcnt == basech.num)  return ;     // nothing added

 for (i = rcnt; i < rcnt+cnt; i++)
   {
     if (i >= basech.num)  break;   
    r = (RBT*) basech.ptrs[i];
    n = (RBT*) basech.ptrs[i+1];
    pc = n->rval & 0xffff;

    xcnt =0;

    if (i >= rcnt+cnt-1)
     {
      n = (RBT*) basech.ptrs[i-1];                  // get bank of last pointer
      r->rval = (r->rval & 0xffff) | (n->rval & 0xf0000);
      pc = g_word(r->rval);
     }
    else
     {
       while (pc != g_word(r->rval) && xcnt < BMAX && i < rcnt+cnt-1)
       {
        xcnt++;             // try to find matching bank...
        if (bkmap[xcnt].bok)  r->rval = (r->rval & 0xffff) | (xcnt << 16);
       }
     }

    if (xcnt >= BMAX) 
    {
      //DBGPRTN("Matching rbase addr not found !");
    }
    else
      {
   //     if (xcnt) DBGPRTN("Rbase %x = %x",r->radd, r->rval);
        set_cmd(r->rval, r->rval+1, C_WORD);
        set_auxcmd(r->rval,pc-1, C_XCODE);          // and add an XCODE as this is data
      }
   }
   sx->done = 1;
 //  DBGPRTN("end rbasp");
   return ;
}



char *calcfiles(char *fname)
 {
  // do filenames
  char *t, *x;
  int i;

  t = strrchr(fname, '\n');          // DOS and Linux versions may have newline at end
  if (t) {*t = '\0';}

  x = fname;                           // keep possible pathname

  t = strrchr(fname, pathchar);         // stop at last backslash
  if (t)
   {          // assume a path in filename, replace path.
    *t = '\0';
    fname = t+1;
    sprintf(fldata.path, "%s%c", x,pathchar);
   }


  x = strrchr(fname, '.');                         // ditch any extension
  if (x) *x = '\0';

  strcpy(fldata.bare,fname);                        // bare file name

  for (i = 0; i < 5; i++)
   {
    if (! *(fldata.fn[i])) strcpy(fldata.fn[i],fldata.path);   // use default path name

    strcat(fldata.fn[i],fldata.bare);                 // bare name
    strcat(fldata.fn[i],fd[i].suffix);                // plus suffix
   }
 return fldata.bare;
 }


/***************************************************************
 *  disassemble EEC Binary
 ***************************************************************/
short dissas (char *fstr)         // filedata added
{
  init_structs();
  calcfiles(fstr);
  openfiles();

  if (fldata.error)
    {
     printf("File not found or File error\n");
     return fldata.error;
    }

  wnprtn("\n ----------------------------");
 
  wnprtn(" SAD Version %s", SADVERSION);
 wnprtn(" ----------------------------");
 wnprtn(0);
   
  if (readbin() < 0)               // and does init_structs in here
  {
    wnprtn("abandoned !!");
    printf("can't process");
    return 0;  
  }

  //verify_chk ();
  show_prog(++anlpass);            //anlpass = 1  cmds and setup
  getudir();                       // AFTER readbin...

  show_prog(++anlpass);            //anlpass = 2  signature pass
  prescan_sigs();                  // full pass for signatures


 
  wnprt("\n\n --- Start Disassembly -----\n");
 // wnprt("-- commands before scan start\n");
    // DBGPRTN("# aux = %d, cmds = %d", auxch.num,cmdch.num);

 // prt_sigs(&sigch);
 // prt_dirs(0);

  scan_all ();

  // should check if anything on HOLD !!



 // wnprt("\n\n------------Post scan ------------\n");

 // ----------------------------------------------------------------------

  do_jumps();
 
  code_scans();    // turn all scans into code - after any hole fills.....
  
  do_ddt();        // code, data, subnames  (need to add any scans from pushes here too....)
  
  do_subnames();


  show_prog(++anlpass);             //anlpass = 3  scan phase
  // ----------------------------------------------------------------------
  show_prog (++anlpass);            //anlpass = 4

 // DBGPRT("max alloc = %d\n", maxalloc);
 //   DBGPRT("cur alloc = %d\n", curalloc);
 //   DBGPRT("mcalls = %d\n", mcalls);
 //   DBGPRT("mrels  = %d\n", mrels);
 //   DBGPRT("xcalls = %d\n", xcalls);
 //   DBGPRT("tscans = %d\n", tscans);

   wnprtn("\n\n --- Outputting Listing to %s------------\n",fldata.fn[1]);

  anlpass = ANLPRT;

  do_listing ();
  newl(2);     // flush output file

  // must remember to drop subs with no name ...


//    wnprt("\n\n -- DEBUG INFO --\n");
 //   prt_jumps(&jpfch);
 //   prt_scans(1);
 //   prt_ddt();
 //   prt_sigs(&sigch);

 

  wnprtn("\n\n---------------------------------------------------------------------------------------------");
  wnprtn("The disassembler has scanned the binary and arrived at the following final command list.");
  wnprtn("This list is not guaranteed to be perfect, but should be a good basic start.");
  wnprtn("This following list can be copied and pasted into a dir file.  ( #cmd = user command/directive)");
  wnprtn("---------------------------------------------------------------------------------------------\n\n");
  
  prt_dirs(1);
  free_structs();
  wnprtn( "\n\n------ END Disassembly run ------\n");
  printf ("\n End of run\n");
  closefiles();
  return 0;
}

/*void put_config(char* p)
{
 // put config if sad.ini is null

  short i;
  char *t;

  if (!fldata.fl[5])      // double check
   {
    fldata.fl[5] = fopen(fldata.fn[5], "w");    //open for write
    if (!fldata.fl[5])
      {
       fldata.error = 6;                        // error opening file
       return;
      }

    for (i =0; i < 5; i++)
      {
       t = strrchr(fldata.fn[i], pathchar);          // is there a path ?
       if (t)
        {
         *t = '\0';                     // terminate the path
         t = fldata.fn[i];               // set path
        }
       else
        {
         t = fldata.dpath;                // set default path
         sprintf(fldata.fn[i],"%s%c", t,pathchar);   // and put in file path
        }
       fprintf (fldata.fl[5], "%s%c    #%s location \n", t,pathchar,fd[i].suffix);
      }
    fclose(fldata.fl[5]);

   }
}*/

int get_config(char** pts)
 {
  /* pts [0] = exe path (and default) ,
   * [1] = config file READ path or zero.  (use default path)
   * [2] = config WRITE path or zero
   */

  short i;
  char *t;

  // strchr(cmdline, '\')   for first occurence   DOS (and 'x:'
  // strchr(cmdline, '/')   for first occurence   UNIX

  t = pts[0];
  if (*t == '\"' || *t == '\'') t++;      // skip any opening quote
  strcpy(fldata.dpath, t);

  t = strchr(fldata.dpath, '\\');
  if (t) pathchar = '\\';                    // assume DOS or WIN
  else
   {
    t = strchr(fldata.dpath, '/');
    if (t) pathchar = '/';                   // assume UX
   }
   
  if (!t) printf("error in cmdline !!");

  t = strrchr(fldata.dpath, pathchar);           // stop at last backslash to get path
  if (t) *t = '\0';

  if (pts[1])
  {
     strncpy(fldata.fn[5],pts[1], 255);           // copy max of 255 chars
     i = strlen(fldata.fn[5]);                    // size to get last char
     t = fldata.fn[5]+i-1;                        // t = last char
     if (*t == pathchar) *t = 0; else t++;      

     sprintf(t, "%c%s",pathchar,fd[5].suffix);    // and add sad.ini to it...

     fldata.fl[5] = fopen(fldata.fn[5], fd[5].fpars);
     if (!fldata.fl[5])
       {
           printf("\ncan't find config file %s\n", fldata.fn[5]);
           return 1;                // no config file in -c
       }
  }

 else
  {
    // else use bin path to make up file file name of SAD.INI
    sprintf(fldata.fn[5], "%s%c%s",fldata.dpath,pathchar,fd[5].suffix);
    fldata.fl[5] = fopen(fldata.fn[5], fd[5].fpars);         //open it if there
  }

 // printf ("\nconfig file = %s", fldata.fn[5]);

   if (fldata.fl[5])
     {                                           // file exists, read it
     for (i = 0; i< 5; i++)
      {
       if (fgets (flbuf, 256, fldata.fl[5]))
        {
         t = strchr(flbuf, ' ');                 // first space
		 if (!t) t = strchr(flbuf,'#');          // or comment
         if (!t) t = strchr(flbuf, '\n');        // or newline
         if (t) 
			{
			 *t = '\0';
		     t--;
             if (*t != pathchar) sprintf(t+1, "%c", pathchar);
			}
		 strcpy(fldata.fn[i], flbuf);
		 
       }
      }
     fclose(fldata.fl[5]);                 //close leaves handle addr
     strcpy(fldata.path, fldata.fn[0]);
     printf("\n");
    }
  else printf(" - not found, assume local dir\n");
  
//  if (pts[2])  put_config(pts[2]);
  return 0;
 };