
/******************************************************
 *  8065 disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
  ******************************************************
 * see shared.h for BUILD NOTES
 *
 ******************************************************
 *  Declarations and includes
 *******************************************************/

#include  "core.h"                // calls shared.h

/**********************************************************************************
local global declarations
***********************************************************************************/

cchar *empty = "\0\0\0";      // safety. allows ptr increments, fixes compiler warnings



//*****************************************************************************
// code handler/emulation procs for opcode struct

SBK* clr  (SBK *, INST *); SBK* ldx  (SBK *, INST *); SBK* stx  (SBK *, INST *); SBK* orx  (SBK *, INST *);
SBK* addx (SBK *, INST *); SBK* andx (SBK *, INST *); SBK* neg  (SBK *, INST *); SBK* cpl  (SBK *, INST *);
SBK* sub  (SBK *, INST *); SBK* cmp  (SBK *, INST *); SBK* mlx  (SBK *, INST *); SBK* dvx  (SBK *, INST *);
SBK* shl  (SBK *, INST *); SBK* shr  (SBK *, INST *); SBK* popw (SBK *, INST *); SBK* pshw (SBK *, INST *);
SBK* inc  (SBK *, INST *); SBK* dec  (SBK *, INST *); SBK* bka  (SBK *, INST *); SBK* cjm  (SBK *, INST *);
SBK* bjmp (SBK *, INST *); SBK* djm  (SBK *, INST *); SBK* ljm  (SBK *, INST *); SBK* cll  (SBK *, INST *);
SBK* ret  (SBK *, INST *); SBK* scl  (SBK *, INST *); SBK* skj  (SBK *, INST *); SBK* sjm  (SBK *, INST *);
SBK* pshp (SBK *, INST *); SBK* popp (SBK *, INST *); SBK* nrm  (SBK *, INST *); SBK* sex  (SBK *, INST *);
SBK* clc  (SBK *, INST *); SBK* stc  (SBK *, INST *); SBK* die  (SBK *, INST *); SBK* nop  (SBK *, INST *);
SBK* clv  (SBK *, INST *);

// and print procs (code in print.cpp)

uint pp_divprt (INST *c);   uint pp_bitjmp (INST *c);   uint pp_jmp   (INST *c);   uint pp_call (INST *c);
uint pp_cond   (INST *c);   uint pp_3op    (INST *c);   uint pp_2op   (INST *c);   uint pp_cy   (INST *c);
uint pp_pswf   (INST *c);   uint pp_pswc   (INST *c);   uint pp_btws  (INST *c);   uint pp_an3x (INST *c);
uint pp_djnz   (INST *c);   uint pp_pswv   (INST *c);   uint pp_nrm   (INST *c);   uint pp_sce  (INST *c);
uint pp_psh    (INST *c);   uint pp_pop    (INST *c);

/*****************************************************
 * opcode value to main opcode struct index (opctbl below).
 * index 0 is an invalid opcode
 *****************************************************/

 uchar opcind[256] =
{
// 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
   76,   89,   96,   91,   0,    98,  100,   93,   4,    1,    5,    0,    6,    3,    7,    94,    // 00 - 0f
// SKIP, CLRW, CPLW, NEGW,       DECW, SEXW, INCW, SHRW, SHLW, ASRW,       SHRD, SHLD, ASRD, NORM
   112,  88,   95,   90,   0,    97,   99,   92,   8,    2,    9,    0,    0,    0,    0,    0,     // 10 - 1f
// RBNK, CLRB, CPLB, NEGB,       DECB, SEXB, INCB, SHRB, SHLB, ASRB
   77,   77,   77,   77,   77,   77,   77,   77,   78,   78,   78,   78,   78,   78,   78,   78,    // 20 - 2f
// SJMP                                            SCALL
   70,   70,   70,   70,   70,   70,   70,   70,   71,   71,   71,   71,   71,   71,   71,   71,    // 30 - 3f
// JNB                                             JB
   13,   13,   13,   13,   25,   25,   25,   25,   29,   29,   29,   29,   20,   20,   20,   20,    // 40 - 4f  mult mode 40 to cf
// AN3W                    AD3W                    SB3W                    ML3W
   12,   12,   12,   12,   24,   24,   24,   24,   28,   28,   28,   28,   18,   18,   18,   18,    // 50 - 5f
// AN3B                    AD3B                    SB3B                    ML3B
   11,   11,   11,   11,   23,   23,   23,   23,   27,   27,   27,   27,   16,   16,   16,   16,    // 60 - 6f
// AN2W                    AD2W                    SB2W                    ML2W
   10,   10,   10,   10,   22,   22,   22,   22,   26,   26,   26,   26,   14,   14,   14,   14,    // 70 - 7f
// AN2B                    AD2B                    SB2B                    ML2B
   31,   31,   31,   31,   33,   33,   33,   33,   35,   35,   35,   35,   38,   38,   38,   38,    // 80 - 8f
// ORW                     XRW                     CMPW                    DIVW
   30,   30,   30,   30,   32,   32,   32,   32,   34,   34,   34,   34,   36,   36,   36,   36,    // 90 - 9f
// ORB                     XORB                    CMPB                    DIVB
   41,   41,   41,   41,   43,   43,   43,   43,   45,   45,   45,   45,   46,   46,   46,   46,    // a0 - af
// LDW                     ADCW                    SBBW                    LDZBW
   40,   40,   40,   40,   42,   42,   42,   42,   44,   44,   44,   44,   47,   47,   47,   47,    // b0 - bf
// LDB                     ADCB                    SBBB                    LDSBW
   49,    0,   49,   49,   48,    0,   48,   48,   50,   50,   50,   50,   52,    0,   52,   52,    // c0 - cf
// STW (no imd)            STB (no imd)            PUSHW                   POPW (no imd)
   54,   63,   64,   56,   60,   58,   66,   69,   55,   62,   65,   57,   61,   59,   67,   68,    // d0 - df
// JNST, JLEU, JGT,  JNC,  NVT,  JNV,  JGE,  JNE,  JST,  JGTU, JLE,  JC,   JVT,  JV,   JLT,  JE
   72,    0,    0,    0,   0,    0,    0,    75,    0,    0,    0,    0,    0,    0,    0,   73,    // e0 - ef
// DJNZ                                      JUMP                                            CALL
   80,   82,   84,   86,   107,  108,  109,   0,   102,  101,  103,  104,  105,  110,  111,  106     // f0 - ff
// RET,  RETI, PSHP, POPP, Bnk0, Bnk1, Bnk2,       CLC,  STC,  DI,   EI,  CLVT,  Bnk3, SIGN,  NOP
};



/*****************************************************
 * OPC struct - main opcode definition, index via opcind (0x0 - 0xff full list)
 * main struct is ordered to put similar opcodes together.
 *
 * Note - '\1', '\2' , '\3' used in some strings to indicate operand number
  *****************************************************************************
 *
 * 806x bit order. 7 is MS, 0 LS, = bit order is 7,6,5,4,3,2,1,0 as read.
 * PSW mask is    Z N V VT C ST    as per Ford manuals.
 *
 * Operand sizes shown here as set of 4 are 'field end bitno' 0-0x1f
 * all have start field of zero, plus flags of 0x20 for signed, 0x40 for write op
 * also + 0x100 means 'print operand size and sign' in listing

 List of values -
 *    7,  0xf,   0x17,  0x1f   unsigned read,  byte, word, triple, long
 * 0x27,  0x2f,  0x37,  0x3f     signed read,  byte, word, triple, long  (+ 0x20)
 * 0x47,  0x4f,  0x57,  0x5f   unsigned write, byte, word, triple, long  (+ 0x40)
 * 0x67,  0x6f,  0x67,  0x7f   signed   write, byte, word, triple, long  (+ 0x60)
 *
 * (note that '24 bit triple' does not exist for any opcodes, but many bins use
 * 24 bit (+triple) variables, so included as option
 *
 * Operands when decoded -
 *  Op[3] is ALWAYS the WRITE (destination) operand.
 *  Internal calculations are always [3] = [2] <op> [1] to provide consistent handling.
 *  Operands are copied and swopped around internally as required in the opcode handlers
 ***********************************************************************

 * Signature index value for alike opcodes - MAX 63 - in OPC struct below
 * used in 'fingerprint' matching for a kind of regexp functionality

 * 0  special    1  clr      2  neg/cpl   3 shift l    4 shift R    5 and
 * 6  add        7  subt     8  mult      9 or         10 cmp       11 divide
 * 12 ldx        13 stx      14 push      15 pop       16 statjmp   17 subr
 * 18 djnz       19 condjmp  20 ret       21 carry     22 other     23 jb/jnb
 * 24 dec        25 inc

 * 40+ are skipped by default, but detectable as -
 * 40 sig        41 pushp    42 popp     43 di         44 ei        45 (RAM)bnk
 * 46 (ROM)bnk   47 nop

 (statjmp is static jump (GOTO) , condjmp is conditional jump)

 * notes -
 * 23 and 19 (jnb/jb and condjumps) can match
 * 12 and 13 (ldx, stx) can match

 ***************************************************************
 * OPC item layout (for one row)
 * signature index,
 * 'fe' prefix allowed - prefixed version is ALWAYS next table entry,
 * 8065 opcode only,
 * rombank prefix allowed  (not all opcodes allow bank prefix)
 * updates PSW register
 * number of operands,
 * (unused)
 * 4 operand sizes (0,1,2,3 as above  0 is for index address offset)
 * opcode handler (subroutine)
 * opcode print handler (subrotuine)
 * opcode name
 * 'pseudo source' string, mostly the actual operation
 ***********************************************************************************/



OPC opctbl[] = {                                                               // 113 entries
{ 0 , 0, 0, 0,  0, 0,  0,0, 0x0  ,  0x0  , 0x0   , 0     ,pp_sce ,"!INV!" , empty },        // 0  all invalid opcodes

{ 3 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0xf  , 0x4f  , shl   ,pp_2op ,"shlw"  , "<<=" },        // 1
{ 3 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x7  , 0x47  , shl   ,pp_2op ,"shlb"  , "<<=" },
{ 3 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x1f , 0x15f , shl   ,pp_2op ,"shldw" , "<<=" },

{ 4 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0xf  , 0x4f  , shr   ,pp_2op ,"shrw"  , ">>=" },        // 4
{ 4 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x2f , 0x16f , shr   ,pp_2op ,"asrw"  , ">>=" },
{ 4 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x1f , 0x15f , shr   ,pp_2op ,"shrdw" , ">>=" },        // 6
{ 4 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x3f , 0x17f , shr   ,pp_2op ,"asrdw" , ">>=" },
{ 4 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x7  , 0x47  , shr   ,pp_2op ,"shrb"  , ">>=" },        // 8
{ 4 , 0, 0, 0,  1, 2,  0,0, 0x7  ,  0x27 , 0x167 , shr   ,pp_2op ,"asrb"  , ">>=" },

{ 5 , 0, 0, 1,  1, 2,  0,0, 0x7  ,   0x7 ,  0x47 , andx  ,pp_btws  ,"an2b" , "&=" },        // 10
{ 5 , 0, 0, 1,  1, 2,  0,0, 0xf  ,   0xf ,  0x4f , andx  ,pp_btws  ,"an2w" , "&=" },
{ 5 , 0, 0, 1,  1, 3,  0,0, 0x7  ,   0x7 ,  0x47 , andx  ,pp_an3x  ,"an3b" , "&"  },
{ 5 , 0, 0, 1,  1, 3,  0,0, 0xf  ,   0xf ,  0x4f , andx  ,pp_an3x  ,"an3w" , "&"  },

{ 8 , 1, 0, 1,  1, 2,  0,0, 0x7  , 0x107 , 0x14f , mlx   ,pp_3op ,"ml2b"  , "*" },          // 14  2op but show long for sizes
{ 8 , 0, 0, 1,  1, 2,  0,0, 0x27 , 0x127 , 0x16f , mlx   ,pp_3op ,"sml2b" , "*" },
{ 8 , 1, 0, 1,  1, 2,  0,0, 0xf  , 0x10f , 0x15f , mlx   ,pp_3op ,"ml2w"  , "*"  },
{ 8 , 0, 0, 1,  1, 2,  0,0, 0x2f , 0x12f , 0x17f , mlx   ,pp_3op ,"sml2w" , "*" },

{ 8 , 1, 0, 1,  0, 3,  0,0, 0x7  , 0x107 , 0x14f , mlx   ,pp_3op ,"ml3b"  , "*"  },          // 18
{ 8 , 0, 0, 1,  0, 3,  0,0, 0x27 , 0x127 , 0x16f , mlx   ,pp_3op ,"sml3b" , "*"  },
{ 8 , 1, 0, 1,  0, 3,  0,0, 0xf  , 0x10f , 0x15f , mlx   ,pp_3op ,"ml3w"  , "*"  },
{ 8 , 0, 0, 1,  0, 3,  0,0, 0x2f , 0x12f , 0x17f , mlx   ,pp_3op ,"sml3w" , "*" },

{ 6 , 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7   , 0x47  , addx  ,pp_2op ,"ad2b"  , "+=" },                 // 22  need rbase convert !!
{ 6 , 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf   , 0x4f  , addx  ,pp_2op ,"ad2w"  , "+=" },
{ 6 , 0, 0, 1,  1, 3,  0,0, 0x7  , 0x7   , 0x47  , addx  ,pp_3op ,"ad3b"  , "+" },
{ 6 , 0, 0, 1,  1, 3,  0,0, 0xf  , 0xf   , 0x4f  , addx  ,pp_3op ,"ad3w"  , "+" },

{ 7 , 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7   , 0x47  , sub   ,pp_2op  ,"sb2b"  , "-=" },                 // 26
{ 7 , 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf   , 0x4f  , sub   ,pp_2op  ,"sb2w"  , "-=" },
{ 7 , 0, 0, 1,  1, 3,  0,0, 0x7  , 0x7   , 0x47  , sub   ,pp_3op  ,"sb3b"  , "-" },
{ 7 , 0, 0, 1,  1, 3,  0,0, 0xf  , 0xf   , 0x4f  , sub   ,pp_3op  ,"sb3w"  , "-" },

{ 9 , 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7   , 0x47  , orx   ,pp_btws ,"orb"   , "|=" },               // 30
{ 9 , 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf   , 0x4f  , orx   ,pp_btws ,"orw"   , "|=" },
{ 9 , 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7   , 0x47  , orx   ,pp_btws ,"xorb"  , "^=" },
{ 9 , 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf   , 0x4f  , orx   ,pp_btws ,"xrw"   , "^=" },

{ 10, 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7  , 0x7    , cmp   ,pp_sce ,"cmpb"  , empty },            // 34     (no write)
{ 10, 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf  , 0xf    , cmp   ,pp_sce ,"cmpw"  , empty },

{ 11, 1, 0, 1,  1, 2,  0,0, 0x7  , 0x10f , 0x147 , dvx   ,pp_divprt ,"divb"  , empty  },          //36   2 line print
{ 11, 0, 0, 1,  1, 2,  0,0, 0x27 , 0x12f , 0x167 , dvx   ,pp_divprt ,"sdivb" , empty  },
{ 11, 1, 0, 1,  1, 2,  0,0, 0xf  , 0x11f , 0x14f , dvx   ,pp_divprt ,"divw"  , empty   },
{ 11, 0, 0, 1,  1, 2,  0,0, 0x2f , 0x13f , 0x16f , dvx   ,pp_divprt ,"sdivw" , empty   },

{ 12, 0, 0, 1,  0, 2,  0,0, 0x7  , 0x7   , 0x47  , ldx   ,pp_btws ,"ldb"   , "=" },                // 40
{ 12, 0, 0, 1,  0, 2,  0,0, 0xf  , 0xf   , 0x4f  , ldx   ,pp_btws ,"ldw"   , "=" },

{ 6 , 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7   , 0x47  , addx  , pp_cy ,"adcb"  , "+" },            // 42
{ 6 , 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf   , 0x4f  , addx  , pp_cy ,"adcw"  , "+" },
{ 7 , 0, 0, 1,  1, 2,  0,0, 0x7  , 0x7   , 0x47  , sub   , pp_cy ,"sbbb"  , "-" },
{ 7 , 0, 0, 1,  1, 2,  0,0, 0xf  , 0xf   , 0x4f  , sub   , pp_cy ,"sbbw"  , "-" },

{ 12, 0, 0, 1,  0, 2,  0,0, 0x107, 0x7   , 0x14f , ldx   ,pp_2op ,"ldzbw" , "=" },                // 46
{ 12, 0, 0, 1,  0, 2,  0,0, 0x107, 0x7   , 0x16f , ldx   ,pp_2op ,"ldsbw" , "=" },

{ 13, 0, 0, 1,  0, 2,  0,0, 0x7  , 0x7   , 0x47  , stx   ,pp_2op ,"stb"   , "=" },                  // 48 swop operands in handler
{ 13, 0, 0, 1,  0, 2,  0,0, 0xf  , 0xf   , 0x4f  , stx   ,pp_2op ,"stw"   , "=" },

{ 14, 1, 0, 1,  0, 1,  0,0, 0xf  , 0     , 0     , pshw  ,pp_psh ,"push"  , "push(\1);" },                 // 50
{ 14, 0, 1, 1,  0, 1,  0,0, 0xf  , 0     , 0     , pshw  ,pp_psh ,"pusha" , "altpush(\1);" },
{ 15, 1, 0, 1,  0, 1,  0,0, 0xf  , 0     , 0x4f  , popw  ,pp_pop ,"pop"   , "\3 = pop();" },               // 52
{ 15, 0, 1, 1,  0, 1,  0,0, 0xf  , 0     , 0x4f  , popw  ,pp_pop ,"popa"  , "\3 = altpop();" },

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswf ,"jnst"  , "STK" },         // 54  to 71 cond jumps
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswf ,"jst"   , "STK" },          // 55

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswc ,"jnc"   , "CY" },            // 56
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswc ,"jc"    , "CY" },            // 57

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswf ,"jnv"   , "OVF" },           // 58 was pwsv
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswf ,"jv"    , "OVF" },           // 59

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswf ,"jnvt"  , "OVT" },          // 60
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_pswf ,"jvt"   , "OVT" },          // 61

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jgtu"  , ">"   },                     // 62
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jleu"  , "<="  },                     // 63

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jgt"   , ">"   },                 // 64
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jle"   , "<="  },                 // 65

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jge"   , ">="      },                 // 66
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jlt"   , "<"       },                 // 67

{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"je"    , "="       },                 // 68
{ 19, 0, 0, 1,  0, 1,  0,0, 0    , 0     , 0     , cjm   ,pp_cond ,"jne"   , "!="      },                 // 69

{ 23, 0, 0, 1,  0, 2,  0,0, 0xf  , 0x7   , 0     , bjmp  ,pp_bitjmp ,"jnb"   , empty },         // 70   op[2] has reg and bit
{ 23, 0, 0, 1,  0, 2,  0,0, 0xf  , 0x7   , 0     , bjmp  ,pp_bitjmp ,"jb"    , empty },        // 71

{ 18, 0, 0, 1,  0, 2,  0,0, 0x7  , 0x7   , 0x47  , djm   ,pp_djnz ,"djnz"  , empty },           // 72

{ 17, 1, 0, 1,  0, 1,  0,0, 0    ,  0    , 0     , cll   ,pp_call ,"call"  , empty },                        // 73   Long call
{ 17, 0, 1, 1,  0, 1,  0,0, 0    ,  0    , 0     , cll   ,pp_call ,"calla" , empty },                        // 74
{ 16, 0, 0, 1,  0, 1,  0,0, 0    ,  0    , 0    , ljm   ,pp_jmp  ,"jump"  , empty },                        // 75   Long jump

{ 16, 0, 0, 0,  0, 0,  0,0, 0    ,  0    , 0    , skj   ,pp_jmp  ,"skip"  , empty },                       // 76   skip (= sjmp [pc+2])
{ 16, 0, 0, 1,  0, 1,  0,0, 0    ,  0    , 0    , sjm   ,pp_jmp  ,"sjmp"  , empty },                       // 77   short call, jump
{ 17, 1, 0, 1,  0, 1,  0,0, 0    ,  0    , 0    , scl   ,pp_call ,"scall" , empty },                        // 78
{ 17, 0, 0, 1,  0, 1,  0,0, 0    ,  0    , 0    , scl   ,pp_call ,"sclla" , empty },                        // 79

{ 20, 1, 0, 1,  0, 0,  0,0, 0    ,  0    , 0    , ret   ,pp_sce ,"ret"   , "return;" },                   // 80  returns
{ 20, 0, 1, 1,  0, 0,  0,0, 0    ,  0    , 0    , ret   ,pp_sce ,"reta"  , "altreturn;" },                // 81
{ 20, 1, 0, 1,  0, 0,  0,0, 0    ,  0    , 0    , ret   ,pp_sce ,"reti"  , "return;" },
{ 20, 0, 1, 1,  0, 0,  0,0, 0    ,  0    , 0    , ret   ,pp_sce ,"retia" , "altreturn;" },                   // 83

{ 41, 1, 0, 0,  0, 0,  0,0, 0    ,  0    , 0    , pshp  ,pp_sce ,"pushp" , "push(PSW);" },                // 84 push psw
{ 41, 0, 1, 0,  0, 0,  0,0, 0    ,  0    , 0    , pshp  ,pp_sce ,"pshpa" , "altpush(PSW);" },
{ 42, 1, 0, 0,  1, 0,  0,0, 0    ,  0    , 0    , popp  ,pp_sce ,"popp"  , "PSW = pop();" },              // 86
{ 42, 0, 1, 0,  1, 0,  0,0, 0    ,  0    , 0    , popp  ,pp_sce ,"poppa" , "PSW = altpop();" },             // 87


{ 1 , 0, 0, 0,  0, 1,  0,0, 0x7  ,  0    , 0x47 , clr   ,pp_btws ,"clrb"  ,  "\3 = 0;" },                // 88
{ 1 , 0, 0, 0,  0, 1,  0,0, 0xf  ,  0    , 0x4f , clr   ,pp_btws ,"clrw"  ,  "\3 = 0;" },

{ 2 , 0, 0, 0,  1, 1,  0,0, 0x7  ,  0    , 0x47 , neg   ,pp_sce ,"negb"  ,  "\3 = -\1;" },                // 90
{ 2 , 0, 0, 0,  1, 1,  0,0, 0xf  ,  0    , 0x4f , neg   ,pp_sce ,"negw"  ,  "\3 = -\1;" },

{ 25, 0, 0, 0,  1, 1,  0,0, 0x7  ,  0    , 0x47 , inc   ,pp_sce ,"incb"  ,  "\3++;"  },                   // 92
{ 25, 0, 0, 0,  1, 1,  0,0, 0xf  ,  0    , 0x4f , inc   ,pp_sce ,"incw"  ,  "\3++;" },                    // 93
{ 22, 0, 0, 0,  1, 2,  0,0, 0x107,  0x11f, 0x15f, nrm   ,pp_nrm ,"norm"  ,  empty },
{ 2 , 0, 0, 0,  1, 1,  0,0, 0x7  ,  0    , 0x47 , cpl   ,pp_sce ,"cplb"  ,  "\3 = ~\1;" },                // 95
{ 2 , 0, 0, 0,  1, 1,  0,0, 0xf  ,  0    , 0x4f , cpl   ,pp_sce ,"cplw"  ,  "\3 = ~\1;" },

{ 24, 0, 0, 0,  1, 1,  0,0, 0x7 ,   0    , 0x47 , dec   ,pp_sce ,"decb"  ,  "\3--;" },                    // 97
{ 24, 0, 0, 0,  1, 1,  0,0, 0xf ,   0    , 0x4f , dec   ,pp_sce ,"decw"  ,  "\3--;"  },
{ 22, 0, 0, 0,  1, 1,  0,0, 0x7 ,   0    , 0x16f, sex   ,pp_sce ,"sexb"  ,  "\3 = \1;" },
{ 22, 0, 0, 0,  0, 1,  0,0, 0xf ,   0    , 0x17f, sex   ,pp_sce ,"sexw"  ,  "\3 = \1;" },                // 100


{ 21, 0, 0, 0,  1, 0,  0,0, 0,      0    , 0    , stc   ,pp_sce ,"stc"   ,  "CY = 1;" },                  // 101
{ 21, 0, 0, 0,  1, 0,  0,0, 0,      0    , 0    , clc   ,pp_sce ,"clc"   ,  "CY = 0;" },                  // 102
{ 43, 0, 0, 0,  0, 0,  0,0, 0,      0    , 0    , die   ,pp_sce ,"di"    ,  "interrupts OFF;" },
{ 44, 0, 0, 0,  0, 0,  0,0, 0,      0    , 0    , die   ,pp_sce ,"ei"    ,  "interrupts ON;" },
{ 48, 0, 0, 0,  0, 0,  0,0, 0,      0    , 0    , clv   ,pp_sce ,"clrvt"  ,  "OVT = 0;" },
{ 47, 0, 0, 0,  0, 0,  0,0, 0,      0    , 0    , nop   ,pp_sce ,"nop"    ,  empty },

{ 45, 0, 1, 0,  0, 0,  0,0, 0,      0    , 0    , bka   ,pp_sce ,"regbk"  ,  empty },                     // 107
{ 45, 0, 1, 0,  0, 0,  0,0, 0,      0    , 0    , bka   ,pp_sce ,"regbk"  ,  empty },
{ 45, 0, 1, 0,  0, 0,  0,0, 0,      0    , 0    , bka   ,pp_sce ,"regbk"  ,  empty },                     // 109
{ 45, 0, 1, 0,  0, 0,  0,0, 0,      0    , 0    , bka   ,pp_sce ,"regbk"  ,  empty },                     // 110

{ 46, 0, 1, 0,  0, 1,  0,0, 7,      0    , 0    , 0     ,pp_sce ,"rombk"  ,  empty },                     // 111 bank prefix (8065)
{ 40, 0, 0, 0,  0, 0,  0,0, 0,      0    , 0    , 0     ,pp_sce ,"!PRE!"  ,  empty }                      // 112 signed prefix

};


// other preset string lists and definitions in command.cpp or print.cpp


/*
PSW flags, may be needed

bits desc
13-10   Bank select (rombank - one off)
9-8  RAMBANK

bit desc   8061 bottom byte

7  Z zero                ZERO
6  N negative            NEG
5  V overflow            OVF
4  VT oflow trap         OVT
3  C  carry              CY
2  N | Z   Signed <=      (NEG | ZERO)
1  C | !Z   Signed >      ( CY | !ZERO)
0 ST  Sticky          ST


*/


//sfr specs ?
//* by SFR  register - Read,write, byte,word, write mask?

SFRSPEC sfr61 [] {
0x0,  1, 1, 1, 1,
0x1,  1, 1, 1, 1,       //dummy so index works
0x2,  1, 1, 1, 0,
0x3,  1, 1, 1, 0,
0x4,  1, 1, 1, 0,
0x5,  1, 1, 1, 0,
0x6,  1, 0, 0, 1,           // r o word
0x7,  0, 0, 0, 0,
0x8,  1, 1, 1, 0,             //read write byte
0x9,  1, 0, 1, 0,                         //effectively read only
0xa,  1, 0, 1, 0,                //effectively read only
0xb,  1, 0, 1, 0,           //r o byte
0xc,  1, 1, 1, 0,
0xd,  1, 1, 1, 0,
0xe,  1, 1, 0, 1,               // r w word
0xf,  0, 0, 0, 0

};


SFRSPEC sfr65 [] {

0x0,  1, 1, 1, 1,
0x1,  1, 1, 1, 1,       //dummy so index works
0x2,  1, 1, 1, 0,
0x3,  1, 1, 1, 0,
0x4,  1, 1, 1, 0,
0x5,  1, 1, 1, 0,
0x6,  1, 0, 1, 1,          // r o word
0x7,  1, 1, 1, 0,           //r w byte
0x8,  1, 1, 1, 0,             //read write byte
0x9,  1, 0, 1, 0,                          //effectively read only
0xa,  1, 0, 1, 1,                // effectively read only
0xb,  1, 0, 1, 1,           //r w byte
0xc,  1, 1, 1, 1,
0xd,  1, 1, 1, 1,
0xe,  1, 1, 0, 1,              // r w word
0xf,  1, 0, 1, 0,           //r o byte

0x10, 1, 1, 1, 1,            // 0x10  maybe byte OK
0x11, 1, 1, 1, 0,
0x12, 1, 1, 0, 1,            //maybe byte OK
0x13, 1, 1, 1, 0,
0x14, 1, 1, 0, 1,            //maybe byte OK
0x15, 1, 1, 1, 0,
0x16, 1, 1, 1, 1,            //maybe byte OK
0x17, 1, 1, 1, 0,
0x18, 1, 0, 1, 1,          //r o word so how does jnb work ?
0x19, 1, 1, 1, 0,
0x1a, 1, 1, 1, 1,            // byte OK
0x1b, 1, 1, 1, 0,         //80         only bit 7
0x1c, 1, 0, 0, 1,            //r o word
0x1d, 1, 1, 1, 0,
0x1e, 1, 1, 0, 1,            //maybe byte OK
0x1f, 1, 1, 1, 0,
};




 /**************************************************
 * main global variables
 **************************************************/

//time_t  timenow;


/* bank layout.
* Full bank malloced to each relevant bkmap[bankno].fbuf, as 0-0xffff, for max of 16 banks.
* Regs (< 0x400) are mapped to Bank 8. Single banks are mapped completely in bank 8.
*/

BANK bankmap[BMAX+1] = {0};               // for 16 banks max. (+1 means an extra bank 16, or 15 +1)

#ifdef XDBGX
  // debug counters various
  int DBGrecurse  = 0;
  int DBGnumops   = 0;          // num of opcodes
#endif


uint tscans   = 0;            // total scan count, for safety
int  xscans   = 0;            // main loop (for investigation)
int  opcnt    = 0;            // opcode count for emulate
uint anlpass  = 0;            // number of passes done so far
int  numbanks = -1;           // number of banks-1  (so single banks are = 0)
int  recurse  = 0;            // recurse count check

BASP basepars;

// instance holders (each hold one analysed opcode and all its operand data)


INST  cinst;                 // current (default) opcode data holder for std scans
INST  sinst;                 // search instance   opcode data holder (for conditionals, params, etc )
INST  einst;                 // current emulate holder
INST  vinst;                 // vector scans (and test checks)


// special scan blocks, outside standard scan chain

SBK   tmpscan;                // scan block for searches
SBK   *emuscan;               // scan block for imd pushes in emulate

EMULOG emuargs[EMUGSZ];        // tracking argument LBKs, created in emulate, for size fixup

FSTK  scanstk[STKSZ+1];       // fake stack holder for previous subr calls (for arg tracking) [=16 + me at 0]
FSTK  emulstk[STKSZ+1];       // fake stack for emulation - emulate all parameter/argument getters
FSTK  teststk[STKSZ+1];       // fake stack for vect and push testing

uchar *fbinbuf;               // bin file goes in here. Now not freed until end so that FM20M06 ...


/*****************************
external service subroutines
**************************/


BASP *get_basepars(void)
{
  return &basepars;
}


const OPC* get_opc_entry(uchar ix)
{
    if (ix > NC(opctbl)) return NULL;
    return opctbl+ix;
}

uchar get_opc_inx(uchar cd)
{
    return opcind[cd];
}

uint get_anlpass(void)
{
    return anlpass;
}

int get_numbanks(void)
{
    return numbanks;
}

void set_numbanks(int n)
{
    numbanks = n;
}


BANK *get_bankmap(uint bank)
{
    return bankmap + bank ;
}


BANK *mapbank(uint addr)
{
 // if register, force to bank 8

  if (addr < 0x400) return bankmap + 9;
  return bankmap + ((addr >> 16) & 0xf);
}



uchar* map_addr(uint *addr)
 {
  // used by g_word, g_byte, g_val  etc
  // Allow 0 to max bank address
  // registers are always mapped to bank 8

  uint x;
  BANK *b;

  x = *addr;
  *addr = nobank(*addr);   // return addr without bank

  if (*addr < 0x400) return bankmap[9].fbuf;

  b = mapbank(x);

  if (!b->bok || x > b->maxromadd) return NULL;

  return b->fbuf;         // valid address, return pointer

 }





uint bytes(uint fend)
{  //size in bytes. From field end (assumes start at 0)
 fend &= 31;         // drop sign
 //if (!fend) return 0;        //zero is valid !!

 return (fend / 8) + 1;
}



// address handling subrs  0xfffff  (4 bits bank, 16 bits addr in bank)


uint minadd(uint addr)
{  // min address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b->bok) return 0xffff;           // invalid
 return b->minromadd;
}

uint maxadd (uint addr)
{   // max program address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b->bok) return 0;           // invalid
 return b->maxromadd;
}



uint val_rom_addr(uint addr)
 {
 // return 1 if valid 0 if invalid.
 // allows ROM addresses only.
 // check Code, jumps, immds

 BANK *b;

 b = mapbank(addr);

 if (!b->bok) return 0;           // invalid

 if (addr < b->minromadd) return 0;   // minpc = PCORG
 if (addr > b->maxromadd) return 0;   // where fill starts

 return 1;        // within valid range, OK
 }

// uint valid_addr  may as well use map_addr






uint valid_reg(uint reg)
{
  // return 0 for invalid,
  // 1 for general register
  // 2 for zero reg
  // 3 for SFR
  // 4 for stack register

  reg = nobank(reg);

  if (get_cmdopt(OPT8065))
    {  // 8065
     if (reg > 0x3ff) return 0;          // invalid
     if (!(reg & 0xfe) ) return 2;       // zero reg = R0, R1, R100/1, R200/1, R300/1
     if (reg > 0x23)  return 1;          // general
     if (reg < 0x20)  return 3;          // SFR
     return 4;                           // stack registers
    }
  else
  {    // 8061
    if (reg > 0xff)  return 0;            // invalid
    if (reg < 2)     return 2;            // zero reg = R0, R1
    if (reg > 0x11)  return 1;            // general
    if (reg < 0x10)  return 3;            // SFR
    return 4;                             // stack
  }
  return 0;        //safety
}


// ---------------------------- fieldwidth and masks etc




uint get_signmask(uint fend)
{  // calc mask for sign bit

 if (!(fend & 32)) return 0;      // not signed

 return (1 << fend);

}


uint get_sizemask(uint fend)
{  // calc size mask
 uint ans;

 // or is this (1 << (fend+1)) - 1; ??

 fend &= 31;
 ans = 1;

 while (fend--)
  {
    ans <<= 1;
    ans |= 1;
  }

 return ans;
}


uint get_startval(uint fend)
{  // calc startval for functions
 uint ans;

 ans = get_sizemask(fend);

 if (fend < 32) return ans;

 ans >>= 1;    // one bit less for sign bit

return ans;

}





//----------------------------------------------------------

uint addbank(uint addr, CINST *c, uint bkset)
  {

   addr = nobank(addr);
   if (valid_reg(addr)) return addr;


// redundant ???
   if (!numbanks) return addr |= basepars.datbnk;

   if (c && c->bank) return addr | (c->bank << 16);
   return  addr | bkset;
 }


uint codebank(uint addr, CINST *c)
 {
   // get BANK opcode to override default
   return addbank(addr,(CINST*) c,basepars.codbnk);
 }

uint databank(uint addr, CINST *c)
 {
   // get BANK opcode to override default
   return addbank(addr,(CINST*) c,basepars.datbnk);
 }



int g_byte (uint addr)
{                    // addr is  encoded bank | addr
  uchar *b;

  b = map_addr(&addr);
  if (!b) return 0;        // safety
  return (int) b[addr];
}


uint g_word (uint addr)
{         // addr is  encoded bank | addr
  uint val;
  uchar *b;

  b = map_addr(&addr);                      // map to required bank
  if (!b)  return 0;

  val =  b[addr+1];
  val =  val << 8;
  val |= b[addr];
  return val;

 }


FSTK*  get_stack(uint proctype, uint ix)
{
  // so get_stack(s, 0) returns base stack (actually entry [1]),
  // (-1) would allow 'me' to be put on virtual bottom of stack (at [15]).

  FSTK *t;

  t = 0;

  switch(proctype)
   {
    default:
        t = scanstk;
        break;

    case 3:
    case 5:
        t = teststk;
        break;

    case 4:
        t = emulstk;
        break;
   }

  ix++;                 // move up one ?
  ix &= 0xf;            //15 entries with wrap

  return  t+ix;



}


int sjmp_ofst (uint addr)
{  // negate value according to sign bit position
   // used ONLY for jumps
   // special 11 bit signed field.....

  int ofs;

  ofs = g_byte(addr) & 7;                // bottom 3 bits
  ofs <<= 8;
  ofs |= g_byte(addr+1);                 // and next byte
  if (ofs & 0x400) ofs |= 0xfffffc00;    // negate if sign bit set

  return ofs;
}


int upd_ram(uint add, int val, uint fend)
 {
  int ans;
  uint i, sz;
  uchar *b;

//may need to make this bitwise ??

  if (valid_reg(add) == 2) return 0;      // R0/1, 100/1, 200, 300 all fixed at zero
  if (val_rom_addr(add))
    {
      #ifdef XDBGX
      DBGPRT(0,"!! INV ROM WRITE %x = %x",add, val);
      #endif
      return 0;                                // No writes to ROM
    }

  b = map_addr(&add);
  if (!b) return 0;        // safety

  ans = 0;
  sz = bytes(fend);                            // size in bytes

  for (i = add; i < add + sz; i++)
    {
      uchar x;
      x = b[i];                      // old value
      b[i] = val & 0xff;
      if (x != b[i])  ans = 1;       // value has changed
      val >>= 8;
    }

  return ans;
 }


int check_argdiff(int nv, int ov)
{
  // value difference for arguments
  // nv is new value, ov is original

  int cnt;

  cnt = nobank(nv) - nobank(ov);    //is this wrong for multibanks ?
  if (cnt > 0 && cnt < 32) return cnt;

  #ifdef XDBGX
  if (nv != ov) DBGPRT(1,"Argdiff reject %x %x", nv,ov);
  #endif
  return 0;
}





uint rb_invchk(SBK *s, CINST *c)
{
  COPER *o;
  RBT *b;
  uint val;

   // check if rbase not valid

   if (!(s->proctype == 2)) return 1;        // only in scan mode, ignore tests too


   o = c->opnd + 3;

   if (valid_reg(o->reg) != 1) return 0;      // general reg only
   if (o->optype)  return 0;                  // plain register only for destination

   b = get_rbt(o->reg,c->ofst);                 // is it an rbase (default)?

   if (!b) return 0;

 //  if (b)
  //   {
       if (b->usrcmd)         //b->lock ??
        {
          #ifdef XDBGX
           DBGPRT(1,"usrcmd bans rbase update %x", o->reg);
          #endif
          return 1;
        }

       if (b->lock)
        {
          #ifdef XDBGX
           DBGPRT(1,"Lock bans rbase update %x", o->reg);
          #endif
          return 1;
        }



  //   }

  // rbase exists, invalidate if not same value or not immediate
  // or if cmpare opcsub = 0 ??

   val = databank(o->val,c);          // val is an address, add bank   NO !! [1]
   //may need more for compares yet....

   if (b->val != val || c->opcsub != 1)       // reg not = an immediate
      {
        b->ucnt++;
        if (b->ucnt > 2)
          {
            b->oinv = 1;
            b->invat = c->ofst;
            #ifdef XDBGX
              DBGPRT(1,"Inv R%x (%x)", o->reg, c->ofst);
            #endif
          }
      }            // end check rbase
  return 0;        // update allowed
}



SBK* do_code (SBK *s, INST *c);

void do_one_opcode(uint xofst, SBK* ts, INST *dest)
  {
      //can't do get_opcode, because autoincs etc.
   memset(ts,0,sizeof(SBK));    // safety clear
   ts->curaddr  = xofst;
   ts->nextaddr = xofst;
   ts->start    = xofst;
   do_code(ts,dest);
  }



int mark_emu(SBK *s, FSTK *t, uint i)
 {
  // get the calling proc from opcode before any new args
  // and flag it as emulate reqd
  // called from check_emuargs (scans - upd_watch)
  // and do_args (emulate)

   SBK *x;
   SUB * sub;
   uint ans, ofst;
   CINST *c;

    #ifdef XDBGX
      DBGPRT(0,"In Mark_emu (ARGS) for %x (%d)", s->start, t->ftype);
      if (t && t->sblk) DBGPRT(1," FSTK %x %x", t->sblk->nextaddr, t->sblk->start); else DBGPRT(1, " NO addr ");
    #endif

   if (t->ftype > 3) return 0;           //not pushp or similar

/*test2   see if we can find same block via stack (caller)
// this seems to work... with sbk added, so we even need to check it's a CALL ??

x = s;                //current block

while (x)
{
    if (x->start == t->sblk->start)
    {
      #ifdef XDBGX
      DBGPRT(1,"%x found via caller (%x=%x", x->nextaddr, x->curaddr, g_byte(x->curaddr));
      #endif
      break;
    }
 x = x->caller;
}  */


   ans = 0;

   if (!t || !t->sblk) return 0;

   ofst = t->sblk->nextaddr;          //origadd;          // DO need scan stack therefore

   if (!ofst) return 0;

  // do_one opcode works in test mocde, whereas find_opcode may not.  //this works for A(L non test
//  do_one_opcode(t->sblk->curaddr, &tmpscan, &sinst);        //or should do a sig ??

   //  #ifdef XDBGX
   //   DBGPRT(1,"%x one opcode, sig = %d addr = %x", t->sblk->curaddr, sinst.sigix, sinst.opnd[1].addr);
   //   #endif


   c = find_opcode(ofst, 0);    // opcode before args for subr call - but this stops parallel chain TEST working
   //but this just drops back to t->sblk->curraddr ? Not

   if (!c || c->sigix != 17)
// if (sinst.sigix != 17)
     {
      #ifdef XDBGX
      DBGPRT(1,"Call not found !!");
      #endif

      return 0;
     }
   // CALL,

   // ofst = sinst.ofst;
   ofst = c->ofst;
   x = get_scan(c->opnd[1].addr,c->opnd[1].addr);      // make sure this is subr call

 // x = get_scan(sinst.opnd[1].addr,sinst.opnd[1].addr);      // make sure this is subr call



   if (x) { ans = x->start;

           #ifdef XDBGX
           DBGPRT(1,"call got = %x", x->start);
            #endif
   }

   sub = get_subr(sinst.opnd[1].addr);

   if (sub && sub->size)
      {
        #ifdef XDBGX
         DBGPRT(1,"args ignored - size set %d", sub->size);           //size set, no emul
        #endif
        return ans;
      }

//but this might not be correct x !!

//x can be equal to s, but maybe better to use stack to sort this out ?

      if (x && !x->emulreqd)
        {
         #ifdef XDBGX
           DBGPRT(1,"SET EMUREQD for sub %x substart %x", x->start, x->substart);
           DBGPRT(1,"SET ARGSGET for sub %x substart %x", s->start, s->substart);
         #endif
         x->emulreqd = 1;           // this is where args attached
         s->argsget  = 1;           // and mark caller
        }

//emulate here ??



   return ans;
  }








void check_emuargs(SBK *s, OPER *o)
  {
    // check if this register qualifies the subroutine for EMULATE
    // i.e. is popped and modified....

    // only used in scanning mode. Called from upd_watch.
    //may be able to ditch this one in favour of 'after proc' for scanning.


   int i, args;
   FSTK *t;
   RST *r;

   if (valid_reg(o->reg) == 2) return;    // not for R0

   r = get_rgstat(o->reg);           //, s->substart);

   if (!r || !r->popped) return;              // not popped

 //  r->ofst = o->val;                   // update to inc'ed address
 //  r->inc = 1;                         // set inc
   r->fend = 15;                       // autoincs are ALWAYS word ??



  if (!(s->proctype & 2)) return;       // scanning (+test) only


   #ifdef XDBGX
     DBGPRT(1,"%x  R%x check emuargs", s->curaddr, o->reg);
   #endif

   // subroutine immediately before the ARGS is the one which needs to be
   // flagged as EMUREQD and THIS subr flagged as an ARGS getter
   // could do this on the PUSH ??
   //unlike scan, cannot have a zero.

   args = 0;

   for (i=STKSZ-1; i >= 0; i--)
      {
        t = get_stack(s->proctype,i);                             // this is UP the scan mode call chain

        if (t->ftype == 1 && t->popreg == o->reg)
          {
            args = check_argdiff(o->val, t->sblk->nextaddr);     // found register (and popped addr)
             if (args)
                {
                  #ifdef XDBGX
                   DBGPRT(0,"found FSTK [%d] for R%x (%d) %d args", i, o->reg, args);
                   if (t->sblk) DBGPRT(0," FSTK = %x", t->sblk->nextaddr);
                   DBGPRT(1,0);
                   #endif
                   break;
                }
           }
      }

   // don't actually have to find a stack entry if scanning
   // check against zero address

   if (!args)
   {
     args = check_argdiff(o->val, 0);
     #ifdef XDBGX
        if (args) DBGPRT(1,"%x No FSTK (%d args)", s->curaddr, args);
     #endif
  }

   if (args)
    {
     // flag this subr doing the arg get so that
     // calling it will trigger emu of the caller.
     mark_emu(s, t, i);  // seems to be OK even if not found (by i)
    }

  }




void upd_watch(SBK *s, INST *c)
 {
   //last thing to check after everything else. RAM updates and autoinc,
   //but can change values here as opcode already saved

  OPER *o;

  if (!s->proctype) return;          // not sig checks


  o = c->opnd + 3;                       // destination entry (write op)

  //but some opcodes update other things too....(norm, div)

  rb_invchk(s,c);            // generic check for invalidate

  if (o->fend & 0x40)        // writes to somewhere...
    {
      if (upd_ram(o->addr,o->val,o->fend))
         {
          #ifdef XDBGX
           DBGPRT(0,"%05x ",s->curaddr);
           if (s->proctype & 1) DBGPRT(0, "Alt ");                 // emulating
           if (s->proctype & 4) DBGPRT(0, "E ");                 // emulating

           DBGPRT(0, "RAM %x = %x (%d)", o->addr, o->val, o->fend);
           if (c->opcsub > 1)  DBGPRT(0, "[%x]", o->addr);      // for indirect
           DBGPRT(1,0);
          #endif
         }
    }                // end if write op


  if (c->opcsub != 2) return;

  // indirect only from here

  // stx can autoinc op[3], as ops are swopped, all others are op[1]

  if (c->sigix != 13)  o = c->opnd + 1;              // sce entry

  if (o->optype != OPAINC) return;    // Not autoinc - ignore

  // addr & val replaced with indirect ones.  Put originals back for increment
  o->val  = o->addr;                // value = address accessed
  o->addr = o->reg;                 // restore orig register address

  // kill rbase if increment
  if (o->rbs)
    {
      RBT *b;
      b = get_rbt(o->reg,0);
      if (b && !b->usrcmd && !b->lock)
        {
          b->oinv = 1;      // cannot increment an rbase
          b->invat = c->ofst;
          #ifdef XDBGX
               DBGPRT(1,"Inv r%x (%x) ++", o->addr, c->ofst);
          #endif
        }
    }

  o->val += bytes(o->fend);       // autoinc is by op size

  upd_ram(o->reg,o->val,15);        // word update

  check_emuargs(s,o);               // check if popped register is changed - move some to push ?? always dest +3

  //maybe kick off emulate HERE !!! or at the push ??

  #ifdef XDBGX
      DBGPRT(1, "%05x R%x++ = %x ",s->curaddr,  o->reg, o->val);
  #endif

}



void upd_regsizes(SBK *s,INST *c)
 {

   RST *r;

// check only that -
// [1] is an arg, do readsize
// [2] is an arg, do readsize
// [3] cancels arg status

// register size arguments

//note - opcsub 2 and 3 demand that op [1] is WORD

//may be more rules yet....

//use fend &0x40 as write ?? always [3] but may be useful for a loop...


if (c->numops == 1)
 {          //single op is always write.....clear arg.
   r = get_rgstat(c->opnd[3].addr);

    if (r && r->arg){ r->arg = 0;     // clear arg status if written to.
  //  DBGPRT(1, "%x clear 1 arg R%x", s->curaddr, r->reg);
  }

  }

if (c->opnd[3].reg != c->opnd[1].reg)
  {     // extra check for R38 = [R38]....as xdt2 and eqe3
    r = get_rgstat(c->opnd[3].addr);

    if (r && r->arg && r->cofst != s->curaddr){ r->arg = 0;     // clear arg status if written to.
 //   DBGPRT(1, "%x clear N arg R%x", s->curaddr, r->reg);
 }

  }

    r = get_rgstat(c->opnd[2].reg);

    if (r && r->arg && r->fend < c->opnd[2].fend) { r->fend = c->opnd[2].fend;
 //  DBGPRT(1, "%x argsz R%x = %d", s->curaddr, r->reg, r->fend);
    }
    r = get_rgstat(c->opnd[1].reg);

    if (r && r->arg)
      {

        if (c->opcsub > 1)
           {
            r->fend = 15;         // always word NO.... make opcode size...
            r->ref = 1;           // used as address
           }
        else
          {
        if (r->fend < c->opnd[1].fend)  r->fend = c->opnd[1].fend;
          }
   //         DBGPRT(1, "%x argsz R%x = %d", s->curaddr, r->reg, r->fend);
      }

    r = get_rgstat(c->opnd[3].addr);

    if (r && r->arg && r->cofst != s->curaddr) {r->arg = 0;     // clear arg status if written to.
 // DBGPRT(1, "%x clear arg R%x", s->curaddr, r->reg);
 }
}




uint chk_inv_ops(SBK *s, INST *c)
{
  OPER *o;
  SFRSPEC *p;

  uint i,error, x, fend;

  error = 0;

  for (i=1; i <= c->numops; i++)
   {
    o = c->opnd + i;
    if (error) break;

    x = valid_reg(o->reg);

    if (x == 3)           //SFR
     {
       if (get_cmdopt(OPT8065)) p = sfr65 + o->reg; else p = sfr61 + o->reg;

       fend = o->fend & 31;              //drop flags.

       //generic checks first
       if (o->optype == OPIND) error = 1;                      // sfr indirect
       if (o->optype == OPINX) error = 2;                      // sfr indexed
       if (o->optype == OPAINC && o->reg != 5) error = 3;      // only watchdog incremented

if (c->sigix != 12 && c->sigix !=13 && c->sigix !=1 && c->sigix != 5 && c->sigix != 9 && c->sigix != 23 && c->sigix != 10
&& c->sigix !=6  && c->sigix != 7 &&c->sigix !=25)  error = 33;              //test


// clrb, xor,jb, cmp....

//opcodes ? shifts make no sense, mult/div, norm, cpl neg, (add, sub do for times)
// or/and maybe      inc for r5. but


       if (o->fend & 64)
          {
            if(!p->write) error = 4;     // no write
          }
       else
          {
            if(!p->read) error = 5;     // no read
          }

       if (fend == 7 && !p->byte) error = 6;         // byte only
       if (fend == 15 && !p->word) error = 7;        // word only
       if (fend > 15)  error = 8;

     }
   }

     // specific ops outside the loop....

     if (get_cmdopt(OPT8065))
       {       //extra write checks
         o = c->opnd + 3;
         if (o->fend & 64)
           {      // write ops to SFRs
             if (o->reg == 0xa  && (c->opnd[1].val & 0xef)) error = 9;   // only bit 4 (=0x10) for RA  (mem_expand)
             if (o->reg == 0x1b && (c->opnd[1].val & 0x7f)) error = 10;   // only bit 7 (=0x80) for R1b  (HOS used)
           }
       }

  if (error)
    {
    #ifdef XDBGX
    if (s->proctype)
    {
     DBGPRT(1, "\nZSZS %d INV opcode %x R%x (%x)", error, s->curaddr, o->reg, p->reg);
    }
    #endif

   }
  return error;
}


uint check_skip(SBK *s, INST *c)
 {
   /* if opcode = skip, check that next opcode is valid. only a single byte (i.e. no operands) or SIGN (0xfe)
   *  otherwise skip MUST be an invalid opcode. Other opcodes make no sense too (ret, skip, etc)
   *  assumes this is not a single data byte (at zero,  could be...
   *  some bins do a skip over the SIGN prefix - not just a skip, could be a goto.....
   */

   const OPC *n;
   uint ix;

   if (!s->proctype) return c->opcix;

   if (c->opcix == 76)
    {
      ix = opcind[g_byte(s->nextaddr+1)];
      n = opctbl + ix;

      //legal to skip an fe prefix.

     if ((ix >= 106 && ix != 111) || ix == 76 || n->sigix == 20 || n->nops)
     {
      #ifdef XDBGX
        DBGPRT(0,"!INV! opcode [Skip] + %s  ", n->name);
       #endif
       return 0;
     }

    }
 return c->opcix;

 }


uint do_opcode_prefix(SBK *s, INST *c)
{
   //sort out any prefixes and opcode.
   // use error >=200 ?


//opnd[0] for temp holders ?? val, addr ?

  uint xofst, indx, tbk;
  OPC *opl;

  xofst = s->curaddr;                     // don't modify curaddr, use temp instead

  c->opsize = 1;
  c->opcode = g_byte(xofst);              // first byte, opcode or prefix

  indx = opcind[c->opcode];               // opcode table index
  opl = opctbl + indx;

 // if (!indx) return indx;                  // invalid opcode

  // Ford book states opcodes can have rombank(bank) AND fe as prefix (= 3 bytes)

  if (c->opcode == 0x10)                  // bank swop - do as prefix x = 0x10
    {
      if (!get_cmdopt(OPT8065)) return 0;          // not valid for 8061

      c->opcode = g_byte(xofst+2);                   // opcode or sign

      if (c->opcode == 0xfe)
        {                                    // sign prefix after rombank
          c->feflg = 1;                      // set sign flag
          c->opcode = g_byte(xofst+3);               // mark and skip sign
        }

      indx = opcind[c->opcode];
      opl = opctbl + indx;                  // map to correct opcode

      if (!opl->rbnk) return 0;           // rombank prefix not valid for opcode

      tbk = g_byte(xofst+1);           //bank no after prefix

      if (tbk & 0xf6)
           {                      //use mask as banks not always decided yet (0,1,8,9)
           #ifdef XDBGX
           if (s->proctype) DBGPRT(1,"ROMBNK - Invalid bank %d (%x)", tbk, c->ofst);
           #endif
         return 0;                     // invalid bank
        }

      tbk &= 0xf;
                                  //could use sigix...or even opcix temp....
      c->bank = tbk+1;                               // set bank for this opcode 4 bits
      c->opsize +=2;
      xofst +=2;
                                   //skip prefix+bank
      if (c->feflg) {xofst++;  c->opsize++; indx += 1; }

    }

  if (c->opcode == 0xfe)            //indx == 111)          // x = 0xfe
    {                                  // this is the SIGN prefix (0xfe), find signed/alt opcode

      c->opcode = g_byte(xofst+1);
      indx = opcind[c->opcode];                 // get opcode
      opl = opctbl + indx;
      xofst++;

      if (! opl->sign) return 0;      // sign prefix not valid

           indx += 1;                   // get prefix opcode index instead
           c->feflg = 1;
           c->opsize ++;

     }

  if (opl->p65 && !get_cmdopt(OPT8065)) return 0;     // 8065 opcode only

  if (indx > 110) return 0;         // safety if totally lost.....

//opcode already set...

  c->opcsub   = c->opcode & 3;                               // this is valid only for multimode opcodes
  c->opcix    = indx;                                 // opcode index
  c->sigix    = opl->sigix;                           // sig index

  c->ofst     = s->curaddr;                           // original start of prefix+opcode
  c->numops   = opl->nops;
  c->opsize  += c->numops;
  s->nextaddr = xofst;                      // address of actual opcode after prefixes - for eml calls
                                            // curaddr is start of prefixes.

  return indx;

}





SBK* do_code (SBK *s, INST *c)

{
     /* analyse next code statement from ofst and print operands
      * s is a scan blk, c is holding instance for decoded op.
      * pp-code calls this with a dummy scan block in print phase
      */

  uint i, indx;
  OPC *opl;
  OPER *o;
  SBK* ans;

  basepars.codbnk = g_bank(s->curaddr);                // current code bank from ofst

  if (s->stop)
   {
    #ifdef XDBGX
     if (s->tstvect) DBGPRT(0,"Test ");
     DBGPRT(1,"Already Scanned s %x c %x e %x", s->start, s->curaddr, s->nextaddr);
    #endif
    return s;
   }

    if (s->curaddr > maxadd(s->start) || s->curaddr < minadd(s->start))
    {
      #ifdef XDBGX
           if (s->tstvect) DBGPRT(0,"Test ");
      if (anlpass)  DBGPRT(1,"Scan > Illegal address %x from blk %x, STOP", s->curaddr, s->start);
      #endif
      s->stop = 1;
      return s;
    }


  if (!val_rom_addr(s->curaddr))
    {
     #ifdef XDBGX
          if (s->tstvect) DBGPRT(0,"Test ");
     if (anlpass) DBGPRT(1,"Invalid code address %05x", s->curaddr);
     #endif
     s->inv = 1;
     s->stop = 1;
     s->nextaddr = s->curaddr+1;
     return s;
    }



//----------------------------

  memset(c,0,sizeof(INST));                    // clear INST entry

  if (s->proctype == 2) c->save = 1;           // save for real scan ONLY

  indx = do_opcode_prefix(s, c);

  indx = check_skip(s,c);

  if (!indx)          // error
  {

    s->stop = 1;
    c->save = 0;                          //don't save it. (safety)
    if (s->proctype)     s->inv = 1;
    s->nextaddr += 2;      // to get nextaddr right for end_scan



   if (s->proctype)               // not if signature checking or printing
     {
       #ifdef XDBGX
            if (s->tstvect) DBGPRT(0,"Test ");
         DBGPRT(1,"Invalid opcode (%x) at %x P%d A_%d", g_byte(s->nextaddr), s->curaddr, s->proctype, anlpass);
       #endif


  //     if (s->tstvect) cleantestdata(s);     //send failed block in No, do in scan_blk....


     }

    if (anlpass >= ANLPRT )  xprt(MSGFILE, 1,"Invalid opcode at %x [P%d]", s->curaddr, s->proctype);   // report only once (in print)

   return s;
  }

//------------
 opl = opctbl + indx;

  o = c->opnd;                                     // shortcut to operands (4 off)

  for (i = 0; i < 4; i++) o[i].fend = opl->fend[i];



//  if (xofst == 0x96ead && s->proctype == 1)
//{
// DBGPRT(0,0);
// }


  ans = opl->eml(s, c);                     // opcode handler - modifies nextaddr, maybe new scan block
                                            // s->nextaddr now points to next opcode

  chk_inv_ops(s, c);       // don't link in to invalid yet...TRY HERE !!


  if (c->save) add_opcode(c);              // save opcode if not done yet

  upd_watch(s, c);

//  if (s->logdata)
 if (s->proctype) add_dtr(s,c);                // data tracking (emulate too ??)

  if (s->proctype & 4) upd_regsizes(s,c);       // emulate only

 s->curaddr = s->nextaddr;                 // next opcode address

 return ans;                               // DOES return SBK from opcode handler...

 }
















void log_emuargs(LBK *l, SBK *s)
{
int i;

if (!l) return;

for (i = 0; i < EMUGSZ; i++)
  {
   if (emuargs[i].cmd == l) break;     // already logged

   if (!emuargs[i].cmd)
     {
      emuargs[i].cmd = l;
      emuargs[i].sblk = s;
      #ifdef XDBGX
      DBGPRT(1,"log_emu %x = %d",l->start,i);
      #endif
      break;
     }
  }
     #ifdef XDBGX
if (i >= EMUGSZ) DBGPRT(1,"NO SLOTS FOR LOG EMU");
  #endif

}


int cellsize(ADT *a)
{
  // byte size of single ADT entry
 if (!a) return 0;
 return a->cnt * (bytes(a->fend));
}


int listsize(void *x)
{
 // list size to next newline (last par).
 // may continue (via seq) from last newline
 // use chain.lastix in case it hits end

 return adtchnsize(x, 1);

}

int totsize(void *x)
{
 // total size of aditional data chain
 // ignore newlines
 return adtchnsize(x, 0);
}





void do_args(INST *c, SBK *s, FSTK *t, uint i)
 {
    // called from pushw or stx in emulate only.
    // this adds args command and base ADT
    // but not if subr has args attached ?? how to get this ???

   LBK  *l;
//   SBK *s;
   OPER  *o;
   uint args;
   uint addr, ix;

 //  s = c->scanblk;
   o = c->opnd + 1;

   addr = o->val | g_bank(t->sblk->nextaddr);
   args = check_argdiff(addr, t->sblk->nextaddr);
   if (!args) return;

 //  addr =
 mark_emu(s,t,i);                     // mark calling subr for args

  // if (!addr) return;

   // probably, data entries will have been created for reqd args.
   // Delete them first.........
//  for (addr = t->origadd; addr < t->origadd+args; addr++)

   for (addr = t->sblk->nextaddr; addr < t->sblk->nextaddr+args; addr++)
   {


     l = get_cmd(addr, 0);  //what - any command ??
     if (l)
       { //what about any adts?
        CHAIN *x;
        x = get_chain(CHADNL);
        addr = l->end; // skip to end of this block
        if (l->fcom <= C_LONG) chdelete(x, x->lastix,1);
       }
   }
 //  t->origadd += args;

   l = get_aux_cmd(t->sblk->nextaddr,0);                   // any command here ?

   if (l && l->usrcmd)
    {
      #ifdef XDBGX
        DBGPRT(0,"USER CMD (AUX) for %x (%x-%x)", t->sblk->nextaddr, l->start, l->end);
      #endif
      return;
    }

   if (l)
     {
      if (l->fcom == C_ARGS)
        {    // a further set, probably call same getter again
             // THIS DOESN'T CHECK FOR OVERLAPS !!!
         int tsz;
         #ifdef XDBGX
          DBGPRT(1,"Found ARGS CMD %x-%x (%d)", l->start, l->end, l->size);
        #endif
        tsz = totsize(vconvi(l->start));         //debug test
         if (tsz != l->size)
           {
#ifdef XDBGX
            DBGPRT(1,"PANIC - sz no match, %d %d", l->size, tsz);
#endif
           }

//have curaddr.......

    //     if (add_args(l, t->popreg, args) > 0)   // NOT POPREG  gabble !!
    //     {
     //    #ifdef XDBGX
     //    DBGPRT(1,"ARGS-extend to %d, %x-%x", args, l->start, l->end);
    //     #endif
     //    }

        }
     }
   else
     {    // aux cmd not found, add a new args command


     // check if aux is adjacent here....normally merge is banned, but makes sense here
     // looked for t->origadd above, so don't need to search again.

     ix = get_lastix(CHAUX);
     ix--;                            //safe if underflows
     l = (LBK*) get_chitem(CHAUX, ix);

     addr = (o->val | g_bank(t->sblk->nextaddr))-1;

     if (l && (l->end+1) == t->sblk->nextaddr)
       {
          l->size += args;        //args extend.
          l->end += args;

               #ifdef XDBGX
        DBGPRT(1,"ARGS extend %d, %x-%x", l->size, t->sblk->nextaddr, addr);
       #endif
       }

     else
      {
      l = add_aux_cmd(t->sblk->nextaddr, addr, C_ARGS);

      if (l) { l->size = args;                               // temp safety for skips
      if (get_cmdopt(OPTCMPA))  l->cptl = 1;           //compact layout
       #ifdef XDBGX
        DBGPRT(1,"ARGS create %d, %x-%x", args, t->sblk->nextaddr, addr);
       #endif
      }
   //   else DBGPRT(1, "**** add args FAILS %d, %x-%x", args, t->sblk->nextaddr, addr);

     }
        t->sblk->nextaddr += args;               //this seems to work.... can't update BOTH !!

     }
   log_emuargs(l,t->sblk);

 }



void rb_addchk (SBK *s, CINST *c)
{
  COPER *o, *v;
  RBT *b;     //, *x;

   // check to ADD a rbase

   if (s->proctype != 2)  return;          // only in scan mode
   if (c->opcsub != 1) return;             // immediates only

   o = c->opnd + 3;
   v = c->opnd + 1;

   if (valid_reg(o->reg) != 1) return ;    // general reg only

   if (o->optype)  return ;                // plain register only

   b = get_rbt(o->reg,c->ofst);            // is it an rbase (default)?

   if (b) return;                          // already has rbase

   // this allows R76 in AA.................

   // ldw, stw or ad3w and immediate - new rbase candidate
   // 41  LDW       49   STW          25 ad3w

   if (c->opcix == 41 || c->opcix == 49)
     {
       if (valid_reg(v->val)) return;          // can't point to another reg
       b = add_rbase(o->reg, databank(v->val,c),0,0xfffff);
     }

   if (c->opcix == 25)
     {                   // ad3w only if (rbase + immediate)
      b = get_rbt (c->opnd[2].reg,c->ofst);          // is [2] an rbase ?
      if (b) b = add_rbase(o->reg, c->opnd[1].val + b->val,0,0xfffff);
     }
   if (b)
      {
       b->setat = c->ofst;
       if (s->tstvect) b->test = 1;
      }
   }







//------------------------------------------------------------------------







void rbasp(SIG *sx, SBK *b)
{

  //add rbase entries from signature match

  uint add, add2, reg, i, cnt;   //,xcnt,rcnt;
  RBT *r;
  uint  pc;
  LBK* l;
  ADT *a;

  if (b) return;                  // no processing if subr set

 // rcnt = chbase.num;              // may have rbases already. start at 'NEW' point
  reg = (sx->vx[1]);                // & max_reg());
  add = sx->vx[3];
  cnt = g_byte(add);              // no of entries in list

  #ifdef XDBGX
  DBGPRT(1,"In rbasp - begin reg %x at %x, cnt %d", reg, add, cnt);
  #endif

  if (!valid_reg(reg))
  {
     #ifdef XDBGX
      DBGPRT(1,"Invalid register (%x)", reg);
     #endif
    return ;
   }

  add_cmd(add, add+1, C_BYTE|C_SYS);
  l = add_cmd(add+2, add+(cnt*2)+1, C_WORD|C_SYS);

  if (get_lasterr(CHCMD))                     //chcmd.lasterr)
   {
     #ifdef XDBGX
      DBGPRT(1,"Rbase Already Processed");
     #endif
    return ;
   }


  if (l)

 /*   { a = append_adt(vconvi(l->start),0); //old setup
  if (a)
   { a->fend = 15;
     a->vaddr = 1;
     a->prdx = 0;
     a->bank = basepars.datbnk >> 16;               // add + default data bank
       a->pfw   = get_pfwdef(a);
   }
    }  */

   {
 a = (ADT*) chimem(CHADNL);           //assemble ADT for insert


  if (a)
   { a->fid = vconvi(l->start);
     a->cnt    = 1;
     a->fend   = 15;
     a->bank   = l->start >> 16; //databank(l->start,0);       ??
  //   a->vaddr = 1;
     a->dptype = DP_DADD;                 //prdx = 0;

     a->pfw   = get_pfwdef(a);
     append_adt(a);                               // (vconvi(l->start),0);
   }
    }
  pc= add+2;

 //  check bank is correct ?
    add = g_word(pc);                       // register pointer
    add |= basepars.datbnk;                 // + default data bank
    add2 = g_word(add);                     // check pointer is consistent with next entry
    i = g_word(pc+2);                       // next in list

  #ifdef XDBGX
  if (i != add2)
    {
     // rbases point to different bank
     DBGPRT(1,"RBASEDBNK %x != %x", i, add2);      //i = 3130
  //   find_data_bank();
    }
  #endif

  for (i = 0; i < cnt; i++)
   {
    add = g_word(pc);                       // register pointer
    add |= basepars.datbnk;               // add + default data bank

   if (nobank(add) < 0x2020) break;
   add_cmd(add, add+1, C_WORD|C_SYS);
   add2 = g_word(add);                     // check pointer is consistent with next entry
    // also valid for last pointer for xcode

   r =  add_rbase(reg,add,0,0xfffff);                     // dbg message in here....
   if (r) r->lock = 1;                                    // not as high as user, next level
   upd_ram(reg,add,15);
   add_aux_cmd(add,add2-1, C_XCODE|C_SYS);          // and add an XCODE as this is data

    reg+=2;
    pc+=2;
   }

 /* now check pointers for XCODE directives for newly added entries

 if (rcnt == chbase.num)  return ;     // nothing added

 for (i = rcnt; i < rcnt+cnt; i++)
   {
     if (i >= chbase.num)  break;
    r = (RBT*) chbase.ptrs[i];
    n = (RBT*) chbase.ptrs[i+1];
    pc = n->rval & 0xffff;

    xcnt =0;

    if (i >= rcnt+cnt-1)
     {
      n = (RBT*) chbase.ptrs[i-1];                  // get bank of last pointer
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
		#ifdef XDBGX
        DBGPRT(1,"Matching rbase addr not found !");
		#endif
    }
    else
      {
   //     if (xcnt) DBGPRTN("Rbase %x = %x",r->radd, r->rval);
      //  set_cmd(r->rval, r->rval+1, C_WORD);
     //   set_auxcmd(r->rval,pc-1, C_XCODE);          // and add an XCODE as this is data
        add_data_cmd(val, val+1, C_WORD|C_CMD,0);
        add_code_cmd(val,val2-1, C_XCODE);          // and add an XCODE as this is data
      }
   }
   sx->done = 1;
   #ifdef XDBGX
   DBGPRT(1,"end rbasp");
   #endif
   return ;*/
}


/*
void check_rbase(void)       //SIG *sx, SBK *dummy)
{

// rewrite to ditch the sig scan
// CAN'T DO THIS - 0FAB does not use f0 as start point !!!
 // must go back to signature !!

  //check if rbase caclcon pointer entries are present
  // 2020 num of levels (always 8)
  // 2021 no of calibrations (1)
  // 2022-20A1 cal pointers = 7f 0r 64 pointers
  // presume 2060 for 8065.

  int i, cnt,bank, bf;
  RBT *r;
  uint  pc, addr, val, val2;

 #ifdef XDBGX
  DBGPRT(1,"In chk_rbasp");
  #endif

// do NOT use numbanks, as CAN have single bank 8065 !!
  if (P8065) addr = 0x82060; else addr = 0x82020;

  cnt = g_byte(addr);                     // no of entries
  bf = 0;
  pc = addr+2;
  val2 = g_word(pc);

  if (cnt != 8)
   {             // may be a bigger number somewhere....
    #ifdef XDBGX
     DBGPRT(1,"Not Found");
    #endif
   return;
   }

  bank = 8;

  for (i = 1; i < cnt; i++)
   {
    addr = val2 | (bank << 16);
    if (!val_data_addr(addr)) break;  // cancel all

    val = g_word(addr);                    // value at pointed to
    pc += 2;                               // still in bank 8
    val2 = g_word(pc);                     // get next match (was addr)

    if (val != val2)
     {
      if (!numbanks)  break;
      // check if rbase points to another bank (typically b1)
      if (!bf)
        {
         // not checked yet
          for (bank = 0; bank < BMAX; bank++)
              {
                if (bkmap[bank].bok)
                 {          // bank is OK
                   addr = nobank(addr) | (bank << 16);
                   val = g_word(addr);                    // value at pointed to
                  if (val == val2) {bf = 1; break;}                      // bank found
                 }
              }
        }
     }
   }

  if (i < cnt)
   {
    #ifdef XDBGX
    DBGPRT(1,"Not Found");
    #endif
    return;
   }


  #ifdef XDBGX
    DBGPRT(1,"FOUND RBASE, points to bank %d", bank);
  #endif

  // OK, matched list and bank................

  if (P8065) addr = 0x82062; else addr = 0x82022;

  add_data_cmd(addr-2, addr-1, C_BYTE|C_CMD, 0);
  add_data_cmd(addr, addr+(cnt*2)-1, C_WORD|C_CMD, 0);

  pc = 0xf0;                                // always 0xf0 ?? NO !!!!
  for (i = 0; i < cnt; i++)
   {
    val   = g_word(addr);                    // value at pointed to
    val  |= (bank << 16);
    val2  = g_word(val);
    val2 |= (bank << 16);

    r = add_rbase(pc,val,0,0);             // message in here....
    if (r)
      {
       r->cmd = 1;
       upd_ram(pc,val,2);                           // and set register
       add_data_cmd(val, val+1, C_WORD|C_CMD,0);
       add_code_cmd(val,val2-1, C_XCODE);          // and add an XCODE as this is data
      }

    pc+=2;
    addr+=2;
   }

   #ifdef XDBGX
   DBGPRT(1,"end rbasp");
   #endif
   return ;
}*/






/************************************************
 *  Symbols
 ************************************************/


int do_table_sizes(uint start, int scols, uint *apnd, uint maxend)
{
  // assume a table for now...............
// so a rectangle of numbers...
// last byte may be a filler.
// last byte/word by command may or may not be correct end....
// start should be OK though.

// how to 'trim' the sizes ???


// may even try just counting the number of values ?? maybe their position too ?
//position as array ? maybe....


// to decide on table structure more closely, need an array of rows-1 x cols-1
// and keep ALL the interdigit differences to get best fit.   this should also
// allow splitting up unknown blocks into probable tables.............
// so algorthm must be size independent ??


//or find a central value ? (mean/median) or modal value (most common)




// func finder should be child's play compared to this..........


//looks like we need a function finder too........................
// and then a struct finder..............

  int i,j;
  uint apend;
  int size, addr;
  int rmax;
  int cmax;
  int rscore, cscore;
  int row, col, lval, val, dif;
  int cols, rsize;        // bit array  ??

  int ansrow, anscol;

  int cdif, rdif, tot, ansscore;        //test

  ansscore = 20000;
  anscol = 0;
  ansrow = 0;

  apend = *apnd;
  cols = 0;
  scols = nobank(scols);


#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"do table data sizes for %x-%x cols %d, max %x", start, apend, scols, maxend);
 #endif

 for (j = 0; j < 2; j++)
 {            // for apparent and maxend

   if (!j)  size = apend-start;
   else
     {
       if (maxend == apend) break;  else  size = maxend-start;
     }


// first find candidate sizes


  for (i = 3; i < 32; i++)
    {
      row = size/i;
      if (row*i == size && row < 32) cols |= (1<<i);
      if (i == scols) cols |= (1<<i);                      // add suggested col
    }


  if (!cols)
      {
       row = g_byte(start+size);
       if (row == 0xff)
         {  // could be terminator
           size--;
           #ifdef XDBGX
            DBGPRT(1,"terminator removed");
           #endif

// temp repeat fix this later............but this works so far

  for (i = 3; i < 32; i++)
    {
      row = size/i;
      if (row*i == size && row < 32) cols |= (1<<i);
      if (i == scols) cols |= (1<<i);                      // add suggested col
    }




#ifdef XDBGX
 DBGPRT(1,"no valid rows/cols for %d", size);
#endif
      } }



// find most common value first ??


  for (i = 1; i < 17; i++)
   {
     cols >>= 1;
     if (cols & 1)
     {
      rmax = 0;
      rscore = 0;
      cmax = 0;
      cscore = 0;
      cdif = 0;
      rdif = 0;
      rsize = size/i;     //possible row size

//but this may be too many rows..........
  #ifdef XDBGX
      DBGPRT(1, "- - - - %d x %d (%d)", i, rsize, size );
  #endif

      // do rows first  (cols within each row)

      for (row = 0; row < i; row++) // for each row
        {
         addr = start+(row*rsize);              // start of this row (col zero)
         lval = g_byte(addr);                   // first col of row

         for(col = 1; col < rsize; col++)   // col in row
          {
            val = g_byte(addr+col);
            dif =  abs(val-lval);
            if (dif > rmax) rmax = dif;         // max dif for this row
            rscore += dif;                      // all rows;
            lval = val;                         // update last value
          }

        }


      // do cols (rows in each col)

      for (col = 0; col < rsize; col++)   // col in row
        {
         addr = start+col;                // start of this row
         lval = g_byte(addr);               // first row of col

         for (row = 1; row < i; row++) // for each row
          {
            val = g_byte(addr+(row*rsize));
            dif =  abs(val-lval);
            if (dif > cmax) cmax = dif;
            cscore += dif;
            lval = val;
          }
         }

// max diff appears to be the real deal.

        dif = (rsize-1)*(i-1);    // no of elements
        if (dif) {
        rdif = (rscore*10) / dif;
        cdif = (cscore*10) / dif;
        }

    tot = rdif +cdif;


  #ifdef  XDBGX
       DBGPRT(1,"%dx%d  rdf,cdf %d+%d/%d = %d", i,rsize,rdif,cdif,dif, tot);
  #endif

      if (tot >= 0 && tot < ansscore)
      {
      ansscore = tot;
      ansrow = rsize;
      anscol = i;
      }

     if (!tot && anscol < 4 && i > 4 and i < 8)
        {  // choose a middle value for small cols and all zeroes
          ansscore = -1;
          ansrow = rsize;
          anscol = i;
        }
    }     // end possible colcnt

  }  // end col count loop
}
#ifdef XDBGX

  DBGPRT(1,"LOWEST %dx%d TOT %d ",anscol, ansrow, ansscore );
#endif

// if score = 0 then default to common sizes.

if (ansrow > 16 || ansscore == 0) ansrow = 0;

 *apnd = (ansrow*anscol);
 return ansrow;
}

//LOOP l;

void turn_dtd_into_data(void)
{
 CHAIN *x;
  uint ix, sz;      //, dcmd, start,lstart;    //cix,dix , end;
  DTD *d;

#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"dtd to data");
#endif

// start with all loops must be spotted via autoinc
// assume non-qual opcodes already dropped....

x = get_chain(CHDTDD);        //data chain

 for (ix = 0; ix < x->num; ix++)
   {
    d = (DTD*) x->ptrs[ix];

    if (!d->dscan && val_rom_addr(d->dataptr))               // dtd has RAM addrs
      {
           sz = bytes(d->fend);
           add_cmd(d->dataptr, d->dataptr+sz-1,  sz);       // add data entry
           d->dscan = 1;
         }


      }

//   }

}
 /*   if (d->opcsub == 1 )
     {
      lstart = 1;
       while ((a = get_dtkd(&chdtka, d, lstart)))
         {
          lstart = a->stdat+1;          //next start
          start = a->stdat;
          if (val_rom_addr(start))
             {
              p = get_cmd(start,0);
              DBGPRT(0,"%x IMD = %x ", d->ofst,a->stdat);
 if (p) DBGPRT(1," IGNORED %x-%x %s",p->start, p->end,cmds[p->fcom]); else DBGPRT(1,0);
//get cmd first.................

          //    DBGPRT(1,"%x-%x %s",p->start, p->end,cmds[p->fcom]);
            //  else DBGPRT(1, "***");
             }
         }
      //  DBGPRT(1,0);
      }
   }

/pushes first ??
// these are ALL INCORRECT except for 79f9 on A9L

 for (ix = 0; ix < chdtk.num; ix++)
   {


    d = (DTK*) chdtk.ptrs[ix];
  //  dcmd = bytes(d->fend);       //may add an fcom instead...

    if (d->psh)       // && d->opcsub == 1)
       { //imd push
    //    DBGPRT(0, "%x is push ", d->ofst);
        lstart = 1;

        while ((a = get_dtkd(&chdtka, d, lstart)))
         {
          lstart = a->stdat+1;          //next start
          start = a->stdat;
          if (val_rom_addr(start))
             {

              get_cmd(start,0);
              cix = chcmd.lastix;
              while (cix < chcmd.num && start)
                {
                  p = (LBK*) chcmd.ptrs[cix--];
                  if (p->end < start) break;             //no overlap
                  if (p->start <= start && p->end >= start) start = 0;
                }
              if (start) DBGPRT(1,"push %x from %x",start, d->ofst);
             }
//get cmd to make sure it's not used.
         }
   //     DBGPRT(1,0);
       }
   }
  //      add_cmd(start, start+bytes(d->fend)-1,bytes(d->fend), d->ofst);
  //scan dtk chain and add data entries
        DBGPRT(2,0);
/

x = get_chain(CHDTK);
  for (ix = 0; ix < x->num; ix++)
   {
    d = (TRK*) x->ptrs[ix];
    dcmd = bytes(opctbl[d->opcix].fend[1]);       //may add an fcom instead...
    if (opctbl[d->opcix].sigix == 14) push = 1; else push = 0;

    // find attached data start(s)

    lstart = 1;

#ifdef XDBGX
    if (d->ainc) DBGPRT(1, "%x is INC++ sz %d", d->ofst, d->ainc);
    #endif

    while ((a = get_dtkd(CHDTKO, d->ofst, lstart)))
       {
        lstart = a->stdat+1;          //next start
        start = a->stdat;


  //      add_cmd(start, start+bytes(d->fend)-1,bytes(d->fend), d->ofst);       // add data entry TEMP

#ifdef XDBGX
        DBGPRT(1,"%x dat=%x opc %d",d->ofst, a->stdat, d->opcsub);
#endif
        // get nearest command, for overlap checks with data

      //  end = start + bytes(d->fend)-1;      //end of this data item
        p = get_cmd(start,0);

            if (p && p->start <= start && p->end >= start)
             {
               //overlap - kill if matches criteria

               if (p->fcom == dcmd)
                {
#ifdef XDBGX
                 DBGPRT(1,"duplicate command %x %s", start, cmds[dcmd].str);
#endif
                 start = 0;                         // kill insert
                 break;
                }
             // words/bytes/longs within func/tab (+ struct/timer for now). args ?? - different chain)
               if ( dcmd >= C_BYTE && dcmd <= C_LONG && p->fcom >= C_TABLE && p->fcom <= C_TIMR)
                {
#ifdef XDBGX
                 DBGPRT(1,"overlap struct %x %s", start, cmds[dcmd].str);
#endif
                 start = 0;
           //      a->olp = 1;
                 break;
                }
               else
                {
#ifdef XDBGX
                 DBGPRT(1,"overlap %x %s from %x with %x-%x %s", a->stdat, cmds[dcmd].str, d->ofst, p->start, p->end,cmds[p->fcom].str);
#endif
                 if (p->fcom == C_CODE && !push)         //push
                   {
#ifdef XDBGX
                     DBGPRT(1, " **** CODE ***");
#endif
//a->inv = 1;                  // temp..........
if (p->end-start < 16 )  //&& start > 0x9201f)
      {
#ifdef XDBGX
        DBGPRT(1,"inx?");
#endif
       }

                   }
                //and extra work to go here..............
                 break;
                }
             }
     //     }          //  end overlap check



// if imd, then look for a To at d->ofst+d->ocnt, for a loop with start at...
// AA has start and END, to be careful of...check by using inc....



      /  if (start && d->ainc)
          {
            // inc is set, look for a loop. may be better after an imd ??? will be before the inc ??
        //    j = 0;
            cix = get_jump(&chjpf,d->ofst);       // from jump (after inc opcode)
            while (cix < chjpf.num)
             {
               m = (JMP*) chjpf.ptrs[cix];
               if (m->toaddr > d->ofst) break;      // must span d->ofst
               if (m->toaddr <= d->ofst && m->toaddr != m->fromaddr &&m->fromaddr >= d->ofst && m->back) j = m;
               cix++;



    // next jump ? prev jump?
          //          if (dix < chjpt.num) j = (JMP*) chjpt.ptrs[dix]; else break;

             }

         *    j = (JMP*) chjpt.ptrs[cix];   // candidate jump


               if (j && j->back)          // && j->fromaddr > d->ofst)
                 {  //loop spans this increment.
                    //new subr ??
                    //need to check loop size too, A9L JUMPS to SUBRS....
#ifdef XDBGX
                   DBGPRT(1,"Loop? %x (%x-%x)", d->ofst, j->toaddr,j->fromaddr);
#endif
                  // set up loop holder
                   memset(&l,0,sizeof(LOOP));              //clear loop struct
                   l.increg = d->rgvl[1];
                   l.incr = d->ainc;
                   l.start = j->toaddr;                  // where loop starts
                   l.end = j->fromaddr;

                   dix = ix;

while (dix && dix < chdtk.num)
 {
  DTD *b;
   n = (TRK*) chdtk.ptrs[dix--];

   if (n->ofst < l.start-32) break;
   if (n->opcsub == 1 && n->rgvl[2] == l.increg)
     {
       b = get_dtkd(&chdtko, n->ofst, 1);
     if (b) {  l.datastart = b->stdat;
#ifdef XDBGX
       DBGPRT(1,"loop imd %x = %x", n->ofst, b->stdat);
#endif
       break;}
     }
  }



}

} */
/*go back to find start address of designated register..............
// so this must be FROM ordered..............



// can use ix here for further loop
  //     n = (DTK*) chdtk.ptrs[ix];






       while (d->ofst < l.end && ix < chdtk.num)
          {     //inside loop
            ix++;
            n = d;
            d = (DTK*) chdtk.ptrs[ix];
             if (d->ainc && d->ofst != n->ofst && d->rreg == n->rreg) l.incr += d->ainc;
             else
               {
                 if (!l.incr2) l.incr2 = d->start[0]-n->start[0];
               //  else if (l.incr2 != d->start[0]-n->start) DBGPRT(1,"inc mismatch");
                }
      //       DBGPRT(1,"L %x", d->from);
          }

            DBGPRT(1,"loop analyse %x-%x for R%x inc %x %x start %x",l.start,l.end, l.increg, l.incr, l.incr2, l.datastart);
     //  ix--;   // to stay at end of loop

// and HERE is where to find start, end etc conditions ??
// inc should be FIRST in loop.............
// if jump is STAT then need to find exit condition
        }
     }


/
// else DBGPRT(1,"no p");



    if (val_rom_addr(start) && !push && d->opcsub > 1)
       {
//ignore reg to reg and immds
          //split out add_cmd for ix value fo range checks ? or just do bfindix ??

        k = add_cmd(start, start+dcmd-1,dcmd, d->ofst);       // add data entry

        if (!k)
          {
#ifdef XDBGX
           DBGPRT(1,"no cmd [%d] (%x %s)", ix, start, cmds[dcmd].str);
#endif
          }
       }

   } // end while 'a'


}          //master 'for'



}       // end of subr





*/



/*
void add_opnd_data(INST *c)
{
  OPER *o;
  LBK *k;

// THIS ONE  to change to TRACKING data ??


 // LBK *k;
  int addr, sig;             //x

  if (anlpass >= ANLFFS) return;
 // if (s->nodata) return;


  addr = 0;
  sig = c->sigix;

  if (sig == 14)  return;    // not push



 --- opcsub == 1 for immediates with valid ROM / RAM address ??
 // how to tell what's real though ? Assume it always calls a indirect ??


  if (c->opcsub == 1)
  {     //ldx or add with immediate only. ALSO check it's not a cmp limit check ?
    if (sig == 12 || sig == 6)
      {
       o = c->opnd+1;

    //   if (val_rom_addr(o->addr))  addr = o->addr ;

    //   if (c->wop == 1) addr = o->addr;

       if (nobank(addr) > PCORG)
         {
          x = find_last_pxsw(c->ofst,0);
          if (x && sinst.opcix > 33 && sinst.opcix < 36)
           {  // found  a cmp
              if (sinst.opnd[2].addr == c->opnd[2].addr
               && sinst.opnd[2].val == c->opnd[2].val)
             {  // cmp matches the ldx, i.e. a LIMIT CHECK
              addr = 0;
             }
           }
         }
       }
    }         /


// indirect but NOT for writes (STX) or zero wops

  if (c->opcsub == 2)
   {
     o = c->opnd+1;
     if (c->wop > 1) addr = o->addr;

  //   if (c->inc)
 //    { //struct ?
 //    DBGPRT(1,"INC++ STRUCT?");
 //    }
   }

 // indexed

  if (c->opcsub == 3)
  {
   o = c->opnd+4;                         // = saved opnd[1]

 // if (!o->wsize)
 // {
  // if [0] is a valid ROM address, then use that.
  // if R[1] is a valid ROM address with small offset, then use R[1]
  // if R[1] is an rbase, use combined address in [1]

   if (c->opnd[1].rbs) addr = c->opnd[1].addr;    // combined address
   else
    {
     addr = c->opnd->addr;                 // address of offset [0]
     if (addr >= (PCORG+0x8))
       {
        addr = databank(addr,c);       // use fixed offset
       }
     else
      {
       addr = o->addr; // fixed offset is too small, use register ?
       if (addr >= (PCORG+0x8)) addr = databank(addr,c); else addr = 0;
      }
    }
  // }
  }


  if (addr)
     {            // addr must be val_rom_addr
      k = add_cmd (addr,  addr+bytes(o->fend)-1, bytes(o->fend), c->ofst);       // add data entry
  //    if (k)
 //      {    // add offset and type for later checks
 //        k->opcsub = c->opcsub;
    //    if (c->opcsub == 3 && c->opnd[4].addr)
    //    if (c->opnd[4].val > 0 && c->opnd[4].val < 17)
    //    k->soff = c->opnd[4].val;                // offset 0-16
  //     }
     }
 }


}
}
}
}
*/




//could add arg for test mode....

void turn_scans_into_code (void)
 {

  // turn scan list into code commands.

  uint ix;
  SBK *s, *t;
  CHAIN* x;

  x = get_chain(CHSCAN);
  for (ix = 0; ix < x->num; ix++)
   {
    s = (SBK*) x->ptrs[ix];
    if (s->inv)
     {
       // inv block check.  if another block overlaps this one.
       // may be a run-on due to faulty args or a stack shortcut, etc.
       // also for debugs, can add code up to end, if it doesn't overlap with anything
       //s->nextaddr will be address AFTER the inv
       // ...THIS IS A HORRIBLE KLUDGE !!! but it works...
       if (ix < x->num-1)
         {
          t = (SBK*) x->ptrs[ix+1];
          if (!t->inv && t->start < s->nextaddr)
            {
             // next block not invalid and overlaps this one
             #ifdef XDBGX

             DBGPRT(1,"Invalid scan %x-%x overlapped by %x-%x",s->start, s->nextaddr,t->start, t->nextaddr);
             #endif
             add_cmd (s->start,t->nextaddr,C_CODE);
             s = NULL;                  //continue;                                 // return to loop
           }
       }        // ix < chscan.num
     }         // s->inv
    else
    if (s && (s->stop))
      {   // valid block to turn to code, not inv
       add_cmd (s->start,s->nextaddr,C_CODE);
      }
   }            // for loop
 }


void end_scan (SBK *s)
{

  if (!s->proctype) return;       //not sigs
  if (get_cmdopt(OPTMAN)) return;

  s->stop = 1;

  if (s->nextaddr > s->start)
    {
     s->nextaddr--;         // is now end of block

  //   b = get_subr(s->substart);
  //   if (b && s->nextaddr > b->end) b->end = s->nextaddr;        // fucked up by jumps out, need linear scan to fix.
    }

  #ifdef XDBGX
   else  DBGPRT(1," Start > End");
  #endif


  #ifdef XDBGX
   if (s->proctype)
     {

      DBGPRT(0,"END scan %x at %x", s->start, s->nextaddr);
      if (s->inv)  DBGPRT (0," !! Invalid Set");
      DBGPRT(1,0);
     }
  #endif

//do scan jmps

}

/*************************************
Multiple scale generic get value.
for varibale bit fields (0-31)
Auto negates via sign (fend & 32) if required
***********************************/


int scale_val (int val, uint fstart, uint fend)
{
  // separate 'scale_value' for bit fields

  int mask, sign;

  sign = (fend & 0x20);                        // keep sign state

  if (fstart == fend) sign = 0;                // no sign for single bit

  if (fstart) val >>= fstart;                  // shift down (start bit -> bit 0)
  mask = 0xffffffff >> (31 + fstart - fend);   // make mask
  val &= mask;                                 // and extract required field value

  if (sign)
    {                                          // signed set
     sign = 1 << fend;                         // sign mask
     if (val & sign)
       {                                        // if negative
        mask = ~mask;                           // flip for set negative (full 32 bit)
        val = val | mask;                       // add mask for -ve
       }
    }

return val;
}


int g_val (uint addr, uint fstart, uint fend)
{                   // addr is encoded bank | addr
  int val, start,end;
  uchar *b;
  uchar *t;

  b = map_addr(&addr);

  if (!b)  return 0;               // map to required bank
  t = (uchar*) &val;

  start = (fstart & 0x1f);        // max fields
  end   = (fend   & 0x1f);        // use end (fend has sign)

  if (start > end)  return 0;     // safety check

  start = start / 8;
  end   = end   / 8;

  val = 0;

  while (start <= end)
    {
     t[start] = b[addr+start];
     start++;
    }

  val = scale_val(val, fstart, fend);

  return val;

}



void avcf (SIG *z, SBK *blk)
  {
    // for stackptr as background task list pointer.
    // this scans Forwards

    // PUSHW  makes stack go DOWN    SP -= 2;   [SP] = Ra;
    // RET    makes stack go UP      PC = [SP];  SP += 2;
    // POP    makes stack go UP      Ra = [SP];  SP += 2;

    //BUT CANNOT assume address given is start OR end !!!

    int start, addr, val, bk;
    SBK *x;
    LBK *s;
    ADT *c;

    if (blk->proctype != 2) return;

    addr = z->vx[4];
    start = addr;
    bk = g_bank(z->start);
    #ifdef XDBGX
    DBGPRT(1,"in avcf, start = %05x", start);
   #endif
    while (1)
     {
        val = g_word(addr);
        val |= bk;

        #ifdef XDBGX
        DBGPRT(0, "vect %x ", addr);
        #endif
        x = add_scan(val, J_SUB,blk);         //no scan_blk ??

     if (!x) {addr -=2; break;}

     addr += 2;
     }


    s = add_cmd (start,addr, C_VECT);

    if (s)
     {
     // do bank here......only if not same (once)
       c = (ADT*) chimem(CHADNL);
       if (c) {
   //        s->adt = 1;
       c->fid = vconvi(s->start);
       if (bk == g_bank(addr)) c->cnt = 0;
       else c->cnt   = 1;
       c->bank = bk >> 16;                 // always add bank ?
       append_adt(c);          //vconvi(s->start),0);
       }
     }

    #ifdef XDBGX
    DBGPRT(1,"avcf stop at %05x", addr);
   #endif
  }



uint reg_eq(uint ra, uint rb, uint size)
{

 if (ra == rb) return 1;     // straight match

 if (ra & 1)
  {       // odd register (in) - check for word ops to match
   if ((size & 3) > 1 && (ra - 1) == rb) return 2;
  }

return 0;         // no match
}

int find_list_base(CINST *c, uint ix, int num)
{
    // try to find a list 'base address'.
    // expect an add of an immediate value
    // allow for ldx to swop registers

   int ans, xofst;
   uint reg;

   ans = 0;
   xofst = c->ofst;
   reg = c->opnd[ix].reg;



   while (xofst)
    {   // look back for an add,ad3w or ldx (max 'num' instructions back)

      c = find_opcode (xofst, 0);             // get previous opcode
      if (!c) break;

      num--;
      if (num <= 0) break;

      xofst = c->ofst;                   // new ofst

      if (c->sigix == 12)  // ldw
        {
         if (c->opnd[2].addr == reg)
           {
            // ldx from another register, change register
            if (!c->opcsub) reg = c->opnd[1].addr;

            // an immediate value
            if (c->opcsub == 1)
              {
                ans = c->opnd[1].addr;     // addr (should be) val+databank
                if (val_rom_addr(ans)) return ans;
              }
           }
         }

       if (c->sigix == 6) // ad2w, ad3w
         {
          if (c->opnd[3].addr == reg)          // sinst.opnd[sinst.wop].addr == reg)
            {
             if (c->opcsub == 1)
               {                              // found add for register
                 ans = c->opnd[1].addr;     // addr is val+databank
                 ans = databank(ans,c);
                 if (val_rom_addr(ans)) return ans;
               }
            }
         }

      }          //end while

  // but even if not found, may still have a valid address already in register as last resort.
  if (xofst)
    {
    ans = c->opnd[ix].val;
    if (val_rom_addr(ans)) return ans;
    }
  else ans = 0;

return ans;
}

void match_opnd(CINST *c, SFIND *f)
{
   COPER *s;
   uint m, ix;

   for (ix = 0; ix < 2; ix++)
    { // for each possible answer/register
     if (valid_reg(f->rg[ix]) && !f->ans[ix])
      {

       if (c->sigix == 12)
        {  // ldw - get operands
        //can use 'c'   here with inst chained!!

         s = c->opnd + 3;                       // where the write op is

         m = reg_eq(f->rg[ix], s->addr, s->fend);

         if (m)
           {        // found register [3] = [1] for ldx
            if (c->opcsub == 1)
              {   // immediate, look for wop override
               f->ans[ix] = c->opnd[1].addr;               //val ?;
               #ifdef XDBGX
                 DBGPRT(0," Found imm R%x = %x [%x]", f->rg[ix], f->ans[ix], c->ofst);
               #endif
              }
            else
            if (!c->opcsub)
              {  // ldx from another register, change register, but NOT
                #ifdef XDBGX
                  DBGPRT(0," Chg R%x = R%x [%x]", f->rg[ix], c->opnd[1].addr, c->ofst);
                #endif
                f->rg[ix] &= 1;                       // is it odd ?
                f->rg[ix] += c->opnd[1].addr;       // replace with new reg (may be odd)

                //but what if reg preloaded like BWAK3N2 ??

              }          // end if match reg
           }


        }    //end ldx
       else

       if (c->sigix == 6 && c->numops == 3)      //sigix = 25 ??
         {      // ad3w (ad2w ??)


          if (c->opnd[3].addr == f->rg[ix])
           {
             //RBT *b;
            // b = get_rbt(c->opnd[2].reg, c->ofst);

            if (c->opnd[2].rbs)
             {     // ad3w with an rbase.       //deosn't find_list base do this ?
               if (c->opcsub == 1)
                 {  // immediate.  op[3].val is correct as bank is in rbase.
                   f->ans[ix] = c->opnd[3].val;
                   #ifdef XDBGX
                      DBGPRT(0," ad3w R%x = %x at %x", f->rg[ix], f->ans[ix], c->ofst);
                   #endif
                 }
               else
                 {
                  // if it's a register, or indirect, then need to find a list base for multiple structs (tabs or funcs)
                  // this might be widened to non_rbase ??
                  // and check spacing for tabs ???
                  f->lst[1] = c->opnd[2].val;                // this is rbase address for possible list start
                  m = find_list_base(c,1,15);
                  if (m)
                    {
                     f->lst[0] = m;
                     f->ans[ix] = m;       // for stopping the search
                     #ifdef XDBGX
                       DBGPRT(0," Found LIST R%x -> %x [%x]", f->rg[ix], m, c->ofst);
                     #endif
                    }
                  else
                   {
                  #ifdef XDBGX
                      DBGPRT(0," ad3w unknown R%x at %x", f->rg[ix], c->ofst);
                   #endif
                   }
                 }
             }
            else
             {         // not rbase
              #ifdef XDBGX
               DBGPRT(0," ad3w !rbase R%x at %x", f->rg[ix],  c->ofst);
              #endif
             }
           }
         }             // end ad3w

           // may be other pointer stuff here xdt2 does ad2w
      }     // end if f->x
    }    // end for i
 }



void set_xfunc(SFIND *f)
 {
  // set function start and one row.
  // extended later with extend_func
  // check start value for sign/unsign

  uint startval, val, fendin;
  int rowsize;       // increment, entry (read) size for g_val

  ADT *a, *b;
  LBK  *k;
  SPF *p;

  p = f->spf;
  #ifdef XDBGX
  DBGPRT(1,"Check Func %x, I %x, O %x", f->ans[0], p->fendin, p->fendout);
  #endif



  fendin = p->fendin;                   //local as it may change
  startval = get_startval(fendin);     //cnv[szin].startval;      //(~casg[szin]) & camk[szin];     // start value unsigned

  val = g_val(f->ans[0],0, fendin);         // read first input value (as unsigned)
  val &= get_sizemask(fendin);     //  cnv[szin].sizemask;            //camk[szin];                     // and fit to startval

  if (val != startval)  // try alternate sign....
    {
     fendin ^= 32;          // swop sign flag
     #ifdef XDBGX
     DBGPRT(1,"require Swop sign in - val %x (%x)", val, startval);
     #endif

     startval = get_startval(fendin);       //cnv[szin].startval;  // (~casg[szin]) & camk[szin];     // start value

     if (val != startval)
      {
   //    #ifdef XDBGX
   //      DBGPRT(1,"func %x Invalid %x expect %x", f->ans[0], val, startval);
   //    #endif
       return ;
      }
    }

    rowsize = bytes(fendin) + bytes (p->fendout);

    k = add_cmd (f->ans[0], f->ans[0] + rowsize-1, C_FUNC);

  if (!k) return;
  if (k->usrcmd) return;
  if (get_lasterr(CHCMD)) return;             //chcmd.lasterr) return;
  if (get_cmdopt(OPTCMPD)) k->cptl = 1;   // layout by default

//test here...


  // cmd OK, add size etc (2 sets for func)

       add_autosym(3,f->ans[0]);            // auto func name, can fail
       k->size = rowsize;                   // size of one row

       a = (ADT*) chimem(CHADNL);
       a->fid =  vconvi(k->start);
       a->bank = k->start >> 16;
       a->cnt = 1;
       a->fend = fendin;                     // size, word or byte
       a->pfw   = get_pfwdef(a);
       a->dptype = DP_DEC;             //prdx = 2;                         // default decimal print
   //    append_adt(a);              //vconvi(k->start),0);            //move this to end....

       b = (ADT*) chimem(CHADNL);
       b->fid = a;       //vconvi(k->start);       //becomes 'a' from above...
       b->cnt = 1;
              b->bank = k->start >> 16;
       b->fend = p->fendout;              // size out
       b->pfw =  get_pfwdef(a);
       b->dptype = DP_DEC;        //prdx = 2;                         // default decimal print
     //  append_adt(a);              //,0);
        append_adt_mult (a);
    //   k->adt = 1;


 }

/*      saved...
      add_autosym(3,f->ans[0]);            // auto func name, can fail
       k->size = rowsize;                   // size of one row

       a = (ADT*) chimem(CHADNL);
       a->fid = vconvi(k->start);
       a->cnt = 1;
       a->fend = fendin;                     // size, word or byte
       a->pfw   = get_pfwdef(a);
       a->dptype = DP_DEC;             //prdx = 2;                         // default decimal print
       append_adt(a);              //vconvi(k->start),0);            //move this to end....

       a = (ADT*) chimem(CHADNL);
       a->fid = vconvi(k->start);       //becomes 'a' from above...
       a->cnt = 1;
       a->fend = p->fendout;              // size out
       a->pfw =  get_pfwdef(a);
       a->dptype = DP_DEC;        //prdx = 2;                         // default decimal print
       append_adt(a);              //,0);
    //   k->adt = 1;


 }

*/


void fnplu (SIG *s,  SBK *blk)
 {
    // func lookup subroutine
    // set up subroutine(s) only
    // func data added later by jump trace

   SUB *subr;
   SBK *t;
   SPF *f;
   CINST *x;
   CSTR *anames;
   int size;
   uint i, taddr;
   uint xofst, imask, omask, isize, osize, ix;

   if (!s || !blk)     return;
   if (blk->proctype != 2) return;           // real scan only // but also for test ??
   if (!blk->substart) return;
   if (!s->vx[3])       return ;                 //sign holder

   subr = add_subr(blk,blk->substart);  // ok if already exists
   if (!subr) return;

   f = find_spf(vconvi(subr->start), 4);
   if (f) return;               // already processed, assume f always OK

   size  = s->vx[2]/2;                // size from sig (unsigned) as BYTES
   taddr = s->vx[4];                  // address register (need +80000 ??)

   #ifdef XDBGX
     DBGPRT(1,"in fnplu for %x (%x)", blk->substart, s->start);
   #endif

   blk->nodata = 1;        // no data to collect (it's where sig is)

   size = (size*8) -1;               // convert to fend style



   //start at current block start by default

   xofst = blk->substart;

   if (blk->start != blk->substart)
     {        //if got here by jump
       t = blk;
       while (t && t->substart != t->start)
        {
         t = get_scan(t->substart,0);       // get to original sub call.
        }
       if (t)
          {
             xofst = t->start;
    //         t->logdata = 0;
       //     #ifdef XDBGX
      //      DBGPRT(1,"set nodata %x", t->start);
      //    #endif
          }
     }



if (s->vx[1] == 0)
 {
        //this is right way round for A9L
   // v[10] and [11] are already MASK values....

   osize = (s->vx[10] & 0x1000) ? 0 : 32 ;              // signed out if bit SET, 1000 = signed if NOT set
   isize = (s->vx[11] & 0x1000) ? 0 : 32 ;              // signed in  if bit SET, 1000 = signed if NOT set
   osize |= size;
   isize |= size;

   omask = s->vx[10] & 0xff;                         // bit mask value, for sign out flag
   imask = s->vx[11] & 0xff;                         // bit mask value, for sign in  flag
   ix = get_opcode_ix(xofst);   //  get current

   i = 0;
   while (valid_ix(CHOPC, ix))
      {  // look forwards for OR, LDX, STX in 22 opcodes from front of found subr

       x = (CINST*) get_chitem(CHOPC,ix);
       if (!x) break;
       i++;
       if (i > 25) break;

       xofst = x->ofst;                   // new ofst
       if (xofst == s->start)  break;     //stop at start of sig

       if (x->sigix == 9)           // OR
         {
           if (x->opnd[2].addr == s->vx[3])       // matches register
             {
              if (omask & x->opnd[1].val)  osize ^= 32;    // flip sign out if match;
              if (imask & x->opnd[1].val)  isize ^= 32;    // flip sign in  if match

              #ifdef XDBGX
                  DBGPRT(1,"find OR at %x (I %x, O %x)", x->ofst, isize, osize);
               #endif
              }
         }

        if (x->sigix == 12  && !x->opcsub)       //  LDX
          {
            if (x->opnd[3].addr == taddr)      //matches address register
              {
                // for args, as they must be loaded via ldx ....
                taddr = x->opnd[1].addr;
              }
          }

         if (x->sigix == 20) break;   // not through a RET (forwards)
         if (x->sigix == 16)
           {        // allow jumps forwards
     //        xofst = x->opnd[1].addr;
             ix = get_opcode_ix(x->opnd[1].addr);    // get_copcode(xofst);   //  get current
           }
         else
          {
             ix++;          // x = find_opcode (xofst, 1);    // get next ofst in chain.ix++

          }
      //   if (!x) break;
         xofst = x->ofst;                   // new ofst
    }  //while
//end of find bits - type 0 of fnlu (fnlu1)
 }

if (s->vx[1] == 1)
 {  //  type 2 fnlu2 sign in s-vx directly

   osize = size;
   isize = size;

   if (s->vx[10] & 0x4000) osize |= 0x20;        // signed op if 0x4000 set
   if (s->vx[11] & 0x4000) isize |= 0x20;
 }


   // set function type

   //check option for naming !!

   anames = get_cstr(AUTONAMES,0);

   for (i = 5; i < 13; i++)
     {
      if (anames[i].par2 == isize && anames[i].par3 == osize)
        {
         add_autosym(i|C_SYS|C_RENAME,blk->substart);             // add subr name acc. to type

         f = append_spf(vconvi(subr->start), 4, 0);
         if (f)                  //f->spf = 4;
         {
         f->fendin  = isize;
         f->fendout = osize;
         f->addrreg = taddr;  //address
         }
         #ifdef XDBGX
              DBGPRT(1,"func ix=%d (%x)", i, blk->substart);
          #endif
          break;
         }
     }
 }



void set_xtab(SFIND *f)
{

 // add table from col and rows size.
 // can't fully check, do sanity check on sizes

  uint xend;
  LBK *k;
  ADT *a;
  int ofst, rows, cols;

// probably need a maxend safety here..... and a sign check....

  ofst = f->ans[0];
  rows = 2;
  cols = nobank(f->ans[1]);

  #ifdef XDBGX
   DBGPRT(1,"SET TAB Add %x cols %x Rows %x inx %x ss %d", ofst, cols, rows, f->spf->spf,f->spf->fendout);
   #endif




  if (cols < 2 || cols > 32) cols = 1;

  xend = ofst;
  xend += (rows*cols)-1;

  k = add_cmd(ofst, xend, C_TABLE|C_SYS);

  if (!k) return;
  if (k->usrcmd) return;
  if (get_lasterr(CHCMD)) return;

  if (get_cmdopt(OPTCMPD)) k->cptl = 1;   // layout
  add_autosym(4,ofst);                    // auto table name, can fail


//multiple adts

     a = (ADT*) chimem (CHADNL);
     if (a)
      {
     a->fid = vconvi(k->start);
     a->fend = f->spf->fendout;  // includes sign
     k->size = cols * bytes(a->fend);
     a->cnt = cols;
     a->bank = k->start >> 16;
     a->dptype = DP_DEC;        //prdx = 2;                         // default decimal print
     a->pfw =  get_pfwdef(a);
     append_adt(a);
     }


  // probably need some more checks, but simple version works for now...

}



void tbplu (SIG *s,  SBK *blk)

{
//word and byte tables come here

  // uint adr;
 //  int  cols, rows;
   uint xofst, taddr, tcols, osign, omask,i;
   SIG *t;
   SUB *subr;
   SBK *x;
   SPF *f;
   CINST *c;

   if (!s || !blk)         return;
   if (blk->proctype != 2) return;            // scan only
   if (!blk->substart)     return;
   if (!s->vx[3])          return ;

   subr = add_subr(blk,blk->substart);  // ok if it already exists

   if (!subr)       return;

   #ifdef XDBGX
     DBGPRT(0,"In TBPLU %x" , blk->curaddr);
   #endif

   blk->nodata = 1;        //no data collection


   f = find_spf(vconvi(subr->start), 5);
   if (f) return;               // already processed, assume f always OK

   taddr = s->vx[4] ;            // table address register
   tcols = s->vx[3] ;            // num cols in this register
   osign = 0;
   omask = 0;


   // In tab lookup, sign flag is in the INTERPOLATE subroutine
   // subr call address saved in s->v[0] so look for tabinterp sig there

  t = get_sig(s->vx[1]);

  if (!t)
     {
        if (s->vx[0] == 1)  t = do_sig(PATWINTP,s->vx[1]);     // scan for word interpolate
        else               t = do_sig(PATBINTP,s->vx[1]);     // scan for byte interpolate (default)
     }

   if (t)
     {           //   found tab signed interpolate


     // [10] is bit (as mask already). 0, jnb, 1000 jb
     // [2] is 0xfe or not (1 or 0)

//may need more here to check against a signed mult or not

      osign = (t->vx[10] & 0x1000) ? 1 : 0;   //jb, jnb
      omask = t->vx[10] & 0xff;       //      1 << (t->vx[10] & 0xff);

    #ifdef XDBGX
        DBGPRT(1," SIGN = %d (R%x mask=%x) ", osign, t->vx[3], t->vx[10]);
      #endif
      //and add an autosym for interp ??
      // add_autosym(x|C_SYS|C_RENAME, s->vx[1]);             // add subr name acc. to type
     }



  xofst = blk->substart;

   if (blk->start != blk->substart)
     {        //if got here by jump
       x = blk;
       while (x && x->substart != x->start)
        {
         x = get_scan(x->substart, 0);       // get to original sub call.
        }
       if (x)  xofst = x->start;

     }
          #ifdef XDBGX
            DBGPRT(1,"start at %x", xofst);
                  #endif


   i = 0;

   c = get_copcode(xofst);   //  get current

   while (xofst)
      {  // look for an LDX, OR

        if (!c) break;
        i++;
        if (i > 25) break;
        if (xofst == s->start)  break;     //stop at sig start

        if (c->sigix == 12 && !c->opcsub)            //ldw reg->reg
          {
           if (c->opnd[3].addr == taddr) taddr = c->opnd[1].addr;
           if (c->opnd[3].addr == tcols) tcols = c->opnd[1].addr;
          }

         if (t && c->sigix == 9)
           {
             if (c->opnd[2].addr == t->vx[3])
               {
                 if (omask & c->opnd[1].val)  osign ^= 1;    // flip signout to match                     // found register to match 't'
                 #ifdef XDBGX
                    DBGPRT(1,"find OR at %x sign = %d", c->ofst,osign);
                 #endif

               }
            }

         if (c->sigix == 20) break;   // not through a RET (forwards)
         if (c->sigix == 16)
           {        // allow jumps forwards ?
             xofst = c->opnd[1].addr;
             c = get_copcode(xofst);   //  get current
           }
         else
           {
            c = find_opcode (xofst, 1);    // get next ofst
           }
         if (!c) break;
         xofst = c->ofst;                   // new ofst

      }           // while
        #ifdef XDBGX
            DBGPRT(1,"exit at %x", xofst);
                  #endif


  if (s->vx[0])  xofst =  15 + osign;       // word table + size+sign;
  else          xofst =  13 + osign;       // byte table+size+sign;

  add_autosym(xofst|C_SYS|C_RENAME, blk->substart);             // add subr name acc. to type

   f = append_spf(vconvi(subr->start), 5, 0);
   if (f)
     {
       if (s->vx[0]) f->fendin = 15; else f->fendin = 7;                   // size in;
       f->fendout = f->fendin;              //sizeout = 1;                // need better than this ??
       if (osign) f->fendout |= 32;         //sizeout |= 8;
       f->addrreg = taddr;           //reg holding address
       f->sizereg = tcols;           // reg holding cols

  //   taddr = s->vx[4] ;            // table address register
 //  tcols = s->vx[3] ;            // num cols in this register
     }
   #ifdef XDBGX
     DBGPRT(1,"tab spf=5 (%x)", blk->substart);
   #endif
}



void encdx (SIG *s,  SBK *blk)
{
  //only run this in emulate ?

  int enc, reg;
  RST *r;

  MATHX *x;
  FKL *l;

  if (!blk || !s) return;
  if (blk->proctype != 4) return;

 if (s->vx[9])
  {
     reg = s->vx[9];
     if (s->vx[1] == 2) reg = g_word(reg);     //is this always indirect ??
     // and may need to create an rgstat.....
  }
  else reg = s->vx[8];


  #ifdef XDBGX

   DBGPRT(0,"in E encdx");
   DBGPRT(0," for R%x (=%x) at %x S%x", s->vx[8], reg, blk->curaddr, blk->substart);
   DBGPRT(1,0);
  #endif

   enc = s->vx[0];                                       // fixed par from pattern
   if (enc == 4 && s->vx[3] == 0xf)  enc = 2;            // type 2, not 4
   if (enc == 3 && s->vx[3] == 0xfe) enc = 1;            // type 1, not 3


  reg = nobank(reg);

 // replace with v5 ....link to/make a calc

  l = get_link(CHSIG, s,0, 0,1);                 // signature to calc link if exists

  if (!l)
   {           // add link,   calc to sig..
    x = add_encode(CHSIG, s,enc, nobank(s->vx[5]));  // add calc if not there

    #ifdef XDBGX
       DBGPRT(1,"add enc link calc -> sig");
    #endif

   }
  else x = (MATHX*) l->keydst;

  r = get_rgstat(reg);

  if (r)
   {       // mark rg status as encoded first
    r->calc = x;
    r->fend = 15;                   // enc is always a word
    #ifdef XDBGX
     DBGPRT(1,"set enc for %x (%x %x)", reg, enc,nobank(s->vx[5]));
    #endif
   }












    // ?? what is this for )= 3654 A9L) - multiple addresses
   while ((r = get_next_rgstat(reg)))
   {
           r->calc = x;
  //   r->enc = enc;
    r->fend = 15;                    // enc is always a word
  //  r->data  = nobank(s->vx[5]);     // 'start of data' base reg
    #ifdef XDBGX
 //   DBGPRT
     DBGPRT(1,"set enc for %x (%x %x)", reg, enc,nobank(s->vx[5]));
    #endif
   }




}



void ptimep(SIG *x, SBK *blk)
{
    // don't have to have a subr for this to work, so can do as
    // a preprocess, but logic works as main process too (but only once)


  LBK *k;
  ADT *a;
  uint start;
  int val,z;
  uint ofst;

   if (blk->proctype != 2) return;
  if (x->done)
   {
     #ifdef XDBGX
     DBGPRT(1,"ptimep - sig done");          //move this later
     #endif
     return;
   }

 #ifdef XDBGX
  DBGPRT(1,"in ptimep");          //move this later
 #endif

  x->vx[16] = g_word(x->vx[1]);

 // size in v[14].  do a 'find end' for the zero terminator
 // cmd size (Y or W) is (2 or 4)= byte  (3 or 5)=word

 //  start = databank(x->vx[0],0);           //this doesn't work with a const
   start = x->vx[16] | basepars.datbnk;

   z = x->vx[14];

   #ifdef XDBGX
    DBGPRT(0,"Timer list %x size %d", x->vx[16], z+3);
    //if (s) DBGPRT(0," Sub %x", s->addr);
    DBGPRT(1,0);
   #endif

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

    k = add_cmd(start, ofst, C_TIMR);      // this basic works, need to add symbols ?....
    if (k) {
        a = (ADT*) chimem (CHADNL);
        if (a)
          {
            a->fid = vconvi(k->start);
            a->fend = (z*8) -1;
            a->cnt = 1;
            a->bank = k->start >> 16;       //databank ??
            a = append_adt(a);        //vconvi(k->start),0);
    }

// a->fnam = 1;
}

  x->done = 1;  // once only for this sig

}



 /*
 *  for move into auto timer above.
 * void set_time(CPS *cmnd)
 {
  LBK *blk;
  int val,type,bank;
  int xofst;
  short b, m, sn;
  SYM *s;

  char *z;

  blk = set_cmd (cmnd->start,cmnd->end,cmnd->fcom,1);
  set_adnl(cmnd,blk);
  DBGPRT(1,0);
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





int check_sfill(uint addr)
{
  int val, ofst;
  // 4 or more repeated values cannot be code....

  ofst = addr;
  val = g_byte(ofst);
  for (ofst = 0; ofst < 4; ofst++)
   {
     if (g_byte(ofst+addr) != val) break;
   }
 if (ofst < 4) return 0;

 return 1;
}






/*
void add_arg_data(ADT *a, uint ofst)
{
   uint val,sz;

   //but 7e3b gets only one byte...so it's  NOT same as encode....

   val = decxode_addr(a, ofst);
   sz = bytes(a->fend);                //no.... val is address, but what is accessed from this address?

   add_cmd (val,  val+sz -1, sz, ofst);     //change to dtk ???

}
*/


void fix_args(SBK *s)
{
// check argument sizes and amend
// now called after emulate.................     //first draft.....called by skip args
// to ensure all processing has been done

//called LAST after emulate, and caller block executed

//s = emu->caller....

  RST *r;
  LBK *k;
  ADT *a;
  uint i, addr;    //, endaddr;

 // if (emu->start == 0x972a3)
   //   DBGPRT(0,0);


// CHECK emu->start to emu->nextaddr via calls....
//but if s becomes caller, then this logic changes


  #ifdef XDBGX
    DBGPRT(1,"\n FIX args (caller) %x %x", s->start, s->nextaddr);
    DBG_rst();
    DBG_emuargs();

  #endif


// NO!!   make each subr responsible for its 'own' args only !!
// (via emu ??) or make argsget a bitmask for levels ??
// OR caller->nextaddr


 for (i = 0; i < EMUGSZ; i++)
 {
   k = emuargs[i].cmd;            // next args command (LBK)

   if (!k) break;

   addr = k->start;
  // endaddr   = k->end;
      #ifdef XDBGX
  DBGPRT(1,"args %x -> %x", k->start, k->end);
  #endif

   a = get_last_adt(vconvi(k->start),0);                //test !! seems OK

   if (a)
     {
    #ifdef XDBGX
           DBGPRT(1,"%x args already added", k->start);
           #endif
       addr = k->end + 1;
     }


     // if using nextaddr, may have added args already....???
  //   if (emu->caller->nextaddr == k->end+1) //   or k->start......


   if (s->nextaddr >= k->start && s->nextaddr <= k->end+1)
     {
      a = get_last_adt(vconvi(k->start),0);

      #ifdef XDBGX
          DBGPRT(1,"Fix Args at %x", addr);
      #endif

   while(addr <= k->end)
     {
      r = get_rgstata(addr);             //get register by address....

      if (r)
        {        // found register.
             #ifdef XDBGX
          DBGPRT(0,"R%x match %x", r->reg, r->argofst);
          DBGPRT(1," fend %d", r->fend);
         #endif

 // find a way to merge matching adts ??
//get_last_adt ???

         if (r->calc)
          {     // attached calculation
            #ifdef XDBGX
              DBGPRT(1," Add CALC R%x", r->reg);
            #endif

//if r->calc = get_link (a,0,0);
//  l = get_link(a, 0, 0);            //l = get_link(a, 0, 0);       // fwd link adt to mathx
// if l && l->key = r->calc) or something.
    /*        if (a && a->enc == r->enc && a->data == r->data)
              { //matched enc, just increase count
                a->cnt++;
           //     add_arg_data(a, addr);
                addr += 2;
              }
            else  */
            {
            a = (ADT*) chimem(CHADNL);
          if (a) {
               // is bank right, or should it be databank ?
           a->fid =  vconvi(k->start);
           a->fnam = 1;
             a->bank = (r->argofst >> 16);
           a->cnt  = 1;
           a->fend = 15;          // word for enc
           a->dptype = DP_HEX;
           addr += 2;
           append_adt(a);           //vconvi(k->start),0);

           add_link(CHADNL,a,CHMATHX,r->calc,1);
            }
           }
          }
         else
          {   // not enc CHECK emu->start to emu->nextaddr via calls....

       #ifdef XDBGX
              DBGPRT(0," Add R%x (%d)", r->reg,r->fend);
              if (r->ref) DBGPRT(0, " ptr");
              DBGPRT(1,0);
            #endif

            a = (ADT*) chimem (CHADNL);    //append_adt(vconvi(k->start),0);         // NOT addr...
        if (a) {
           a->fid = vconvi(k->start);         // NOT addr...
           a->cnt  = 1;
           a->fend = r->fend;          // word for enc
                  a->bank = k->start >> 16;   // default
           a->dptype = DP_HEX;
           if (r->ref)
             {
               a->fnam = 1;
               a->bank = (r->argofst >> 16);

             }
        //   add_arg_data(a, addr);
           addr += bytes(r->fend);
             a = append_adt(a);        //vconvi(k->start),0);         // NOT addr...
            }  }
         }



if (!r)  addr++;     //temp loop fix ?
   }           //  while



 }        //addr < end


//** must do size check here !! ***** or inside loop !!
// if (k->start + totsize(vconvi(k->start)) != k->end+1)
//replace with bytes....




 }            // end for loop


  emptychain(CHEMUL);          // clear all emulate scans
  emptychain(CHRGST);
  emptychain(CHRGSTA);        // and regstats

  #ifdef XDBGX
//    if (changed)
//DBG_rgchain();
    DBGPRT(1,"\nEnd FIX args");

  #endif

  //  add_arg_data(sub->start, caller->nextaddr);  for whole chain ?? here somewhere....
}

/*
int do_arg_data (int ofst, ADT *a, int pfwo)
 {

// print a SINGLE ITEM from *a, even if a->cnt is set
// done this way for ARGS printouts.

  int pfw, val;
  SYM* sym;

  sym = NULL;
  pfw = 1;                               // min field width

  // pfw is still not always right in a struct, especially if names appear.
  // also args always needs min fieldwidths, so allow for a->pfw at zero

  if (pfwo) pfw = plist[pfwo];
  else
  {
  if (a->pfw)
   {                    // NOT FOR ARGS !!
    if (a->pfw < cnv[a->ssize].pfwdef) pfw = cnv[a->ssize].pfwdef;      // default to preset
    else pfw = a->pfw;                                                  // user specified
   }
  }
  val = decxode_addr(a, ofst);                        // val with decode if nec.  and sign - READS ofst
  if (a->fnam) sym = get_xsym(0, val,0,7,ofst);        // READ sym AFTER any decodes - was xval

  if (sym) pstr(0,"%s",sym->name);  // pstr(0,"%*s",-pfw,sym->name); -ve fw doesn't work ??
   else
  if (a->fdat)
     {      // float - probably need to extend pfw to make full sense - allow override with X ?
      sprtfl((float) val/a->fdat, pfw+4);         // add 4 for float part
      pstr(0,"%s", nm+64);
     }
   else
     {
       if (a->prdx == 0) pstr(0,"%*x", pfw, nobank(val));   // HEX   nobank ??
       if (a->prdx == 1) pstr(0,"%*d", pfw, val);           // decimal print
       if (a->prdx == 2) pbin(val, a->ssize);           // binary  print, need size
 //   else
 //    { // hex prt
 //     //if (!numbanks) don't KNOW whether this is a value or an address...vaddr
 //     val = nobank(val);      // but NOT for bank addresses !
 //     pstr(0,"%*x", pfw, val);
 //    }
     }
  return cnv[a->ssize].bytes;         //casz[a->ssize];
}
*/


void skip_args(SBK *s)
 {
     //called in scan_blk to skip over any args found.

   LBK *k, *e;
   int i;

   k = (LBK*) 1;   // to start loop..


   while (k)
    {
     k = get_aux_cmd(s->curaddr,C_ARGS);
     if (k)
       {
   //         if (!k->size)
   //      {
   //       #ifdef XDBGX
   //        DBGPRT(1,"precheck ARGSIZE IS ZERO !! %x", s->curaddr);
   //      #endif
   //      }

     if (!k->size)
         {
          #ifdef XDBGX
           DBGPRT(1,"ARGSIZE IS ZERO !! %x", s->curaddr);
         #endif
         s->curaddr ++;
         }

    //      fix_xargs(k,s);  //but regstats deleted by now...........
    //      add_arg_data(k->start, s->curaddr);

        s->curaddr += k->size;





        #ifdef XDBGX
         DBGPRT(1,"SKIP %d args -> %x", k->size, s->curaddr);
        #endif

return;             //test


        // fix args with new sizes, encode, etc
        // cleanup reg status and arg holders
        // clear rgargs matching k->start
        // clear emuargs matching k->start



          for (i = 0; i < EMUGSZ; i++)
             {
               e = emuargs[i].cmd;
                   #ifdef XDBGX
               if (e) DBGPRT(1,"EMUARGS = %x (%d)", e->start, i);
              #endif
               if (e && e->start == k->start)
               {
                #ifdef XDBGX
                DBGPRT(1,"drop EMUARG %d = %x", i, e->start);


                if (e->end != k->end)


                DBGPRT(1,"size mismatch EMUARG %d = %x", i, e->start);
                #endif

         //       emuargs[i] = 0;
               }
             }
        }
       }
    }


void do_signatures(SBK *s)
 {
  SIG *g;

   if (!s->proctype) return;                // not if sigs

   // some sigs (encdx) done in emulate....

//if (s->curaddr == 0x932bc)
//{
//    DBGPRT(0,0);
//}

// get_copcode saves multiple sig scans, and should be faster for multibank bins
// scan-sigs has get-sig at start anyway as safety check.

  if (get_copcode(s->curaddr))            // code, has this been scanned already ?
    {
      g = get_sig(s->curaddr);           // Yes, check for any sig at this address
    }
  else
    {
      g = scan_sigs(s->curaddr);         // scan required sigs if not code scanned before
    }

  if (g && g->hix) do_sigproc(g,s);     // allow multiple calls for funclu etc.

 }



uint scan_blk(SBK *s, INST *c)
 {
   // used for both scan AND emulate
   uint ans;
   if (!s) return 0;              // safety

   //or should this do scan then turn it into final code... maybe save the multiple passes thatw ay ??




   show_prog ();                  //anlpass);

   // need to check that it's not a faulty filler block......

   if (!s->stop && check_sfill(s->curaddr))
     {
      s->stop = 1;
      s->inv = 1;
      #ifdef XDBGX

       DBGPRT(1,"Invalid (scan) %05x from %05x",s->start, s->curaddr);
      #endif
      return 0;
     }

     if (s->stop || s->inv)
       {
         #ifdef XDBGX
         DBGPRT(1,"Ignore Scan %x", s->start);
         #endif

         return 0;
        }

     #ifdef XDBGX
     DBGPRT(1,0);
     DBG_sbk(0,s);
       #endif

   tscans++;

  //  if (s->scnt > 2000)
  //    {
  //    s->scanned = 1;
  //    DBGPRT(0,"too many scans !"); return;
  //   }

   s->scnt++;                                   // scan count by block

   while (!s->stop && !s->inv)                 // inv seems to stop loops, but should it be overridden with args detection ?
    {

   //       if (s->curaddr == 0x932bf)
//  {
//  DBGPRT(0,0);
//  }


     skip_args(s);                             // check for ARGS command(s) but fix shouldn't go here........
     do_signatures(s);                         // moved to here

     s = do_code (s, c);                       // do opcode (recurses here)

     if (s->proctype & 4)              //emulating)
      {                    // safety check for code loops
       opcnt++;

       if (opcnt > 10000)
       {
        s->stop = 1;
            #ifdef XDBGX
        DBGPRT(1,"Hit opcode limit %x  %x", s->start, s->curaddr);
            #endif
       }
      }
        ans = 1;
    } //end of this loop.

   #ifdef XDBGX
     DBGPRT(0,"Stop ");

     if (s->proctype & 2 ) DBGPRT(0,"scan");
     if (s->proctype & 4 ) DBGPRT(0,"Emu");
     DBGPRT(1," blk %x at %x", s->start, s->nextaddr);
    #endif

return ans;
}



void build_fake_stack(SBK *s, FSTK *dest)
{

 // from first block BUT misses pops outside calls.
 // build a fake stack from caller list of s.
 // from the stack point of view, a pushp will always be
 // Higher (= older) than its call out.


   int i;
   FSTK *em, *sc;

   s = s->caller;
   i = 1;

   while (i < STKSZ && s)
     {
      em = dest+i;
      sc = scanstk+i;
      em->popreg = 0;
      em->sblk = s;
      em->ftype = 1;         // call

      // check if any entries are popped in scan stack (ref A9l 7aa1, ears do this)
      // and mark in emulstack so that args turn up in right place.

      if (sc->sblk && sc->popreg && em->sblk->nextaddr == sc->sblk->nextaddr)
        {
          em->popreg = sc->popreg;

          #ifdef XDBGX
               DBGPRT(1,"entry %d (%x) popped in scan ", i, em->sblk->nextaddr);         // not quite right....
          #endif
        }

      if (s->pushp)
        {                  // add entry, not copy from stack
          i++;
          em = dest+i;           // new entry
          em->ftype = 4;         // pushp was used
          em->sblk = s;
          em->popreg = 0;
        }

      s = s->caller;
      i++;
     }


  #ifdef XDBGX
    DBG_stack(2);
    DBG_stack(4);
  #endif
}




void emulate_blk(SBK *s)
 {
   SBK *emu;

   if (!s) return;


   show_prog ();           //anlpass);

   #ifdef XDBGX
     DBGPRT(2,0);
     DBG_sbk("Start Emulate",s);
   #endif

   // first, copy call chain from scans into the emuchain
   // to build parallel chain for emulate.
   // then build copy of stack too.

   emu = copy_scanchain(s, 0, CHEMUL);

   build_fake_stack(emu, emulstk);

   memset(emuargs, 0, sizeof(emuargs));       //clear all argument logs


   emptychain(CHRGST);
   emptychain(CHRGSTA);        // clear all regstats


   opcnt = 0;               // global so it survives
   emuscan = 0;
   scan_blk(emu, &einst);   // restarts emulate at base block, now emu...

   // but if a push(imd) somewhere, then this won't be called
   // ....so fiddle it here by calling it LAST, and hope...

   if (emuscan) scan_blk(emuscan, &einst);  //but we DO know where to put it....as s->caller and


     #ifdef XDBGX
             DBGPRT(1,"Stop Emulate %x at %x", emu->start, emu->nextaddr);
             if (emu->inv) DBGPRT(1," with Invalid at %x", emu->curaddr);
          #endif


   fix_args(emu->caller);             // do fix here whilst reg stats present.

  //BUT this does not allow args to be sized in top non-emulated caller !!
  //may need flag to call fix_args in non-emulate...so need to do
  // something with 's'

}






int check_backw(uint addr, SBK *caller, int type)
{
// check if jump backwards for possible loop condition

  int ans, jump;
//  OPC *opl;

  ans = 0;           // continue

// basic address stuff first................

   jump = addr - caller->curaddr;                         //difference

  if (bankeq(addr, caller->curaddr))
     {                                                    // in same bank
      if (addr == caller->nextaddr) ans = 1;              // dummy jump, ignore for scans
      if (addr == caller->curaddr)  ans = 3;              // loopstop

      // backward jump, scanned already and conditional -- why not static too ??
      if (type == J_COND && jump < 0 && get_copcode(addr)) ans = 2;

         if (type == J_STAT && jump < 0 && get_copcode(addr)) ans = 2;   // TEST !!

      if (type == J_STAT)
       {
        // check for bank start, safety loopstops and opening jumps.
        if (nobank(addr) < 0x2010) end_scan (caller);                // END of this block

         if (jump == -1)
           {
            //some bins do a loopstop with di,jmp...............
            if (g_byte(addr) ==  0xfa) ans = 3;
           }

//another check here for backwards ??


       }
     }

  if (ans > 2 && (caller->proctype & 4))        //emulating)
    {    // only check for real loopstops in emulate
        #ifdef XDBGX
     DBGPRT(1,"Loopstop detected (%d) at %x", ans, caller->curaddr);
     #endif
 //    if (!caller->lscan)
 caller->stop = 1;
    }

  return ans;
}



/*
void add_arg_data(uint start, uint ofst)
  {
   ADT *a;
   uint val, i,sz;
   void *fid;

 //  DBGPRT(1,"XZARG DATA for %x", ofst);

   fid = vconvi(start);         //a = start_adnl_loop(&chadnl,start);    //    (ADT*) chmem(&chadnl,0);
 //  a->fid = fid;
 //  a->seq = 0;

 while ((a = get_adt(fid,0)))    //      next_adnl(&chadnl,a)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     val = decoxde_addr(a, ofst);
     sz = bytes(a->fend);

     add_cmd (val,  val+sz -1, sz, ofst);     //change to dtk ???
     ofst += sz;
    }
    fid = a;
  }


}




void skip_args(SBK *s)
 {
     //called in scan_blk to skip over any args found.

   LBK *k, *e;
   int i;

   k = (LBK*) 1;   // to start loop..

   while (k)
    {
     k = get_aux_cmd(s->curaddr,C_ARGS);
     if (k)
       {
            if (!k->size)
         {
          #ifdef XDBGX
           DBGPRT(1,"precheck ARGSIZE IS ZERO !! %x", s->curaddr);
         #endif
         }

     if (!k->size)
         {
          #ifdef XDBGX
           DBGPRT(1,"ARGSIZE IS ZERO !! %x", s->curaddr);
         #endif
         s->curaddr ++;
         }

    //      fix_xargs(k,s);  //but regstats deleted by now...........
    //      add_arg_data(k->start, s->curaddr);

        s->curaddr += k->size;





        #ifdef XDBGX
         DBGPRT(1,"SKIP %d args -> %x", k->size, s->curaddr);
        #endif

return;             //test


        // fix args with new sizes, encode, etc
        // cleanup reg status and arg holders
        // clear rgargs matching k->start
        // clear emuargs matching k->start



          for (i = 0; i < EMUGSZ; i++)
             {
               e = emuargs[i];
                   #ifdef XDBGX
               if (e) DBGPRT(1,"EMUARGS = %x (%d)", e->start, i);
              #endif
               if (e && e->start == k->start)
               {
                #ifdef XDBGX
                DBGPRT(1,"drop EMUARG %d = %x", i, e->start);


                if (e->end != k->end)


                DBGPRT(1,"size mismatch EMUARG %d = %x", i, e->start);
                #endif

         //       emuargs[i] = 0;
               }
             }
        }
       }
    }


*/



/*
void scan_subbs(SBK* caller, INST *c)
 {
  // scan proc sub branches too see if stack fiddling
  // is in branches.  Not reqd if already flagged
  // ADAPT for twin role as subr-sub or gap check ??

  CHAIN *x;
  SBK *s;
  uint ix, num;

  if (!caller) return;
  ix = 0;

  x = get_chain(CHSBCN);

  num = x->num;

  while (ix < x->num)
   {

    s = (SBK*) x->ptrs[ix];

    if (!s->stop && !s->inv && s->scnt < 10)
      {
        if (s->substart == caller->substart)
         {
          #ifdef XDBGX
             DBGPRT(1,0);
             DBGPRT(0,"Subb Scan %d ", ix);
          #endif
          scan_blk(s, c);
          if (x->num != num)
            {
             #ifdef XDBGX
             DBGPRT(1,"NEWCH %d %d", x->num, num);
             #endif
             ix = -1;   // rescan if changed
             num = x->num;
            }
         }
      }
    ix++;
   }

  emptychain(CHSBCN);       // clear all temp subdiv branches , not main entries..
 }
*/

void fakepop(SBK *s)         //was sbk
{
  // drop newest stack entry - underflow safe
  // as push, copy whole stack if necessary,
  // less efficient, but better for keeping track
  // KEEP last pop in entry zero, which is useful for
  // stack shuffling par getters

  FSTK *t;
 // SBK *ans;

  t = get_stack(s->proctype,0);        //swops for emul and scan
//  ans = t[0].sblk;
  memmove (t, t+1, sizeof (FSTK) * (STKSZ-2));    // shuffle sce down, (over [0])
  memset  (t+ (STKSZ-1),0, sizeof(FSTK));          // and clear last entry

//  return ans;
}


int match_stack(INST *c, SBK *s, FSTK *t, uint types)
{
 // get stack entry, with bit mask of types allowed
  int match, ans;
//  SBK *s;

//  s = c->scanblk;

  match = 0;
  ans = 0;         //t->newadd;   // default return;

  if (t && t->sblk)  ans = t->sblk->nextaddr;

  if (t->ftype & types) match = 1;        // OK

 /* switch(t->type)
    {

     default:
     case 1:    // std call, default
     break;

     case 2:    // imd
       ans = t->psw;          // return address as pushed
       break;

     case 4:                  // pushp addr = psw bank goes in 10-13
       ans = t->psw;         // psw
       ans |= ((t->origadd >> 6) & 0x3c00);   // add callers bank in correct place
       break;
   }  */

if (!match && (s->proctype & 4))              //s->emulating)
  {
    if (t && t->sblk) ans = t->sblk->nextaddr;

    #ifdef XDBGX
    DBGPRT(1,"STACK TYPE not match %x != %x at %x", t->ftype, types, s->curaddr);
    DBG_stack(s->proctype);        //,emulstack);
    #endif
  }

return ans;
}



void fakepush(SBK *caller, SBK *me, int addr, int type)
{

  //design keeps virtual stackpointer at ZERO.
  //same as climbing caller chain really.


// add stack entry - overflow safe
// stack shuffle is less efficient than a pointer,
// but will always have newest PUSH at zero which is neater.
// Right alignment for [R22+x] style multibank ldxs

// type 1 = std call, 2=imd, 4 = pushp as bit
// always have newaddr and origaddr as real addresses, so if it is
// used incorrectly for a RET, it will just continue.


// BUT BUT BUT - it may be possible this gets out of step for multiple calls to same subrs ??
//FSTK is shared and so could be screwed up as calls aren't properly reliable if pops go back far enough ??








  FSTK *t;

   if (!caller) return;

   t = get_stack(caller->proctype,0);          //actually maps to entry [1]

   if (caller->proctype & 3)
    {
      // don't do immediate pushes in scan or mode
      if (type == 2) return;
 //     t = scanstack;
     }

   memmove (t+1, t, sizeof (FSTK) * (STKSZ-1));  // shuffle stack up
   memset(t,0, sizeof(FSTK));       // clear entry[0] safety

  if (type < 4) t->sblk    = caller;
   t->ftype    = type;

   t--;     // entry [0]
   t->sblk = me;
   t->ftype = 1;           //call

}

void do_scan_jmps(SBK *s, INST *c)
{
  CHAIN *x;
  uint ix;
  JMP *j;
  SBK *z;



#ifdef XDBGX
DBGPRT(0, "in do scanjumps %x %x %d", s->start, s->nextaddr, s->type);
DBGPRT(1,0);
#endif


//if (s->nextaddr == s->start)
//{
//    DBGPRT(0,0);
//}





  x = get_chain(CHJPF);

  ix = get_fjmp_ix(CHJPF,s->start);

  while (ix < x->num)
   {
      j = (JMP*) x->ptrs[ix];
       ix++;

       if (j->fromaddr > s->nextaddr) break;

//A9L - backwards jumps ?? should be allowed ? or is it return.....and need new logic for multibanks anyway, but principle proved.
 // bswp == 1

       if ((!j->back && j->toaddr > s->nextaddr) || j->bswp)
       {  // forward jump, outside block, or bank swop
   //       DBGPRT(1,"zxzx fwd %x from %x", j->toaddr, j->fromaddr);

        if (j->jtype == J_COND && !get_copcode(j->toaddr))
           {   // unscanned conditional
               z = add_scan (j->toaddr, J_COND, s);
               scan_blk(z, c);
           }

       if (j->jtype == J_STAT && !get_scan(j->toaddr, 0))
         {  //static jump not scanned
            z = add_scan (j->toaddr, J_STAT, s);
        //    if (!get_lasterr(CHSCAN))
          //    {
                scan_blk(z, c);             //&cinst);
          //    }
         }
       }

       if (j->back && j->toaddr < s->start)
           // backwards jump, outside block
       {
 //   DBGPRT(1,"zxzx back %x from %x", j->toaddr, j->fromaddr);
         if (j->jtype == J_COND && !get_copcode(j->toaddr))
           {        //check opcode for conds.
                z = add_scan (j->toaddr, J_COND, s);
                 scan_blk(z, c);
           }

         if (j->jtype == J_STAT && !get_copcode(j->toaddr))
           {

            z = add_scan (j->toaddr, J_STAT, s);
             scan_blk(z, c);        //&cinst);
           }
        }
}

}



int check_caller_chain(int addr, SBK *caller)
 {
     // only called from sjsubr CALL (subroutines)
  SBK * x;
  int ans, cnt;

  ans = 0;
  cnt = 0;
  x = get_scan(addr,0);
  if (!x) return ans;          // no block

  while (caller)
    {
      if (x == caller)
        {
         ans = 1;
         break;
        }
      caller = caller->caller;
      cnt++;

      if (cnt > 1000) break;
    }
 return ans;
}



void check_emulate(SBK *caller, SBK* newsc, INST *c)
 {
   SBK *x ;     // , *caller;                        // new scan, or temp holder
   SUB *sub;
   ADT *a;
   uint saveddbnk;
 //  CINST *z;

   if (!newsc) return;

    a = NULL;

   sub = get_subr(newsc->substart);
   if (sub) a = get_adt(vconvi(sub->start),0);

   if (a)
       {     // args set by user command no args, just skip
                  //note that cannot do multi level args in a command...................


               #ifdef XDBGX
               DBGPRT(0,"CMD ARGS ");
               #endif
               caller->nextaddr += sub->size;          //sub->adnl);
               //return ?
       }
           else
            {
             if (newsc->argsget)
              {
                   // this is already in chain somewhere ??  need to ESCAPE and restart
               x = get_scan(caller->substart, caller->substart); // this is only true if single level args getter ?? and not if static jump....
               if (x)
                 {
                  #ifdef XDBGX
                   if (!x->emulreqd && x != newsc)
                     {
                       DBGPRT(0,"%x - ARGS Set emulreqd for %x (%x)", c->ofst, x->start, x->substart);
                       DBGPRT(1,"from - new %x (%x) caller %x (s%x) %x", newsc->start, newsc->substart, caller->start, caller->substart, caller->curaddr);
                     }
                  #endif
                  x->emulreqd = 1;
                 }               // from substart

                 // but if this is in the current caller chain, need to go back to it ???
                 // find scan doesn't check if scan is in current chain....and just assumes args are in CALLER,
                 // which is NOT always true........
              }

              // args command created in push (must do that to get number of bytes right) but sizes done later....
              //else       //TEST !!  breaks 7aa1 (728d) fixes 7949 (796c)

              //something to do here with argsget to flag relaibility of arg sizes ???

             if (newsc->emulreqd)
               {
                 newsc->caller = caller;         // reset for stack rebuild (redundant?)
                 caller->regstat = 1;                              // keep sizes until caller block is done....
                 saveddbnk = basepars.datbnk;
                 emulate_blk(newsc);             // and emulate it  maybe use caller...for arg sizes AFTER actual call.
                 basepars.datbnk = saveddbnk;
               }
           }


           //resume at caller->nextaddr,
         }







SBK* do_sjsubr(SBK *caller, INST *c, int type)
{
  /* do subroutine or jump
  * new scanned block (or found one) may become holder of this block
  * for subroutine special funcs (and any args)
  */

  SBK *newsc;                           // new scan, or temp holder
  int addr;
  JMP* j;

  if (!caller->proctype) return caller;        //not sigs
  if (get_cmdopt(OPTMAN)) return caller;                // no scans, use default

  addr = c->opnd[1].addr;
  j = 0;

  switch (type)                                // requested scan type
    {

     default:
       #ifdef XDBGX
       DBGPRT(0,"Error at %x unknown type =%d", caller->curaddr,type);
       #endif
       break;

     case J_RET:                     // return

         // end block and drop back to caller emulate and scan
         //(from caller->curaddr....)
         // if substart ?

         if (caller->caller) add_jump(caller, caller->caller->nextaddr, J_RET);
         else
         j = add_jump (caller, caller->curaddr, J_RET);

    if (j && j->jelse)
         if (caller->incond > caller->curaddr) break;         // TEST - seems OK
     //    if (caller->incond > caller->curaddr) break;              //TEST !!

         // j->bswp for bank swop ...
         end_scan (caller);        // END of this scan block

         do_scan_jmps(caller,c);

         //clear all stack popped here ?? seems to work....
         {

            FSTK *t;
            uint i;
 t = get_stack(caller->proctype,0);

      for (i=0; i < STKSZ; i++)
      {           // clear all popped

       t->popreg = 0;
      }
         }





         break;


     case J_COND:                        // conditional jump

        /* need add scan for 'else' case where there is a static 'goto' e.g. 276a A9L, 2112 AA
         can't really do anything else as jump may be outside the scan block
         could mark as else if track the main block as 'in cond'

         CHANGE to set highest cond jump and ignore all scans in between, roll through

         * backwards conditionals are probably always local loops
         * duplicates screened out by add_scan unless can figure out conditionals differently
        */

          add_jump(caller, addr, type);             // jumps BEFORE checks (for loopstops)
          if (check_backw(addr, caller, type)) break;     // backward jump probably a loop

          if (addr > caller->curaddr && addr > caller->incond) caller->incond = addr;           // TEST seems OK

          if (get_cmdopt(OPTLAB)) add_autosym (2, addr);                 // add autolabel if reqd

    //    -No-  add_scan (addr,type, caller);                   // where cond continues


          break;

     case J_STAT:                       // static jump
        /* backwards static jumps are also probably a code loop as well, but some bins
         * have a common 'return' point, so can't assume this

         CHANGE to ignore if in conditional jump scope.

         * also a static jump can go to a subroutine start, as per signed and
         * unsigned function and table lookups, and so on.
         */

         j = add_jump(caller, addr, type);

         if (j && j->jelse)
         if (caller->incond > caller->curaddr) break;         // TEST - seems OK

          //but can still have data block - need more checks !!!



         end_scan (caller);                                   // END of this block
         if (c->save) add_opcode(c);                          // save before reuse
         do_scan_jmps(caller,c);

         if (check_backw(addr, caller, type)) break;          // backward jump probably a loop

         if (get_cmdopt(OPTLAB)) add_autosym (2, addr);       // add autolabel if reqd


         newsc = add_scan (addr, type, caller);               // new or duplicate scan
         scan_blk(newsc,c);                                   // here would become return newsc;

      //   do_scan_jmps(caller,c);            //caller,c);       //newsc ??
       //  return newsc;
         break;

     case J_SUB:

         add_jump(caller, addr, type);

         if (check_backw(addr,caller,type)) break;
         if (check_caller_chain(addr, caller)) break;

         newsc = add_scan (addr,type,caller);          // nsc = new or zero if dupl/invalid


//call after jump not work after merging substart !!

//log_spf  for tab and func pars ??)
//but WILL need to track back if jump....(aa 2e48)


         //if !newsc duplicate maybe add the ret ??

         fakepush(caller, newsc, addr,1);               // need this for later emulation check
         if (c->save) add_opcode(c);                    // before reuse

         // but scan-blk scans whole block....so could still get faulty addresses...
         // should emulate from the PUSH (when scanning for first time)
         // but once scanned this works as expected, emulated as soon as called

         scan_blk(newsc,c);

        // scan subbranches of this subr for emulation before subr exit
     //   scan_subbs(newsc, c);

    check_emulate(caller, newsc, c);

    #ifdef XDBGX
        DBGPRT(1,"Resume at %x",caller->nextaddr);
        DBGPRT(1,0);
    #endif
        fakepop(caller);

    }        // end switch
return caller;
}



/***********************************************
 *  calculate operand parameter number 'ix'
 *  from 'val' (normally register)
 *  assumes read size already set in operand cell (OPER)
 *  sym names and special fixups done in print phase
 *  sets fields reg (=register) addr (= register) and
 *  val = [reg] (via) fend (size), or val = [rbs_address] (word)
 // indirect and indexed modes handled in calc mult_ops
 ************************************************/

void op_calc (int val, uint ix, INST *c)
  {
  OPER *o;
  RBT *b;

  o = c->opnd+ix;

  if (o->optype == OPBIT) return;             // JB JNB bit flag, handled outside

  if (o->optype == OPIMD)                     // immediate
    {
      o->val  = val;                              // imd val
      o->addr = databank(val,(CINST*) c);         // imd as address
      return;
    }

  // all register from here

  o->reg = val & 0xff;                     // strip bank and safety
  o->reg += basepars.rambnk;               // rambank always zero if 8061

  // rambank + reg goes into addr ??

  // register.  mostly o->addr = o->reg
  // for 8065 direct address word, or greater if odd,
  // change to bank+1 if currently even bank
  // change back to ALL operands (only ix 1 ?? still not quite nailed down)

  if (get_cmdopt(OPT8065) && (o->reg & 1) && bytes(o->fend) > 1 )
    {
        //probably need SFR check !!  valid_reg == 3 for SFR
        //maybe still drop odd bit (Ford manual)

     if ((o->reg & 0x100) == 0) o->reg += 0x100; //add bank
     o->reg ^= 1;         // drop odd bit, even if no bank change
    }

  o->val  = g_val(o->reg,0, o->fend);    // get value from register, sized by op

  o->addr = o->reg;         // generic - addr = reg as default.  Indirect/index will change this.

  // need better check here !

if (!ix)     return;                // not op[0]
if (ix > 2 ) return;                // not op[3]
// if (c->numops == 1) return;         // always writes to single op (not true !! push doesn't !!)


//  if (ix && ix < 3)
 //   {         // only if 1 or 2 ops and not 1 op (3 ops done elsewhere)

     b = get_rbt(o->reg,c->ofst);                  // check if rbs valid (may be user command)

     if (b && c != &einst)
       {                                            // don't do rbase if in emu mode
        o->rbs = 1;
        o->val = b->val;                            // get value from RBASE entry instead, (includes bank)
        // but isn't this an ADDRESS ?
       }

//}
  }

void op_imd (INST *c)
{
  // convert op[1] to imd
  OPER *o;

  o = c->opnd + 1;
  o->optype = OPIMD;
  o->rbs  = 0;
  o->reg  = 0;
  if (basepars.rambnk) o->addr -=  basepars.rambnk;  // remove any register bank
  o->addr  = nobank(o->addr);                         // and any ROM bank
}





/******************************************
attempt to decode vector or vector list from push word or from a sig...
****************************************/





uint test_code_blk(uint addr, SBK *caller)
{

 // SBK *blk;

// CINST *c;

  if (!val_rom_addr(addr))
    {
      #ifdef XDBGX
         DBGPRT(1,"invaddr %x ",addr);
      #endif
      return 0;
    }


/*  build_fake_stack here !!


// TEST !! first, copy caller into ALT chain so duplicates and loops handled
//this gets deleted at end of test


// do equivalent of chain build for emulate.....

blk = (SBK*) chimem(ALTSCAN);//doesn't do this any more !!

*blk = *caller;

blk->caller = 0;

caller = blk;

// add_scan ?
 //  ix = bfindix(x, blk);          // find again in case stuff deleted/merged
  //     chinsert(ch, ix, blk);         // do insert at ix
chinsert(ALTSCAN,0,blk);               //  0 should be fine, empty chain NO !!!

// must now use C_ALT to use parallel chains

//maybe copy caller into alt chain for duplicates ??

// may need an add-test-scan like add-escan


// !! ARGS !!



#ifdef ALTTEST

  blk = copy_scanchain(caller, addr, ALTSCAN);
 //  blk = add_scan (addr, J_SUB|C_ALT , caller);  // C_ALT TEST mode
  scan_blk(blk, &vinst);

  if (!check_altscan_chain())                //0 = true
    {
      add_scan (addr, J_SUB, caller);      // add a real scan
      return 1;                            // success
    }
#endif


#ifndef ALTTEST */


 // blk =

//if (addr == 0x95900)
//{
// DBGPRT(0,0);
//}

  //    c = get_nearcopcode(addr);        //move to scan ??  use testvec as part of check.
// CINST *get_nearcopcode(uint addr)


//if (c)
//{
//    DBGPRT(1, "opc %x  add %x", c->ofst, addr);
//}

  add_scan (addr, J_SUB|C_TEST , caller);


  //don't need the scan immediately for vects.....


   //    if (s->tstvect) cleantestdata();

 // scan_blk(blk, &vinst);                 // test, take scan away
  return 1;

  return 0;
}

//------------------------------------score system


void set_vect_list(SBK *caller, INST *c, uint start, uint end)
 {

// no checks..................
  uint pc, vcall;
  LBK  *s;
 // SBK *blk;

  if (anlpass >= ANLPRT) return;

  pc = start;

  // would need to do PC + subpars for A9L proper display

   #ifdef XDBGX
    DBGPRT(1,"set vect list %x-%x", start,end);
   #endif

  for (pc = start; pc < end; pc +=2)
    {
     vcall = g_word (pc);                            // address from list
     vcall = codebank(vcall, c);                // assume always in CODE not databank

     //check if opcode is valid first....??

     if (val_rom_addr(vcall))
       {
         #ifdef XDBGX
           DBGPRT(0,"vect %x - ", pc);
         #endif
        //blk =
        add_scan (vcall, J_SUB, caller);                     // vect subr (checked inside cadd_scan)
    //    scan_blk(blk, &vinst);
       }
    }

 s = add_cmd (start, end, C_VECT);

if (!get_lasterr(CHCMD))
 {
   if (s && g_bank(caller->start) != g_bank(start))
    {
    // add bank to command
    ADT* a;
    a = (ADT*) chimem(CHADNL);
    a->fid = vconvi(s->start);
    a->bank = caller->start >> 16;
    append_adt(a);
   }
 }

 /* s = add_cmd (start, end, C_VECT);
  if (s && basepars.codbnk != basepars.datbnk)
    {
     // bank goes in ONE addn block to vect.
       a = append_adt(vconvi(s->start),0);
       if (a) {
       a->bank = basepars.codbnk >> 16;                  // add current codbank
       if (g_bank(vcall) == g_bank(start)) a->cnt = 0;  //don't print if match
    //   s->adt = 1;
       }
    } */
     #ifdef XDBGX
     DBGPRT(1,"END vect list");
     #endif
 }

/******************************************
attempt to decode vector or vector list from push word
probably need to split off the vect list (for multibanks)
****************************************/


int do_vect_list(SBK *caller, INST *c, uint start, uint sz)
{
   uint score;
   uint i, addr, lowcalladd, lastinvadd, end;
   SBK *s;
   LBK *l;

   // assume bank is correct in start address
   if (nobank(start) < PCORG) return 0;


   l = (LBK *) get_cmd(start,0);
   if (l)
     {               //already a command here.
          #ifdef XDBGX
           CSTR *cmds;
           cmds = get_cstr(CMDSTR, l->fcom);
           DBGPRT(1,"hit cmd %x %s",start, cmds->string);
          #endif
     return 0;
     }

   if (sz) end = start + sz;               // 0 to sz is an extra pointer sometimes.
   else   end = start + 0xff;             // 128 vectors max (A9L is 0xa4)

   score = 0;                              // max score
   lowcalladd = maxadd(c->ofst);           // lowest call address - for ending list
   lastinvadd = 0;

  // could check start against curinst+cnt to see if an extra offset needs to be added ?
   //this needs probably more checks

   if (start & 1) start++;             // can't start on odd byte, but end should be
   if (!(end & 1)) end --;             // drop last byte if even

   #ifdef XDBGX
    DBGPRT(0,"In do vect %x-%x sz=%x (%d)", start, end, sz, sz);
    DBGPRT(1," from %x", c->ofst);
   #endif

   for(i = start; i <= end; i+=2)       // always word addresses  (score > 0?)
     {

      if (i >= lowcalladd)
       {
         #ifdef XDBGX
         DBGPRT(1,"hit call at %x - stop", i);
         #endif
         end = i-1;
         break;
       }

      s = (SBK*) get_scan (i,0);
      if (s)
        {
         #ifdef XDBGX
          DBGPRT(1,"hit scan %x - stop", i);
         #endif
         end = i-1;
         break;
        }

      l = (LBK *) get_cmd(i,0);
      if (l)
        {
          #ifdef XDBGX
           CSTR *cmds;
           cmds = get_cstr(CMDSTR, l->fcom);
           DBGPRT(1,"hit cmd %x %s",i, cmds->string);
          #endif
          end = i-1;
          break;
        }


      // now check the vect address itself

      // add extra check for end address, as size can be >,<, <=

      addr = g_word (i);
      addr = codebank(addr, c);

      if (val_rom_addr(addr))
       {                             // valid address

        if (addr > start && addr < lowcalladd) lowcalladd = addr;       // keep lowest call

        if (addr && (addr & 0xff) == 0)     // suspicious, but possible
           {
            score += 50;         //just add half
            #ifdef XDBGX
              DBGPRT(1,"Suspicious %x = %x",i, addr);
            #endif
           }
        else score +=100;           // valid address
       }

      else
        {          //invalid address
         #ifdef XDBGX
           DBGPRT(1,"inv add %x (%x)",i, addr);
         #endif
         if (i == start) start +=2;    // first address is invalid, move start is valid

         if (i == lastinvadd+2)
            {              //consecutive invalids - stop
              end = lastinvadd-1;
              #ifdef XDBGX
                DBGPRT(1,"STOP (2 INVS) at %x",end);
              #endif
              break;
            }
         lastinvadd = i;           // keep last invalid pointer
        }


       // more scores ??


   }  // end main loop

  if (lowcalladd < end) end = lowcalladd-1;

  addr = (end-start+1)/2;             // no of vect pointers
 #ifdef XDBGX
   DBGPRT(1,"score vect %x-%x (%d items)|lowcall %x|score %d", start, end, addr, lowcalladd,score);
  #endif

   if (addr < 8)
    {                // is a minimum of 8 pointers
     #ifdef XDBGX
     DBGPRT (1,"REJECT LIST %x from %x, %d ptrs",start, c->ofst, addr);
     #endif
     return 0;
    }

  {
     score = score/addr;
      #ifdef XDBGX
     DBGPRT (0,"FINAL SCORE = %d ",score);
      if (score > 95) DBGPRT (0,"PASS"); else DBGPRT(0,"FAIL");
     DBGPRT(1,0);
     #endif

     if (score > 95) set_vect_list(caller, c, start,end);
     return 1;
   }
  return 0;
}




//----------------------------------------
/*

void shift_comment(INST *c)
 {
  //  do shift replace here - must be arith shift and not register
  int ps;

  if (anlpass < ANLPRT) return ;              // redundant ?

  if (c->opcix < 10 && c->oper[1].imd)
   {     // immediate shift
    if (zero_reg(c->oper + 2))
      {  // R0, timewaster
       ps = sprintf(nm, "#[ Time delay ?] ");
      }
    else
     {
      ps = sprintf(nm, "#[ \x2 ");
      if (c->opcix < 4)
       ps += sprintf(nm+ps,"*");
      else
       ps += sprintf(nm+ps,"/");
      ps += sprintf(nm+ps, "= %d] ", 1 << c->oper[1].val);
     }
    acmnt.ofst = c->ofst;
    acmnt.ctext = nm;
    acmnt.minst = c;
  }
}

*/





void cpy_op (INST *c, uint dst, uint sce)
{
   // straight copy, but keep the orig size for write op.
   uint fend;


   fend = c->opnd[dst].fend;
   *(c->opnd+dst) = *(c->opnd+sce);
   c->opnd[dst].fend = fend;
}

 void decode_other_ops(uint xofst, INST *c)
  {

   OPER *w;
   int nops;

   w = c->opnd+3;                   // write op, fend set if there is one
   nops = c->numops;

   if (nops == 1 && w->fend)
     {
       cpy_op(c,3,1);               // copy [1] to write op [3] if write op

   //    rbinvcheck here !!rb_inv
     }

    if (nops > 1)
     {                                      // calc op[2] as read op
       op_calc (g_byte(xofst + 2), 2,c);
     }

   if (nops == 2 && w->fend)
     {
        // [3] = [2] op [1]  copy op[2] to [3]
        cpy_op(c, 3,2);

        //invcheck [2] ??
     }

  if (nops == 3)
     {
       // [3] = [2] op [1] calc op 3 (always write)
       op_calc (g_byte(xofst + 3), 3,c);
      }

  }


 void decode_dflt_ops(SBK *s, INST *c)
 {  // default - all operands as registers

   c->opcsub = 0;         // no multiple mode

   op_calc (g_byte(s->nextaddr + 1), 1,c);

   decode_other_ops(s->nextaddr,c);

   s->nextaddr += c->numops + 1;

//    if (s->emulating)
// do_regstats(s,c);
 }



void decode_mult_mode(SBK *s, INST *c)
{
  /******************* multiple subtype instructions *****************
    * opcsub values   0 direct  1 immediate  2 indirect  3 indexed
    * order of ops    3 ops       2 ops      1 op
    *                 dest reg    dest reg   sce/dest reg
    *                 sce  reg    sce  reg
    *                 sce2 reg
    * NB indexed mode (3) is a SIGNED offset
    *******************************************
    * Adjust xofst for true size of opcode
    * +2   opcsub 3, if 'long index' flag set
    * +1   opcsub 3, if not set (='short index')
    * +1   opcsub 1  Word op (immediate)
    ******************************************************/

     int firstb,xofst;          //,addr;
     OPER *o;

     xofst = s->nextaddr;
     firstb = g_byte(xofst+1);             // first data byte after (true) opcode

     o  = c->opnd+1;                         // most action is for oper[1]

     switch (c->opcsub)
      {
       case 1:                                          // first op is immediate value, byte or word
         op_imd(c);                                     // op[1] is immediate
         if ((o->fend & 31) > 7)                        // word mode
          {                                             // recalc op1 as word
           xofst++;
           op_calc (g_word(xofst), 1,c);
           c->opsize++;
          }
         else
          {
           op_calc (firstb, 1,c);                       // byte calc for op[1]
          }

         break;

       case 2:     // indirect, optionally with increment - add data cmd if valid
                   // ALWAYS WORD address.

         o->optype = OPIND;                         // indirect op

         if (firstb & 1)
          {                                         // auto increment
           firstb &= 0xFE;                          // drop flag marker
           o->optype = OPAINC;                      // IND + autoinc
          }

         op_calc (firstb, 1,c);                     // calc [1] for new value, always even

         // recalc for indirect after opcal
         // o->addr = register, o->val = [reg] or [rbs]
         // can be marked rbs BUT not valid if incrementing....

         if (!o->rbs || o->optype == OPAINC)  o->val = g_word(o->reg);      // recalc, always word based


         o->addr = databank(o->val, (CINST*) c);     // o->val is an address
         o->val = g_val(o->addr, 0, o->fend);        // get value from address, sized by op
         break;

        // indexed, op[0] is a fixed offset
       case 3:
        {
         OPER *z;                 // operand [0]
         z =  c->opnd;
         if (firstb & 1)
            {
             z->fend = 15;                         // offset is word (unsigned)
             firstb &= 0xFE;                       // and drop flag
             c->opsize += 2;
            }
          else
            {
             z->fend = 39;                         // short, byte offset (signed)
             c->opsize++;
            }

         z->optype = OPOFF;                         // op [0] is an address offset for index
         o->optype = OPINX;
         op_calc (firstb, 1,c);                     // calc op [1] (always even)

     //   recalc for an indexed op  o->addr = final address, o->val = [addr]

         if (!o->rbs) o->val = g_word(o->reg);             // recalc, Always word for indexed
         z->val = g_val(xofst+2,0,z->fend);                // get offset - may be negative  z->addr ? falls over in sigs
         if (z->val < 0 && (z->fend & 0x20)) z->neg = 1;   // flag negative if signed
         else z->addr = databank(z->val, (CINST*) c);      // keep [0].addr as possible 'base' address

         o->addr = o->val + z->val;
         o->addr = databank(o->addr, (CINST*) c);          // get bank, wrap o->addr in 16 bits in op [1]

         o->val = g_val(o->addr,0, o->fend);               // get value from new address, sized by op
         xofst += bytes(z->fend);                          // add size of op zero
        }
         break;

      default:                                 // direct, all ops are registers
         op_calc (firstb, 1,c);                // calc for op[1]
         break;
      }

    //calc rest of ops as plain registers

    decode_other_ops(xofst, c);

    s->nextaddr = xofst + c->numops + 1;


//     if (s->emulating)
  //  do_regstats(s,c);

  }


void pset_psw(SBK *s, int t, int mask)
 {
  // preset ones or zeroes

   mask &= 0x3f;                     // safety

   if (t)
    {
      s->psw |= mask;     // set defined bits
    }
  else
    {
      mask = ~mask;       // flip bits
      s->psw &= mask;     // clear defined bits
    }

 }

void set_psw(SBK *s, int val, int fend, int mask)
 {

    s->psw &= (~mask);     // clear defined bits

  /* set bits according to actual state
    #define PSWZ(x)   (x->psw & 32)        // Z    zero flag
    #define PSWN(x)   (x->psw & 16)        // N    negative
    #define PSWOV(x)  (x->psw & 8)         // OVF  overflow
    #define PSWOT(x)  (x->psw & 4)         // OVT  overflow trap
    #define PSWCY(x)  (x->psw & 2)         // CY    carry
    #define PSWST(x)  (x->psw & 1)         // STK  sticky
*/

    if ((mask & 32)  && !val)      s->psw |= 32;       // zero
    if ((mask & 16)  &&  val < 0)  s->psw |= 16;       // negative

    if (mask & 2)
        {           // carry
         s->psw &= 0x3d;          // clear CY
         if (val > (int) get_sizemask(fend)) s->psw |= 0x2;            //cnv[size].sizemask) s->psw |= 0x2;      // set CY  camk[size]
        }
    if (mask & 8)
        {           // OVF
         s->psw &= 0x37;          // clear OVF
         if (val > (int) get_sizemask(fend)) s->psw |= 0x8;      // set OVF
        }

     if (mask & 4)
        {           // OVT - don't clear, just set it.
         if (val > (int) get_sizemask(fend)) s->psw |= 0x4;      // set OVT
        }
}



int find_vectlist_size(SBK *s, CINST *c, uint ix)
{
    // see if a cmp can be found with register.
    // go back to see if we can find a size
    // allow for later ldx to swop registers

// ix always 1 ?
// could add nrml here too (would be max 32)

      int size, xofst, i;
      uint reg;

  #ifdef XDBGX
    DBGPRT(1,"%x in find vectsize",s->curaddr);

 #endif

      reg = c->opnd[ix].reg;
      xofst = c->ofst;

      c = &sinst;

      size = 0;
      i = 0;

      c = find_opcode(xofst, 0);   //  get current

      while (c)
        {  // look for an LDX

         i++;
         if (i > 15) break;

         if (c->sigix == 12)
            { // ldx from another register, change register

              if (c->opnd[3].addr == reg)
               {
                 reg = c->opnd[1].addr;
                }
            }

          if (c->sigix == 10)
             {                      // compare - found ?
              if (c->opnd[3].addr == reg)
              {  // cmp
               if (c->opcsub == 1)
                 {                             // found cmp for register
                  size = c->opnd[1].val;       // immediate compare, use size
                  return size;
                 }
              }
             }

          c = find_opcode (c->ofst, 0);    // get next ofst in h....
          if (c) xofst = c->ofst;                   // new ofst

        }           // while

return 0;
}






SBK* pshw (SBK *s, INST *c)
{
  OPER  *o;
  int  size, i;
 // SIG *g;
  FSTK *t;
  RST *r;
//  POP *p;

  decode_mult_mode(s,c);

  // Push does NOT push databank, need codebank for addresses
  //  push and pop use current codebank. this is why pushp popp around subrs.
  // does not change PSW

  // if scanning, check for vect pushes first (list or single)
  // immediate value is normally a direct call if valid code addr
  // if indexed or indirect do list check
  // it std register, assume it's an argument getter

  o = c->opnd + 1;

  // add codebank to address if imd, also for print or sign

  // ****  if pop follows immediately don't do any processing
  // as it's a fancy ldx

   if (g_byte(s->nextaddr) == 0xcf)
    {          // pop follows immediately , don't do anything....
            //rombank ?
      #ifdef XDBGX
       if (s->proctype) DBGPRT(1,"%x Push+pop, nfa",c->ofst);
      #endif
      return s;
    }

  if (o->optype == OPIMD)  o->addr = codebank(o->addr,c);

  if (!s->proctype) return s;


  if (s->proctype & 2)
    {
        // scanning

       // change to a switch on opcsub ?

       if (!c->opcsub)
         {
          // register - tidy up stack

          r = get_rgstat(o->reg);
          if (r)  r->popped = 0;

          for (i=0; i < STKSZ; i++)
            {
             t = get_stack(s->proctype,i);
             if (t->popreg == o->reg)
               {
                t->popreg = 0;
                break;
               }
            }

         }

     if (o->optype == OPIMD && val_rom_addr(o->addr))
       {   // treat as a subr call, but this could be a return address
           // this uses CODE bank, not data
           //change display to addr, not val
           add_scan(o->addr, J_SUB,s);        //no scan ??
       }

     if (c->opcsub == 2)
         {
             // indirect - try to find size and base of list
             // size is same as indexed above, (via cmp)
             // but base must have been added earlier, so look for an ADD, X
             // where x is immediate

            i = find_list_base(c,1,10);              // i has correct bank no for multibanks
            size = find_vectlist_size(s, c, 1);
       //     i = databank(i,c);
            if (i)            //val_rom_addr(i)) done in find_list
              {
               do_vect_list(s, c, i, size);
              }
           }

     if (c->opcsub == 3)
         {
           // indexed - try to find size and 'base' of list
           // ops[0] is base (fixed) addr.  ops[4] (copy of ops[1]) is saved register.
           // go backwards to see if we can find a size via a cmp (find_vect_size)
           // only look for list if [0] is a valid address,

           // try [1] too ?? a combined address may be the right one

           i = databank(c->opnd->addr,(CINST*) c);               // add bank first
           if (val_rom_addr(i))
            {            // vect list from (valid) fixed addr part of index
             size = find_vectlist_size(s, c, 1);
             do_vect_list(s, c, i, size);
            }
         }



    }    // end of if scanning


  if (s->proctype & 4)               //emulating)
    {   // similar to push above, but do args if necessary
        // note that as this is done after each scan or rescan,
        // the stack is already built by sj subr....

     if (o->optype == OPIMD && val_rom_addr(o->addr))    //opcsub = 1
       {   // immediate value (address) pushed

       //but should be able to insert this in correct spot.....
        //but we DO know where to put it....
        // s->caller new block
  //then
          emuscan = add_escan(codebank(o->addr,c),s);            // add imd as a special scan

          //caller ->


          fakepush(s, emuscan, o->addr, 2);                               // imd push
       }
     else
       {
        for (i=0; i < STKSZ; i++)
            {
             t = get_stack(s->proctype,i);
             if (t->ftype == 1 && t->popreg == o->addr)
              { // found
                do_args(c,s, t,i);
                t->popreg = 0;                                 // clear popped.
                r = get_rgstat(o->reg);       //, s->substart);          //get_rgstat(o->addr);
                if (r)
                  { r->popped = 0;
   //                 if (t->origadd == (o->val | g_bank(t->origadd)))         //t->origadd)      / modded check ?? ....
          //           { // no change, delete it
            //          #ifdef XDBGX
           //           DBGPRT(1,"not delete rgstat R%x", o->reg);
            //          #endif
                 //     memset(r,0, sizeof(RST));    //chdelete(CHRGST,get_lastix(CHRGST), 1);              //chrgst.lastix,1);
            //        }
                  }
                break;
              }
            }
       }
    }


  #ifdef XDBGX
 //   DBG_stack(s->emulating);
 if (s->proctype)                      //scanning || s->emulating)
 {
    DBGPRT(0,"%x PUSH t=%d a=%x v=%x",c->ofst,c->opcsub, o->addr, o->val);
    if (s->substart) DBGPRT(0," sub %x", s->substart);
    DBGPRT(1,0);

 }
 #endif
return s;

//check_emuargs done is upd_watch...

}



SBK* popw(SBK *s, INST *c)
{

    //is it worth treating pop as ldx Rx,[R10] (or Rx,[20]) ??
//push must precede ??


    //maybe mark if preceeded by a push? or at least ignore stack processing.


   OPER  *o;
   FSTK *t;
   int i;
   RST *r;
 //  POP *p;
 //  SBK *x;

   // does not set PSW

   decode_mult_mode(s,c);

   if (!s->proctype) return s;

   o = c->opnd+3;      // write operand
   o->val = 0;         // safety

   // can get throwaway POP to R0. Ignore it.
   //under this model can ignore the fakepop
   if (valid_reg(o->addr) == 2) return s;

  #ifdef XDBGX
           DBGPRT(1,"%x POP R%x",s->curaddr, o->reg);
  #endif


// if scanning, don't actually care that there is no stack entry.
//can return 0 and then flag if reg > 0.
// this only really matters in emulate..... so ...but **DO** need where the
// emulate flag goes.
//and note that 7aa1 pop/call/push can get out of sync.

/*
  x = find_opcode(c->ofst,0);              //previous opcode

//push 14, pop 15

  if (x->sigix == 14)
   {       //previous opcode is a push
       FLAG SET.... or return.....
*/


 t = get_stack(s->proctype,0);

   #ifdef XDBGX
   DBG_stack(s->proctype);
   #endif



 if (!t->ftype)  return s;           //pop not valid (no stack entries)


//if (s->proctype == 1)
//{ // o->val already zero (above)


     for (i=0; i < STKSZ; i++)
      {           // scan UP callers stack to a valid unpopped entry.

      if (t->popreg == o->reg && t->sblk)
         {       // should never happen... but 7aa1 kind of messes stuff up...and does happen with
                 // emulate....
           #ifdef XDBGX
           DBGPRT(1,"DUPLICATE POP !! R%x, %x", o->reg, t->sblk->nextaddr);
           o->val = t->sblk->nextaddr;
           #endif
           return s;
         }

        if (!t->popreg)
         {   // this entry not popped yet (or maybe pushed back)
          t->popreg = o->reg;
  //        if (s->proctype == 4)                     // TEST !!       //seems to be OK needs stack anyway for arg address compare
          o->val = match_stack(c, s, t, 3);                   // not pop flags, t but this is
          break;
         }
        t++;
      }





if (i >= STKSZ) return s;


  r = add_rgstat(o->reg,0,s->curaddr);      // zero argaddr for popped address stops duplicates

  if (r)
    {
      r->popped = 1;
      r->fend = 15;             // addresses always word

      #ifdef XDBGX
         DBGPRT(1,"%x Set popped for R%x =%x (%x)", c->ofst, o->reg, o->addr, o->val);
      #endif

    }
       #ifdef XDBGX
DBG_stack(s->proctype);
#endif
  return s;
 }




SBK* clr (SBK *s, INST *c)
  {
    // clear


    decode_dflt_ops(s,c);

// as a [3] = [1] to match other opcodes

  c->opnd[3].val = 0;
  c->opnd[1].val = 0;

   if (s->proctype & 4)              // emulating)
      {
       pset_psw(s, 0, 0x1a);      // clear bits N,V,C
       pset_psw(s, 1, 0x20);      // set Z
      }
      return s;
  }

SBK* neg (SBK *s, INST *c)
  {
      // negate

    decode_dflt_ops(s,c);
    c->opnd[3].val = -c->opnd[1].val;
    if (s->proctype & 4) set_psw(s, c->opnd[3].val, c->opnd[3].fend,0x2e);
    return s;
 }


SBK* cpl  (SBK *s, INST *c)
  {
    // complement bits

    decode_dflt_ops(s,c);
    c->opnd[3].val = (~c->opnd[1].val) & 0xffff;         // size ?

    if (s->proctype & 4)                      //emulating)
     {
      pset_psw(s, 0, 0xa);      // clear bits V,C
      set_psw(s, c->opnd[3].val, c->opnd[3].fend, 0x30);     // N and Z
     }
     return s;
  }



void check_R11(SBK *s,INST *c)
{
  // extra handler for write to Reg 11 for both stx and ldx
  // done AFTER assignment, but before upd_watch
  // R11 is not written to anyway by upd_watch
  int val;
  OPER *o;

  if (!s->proctype)    return;
  if (c->opcsub != 1)  return;     // immediate only

  o = c->opnd + 3;                //c->wop;              // always 3

  if ((o->reg & 0x3ff) != 0x11) return;

  // specifically write to ibuf, to keep it for reads ?

  val = c->opnd[3].val & 0xf;
  val++;
  upd_ram(0x11,c->opnd[3].val,7);

  // do this only if it's an imd as this is reliable, else ignore it ???

  if (bankmap[val].bok)
    {              // valid bank
     basepars.datbnk = val << 16;
     #ifdef XDBGX
      DBGPRT(1,"%x R11 Write imd, DataBank=%d",c->ofst, val);
     #endif
    }
 }



void add_regargs(SBK *s, INST *c)
{
     // for ldx and stx to add/set up args
     // must be reg->reg (opcsub = 0) or autoinc (opcsub = 2)
     // !! if stx, may be a 2 without autoinc.

   RST *src, *dst;
 //  POP *p;     //, *z;            // z is temp
  // CHAIN *x;
 //  uint ix;           // , srcreg,destadd;


//if (s->curaddr == 0x977cc && s->proctype)
//{
 //   DBGPRT(0,0);
//}






   if (c->opcsub == 1) return;            // ignore immediates (maybe also indexed ?)

 //  srcreg = c->opnd[1].reg;

   src = get_rgstat(c->opnd[1].reg);         //, s->substart);         // sce/read register (ldx or stx)


   //but if we do 'add' and get update?

 //  dst = add_rgstat(c->opnd[3].reg,s->curaddr, c->opnd[1].addr);

   if (!src || !src->popped) return;

 //  if (src->ofst != s->substart) return;


 //  psrc = get_rgstat(src->popreg, s->substart);         //popsrc register

 //  if (!psrc || !psrc->popped) return;                 //stops repeats ?

#ifdef XDBGX
    DBGPRT(0,"%x in add_regargs src = R%x", s->curaddr, c->opnd[1].reg);
    DBGPRT(1," dst = R%x (%x)", c->opnd[3].reg, c->opnd[3].addr);
    #endif


    do_one_opcode(s->nextaddr, &tmpscan, &sinst);        //or should do a sig ??

    //   lbd or stb....

    if (c->sigix == 12 && sinst.sigix == 13 && c->opnd[3].addr == sinst.opnd[1].addr)
       { //ldb followed by stb - may be a transfer arg [x++] = [y++];
         // and transfer register matches

   //     if (c->opnd[3].addr == sinst.opnd[1].addr)
      //    {
           // register matches
           // sce is still correct but dst becomes becomes  sinst.opnd[3].addr and argofst c->opnd[1].addr

           dst =  add_rgstat(sinst.opnd[3].addr, c->opnd[1].addr, s->curaddr);
           if (dst)
           {
              dst->arg = 1;                   // maybe arg, or
                  //   dst->tfr = 1;                // source level from pop
                 dst->fend = c->opnd[3].fend;
           }
 //ADBGPRT(0,0);             //this seems to work -got here
 //s->nextaddr = tmpscan.nextaddr;         //skip the stx   can't do this as it messes up autoinc

        //  }
       }
 else    dst = add_rgstat(c->opnd[3].reg, c->opnd[1].addr, s->curaddr);        //with true arg ofst and block start
              if (dst)
                {
                 dst->arg = 1;                   // maybe arg, or
                  //   dst->tfr = 1;                // source level from pop
                 dst->fend = c->opnd[3].fend;

                 #ifdef XDBGX
                 DBGPRT(1,"FLAG ARG R%x at %x sz%d", c->opnd[3].reg, c->ofst, dst->fend);
                 #endif
                }
       //    }          //end if popped




//NO !!
       //   if (src->arg && psrc->popped)              // || src->tfr)
              // reg is being used as a transfer
     //      {
        //       src->tfr = 1;
               src->arg = 0;
               //src->argofst = 0;        ??  may bugger up chain.
       //        if (valid_reg(src->argaddr) == 1)


     //      }


             if (0)       //src->tfr)

           //    if (src->ofst == s->start)
               {       //only within same block as created?


             // firstly set size, and reflect back to source.       SIZES later....
           //  src->fend = c->opnd[1].fend;     //      set_rgsize(src, c->opnd[1].fend);

      //       dst = rgstat + src->popreg;           //get_rgstat(src->sreg);

            #ifdef XDBGX
                 DBGPRT(1,"TFR ARG R%x at %x %x", c->opnd[3].addr, src->reg, src->argofst);
                 #endif

//still need original source (R42)   minus 1 because it's inc'ed. what if it isn't ??

   //            dst = add_rgstat(c->opnd[3].addr, s->substart, g_word(psrc->reg)-1);           //TEMP !! c->opnd[1].addr);       // with true arg ofst
              if (dst) {
              dst->fend = c->opnd[3].fend;     //set_rgsize(dst, c->opnd[1].fend);
dst->arg = 1;


              // if first level arg (next to popped)

          //   if (src->tfr)              //NO use opcsub....or
               {
                // this 2nd level arg, copy sce from first level
                 //  how to stop the arg flag when left subr.........
                 // but could be a 'tfr' holder (A9L 77be)
            //    dst = get_rgstat(c->opnd[3].reg, s->start);        //add_rgstat(c->opnd[3].reg, c->opnd[1].fend, src->reg, src->ofst);



              //  if (dst) {dst->arg = 1; src->arg = 0;
                /* if (src->enc)
                    {
                      dst->enc = src->enc;   //src is encoded
                      dst->fend = 15;                   // enc is always a word
                      dst->data  = src->data;     // 'start of data' base reg
                      #ifdef XDBGX
                       DBGPRT(1,"src ENC R%x = R%x", c->opnd[1].reg, c->opnd[3].reg);
                      #endif
                    }  */

                 if (src->calc)
                    {
                      dst->calc = src->calc;   //src is encoded
                      dst->fend = 15;                   // enc is always a word
         //             dst->data  = src->data;     // 'start of data' base reg
                      #ifdef XDBGX
                       DBGPRT(1,"src CALC R%x = R%x", c->opnd[1].reg, c->opnd[3].reg);
                      #endif
                    }







                }
               }           //if dst

             #ifdef XDBGX
              if (src) DBGPRT(1,"RG %x = %x",  c->opnd[3].reg,  c->opnd[1].reg);
             #endif

           }
   //      else

   //     }
}


//  DBGPRT(1,"in add_regargs");








 // x = get_chain(CHPOP);

 // p = get_pop (s->start, c->opnd[1].reg);  //should be first reg in blk
 // ix = x->lastix;

 // p = (POP*) x->ptrs[ix];        //TEMP
//  if (p) DBGPRT(1," POP = %x (%x)", p->popreg, p->popblk);

//  if (p && p->popblk) DBGPRT(1,"popblk = %x %x", p->popblk->nextaddr, p->popblk->start);
 // else DBGPRT(1,"NO popblk");

// z = 0;
// while (ix < x->num)
//  {
//  z = (POP*) x->ptrs[ix];
//  if (z->blkstart != s->start) break;   //no match

  // check opcsub = 0 here first.
 //  if (p)            //z->srcreg == c->opnd[1].reg)
 //  {
   //    DBGPRT(0," POP FOUND for arg = %lx (%x)", z->popreg, z->popblk);
    //   if (z->popblk) DBGPRT(0," %x" , z->popblk->nextaddr);   //this seems to work....
   //    DBGPRT(1,0);
   //    p->modified = 1;

 // if (!c->opcsub)
 // {
  //    DBGPRT(1, "XZ replace src %x -> %x", c->opnd[3].addr, p->srcreg);
     // break;
 // }

 //  if (c->opcsub > 1)
 // {          // ldx or stx to indirect or indexed.

 //     DBGPRT(0, "qq add arg %x <- [%x]", c->opnd[3].addr, p->srcreg);
  //    if (p->popblk) DBGPRT(1, " to %x (%x)", p->popblk->nextaddr, c->opnd[1].addr); else DBGPRT(1,0);

   //  DBGPRT(1, "set CHECK for args");

      // [1]->addr is key !!! source address of this arg....
      // then check from s->curaddr to end of sub to check all op sizes ?? including any new subr calls.
      // maybe s->curaddr forwards (back out of stack) and handling subr calls
      //(emu start)
      //or just to first use after arg set up ? what about subrs called - to end of caller block ????

//but also may handle arg sizes AFTER proc call, so need to get to end of CALLER subr. (=current scan block)
//so that makes sizing probably best on the RET or exit of current (= top) block.


//must check here for A9L style 77be next op being stx ...

   //   do_one_opcode(s->nextaddr, &tmpscan, &sinst);        //or should do a sig ??

   //   if (sinst.opnd[1].reg == c->opnd[3].addr)  DBGPRT(1,"qq SWOP REG %x %x", c->opnd[3].addr, sinst.opnd[3].addr);



   //   append_arg(c->opnd[1].addr, c->opnd[3].addr);        //void *fid, uint reg)       add_arg fid is p->popblk->start NOT s->start.

      //and after this, check it's size ? or wait until end of emulate ??
//  }
//  break;
 // }

 //now check against register.

// ix++;
   // return chpoprg.lastix;   // lastix
//}


//p = find_pop(s->start);  //  vconvi(s->start));


/*so if opcsub = 0 then it's a swop of scource
// if opcsub > 1 it's an arg.
// also stx allows [3] to be indirect..... remember THIS -

77c2: cc,3c               pop   R3c              R3c = pop();                      # This subrs return address
77c4: b2,3d,3a            ldb   R3a,[R3c++]      R3a = [R3c++];                    # Get count of bytes, increment return address
77c7: cc,42               pop   R42              R42 = pop();                      # Get caller's return address
77c9: b2,43,3b            ldb   R3b,[R42++]      R3b = [R42++];       ** this is in multibanks too.
77cc: c6,17,3b            stb   R3b,[R16++]      [R16++] = R3b;
77cf: e0,3a,f7            djnz  R3a,77c9         R3a--;
                                                 if (R3a != 0) goto 77c9;          # Get no of bytes into destination address
77d2: c8,42               push  R42              push(R42);
77d4: c8,3c               push  R3c              push(R3c);                        # and push modified address returns back.
77d6: f0                  ret                    return;
*/
   //     {     // found popped source reg - check if popped or arg





SBK* stx (SBK *s, INST *c)
  {
   /* This is the only opcode to not use op [3] as destination op
    * and can WRITE to indexed and indirect addresses
    * does not change PSW
    * SWOP operands to match ldx so that ALL opcodes have [3] as destination.
    * i.e. [1] = [2], [3] = [1];
    * BUT must then print bare ops backwards
    */

  FSTK *t, *x;
  RST  *r;    //, *b;
  int val, inx;

  decode_mult_mode(s,c);

  //  make this look like ldx  [3] = [1];

  cpy_op(c,3,1);            // op3 = op1  (destination)
  cpy_op(c,1,2);            // op1 = op2  (source)

  rb_addchk (s,c);
  c->opnd[3].val  = c->opnd[1].val;

  // NB. op[3] may be indirect/indexed. ->addr should be correct.
  // this is handled in do_sym_names in print phase,

 // if (anlpass >= ANLPRT)  return s;
  if (!s->proctype)       return s;


// put rbase check in here ?

  // now check if this is a stack operation
  // if it is a stack register, treat similarly to a PUSH

  x = get_stack(s->proctype,0);


  if (valid_reg(c->opnd[3].reg) == 4 && (s->proctype & 4))             // was [1], but swopped now
    {
      inx = -1;

      if (c->opcsub == 2) inx = 0;                    // inr
      if (c->opcsub == 3) inx =  c->opnd[0].addr/2;    // inx

      if (inx >=0 && inx < STKSZ)
        {
         t = x+inx;
         val =  c->opnd[1].val |= g_bank(t->sblk->nextaddr);       // for stack checks
     //    t->newadd = val;     //update both scan and emu

   #ifdef XDBGX
         DBGPRT(1,"PUSH [%d] via E_STX R%x=%x over %x at %x", inx, c->opnd[1].reg, val, t->sblk->nextaddr, c->ofst);
   #endif
          do_args(c,s, t,inx);
          r = get_rgstat(c->opnd[1].reg);    //, s->substart);          // get_rgstat(c->opnd[1].reg);
          if (r) r->popped = 0;               // clear popped from sce reg

         if (t->ftype != 1)
          {
                 #ifdef XDBGX
           DBGPRT(0,"PSW INV PUSH!! %d ",t->ftype);
           DBG_stack(s->proctype);           //,x);
           #endif
           return s;
          }
        }
    }

  add_regargs(s,c);

check_R11(s,c);
return s;
 }





SBK* ldx(SBK *s, INST *c)
  {
   // does not change psw [3] = [1]

   int inx;
   RST *org;    //, *nrg;
   OPER *o;    //, *sce;
   FSTK *t, *x;


   decode_mult_mode(s,c);

   o = c->opnd+3;


//could stx jump here ??




   rb_addchk (s,c);
   o->val = c->opnd[1].val;
  // o->addr = c->opnd[1].addr;        // [3] = [1] as default op

   if (!s->proctype) return s;
 //  if (anlpass >= ANLPRT)  return;

   x = get_stack(s->proctype,0) ;


  // check if [4] =  stack register. If so this is a load from stack,
  // analogous to a POP, and will always be indirect or indexed

  if (valid_reg(c->opnd[1].reg) == 4)
    {
    #ifdef XDBGX
    DBG_stack(s->proctype);           //,x);
    #endif
      inx = -1;
      if (c->opcsub == 2) inx = 0;                     // inr
      if (c->opcsub == 3) inx =  c->opnd[0].addr/2;    // inx

      if (inx >=0 && inx < STKSZ)
        {
         t = x + inx;
         o->val = match_stack(c, s, t, 7);                   // allow anything

          // x = fakepop(s);             //gets SBK of caller
 // p =
 //append_pop(s, o->reg, t->sblk);      // POP* add_pop (uint reg, SBK * s, uint ofst)


         t->popreg = o->reg;
     //    t->popped = 1;                                // stack - mark pop

         org = add_rgstat(o->reg,0, s->curaddr);             //add_rgstat(o->reg, 15, 0, o->val);      //s->substart);          // find or add (word sized)
         org->popped = 1;                      // set popped rgstat
    //     org->inc = 0;


            #ifdef XDBGX
         DBGPRT(1,"%x POP via LDX R%x=%x [I%d] T%d", c->ofst, c->opnd[3].reg, c->opnd[3].val, inx, t->ftype);
         DBGPRT(1,"set popped, size = 2");
    #endif
         if (t->ftype == 1 && g_bank(o->val) != basepars.datbnk)
          {

           basepars.datbnk = g_bank(o->val);
           #ifdef XDBGX
            DBGPRT(1,"set dbnk %d", basepars.datbnk);
            #endif

            // set databank here for correct [Rx++] calls ? if t->type == 1
          }
        }
    }        //end emulate

  check_R11(s,c);

 // if (s->emulating)
  //  {
          // 2 = 1 for ldx (if not stack reg),
          // so get source register from 1 (or 4 if ind or inx)


//if (s->curaddr == 0x87aaa)
//{
//DBGPRT(1,0);
//}


     add_regargs(s,c);
     return s;
}
/*
      org = 0;
      sce = 0;
      if (!c->opcsub) sce = c->oper + 1;
      if (c->opcsub > 1) sce = c->oper + 4;

      if (sce) org = find_rgstat(sce->addr);

      if (org)
        {     // found source reg
          if (org->arg)
              // reg is flagged as arg, so use size
           {
             // really what we are interested in for now is ONLY the SIZE
             set_rgsize(org,o->fend);

             org = find_rgstat(org->sreg);
             set_rgsize(org,o->fend);

             // but now, arg value is copied to a new register/addr, which MAY be used.....
             #ifdef XDBGX
              if (org) DBGPRT(1,"LDX %x=%x to %x", sce->addr, sce->val, c->oper[c->wop].addr);
             #endif

           }
         else
         if (org->popped)
              // reg flagged as popped so make [wop] an arg - would expect this to be indirect or indexed
           {
            nrg = add_rgstat(o->addr, o->fend, c->oper[1].addr);          //org->ofst);     //s->substart);
            if (nrg)
             {
              nrg->arg = 1;
           //   nrg->sarg = 1;
              nrg->sreg  = org->reg;

                 #ifdef XDBGX
              DBGPRT(1,"ADD ARG LDX %x at %x sz%d", nrg->reg, s->start, nrg->fend);
              #endif
             }
           }
        }

  //    }

  }
*/



SBK* orx(SBK *s, INST *c)
 {

  decode_mult_mode(s,c);

  if (c->opcix < 32)    c->opnd[3].val |=  c->opnd[1].val;   // std or
  else  c->opnd[3].val ^=  c->opnd[1].val;                   // xor

   if (!s->proctype) return s;
//CHECK if mem-expand set with only one bank indicates something screwy
//if rbase sig found, then can try to find bank 1

 //  orb   Ra,10            MEM_Expand = 1;

if (c->opnd[3].addr == 0xa && c->opnd[1].val & 0x10)
 {        //Mem_expand bit set
    if (numbanks == 0)
    {
          #ifdef XDBGX
        DBGPRT(1,"%x MEM_EXPAND SET BUT SINGLE BANK !!!!", s->curaddr);
        #endif
        xprt(MSGFILE,1,"##        MEM_EXPAND SET BUT SINGLE BANK !!!!");

// find_data_bank();
        // find bank by rbase data..........

    }
 }









  if (s->proctype & 4)
     {
      pset_psw(s, 0, 0xa);      // clear bits V,C
      set_psw(s, c->opnd[3].val, c->opnd[3].fend, 0x30);     // N and Z
     }
     return s;
 }

SBK* addx(SBK *s, INST *c)
 {
      // 2 or 3 op

   decode_mult_mode(s,c);

   rb_addchk (s,c);
   c->opnd[3].val =  c->opnd[2].val + c->opnd[1].val;

   if (s->proctype & 4)
   set_psw(s, c->opnd[3].val, c->opnd[3].fend, 0x3e);
   return s;
 }


SBK* andx(SBK *s, INST *c)
 {
       // 2 or 3 op

  decode_mult_mode(s,c);
  c->opnd[3].val =  c->opnd[2].val & c->opnd[1].val;

 if (s->proctype & 4)  pset_psw(s, 0, 0xa);
 // set_psw(2, 0x30);
return s;
 }


SBK* sub(SBK *s, INST *c)
 {      // 2 or 3 op

    decode_mult_mode(s,c);

    c->opnd[3].val =  c->opnd[2].val - c->opnd[1].val;

 // sub is a BORROW not a carry
    if (s->proctype & 4)
      {
       set_psw(s, c->opnd[3].val, c->opnd[3].fend, 0x3e);
       s->psw ^= 2;       // complement of carry
      }
      return s;
 }


SBK* cmp(SBK *s, INST *c)
 {
   // same as sub , except [3] is not written.
   // sub is a BORROW not a carry

  decode_mult_mode(s,c);
  c->opnd[3].val =  c->opnd[2].val - c->opnd[1].val;

  if (s->proctype & 4)
    {

     set_psw(s, c->opnd[3].val, c->opnd[1].fend,0x3e);
     s->psw ^= 2;       // complement of carry
    }
    return s;
 }

SBK* mlx(SBK *s, INST *c)
 {     //psw not clear - assume no changes; 2 or 3 op

    decode_mult_mode(s,c);
    c->opnd[3].val =  c->opnd[2].val * c->opnd[1].val;  // 2 and 3 ops
 // PSW ???
return s;
 }

SBK* dvx(SBK *s, INST *c)
 {
   // quotient to low addr, and remainder to high addr
   // always 2 op ??
    long x;
    OPER *o;


    decode_mult_mode(s,c);

    o = c->opnd + 3;

    x = o->val;  // keep original
    if (c->opnd[1].val != 0) o->val /= c->opnd[1].val;    // if 2 ops (wop = 2)

    x -= (x * o->val);             // calc remainder

    if (o->fend > 7)              // long/word
        {
         o->val  &= 0xffff;
         o->val  |= (x << 16);       // remainder
        }
    else
       {          // word/byte
         o->val  &= 0xff;
         o->val  |= (x << 8);       // remainder
        }

// PSW ??
return s;
 }


void decode_shift_ops(SBK *s, INST *c)
{
    //all shifts are 2 op
    uint b;

    c->opcsub = 0;                          // not multimode
    b = g_byte(s->nextaddr+1);             // first data byte after (true) opcode
  //   set_opdata(s->nextaddr+1,7);
     op_calc (b, 1,c);                      // 5 lowest bits only from firstb, from hbook

     if (b < 16) op_imd(c);                 // convert op 1 to absolute (= immediate) shift value
     op_calc (g_byte(s->nextaddr+2),2,c);
     op_calc (g_byte(s->nextaddr+2),3,c);    //and calc destination

 //    set_opdata(s->nextaddr+2,7);
     s->nextaddr += c->numops + 1;
    }


SBK* shl(SBK *s, INST *c)
 {

   /* manual - The right bits of the result are filled with zeroes.
     The last bit shifted out is saved in the carry flag.
      * sticky appears to be last carry ?
   */

  long mask, v;

  decode_shift_ops(s,c);

  if (!s->proctype) return s;
  v = c->opnd[3].val;
  v <<= c->opnd[1].val;  // as a 64 long, for DW shifts

  if (s->proctype & 4)
    {
     set_psw(s, v, c->opnd[3].fend,0x30);               // Z and N
     mask =  get_sizemask(c->opnd[1].fend) + 1;         // get to 'size + 1' bit (= last shifted)
     if (v & mask) s->psw |= 2;  else s->psw &= 0x3d;   // set or clear carry flag
    }
  c->opnd[3].val =  v;
  return s;
 }

SBK* shr(SBK *s, INST *c)
 {
// arith shift vs uns ??

 /* manual  The last bit shifted out is saved in the carry flag.
   The sticky bit flag is set to indicate that, during a right shift, a “1” has been shifted into the
   carry flag and then shifted out.
*/

  decode_shift_ops(s,c);

   if (!s->proctype) return s;
 // s->psw &= 0x3b;         // clear sticky first

  c->opnd[3].val >>=  c->opnd[1].val;    // always 2

// if (!s->scanning) return;
  // and need to sort out carry and sticky here....
  return s;
 }

SBK* inc(SBK *s, INST *c)
 {

   decode_dflt_ops(s,c);
   // ALWAYS 1 (only autoinc is one or two)
   c->opnd[3].val++;

   if (s->proctype & 4)
     {
      set_psw(s, c->opnd[3].val, c->opnd[3].fend, 0x3e);
     }
return s;
 }

SBK* dec(SBK *s, INST *c)
 {

    decode_dflt_ops(s,c);
   // ignore for scans
      c->opnd[3].val--;
    if (s->proctype & 4)
     {
      set_psw(s, c->opnd[3].val, c->opnd[3].fend, 0x3e);
     }
     return s;
 }


int calc_jump_ops(SBK *s, int jofs, INST *c)
{
    // s->nextaddr must correctly point to next opcode.
    // returns jump OFFSET

     OPER *o, *x;
     CINST *z;

     c->opcsub = 0;                        // no multimode
     o = c->opnd + 1;                      // oper[1] is jump address
     x = c->opnd + 0;                      // oper[0] is offset value
     o->optype = OPADDR;                   // address mode
     o->fend = 0xf;                        // for symbol and size
     x->fend = 0xf;
     x->val = jofs;                        // for signature matches
     if (jofs < 0) x->neg = 1;             // for sigs
     x->optype = OPOFF;                    // treat like offset (indexed mode)
     o->addr = jofs + s->nextaddr;
     o->addr = codebank(o->addr, c);       // add CODE bank for jumps


// check overlap to another opcode
    z = get_nearcopcode(o->addr);        // returns c if addr overlaps an opcode/operand

// if (z)
 //  {
 //    DBGPRT(1, "DGDGDG jump overlap opcode %x  addr %x", z->ofst, o->addr);

// }


if (z) return 0;




     return 1;
  }


SBK* cjm(SBK *s, INST *c)
 {    // conditional jump, (short)
    // all types here based upon psw

   s->nextaddr += 2;
   calc_jump_ops(s,g_val(s->nextaddr-1,0,39),c);   //signed byte offset

  // set_opdata(s->nextaddr-1,7);

   if (s->proctype & 3)
    {
    do_sjsubr(s, c, J_COND);
    return s;
    }

if (s->proctype & 4 ) {

// do actual jump if emulating
switch (c->opcix)
   {

    case 54:            // JNST      STK sticky is 1
       if (!(s->psw & 1))   s->nextaddr = c->opnd[1].addr;
       break;

    case 55:            // JST
      if ((s->psw & 1))   s->nextaddr = c->opnd[1].addr;
       break;

    case 56:            // JNC       CY carry is 2
       if (!(s->psw & 2))   s->nextaddr = c->opnd[1].addr;
       break;

    case 57:            // JC
       if (s->psw & 2)   s->nextaddr = c->opnd[1].addr;
       break;

    case 58:            // JNV      OV overflow is 8
       if (!(s->psw & 8))   s->nextaddr = c->opnd[1].addr;
       break;

    case 59:            // JV
      if (s->psw & 8)   s->nextaddr = c->opnd[1].addr;
       break;

    case 60:            // JNVT  OVT overflow trap is 4
       if (!(s->psw & 4))   s->nextaddr = c->opnd[1].addr;
       break;

    case 61:            // JVT
       if (s->psw & 4)   s->nextaddr = c->opnd[1].addr;
       break;

    case 62:            // JGTU  C (2) set and Z (0x20) unset
       if ((s->psw & 0x22) == 2)   s->nextaddr = c->opnd[1].addr;
       break;

    case 63:            // JLEU  C unset OR Z set
       if ((s->psw & 0x22) != 2)   s->nextaddr = c->opnd[1].addr;
       break;

    case 64:            // JGT  N (16) unset and Z (32) unset
       if (!(s->psw & 0x30))   s->nextaddr = c->opnd[1].addr;
       break;

    case 65:            // JLE  N set OR Z set
      if (s->psw & 0x30)   s->nextaddr = c->opnd[1].addr;
       break;

    case 66:            // JGE  N not set N is 16
       if (!(s->psw & 16))   s->nextaddr = c->opnd[1].addr;
       break;

    case 67:            // JLT  N (negative) is set
      if (s->psw & 16)   s->nextaddr = c->opnd[1].addr;
       break;

    case 68:            // JE   (Z set)  Z is 32, zero flag
       if (s->psw & 32)   s->nextaddr = c->opnd[1].addr;
       break;

    case 69:            // JNE    (Z not set)
      if (!(s->psw & 32))   s->nextaddr = c->opnd[1].addr;
       break;

    default:
       break;
   }

#ifdef XDBGX
if (s->nextaddr == c->opnd[1].addr)      // debug confirm jump taken
  {
   DBGPRT(1,"cjumpE %x->%x (psw=%x)", c->ofst, c->opnd[1].addr, s->psw);
  }
#endif

}   // end of emulating
return s;
 }

SBK* bjmp (SBK *s, INST *c)
 {     // Bit Jump.  JB, JNB,
     //  put bit no in [2] address field
    int xofst;
    OPER *o;

    xofst = s->nextaddr;
    s->nextaddr += 3;

    o = c->opnd+2;
    op_calc (g_byte(xofst+1), 2,c);           // register in [2] reg
    o->addr = c->opcode & 7;                  // bit number in [2] addr
    o->optype = OPBIT;                         // and flag it
    calc_jump_ops(s, g_val(xofst+2,0,39),c);   // jump destination, signed byte
    if (s->proctype & 3)
      {
       do_sjsubr(s, c, J_COND);
       return s;
      }

  //  0 is LS bit .... jnb is 70, jb 71

if (s->proctype & 4)
    {

    if (check_backw(c->opnd[1].addr, s, J_STAT)) return s;

    xofst = (1 << o->addr);               // set mask
    // bit set and JB (71)
    if ( (o->val & xofst) && (c->opcix & 1)) s->nextaddr = c->opnd[1].addr;
    // bit not set and JNB (70)
    if (!(o->val & xofst) && !(c->opcix & 1)) s->nextaddr = c->opnd[1].addr;
   }
return s;
 }

SBK* sjm(SBK *s, INST *c)
 { //  a short jump
   //  CANNOT do this as part bit field as it is REVERSE of std word

    int jofs;

    jofs = sjmp_ofst(s->nextaddr);

 //   set_opdata(s->nextaddr+1,7);
    s->nextaddr += 2;

    calc_jump_ops(s, jofs,c);

    if (s->proctype & 3)
      {
       do_sjsubr(s, c, J_STAT);
       return s;
      }

if (s->proctype & 4)
    {
     if (check_backw(c->opnd[1].addr, s, J_STAT)) return s;
     s->nextaddr = c->opnd[1].addr;      // go direct in emu mode
    }
return s;
 }


SBK* ljm(SBK *s, INST *c)
 {      //long jump
   int jofs;

   s->nextaddr += 3;
   c->opsize++;
   jofs = g_val(s->nextaddr-2,0,47);        //signed word

//  jofs = g_val(s->nextaddr,0,47);        //signed word
//   s->nextaddr +=2;
//   c->opsize++;


   calc_jump_ops(s, jofs,c);
 //  set_opdata(s->nextaddr-2,47);
   if (s->proctype & 3)
     {
       do_sjsubr(s, c, J_STAT);
       return s;
     }

if (s->proctype & 4)
    {
 if (check_backw(c->opnd[1].addr, s, J_STAT)) return s;
    s->nextaddr = c->opnd[1].addr;      // go direct in emu mode
    }
    return s;
  }



SBK* cll(SBK *s, INST *c)
 {     // long call
    SBK *n;

    c->opsize++;          //extra byte for word offset

    s->nextaddr += 3;
    calc_jump_ops(s,g_val(s->nextaddr-2,0,47),c);         //signed word
 //   set_opdata(s->nextaddr-2,47);


 /*  if (s->lscan)
    {
      #ifdef XDBGX
       if (anlpass < ANLPRT) DBGPRT(1,"ignore call %x from %x", c->oper[1].addr, s->curaddr);
      #endif
      return;
    }
*/

    if (s->proctype & 2)
      {
   #ifdef XDBGX
   // if (anlpass < ANLPRT)
        DBGPRT(1,"call %x from %x", c->opnd[1].addr, s->curaddr);
    #endif
       do_sjsubr(s, c, J_SUB);
       return s;
      }

if (s->proctype & 4)
    {
       SUB *sub;
       sub = get_subr(c->opnd[1].addr);
       if (sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opnd[1].addr, sub->size);
           #endif
           s->nextaddr += sub->size;
           return s;
         }

     #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opnd[1].addr, s->curaddr);
     #endif

     n = add_escan(c->opnd[1].addr, s);
     fakepush(s, n, c->opnd[1].addr,1);            // std subr call, push return details onto stack

     scan_blk(n, &einst);                      // recursive

// break here ???  if args set ??

    }
    return s;
 }


SBK* scl(SBK *s, INST *c)
 { // short call
   int jofs;
   SBK *n;

   jofs = sjmp_ofst(s->nextaddr);

  // set_opdata(s->nextaddr+1,7);
   s->nextaddr+=2;
   calc_jump_ops(s,jofs,c);

 /*  if (s->lscan)
    {
      #ifdef XDBGX
       if (anlpass < ANLPRT) DBGPRT(1,"ignore call %x from %x", c->oper[1].addr, s->curaddr);
      #endif
      return;
    }
*/

   if (s->proctype & 3)
     {

   #ifdef XDBGX
//  if (anlpass < ANLPRT)
 DBGPRT(1,"call %x from %x", c->opnd[1].addr, s->curaddr);
  #endif

      do_sjsubr(s, c, J_SUB);
      return s;
     }

 if (s->proctype & 4)
    {
       SUB *sub;
       sub = get_subr(c->opnd[1].addr);
       if (sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opnd[1].addr, sub->size);
           #endif
           s->nextaddr += sub->size;
           return s;
         }




   #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opnd[1].addr, s->curaddr);
     #endif



     n = add_escan(c->opnd[1].addr, s);
     fakepush(s, n, c->opnd[1].addr,1);              // std subr call, push return details onto stack

     scan_blk(n, &einst);                          // recursive (as true core)
    }
return s;
 }


SBK* ret(SBK *s, INST *c)
 {           // return

   s->nextaddr++;
   calc_jump_ops(s, -1,c);      // do as loopstop jump


   if (s->proctype & 3)
    {
     if (c->save) add_opcode(c);              // save opcode before new scan
     basepars.rambnk = 0;         // safety
     do_sjsubr(s, c, J_RET);
     return s;
    }

    if (s->proctype & 4)
    {
     fakepop(s);                     // check & drop the stack call
    }

   #ifdef XDBGX
   if (s->proctype)           // scanning || s->emulating)
   {
     DBGPRT(0,"%x RET", s->curaddr);
     if (s->proctype & 4) DBGPRT(0," E");
     DBGPRT(2,0);

   }
   #endif


// always end this block
   end_scan(s);

// }
return s;
}



SBK* djm(SBK *s, INST *c)
 { // djnz
   int xofst;

   xofst = s->nextaddr;
   s->nextaddr += 3;
   op_calc (g_byte(xofst+1), 2,c);              // for first operand print
   op_calc (g_byte(xofst+1), 3,c);              // for calcs as [3] = write

   calc_jump_ops(s,g_val(xofst+2,0,39),c);      //signed byte

   if (s->proctype & 3)
     {
      do_sjsubr(s, c, J_COND);
      return s;
     }

   if (s->proctype & 4)
     {

    //   RST *r;
       OPER *o;
       o = c->opnd+3;
       o->val--;
       if (o->val > 0) s->nextaddr = c->opnd[1].addr;
       else
        {
  //       r = get_rgstat(o->addr, s->start);  // get_rgstat(o->addr);
         //if (r)
            // {
          //      chdelete(CHRGST, get_lastix(CHRGST), 1);      //&chrgst,chrgst.lastix,0);    // delete this - loop variable
                 #ifdef XDBGX
                      DBGPRT(1,"clear rgstat %x", o->addr);
                      #endif
//}
        }
     }
     return s;
 }

SBK* skj(SBK *s, INST *c)
 {  // skip, = jump(pc+2) as static jump
    s->nextaddr++;

    calc_jump_ops(s,1,c);

    if (s->proctype & 3)
      {
          // as this is a static jump, do next opcode....
          //already checked for invalids (in do-code)
           do_one_opcode(s->nextaddr, &tmpscan, &sinst);        //or should do a sig ??
           if (!tmpscan.inv)
             {
               add_opcode(&sinst);              // save opcode if not done yet
               s->nextaddr++;                   // and add it to scan
             }

       do_sjsubr(s, c, J_STAT);

       return s;
      }

  if (s->proctype & 4)
   {
      s->nextaddr++;      // skip next byte
   }
   return s;
 }




SBK* pshp(SBK *s, INST *c)
 {
     // push flags (i.e. psw)

   s->nextaddr++;

    if (!s->proctype) return s;
   s->pushp = 1;

     if (numbanks)
    {
      fakepush(s, 0, s->psw, 4);
         #ifdef XDBGX
//      if (anlpass < ANLPRT)
 DBGPRT(0,"%x PUSHP ", s->curaddr);
  //    DBG_stack(s->emulating);
      #endif
    }
return s;
 }

SBK* popp(SBK *s, INST *c)
 {
     // pop flags (i.e. psw)
     // ignore in scan

   FSTK *x;

   s->nextaddr++;

   if (!s->proctype) return s;

   s->pushp = 0;

   if (s->proctype & 4)         //emulating)
      {
        x = get_stack(s->proctype,0);

       if (numbanks)
      {
       match_stack(c, s, x, 4);       // for error report
       fakepop(s);
      }
}

return s;
 }

SBK* nrm(SBK *s, INST *c)
 {
      decode_dflt_ops(s,c);

//copy 3,1         to get 1 into 3
// 3 shifted up
// 2 is no of shifts



      // from Ford hbook shift left double until b31 = 1   (op 1)
      // and result size is double too....as it's shifted...
      //no of shifts go into [2] which is byte

      // TO DO
      return s;
 }


SBK* sex(SBK *s, INST *c)
 {
     // sign extend - not necessary in this setup ?

      decode_dflt_ops(s,c);
      return s;
 }


SBK* clc(SBK *s, INST *c)
 {
     // clear carry
   s->psw &= 0x3d;
   s->nextaddr++;
   return s;
 }

SBK* stc(SBK *s, INST *c)
 {
     // set carry
   s->psw |= 2;
   s->nextaddr++;
   return s;
 }

SBK* die(SBK *s, INST *c)
 {
     // enable & disable ints

    s->nextaddr++;
    return s;
 }

SBK* clv(SBK *s, INST *c)
 {   // clear trap
  s->psw &= 0x3b;
  s->nextaddr++;
  return s;
 }

SBK* nop(SBK *s, INST *c)
 {
      s->nextaddr++;
      return s;
 }

SBK* bka(SBK *s, INST *c)
 {          // RAM bank swopper 8065 only

   int bk;
   //   According to Ford book, this only sets PSW, so could be cancelled by a POPP
   //   or PUSHP/POPP save/restore,  so may need a PREV_BANK


   s->nextaddr++;


   bk = c->opcix - 107;                          // new RAM bank
   op_imd(c);                                    // mark operand 1 as immediate
   c->opnd[1].addr = bk;
   c->opnd[1].fend = 7;                           // byte sized;
   c->numops = 1;                                // fake a single byte [imd] op

   if (!s->proctype) return s;
   basepars.rambnk = bk * 0x100;

   #ifdef XDBGX
    DBGPRT(1,"New Rambank = %x at %x", bk, s->curaddr);
   #endif
 return s;
 }










// ---------------------------------------------------------------------------------------------









/*

void set_data_vect(LBK *blk)
{
 int ofst, xofst, end, pc;
 int bank;
 ADT *a;

 // for structs only
 if (PMANL)  return;
 if (!blk || blk->fcom != C_STCT) return;

 bank = g_bank(blk->start);      // what if bank is NOT same as struct

 for (ofst = blk->start; ofst < blk->end; ofst += blk->size)
 {
  a = start_adnl_loop(&chadnl, blk->start); //(ADT*) chmem(&chadnl,0);
 // a->fid = blk;

  end = ofst + blk->size-1;
  xofst = ofst;
  while ((a = get_next_adnl(&chadnl,a)) && xofst < end)
    {
      if (a->vaddr)
      {
       pc = g_word(xofst);
       pc |= bank;                  // this assumes vect is always in same bank
       add_scan (pc, J_SUB, 0);      // auto adds subr
      }
     xofst += cellsize(a);        // always 2 No........
    }
  }
}



void add_ans(uint start, ADT *ca)
{  //special adt copy for subr answers
 SPF *f;

// insert ino answer chain
  f = add_spf(start);        // add new one

  f->spf = 1;             // answer
  f->addrreg = c->ansreg;

 // *a = *ca;
 // a->fkey = set_adt_fkey(start,1);

    #ifdef XDBGX
    DBGPRT(0,"      with answer params");
 //   prt_adt(&chans, start, 0, DBGPRT);
    DBGPRT(1,0);
  #endif

}


void cpy_adt(CPS *c, uint start)
{
 // move adt blocks from cmd to new LBK
 // NB. create NEW ones as fid will be in worng place otherwise in adt chain

//  int i;
  uint seq;
  ADT *a, *cp;
  if (!start) return;                      // must have an fid
  if (!c->seq) return;                   // no addnl blocks

  a = start_adnl_loop(&chadcm, 0);
 // a->fid = c->fid;                  //command adnl entries

  while ((a = get_next_adnl(&chadcm,a)))
    {
 //     if (a->seq > c->seq) break; don't need it, chain cleared each command
    //  if (a->ans)
   //    {
   //     add_ans(start,a);        // add answer (in different chain)
   //    }
    //  else

      if (a->cnt)
       {          // use cnt as null checker
        if (c->fcom == C_ARGS || c->fcom == C_SUBR) a->pfw = 0;    // clear all pfw
        cp = add_adt(&chadnl,start,256);      // add new one with correct fid and seq
        seq = cp->fkey;       // save key
        *cp = *a;             //copy block
        cp->fkey = seq;       // restore key
        //set decimal for tabs and funcs
        if ((c->fcom == C_TABLE || c->fcom == C_FUNC) && cp->prdx == 0) cp->prdx = 2;
       }
    }

    #ifdef XDBGX
    DBGPRT(0,"      with addnl params");
    prt_adt(&chadnl,start, 0, DBGPRT);
    DBGPRT(1,0);
  #endif

} */




// amend this for everything - change end if relevant
// to command


int find_text(BANK *b)
{
  // check for text in end filler block
  // text blocks have either "Copyright at ff63" or ".HEX* ff06"  in them...
  // check address above ff00 for copyright and then check blocks....
  // works for now, more or less

// need more than this........................

  // BUT 0FAB has NO COPYRIGHT STRING !!!

  uint ans, val, start, ofst, bk, cnt;

  if (nobank(b->maxromadd) < 0xdfff)  return 0;          // not full bank

  if (strncmp("Copyright", (char *) b->fbuf + 0xff63, 9)) return 0;  // not found

  // matched Copyright string
  bk = g_bank(b->minromadd);

  start = (0xff63 | bk);
  ofst = start;

  ans = 0;
  val = g_byte(ofst);

  while (!val || (val >=0x20 && val <= 0x7f))
   {
      ofst++;
      val = g_byte(ofst);
   }
 // b->maxromadd = start-1;
  add_cmd (start,ofst, C_TEXT |C_SYS);
  add_cmd (ofst+1,b->maxromadd, C_DFLT|C_SYS);    // treat as cmd (cannot override)

  ans = start - 1;

  ofst = (0xff06 | bk);
  cnt = 0;
  val = g_byte(ofst);

  while (ofst < (0xff63 | bk))
   {     // allow a few non printers...6 ?
     if (val < 0x20 || val > 0x7f)
      {
        if (cnt > 5){ add_cmd (ofst-cnt,ofst-1, C_TEXT |C_SYS);
    //     if ((ofst-cnt) < b->maxrpc) b->maxpc = ofst-cnt;
        }
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












void find_fill_txt(BANK *b)
  {

// look for filler at end of bank.

   uint ofst, last;
   int cnt, val;

   last = b->maxromadd;
 //  b->maxpc = b->maxbk;             // default and safety

   ofst = find_text(b);             // any text found ?

   if (!ofst) ofst = b->maxromadd; else last = ofst;

   cnt = 0;
   for (; ofst > b->minromadd; ofst--)
     {
      val = g_byte(ofst);

      if (val != 0xff)
         {
          cnt++;                  // count non 0xff
         }
      if (cnt > 3) break;         // 2 non 0xff and no text match
     }


   ofst += 4;                     // 2 count plus one - may need more for a terminator byte ?

   if (ofst < b->maxromadd)
      {
        // fill at end
       add_cmd (ofst,last, C_DFLT|C_SYS);
       add_aux_cmd (ofst,b->maxromadd,C_XCODE|C_SYS);       // and xcode to match
  //     if (ofst < b->maxpc) b->maxpc = ofst;           //no, allows writes.
      }
  }


void mkbanks(int num, ...)
{
 va_list ixs;
 int i, j, k;
 BANK *b;

 va_start(ixs,num);

 j = 0;
 for (i = 3; i < 7; i++)
    {
     k = va_arg(ixs,int);
     b = bankmap + i;
     b->dbank = k;
//     b->bprt = 1;  //print in msg file
     j++;

     # ifdef XDBGX
       DBGPRT(1,"Set temp bank %d to %d",i, k);
     #endif

     if (j >= num) break;
    }
 va_end(ixs);

}








void scan_all (void)
{
  // check whole scan chain for unscanned items
  // a scan may add new entries so track lowest added scan block
  SBK *s, *saved;
  CHAIN *x;
  uint ix;


  x = get_chain(CHSCAN);

  x->lowins = x->num;                       // reset to highest value for next scan
  ix = 0;

  while (ix < x->num)
   {
    s = (SBK*) x->ptrs[ix];


       if (!s->stop)       // && !s->caller)            //experiment..NO........
         {
   //       if (anlpass >= ANLPRT) newl(1);            // new line for new block print
          saved = s->caller;
          s->caller = NULL;                          // set caller zero as safety for emulate
          recurse = 0;                               // reset recurse level
          #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(0,"Chain Scan");
          #endif
          xscans++;
          scan_blk(s, &cinst);
          s->chscan = 1;
          s->caller = saved;                          // and restore caller

          if (tscans > x->num * 32)               // loop safety check
           {
            xprt(MSGFILE,2,"*********");
            xprt(MSGFILE,1," Abandon - too many scans - a loop somewhere ?");
            xprt(MSGFILE,1,"*********");
            printf("FAILED - too many scan loops\n");
            break;
           }

        }         // end if not scanned

      if (x->lowins <= ix)  ix = x->lowins;            // recheck from lowest new scan inserted
      else  ix++;
      x->lowins = x->num;                       // reset to highest value for next scan
   }

 }





/*******************command stuff ***************************************************/





short init_structs(void)
 {

  // memset(bkmap,0,sizeof(bkmap));
 //  memset(fldata,0,sizeof(fldata));       //may be redundant...


  anlpass  = 0;                              // current pass number

 // gcol     = 0;
 // glastpad  = 0;
//  cmdopts  = 0;

  tscans    = 0;
  xscans    = 0;
  basepars.rambnk    = 0;

//  rgstat = (RST*) cmem(0,0, 0x400* sizeof(RST));

  return 0;
}







uint check_ihandler(uint taddr)
 {
   // check pointer values (int handler) for valid jump, return etc
   // return 1 for valid 0 otherwise
  int val;
  uint jp, cx;
  INST *c;

// allow taddr == d000 to d010 as a special ??? different count ??

  if (!val_rom_addr(taddr)) return 0;

  val = g_byte(taddr);

  // allow up to 7 nops

  jp = taddr+7;
  while (taddr < jp)
    {
     if (val == 0xff) val = g_byte(++taddr); else break;
    }

// ret, retei or pushp
  if (val >= 0xf0 && val <= 0xf2) return 1;

  do_one_opcode(taddr, &tmpscan, &sinst);    //need this as won't be scanned yet

  c = &sinst;

  if (c->sigix == 16 || c->sigix == 23)
    {    // jump, jnb
     jp = sinst.opnd[1].addr;
     cx = g_bank(jp);
     if (cx != g_bank(taddr))
        {    // bank swop
          cx-= 0x10000;
          if (!(cx & 0xf6))
           {
             return 1;       // can't go any further at this point, may be need unverified count ??
           }
        }

     taddr = jp;             //jump destination
     if (val_rom_addr(taddr))
       {
        val = g_byte(taddr);  //not bank swop, get value
        if (val >= 0xf0 && val <= 0xf2) return 1;    // ret, retei or pushp
       }

    }

  taddr = tmpscan.nextaddr;        //try after one unmatched opcode
  //check valid address ?
  val = g_byte(taddr);             // get value
  if (val >= 0xf0 && val <= 0xf2) return 1;    // ret, retei or pushp

return 0;
 }










void check_ihandler(SIG *s, uint taddr, uint ix)
 {
   // check pointer values (int handler) for valid jump, return etc
   // ix = 7 for 8065, 5 for 8061
  int val;
  uint bank;
  uint jp, cx;
  INST *c;

bank = g_bank(taddr);
// allow taddr == d004 as a special ???

  val = g_byte(taddr);

  // allow up to 7 nops, check for bank overflow
  jp = taddr+7;

  while (taddr < jp)
    {
     if (val == 0xff) val = g_byte(++taddr); else break;
      if (g_bank(taddr) != bank) break;


    }

  if (g_bank(taddr) != bank) return;

  if (val >= 0xf0 && val <= 0xf2)
    {  // ret, retei or pushp
     s->vx[ix]++;
     return;
    }

//need this as probably not scanned yet.
  do_one_opcode(taddr, &tmpscan, &sinst);


  c = &sinst;

  if (c->sigix == 16 || c->sigix == 23)
    {    // jump, jnb
     jp = sinst.opnd[1].addr;
     cx = g_bank(jp);
     if (cx != g_bank(taddr))
        {    // bank swop
          cx-= 0x10000;
          if (!(cx & 0xf6))
           {
            s->vx[ix]++;   // bank OK  (0,1,8,9)
            return;       // can't go any further
           }
        }

     taddr = jp;
     if (val_rom_addr(taddr))
       {
        val = g_byte(taddr);  //not bank swop, get value
        if (val >= 0xf0 && val <= 0xf2) {s->vx[ix]++; return;}    // ret, retei or pushp
       }

    }

  taddr = tmpscan.nextaddr;        //try after one unmatchd opcode
  val = g_byte(taddr);             // get value
  if (val >= 0xf0 && val <= 0xf2) {s->vx[ix]++; return;}    // ret, retei or pushp

 }



void check_bank_intvects (BNKF *b)
 {
    uint bank, x, taddr;

    if (!b) return;
    taddr = 0;
    if (!val_rom_addr(b->jdest)) return;

    bank = g_bank(b->pcstart);               // FAKE bank from sig

    // sig below int vects and a valid jump
    taddr = nobank(b->jdest);

    if (taddr >= 0x2010 && taddr < 0x2020) return; // invalid always.

    // check interrupt vects and handlers 8065

    set_cmdopt(OPT8065,1);    //      cmdopts |= 8;              // set 8065 to allow bank swops

    for (x = (bank|0x2010);  x < (bank|0x205f);  x+=2)
      {
       taddr = g_word(x);             // vect pointer
       if (taddr < 0x2060) break;     // not valid

       b->vc65++;                    // 8065 count (valid address)
       b->ih65 += check_ihandler(taddr|bank);
      }

    set_cmdopt(OPT8065,0);   //    cmdopts = 0;                                    // clear 8065


if (b->ih65 < 10)
{     // check interrupt vects 8061
    b->vc65 = 0;
    b->ih65 = 0; //clear 8065 counts


    for (x = (bank|0x2010);  x < (bank|0x201f);  x+=2)
      {
       taddr = g_word(x);             //pointer

       if (taddr < 0x2020) break;
       b->vc61++;         // valid adress
       b->ih61 += check_ihandler(taddr|bank);           //s, taddr|bank, 5);
      }
}

  if (b->jdest <= b->pcstart+b->skip) b->lpstp = 1;                    // loopstop jump (not code start)

  if (b->vc65 > 30 && nobank(b->jdest) > 0x205f) b->cbnk = 1;       // valid jump over int vects - 8065.
  else
  if (b->vc61 > 7  && nobank(b->jdest) > 0x201f) b->cbnk = 1;       // valid jump over int vects - 80611.


}



uint skip_ff(uint addr)
{  //skip leading oxff or oxfa for finding bank
    int c, v;
    c = 0;


//need more than just ff and fa ??? f8 (jsa)

    while (c < 16)
      {
       v = g_byte(addr);
       if (v != 0xff && v != 0xfa) break;
       c++;
       addr++;
      }
return c;
}


void find_banks (uchar *fb)
{
    // file is read in to ibuf [0x2000] onwards - look for start jump and interrupt vectors
    // assemble temp sigs into chain and then check interupt vectors.  scan around each address
    // for missing or extra bytes

    BANK *b, *l;
    uint addr, bank, ix, skp;
    CHAIN *ch;
    INST *c;
    BNKF  blk, *z, *x1, *x5;
    HDATA *fh;


    int fofs;              //file offset

    // temp bank setups so accesses work

    fh = get_flhdr();

    bank = 0x30000;      //bank 3, temp
    b = bankmap+3;
    b->maxromadd = (bank | 0xffff);
    b->minromadd = (bank | PCORG);
    b->bok = 1;                               // temp mark as OK
    b->usrcmd = 0;
    b->cbnk = 0;

    #ifdef XDBGX
       DBGPRT(1,"scan file for banks");
    #endif

    // check full file in loop for ALL banks

  //  mx1 = 0;
 //   mx5 = 0;
    numbanks = -1;

//is every 1k often enough ???

    for (addr = 0; addr < fh->fillen;  addr += 0x1000)
      {
       // keep all matches and then review best candidates
       // bank->fbuf MUST point to 0x2000 whether real or virtual
       // for missing bytes - mapaddr fixed, PC changes, 0x2002, 0x2001
       // for extra bytes   - mapaddr changes +0 to +3, PC set at 0x2000

       b->filstrt = addr;                       // move file offset
       b->filend  = addr + 0xe000-1;            // max possible end point (64k - 2k)

       if (b->filend >= fh->fillen)
         {        // make range safe for file end (adjust end downwards)
          b->filend = fh->fillen-1;                         // end of file (no bank)
          b->maxromadd = b->filend - b->filstrt + (PCORG | bank);
         }

       //allow for missing and extra bytes....
       // if bytes are missing, cannot just map backwards, must adjust start address,
       //but if extra bytes, can move buffer up and start at 2000

       for (fofs = -2; fofs < 3; fofs++)   // 2 bytes each way around 'address'
         {
           b->fbuf = fb + addr + fofs - 0x2000;     // overlay buff onto addresses  (minus 0x2000 offset)

        //     if (fofs < 0) b->fbuf = fb + addr - 0x2002;
      //     else   b->fbuf = fb + addr - 0x2000;     // start addr fixed 0x2000, file start moves

           ix = 0x2000|bank;

           if (fofs < 0)
             {
              ix -= fofs;                  // move pcstart up to match so still starts at file offset zero
             }

          skp = skip_ff(ix);        // skips would only be FF and FA, others make NO SENSE max of 15

          ix += skp;
          do_one_opcode(ix, &tmpscan, &sinst);     //not scanned yet

          c = &sinst;

          if (c->sigix == 16 && c->opcode)
           {  // jump, long or short, but not skip

             //put in local first to keep candidates low
             memset(&blk,0,sizeof(BNKF));

             // don't allow first file address to be negative

             if (addr < 16 && fofs < 0) blk.filestart = 0;
             else           blk.filestart = addr+fofs;

             blk.pcstart   = ix-skp;
             blk.jdest     = c->opnd[1].addr;
             blk.skip      = skp;
             check_bank_intvects (&blk);

             if (blk.ih61 || blk.ih65)
             {
              z = add_bnkf(addr+fofs,ix-skp);
              if (z)
               {
                 *z = blk;       //     copy counts etc.
               }
             }
           }

           /* else      //no, need sparate subr ??
           {
              check_rbase(ix)....
             // check for data bank, via rbase links (e.g FM20M06)
             uint val1,val2;
             ix -= skp;            //put ix back to start
             val1 = g_word(ix);
             if (val1 =
              *
            *
           }
            */

         }
    }                //may move this........... ???

  // look for highest valid counts in address groups

  ch = get_chain(CHBKF);

if (!ch->num)
{
   #ifdef XDBGX
        DBGPRT(1,"NO banks found !!");
        #endif
return;
}
  z = (BNKF*) ch->ptrs[0];
  x1 = z;                         // 8061 highest count
  x5 = z;                         // 8065 highest match

  bank = 3;                             // (fake) start bank

// NB. add one to max count and break, to get assign for last bank.

  for (ix = 1; ix < ch->num+1; ix++)
    {
      z = (BNKF*) ch->ptrs[ix];
      if (x1->ih61 > 6)  x1->tbnk = bank;     // current group match (if qualifies)
      if (x5->ih65 > 30) x5->tbnk = bank;

      if (ix > ch->num) break;                //with above +1, so that last bank gets allocated

      // interrupt vect cnt MUST be all OK otherwise not correct binary
      // check address ranges in s[15] so no interbank overlaps

      if ((z->filestart - x5->filestart) < 10 ) // same file address batch
       {
         if (z->ih65 == 40 && z->ih65 > x5->ih65)
           {           // better 8065 count
             z->tbnk  = bank;           // new match
             x5->tbnk = 0;             // kill old match
             x5 = z;                   // keep new 'max'

           }

         if (z->ih61 == 8 && z->ih65 < 10)     // 8061
           {
              if (z->ih61 > x1->ih61)
                {
                 z->tbnk = bank;              // new match
                 x1->tbnk = 0;                // kill old match
                 x1 = z;
                }
           }
       }
      else
       {      //new batch of addresses
         if (x1->tbnk || x5->tbnk) bank++;     // next bank
         x1 = (BNKF*) ch->ptrs[ix];            // next batch
         x5 = x1;
        }
     }

// done, now sort out temp banks.
// clear all bank OK markers

 for (ix = 3; ix < 7;  ix++)
    {
     b = bankmap + ix;
     b->bok = 0;
    }

// run up bank find list and assign banks...

  for (ix = 0; ix < ch->num; ix++)
    {
     z = (BNKF*) ch->ptrs[ix];
     if (z->tbnk)
       {        //bank set (temp)
        bank = z->tbnk;               //reset bank.............
        b = bankmap + bank;

        if (z->ih65 == 40) b->P65 = 1; else  b->P65 = 0;     // mark bank as 8065
        b->bok = 1;                                          // bank is valid
        numbanks++;
        if (z->cbnk) b->cbnk = 1;                            // code bank (8)
        b->filstrt = z->filestart;                           // actual bank start as buffer offset for 0x2000
        b->filend = b->filstrt + 0xe000-1;                   // default (max) end

        // check beyond end of file
        if (b->filend >= fh->fillen) b->filend = fh->fillen-1;

       // short bank check .............
       l = bankmap + (bank-1);                      // last bank
       if (bank > 3 && b->filstrt < l->filend)
         {   // shorten previous bank if it overlaps start.

          l->filend = b->filstrt-1;
          l->maxromadd = l->filend - l->filstrt + l->minromadd;
          l->maxromadd |= g_bank(l->minromadd);

          #ifdef XDBGX
            DBGPRT(1,"** Short bank %d, %x-%x", bank-1, l->filstrt, l->filend);
          #endif
         }


        b->fbuf = fb + b->filstrt-0x2000;            // set correct fbuf
        b->minromadd = z->pcstart | (bank << 16);                  // 2000, 2001 or 2002
        b->maxromadd = b->filend - b->filstrt + b->minromadd;

        #ifdef XDBGX
        DBGPRT(1,"bank found at %5x, PC %x", b->filstrt, z->pcstart);
        #endif

        //print warning if not on a 000 boundary
        fofs = (b->filstrt & 3);

        if (fofs)
           {
              #ifdef XDBGX
              DBGPRT(1," WARNING - file has extra %d byte(s) at front of bank ", fofs);
              #endif
               xprt(MSGFILE,2," WARNING - file has extra %d byte(s) at front of bank",  fofs);
           }

        fofs = nobank(b->minromadd) - PCORG;

        if (fofs)
           {
              #ifdef XDBGX
              DBGPRT(1," WARNING - file has %d byte(s) missing at front of bank", fofs);
              #endif
               xprt(MSGFILE,2," WARNING - file has %d byte(s) missing at front of bank", fofs);
           }

      }
    }

   #ifdef XDBGX

   DBGPRT(2,"- - - - - bank finds - - - - -");

   ch = get_chain(CHBKF);
   for (ix = 0; ix < ch->num; ix++)
   {
     BNKF *c;         // only for debug

  c = (BNKF*) ch->ptrs[ix];

  if (c->filestart < 0)DBGPRT(0,"   -%x", -c->filestart); else DBGPRT(0,"%5x", c->filestart);
  DBGPRT(0," %5x  %5x", c->pcstart, c->jdest);
  DBGPRT(0," %2d (61 %2d %2d), (65 %2d %2d)", c->skip, c->vc61, c->ih61, c->vc65, c->ih65);
  DBGPRT(0," lp %2d, cd %2d tb %2d", c->lpstp, c->cbnk, c->tbnk);
  DBGPRT(1,0);


     // ch->chprt(ch, 1, ix, DBGPRT);
   }
   DBGPRT(2,0);

#endif
}






void clrbkbit(int ix, int bk)
{
  BANK *b;

  b = bankmap+ix;

  switch (bk)
  {
    case 0:

      #ifdef XDBGX
      if (b->bkmask & 1)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
      #endif
      b->bkmask &= 0xe;      //clear bit 0
      break;

    case 1:

      #ifdef XDBGX
       if (b->bkmask & 2)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
       #endif
      b->bkmask &= 0xd;      //clear bit 1
      break;

   case 8:

      #ifdef XDBGX
       if (b->bkmask & 4)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
       #endif
      b->bkmask &= 0xb;      //clear bit 2
      break;

    case 9:

      #ifdef XDBGX
       if (b->bkmask & 8)  DBGPRT(1, "bank %d cannot be %d", ix, bk);
       #endif
      b->bkmask &= 7;        //clear bit 3
      break;

    default:
      #ifdef XDBGX
      DBGPRT(0,"unknown bank %d", bk);
      #endif
      break;
  }
}

void copy_banks(uchar * fbuf)
 {
  int i, bk, size;
  BANK *to, *from;

// copy to bank+1  for bankfix with sig names

  for (i = 3; i < 7; i++)
   {
     from = bankmap + i;            // temp bank
     if (from->bok)
      {        // valid to transfer
       from->dbank++;
       to   = bankmap + from->dbank;        // new bank
       bk =  from->dbank << 16;

       *to = *from;             // lazy, may change this
       size = to->filend - to->filstrt+1;                          // what if minus filstart ????

       to->fbuf = (uchar*) mem(0,0, 0x10000);                      // malloc a full bank 0-0xffff.

       memset(to->fbuf, 0, 0x2000);           //clear first 0x2000 ??  may be good idea...

       memcpy(to->fbuf+nobank(from->minromadd), from->fbuf+0x2000, size);
       to->minromadd &= 0xffff;
       to->minromadd |= bk;    // substitute bank no

       to->maxromadd &= 0xffff;
       to->maxromadd |= bk;
       to->bok = 1;
       to->bprt = 1;         // print this bank
       find_fill_txt(to);
       from->bok = 0;        // clear old bank
       from->bprt = 0;       // don't print
      }

   }

  basepars.codbnk = 0x90000;                         // code start always bank 8
  basepars.rambnk = 0;                               // start at zero for RAM locations
  if (numbanks) basepars.datbnk = 0x20000;           // data bank default to 1 for multibanks
  else   basepars.datbnk = 0x90000;                  // databank 8 for single





 }

void drop_banks_via_ints (int bk)
 {
    // check outwards from here for other banks
    // from int vect pointers via jumps and bank prefixes.
    // numbanks is 4, 2 to 6 of bkmap valid, the temp slots
    // but real bank nos (to be) assigned to them
    // NB. some bins don't have cross bank int jumps (e.g. 22CA)


    uint tbank, taddr, x;
    int  val, rbank;

    bankmap[bk].bkmask = 0xb;         // 9, 1, 0  not 8
    bankmap[bk].bok = 1;

    tbank = bk << 16;                 // TEMP bank no (so it maps OK)

    for (x = (tbank|0x2010);  x < (tbank|0x205f);  x+=2)
      {
       taddr = g_word(x);             // pointer
       taddr |= tbank;                 // make true address
       val = g_byte(taddr);
       // for 8065, can be a bank swop.  Check next byte is 0,1,8,9 with mask (and THEN a jump ??)

       if (val == 0x10)
         {           // bank swop prefix.
          taddr++;
          rbank = g_byte(taddr);            // get true bank (from code)
          // assume this bank CANNOT be rbank.............
          clrbkbit(bk,rbank);

          taddr++;
          val = g_byte(taddr);             // get opcode after bank prefix

  /*        if (val == 0xe7 || ( val >=0x20 && val <= 0x27))
           {
            // a jump - calc destination
           taddr = do_jumpsig (opcind[val], taddr,0x100000);
           // now try to find a valid opcode in one of the other banks.
           // have a guessed order at this point

          / for (i = 2; i < 6; i++)
             {
               taddr = nobank(taddr);
               taddr |= (i << 16);
               val = g_byte(taddr);
               if (val >= 0xf0 && val <= 0xf2)
                  {
                   if (getflagbnk(i) != rbank)
                    {       // not match !!
                     #ifdef XDBGX
                        DBGPRT(1,"Change (%d) from %d to %d",i, getflagbnk(i),rbank);
                     #endif
                     setflgbnk(i,rbank);
                    }

                #ifdef XDBGX
                else
                {
                 DBGPRT(1,"confirm bank (%d) %d via intr jump",i, rbank);
                }
                #endif

               }
             }
           } */
         }    // end bank check 8065 loop
      }

// now check for an allocated bank after shuffle....



 }

int chk_bank_order (int cbk)
 {
    // cbk = bank location - so check outwards for other banks
    // from int vect pointers

/* cannot change bank 8 !!
   GCM2 is fked up by this code ....
  must check if there IS a clash first....
 */

// 22CA all point to bank 8 !!

  int i, x, ans;
  BANK *b;

  ans = 1;
  // first use interrupt handlers to identify any bank jumps
  // and know that THIS bank cannot be any that are jumped to
  // b->bkmask is bitmask of possible banks


  for (i = 3; i < 7; i++)
    {
     b = bankmap + i;
     if (i != cbk) drop_banks_via_ints(i);
     else
      {
       b->dbank = 8;       // set up bank 8 directly
       b->bkmask = 4;
     #ifdef XDBGX
       DBGPRT(1, "bank %d is 8", i);
       #endif
      }
    }

   // now see if we can definitely find more banks
   // if any bank has single bit, can take that as definite ID
   // and drop that bit from all other banks

   for (i = 3; i < 7; i++)
    {
      b = bankmap + i;

      if (i != cbk)
        {  // not for bank 8
         if (b->bkmask == 1 || b->bkmask == 2 || b->bkmask == 8)
         {  // found a single bit, drop from other banks
           for (x = 3; x < 7; x++)
             {
               if (x != i)
                  {
                    #ifdef XDBGX
                   if (bankmap[x].bkmask & b->bkmask)
                     {
                      DBGPRT(1, "bank %d cannot be %d", x, b->bkmask < 8 ? b->bkmask-1 : 9);
                     }
                   #endif
                   bankmap[x].bkmask &= (~b->bkmask);
                  }
             }
         }
        }
    }

  // now, is there one bit per bank ?? if so have got the
 // bank order.  if not, then we'll have to guess.

   for (i = 3; i < 7; i++)
    {
      b = bankmap + i;
      switch (b->bkmask)
        {
          case 1 :
            b->dbank = 0;
            break;

          case 2 :
            b->dbank = 1;
            break;

          case 4 :
            b->dbank = 8;
            break;

          case 8 :
            b->dbank = 9;
            break;

          default :
           ans = 0;       // not single bits
           break;
        }

      if (!ans)
       {
        xprt(MSGFILE,2, "WARNING - Cannot confirm bank order is correct, selected order is best guess only");
        break;
       }
    }

return ans;
}



int do_banks(uchar *fbuf)
 {
  // set up banks after find_banks, or user command.

  // NO set up RAM bank [16] first 0x2000 of ibuf for ram and regs
  // NOTE - not technically safe if file has missing front bytes, but probably OK

// COPY  file into correct banks layout to c

  int ltj,bank,i, ans;
  BANK *b;

  //  do the checks and bank ordering.....

  bank = -1;                           // bank with the start jump (= code)
  ltj  = 0;                            // temp number of loopback jumps
  ans = 0;

  for (i = 0; i <= numbanks; i++)
      {
       b = get_bankmap(i+3);                // 3 to 6
       // b->fbuf points to bank start..................

       // any bank with P65 means all banks are 8065
       if (b->P65) set_cmdopt(OPT8065,1);              //    cmdopts |= 0x8;

       if (b->cbnk)
          {
           if (bank >= 0)
             {
               xprt(MSGFILE,1," Warning - ");
               xprt(MSGFILE,1,"Found multiple Start Code banks [bank 8]");
               xprt(MSGFILE,1,"Choosing first bank found, others ignored - use bank command to override");
               #ifdef XDBGX
                 DBGPRT(1," ** Found multiple code banks");
               #endif
               b->bprt = 1;    //keep the print
               b->bok = 0;
               b->dbank = 9;
               numbanks--;
            }
           else bank = i;

          }
       else ltj++;       // no of other banks
      }


  if (bank < 0)
     {
      xprt(MSGFILE,1,"Cannot find a Start/Code bank [bank 8]");
      xprt(MSGFILE,1,"Is this file an 8061 or 8065 Binary ??");
      ans = -4;       //return -4;
     }


   #ifdef XDBGX
   DBGPRT(1,"%d Banks, (temp start in 3)", numbanks+1 );
   #endif

  switch (numbanks)
    {

     default:
       xprt(MSGFILE,1,"0 or > 4 banks found. File corrupt?");
       ans = -2;          //return -2;
       break;

     case 0:                      // copybanks autostarts at [3] for source list
       mkbanks(1,8);
       break;

     case 1:                      // 2 bank, only 2 options
       if (bank == 1) mkbanks(2,1,8);
       else mkbanks(2,8,1);
       break;

     // no 3 bank bins (case=2) so far .....

     // MUST handle 3 banks !!!



     case 3:
       // do best guess bank order and then check it
       // can shuffle numbers before the cpy_banks which moves them.
       if (!chk_bank_order(bank+3)) // attempt to sort banks by intrpt handlers

       switch(bank)
        {
         // Alternate bank orders - best guess from bank 8 position

         case 0:
           mkbanks(4,8,9,0,1);
           break;

        case 1:
           mkbanks(4,9,8,0,1);
           break;

         case 2:
           mkbanks(4,0,1,8,9);
           break;

         case 3:
           mkbanks(4,0,1,9,8);
           break;
        }

       break;
      }

// what about multiple 8061 bank 8 ??
   // final copy and fill detection. this subr recalled if user command
 // DBG_banks();

  copy_banks(fbuf);

  #ifdef XDBGX
  list_banks(DBGFILE);
  DBGPRT(1,0);
  #endif

  return ans;
}



// do branch has a FULL copy of SFIND to ensure that parallel branches have their own
// parameter set established at start of each new branch (= a jump to here)
// this is because the registers may CHANGE in each branch.

// void (*set_str) (SFIND);
          //int anix;            // spf number of func moved to SFIND

int readbin(void)
{
  /* Read binary into main block 0x2000 onwards (ibuf[0x2000])
  *  ibuf  [0]-[2000]     RAM  (faked and used in this disassembly)
  *  ibuf  [2000]-[42000] up to 4 banks ROM, at 0x10000 each
  */

  int x;
  FDATA *fl;
  HDATA *fh;
  fl = get_fldata(0);             //bin file
  fh = get_flhdr();

  fseek (fl->fh, 0, SEEK_END);
  fh->fillen = ftell (fl->fh);
  fseek (fl->fh, 0, SEEK_SET);                // get file size & restore to start

  xprt(MSGFILE,2,"# Input file is '%s'",fl->fn);
  xprt(MSGFILE,2,"# File is %dK (0x%x) bytes",fh->fillen/1024, fh->fillen);

//  fldata.error = 0;

  if (fh->fillen > 0x40000)
   {
    xprt(MSGFILE,1,"# Bin file > 256k. Truncated to 256k");  // is this always right ??
    fh->fillen = 0x40000;
   }

  xprt(MSGFILE,1,0);

  fbinbuf = (uchar*) mem(0,0, fh->fillen);
  fread (fbinbuf, 1, fh->fillen, fl->fh);       // read in the file

  #ifdef XDBGX
   DBGPRT(1,"Read input file (0x%x bytes)", fh->fillen);
  #endif

  // file in temp buffer, now read and layout as per banks.

// check for file error ?
//  end = fldata.fillen;

  find_banks(fbinbuf);
  x = do_banks(fbinbuf);               // auto, may be modded later by command

  return x;
}




void set_struct(SFIND *f)
{
 // check for bank first, and then if truly valid
 if (!val_rom_addr(f->ans[0]))  f->ans[0] |= basepars.datbnk;

 if (!val_rom_addr(f->ans[0]))
     {
   //    #ifdef XDBGX
     //    DBGPRT(1,"Invalid Address %x", f->ans[0]);
  //     #endif
       return ;
      }





   // function
   if (f->spf->spf == 4) set_xfunc(f);
   // table

    if (f->spf->spf == 5) set_xtab(f);
}

void do_list_struct(SFIND *f)
{
 uint m, i, addr;       // temp !!
 uint start, gap;      // for pointer list
// LBK *k;
// ADT *a;


// this sort of works, problem is it proceeds down list and can get size and sign wrong.......

// and overlaps don't work right either............





#ifdef XDBGX
 DBGPRT(1," LIST START %x for rbase %x", f->lst[0], f->lst[1]);
#endif

  start = f->lst[0];
// add_data_cmd (m,  m+1, C_WORD, 0);       // add data entry - TEST !!

  m = start;


  if (!f->rg[1])
    {  // func, check first byte is valid.

     while (1)
      {
       addr = g_word(m);
       addr = addr + f->lst[1];
#ifdef XDBGX
         DBGPRT(0," %x ", addr);
#endif

       if (!val_rom_addr(addr)) break;
       i = g_byte(addr);
       if ( i != 0x7f && i != 0xff) break;     //check func start
       f->ans[0] = addr;        //set func address
       set_struct(f);
       m += 2;
       if (get_cmd(m,0)) break;        // is this any good ? (next ptr not done !)
      }
    }
 else
    {  // Table - check size first
     addr = g_word(m);      //start
     i = g_word(m+2);       // next pointer
     gap = i - addr;        // table size
     addr = addr + f->lst[1];
     i = addr;
     f->ans[0] = addr;
     m +=2;
     set_struct(f);

     while(1)
      {
       addr = g_word(m);
       addr = addr + f->lst[1];
#ifdef XDBGX
       DBGPRT(0,"  %x", addr);
#endif
       if ((addr - i) != gap)  break;      //assume all tabs are same size
       i = addr;
       f->ans[0] = addr;
       set_struct(f);
       m +=2;
      if (get_cmd(m,0)) break;
       }
    }

/*   if (m >= start+2) {
    k = add_cmd (start,  m-1, C_WORD|C_NOMERGE, 0);

   if (k)
    {
      a = (ADT*) chmem (CHADNL); //eappend_adt(vconvi(k->start),0);    // only one
   if (a)
     {
      a ->fid = vconvi(k->start);    // only one
      a->fend = 15;         //word
  //    a->bsize = 1;
      a->foff = 1;
      a->data = f->lst[1] & 0xffff;
      a->fnam = 1;
      a->cnt = 1;
      a = append_adt(a);       //vconvi(k->start),0);    // only one
     }
}}

*/
}


void do_branch(JMP *z, int cnt, SFIND f)
 {
    // do a branch from a jump. if jump(s) to here found, do as separate branch
   int xcnt;
   uint xofst, ix;
   JMP *j;
   CINST *c;       //INST *c;
   CHAIN *x;

   if (cnt <= 0) return;

   if (z->fscan)
    {
       #ifdef XDBGX
       DBGPRT(1,0);
       DBGPRT(0,"Branch ignore %x %d [%x %x] (%x %x)", z->fromaddr,cnt, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
      #endif
      return;
    }

      #ifdef XDBGX
       DBGPRT(1,0);
       DBGPRT(0,"Branch start %x %d [%x %x] (%x %x)", z->fromaddr,cnt, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
      #endif
   xcnt = cnt;
   xofst = z->fromaddr;

   while (xcnt)
     {

       c = find_opcode(xofst,0);
       if (!c) break;
       xcnt --;
       xofst = c->ofst;            //test - fixes some things....

       if (!xcnt && f.ans[0])
         {
          #ifdef XDBGX
            DBGPRT(0,"END, set %x at %x", f.ans[0], xofst);
          #endif
          set_struct(&f);        // add table with one row if run out of opcodes
         }

       if (xcnt <= 0)
         {
          #ifdef XDBGX
            DBGPRT(0,"END %x ", xofst);
          #endif
          break;
         }

       match_opnd(c,&f);

       if (f.ans[0] && f.ans[1])
         {
          #ifdef XDBGX
           DBGPRT(1," MATCH R%x=%x R%x=%x", f.rg[0], f.ans[0], f.rg[1], f.ans[1]);
          #endif

          if (f.lst[0]) do_list_struct(&f);
          else          set_struct(&f);   // add func or tab (add override)
          f.ans[0] = 0;
          f.ans[1] = 0;  // are these necessary ??
          f.lst[0] = 0;
          break;
         }
       #ifdef XDBGX
         else DBGPRT(0," %x[%d]", xofst, xcnt);
       #endif

       // see if any jumps in to xofst. if so, launch new branches. ALL jumps ?

       ix = get_tjmp_ix(xofst);     // start point

       x = get_chain(CHJPT);
       while(ix < x->num)
        {
          j = (JMP*) x->ptrs[ix];
          if (j->toaddr != xofst) break;
          #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(1,"JMP %x<-%x %d", j->toaddr, j->fromaddr, cnt);
          #endif


//what if a CALL ????               still partly works via subr, may need more........

         //j->from-j->cmpcnt to skip the cmp
// copy f for each new branch ??
    // if (j->jtype != J_SUB) {
         do_branch(j,xcnt,f);        //why xcnt ??
//         }
         ix++;
        }  // end jumps found loop

       xofst = c->ofst;
     }        // end find opcode loop
   z->fscan = 1;
}



// ** abandon this for funcs and do a liear search and verify ???
// MUCH easier than tables are.....................
// but STIL need a decent table recognizer................



void do_calls(SUB * sub, SFIND f)
{
  // do the 'base' (i.e. the subr calls outwards) separately to reset params

 JMP *j, *k;
 LBK *g;
 ADT *a;
 CHAIN *x;
 uint jx, ix, ofst;
 MHLD *h;


      #ifdef XDBGX
        DBGPRT(1,0);
        DBGPRT(1,"Sub Base %x (ss %x)", sub->start, f.spf->fendout );
      #endif


 ix = get_tjmp_ix(sub->start);     // start point



 x = get_chain(CHJPT);

 while(ix < x->num)
   {
    j = (JMP*) x->ptrs[ix];

    if (j->toaddr != sub->start) break;

    if (j->jtype == J_SUB)              // sub only for from FIRST call
        {
     //     if (f.rg[1] == 1) f.ans[1] = j->fromaddr + j->size;     //func only, for override
     #ifdef XDBGX
  DBGPRT(1,0);
          DBGPRT(0,"Call %x - [%x %x] (%x %x) ", j->fromaddr , f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
#endif
        //j->from-j->cmpcnt to skip the cmp
        //j->from+j->size to check for arguments
        // defined args are same as 'args' data command.....



//if (j->fromaddr == 0x2867e)
//{
 //   DBGPRT(0,0);
//}




        ofst = j->fromaddr + j->size;

        a = get_adt(vconvi(sub->start),0);

        if (a)
          {    // same as data command, but user specified
            h = do_adt_calc(a, ofst);

            f.ans[0] =  h->total.ival;      //ans =   decode_addr(a, ofst);
             #ifdef XDBGX
            DBGPRT(1,"do sub args at %x = %x", g->start, f.ans[0]);    // assume first parameter ???
             #endif
            mfree(h,sizeof(MHLD));     // free calc holder
           // need datbank ?? from the
          }

        else
        {
          g = get_aux_cmd(ofst, C_ARGS);
          if (g)
           {         // assume first parameter ???  how to tell ??
            a = get_adt(vconvi(g->start),0);
            if (a) {
            h = do_adt_calc(a, ofst);
            f.ans[0] =  h->total.ival;      //ans =   decode_addr(a, ofst);
            mfree(h,sizeof(MHLD));     // free calc holder

            #ifdef XDBGX
             DBGPRT(1,"do args at %x = %x", g->start, f.ans[0]);    // assume first parameter ???
            #endif
            // need datbank ??
           }}
        }

      do_branch(j, 20,f);

          // do_branch scans back from j->from FIRST before checking for jumps,
          // so add an extra check and loop for jumps to j->from
         // 20 instructions ?

          jx = get_tjmp_ix(j->fromaddr);     // start point
          while(jx < x->num)
            {
             k = (JMP*) x->ptrs[jx];
             if (k->toaddr != j->fromaddr) break;
             #ifdef XDBGX
              DBGPRT(1,0);
              DBGPRT(1,"To+ %x - [%x %x] (%x %x) ", j->fromaddr, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
             #endif
             //j->from-j->cmpcnt to skip the cmp
// copy f for each new branch ??
             do_branch(k,20,f);                //rg1,rg2,rg3,ans1,ans2,ans3);
             jx++;
            }  // end jump loop


          #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(1,"End call branch %x", j->fromaddr);
          #endif
       }
    ix++;

   }

}


LBK* del_olaps(uint addr, uint ix)
{
 LBK *n;
 CHAIN *x;

 x = get_chain(CHCMD);
 n = 0;
 if (ix < x->num) n = (LBK *) x->ptrs[ix];
 while (n && n->start < addr)
     {
      // n MUST be wrong - delete it. keep if higher or cmd
      if (n->usrcmd) break;
      if (n->fcom < C_TEXT) chdelete(x,ix,1); else break;
      if (ix < x->num) n = (LBK *) x->ptrs[ix]; else n = 0;
     }
return n;
}


//uint getfunceval(uint size)
//{


//}

void extend_func(uint ix)
{
  ADT *a, *b;
  LBK *k, *n;
  CHAIN *x;

  uint eadr, rsize, adr, val, eval;     //size


  x = get_chain(CHCMD);

  k = (LBK *) x->ptrs[ix];

  ix++;
  if (ix < x->num) n =  (LBK *) x->ptrs[ix]; else n = 0;
      // check end of func

  eadr = maxadd(k->end);                 // safety
  a = get_adt(vconvi(k->start),0);        // entry 1
  if (!a) return;
  b = get_adt(a,0);            // entry 2
  if (!b) return;

 // extend func to zero or max negative here

  rsize = bytes(a->fend) + bytes(b->fend);    // whole row size in BYTES
  eval  = get_signmask(a->fend);              // end value
  adr   = k->start;

  // need to check next command as well....

  while (adr < eadr)                      // check input value for each row (UNSIGNED)
    {
     adr += rsize;
     val = g_val(adr,0, (a->fend & 31));      // next input value as UNSIGNED
     if (val == eval)
        {
         eadr = adr + rsize-1;            // hit end input value, break
         break;
        }
    }

  k->end = eadr;                        // stage 1 of extend (to end value

  //check here if any command occurs inside the function and delete them
  n = del_olaps(eadr, ix);

  //adr is now at first zero row, and val is last value

  eadr = maxadd(k->end);           // reset end (why??)

  if (n)
    {
      if (n->usrcmd) eadr = n->start;  // can't go past a user command.
      if (n->fcom > C_TEXT) eadr = n->start;  // or past funcs or tabs (etc)
    }
  // set row size for whole row for extend check.
  // word ->long, byte->word

  rsize = (a->fend & 31) + (b->fend & 31) +1;        // row size (unsigned)

  eval = g_val(adr,0, rsize);         // and its full row value

// must check overlap with data HERE

  while (adr < eadr)
    {       // any matching final whole rows
     adr += bytes(rsize);
     val = g_val(adr,0, rsize);
     if (val != eval) break;
     if (n && !eval && n->start == adr) break;   // match next cmd for extended zeroes
    }

  // adr is now start of next item (end of extend values)
  // if end value is zero need extra check
  // otherwise need to delete any skipped entries........

  n = del_olaps(adr, ix);

 //check again for overlaps as del-olaps may have moved n

 if (n && adr > n->start)
    {
      if (n->usrcmd) adr = n->start;  // can't go past a user command.
      if (n->fcom > C_TEXT) adr = n->start;  // or past funcs or tabs (etc)
    }


  // need any more here ???

//  if (eval != 0 || n == 0 || n->start >= adr)
//    {  // end of list or not zero value or no overlap
//     if (adr > k->end) k->end = adr-1;
//    }
//  else
//    {     // check next command

      // n is < and value is 0
                   // probably wrong, but safe for now
                   // extend if only one word/byte
   //             k->end = n->start-1;          // TEMP !!


                 k->end = adr-1;
 //   }


//now check if all outputs are in upper byte and < 16
// if so this is dimension for table, and can autosize it.


  rsize = bytes(a->fend) + bytes(b->fend);    // whole row size in BYTES

/*
if (rsize == 4)
  {          //word funce - check for table dim style (top byte only)
   uint lval, chg;
// prob need more rules (like eval should always be <= lval...but val must change also
// and end up at zero ??)

// need to check not signed as well !!

  adr  = k->start + bytes(b->fend);
  eadr = k->end;
  eval = 1;                         // flag
  chg  = 0;                       //value changed
  lval = 0x2000;                     // 32 in top byte
  while (adr < eadr)                      // check input value for each row (UNSIGNED)
    {
     val = g_val(adr,0, (b->fend & 31));      // next input value as UNSIGNED
     if (val & 0xff)
        { //bottom byte set
         eval = 0;
         break;
        }
     if (val != lval) chg = 1;

     if (val > lval)
       {  // downward trend only
         eval = 0;
         break;
        }
      adr += rsize;
      lval = val;
    }

if (eval && chg)
 {       //passes test as dimension func for table
   b->div = 1;
   b->fldat = 256.0;           //set divisor at 256
//change name ??
//add_mcalc....
 }

}          */




 }


// experimentals


int endval (uint fend)
{
  switch (fend)
  {
      default:
        return 0;
      case 0x27:
        return 0x80;
      case 0x2f:
        return 0x8000;
  }

}

int startval (uint fend)
{
  switch (fend)
  {
      default:
        return 0;
      case 0x7:
        return 0xff;
      case 0x27:
        return 0x7f;
      case 0xf:
        return 0x8000;
      case 0x2f:
        return 0x7fff;
  }

}





/*

void test_func(uint start, uint fend)
{
    // fend & 0x20 for sign...
    // 7 27, f 2f

uint addr, end, rsize;
int val,lval, cnt;

//if (start == 0x827b4)
//{
//DBGPRT(1,0);
//}

rsize = bytes(fend);          // size in bytes

lval = g_val(start, 0, fend);   // start value as unsigned

if (lval != startval(fend)) return ;

val = lval;

// need row extender in here for some short tables

rsize *= 2;      // whole row size, input values only.
addr = start + rsize;
end  = start + 128;      // safety

cnt = 1;
while (addr < end)
  {
    val = g_val(addr, 0, fend);
    if (val >= lval) break;
    lval = val;
    if (lval == endval(fend)) break;
    addr += rsize;
    cnt++;
}

// 3 rows = 6 * size
if (cnt > 3)
{
      #ifdef XDBGX
DBGPRT (1,"possible func at %x-%x (%d)", start, addr, fend);
#endif
}


}




uint scan_func(LBK *s, LBK *n)       //uint start, uint end)
{

   uint addr, start, end, val;  //, cnt;

   start = s->end+1;
   end  = n->start-1;

   if (g_bank(start) != basepars.datbnk) return 0;

   #ifdef XDBGX
      DBGPRT(1,"func scan %x-%x", start, end);
   #endif

   addr = start;

  //   uint bytes(uint fend)

while (addr < end)
  {
   val = g_byte(addr);
   if (val == 0xff || val == 0x7f)
    {
     test_func(addr,0x7);       // uns byte
     test_func(addr,0x27);      // sign byte
     if (!(addr &1))
      {
       test_func(addr,0xf);
       test_func(addr,0x2f);  // word
      }
     }
    addr++;

 }
 return 0;
}

*/



void extend_table(uint ix)
{
  uint apeadr, maxadr, size, val, eval, temp,jx;
  ADT *a;
  LBK *k, *n;
  CHAIN *x;

  x = get_chain(CHCMD);

  k = (LBK *) x->ptrs[ix];

  // DBGPRT(1,"extend table for %x", k->start);
 // attempt to extend by data,use external proc
 // to do best match by data for row and col sizes

  if (ix >= x->num)
   {
     apeadr = maxadd(k->end)+1;
     maxadr = apeadr;
   }
  else
   {
    jx = ix+1;
    n = (LBK *) x->ptrs[jx];

  //  DBGPRT(1,"next cmd is %x-%x", n->start, n->end);
    apeadr = n->start;             // apparent end from next cmd

    while ( n->fcom < C_TEXT && !n->usrcmd)
     {  // struct of user command
       jx++;
       if (jx >= x->num) break;
       n = (LBK *) x->ptrs[jx];    //skip all byte and word
     }

    maxadr = n->start;            // max end. not skippable
   }

  a = get_adt(vconvi(k->start),0);

  if (!a) return;
  size = apeadr - k->start;    // size in bytes to apparent end
  eval = size/a->cnt;          // max rows
  val  = eval * a->cnt;        // size in whole rows

 temp = apeadr;
 do_table_sizes(k->start, a->cnt, &temp, maxadr);  // temp, for all tables


 if (a->cnt > 2)
  {  // must have valid rows
  if (val == size)
    {
      // MATCH with extra rows.... eadr is next cmd
      k->end = apeadr-1;
      return;
    }

  if (val+1 == size)
   {   // could be a filler - need to check next data cmd ?
      if (g_byte(apeadr-1) == 0xff) k->end = apeadr-2;
      return;
   }
  }

   // default - move table to nearest whole row ?? not perfect, but OK for now.
   // then investigate actual data for sizes
   k->end = k->start+val-1;       // temp

  size = do_table_sizes(k->start, a->cnt, &apeadr, maxadr);

  //need to get end address out as well.......use apeadr

 if (size)
     {
      a->cnt = size;
      k->size = size;  // should do a dsize() really         // 1st test
     if (k->start+apeadr >= k->end) k->end = k->start+apeadr-1;
     }
 else
     {
// er....
    }

 del_olaps(k->end,ix+1);       //remove any spanned entries

}



void do_structs()

 {
   uint i;
   SUB *sub;

   SFIND f;
   SPF *x;
   CHAIN *c;
   // go down tab and func subroutines call trees  and find params (structs)

   // func subroutines
#ifdef XDBGX
DBGPRT(1, "in Do_Structs");
#endif

   c = get_chain(CHSUBR);

   for (i = 0; i < c->num; i++)
   {
    sub =  (SUB *) c->ptrs[i];
    x = get_spf(vconvi(sub->start));

    if (x)
     {

//        #ifdef XDBGX
 //         DBGPRT(1, "For subr %x", sub->start);
  //      #endif


      // common to both
      f.lst[0] = 0;
      f.spf    = x;
      f.rg[0]  = x->addrreg;        // address
      f.ans[0] = 0;

      //functions
      if (x->spf == 4)
       {
        f.rg[1]  = 0;              // no size reg
        f.ans[1] = 1;              // mark as found
       }

      // table subroutines

      if (x->spf == 5)
       {
        f.rg[1]  = x->sizereg;        // cols
        f.ans[1] = 0;
       }

      do_calls(sub,f);
    }

   }


}


// now extend the end of funcs for the extra filler rows.....
// probably need to check tables too.

void extend_structs()
{
   uint i;
   LBK *k;
   CHAIN *c;

   c = get_chain(CHCMD);

   for (i = 0; i < c->num; i++)
   {
     k = (LBK *) c->ptrs[i];
     if (!k->usrcmd)
       {
        if (k->fcom == C_FUNC)  extend_func(i);
        if (k->fcom == C_TABLE) extend_table(i);
       }
   }
}


/***************************************************************
 *  disassemble EEC Binary
 ***************************************************************/
short disassemble (char *fstr)
{
  int ans;   //, i;
  BANK *b;
  FDATA *fl;
  HDATA *fh;

  fl = get_fldata(0);
  fh = get_flhdr();
  init_structs();
  calcfiles(fstr);
  ans = openfiles();

  if (ans & 1)
   {
     printf("Cannot open '%s'\n",fl[0].fn );
     return 1;
   }

 /* i = 0;
  while (ans)
   {
    if (ans & 1) xprt(MSGFILE,0,"Cannot open '%s' \n",fl[i].fn );
    ans >>= 1;
    i++;
   }
*/

 /*      if (ans >= 0)
    {    // use bitmask for any errors.
     printf("File '%s' not found or cannot open\n",fldat[flhdr.error].fn );
     return flhdr.error;
    }
*/

//check file errors HERE




 // time(&timenow);
 // sprintf(nm, "%s", ctime(&timenow));

/*


#include <stdio.h>
#include <time.h>

void main()
{
    time_t t;
    time(&t);
    clrscr();

    printf("Today's date and time : %s",ctime(&t));
    getch();
}


*/

//  MSGFILE 2 , DBGFILE 5 numfiles 6....


  xprt(MSGFILE,1,0);
  xprt(MSGFILE,1,"# ----------------------------");
  xprt(MSGFILE,1,"# SAD Version %s (%s)", SADVERSION, SADDATE);
  xprt(MSGFILE,2,"# ----------------------------");

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(1,"# ----------------------------");
    DBGPRT(1,"   SAD Version %s (%s)", SADVERSION, SADDATE);
    DBGPRT(2,"# ----------------------------");
  #endif


  ans = readbin();


//errors ??
  /* read bin file and sort out banks
   but allow getudir in case of manual set banks */

  anlpass++;            //anlpass = 1  cmds and setup
  show_prog();

  getudir();                       // AFTER readbin...

  if (numbanks < 0)
  {
     // wrong if no banks
    xprt(MSGFILE,1,"Abandoned !!");
    printf("Abandon - can't process - see warnings file\n");
    return 0;
  }

  anlpass++;       //anlpass = 2  signature pass
  show_prog();    //++anlpass);
  prescan_sigs();

   //check_rbase();                     // check static rbase setup (2020 etc, not as sig)

  xprt(MSGFILE,2,"# ----- Start Disassembly phase 1 -----");

  scan_all ();        // do all the analysis

 // ----------------------------------------------------------------------

  anlpass++;       //anlpass = 3  scan phase
  show_prog();              //++anlpass);             //anlpass = 3  scan phase

  do_jumps();

  xprt(MSGFILE,2,"# ----- End   Disassembly phase 1 -----");
  xprt(MSGFILE,2,"# ----- Start Disassembly phase 2 -----");

   // ----------------------------------------------------------------------
   // and do a whole new pass here for the data ??? loops,structs, tables....
   // ----------------------------------------------------------------------



  // scan_dc_olaps();            // check and tidy code and data overlaps


// scan funcs
// scan tabs ??

  // scan_data_gaps();

 //  scan_lpdata();


   anlpass++;           //anlpass = 4 ANLFFS

  show_prog ();             //++anlpass);            //anlpass = 4 ANLFFS

  xprt(MSGFILE,2,"# ----- End   Disassembly phase 2  -----");

  turn_scans_into_code();      // turn all valid scans into code

  scan_opcodes();               // rbase checks

  do_structs();                 // recursive loop down call branches tablu and funclu

 // but scan gaps can add more jumps !!!

 // scan_cmd_gaps(scan_func);        not yet!!          //find funcs and tables here ??  after code before data ?.



 //check_dtk_links();

 turn_dtd_into_data();                           // add data found fom dtd

 scan_cmd_gaps(&cinst,scan_cgap);                // code scans

 scan_cmd_gaps(&cinst,scan_fillgap);             // fill data

 turn_dtd_into_data();                           // add data found fom dtd
 do_structs();                 // recursive loop do again after code scans


 extend_structs();

//after data processed, clear dtk+dtd and check over ???

 //validate_rbases();                    // scan_opcodes recheck for any newly discovered rbases

// and now look for other functions and tabs ?? discover_struct() :

  // ----------------------------------------------------------------------





 anlpass++;          //anlpass = 5 = ANLPRT

   show_prog ();           //++anlpass);                 //anlpass = 5 = ANLPRT

  #ifdef XDBGX
    DBGPRT(1,"#\n####################################################################################################");
    DBG_data();
  #endif

  xprt(MSGFILE,2,"# ----- Output Listing to file %s", fl[1].fn);

  anlpass = ANLPRT;                   // safety
  do_listing ();
  xprt(LSTFILE,2,0);     // flush output file
  xprt(LSTFILE,2, " ##########   END of Listing   ##########");



// could put messages in here with wnprt.................


  xprt(MSGFILE,2,0);
  xprt(MSGFILE,1,"# ---------------------------------------------------------------------------------------------");
  xprt(MSGFILE,1,"# The disassembler has scanned the binary and produced the following command list.");
  xprt(MSGFILE,1,"# This list is not guaranteed to be perfect, but should be a good base.");
  xprt(MSGFILE,1,"# Commented lines for information bu may be uncommented for use (e.g. banks)");
  xprt(MSGFILE,1,"# This following list can be copied and pasted into a directives file.");
  xprt(MSGFILE,3,"# ---------------------------------------------------------------------------------------------");

  list_dirs(MSGFILE);

  mfree(fbinbuf,fh->fillen);         //and file buffer


 for (ans = 0; ans < BMAX; ans++)
  {
   b = bankmap + ans;

   if (b->bok)  mfree(b->fbuf, 0x10000);             //temp

  }

//  mfree(rgstat, 0x400 * sizeof(RST));
  free_structs();
  xprt(MSGFILE,2,0);
  xprt(MSGFILE,2, "# ----- END of disassembly run -----");
  printf ("\n END of run\n");
  closefiles();
  return 0;
}

