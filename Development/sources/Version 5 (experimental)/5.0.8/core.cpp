
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version number and notes in shared.h
 ******************************************************/

  #include  "core.h"             // this calls shared.h

/**********************************************************************************
local declarations
***********************************************************************************/

 void do_code (SBK *, INST *);


//*****************************************************************************
// code emulation subroutines for opcodes in OPC struct (below)

SBK* clr  (SBK *, INST *); SBK* ldx   (SBK *, INST *); SBK* stx  (SBK *, INST *); SBK* orx  (SBK *, INST *);
SBK* add  (SBK *, INST *); SBK* andx  (SBK *, INST *); SBK* neg  (SBK *, INST *); SBK* cpl  (SBK *, INST *);
SBK* sbt  (SBK *, INST *); SBK* cmp   (SBK *, INST *); SBK* mlx  (SBK *, INST *); SBK* dvx  (SBK *, INST *);
SBK* shl  (SBK *, INST *); SBK* shr   (SBK *, INST *); SBK* popw (SBK *, INST *); SBK* pshw (SBK *, INST *);
SBK* inc  (SBK *, INST *); SBK* dec   (SBK *, INST *); SBK* bka  (SBK *, INST *); SBK* cjm  (SBK *, INST *);
SBK* bjm  (SBK *, INST *); SBK* djnz  (SBK *, INST *); SBK* ljm  (SBK *, INST *); SBK* cll  (SBK *, INST *);
SBK* ret  (SBK *, INST *); SBK* scl   (SBK *, INST *); SBK* skj  (SBK *, INST *); SBK* sjm  (SBK *, INST *);
SBK* pshp (SBK *, INST *); SBK* popp  (SBK *, INST *); SBK* nrm  (SBK *, INST *); SBK* sex  (SBK *, INST *);
SBK* clc  (SBK *, INST *); SBK* stc   (SBK *, INST *); SBK* edi  (SBK *, INST *); SBK* nop  (SBK *, INST *);
SBK* clv  (SBK *, INST *);


/*******************************************************************************
 * opcode index, opcode to OPC table (below)
 * 0 - 0xff maps to main OPC table index.
 * OPC (main) table is ordered to group opcode types together
 * 0 is an invalid opcode or address mode.
 * **********************/

uchar opcind[256] =
{
// 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
   76,   89,   96,   91,   0,    98,   100,  93,   4,    1,    5,    0,    6,    3,    7,    94,    // 00 - 0f
// SKIP, CLRW, CPLW, NEGW, INV,  DECW, SEXW, INCW, SHRW, SHLW, ASRW,       SHRD, SHLD, ASRD, NORML
   112,  88,   95,   90,   0,    97,   99,   92,   8,    2,    9,    0,    0,    0,    0,    0,     // 10 - 1f
// RBNK, CLRB, CPLB, NEGB, INV,  DECB, SEXB, INCB, SHRB, SHLB, ASRB
   77,   77,   77,   77,   77,   77,   77,   77,   78,   78,   78,   78,   78,   78,   78,   78,    // 20 - 2f
//  SJMP                                           SCALL
   70,   70,   70,   70,   70,   70,   70,   70,   71,   71,   71,   71,   71,   71,   71,   71,    // 30 - 3f
//  JNB                                            JB
   13,   13,   13,   13,   25,   25,   25,   25,   29,   29,   29,   29,   20,   20,   20,   20,    // 40 - 4f
// AN3W                    AD3W                    SB3W                    ML3W
   12,   12,   12,   12,   24,   24,   24,   24,   28,   28,   28,   28,   18,   18,   18,   18,    // 50 - 5f
// AN3B                    AD3B                    SB3B                    ML3B
   11,   11,   11,   11,   23,   23,   23,   23,   27,   27,   27,   27,   16,   16,   16,   16,    // 60 - 6f
// AN2W                    AD2W                    SB2W                    ML2W
   10,   10,   10,   10,   22,   22,   22,   22,   26,   26,   26,   26,   14,   14,   14,   14,    // 70 - 7f
// AN2B                    AD2B                    SB2B                    ML2B
   31,   31,   31,   31,   33,   33,   33,   33,   35,   35,   35,   35,   38,   38,   38,   38,    // 80 - 8f
// ORW                     XORW                    CMPW                    DIVW
   30,   30,   30,   30,   32,   32,   32,   32,   34,   34,   34,   34,   36,   36,   36,   36,    // 90 - 9f
// ORB                     XORB                    CMPB                    DIVB
   41,   41,   41,   41,   43,   43,   43,   43,   45,   45,   45,   45,   46,   46,   46,   46,    // a0 - af
// LDW                     ADCW                    SBBW                    LDZBW
   40,   40,   40,   40,   42,   42,   42,   42,   44,   44,   44,   44,   47,   47,   47,   47,    // b0 - bf
// LDB                     ADCB                    SBBB                    LDSBW
   49,    0,   49,   49,   48,    0,   48,   48,   50,   50,   50,   50,   52,    0,   52,   52,    // c0 - cf
// STW (no imm)            STB (no imm)            PUSHW                   POPW (no imm)
   54,   63,   64,   56,   60,   58,   66,   69,   55,   62,   65,   57,   61,   59,   67,   68,    // d0 - df
// JNST, JLEU, JGT,  JNC,  JNVT, JNV,  JGE,  JNE,  JST,  JGTU, JLE,  JC,   JVT,  JV,   JLT,  JE
   72,    0,    0,    0,    0,    0,    0,   75,    0,    0,    0,    0,    0,    0,    0,   73,    // e0 - ef
// DJNZ                                      JUMP                                            CALL
   80,   82,   84,   86,  107,  108,  109,    0,  102,  101,  103,  104,  105,  110,  111,  106     // f0 - ff
// RET,  RETI, PSHP, POPP,Bnk0, Bnk1, Bnk2,       CLC,  STC,  DI,   EI,   CLVT, Bnk3, SIGN, NOP
};




/****************************************************************************
 OPC table - main opcode definition, index for 0-0xff above
*
* Code Note - in ansi 'C' rules, Octal escape sequences (\nnn) are limited to three digits,
* but hex sequences (\xnn..) are not.
* Octal is used here in 'psuedo source' strings to embed operand number, below so that -
* 1) Letters are not interpreted as part of an embedded number (e.g. 'C' of "CY =").
* 2) entry as /007 will enable number next to a hex letter (not actually used here)
* '\2x' in the string means print the size and sign flags for operand 'x' (1-3).
*****************************************************************************
*
* 806x bit order. 7 is MS, 0 LS, = bit order is 7,6,5,4,3,2,1,0 as read.
*
* Operand sizes shown here are 'field end bit', with an implied field start of 0,
* with 0x20 (32) for signed, 0x40 (64) for write op,  giving values -
*    7,  0xf,   0x17,  0x1f   unsigned read,  byte, word, triple, long
* 0x27,  0x2f,  0x37,  0x3f     signed read,  byte, word, triple, long  (+ 0x20)
* 0x47,  0x4f,  0x57,  0x5f   unsigned write, byte, word, triple, long  (+ 0x40)
* 0x67,  0x6f,  0x67,  0x7f   signed   write, byte, word, triple, long  (+ 0x60)
* (note that 'triple' does not exist for opcodes, but many bins use 24 bit (+triple) variables
*
* EXTRA FLAGS in upper byte of fend, NOT passed onwards into opcode instance.
* 0x100  sign and size marker (may use 200 to split sign and size)
* 0x400  for divide, increment value by size (for mod in divide)
* 0x800  adcw/sbbw carry and skip operand if null
*
*
*
*
* Operands when decoded -
*  Op[3] is ALWAYS the WRITE (destination) operand
*  internal calcs are always [3] = [2] <op> [1]
*  operands [2] and [1] are copied/swopped as reqd. in opcode handlers to achieve this
***********************************************************************
* 'Signature index' is used by signature matching to group like opcodes together
* values are (0 - 63 max)
* 0  special   1  clr    2  neg,cpl   3 shift l   4 shift R   5 and
* 6  add       7  subt   8  mult      9 or        10 cmp     11 divide
* 12 ldx       13 stx    14 push     15 pop       16 sjmp    17 subr
* 18 djnz      19 cjmp   20 ret      21 carry     22 other   23 bit
* 24 dec       25 inc
* 40+ are classed as 'optional' and skipped by default, but detectable with -
* 40 sign     41 pushp    42 popp    43 di       44 ei       45 RAMbnk
* 46 ROMbnk   47 nop      48 others
*
* (sjmp is static jump , cjmp is conditional jump)
*
* OPC item layout (one row)
* signature index,  'fe' prefix allowed (prefixed version is ALWAYS next table entry),
* 8065 only, updates PSW mask,  number of ops, show_sign,  3 op sizes (1,2,3 as above),
* opcode name , pseudo source string

 * PSW mask is    Z N V VT C ST    in order of manual.
***********************************************************************************/


const OPC opctbl[] = {                                                              // 113 entries
{ 0 ,  0, 0,   0, 0,   0,     0,     0,       0   ,"!INV!" , "" },             // zero for invalid opcodes

{ 3 ,  0, 0,   0x3e, 2,   0x7,   0xf,   0x4f , shl   ,"shlw"  , "\3 <<= \1;" },                // 1  3 = 2 << 1
{ 3 ,  0, 0,   0x3e, 2,   0x7,   0x7,   0x47 , shl   ,"shlb"  , "\3 <<= \1;" },
{ 3 ,  0, 0,   0x3e, 2,   0x7,   0x1f,  0x15f, shl   ,"shldw" , "\3 <<= \1;" },

{ 4 ,  0, 0,   0x23, 2,   0x7,   0xf,   0x4f,  shr   ,"shrw"  , "\3 >>= \1;" },                // 4
{ 4 ,  0, 0,   0x33, 2,   0x7,   0x2f,  0x16f, shr   ,"asrw"  , "\3 >>= \1;" },
{ 4 ,  0, 0,   0x3e, 2,   0x7,   0x1f,  0x15f, shr   ,"shrdw" , "\3 >>= \1;" },               // 6
{ 4 ,  0, 0,   0x33, 2,   0x7,   0x3f,  0x17f, shr   ,"asrdw" , "\3 >>= \1;" },
{ 4 ,  0, 0,   0x23, 2,   0x7,   0x7,   0x47,  shr   ,"shrb"  , "\3 >>= \1;" },                // 8
{ 4 ,  0, 0,   0x33, 2,   0x7,   0x27,  0x167, shr   ,"asrb"  , "\3 >>= \1;" },

{ 5 ,  0, 0,   0x30, 2,   0x7,   0x7,   0x47,  andx  ,"an2b"  , "\4\2 &= \1;"  },              // 10
{ 5 ,  0, 0,   0x30, 2,   0xf,   0xf,   0x4f,  andx  ,"an2w"  , "\4\2 &= \1;" },
{ 5 ,  0, 0,   0x30, 3,   0x7,   0x7,   0x47,  andx  ,"an3b"  , "\4\3 = \2 & \1;"  },
{ 5 ,  0, 0,   0x30, 3,   0xf,   0xf,   0x4f,  andx  ,"an3w"  , "\4\3 = \2 & \1;" },

{ 8 ,  1, 0,   0,    2,   0x7,   0x107, 0x14f, mlx   ,"ml2b"  , "\3 = \2 * \1;" },           // 14 technically psw mask of 1...
{ 8 ,  0, 0,   0,    2,   0x27,  0x127, 0x16f, mlx   ,"sml2b" , "\3 = \2 * \1;"},
{ 8 ,  1, 0,   0,    2,   0xf,   0x10f, 0x15f, mlx   ,"ml2w"  , "\3 = \2 * \1;"  },
{ 8 ,  0, 0,   0,    2,   0x2f,  0x12f, 0x17f, mlx   ,"sml2w" , "\3 = \2 * \1;"},

{ 8 ,  1, 0,   0,    3,   0x7,   0x107, 0x14f, mlx   ,"ml3b"  , "\3 = \2 * \1;"  },          // 18
{ 8 ,  0, 0,   0,    3,   0x27,  0x127, 0x16f, mlx   ,"sml3b" , "\3 = \2 * \1;" },           // 19
{ 8 ,  1, 0,   0,    3,   0xf,   0x10f, 0x15f, mlx   ,"ml3w"  , "\3 = \2 * \1;"  },          // 20
{ 8 ,  0, 0,   0,    3,   0x2f,  0x12f, 0x17f, mlx   ,"sml3w" , "\3 = \2 * \1;" },           // 21

{ 6 ,  0, 0,   0x3e, 2,   0x7,   0x7,   0x47,  add   ,"ad2b"  , "\3 += \1;" },                 // 22
{ 6 ,  0, 0,   0x3e, 2,   0xf,   0xf,   0x4f,  add   ,"ad2w"  , "\3 += \1;" },
{ 6 ,  0, 0,   0x3e, 3,   0x7,   0x7,   0x47,  add   ,"ad3b"  , "\3 = \2 + \1;"  },
{ 6 ,  0, 0,   0x3e, 3,   0xf,   0xf,   0x4f,  add   ,"ad3w"  , "\3 = \2 + \1;" },

{ 7 ,  0, 0,   0x3e, 2,   0x7,   0x7,   0x47,  sbt   ,"sb2b"  , "\3 -= \1;" },                 // 26
{ 7 ,  0, 0,   0x3e, 2,   0xf,   0xf,   0x4f,  sbt   ,"sb2w"  , "\3 -= \1;" },
{ 7 ,  0, 0,   0x3e, 3,   0x7,   0x7,   0x47,  sbt   ,"sb3b"  , "\3 = \2 - \1;"  },
{ 7 ,  0, 0,   0x3e, 3,   0xf,   0xf,   0x4f,  sbt   ,"sb3w"  , "\3 = \2 - \1;" },

{ 9 ,  0, 0,   0x30, 2,   0x7,   0x7,   0x47,  orx   ,"orb"   , "\4\3 |= \1;" },               // 30
{ 9 ,  0, 0,   0x30, 2,   0xf,   0xf,   0x4f,  orx   ,"orw"   , "\4\3 |= \1;" },
{ 9 ,  0, 0,   0x30, 2,   0x7,   0x7,   0x47,  orx   ,"xorb"  , "\4\3 ^= \1;" },
{ 9 ,  0, 0,   0x30, 2,   0xf,   0xf,   0x4f,  orx   ,"xorw"   , "\4\3 ^= \1;" },

{ 10,  0, 0,   0x3e, 2,   0x7,   0x7,   0,     cmp   ,"cmpb"  , "" },                          // 34
{ 10,  0, 0,   0x3e, 2,   0xf,   0xf,   0,     cmp   ,"cmpw"  , "" },

{ 11,  1, 0,   0x0c, 2,   0x7,   0x10f, 0x547, dvx   ,"divb"  , "\3 = \2 / \1;\n\3 = \2 % \1;" },           // 36
{ 11,  0, 0,   0x0c, 2,   0x27,  0x12f, 0x567, dvx   ,"sdivb" , "\3 = \2 / \1;\n\3 = \2 % \1;"},            // 37
{ 11,  1, 0,   0x0c, 2,   0xf,   0x11f, 0x54f, dvx   ,"divw"  , "\3 = \2 / \1;\n\3 = \2 % \1;" },           // 38
{ 11,  0, 0,   0x0c, 2,   0x2f,  0x13f, 0x56f, dvx   ,"sdivw" , "\3 = \2 / \1;\n\3 = \2 % \1;"},            // 39

{ 12,  0, 0,   0,    2,   0x7,   0x7,   0x47,  ldx   ,"ldb"   , "\4\3 = \1;" },                // 40
{ 12,  0, 0,   0,    2,   0xf,   0xf,   0x4f,  ldx   ,"ldw"   , "\4\3 = \1;" },

{ 6 ,  0, 0,   0x3e, 2,   0x807, 0x7,   0x47,  add   ,"adcb"  , "\3 += \1;"},            // 42
{ 6 ,  0, 0,   0x3e, 2,   0x80f, 0xf,   0x4f,  add   ,"adcw"  , "\3 += \1;"},
{ 7 ,  0, 0,   0x3e, 2,   0x807, 0x7,   0x47,  sbt   ,"sbbb"  , "\3 -= \1;"},
{ 7 ,  0, 0,   0x3e, 2,   0x80f, 0xf,   0x4f,  sbt   ,"sbbw"  , "\3 -= \1;"},

{ 12,  0, 0,   0,    2,   0x107, 0x107, 0x14f, ldx   ,"ldzbw" , "\3 = \1;" },                // 46
{ 12,  0, 0,   0,    2,   0x127, 0x107, 0x16f, ldx   ,"ldsbw" , "\3 = \1;" },

{ 13,  0, 0,   0,    2,   0x7,   0x7,   0x47,  stx   ,"stb"   , "\4\3 = \2;" },                // 48
{ 13,  0, 0,   0,    2,   0xf,   0xf,   0x4f,  stx   ,"stw"   , "\4\3 = \2;" },                 //  N.B. ops swopped later

{ 14,  1, 0,   0,    1,   0xf,   0,     0,     pshw  ,"push"  , "push(\1);" },                 // 50
{ 14,  0, 1,   0,    1,   0xf,   0,     0,     pshw  ,"pusha" , "pushalt(\1);" },
{ 15,  1, 0,   0,    1,   0xf,   0,     0x4f,  popw  ,"pop"   , "\3 = pop();" },               // 52
{ 15,  0, 1,   0,    1,   0xf,   0,     0x4f,  popw  ,"popa"  , "\3 = popalt();" },

//conditionals use sizes for different items PSW_Z, PSW_N, PSW_V, PSW_VT, PSW_C , PSW_ST .    OK
// use /xc (12) as bitmask for flags ??  maybe

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jnst"  , "\xbPSW_ST = 0) \x9" },          // 54  to 71 cond jumps
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jst"   , "\xbPSW_ST = 1) \x9" },          // 55

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jnc"   , "\10PSW_C = 0) \x9" },            // 56  (\10 = 8)
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jc"    , "\10PSW_C = 1) \x9" },            // 57

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jnv"   , "\xbPSW_V = 0) \x9" },           // 58
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jv"    , "\xbPSW_V = 1) \x9" },           // 59

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jnvt"  , "\xbPSW_VT = 0)" },          // 60
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jvt"   , "\xbPSW_VT = 1)" },          // 61

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jgtu"  , "\7>"   },                     // 62
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jleu"  , "\7<="  },                     // 63

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jgt"   , "\7>"       },                 // 64
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jle"   , "\7<="      },                 // 65

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jge"   , "\7>="      },                 // 66
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jlt"   , "\7<"       },                 // 67

{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"je"    , "\7="       },                 // 68
{ 19,  0, 0,   0,    1,   0,     0,     0,     cjm   ,"jne"   , "\7!="      },                 // 69

{ 23,  0, 0,   0,    2,   0,     0x7,   0x7,   bjm   ,"jnb"   , "\xb\3\2 = 0) \x9" },         // 70
{ 23,  0, 0,   0,    2,   0,     0x7,   0x7,   bjm   ,"jb"    , "\xb\3\2 = 1) \x9" },         // 71

{ 18,  0, 0,   0,    2,   0,     0,     0x47,  djnz  ,"djnz"  , "\3--;\nif (\3 != 0) \x9" },  // 72

{ 17,  1, 0,   0,    1,   0,     0,     0,     cll   ,"call"  , "\5" },                        // 73   Long jumps
{ 17,  0, 1,   0,    1,   0,     0,     0,     cll   ,"calla" , "\5" },                        // 74
{ 16,  0, 0,   0,    1,   0,     0,     0,     ljm   ,"jump"  , "\x9" },                       // 75

{ 16,  0, 0,   0,    0,   0,     0,     0,     skj   ,"skip"  , "\x9" },                       // 76 skip (= sjmp [pc+2])
{ 16,  0, 0,   0,    1,   0,     0,     0,     sjm   ,"sjmp"  , "\x9" },                       // 77     short jumps
{ 17,  1, 0,   0,    1,   0,     0,     0,     scl   ,"scall" , "\5" },                        // 78   short call
{ 17,  0, 0,   0,    1,   0,     0,     0,     scl   ,"sclla" , "\5" },                        // 79

{ 20,  1, 0,   0,    0,   0,     0,     0,     ret   ,"ret"   , "return;" },                   // 80  returns
{ 20,  0, 1,   0,    0,   0,     0,     0,     ret   ,"reta"  , "altreturn;" },
{ 20,  1, 0,   0,    0,   0,     0,     0,     ret   ,"reti"  , "return;" },
{ 20,  0, 1,   0,    0,   0,     0,     0,     ret   ,"retia" , "altreturn;" },                   // 83

{ 41,  1, 0,   0,    0,   0,     0,     0,     pshp   ,"pushp" , "push(PSW);" },                // 84 push psw
{ 41,  0, 1,   0,    0,   0,     0,     0,     pshp   ,"pshpa" , "pushalt(PSW);" },
{ 42,  1, 0,   0x3f, 0,   0,     0,     0,     popp   ,"popp"  , "PSW = pop();" },              // 86
{ 42,  0, 1,   0x3f, 0,   0,     0,     0,     popp   ,"poppa" , "PSW = popalt();" },             // 87


{ 1 ,  0, 0,   0,    1,   0x7,   0,     0x47,  clr   ,"clrb"  ,  "\4\3 = 0;" },                // 88
{ 1 ,  0, 0,   0,    1,   0xf,   0,     0x4f,  clr   ,"clrw"  ,  "\4\3 = 0;" },

{ 2 ,  0, 0,   0x3e, 1,   0x7,   0,     0x67,  neg   ,"negb"  ,  "\3 = -\1;" },                // 90
{ 2 ,  0, 0,   0x3e, 1,   0xf,   0,     0x6f,  neg   ,"negw"  ,  "\3 = -\1;" },

{ 25,  0, 0,   0x3e, 1,   0x7,   0,     0x47,  inc   ,"incb"  ,  "\3++;"  },         // 1++ ?                   // 92
{ 25,  0, 0,   0x3e, 1,   0xf,   0,     0x4f,  inc   ,"incw"  ,  "\3++;" },                    // 93
{ 22,  0, 0,   0x22, 2,   0x107, 0x11f, 0x47,  nrm   ,"norm"  ,  "\1 = normalize(\2);\n\2 <<= \1; " },
{ 2 ,  0, 0,   0x30, 1,   0x7,   0,     0x47,  cpl   ,"cplb"  ,  "\3 = ~\1;" },                // 95
{ 2 ,  0, 0,   0x30, 1,   0xf,   0,     0x4f,  cpl   ,"cplw"  ,  "\3 = ~\1;" },

{ 24,  0, 0,   0x3e, 1,   0x7,   0,     0x47,  dec   ,"decb"  ,  "\3--;" },                    // 97
{ 24,  0, 0,   0x3e, 1,   0xf,   0,     0x4f,  dec   ,"decw"  ,  "\3--;"  },
{ 22,  0, 0,   0x30, 1,   0x107, 0,     0x16f, sex   ,"sexb"  ,  "\3 = \1;" },
{ 22,  0, 0,   0x30, 1,   0x01f, 0,     0x17f, sex   ,"sexw"  ,  "\3 = \1;" },                // 100


{ 21,  0, 0,   1,    0,   0,     0,     0,     stc   ,"stc"   ,  "PSW_C = 1;" },                  // 101
{ 21,  0, 0,   1,    0,   0,     0,     0,     clc   ,"clc"   ,  "PSW_C = 0;" },                  // 102
{ 43,  0, 0,   0,    0,   0,     0,     0,     edi   ,"di"    ,  "interrupts OFF;" },
{ 44,  0, 0,   0,    0,   0,     0,     0,     edi   ,"ei"    ,  "interrupts ON;" },
{ 48,  0, 0,   0,    0,   0,     0,     0,     clv   ,"clrvt" ,  "PSW_VT = 0;" },
{ 47,  0, 0,   0,    0,   0,     0,     0,     nop   ,"nop"   ,  "" },

{ 45,  0, 1,   0,    0,   0,     0,     0,     bka   ,"regbk"  ,  "" },                     // 107
{ 45,  0, 1,   0,    0,   0,     0,     0,     bka   ,"regbk"  ,  ""},    //"" },
{ 45,  0, 1,   0,    0,   0,     0,     0,     bka   ,"regbk"  ,  "" },                     // 109
{ 45,  0, 1,   0,    0,   0,     0,     0,     bka   ,"regbk"  ,  "" },                     // 110

{ 46,  0, 1,   0,    1,   0x7,   0,     0,     0     ,"rombk"  ,  "" },                     // 111 bank prefix (8065)
{ 40,  0, 0,   0,    0,   0,     0,     0,     0     ,"!PRE!"  ,  "" }                      // 112 signed prefix

};


// Auto names - typically appended with address.
// items - cmd mask,  spf,  size in, size out, name
// sizes are for function and table subroutine names as opcode table
// spf derived from here for func (=2) tab (=3) and others
// Also math funcs go in here and use spf as number incrmenet (e.g. enc1, enc2)

AUTON autonames [] = {
 { 0,      0,  0, 0, ""     },          // 0 null entry
 { 0x80 ,  0,  0, 0, "Sub"     },          // 1   option OPTPROC   auto proc names (func)
 { 0x10 ,  0,  0, 0, "Lab"     },          // 2   option OPTLABEL  label names (lab)
 { 0x200,  0,  0, 0, "Func"    },          // 3   option OPTFUNC   function data names
 { 2    ,  0,  0, 0, "Table"   },          // 4   option OPTTABL   table data names

 { 4    ,  2,    7,    7, "UUYFuncLU" },       // 5   option OPTSTRS   auto names for function and table lookup subroutines
 { 4    ,  2,    7, 0x27, "USYFuncLU" },
 { 4    ,  2, 0x27,    7, "SUYFuncLU" },
 { 4    ,  2, 0x27, 0x27, "SSYFuncLU" },

 { 4    ,  2,  0xf,  0xf, "UUWFuncLU" },        // 9    word functions
 { 4    ,  2,  0xf, 0x2f, "USWFuncLU" },
 { 4    ,  2, 0x2f,  0xf, "SUWFuncLU" },
 { 4    ,  2, 0x2f, 0x2f, "SSWFuncLU" },

 { 4    ,  3,    7,    7, "UYTabLU"  },         // 13     byte table
 { 4    ,  3, 0x27, 0x27, "SYTabLU"  },
 { 4    ,  3,  0xf,  0xf, "UWTabLU"  },          //15    word table
 { 4    ,  3, 0x2f, 0x2f, "SWTabLU"  },          // 16

{ 0,      0,  0, 0, ""     },                 // 17 null entry
{ 0,      0,  0, 0, ""     },                 // 18 null entry
{ 0,      0,  0, 0, ""     },                 // 19 null entry

{ 0    ,  1, 0, 0, "Encode"  }               // sys math functions from here


// { 0    ,  4,  7,  7, "Timer"    }          // 17   possible special for timers ? (not used ).
 };




 /**************************************************
 * main global variables
 **************************************************/

// time_t  timenow;



/****************** bank layout ***************
* To make code simpler, banks are stored as +1 internally.  Max valid banks are 1,2,9,10 for bins
* A full bank of 64K is malloced to each relevant bkmap[bankno].fbuf, as 0-0xffff, for a max of 14 banks.
* Registers (< 0x400) are always mapped to Bank 9. Single bank bins are mapped completely in bank 9.
* bank nums are converted in (+1), and out (-1). Number of banks is stored as (num-1) so that a simple
* check of (!numbanks) defines single or multibank binary files
************************************************/

BANK bkmap[BMAX];               // 15 banks max.

int   numbanks;               // number of banks-1  (so single bank bin =  0)

// Binary file byte type
// info about each binfile byte - for opcode, operand, data etc.
 /* ftype values (bottom 3 bits)
 * 1 = code
 * 2 = read data
 * 3 = write data (regs and RAM) not sure yet
 * 4 = args data
 */

// malloced data

uchar  *ftype;                // data type per byte, opcode start - need this for ALL locations
//RBST *test;
RST    *regstat;              // 0x400 entries, malloced, for all registers
RPOP   *regpops;              // aligned with stackptr for holder of extra data

uchar *binfbuf;              //  binary file is read in to here (malloced)

uint  prlist [32];            // generic param list and column positions for structs



uint  tscans  = 0;            // total scan count, for safety
int   xscans  = 0;            // main loop (for investigation)
int   opcnt   = 0;            // opcode count for emulate
int   anlpass = 0;            // number of passes done so far


//int   recurse = 0;            // recurse count check

// instance holders (each hold one analysed opcode and all its operand data)

INST  cinst;                 // current (default) instance for std scans
INST  sinst;                 // search instance (for conditionals, params, etc)
INST  einst;                 // emulate instance.

// special scan blocks, outside standard scan chain

SBK   tmpscan;                // local scan block for searches
SBK   *emuscan;               // scan block (ptr) for emulate scans/runs


// SPF emuspf ???            // similar global spf holder ??? then xfer to adt ?? or not ??
// would need 4 ? to match regs ???
// or create and attach to the register and reuse ??   maybe.....
// need spf to get order of regs.

char  strbuf [256];            // for string handling




 // master exit and cleanup routines




uint get_anlpass(void)
{
    return anlpass;
}


int  get_numbanks(void)
{
return numbanks;
}


void  set_numbanks(int val)
{
   numbanks = val;
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



BANK* get_bkdata(uint ix)
{
    if (ix > 16) return 0;
    return bkmap+ix;
}



AUTON * get_autonames(void)
{
    return autonames;
}




BANK *mapbank(uint addr)
{
 // if register or single bank, force to bank 8 (9)
 // NB. bank 9 may not be valid yet for sig/bank find

  BANK *b;

  if (!numbanks || addr < 0x400)
    b = bkmap + 9;
  else
    b = bkmap + ((addr >> 16) & 0xf);

  if (b->bok) return b;
  return 0;
}


uint bytes(uint fend)
{  // size in bytes. From field end (assumes start at 0)

 fend &= 31;         //drop sign and read/write
 return (fend / 8) + 1;
}


//but these should keep LARGEST for emulate -how to do that ??

///NO, largest size in REGSTAT


void set_ftype (uint ofst, uchar val)
{
  // 1 for code start, 4 for argument start

  BANK *b;
  uchar *x;

  b = mapbank(ofst);
  if (!b) return;         // invalid

  x = b->ftype + nobank(ofst);
 // *x &= 0xf8;                  // bottom 3 bits;

//set size as 7 if not already set
 // if (!(*x)) *x |= 0x38;       // 7 << 3
  *x = val;

 // b->ftype[nobank(ofst)] = val;
}




void set_ftype_data (uint ofst, uint fend)
{
  // always for opcode data val always 2...
  BANK *b;
  uint i, end;
  uchar *x;

  b = mapbank(ofst);
  if (!b) return;     // invalid

  ofst = nobank(ofst);

  end = ofst+bytes(fend);

  for (i = ofst; i < end; i++)
    {
      x = b->ftype + i;
      *x = 2;
    }
}


uchar get_ftype (uint ofst)
{
  // for code start
  BANK *b;

  b = mapbank(ofst);
  if (!b) return 0;     // invalid

  ofst = nobank(ofst);        //testing
  return b->ftype[ofst] & 0x7;
}

uchar get_fsize (uint ofst)
{
  // for code start
  BANK *b;

  b = mapbank(ofst);
  if (!b) return 0;     // invalid

  ofst = nobank(ofst);        //testing
  return b->ftype[ofst] >> 3;
}




// address handling subrs  0xfffff  (4 bits bank, 16 bits addr in bank)


uint minromadd(uint addr)
{  // min address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b) return 0xffff;           // invalid
 return b->minromadd;
}


uint maxromadd (uint addr)
{   // max bank address for this bank
 BANK *b;
 b = mapbank(addr);
 if (!b) return 0;           // invalid
 return b->maxromadd;
}


uint val_rom_addr(uint addr)
 {
 // return 1 if valid 0 if invalid.
 // allows ROM addresses only.
 // check Code, jumps, immds

 BANK *b;

 b = mapbank(addr);

 if (!b) return 0;               // invalid
 if (addr < b->minromadd) return 0;   // minpc = PCORG
 if (addr > b->maxromadd) return 0;   // where fill starts

 return 1;        // within valid range, OK
 }



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
    if (reg > 0xff)  return 0;
    if (reg < 2)     return 2;           // zero reg = R0, R1
    if (reg > 0x11)  return 1;            // general
    if (reg < 0x10)  return 3;            // SFR
    return 4;                             // stack
  }
  return 0;        //safety
}



RST* get_rgstat(uint reg)
{

    if (valid_reg(reg) != 1) return 0;
    reg = nobank(reg);       //safety

    return regstat+reg;
}


void set_rgsize(OPS *sce)
 { //set size if arg flag set....
   RST *r;
   uint fend;

   fend = sce->fend & 31;    // sce size from op
   r = get_rgstat(sce->addr);

   if (!r)          return;
   if (!r->arg)     return;
   if (r->fendlock) return;

   if (fend > r->fend) {
       r->fend = sce->fend;
   r->fendlock = 1;}
 }



RPOP* get_rpop(uint ix)
{
 // ix &= 7;          //safety 0-7
  return regpops+ix;
}


/*
uint add_rpop(uint reg, RPOP**x)
{
  uint ix;

// use stackptr to match STACK entries

 for (ix = 1; ix < 8; ix++)
   {
     (*x) = regpops + ix;
     if (reg == (*x)->popreg) return ix;
   }

//register not found
   for (ix = 1; ix < 8; ix++)
   {
     (*x) = regpops + ix;
     if ((*x)->popreg == 0) {(*x)->popreg = reg; return ix;}
   }
   return 0;
}
*/

void clr_pop(uint reg)
{
RST *g;
RPOP *p;

g = get_rgstat(reg);
if (!g->popped) return;

p = regpops + g->popped;

p->popreg = 0;
g->popped = 0;

}


// ---------------------------- fieldwidth and masks etc



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


uint get_endval(uint fend)
{  // calc end value for function

 if (!(fend & 32)) return 0;      // not signed
 fend &= 31;
 return (1 << fend);              //sign bit at top

}


uint get_startval(uint fend)
{  // calc startval for functions
 uint ans;

 ans = get_sizemask(fend);

 if (fend < 32) return ans;

 ans >>= 1;    // one bit less for sign bit

return ans;

}




uint codebank(uint addr, INST *c)
 {
   addr = nobank(addr);
   if (valid_reg(addr)) return addr;             //register

   if (c->bank) return addr | (c->bank << 16);     // bank prefix in opcode

   return addr | (c->scan->codbnk << 16);          // standard from scan blk
 }


uint databank(uint addr, INST *c)
 {
   addr = nobank(addr);
   if (valid_reg(addr)) return addr;

   if (c->bank) return addr | (c->bank << 16);

   return  addr | (c->scan->datbnk << 16);


 }

// SFR ??

int g_byte (uint addr)
{                    // addr is  encoded bank | addr
  BANK *b;
  uchar *m;

  b = mapbank(addr);
  if (!b) return 0;        // safety

  m = b->fbuf;             // valid address, return pointer

  addr = nobank(addr);
  return (int) m[addr];
}


uint g_word (uint addr)
{         // addr is  encoded bank | addr

                     // addr is  encoded bank | addr
  BANK *b;
  uchar *m;
  uint val;
  b = mapbank(addr);
  if (!b) return 0;        // safety

  m = b->fbuf;             // valid address, return pointer

  addr = nobank(addr);

  // if (addr > 0xfffe) return 0;

  val =  m[addr+1];
  val =  val << 8;
  val |= m[addr];
  return val;
 }


/*************************************
Multiple scale generic get value.
for varibale bit fields (0-31)
Auto negates via sign (fend & 32) if required
***********************************/

int g_val (uint addr, uint fstart, uint fend)
{  // addr is encoded bank | addr
   // may need two addresses for >16 bit fields ??

  BANK *b;
  uchar *m;                     //SFR ???
  uint start, stop,i;
  uint mask, sign;

  union
  {
    int val;
    uchar t[4];
  } lmem;


  b = mapbank(addr);
  if (!b) return 0;        // safety

  m = b->fbuf;             // get mem pointer

  addr = nobank(addr);

  fstart &= 31;                    // max field (safety)
  sign = (fend & 32);
  fend &= 31;

  if (fend == fstart) sign = 0;    // no sign for single bit
  if (fstart > fend) return 0;     // safety

  stop   = fend   /8;
  start  = fstart /8;

  start += addr;
  if (start > 0xffff) return 0;
  stop += addr;
  if (stop > 0xffff) stop = 0xffff;

  lmem.val = 0;
  i = 0;

  while (start <= stop)
    {
     lmem.t[i++] = m[start++];
    }

  // do the field extract and sign

  if (fstart) lmem.val >>= fstart;                  // shift down (so start bit -> bit 0)
  mask = 0xffffffff >> (31 + fstart - fend);   // make mask of field to match
  lmem.val &= mask;                                 // and extract required field

  // set sign, reusing mask

  if (sign)
    {
     mask = ~mask;                           // flip for set negative
     sign = 1 << fend;                       // sign mask
     if (lmem.val & sign) lmem.val = lmem.val | mask;
    }

return lmem.val;

}













int sjmp_ofst (uint addr)
{
   // special 11 bit signed field.
   // negate value by sign bit
   // used ONLY for short jumps and calls


  int ofs;

  ofs = g_byte(addr) & 7;                // bottom 3 bits
  ofs <<= 8;
  ofs |= g_byte(addr+1);                 // and next byte
  if (ofs & 0x400) ofs |= 0xfffffc00;    // negate if sign bit set

  return ofs;
}



int upd_ram(uint addr, int val, uint fend)
 {
  int ans;
  uint start, stop;
  BANK *b;
  uchar *m;
  // CAN'T compare against ROM min/max for updates allowed
  // NB.'short bank' bins (RZASA) have RAM high up.
  // does not cater for special chips with memory mapping.

  b = mapbank(addr);
  if (!b) return 0;        // bank check for any strange address

  //could still be a wild address ?
// uint maxromadd (uint addr) b->maxromadd;   has bank in minromadd maxromadd

  if (val_rom_addr(addr))     return 0;                   // No writes to ROM NO good for dud address !!

  if (valid_reg(addr) == 2)   return 0;                   // or Zero regs

// if (valid_reg(addr) = 3   SFR. off to new section.
// also any special chips would need checks here

//do need SFR to stop byte overlaps with writes. Maybe need special all word array, or something

  m = b->fbuf;

  start = nobank(addr);

  ans = 0;
  stop = start + bytes(fend);                            // size in bytes

  while (start < stop)
    {
      if (m[start] != (val & 0xff)) ans = 1;       // value has changed
      m[start] = val & 0xff;
      val >>= 8;
      start++;
    }

  return ans;
 }

  // {
     //mem.t[start] = m[start];
     //start++;
   // }
 // for (i = xadd; i < xadd + sz; i++)

uint get_opstart (int forb, uint ofst)
{
  // get previous or next opcode start
  // forb = 0 for previous, 1 for next

  int cnt;
  uint end;

  end = maxromadd(ofst);
  cnt = 0;

// CAN scan THIS opcode if forwards, but not if backwards
// max possible code length is 8 (bank+fe+long index)

  while (ofst > 0 && ofst <= end)
   {
    if (forb) ofst++;  else ofst--;
    cnt++;
    if (get_ftype(ofst) == 1) break;
    if (cnt > 8) break;          // no opcodes here
   }

  if (ofst > end)  return 0;
  if (cnt > 8) return 0;

  return ofst;

}


uint find_opcode(int forb, uint xofst, const OPC **opl)
 {
  // would be simple lookup if it weren't for the prefixes.
  // d = 0 for previous, 1 for next
  int x, indx;

  *opl = 0;
  xofst = get_opstart (forb, xofst);      // previous or next

  if (!xofst) return 0;
  if (!val_rom_addr(xofst)) return 0;

  x = g_byte(xofst);
  indx = get_opc_inx(x);                       // opcode table index
  *opl = opctbl + indx;

 // Ford book states opcodes can have rombank(bank) AND fe as prefix (= 3 bytes)

   if (indx == 112)                     // bank swop - do as prefix
     {
      if (!get_cmdopt(OPT8065)) indx = 0;             // invalid for 8061
      else
       {                                // skip op & bank, 2 bytes
        x = g_byte(xofst+2);            // get true opcode
        indx = opcind[x];
       }
     }

  if (indx == 111)                      // SIGN prefix
     {                                  // skip prefix, 1 byte
      x = g_byte(xofst + 1);
      indx = opcind[x];                 // get base opcode
      *opl = opctbl + indx;
      if ((*opl)->sign)  indx += 1;     // get prefix opcode
      else indx = 0;                    // prefix not valid
     }

  if ((*opl)->p65 && !get_cmdopt(OPT8065)) return 0;     // 8065 check only
  if (!indx) return 0;
  *opl = opctbl + indx;                 // set opl for return
  return xofst;
 }

void clr_inst(INST *x)
{
       memset(x,0,sizeof(INST));       // set all zero
       x->opr[1].rgf = 1;
       x->opr[2].rgf = 1;
       x->opr[3].rgf = 1;
}


void do_one_opcode(uint xofst, SBK* ts, INST *dest)
  {
   memset(ts,0,sizeof(SBK));    // safety clear
   ts->curaddr = xofst;
   ts->start   = xofst;
   set_scan_banks(ts,0);
   do_code(ts,dest);
  }



int find_opcodes(int forb, uint addr, int num, uint n, ...)     // number back from start
 {
   // finds previous or next opcodes in list (sigix) max num is NUMPARS (16)
   // use get_pn_opcode above, may be merged in here later

   // add a check for block terminators - jumps, rets etc.
   // forb = 0 for previous, 1 for next
   // num = num of opcodes to scan (n and list is the opcodes)
   // results in sinst instance

    int xofst, ans, cnt;
    uint i;
    const OPC *opl;
    va_list ix;

    // assemble opcode sigixs into array
    va_start(ix,n);

    memset(prlist,0,sizeof(prlist));

    if (n > NC(prlist)) n = NC(prlist);

    i = n;
    while (i--)
     {
      prlist[i] = va_arg(ix, int);
     }

    va_end(ix);

    // OK -now see if we can find a match

    xofst = addr;
    if (forb)  xofst--;        // allow use of supplied xofst if going forwards, not backwards
    ans = 0;
    cnt = 0;
    // x->ofst must fall within specifed range (backwards from current). Allow for curinst
    // being fed in so <=

    while (!ans)
     {
      xofst = find_opcode(forb, xofst, &opl);
      if (!xofst) break;

      cnt ++;
      if (cnt > num) {break;}

     // 16  jump, 20 ret.
      if (forb && opl->sigix == 20) break;   // not through a RET (forwards)

      if (forb && opl->sigix == 16)
        {        //allow jumps if forwards set
          do_one_opcode(xofst, &tmpscan, &sinst);
          xofst = sinst.opr[1].addr;
          xofst--;
        }

//can do va_start ... va_arg ... va_end instead....
      for (i = 0; i < n; i++)
        {
         if (opl->sigix == prlist[i])  ans = xofst;
        }
     }

    if (ans)
      {
       do_one_opcode(xofst, &tmpscan, &sinst);
      }
    else
      {
       clr_inst(&sinst);
      }
    return ans;
 }




void set_emuargs(SBK *s, RPOP *g)
{
     //always in real scan, not emu.

      #ifdef XDBGX
        DBGPRT(1, "Mark as args %x", s->start);
      #endif

      s->getargs = 1;

     // if layers in between ??

     // if jumped to, need to mark ORIGINAL block (s->jfrom) but this is just a note

     //get link....

  //   if (g->tgtscan == s->jfrom)
   //    {
   //              #ifdef XDBGX
   //      DBGPRT(1, "JUMP TO %x ", g->tgtscan->start);
      //         #endif
  //     }

     if (g->tgtscan == s)
      {
        s->emureqd = 1;
        s->getargs = 0;
        #ifdef XDBGX
        DBGPRT(1, "Mark emu, kill args %x", s->start);
        #endif
      }
     else
      {
               #ifdef XDBGX
        DBGPRT(1, "Mark emu %x (for %x)", g->tgtscan->start, g->tgtscan->nextaddr);
        #endif
        g->tgtscan->emureqd = 1;
      }
}




uint check_args(SBK *s, OPS *o)
 {
  // called by push or equiv   stx [stkptr + inx]
  // (push = stkptr -= 2;  [stkptr] = [Rx];)

  // could do some fixed args in scan mode, but have to keep emulate for CARD etc,
  // so may as well emulate everything ?

  // proctype = 1 (std) flag emu
  // proctype = 4 emul, for doing args

  uint  ans;
  RST *g;
  RPOP *p;
  STACK *x;
  LBK *k;
  ADT *a;

  if (!o) return 0;
  if (!o->addr) return 0;
  if (!s->proctype) return 0;           // safety

  // first, difference calc, both scan and emulate

 // ans = 0;

  #ifdef XDBGX
      DBGPRT(1,"%x PUSH - In check args",s->curaddr);
  #endif

  g = get_rgstat(o->addr);                          // register status
  if (!g) return 0;
  if (!g->popped) return 0;                         // where args get attached (nextaddr)

  p = get_rpop(g->pinx);
  x = s->stack+g->pinx;


  if (s->proctype == 1)
    {
     // for scans, use 'args' in regstat, don't do actual maths
     if (!p->argscan) return 0;

  //   if (o->addr == p->argscan->popreg)
       {
        ans = g->argcnt;

        if (!ans)
        {
          // check for difference as well in case not done by increments.
          ans = o->val - nobank(x->nextaddr);
          if (!ans || ans > 32) return 0;
        }
       }

     set_emuargs(s, p);
     #ifdef XDBGX
     DBGPRT(1, "args %d to %x, target %x", ans, s->start, p->argscan->start);
#endif
     return ans;
    }


  if (s->proctype == 4)  // emulate
    {
      SBK *scn;
    //  SPF *f;

      if (!p->argemu) return 0;           //crash fix ??

     #ifdef XDBGX
      DBGPRT(1,"argemu=%x %x",p->argemu->start, p->argemu->nextaddr);
      #endif

   //   if (o->addr == p->argemu->popreg)
        {      // register matches stack entry (via reg status)
          o->val |= g_bank(p->argemu->nextaddr);
          ans = o->val - p->argemu->nextaddr;                //orig->nextaddr;
          if (ans > 32)
             {
     #ifdef XDBGX
              DBGPRT(1, "FAIL %x - %x", p->argemu->nextaddr, o->val);
#endif
              ans = 0;                            // ears has a 20 byte arg.
             }
        }

      if (!ans) return 0;         // nothing to do

     #ifdef XDBGX
   DBGPRT(1,"E%x Add args at %x size %d ", s->curaddr, p->argemu->nextaddr, ans);
   #endif

   scn = p->tgtscan;          //real scan

 //if not set as emulate then DON'T add args at this point ...stack could be wrong.

   if (scn && !scn->emureqd && !scn->getargs)
    {
             #ifdef XDBGX
      DBGPRT(1, "E Mark non emu %x", scn->start);
      #endif
      scn->emureqd = 1;                 // flag it
      scn->estop = 1;
      return 0;
    }

//spf should be attached to ptarget here

   if (get_ftype(p->argemu->nextaddr) == 4 )                 //orig->nextaddr) == 4)
     { // already flagged as args ??
          #ifdef XDBGX
       DBGPRT(1,"ZZZZ Duplicate Args %x", p->argemu->nextaddr);         //orig->nextaddr);
       #endif
  //     k = 0;
     }
   else
    {

      k = add_aux_cmd(p->argemu->nextaddr, p->argemu->nextaddr +ans-1, C_ARGS, s->curaddr);
      if (k)
        {
            if (get_cmdopt(OPTARGC)) k->cptl = 1;
         p->auxcmd = k;       //keep ref
         set_ftype(k->start,4);
        }
      else return 0;

      /*HERE need cross link to spf ???

      // link k to argsblk, or ptarget ?
      // argsblk is for k, spf is ptarget...

      // or swap argsblk to aux_cmd for further use ???


// in p->fblk ???   should be...

       f = find_spf(scn,0,0);         // get related spf in REAl scan.   YES!!! correct spf !!!
       //can be 5 or 10 ...

DBGPRT(0,"%x ", s->curaddr);

if (f) DBGPRT(0,"found %d", f->spf); else DBGPRT(0,"NOT found dfdf");

 DBGPRT(1," spf for %x ", p->tgtemu->start);        //why doesn't [0] work ??

//add link here......lbk->spf       add_link...

    //  set_ftype(p->argscan->nextaddr,4);          //orig->nextaddr, 4);         //only first one ???       //set_ftypedat ??
*/

      if (get_lasterr(CHAUX))            // && k->from == x->curaddr)
        {
                 #ifdef XDBGX
         DBGPRT(1,"ZZZZ Duplicate Args cmd %x", p->argemu->nextaddr);      //orig->nextaddr);              //this must be
         #endif
        }
      else
       {
  /*       a = get_adt(vconv(k->start),0);      // does adt already exist ?
         if (a)
           {
            a->cnt *= 2;                       //probably never true ?
           }
         else
         {
           a = add_adt(vconv(k->start),0);        //do we need to do this if reconcile later ???
           if (a) {k->adt = 1; a->cnt = ans;}

         }  */

        k->size = ans;           //adn_totsize(vconv(k->start));


//these updates should go to where the args actually go.........but they are a copy from the master caller....
// so these need to be passed upwards (emu stacks only).
// but tgtemu

        p->argemu->nextaddr += ans;                 // this might stop swop to aux link ?? was originally argblk....
        x->nextaddr += ans;                            // and stack
        x->modded = 1;                                 // mark change

       }
    }

return ans;

 }   // end emulate

return 0;

 }



void check_args_inc(SBK *s, OPS *o)
  {
      // called by calc_mult_ops when in autoinc (and indirect) ONLY
      // always via ldb from popped ragister (??)
      // note djnz and ldb/stb combination.........

    // check if register qualifies subroutine for EMULATE
    // i.e. was popped and modified....
    // only used in scanning mode. Called from upd watch.
    // still need the check at a push/stx as multibanks can do it instead.


   RPOP *p;
   RST *r;
 //  SPF *f;
 //  OPS *o;

  if (s->proctype == 1)
    {
        //no emu ?? this will probably have to change.

//     o = c->opr+4;                  // sce op for increment check.
     r = get_rgstat(o->addr);

     if (!r || !r->popped) return;       // register not popped, so ignore/exit

     p = get_rpop(r->pinx);

     if (!p->tgtscan)
      {
               #ifdef XDBGX
        DBGPRT(1,"%x !!! PTARGET IS ZERO !!!", s->start);
        #endif
        return;
      }

     #ifdef XDBGX
       DBGPRT(1,"%x pop+inc for %x", s->curaddr, p->tgtscan->start);
     #endif

     // only do the flags for scan mode

     set_emuargs(s,p);

    }


/*
if (s->proctype == 4)
 {
   f = find_spf(s,10,1);

   if (!f)
    {
      f = add_sxpf(s,10,1);             // a dummy spf for now, but with right level ??
      DBGPRT(1,"add spf 10 to %x", s->start);
    }


//check not already in list ? or keep order as more important ?


//need parallel stack here for move to the real scan blocks.... do by a get_scan ???? emulate only ?



   if (f)
   {
     f->dpars[f->argcnt] =  c->opr[3].addr;        //add destination to args list .. args list should be for caller ???
     f->argcnt++;
   }
}*/
  }



void do_rbt_check(INST *c, SBK *s, OPS *o, uint ch)
{

  // called for every possible update
  // add check for lower byte modification to invalidate whole word
  //OPS o is the write op.

  RBT *b;
  RST *l;
  uint xadd;

  if (!o->rgf) return;                     // only registers
  if (valid_reg(o->addr) != 1) return;     // no special regs

  xadd = o->addr;
  if (o->addr & 1) xadd = o->addr-1;        // word equiv in case byte update

  if (c->sigix == 13 && c->opcsub != 1) return; // not if via stw indirect

  l = get_rgstat(xadd);

  // check for changes before looking for rbase

  if (!l || l->invrbt) return;        // invalid already


  if (c->opcsub != 1)  l->rbcnt++;     // not immediate

  if (ch) l->rbcnt++;                  // value changed

    if (l->rbcnt > 2)
     {
      l->invrbt = 1;          //not valid anymore
      #ifdef XDBGX
       DBGPRT(0,"rbase R%x invalid (%x)", xadd, o->addr);
       #endif
       b = get_rbase(xadd,0);                 // is it an rbase (checks regstat)?
       if (b && !b->usrcmd) b->inv = 1;
    }

  if (l->invrbt) return;

  b = get_rbase(xadd,0);                 // is it an rbase (checks regstat)?

 // if (b)
 //   {           // entry in chain, probably user command.
                // rbase already exists, invalidate if not same value or not immediate
                // check for byte as well as word
   //   if (b->inv)     return;  // already marked as invalid

     // if (b->usrcmd)
       // {
  //        l->invrbt = 0;         //probably redundant, but safety reset....
    //     #ifdef XDBGX
    //      DBGPRT(1,"Rbase - user cmd bans invalid %x", o->addr);
    //     #endif
   //      return;
    //    }
  //  }         // end check rbase
    if (!b)           //else
     {
       // only add if ldw, stw or ad3w and immediate, not a register
       // reg must not be SFR

     if (o->addr & 1) return;                    // rbase must be a WORD

     if (c->opcsub == 1 && valid_reg(o->val) < 2 && (o->fend & 31) == 15)
      {
       if (c->opcix == 41 || c->opcix == 49 || (c->opcix == 25 && c->opr[2].rbs))
       {
         if (!valid_reg(o->val)) o->val |= (s->datbnk << 16);                 // immediates strip the bank, put it back
         add_rbase(xadd, o->val,0,0xfffff);
         l->invrbt = 0;                        //mark as valid;
       }
      }
     }


   }






void upd_watch(SBK *s, INST *c)
 {
  OPS *o;
  int ch;

  if (!s->proctype) return;

  o = c->opr + 3;                           // destination, 3 is write op

  if (!(o->fend & 64)) return;         // not a write.
  if (c->inv)  return;                 // invalid address


 ch = upd_ram(o->addr,o->val,o->fend);     // ch = 1 if changed
 do_rbt_check(c, s,o, ch);


  // write op - upd_ram returns 1 if RAM changed.
 // if (upd_ram(o->addr,o->val,o->fend)) do_rbt_check(s,o,c);

/*
#ifdef XDBGX

    if (s->proctype == 4) DBGPRT(0, "E");
    DBGPRT(0,"%05x [%x] = %x (%d)",s->curaddr, o->addr, o->val, bytes(o->fend));

    if (c->opcsub > 1)
      {
       DBGPRT(0, "[R%x", c->opr[4].addr);      // for indirect
       if (c->opcsub == 3) DBGPRT(0, "+%x", c->opr[0].addr);      // for indexed
       DBGPRT(0,"]  [%x]", c->opr[1].addr);
      }

    DBGPRT(1,0);
  #endif
*/







 }


//------------------------------------------------------------------------



/************************************************
 *  Symbols
 ************************************************/

//AUTON autonames;


SYM *new_autosym (uint ix, uint ofst)
{

// add auto sym with address
// all of these are whole read symbols

  SYM *xsym;
  uint c;

  if (anlpass >= ANLPRT) return 0;

  c = ix & 0xff;

  if (c >= NC(autonames)) return NULL;              // safety check

  if (!get_cmdopt(autonames[c].mask)) return NULL;  // autoname option not set

  // make name first

  c =  sprintf(strbuf,   "%s", autonames[c].pname);
  // add address
  c += sprintf(strbuf+c, "_");
  if (numbanks)  c += sprintf(strbuf+c, "%x", (ofst >> 16) -1);   // bank if reqd
  sprintf(strbuf+c, "%04x", nobank(ofst));

  c = 7 | (ix & 0xf00);    // C_USR, C_SYS, C_REN
  xsym = add_sym(strbuf,  ofst, 0, c, 0,0xfffff);     // add new (read) symbol, or rename

  if (xsym && xsym->usrcmd) return xsym;               // can't change a cmd sym

 /* if (ix > xsym->xnum)
    {     //only if higher level autonumber
      new_symname (xsym, strbuf);
      xsym->xnum = ix;
    }  */

  return xsym;
}



/*  separate table analysis ??
 *
 * consider
 *
 * 0  1  2  3  4  5  6  7
 * 8  9  a  b  c  d  e  f
 * 10 11 12 13 14 15 16 17
 * 18 19 1a 1b 1c 1d 1e 1f
 *
 * start at 9  ( x = rowsize + 1) through to e  (= x+rowsize-2)
 * compare
 * x -> x-rowsize-1 (0)
 * x -> x-rowsize   (1)
 * x -> x-rowsize+1 (2)       ...loop 1 ??
 * x->  x-1         (8)
 * x-> x + 1;       (a)
 * x-> x+rowsize-1  (10)
 * x-> x+rowsize    (11)
 * x-> rowsize + 1  (12)    ..loop 2
 * to get a 'surface' like cell compare
 *

*/




int do_table_sizes(uint start, int scols, uint *apnd, uint maxend)
{
  // assume a table for now...............
// so a rectangle of numbers...
// last byte may be a filler.
// last byte/word by command may or may not be correct end....
// start should be OK though.

// how to 'trim' the sizes ???


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


if (start == 0x9d4db)
{
    DBGPRT(0,0);
}


#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"old do data table sizes for %x-%x cols %d, max %x", start, apend, scols, maxend);
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


// if score = 0 then default to common sizes.

if (ansrow > 20 || ansscore == 0) ansrow = 20;

 *apnd = (ansrow*anscol);


 #ifdef XDBGX

  DBGPRT(1,"LOWEST %dx%d TOT %d end=%x",anscol, ansrow, ansscore, start+ *apnd );
#endif






 return ansrow;
}





int score_cell(uint start, uint fend, uint rowsize)
{

 /* from 'x' compare adjacent cells for a table
 * x -> x-rowsize-1
 * x -> x-rowsize
 * x -> x-rowsize+1    loop 1
 * x->  x-1
 * x-> x + 1;          loop 2
 * x-> x+rowsize-1
 * x-> x+rowsize
 * x-> rowsize + 1     loop 3
 *
 */


// try squareing the values to get rid of negatives ??

  int baseval, val, adjval, score;
  uint i;
  score = 0;

  val = g_val(start,0, fend);
  baseval = (val*val) & get_sizemask(fend);

  //row above, 3 cells
  for (i = start-rowsize - 1; i < start-rowsize+2; i++)
    {
      val = g_val(i,0,fend);
      adjval = (val*val) & get_sizemask(fend);

      if (baseval > adjval) score += baseval - adjval;
      else score += adjval-baseval;
    }

   // current row, 2 cells, skip start
  for (i = start-1 ; i < start+2; i+=2)
    {
      val = g_val(i,0,fend);
       adjval = (val*val) & get_sizemask(fend);
      if (baseval > adjval) score += baseval - adjval;
      else score += adjval-baseval;
    }

   //row below, 3 cells
  for (i = start+rowsize - 1; i < start+rowsize+2; i++)
    {
      val = g_val(i,0,fend);
      adjval = (val*val) & get_sizemask(fend);
      if (baseval > adjval) score += baseval - adjval;
      else score += adjval-baseval;
    }

  return score;
}





int score_table (uint start, uint colsize, uint rowsize, uint fend)
{

 int score, tries;
 uint cell;
 uint end;

// uint colno;
tries = 1;

  #ifdef XDBGX
  DBGPRT(0, "new table %x, %d X %d", start, rowsize, colsize);
  #endif
  score = 0;

  end = start + colsize*(rowsize-1) -1;          // last row -1 , last column -1.  Not last row or last col
  start += colsize+1;                            // 2nd row, 2nd column.  Not first row or first col

 for (cell = start;  cell < start+colsize; cell++)
   {                           // for each inside row...
     if (cell >=end) break;
     for (cell = start; cell < start+colsize-2; cell++)
     {      // for each inside column
           score += score_cell(cell, fend, rowsize);
           tries++;
     }
     start += colsize;
   }

   if (score < 0)  score = -score;
   score = score / tries;

 #ifdef XDBGX
   DBGPRT(1, " score = %d, tries %d", score, tries);
   #endif
return score;
}












int new_table_sizes(uint start, uint scols, uint fend)
{
  // a 'surface' in excel terms...........

uint colsize, rowsize;



#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"do NEW data table anal for %x cols %d fend %x", start, scols, fend);
 #endif


// allow for 3 to 32 rows and cols max  (1024 cells max)


if (scols)
  {        //assume scols is valid, just look for end
     for (rowsize = 4; rowsize < 32; rowsize++)
        {            //  up to 32 rose
          score_table(start, scols, rowsize, fend);
        }


  }
  else
  {
     // no rowsize


  for (colsize = 3; colsize < 32; colsize++)
    {
      for (rowsize = 3; rowsize < 32; rowsize++)
        {            //  3 x 3 up to 32 x 32
          score_table(start, colsize, rowsize, fend);
        }
    }
  }
return 0;
}



SBK *make_emu_stack(uint addr, SBK *caller)
{
    // rebuild stack by copying SBK blocks to emu chain
    // return base. should work for both chains ?

  SBK *base, *x;
  STACK *stk;
  int i;

//do i need a global callr too ?
  base = add_escan(addr, caller);            // copy 'base' SBK as new call from caller.

//mark 'end of stack'  somehow ???  but this is going to go backwards anyway....




  for (i = 0; i < 8; i++)
   {
     stk = base->stack + i;
     if (stk->type == 1)
      {
        x = get_scan(CHSCAN,stk->start);
        add_escan(0, x);        // copy entry unchanged from scan to emulate chain
  //      stk->sc = x;                     // replace stk ptr with new block
      }
     // keep everything else with no changes.
   }

return base;
}




void end_scan (SBK *s)
{
 // SUB *b;

  if (!s->proctype || get_cmdopt(OPTMANL) ) return;

  s->stop = 1;
 // s->force = 0;
  if (s->nextaddr > s->start)
    {
     s->nextaddr--;  // end of this block is (nextaddr-1)
    }


// SPF and emulate here ??

// if spf,  check all jumps TO this block (completed scans) ....
// (by this sig, by this pop/push ?)  similar to func/tab data scan

 // #ifdef XDBGX
 //  else  DBGPRT(1," Start > End");
 // #endif

//but emulate idea needs different logic to do here...

/*
{  // TEST  // do emulate HERE, before nextaddr and args and dodgy opcodes.....

uint agc;
LBK *k;

           agc = get_ftype(s->nextaddr);

             if (s->emulreqd && agc != 4)     // could be 2 && !caller->emulating)
               {
                 saveddbnk = datbnk;
                 s->endaddr = s->curaddr;
                 emulate_blk(s);
                 datbnk = saveddbnk;
               }

             if (agc == 4)
               {
                while ((k = get_cmd(CHAUX,s->nextaddr, C_ARGS)))
                  {
                   s->nextaddr += k->size;
                  }


               }
} */

//s->popreg = 0;
//s->argcnt = 0;          //safety


  #ifdef XDBGX
//  if (s->proctype) redundant see above
 // {
     if (s->proctype == 1) DBGPRT(0,"End scan");
     if (s->proctype == 4) DBGPRT(0,"End Escan");

   DBGPRT(0," %x at %x", s->start, s->nextaddr);
   if (s->inv)  DBGPRT (0," Invalid Set");
   DBGPRT(2,0);
//  }
  #endif



}






/* print needs full calc, but decode addr only needs the address
 * part for argument calcs. so this is a cut down do adt calc
 * seems to work OK.....
 */


int decode_addr(ADT *a, uint ofst)
{
    // not quite a dummy............
  int val;
  FKL *l;
 //  MATHN *n;         // DEBUG
  MATHN *n;
  MHLD  *clchdr;   // calc holder

   l = get_link(a, 0, 0);            //l = get_link(a, 0, 0);       // fwd link adt to mathx

   if (l) n = (MATHN*) l->keydst;                       // this is first math term

   // no attached calc, so return plain int value from ADT ofst (default)
   // in calc holder

   else                         // if (!l)
     {
      val  = g_val (ofst, a->fstart, a->fend);
      return val;
     }


   // calc attached, get and setup details

   clchdr = do_tcalc(n, 0, a, ofst);            // x->fid, 0, a, ofst);        // calc - may reset print mode.

   // do tcalc returns an mhld struct

   val = clchdr->total.ival;
   mfree(clchdr, sizeof(MHLD));          //free calc holder
   return val;
}


void avcf (SIG *z, SBK *blk)
  {
    // tasklist from stkptr2 retei pushes stackpointer UP

    int start, addr, val, bk;
    LBK *s;
    ADT *c;

    if (!blk->proctype) return;
    addr = z->v[4];

    bk = (z->v[7] >> 4) + 1;      //bank from the R11

    // if (!bk) ... bk = default databank ??

    start = addr | (bk << 16);
    addr = start;                                //  bk = g_bank(z->start);
    bk = g_bank(blk->start);                     // reset to code bank

   #ifdef XDBGX
    DBGPRT(1,"in avcf, start = %05x", addr);
   #endif

    while (1)
     {
        val = g_word(addr);
        val |= bk;
        if (!val_rom_addr(val)) break;
        add_scan(blk->chn,val, 0, J_SUB, blk);            //blk);                // was zero
        addr += 2;
     }

    s = add_cmd (start, addr, C_VECT, z->start);

    if (s)
     {
     // do bank here......
       c = add_adt(vconv(s->start),0);    // only one
       if (c)
         {
           s->adt = 1;
           c->bank = bk >> 16;                 // always add bank
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

int find_list_base(INST *c, uint num)           //uint ix, uint num)
{
    // try to find a list 'base address'.
    // expect an add of an immediate value
    // allow for ldx to swop registers          e.g. A9L

//..ix is always 4 ??

      int ans, xofst;
      uint reg;

      ans = 0;
      reg = c->opr[4].addr;              //ix
      xofst = c->ofst;

      while (xofst)
         {         // look for an add,ad3w or ldx - max 'num' instructions back
          xofst = find_opcodes (0, xofst, num, 2, 6, 12);

          if (xofst)
            {          //check word ?
             if (sinst.sigix == 12 && sinst.opr[2].addr == reg)
              {  // ldw
                if (!sinst.opcsub) // from another register, change register
                reg = sinst.opr[1].addr;
                if (sinst.opcsub == 1)    // immediate
                  {
                  ans = sinst.opr[1].addr;     // addr is val+databank
                  if (val_rom_addr(ans)) return ans;
                  }
              }

             if (sinst.sigix == 6 && sinst.opr[3].addr == reg)         // sinst.wop].addr == reg)   //2].addr == reg)
              {  // ad2w, ad3w
               if (sinst.opcsub == 1)
                 {                              // found add for register
                  ans = sinst.opr[1].addr;     // addr is val+databank
                  if (val_rom_addr(ans)) return ans;
                 }
              }
            }
         }
  // but even if not found, may still have a valid address already in register.
 // if (xofst)
  //  {
  //  ans = c->opr[ix].val;
  //  if (val_rom_addr(ans)) return ans;
  //  }
//  else
 ans = 0;

return ans;
}

void match_opnd(uint xofst, const OPC *opl, SFIND *f)
{
   OPS *s;
   uint m, ix;

   for (ix = 0; ix < 2; ix++)
    { // for each possible answer/register general reg only
     if (valid_reg(f->rg[ix]) == 1 && !f->ans[ix])
      {

       if (opl->sigix == 12)
        {  // ldw - get operands
         do_one_opcode(xofst, &tmpscan,&sinst);
         s = sinst.opr + 3;            //sinst.wop;       // where the write op is

         m = reg_eq(f->rg[ix], s->addr,s->fend);

         if (m)
           {        // found register [2] = [1] for ldx
            if (sinst.opcsub == 1)
              {   // immediate, look for wop override
               f->ans[ix] = sinst.opr[1].addr;               //val;
               #ifdef XDBGX
                 DBGPRT(0," Found imm R%x = %x [%x]", f->rg[ix], f->ans[ix], xofst);
               #endif
              }
            else
            if (!sinst.opcsub)
              {  // ldx from another register, change register, but NOT
                #ifdef XDBGX
                  DBGPRT(0," Chg R%x = R%x [%x]", f->rg[ix], sinst.opr[1].addr, xofst);
                #endif
                f->rg[ix] &= 1;                       // is it odd ?
                f->rg[ix] += sinst.opr[1].addr;       // replace with new reg (may be odd)
              }          // end if match reg
           }


        }    //end ldx
       else

       if (opl->sigix == 6 && opl->nops == 3)
         {      // ad3w (ad2w ??)
          do_one_opcode(xofst, &tmpscan,&sinst);

          if (sinst.opr[3].addr == f->rg[ix])
           {
            if (sinst.opr[2].rbs)
             {     // ad3w with an rbase.
               if (sinst.opcsub == 1)
                 {  // immediate.  op[3].val is correct as bank in rbase.
                   f->ans[ix] = sinst.opr[3].val;
                   #ifdef XDBGX
                      DBGPRT(0," ad3w R%x = %x at %x", f->rg[ix], f->ans[ix], xofst);
                   #endif
                 }
               else
                 {
                  // if it's a register, or indirect, then need to find a list base for multiple structs (tabs or funcs)
                  // this might be widened to non_rbase ??
                  // and check spacing for tabs ??? op[4] is orig register, same subr as vects
                  f->lst[1] = sinst.opr[2].val;                // this is rbase address for possible list start
                  m = find_list_base(&sinst,15);            //4,15);
                  if (m)
                    {
                     f->lst[0] = m;
                     f->ans[ix] = m;       // for stopping the search
                     #ifdef XDBGX
                       DBGPRT(0," Found LIST R%x -> %x [%x]", f->rg[ix], m, xofst);
                     #endif
                    }
                  else
                   {
                  #ifdef XDBGX
                      DBGPRT(0," ad3w unknown R%x at %x", f->rg[ix], xofst);
                   #endif
                   }
                 }
             }
            else
             {         // not rbase
              #ifdef XDBGX
               DBGPRT(0," ad3w !rbase R%x at %x", f->rg[ix],  xofst);
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

  ADT *a;
  LBK  *k;
  //AUTON *anm;

  //anm = autonames+f->spf;

  #ifdef XDBGX
  DBGPRT(1,"Check Func %x, I %x, O %x", f->ans[0], f->spf->fendin, f->spf->fendout);
  #endif



  fendin = f->spf->fendin;      //local as it may change
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
       #ifdef XDBGX
         DBGPRT(1,"func %x Invalid start value %x, expect %x", f->ans[0], val, startval);
       #endif
       return ;
      }
    }

    rowsize = bytes(fendin) + bytes (f->spf->fendout);

    k = add_cmd (f->ans[0], f->ans[0] + rowsize-1, C_FUNC, 0);

    if (k && !k->usrcmd)
      {                                     // cmd OK, add size etc (2 sets for func)

       new_autosym(3,f->ans[0]);            // auto func name, can fail
       k->size = rowsize;                   // size of one row

       a = add_adt(vconv(k->start),0);
       k->adt = 1;
       a->cnt = 1;
       a->fend = fendin;                           // size, word or byte
       set_pfwdef(a);
       a->dptype = DP_DEC;                         // default decimal print

       a = add_adt(a,0);
       a->cnt = 1;
       a->fend = f->spf->fendout;                 // size out
       set_pfwdef(a);
       a->dptype = DP_DEC;                         // default decimal print
      }

 }







void fnplu (SIG *s,  SBK *blk)
 {
    // func lookup subroutine
    // set up subroputine only - func added later by jump trace

 //  SUB *subr;
   SPF *f;
   int size;
   uint i, taddr;
   uint xofst, imask, omask, isize, osize;

   if (!s || !blk)     return;
   if (!blk->proctype) return;
   if (!s->v[3])       return ;         //func sign holder

   add_link(CHSCAN,blk,CHSIG,s,1);    // link sig to scan block

   f = find_spf(vconv(blk->start), 2, 0);
   if (f)
      {       // already processed - but could add the table here............

       #ifdef XDBGX
       DBGPRT(1,"fnplu Done %x", blk->start);
       #endif
        return;
      }

   add_subr(blk->start);        // ok if already exists
   size  = s->v[2]/2;           // size from sig (unsigned) as BYTES

   #ifdef XDBGX
     DBGPRT(1,"in fnplu for %x", blk->start);
   #endif

      taddr = g_word(s->v[4]);
           #ifdef XDBGX
       DBGPRT(1,"func = %x", taddr);
#endif
   taddr = s->v[4];

//   blk->logdata = 0;        // no data to collect (it's where sig is found)
  // #ifdef XDBGX
 //    DBGPRT(1,"set nodata %x", blk->start);
 //  #endif

   size = (size*8) -1;               // convert to fend style

//these are bit numbers now - need new conversion

   osize = (s->v[10] & 0x1000000) ? 0 : 32;              // signed out if bit SET, 1000000 = signed if NOT set
   isize = (s->v[11] & 0x1000000) ? 0 : 32;              // signed in  if bit SET, 1000000 = signed if NOT set
   osize |= size;
   isize |= size;

   omask = 1 << (s->v[10] & 0xf);                         // bit mask value, sign out flag
   imask = 1 << (s->v[11] & 0xf);                         // bit mask value, sign in  flag

// do spf here, then the next bit, and must check for all jumps TO the sig as well, and probably
// indirectly/recursively BUT WHAT IF JUMP TO DOES THIS BLOCK FIRST, or runs through (AA) ??

// look for OR opcodes
// also look for changed register for address holder (ldx or stx)
// this bit must happen for jumps to as well...........(in subr)

   xofst = blk->start;             //substart;
   while (xofst)
    {


     // look for an OR, LDX, STX in 22 opcodes forwards front of subr
     xofst = find_opcodes (1, xofst, 22, 3, 9, 12, 13);

     if (xofst >= s->start) break;             // && xofst <= s->end) break;
     if (!xofst)    break;

        if (sinst.sigix == 9 && sinst.opr[2].addr == s->v[3])
          {
           #ifdef XDBGX
             DBGPRT(1,"find OR at %x", sinst.ofst);
           #endif
           if (omask & sinst.opr[1].val)  osize ^= 32;    // flip sign out if match;
           if (imask & sinst.opr[1].val)  isize ^= 32;    // flip sign in  if match
          }

    //    if (!sinst.opcsub && sinst.sigix == 12 && sinst.opr[2].addr == taddr)
     //      {
             // this may also work for args, as they must be loaded via ldx ....
     //        taddr = sinst.opr[1].addr;
     //      }

        xofst++;                                      // skip to next instruction

    }

  //set xfunc here
   // set function type by size match, in and out  5 - 12 in aunames table.

   for (i = 5; i < 13; i++)
     {
      if (autonames[i].fendin == isize && autonames[i].fendout == osize)
        {
         new_autosym(i|C_SYS|C_REN,blk->start);          // add subr name acc. to type

         f = add_spf(vconv(blk->start),2, 1);             // add type 2 level 1
         f->fendin  = isize;
         f->fendout = osize;
         f->argcnt = 1;
         f->pars[0].reg = taddr;                // size and address reg

         #ifdef XDBGX
              DBGPRT(1,"add spf 2 for ix %d (%x)", i, blk->start);
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
   DBGPRT(1,"SET TAB Add %x cols %x Rows %x inx %x ss %d", ofst, cols, rows, f->spf, f->spf->fendout);
   #endif

  if (cols < 2 || cols > 32) cols = 1;

  xend = ofst;
  xend += (rows*cols)-1;

  k = add_cmd(ofst, xend, C_TABLE, 0);

  if (k && !k->usrcmd)                       // in case tab by user cmd....
    {
     if (get_cmdopt(OPTSTCA)) k->argl = 1;              // arg layout by default
     new_autosym(4,ofst);                 // auto table name, can fail
     a = add_adt(vconv(k->start),0);            // just ONE adt
     k->size = cols;                      // size for one line ( = cols)
     k->adt = 1;
     a->cnt = cols;
     a->dptype = DP_DEC;                         // default decimal print

     a->fend = f->spf->fendout;   // includes sign
     set_pfwdef(a);

    }

  // probably need some more checks, but simple version works for now...

}



void tbplu (SIG *s,  SBK *blk)

{
  //works for both old TBLU  and new (TBLU2) sigs
  // for older table sig have to find interpolate sig to sort out sign
  // s->v[10] and s->v[15] copied in, and size will be in [2] for word tables

   uint xofst, taddr, tcols, osign, omask;
   SIG *t;
   SPF *f;
   AUTON *n;

   if (!s || !blk)     return;
   if (!blk->proctype) return;
   if (!s->v[3])       return ;


   f = find_spf(vconv(blk->start), 3, 0);
   if (f)
      {       // already processed
       #ifdef XDBGX
       DBGPRT(1,"tbplu Done %x", blk->start);

       #endif
        return;
      }

   add_subr(blk->start);  // ok if it already exists substart

   #ifdef XDBGX
     DBGPRT(1,"In tbplu ");
   #endif



   add_link(CHSCAN,blk,CHSIG,s,1);    // link sig to scan block

//need SIZE in here, just like funclu. later bins have WORD tables.

   osign = 0;
   omask = 0;

   // In tablu lookup, sign flag is in the INTERPOLATE subroutine
   // subr call address saved in s->v[1] so look for tabinterp sig there.

   taddr = s->v[1] | (blk->codbnk << 16);            // subr call
   t = get_sig(taddr);

   if (!t)
     {
        if (!s->v[0])     t = do_sig(PATBINTP,taddr);          //scan for interpolate sig but needs to be different !
        if (s->v[0] == 2) t = do_sig(PATWINTP,taddr);
     }

   if (t)
     {           //   found tab signed interpolate
      #ifdef XDBGX
        DBGPRT(1," INTR SIGN Flag = %x (%x)", t->v[3], t->v[10]);
      #endif
      s->v[10] = t->v[10];          // copy bit mask
      s->v[15] = t->v[3];           // and register
     }

   taddr = s->v[4] ;            // table address register
   tcols = s->v[3] ;            // num cols in this register

   osign = (s->v[10] & 0x1000000) ? 1 : 0;         // JB or JNB
   omask =  1 << (s->v[10] & 0xf);                 // convert bit to mask

   xofst = blk->start;
   while (1)
    {    // look for an OR, LDX, STX from front of subrup to sig start

     xofst = find_opcodes (1, xofst, 22, 3, 9, 12, 13);

     if (xofst >= s->start) break;
     if (!xofst) break;

     if (sinst.sigix == 9 && sinst.opr[3].addr == s->v[15])
          {                       // found register to match 't'
            #ifdef XDBGX
               DBGPRT(1,"find OR at %x", sinst.ofst);
            #endif
            if (omask & sinst.opr[1].val)  osign ^= 1;    // flip signout to match
          }

    //    if (!sinst.opcsub && sinst.sigix == 12)     ??  does a bin use LDX ??
      //     {
               // ldx reg-reg this may also work for args, as they must be loaded via ldx ....
               // opcsub = 0 and set opr[1].addr
        //     if (sinst.opr[2].addr == taddr) taddr = sinst.opr[1].addr;
        //     if (sinst.opr[2].addr == tcols) tcols = sinst.opr[1].addr;
          // }
        xofst++;                                      // skip to next instruction

    }

//   blk->logdata = 0;        // no data logging (it's where sig found)
 //  #ifdef XDBGX
 //     DBGPRT(1,"set nodata %x", blk->start);
 //  #endif

   // spf 13->16 in aunames table
   // spf = 13 + osign;        // + size....

   f = add_spf(vconv(blk->start),3, 1);

   xofst =  13 +s->v[0] + osign;       //table+size+sign;

   new_autosym(xofst | C_SYS | C_REN, blk->start);            // add subr name acc. to type

   n = autonames + xofst;

   f->fendin  = n->fendin;
   f->fendout = n->fendin;
   f->argcnt = 2;
   f->pars[0].reg = taddr;                        // reg holding address
   f->pars[1].reg = tcols;                        // reg holding cols

   #ifdef XDBGX
     DBGPRT(1,"tab spf=%d (%x)", f->spf, blk->start);       //substart);
   #endif

}




void encdx (SIG *s,  SBK *blk)
{

  uint enc, reg, i, j;
  RST *r;       //, *z;
  RPOP *p;
  SPF *f;
  LBK *k;
  DPAR *d;
  MATHN *n;
  FKL *l;

  if (!blk || !s) return;
  if (blk->proctype != 4) return;        //not emulating

  #ifdef XDBGX

   DBGPRT(0,"in E encdx");
   DBGPRT(0," for R%x (=%x) at %x", s->v[8], g_word(s->v[8]), blk->start);  //curaddr);
   DBGPRT(1,0);
  #endif

//make this an spf too ......

   enc = s->v[0] ;             //->vflag + 2;
   if (enc == 4 && s->v[3] == 0xf)  enc = 2;            // type 2, not 4
   if (enc == 3 && s->v[3] == 0xfe) enc = 1;            // type 1, not 3

 #ifdef XDBGX

   DBGPRT(0,"enc = %d", enc);
   DBGPRT(0," for base R%x", s->v[5]);  //curaddr);
   DBGPRT(1,0);
  #endif

  if (s->v[9])
  {
     reg = s->v[9];
     if (s->v[1] == 2) reg = g_word(reg);     //is this always indirect ??
     #ifdef XDBGX
        DBGPRT(1,"encdx R%x -> R%x", s->v[9], reg);
     #endif

  }
  else reg = s->v[8];


  l = get_link(s,0,1);                 // get math link to signature

  if (!l)
   {           // no ? add one.
    n = add_encode(CHSIG, s,enc, nobank(s->v[5]));  // add calc if not there
    #ifdef XDBGX
       DBGPRT(1,"add link calc -> sig");
    #endif

   }
  else n = (MATHN*) l->keydst;


 // reg = nobank(reg);              //redundant ??
  r = get_rgstat(reg);

   if (r && r->sreg)
     {       // it is an arg ....  mark rg status as encoded first
      #ifdef XDBGX
        DBGPRT(1,"set enc %d for R%x", enc, reg);
      #endif
      r->fend = 15;
     }

// z = get_rgstat(r->sreg);           //get details from source register

//if (!z) return;
// n =
//put one enc calc here, then link it to register args as marker ???
//use v[15] ??

  //  must find it in the spf.....this could become separate subr.......

  //add enc calculate to sig instead and mark it. HERE instead ??? attach it to the sig ???
  //how then to get it to the param...
 //(ADT *a,uint type, uint reg)
  //    n = add_encode(CHSIG, s,enc, nobank(s->v[5]));  //chain, fid, type, register

  //   #ifdef XDBGX
  //      if (!n) DBGPRT(1,"enc calc fails for R%x", reg);
  //    #endif





  for (i = 0; i < 8; i++)
    {
      p = regpops + i;          //z->pinx;           //doesn't work for 'remote' encoders after push/pop
      f = p->fblk;

#ifdef XDBGX
      if (!f)
      {
          DBGPRT(0,"NOSPFNOSPF");
      }
#endif



      if (f)
      {
  //    f->sigaddr = blk->curaddr;               //log for maths func
         for (j = 0; j < f->argcnt; j++)
         {
           d = f->pars+j;
           if (d->reg == reg)
             {                         // this works for 3695
                                       //


               k = p->auxcmd;        //        x = (SBK*) f->fid;  get_auxcmd
               if (k)
               {
                        #ifdef XDBGX
                   DBGPRT(1, "FOUND R%x in spf, block %x", reg, k->start);
                   #endif
               }


               // d->spf                 has spf attached.

               //attach link from encode to d (fpars+j) ??

               add_link(CHSPF,f, CHMATHN,n,1);

               d->calc = 1;   // add_link

                  #ifdef XDBGX
                DBGPRT(1,"add link calc -> spf");
              #endif



          //     d->enctype = enc;
          //     d->encbasereg = nobank(s->v[5]);

               //s->curaddr ?/ to combine enc maths...
             }
         }
      }
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

  if (!blk->proctype) return;
  if (x->done) return ;

 #ifdef XDBGX
  DBGPRT(1,"in ptimep");          //move this later
 #endif

  x->v[0] = g_word(x->v[1]);

 // size in v[14].  do a 'find end' for the zero terminator
 // cmd size (Y or W) is (2 or 4)= byte  (3 or 5)=word

  //start = databank(x->v[0],0);
  start =  x->v[0] | (blk->datbnk << 16);


   z = x->v[14];

   #ifdef XDBGX
    DBGPRT(0,"Timer list %x size %d", x->v[0], z+3);
    //if (s) DBGPRT(0," Sub %x", s->addr);
    DBGPRT(1,0);
   #endif

  // val = 1;
   a = 0;

 // should also check for next cmd....and put names in here....

   ofst = start;

   while (ofst < maxromadd(ofst))
    {
      val = g_byte(ofst);
      if (!val) break;
      ofst += z;
      if (val & 1) ofst+=3; else ofst+=1;
    }

    k = add_cmd(start, ofst, C_TIMR, 0);      // this basic works, need to add symbols ?....

// cnt = 0-31 for type, with fstart/fend for size ???









    if (k) a = add_adt(vconv(start),0);                   // only one (for now)

    if (a) {a->fend = (z*8) -1;  a->cnt = 1;


// a->fnam = 1;
}

  x->done = 1;

}


/*
void ptimep(SIG *x, SBK *blk)
{
    // don't have to have a subr for this to work, so can do as
    // a preprocess, but logic works as main process too (but only once)

  LBK *k;
  ADT *a;
  uint start;
  int val,z;
  uint ofst;

  if (blk->proctype !=  1) return;                //!blk->scanning) return;
  if (x->done) return ;

 #ifdef XDBGX
  DBGPRT(1,"in ptimep");          //move this later
 #endif

  // x->v[0] = databank(g_word(x->v[1]),0);

  x->v[0] = g_word(x->v[1]) | (blk->datbnk << 16);

 // size in v[14].  do a 'find end' for the zero terminator
 // cmd size (Y or W) is (2 or 4)= byte  (3 or 5)=word

   start = x->v[0];
   z = x->v[14];

   #ifdef XDBGX
    DBGPRT(0,"Timer list %x size %d", x->v[0], z+3);
    //if (s) DBGPRT(0," Sub %x", s->addr);
    DBGPRT(1,0);
   #endif

 //  val = 1;
   a = 0;

 // should also check for next cmd....and put names in here....
 //add sym timer_list at start...
   add_sym("Timer_List",start,0, 7+C_SYM,0,0xfffff);




   ofst = start;

   while (ofst < maxromadd(ofst))
    {
      val = g_byte(start);
      if (!val) break;                         //terminator hit
      ofst = start + z;
      if (val & 1) ofst += 2;
      k = add_cmd(start, ofst, C_STCT, 0);      // this basic works, need to add symbols ?....

      if (k)
        {
         a = add_adt(vconv(start),0);
     //    if (start > x->v[0]) k->nl = 0;     // clear newline from cmd
        }
      if (a)
        {
            //first is a byte flag set by default;
            k->adt = 1;
            a = add_adt(a,0);
            if (a)
            {
             a->fnam = 1;
             if (z == 2) a->fend = 15;          //otherwise single byte, = 7
             if (val & 1)
               {
               a = add_adt(a,0);
               if (a)
                 {
                     a->cnt = 2;       //two bytes, but first one is a mask !!
               //   a->fend = 15;  // word sized, a cheat as it's 2 bytes
         //         a->dsb = 2;
                  a->fnam = 1;
                 }
               // a = add_adt(&chadnl,k->start,4); a->fnam = 1;
               }
            }
        }
      start += adn_totsize(vconv(start));
     }

  x->done = 1;
  k->end += 1;
  k->term = 1;       // add terminator to last struct
}

*/


int check_sfill(uint addr, uint cnt)
{
  int val;
  uint i;
  // cnt or more repeated values cannot be code....

  val = g_byte(addr);

  // if (val == 0xff) return 1;       //no nops ??  NO !!
  if (cnt < 2) cnt = 2;

  for (i = 1; i < cnt; i++)
   {
     if (g_byte(addr+i) != val) break;
   }
 if (i < cnt) return 0;

 return 1;
}




/*
void build_fake_stack(SBK *s)
{

 // build a fake stack from caller list of s.
 // from the stack point of view, a pushp will always be
 // Higher (= older) than its call out.
 // only for emulate mode

  int i;
 // SBK *x;
  FSTK *c, *d;

  c = emulstack;
  d = scanstack;


   memset(c, 0, sizeof(emulstack));

   s = s->caller;

   i = 0;

   while (i < STKSZ && s)
      {
    //    if (!s->pop ped)
        {
         c[i].origadd = s->nextaddr;
         c[i].newadd = s->nextaddr;
         c[i].psw = 0;
         c[i].type = 1;

        if (d[i].origadd == c[i].origadd && d[i].pop ped)  c[i].pop ped = 1;  // temp fixup for A9L, seems to work ??


         if (s->subr && s->subr->php)
          {
           i++;
           c[i].type = 4;
           c[i].origadd = s->nextaddr;
           c[i].newadd = s->nextaddr;
          }
        }
        s = s->caller;
        i++;
      }
          #ifdef XDBGX
DBGPRT(1,"Build fake");
DBG_stack(s, c);

#endif
}
*/







void do_signatures(SBK *s)
 {
  SIG *g;          //was s->curaddr

  if (!s->proctype) return;

  if (get_ftype(s->nextaddr) == 1)                // code, has this been scanned already ?
    {
       g = get_sig(s->nextaddr);
    }
  else
    {
     g = scan_sigs(s->nextaddr);               // scan sig if not code scanned before (faster)
    }

 do_sigproc(g,s);           //call signature handler

 }

/*
void fix_args(LBK *l, SBK *s)
{
// check argument sizes and amend
// first draft.....called when args are skipped
// to ensure all processing has been done

  RST *r;
  LBK *k;
  ADT *a;
  MNUM p1, p2;
 // MATHN *n;
 // MATHX *c;
  void *fid;
  int i, size;

  #ifdef XDBGX
    DBGPRT(1,"\nFIX args");
  #endif



 for (i = 0; i < EMUGSZ; i++)
 {
   k = emuargs[i];            // next args command (LBK)

   if (k && k->start == l->start)
   {
    size = k->size;

    fid = l;

    while ((a = get_adt(fid,0)))
     {          // for each level
   //   r = get_rgstat(a->dreg); // find register

      if (0)                  //r)
        {
            #ifdef XDBGX
         DBGPRT(0,"R%x match %x", r->reg, l->start);
         DBGPRT(1," fend rg(%d) ad(%d)", r->fend, a->fend);
         #endif
         if (r->enc)
          {     // convert to encoded...........
             #ifdef XDBGX
              DBGPRT(1," Convert R%x to encoded %d", r->reg,r->enc);
            #endif


           // add_sys_mterm for calculation

          //c =
           p1.dptype = DPDEC;     // integer
           p1.ival = r->enc;   // encode type
       //    p1.pmode = 4;       //decimal

           p2.dptype = DPHEX;
           p2.ival = r->data;        //reg ??;
      //     p2.pmode = 1;      // hex print

           add_smterm(a, 5, &p1, &p2);        // 5 is enc , new link (  in new setup r->enc, r->reg);     // adt, func - do rest afterwards


           //   a->enc  = r->enc;
           //   a->data = r->data;
           //   a->data |= datbnk;




           a->fnam = 1;
           a->cnt  = 1;      // as args added in twos anyway
           a->fend = 15;         // word
          }
         else
          {     // not encoded, adnl in twos anyway, simple change
                // if rgstat size bigger (word) (use 3 for byte and word)
           if ((r->fend & 31) > (a->fend & 31) && a->cnt > 1)
             {         //byte
              a->cnt  = 1;
              a->fnam = 1;

              a->fend = (r->fend & 15);        //no bigger than word
         //     a->fend |= (r->fend & 32);       // ignore sign

              #ifdef XDBGX
              DBGPRT(1,"Change R%x size to %d", r->reg, r->fend);
              #endif
             }
          }


// add a data item here ??
// add = decode_addr(a, s->curaddr);       no, need separate subr                 // val with decode if nec.  and sign - READS ofst
        }
                  #ifdef XDBGX
      else  DBGPRT(1,"R%x not found for %x", 0, l->start);
      #endif
      fid = a;

     }

       // sanity check here
     if (size != adn_totsize(k))
       {
         #ifdef XDBGX
           DBGPRT(1,"ADDNL SIZE NOT MATCH %d %d", size, adn_totsize(k) );
         #endif
   //     free_adnl(&chadnl,k);            //all chain ??
    //    a = add_adt(&chadnl,k,0);
    //    a->cnt = size;
       }

   }

 }      /// for emuargs loop

  #ifdef XDBGX
//    if (changed)
    DBG_rgchain();
    DBGPRT(1,"\nEnd FIX args");

  #endif



}


 void add_arg_data(void *fid, uint ofst)
  {
   ADT *a;
   uint val, i,sz;

 //  a = get_adnl(CHADNL, fid);          //start_adnl_loop(CHADNL,fid);


 while ((a = get_adt(fid,0)))         // get_next_adnl(CHADNL,a)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     val = decode_addr(a, ofst);            // only for address and size ?
     sz = bytes(a->fend);

     add_cmd (val,  val+sz -1, sz, ofst);     //change to dtk ???
     ofst += sz;
    }
 fid = a;
  }


}


*/



void reconcile_args(LBK *k)
{
  uint j;

  SPF *s;
  DPAR *d, *n;
  RST *g;
  ADT *a;
  FKL *l;

 int size, before,after;


//this would probably work for static args as well....



 if (k->spfdone) return;

 #ifdef XDBGX
   DBGPRT(0, "reconcile args %x", k->start, k->size);
#endif

   s = find_spf(vconv(k->start),10,0);
 //  a = get_adt(vconv(k->start),0);                 //always one adt of k->size ??

 #ifdef XDBGX
  DBGPRT(1,"  spf = %x", s);      // adt = %x", s, a);
#endif

  size = k->size;            // keep overall size

  before = k->size;        //adn_totsize(vconv(k->start));       // = k->size....


  // if (size == 1) return;     //temp fix for single bytes ??

  a = (ADT*) vconv(k->start);        // start fid

   if (s)
     {
  //     DBGPRT(1,"  adtsize = %d, spfsize =  %d", size, s->argcnt);
    //   DBGPRT(0,"SPF %x %d", s->fid, s->argcnt);

       for (j = 0; j < s->argcnt; j++)
         {
           d = s->pars + j;
           g = get_rgstat(d->reg);


           d->dfend = g->fend & 0xf;            // fiddle here for max of word ....
           if (d->reg & 1) d->dfend = 7;      // don't allow odd words

           if (d->dfend > 7)  //only evens ??
            {
              n = d+1;           // next entry already used by this one
              if (n->reg == d->reg+1) n->skip = 1;
            }


// a->cnt is the master size control here....if size or anything else (e.g encode) happens, must create a separate adt entry.
// one spf per aux cmd I think. DON'T THINK THIS CHECKS FOR ONE ARG !!!



          if (!d->notarg  && !d->skip)             //skip transfer items
           {
             //
            if (size <= 0) break;


        //    a->cnt = 1;             // per param ??  bytes(d->dfend);

            if (d->dfend)
{  a = add_adt(a,0);
 a->fend =  d->dfend;

            if (size == 1  && (d->dfend & 0x1f) > 7)
            {
               DBGPRT(0,0);
               a->fend = 7;
             //  a->fend |= d->dfend & 0x20;  //sign
            }

            if ((a->fend & 0x1f) > 7) a->fnam = 1;

            if (d->calc)
              {
                l = get_link(s, 0, 1);          // get calc for this spf
               if (l)  add_link(CHADNL,a,CHMATHN,l->keydst,1);
             //  a->fend = 15;                     //assume word ??  not always
                  // get math calc from...er
       //         MATHX *add_encode(ADT *a, uint type, uint reg)        //but how to reuse ??
              }
}
            size -= bytes(d->dfend);
       //     if (size > 0) a = add_adt(a,0); else break;      //next one or stop if no size left
           }



         }

     }

// check a totsize BEFORE AND AFTER !!!  76e0 A9L overlaps.........

after = adn_totsize(vconv(k->start));
 //while a ??


if (before != after)
{
    DBGPRT(0,0);
}




 if (a)
   {
//inital adt are just bytes and size..................use spf to turn into proper sizes and types





   }




     // and here fix the adt entries to match the spfs....






 k->spfdone = 1;
     #ifdef XDBGX
 DBGPRT(2,0);
 #endif
  }


//so can get at auxcmd easily

 // SBK *tgtblk[2];         // real [0] and emu [1] subr scan block.  (i.e. the call before nextaddr)
 //SBK *argblk[2];         // real [0] and emu [1] block where nextaddr is (where the args actually are, and caller of subr)
 //LBK* auxcmd;           // args command
 //SPF* fblk;             // link to spf block (attached to real scan block anyway ?)
 //uint reg : 8;         // register
//}

//}








void skip_args(SBK *s)
 {
   LBK *k;     //, *e;
 //  int i;


//and fix args in here................

  // if (s->proctype != 1) return;    // not for sigs or emulate.


   k = (LBK*) 1;   // to start loop..

   while (k)
    {
     k = get_cmd(CHAUX,s->curaddr,C_ARGS);
     if (k)
       {
         reconcile_args(k);            //find related spf if there is one, update arg sizes.


  //       fix_args(k,s);
    //      add_arg_data(k, s->curaddr);

        s->curaddr += k->size;
        #ifdef XDBGX
         DBGPRT(1,"SKIP %d args -> %x", k->size, s->curaddr);
        #endif

        if (!k->size)
         {
          #ifdef XDBGX
           DBGPRT(1,"ARGSIZE IS ZERO !! %x", s->curaddr);
         #endif
         s->curaddr ++;
         }

        /* fix args with new sizes, encode, etc
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
             }*/
        }
       }
    }



void scan_blk(SBK *s, INST *c)
 {
   // used for both scan AND emulate

   if (!s) return ;
   if (!s->proctype)
   {
     #ifdef XDBGX
       DBG_sbk("Dummy",s);
     #endif
     return;         // safety
   }

   show_prog (anlpass);

// done or invalid

   if (s->stop || s->inv)
       {
         #ifdef XDBGX
          DBG_sbk("No",s);
          DBGPRT(1,0);
         #endif
         return;
        }

   // check not a faulty ptr to filler block

   if (check_sfill(s->curaddr,4))
      {
       s->stop = 1;
       s->inv = 1;
       #ifdef XDBGX
          DBG_sbk("Invalid (fill)",s);
        #endif
       return ;
      }


/*  limited emulate idea - not used at the moment.

 if (s->proctype == 4  && s->curaddr >= s->endaddr)
   {

    #ifdef XDBGX
          DBG_sbk("*Stop Emulate* ",s);
       #endif
       s->ignore = 1;
       s->stop = 1;
       return ;
   }
*/

//if (s->proctype == 2) DBGPRT(0, "XZXZTST");

   tscans++;

   #ifdef XDBGX
      DBG_sbk(0,s);
   #endif

   while (1)
    {
      if (s->stop || s->inv) break;               // standard stop
      if (s->estop && s->proctype == 4) break;    // emulate stop

     skip_args(s);                             // check for any ARGS command(s) added
     do_signatures(s);                         // moved sig checks to here

     do_code (s, c);                           // do opcode

     s->curaddr = s->nextaddr;                 // next opcode address

     if (s->proctype == 4)                 // emulating
      {                                    // safety check for code loops
       opcnt++;

       if (opcnt > 1000)
       {
        s->stop = 1;
            #ifdef XDBGX
        DBGPRT(1,"Hit opcode limit (%d) %x  %x", opcnt, s->start, s->curaddr);
            #endif
       }
      }

    }

if (s->estop)
    {
             #ifdef XDBGX
             DBGPRT(1,"ESTOP set at %x", s->curaddr);
             #endif
    }
s->estop = 0;                    // stop flag for arguments.

}

//may move to chain.cpp...........

uint do_test_scan(uint start, uint end, int type, SBK *caller)
{
    // wrapper around scan block for testing vects etc. does same as main scan but
    // in test chain, and checks afterwards

  CHAIN *x;
  SBK *z, *s;
  uint ix, ans;
  INST i;

return 1;      // TEMP


  // if scan already exists and is done (stop set) then skip and assume it's OK...
  // need more here (stack) if PUSHED by imd, or uses args ??

// add_scan in here ???

  if (get_scan(CHSCAN,start)) return 1;            //real scan already exists.


  s = add_scan (CHSTST, start, end, type, caller);

  //change proctype ???

     #ifdef XDBGX
  DBGPRT(1, "\n SCAN TEST");
  #endif

  scan_blk(s,&i);      // but in test block ?

  x = get_chain(CHSTST);

  ans = 1;

  for (ix = 0; ix < x->num; ix++)
   {

     z = (SBK*) x->ptrs[ix];
     if (z->inv)
       {
        ans = 0;
        break;
       }
   }

if (ans)
 {          //no invalids found, copy blocks to main chain
      #ifdef XDBGX
 DBGPRT(1, "\n PASS scan test");
#endif
/* for (ix = 0; ix < x->num; ix++)
   {

     z = (SBK*) x->ptrs[ix];
    // if (z->inv)
    //   {
      //  ans = 0;
      //  break;
      // }
   }
*/

 }
else
{
         #ifdef XDBGX
DBGPRT(1, "\n FAIL scan test");
#endif
}


  clearchain(CHSTST);
return ans;
}







void emulate_blk(uint addr, SBK *caller)
 {
   // the entry point for an emulation run, from scan blk s.


   // must check for redundant emulation runs.....
   // if a subr higher up the stack has emu set, then should not need to start a run from this block....
   // can do this now, from new stack setup.

//how to stop repeat calls, but

//   SBK *x;
 //  RPOP *p;
   uint ix;
 //  int ptr;

   if (!caller) return ;               // safety
   if (!caller->proctype) return ;

   show_prog (anlpass);

   #ifdef XDBGX
     DBGPRT(2,0);
     DBGPRT(1,"In Emulate for %x", addr);
   #endif

//   cleareptrs();       // clear all emulate scans at start of emulate...

   opcnt = 0;               // global so it survives


 // clear all register fields to do with emulate (keep rbase fields)

 for (ix = 0; ix < 0x400; ix++)
    {
      regstat[ix].fend   = 0;
      regstat[ix].sreg   = 0;
      regstat[ix].argcnt = 0;
      regstat[ix].popped = 0;
      regstat[ix].arg    = 0;

    }

 //may need to clear popregs too.

 memset(regpops,0, 8* sizeof(RPOP));

 //but this means only 2 are ever set ???



//emucaller ?


// make a new call stack with scan blocks in emu chain.

 emuscan = make_emu_stack(addr,caller);

 scan_blk(emuscan, &einst);

      #ifdef XDBGX
             DBGPRT(1,"Emulate STOP %x at %x", emuscan->start, emuscan->nextaddr);
             if (emuscan->inv) DBGPRT(1," with Invalid at %x", emuscan->curaddr);
          #endif

clearchain(CHSTST);

}


















/*
void scan_loop(SBK *s, INST *c, uint ofst)
 {
   // used for emulate of loop, has a stop ofst...

   int lscans = 0;

   if (!s) return;              // safety

    #ifdef XDBGX
     DBGPRT(0,"Emu");
     DBGPRT(0," Loop");
     DBGPRT(1," blk %x at %x", s->start, s->nextaddr);
    #endif





   show_prog (anlpass);

   s->scnt++;                                   // scan count by block

   while (!s->stop && !s->inv)                 // inv seems to stop loops, but should it be overridden with args detection ?
    {
     skip_args(s);                             // check for ARGS command(s)

     do_code (s, c);                           // do opcode
     s->curaddr = s->nextaddr;                 // next opcode address

     if (s->curaddr > ofst) break;

     lscans++;

     if (lscans > 2000)        // > 10000)
       {
        s->stop = 1;
        #ifdef XDBGX
         DBGPRT(1,"Hit opcode limit %x  %x", s->start, s->curaddr);
        #endif
       }
    }

    #ifdef XDBGX
     DBGPRT(0,"Stop ");
     DBGPRT(0,"Emu");
     DBGPRT(1," blk %x at %x", s->start, s->nextaddr);
    #endif

}
*/



/*SBK *fakepop(SBK *s, OPS *o)
{
   // Officially,  Rx = [stkptr];  (SP)+=2;

   // change this to a wrap type from stkptr
   // but keep other entries as valid.
   // so pop returns value and changes pointer but does not erase anything

   SBK *x;
   AGT *a;

   // do a fake pop via sbk stack

   x = s->stack[s->stkptr];      //current top of stack

   if (x && o && !zero_reg(o))       //ignore if r0
    {
     o->val = x->nextaddr;
     a = args+s->stkptr;     // create args entry
     memset(a, 0, sizeof(args[0]));        //clear this entry only
     a->start = s->curaddr;             // where this pop is
     a->scb = x;
     a->reg = o->addr;
     a->pop = 1;             // pop ped and not pushed
    }

   s->stkptr++;             //increment AFTER setup

   return x;

}


void fakepush(SBK *s, SBK *newblk)
 {
  // officially stkptr -=2; [stkptr] = [Rx];
  // so moves stackptr first....

  // must allow inserts with wrap.........
  // to keep older entries valid, despite a push


  AGT *a;

   s->stkptr--;
   s->stack[s->stkptr] = newblk;       /// but this puts newblk ABOVE the push.... wrong !!

   a = args+s->stkptr;     // create args entry
   a->pop = 0;
   a->scb = newblk;
   a->reg = 0;

   s->stkptr--;
   s->stack[s->stkptr] = s;    // move marker

   a = args+s->stkptr;     // create args entry
   a->pop = 0;
   a->scb = newblk;
   a->reg = 0;

   s->stkptr++;

}



int get_staxck(SBK *s, FSTK *t, int types)
{
 // get stack entry, with bit mask of types allowed
  int xmatch, ans;

  xmatch = 0;
  ans = t->newadd;   // default return;

  if (t->type & types) xmatch = 1;        // OK

  switch(t->type)
    {

     default:
     case 1:    // std call, default
     break;

     case 2:    // imd
       ans = t->psw;          // return address as pushed
       break;

     case 4:                 // pushp addr = psw bank goes in 10-13
       ans = t->psw;         // psw
       ans |= ((t->origadd >> 6) & 0x3c00);   // add callers bank in correct place
       break;
   }

if (!xmatch && s->proctype == 4)          //emulating)
  {
    ans = t->origadd;

    #ifdef XDBGX

    DBGPRT(1,"E STACK TYPE not match %x != %x at %x", t->type, types, s->curaddr);
    DBG_stack(s,t);

    #endif
  }

return ans;
}







}

   if (caller->proctype == 4)  t = emulstack;
   else
    {
      // don't do immediate pushes in scan mode
      if (type == 2) return;
      t = scanstack;
     }

   memmove (t+1, t, sizeof (FSTK) * (STKSZ-1));  // shuffle stack up
   memset(t,0, sizeof(FSTK));       // clear entry[1]

   t->type    = type;
   t->newadd  = caller->nextaddr;
   t->origadd = caller->nextaddr;
   t->psw     = addr;

}


int check_caller_chain(int addr, SBK *caller)
 {
 SBK * x;
 int i;
  // scan may not exist yet (legally)

  x = get_scan(addr);
  if (!x) return 0;

  for (i = 0; i < 8; i++)

  //while (caller)
    {
      if (x->stack[i] == caller) break;
      if (x->stack[i] == 0) break;            //shortcut
  //    if (caller == caller->caller) break;     //not reqd ??
      //caller = caller->caller;
    }
  if (!x->stack[i]) return 0;
 return 1;
}






SBK* find_dst(SBK *s, uint lev)
{
  int sp,v;
 // SBK *x;
  AGT *r;
  sp = -1;
 // x = 0;

// find where THIS block is in the stack
// do seem to need this
DBGPRT(1,"in find dst %d",lev);

  for (v = 0; v < 8 ; v++)
   {
     if (s == s->stack[v].sc)
       {
         sp = v;             // found
         break;
       }

   }

 if (sp < 0) { DBGPRT(1,"dst=ZERO"); return 0; }
   // not found


for (v = 0; v < 8; v++)
  {
      r = args+v;
    if (s->stack[v].sc)
     {
      DBGPRT(0, " [%d",v);
      if (v==s->stkptr) DBGPRT(0,"*");
      DBGPRT(0,"]");
      DBGPRT(0,"%x (%x)", s->stack[v].sc->nextaddr, s->stack[v].sc->start);
      if (r->pop) DBGPRT(0, "P");      //po pped
     }
  }
DBGPRT(1,0);


v = s->stkptr;


 find entry which is check if any stack entry is already pop ped

  sp = (sp+lev) & 7;      //next entry 'up'

  r = args + sp;

  while (r->scb && r->pop)
    {
     if (r->scb == s) break;         //safety, not this one  WRONG
     if (r->scb) DBGPRT(1,"%d %x (%x) %d",sp, r->scb->nextaddr, r->scb->start, r->pop);
     sp = v;
     v++;
     v &=7;
     r = args + v;
    }


  v = (sp+lev) & 7;      //next entry 'up'

  r = args + v;

  while (r->scb && r->pop)
    {
     if (r->scb == s) break;         //safety, not this one  WRONG
     if (r->scb) DBGPRT(1,"%d %x (%x) %d",sp, r->scb->nextaddr, r->scb->start, r->pop);
     v++;
     v &=7;
     r = args + v;
    }


//dst (sp) seems to be correct place for emu....



 // if (sp >= 0)
  //  {
  //   x = s->stack[sp];       //next call up (but it's NOT always !!)

     #ifdef XDBGX
   // if (x)
   if (s->stack[v].sc) DBGPRT(1, "dst p = %d (%x)",v, s->stack[v].sc->start);
   else DBGPRT(0,"dst=ZERO");
     #endif
  //  }
 return s->stack[v].sc;
 }




void add_xargs(SBK *x, uint cnt)
{
LBK *k;
ADT *a;

if (!x || !cnt) return;

  DBGPRT(1, " &&& add args %d at %x", cnt, x->nextaddr); //              adt %x %d pars", s->start, f->dpars[0]);

  k = add_aux_cmd(x->nextaddr, x->nextaddr+cnt, C_ARGS,0);

  if (k)
   {
     a = add_adt(k,0);
     if (a)
       {
         a->cnt = cnt;
         k->size = adtchnsize(k, 0);
         x->nextaddr += cnt;            //and move beyond the args
      }
   }
}

*/


void check_spf (SBK* s, uint calltypex)
{
 // SUB *sub;
  SPF *f;
  SBK *x;               //to scan backwards...........
  CHAIN *jch, *sch;
 // LBK *k;
  // SPF *r;
//  SUB *sb, *xb;
  void *fid;

//  CHAIN *c;
  JMP *j;
  FKL *l;
  SIG *g;
 // ADT *a;
  uint ix, jx, zx;
//  int v;        //, sp;

  if (!s) return;
  if (s->inv) return;
  if (!s->stop) return;  // not scanned yet

     #ifdef XDBGX
DBGPRT(1,"In check_spf (%x)", s->start);
#endif

// block 's' has just been scanned, and has finished. now check for any spfs added
// calltype is J_STAT or J_CALL  (currently)


 // x = find_dst(s,1);             // destination sbk for calls this to become an address.....
  //becomes nextaddr address for args instead....
 // x = s->stack[sp];

         jch = get_chain(CHJPT);            //'jump to' chain
               sch = get_chain(CHSCAN);            //'scan' chain

 // if (x) xb = get_subr(x->subaddr); else xb = 0;



 // sb = get_subr(s->subaddr);

  fid = vconv(s->start);

  while ((f = get_spf(fid)))
    {
             #ifdef XDBGX
     DBGPRT(1, " &&&x spf %d l%d", f->spf, f->level);     // sce->start);
     #endif

     switch (f->spf)
       {
         default:
          break;

         case 2:
         case 3:
           // func and table lookups
           // get and check jumps to a lookup signature

           l = get_link(s,0, 0);
           if (l)
             {
              g = (SIG*) l->keydst;               // get the attached sig
              ix = get_tjmp_ix(s->start);         // find first jump to this subr

              //ix = get_tjmp_ix(g->start);         // find first jump to this sig  WRONG!!

              while (ix < jch->num)               // max is end of chain
               {
                j = (JMP*) jch->ptrs[ix];         // get_item(CHJPT,ix);               // get previous possibles
                if (j->toaddr != s->start) break;

                if (j->jtype == J_STAT && !j->back)
                  {
                    jx = get_scan_range(CHSCAN, j->fromaddr);         //get first scan TO this address

                    for (zx = get_lastix(CHSCAN); zx <= jx; zx++)
                       { //what if more than one block ??
                        // what if somthing jumps to THIS block ??
                          x = (SBK*) sch->ptrs[zx];
                          #ifdef XDBGX
                          DBGPRT(1, "run sigproc %s %x (%x)", g->name, x->start, x->nextaddr);
                          #endif
                          do_sigproc(g,x);     // not same sub
                    //      check_spf(x,calltypex);              //infinite loop ???  not reqd with bug fix above ??
                        }
                  }
                ix++;


             }
             }
           break;
 /*        case 5:
           // fixed args

           // level drops if CALL but not if JUMP ?? TO BE SORTED

           if (!f->dpars[0]) break;

           if (f->level > 1 )          //&& !f->adtdone)
             {  // copy spf to one up caller chain
              copy_spf(x,f,-1);            //copies f to end of chain....
              x->spf = 1;
              DBGPRT(1, " &&& copy spf %d (l=%d)to %x", f->spf, f->level-1, x->start);
              DBGPRT(1,"DST (X) = %x, %x", x->start, x->nextaddr);
             }


//can't do the copy spf as I intended as the 7278 messes up the sequence, as the push happens before the subr get processed....

//do an extra check here for open pops ??
//or don't convert the adt , and get actual pop addresses with args.........???
//turn this into args....NO ADT

           if (f->level == 1 && !f->adtdone)
             {
               add_xargs(x,f->dpars[0]);
               f->adtdone = 1;
             }


/
              a = get_last_adt(s);
              a = add_adt(a,0);
              a->cnt = f->dpars[0];              //temp...need more...........
              s->adtsize = adtchnsize(s,0);      //  sb->size = adtchnsize(sb, 0);
              f->adtdone = 1;                     //stop duplicates
             }    //

           break;



      case 6:

//this will have to become an emulate.............

           if (!f->dpars[0]) break;


           if (f->level > 2)       // && !f->adtdone)
             {   // copy spf to one up caller chain
              copy_spf(x,f,-1);
              x->spf = 1;
              DBGPRT(1, " &&& copy spf %d (l=%d)to %x", f->spf, f->level-1, x->start);
             }




//CANNOT DO THIS WITH DELAYED SPF 5 HAVE TO DO IT NOW!!!!

           if (f->level == 2)             // && !f->adtdone) not for muliple callers
            {
              SPF *t;
    //            x = find_dst(s,2);            //TEST !!

              v = g_byte(x->nextaddr);
              DBGPRT(1, " &&& spf 6 l2 - get pars [%x] %x = %d", x->start, x->nextaddr, v);
              f->dpars[1] = v;




//DBGPRT(1,"would be sp = %x,%x (%x, %x)", sp+1, s->stack[sp+1]->nextaddr, s->stack[sp+1]->curaddr, s->stack[sp+1]->start);     //this gets right address !!
//curaddr is right for tag of emureqd, nextaddr is right for where args go.



           //   if (x)  { DBGPRT(1, " &&& copy spf 6>5 to args %d for %x", v, x->start);}
                          k = add_aux_cmd(x->nextaddr, x->nextaddr+f->dpars[0]-1, C_ARGS,s->curaddr);


                 //    fid = get_last_adt(l);
if (k) {
                 a = add_adt(k,0);
          if (a)  {a->cnt = v;
           k->size = adtchnsize(k, 0);
           x->nextaddr += f->dpars[0];            //and move beyond the args
              }}}



              t = copy_spf(x,f,-1);

              t->spf = 5;                       // change to fixed arg type
              t->dpars[0] = t->dpars[1];
              x->spf = 1;
              DBGPRT(1, " &&& copy spf 6>5 l=%d to %x", f->level-1, x->start);

              f->adtdone = 1;

       //       if (f->level == 2)             // && !f->adtdone) not for muliple callers
       //     {
         //                x = find_dst(s,2);
    //       DBGPRT(1," EMULATE HERE %x from %x", x->start, x->curaddr);


        //    }
            }

           break;

*/




       }                //end switch



     fid = f;
    }

    }




void op_calc (int pc, int ix, SBK *s, INST *c)

 //  calculate operand parameters for index 'ix'
 //  from 'pc' (typically the register address)
 //  assumes read size as already set from operand cell (OPS)
 //  sym names and special fixups done in print phase
 //  indirect and indexed address modes handled in calc mult_ops

 {
  OPS *o;
  RBT *b;

  o = c->opr+ix;

  o->addr = pc;
  o->rbs = 0;
  b = 0;

  if (o->rgf)
     {                                          // if register (default)
      o->addr &= 0xff;                          // strip bank and safety check
      o->addr += (s->rambnk * 0x100);                     // rambank always zero if 8061

      // for 8065 direct address word and larger,
      // if odd, change to regbank+1 if regbank is 0 or 2

      if (get_cmdopt(OPT8065) && (o->addr & 1) && bytes(o->fend) > 1)
        {  // add 100 for even reg banks only
         if ((o->addr & 0x100) == 0) o->addr += 0x100;  // 0->1, and 2->3
         o->addr ^= 1;         // drop odd bit, even if no bank change
        }

      if (c != &einst)
         {
             b = get_rbase(o->addr,c->ofst);       // not in emulate, check regstat first
         }



     }         // end register
  else   o->addr = databank(o->addr,c);         // generic address, add bank

  if (o->imm || o->bit) o->val = pc;            // immediate or bit flag, so pc is absolute value

  else o->val = g_val(o->addr,0, o->fend);     // get value from reg address, sized by op

  if (b)
   {                                            // don't do rbase if in emu mode
    o->rbs = 1;
    o->val = b->val;                            // get value from RBASE entry instead
   }

  }




/*  Wrong ?? or, at least doesn't seem to cover everything
if (s->proctype == 4)
{
    RST *g;
    g = get_rgstat(o->addr);
    if (g && o->fend > g->fend)
{

 g->fend = o->fend;  // this is only rg size update, and for largest
  DBGPRT(1,"set fend %d for R%x",g->fend,o->addr);
}
}*/




void op_imd (SBK *s, INST *c)
{
  // convert op[1] to imd - always op[1]
  OPS *o;

  o = c->opr + 1;
  o->imm  = 1;
  o->rgf  = 0;
  o->rbs  = 0;
// o->sym ??
 // if (s->rambnk) o->addr -=  (s->rambnk*100);  // remove any register bank ??
  o->val  = nobank(o->addr);                         // and any ROM bank
}



void do_vect_list(INST *c, uint start, uint end, SBK *caller)
 {

// no checks..................
  uint pc, vcall;
  LBK  *k;
  SBK *s;
  ADT *a;
 // INST i;
 // if (!s->proctype) return;

 // pc = start;

  // would need to do PC + subpars for A9L proper display

   #ifdef XDBGX
    DBGPRT(1,"do vect list %x-%x", start,end);
   #endif

  for (pc = start; pc < end; pc +=2)
    {
     vcall = g_word (pc);                            // address from list
     vcall = codebank(vcall, c);                     // assume always in CODE not databank

     s = 0;

     if (!val_rom_addr(vcall))  vcall = 0;      //can drop this, done in add_scan ?
     else s = get_scan(CHSCAN, vcall);

     if (s && (s->stop || s->inv)) vcall = 0;      //done already ?? or is just existing OK ??

      if (get_ftype(vcall)) vcall = 0;           //????  anything ? code or data or operand ??


// use tmpscan to scan and check for dud pointers instead.

       if (vcall)

        //s =
        add_scan (caller->chn,vcall, 0, J_VECT, caller);             // vect subr (checked inside cadd_scan)

     //       s =  add_scan (CHSTST, vcall, 0, J_VECT, caller);             // vect subr (checked inside cadd_scan)

     /*          #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(0,"Vect ");                   // do scan here ???
          #endif            */
       //   test_scan_blk(s, &i);



    }


  k = add_cmd (start, end, C_VECT,0);
  if (k && caller->codbnk != caller->datbnk)
    {
     // bank goes in ONE addn block.
       a = add_adt(vconv(start),0);
       if (a) {
           k->adt = 1;
       a->bank = caller->codbnk;                  // add current codbank
       if (g_bank(vcall) == g_bank(start)) a->cnt = 0;  //don't print if match
       }
    }
     #ifdef XDBGX
     DBGPRT(1,"END vect list");
     #endif
 }

/******************************************
attempt to decode vector or vector list from push word
probably need to split off the vect list (for multibanks)
****************************************/

int check_vect_list(SBK *caller, uint start, uint sz, INST *c)
{
   uint fails;
   uint i, addr, penalty, iflag, end;
   SBK *s;
   LBK *l;
 //  INST z;


   // assume bank is correct in start
   if (nobank(start) < PCORG) return 0;

   if (sz) end = start + sz;               // 0 to sz is an extra pointer sometimes.
   else   end = start + 0xff;             // 128 vectors max (A9L is 0xa4)

   if (end > maxromadd(end)) end = maxromadd(end);  // safety


// need to do a test scan of all these vects...............
// theoretically from start until fail....

///x x x x x


//allow for vect list to have been moved on by one pointer...
   for (i = start; i < start+3; i+=2)
     {
       l = (LBK *) get_cmd(CHCMD,i,0);
       if (l && l->fcom == C_VECT)
        {     // assume duplicate
         #ifdef XDBGX
          DBGPRT(1,"Dupl Vect %x=%x",l->start, l->end);
         #endif
         return 0;
        }
     }

   fails = 0;                              // fail score
   penalty = 1;                 //use increasing fail penalty
   iflag = 0;

   //this needs probably more checks

   if (start & 1) start++;             // can't start on odd byte, but end should be
   if (!(end & 1)) end --;             // drop last byte if even

   #ifdef XDBGX
    DBGPRT(0,"In check_vect %x-%x size=%x", start, end, sz);
    DBGPRT(1," from %x", c->ofst);
   #endif

   for(i = start; i <= end; i+=2)       // always word addresses
     {
      s = (SBK*) get_scan (CHSCAN,i);
      if (s)
        {
         #ifdef XDBGX
          DBGPRT(1,"found scan %x", i);
         #endif
         end = i-1;
         break;
        }

// xcodes ??


      l = (LBK *) get_cmd(CHCMD,i,0);
      if (l)
        {
          #ifdef XDBGX
            DBGPRT(1,"found %s %x ", get_cmd_str(l->fcom), i);
          #endif
          if (l->fcom < C_CODE) fails +=2; // just score a data item
          else
          {
           end = i-1;    // stop list if anything else
           break;
          }
        }


// if ( get_ftype(i)


/*CHECK VECT !!!


      // now check the vect address itself

      // add extra check for end address, as size can be >,<, <=

      // add check that previous get should be 0xf1 0xfo (rtet, retie ??

      */

      addr = g_word (i);
      addr = codebank(addr, c);

      if (do_test_scan (addr, 0, J_VECT, caller))
       {
          // pass
       }
      else
      {
          // fail
      }

// if (add_scan() == 0)    dead.

// else  uint test_scan_blk(SBK *s, INST *c)

//test alike endings ??


      if (val_rom_addr(addr))
        {                                   // valid pointer address



//if (get_ftype(addr))

        if (addr > start && addr < end)
           {
             #ifdef XDBGX
             DBGPRT(1,"call < end, move to %x", addr-1);
             #endif
             end = addr-1;
           }

         if ((addr & 0xff) == 0)     // suspicious, but possible
           {
            fails += penalty;
            penalty++;
            #ifdef XDBGX
             DBGPRT(1,"Suspicious %x = %x",i, addr);
            #endif
           }
        }
       else
        {            // INVALID pointer address
          if (i == start)
            {
              start+=2;    // first address is invalid, move start (no fail)
              #ifdef XDBGX
                 DBGPRT(1,"Move start to %x (inv %x)",i, addr);
              #endif
            }
          else
            {
             fails += (penalty*2);
             penalty ++;
             #ifdef XDBGX
             DBGPRT(1,"inv_add %x=%x",i, addr);
             #endif
            }

         if (!iflag) iflag = 1;
         else
          { end = i-1;       //  only 1 invalid address allowed
            break;
           }
        }     //end invalid

   }  // end main loop



// add a SIZE check

   if (end <= start) i = 0;           //unsigned, so safety check.
   else  i = (end-start)/2;           // number of pointers

    #ifdef XDBGX
   DBGPRT(1,"End Vectlist %x-%x, %d ptrs fails %d", start, end, i,fails);
  #endif


   if (i < 8)
   {                // is a minimum of 8 pointers (always ???)
     #ifdef XDBGX
     DBGPRT (1,"REJECT LIST %x from %x, %d few ptrs",start, c->ofst, i);
     #endif
     return 0;
   }


   #ifdef XDBGX
     DBGPRT (1,"Final fails %d penalty %d",fails, penalty);
   #endif

   if (fails) fails = i/fails;   else fails = i;  // if no fails, all pointers                          (score)/i;       // = fails*100/number

   #ifdef XDBGX
     DBGPRT (0,"Final score %d = ",fails);
   #endif

   if (fails > 2)
     {        // i.e. fails is < 50% of ptrs...
       do_vect_list(c, start,end, caller);
       #ifdef XDBGX
          DBGPRT (1,"PASS");
       #endif
       return 1;
     }

   #ifdef XDBGX
      DBGPRT(1,"FAIL");
   #endif

  return 0;
}




/*
void shift_comment(INST *c)
 {
  //  do shift replace here - must be arith shift and not register
  int ps;

  if (anlpass < ANLPRT) return ;              // redundant ?

  if (c->opcix < 10 && c->opr[1].imd)
   {     // immediate shift
    if (zero_reg(c->opr + 2))
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
      ps += sprintf(nm+ps, "= %d] ", 1 << c->opr[1].val);
     }
    acmnt.ofst = c->ofst;
    acmnt.ctext = nm;
    acmnt.minst = c;
  }
}

*/


/*
void do_regstats(INST *c)
 {
   int ix;
   OPS *o;
   RST *r;

   ix = 1;

   if (c->opcsub == 2)
    {
     // indirect - use opr[4] for reg details
     o = c->opr+4;
     r = get_rgstat(o->addr);

     if (r && r->arg)
      {
       // arg used as address, so will always be WORD
       r->inr = 1;

       // add here instead ??
      }
      ix = 2;      //done register 1
    }


// register size arguments, for arguments
// op [1] or op[2]

   while (ix < 3)
    {
     o = c->opr+ix;
     r = get_rgstat(o->addr);

     ix++;
    }

}
*/

void set_inv_opcode (SBK *s)
{
  s->inv = 1;

  //if previous op was a CALL - should we check for arguments here ????


  if (s->proctype)
   {

    // if (anlpass)
  //      {
          #ifdef XDBGX
          if (s->proctype == 4) DBGPRT(0, "E ");
             DBGPRT(1,"Invalid opcode (%x) at %x [%d]", g_byte(s->curaddr), s->curaddr, anlpass);
          #endif
       //   wnprt(1,"Invalid opcode at %x [pass %d]", s->curaddr- 0x10000, anlpass); // report only once
  //      }
     s->nextaddr += 2;   // move nextaddr for end_scan  (is this necessary ??)

     end_scan (s);

   }
  return;
}

uint chk_inv_operands(SBK *s, INST *c)
{
  OPS *o;
//  const OPC *opl;
  uint i,ans, x, fend;

  ans = 0;

  //indirect & indexed check

  if (c->opcsub > 1)
    {  // both CPUs - no indirects or indexes for SFR, except R0
     o = c->opr + 4;                //  copy of [1] if indirect or indexed
     i = 2;                         // don't recheck opr[1]

     // 0 invalid, 1 = general  2 = zero reg, 3 sfr, 4 stack

     x = valid_reg(o->addr);

     if (x > 1)    //check copy in [4]
       {
        if (c->opcsub == 2 && x == 3) ans = 1;    // no sfr indirects , zero reg
        if (c->opcsub == 3 && x == 3) ans = 2;    // zero regs OK with inx.  NB. 4tab does R0 = [R0] as time waster

        if (o->addr != 5 && o->inc) ans = 3;      // R5 is only inc which makes sense
       }
    }
  else i = 1;        //if not indirect


  //check other/all operands

  // specific ops outside the loop....

     if (get_cmdopt(OPT8065))
       {
         // [3] = op [1]  style .....

         if (c->opr[3].fend & 64)
           {      // write ops to SFRs
             if (c->opr[3].addr == 0xa  && c->opr[1].val != 0x10 && c->opr[1].val != 0) ans = 4;   // only bit 4 for R10  (mem_expand)
             if (c->opr[3].addr == 0x1b && c->opr[1].val != 0x80 && c->opr[1].val != 0x7f && c->opr[1].val != 0) ans = 5;   // only bit 7 for R1b  (HOS used)
           }
       }


  for (; i <= c->numops; i++)
   {
    o = c->opr + i;
    if (ans) break;

    fend = (o->fend & 31);             // drop sign and write for SFRs

    if (o->rgf && valid_reg(o->addr) == 3)
     {
       //both CPUs, SFR check, skip zero regs

       if (get_cmdopt(OPT8065))
        {      //8065.
          if ((o->fend & 64) && (o->addr == 6 || o->addr == 0xf || o->addr == 0x18 || o->addr == 0x1c)) ans = 6;  // no write 6,f,18,1c

          if (fend < 15)
             {
              if (o->addr == 6) ans = 7;
              if (o->addr == 0x1c) ans = 14;
        //      if (o->addr > 0xd && o->addr < 0x19 && !(o->addr & 1)) ans = 8;    // word only 0xe -> 1c evens
             }
        }
      else
       {   // 8061

          if ((o->fend & 64) && (o->addr == 6 || o->addr == 0xB)) ans = 9;     // no writes 6, B

          if  (o->addr == 7 || o->addr == 0xf )  ans = 10;                       // no direct access (word only)
          if (fend < 15 && (o->addr == 6 || o->addr == 0xe )) ans = 11;      // word only

          if (fend > 7)
            {
             if (o->addr > 1 && o->addr < 6) ans = 12;        // 2 to 5   are byte only
             if (o->addr > 7 && o->addr < 0xe) ans = 13;      // 8 to 0xd are byte only
            }
       }

     }   // end sfr check

      // check sizes for all ops, including SFRs.

    if (!o->imm)
      {      // addresses only.....not immediates
   //    fend = (o->fend & 31);
    //   if (o->fend) fend = (o->fend & 31);                // drop sign

    //   if (fend > 15 && (o->addr & 3))  ans = 14;            // longs must be on 4 byte boundary (not sure if true)
       if (fend > 7  && (o->addr & 1))  ans = 15;            // words must be on 2 byte boundary
      }

  } // end for numops loop






if (ans)
{
    #ifdef XDBGX
    if (s->proctype == 1)
    {
     DBGPRT(1, "\nZSZS %d INV opcode %x R%x", ans, s->curaddr, o->addr);
    }
    #endif
  //  if (s->proctype == 4) set_inv_opcode (s);       //emu only ??

// if (s->proctype)
// {
 //  fend = c->ofst;
 //  fend = find_opcode(0, fend, &opl);
 //  if (fend && opl->sigix == 17) { set_inv_opcode (s); s->curaddr -= 1; } //dodgy operands after call ???
// }
}
  return ans;           // OK
}

void cpyop(INST *c, uint dst, uint sce)
{
   // straight copy, but keep the orig size for write op.
   uint w;

   w = c->opr[dst].fend;
   *(c->opr+dst) = *(c->opr+sce);
   c->opr[dst].fend = w;
}


 void calc_1op(SBK *s, INST *c)
 {  // single operand as register

    c->opcsub = 0;         // no multiple mode

    op_calc (g_byte(s->nextaddr + 1), 1,s,c);
    if (s->proctype == 1) set_ftype_data(s->nextaddr + 1,7);

    cpyop(c,3,1);                  //op[3] is op[1] + write

    s->nextaddr += 2;

    chk_inv_operands(s,c);

//   add_dtk(s,c); ??
// rgsize ??
  //  do_regstats(c);
 }

 void calc_2op(SBK *s, INST *c)
 {  // 2 operands as registers

    c->opcsub = 0;         // no multiple mode

    op_calc (g_byte(s->nextaddr + 1), 1,s,c);
    if (s->proctype == 1)  set_ftype_data(s->nextaddr + 1,7);
    op_calc (g_byte(s->nextaddr + 2), 2,s,c);
    if (s->proctype == 1)  set_ftype_data(s->nextaddr + 2,7);

    cpyop(c,3,2);         //op[3] is op[2] + write


    s->nextaddr += 3;

    chk_inv_operands(s,c);

//   add_dtk(s,c); ??
// rgsize ??
 //   do_regstats(c);
 }







void calc_mult_mode(SBK *s, INST *c)
{
  /******************* multiple address mode, by subtype *****************
    * opcode subtype values   0 direct  1 immediate  2 indirect  3 indexed
    * calculates operands by subtype (=address mode) and number of ops
    * always in op[1], others always register.
    *
    * opcodes are defined as
    * 3 ops     sce1, sce2, dest    for   [3] = [2] <op> [1]
    * 2 ops     sce1, dest          for   [2] = [2] <op> [1]
    * 1 op      dest (sce1)         for   [1] =     <op> [1] or [1]= <op>
    *
    *  organised as
    * Num     op[3]    op[2]    op[1]       internal maths
    * 3 ops   dest     sce2     sce1       [3] = [2] <op> [1]
    * 2 ops   #sce2    sce2     sce1       [3] = [2] <op> [1]
    * 1 op    #sce1    -        sce        [3] =     <op> [1] or [3]= <op>
    *
    * '#' means copy of, adjusted write size as necessary (e.g. DIV)
    * ops are swopped or copied as required (STX swops, others copy).
    * for multimode address opcodes -
    * op[4] is copy of op[1] before indexed and indirect ops calculated
    * op[4] is incremented in ->val if autoinc set
    * op[0] is offset (indexed) or intermediate address (indirect) ops

    *******************************************
    * xofst adds for true size of opcode
    * +2   opcsub 3, if 'long index' flag set
    * +1   opcsub 3, if not set  (='short index')
    * +1   opcsub 1  Word op     (immediate)
    ******************************************************/

     int firstb,xofst,addr;
     OPS *o, *cp;

     xofst = s->nextaddr;

     firstb = g_byte(xofst+1);             // first data byte after (true) opcode
        if (s->proctype == 1) set_ftype_data(xofst+1,7);

     o = c->opr+1;                         // this is ALWAYS op [1]
     cp = c->opr+4;                        // copy goes in [4]

     switch (c->opcsub)
      {
       case 1:                                          // op[1] immediate value, byte or word
         op_imd(s,c);                                   // mark as imd, not reg.

         if ((o->fend & 31) > 7)
          {                                             // recalc op[1] as word
           xofst++;
           op_calc (g_word(xofst), 1,s,c);
           if (s->proctype == 1) set_ftype_data(xofst,o->fend);
          }
         else
          {
           op_calc (firstb, 1,s,c);                      // op[1] is byte
          }
       *cp = *o;                                      // keep immediate for sigs
        set_rgsize(cp);
       break;

       case 2:     // indirect, with optional increment ALWAYS word (for address)
                   // Save pre-indirect to [4] for increment by upd_watch

         if (firstb & 1)
          {                                         // auto increment if odd
           firstb &= 0xFE;                          // drop flag marker
           o->inc = 1;
          }

         op_calc (firstb, 1,s,c);                   // calc [1] for new value, always even
         o->ind = 1;                                // indirect op
         o->bkt = 1;
         *cp = *o;                                  // keep orig sce register details

         if (s->proctype)
          {
            // sort out correct indirect address and value
     //       o->rgf = 0;                                // not a register anymore but this breaks rbases
            o->addr = g_word(o->addr);               // get address from register, always as word
            o->addr = databank(o->addr, c);          // add bank
            if (o->addr > maxromadd(o->addr)) c->inv = 1;    //invalid address
            o->val  = g_val(o->addr, 0, o->fend);    // get value from address, sized by op
            cp->val = o->addr;                       // save address in the op copy as a value
            cp->fend = 15;                           // val in copy is now a WORD
            set_rgsize(cp);                          // sce, for read ?
          }

         break;

       case 3:                                          // indexed, op[0] is a fixed offset

         if (firstb & 1)
            {
             c->opr->fend = 15;                        // long, offset is unsigned word
             firstb &= 0xFE;                           // and drop flag
            }
          else
            {
             c->opr->fend = 39;                        // short, offset is signed byte
            }

         c->opr->off = 1;                               // op [0] is an address offset
         o->inx = 1;                                    // indexed op
         o->bkt = 1;                                    // print brackets
         op_calc (firstb, 1,s,c);                       // calc op [1]
         if (!o->rbs) o->val = g_word(o->addr);         // always a word
         *cp = *o;                                      // keep orig sce register details
         set_rgsize(cp);                                // sce, for read, before ?
         c->opr->addr = g_val(xofst+2,0,c->opr->fend);  // get offset or base value NB. SIGNED

        if (s->proctype == 1)  set_ftype_data(xofst+2,c->opr->fend);

         addr = c->opr->addr;
         if (addr < 0 && (c->opr->fend & 32)) c->opr->neg = 1;    // flag negative if signed

         if (s->proctype)              // anlpass < ANLPRT)
          {                                            // get actual values for emulation
           addr = c->opr->addr + o->val;
           //o->addr may be NEGATIVE at this point !!    probably never..........
           if (addr < 0)
           {
             addr = -addr;                             // wrap around ?
           }
     //      o->rgf = 0;                                 // not a register - breaks rbases
           o->addr = databank(addr, c);                // get address and bank
           o->val = g_val(o->addr,0, o->fend);         // get value from final address, sized by op
           if (o->addr > maxromadd(o->addr)) c->inv = 1;    //invalid address
          }

         xofst += bytes(c->opr->fend);                  // size of op zero;

         break;

      default:      // direct, all ops are registers
         op_calc (firstb, 1,s,c);                 // calc for op[1]
         *cp = *o;                        // keep orig register details in [4] (for convenience)
         set_rgsize(cp);              // keep size
         break;
      }

   if (c->numops > 1)
     {  // calc op[2] (as read op)
       op_calc (g_byte(xofst + 2), 2,s,c);

      if (s->proctype == 1)     set_ftype_data(xofst+2,7);
     }

   if (c->numops == 3)
     {
       // [3] = [2] op [1] calc op 3 (always write)
       op_calc (g_byte(xofst + 3), 3,s,c);
     if (s->proctype == 1)      set_ftype_data(xofst+3,7);
      }

   if (c->numops == 2)
     {
        // [3] = [2] op [1]  calc op[3] from [2]
        op_calc (g_byte(xofst + 2), 3,s,c);          //calc [2] again but as write op
        //rg-size already set
     }

   chk_inv_operands(s,c);

   // do autoinc, and check if this is part of argument extract.
   // NB. must be indirect (2) for o->inc to be set

   // check_args_lists(s,c);           // maintain arg list if


   if (cp->inc)
       {
        // increment ram for next register access. update ram via the operand copy
        // possibly only if popped for args counts ??

        #ifdef XDBGX
          if (s->proctype)
          {
            if (s->proctype == 4) DBGPRT(0,"E ");
            DBGPRT(1, "%x R%x++ = %x+%x",c->ofst, cp->addr, cp->val, bytes(o->fend));
          }
        #endif

        if (s->proctype == 1)    //scanning, just flag...
          {
            RST *r;
            r = get_rgstat(cp->addr);
            if (r) r->argcnt++;
            check_args_inc(s, cp);                   // args check by increment (pop-push check elsewhere).
          }

        if (s->proctype == 4)            //emulating, do actual increment
         {
          cp->val += bytes(o->fend);              // and increment from original size
          upd_ram(cp->addr,cp->val,cp->fend);     // always a WORD update
       //   check_args_inc(s, c);                   // args check by increment (pop-push check elsewhere).
         }
       }

   if (!s->inv) s->nextaddr = xofst + c->numops + 1;     //may have been set by chk_inv

   add_dtk(s,c);

   //  do_regstats(c);

  }


void pset_psw(SBK *s, int t, int mask)     //        INST *c, int t, int mask)
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

void set_psw(SBK *s, int val, int size, int mask) // INST *c, int val, int size, int mask)
 {

    s->psw &= (~mask);     // clear defined bits

  /* set bits according to actual state
   *
   * bits         func
   * 0             st
   * 1             carry
   * 2             trap
   * 3             ovf
   * 4             neg
   * 5             zero
   * <spare>
   * 8,9 rambank      >8
   * 10-13 codebank
   *
   *
   *
   *
   * define PSWSETZERO  x->psw |= 32;
     define PSWCLRZERO  x->psw & =0xffdf      ??
   * define PSWSETNEG   x->psw |= 16;
   * define PSWCLRNEG   x->psw &= 0xffef;


   *
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
         if (val > (int) get_sizemask(size)) s->psw |= 0x2;    // set CY  camk[size]
        }
    if (mask & 8)
        {           // OVF
         s->psw &= 0x37;          // clear OVF
         if (val > (int) get_sizemask(size)) s->psw |= 0x8;      // set OVF
        }

     if (mask & 4)
        {           // OVT - don't clear, just set it.
         if (val > (int) get_sizemask(size)) s->psw |= 0x4;      // set OVT
        }
}



int find_vect_size(SBK *s, INST *c)                  //, int ix)
{
    // see if a cmp can be found with register.
    // go back to see if we can find a size
    // allow for later ldx to swop registers

// IX is always 4 ??

// could add nrml here too (would be max 32)

      int size, xofst;
      uint reg;

      size = 0;
      reg = c->opr[4].addr;                   // ix
      xofst = c->ofst;

      while (xofst)
         {         // look for a cmp or ldx at max 10 instructions back
          xofst = find_opcodes (0, xofst, 10, 2, 10, 12);

          if (xofst)
            {
             if (sinst.sigix == 12 && sinst.opr[2].addr == reg)
              {  // ldx from another register, change register
                reg = sinst.opr[1].addr;
              }

             if (sinst.sigix == 10 && sinst.opr[2].addr == reg)
              {  // cmp
               if (sinst.opcsub == 1)
                 {                             // found cmp for register
                  size = sinst.opr[1].val;     // immediate compare, use size
                  return size;
                 }
              }
            }
         }

return size;
}

/*
uint pop_local(SBK *s)
{
   int i;
   uint ans;
   ans = 0;
for (i = 7; i >= 0; i--)
 {
  if (!(s->stack[i] & 0x1000000))
   {
     ans = s->stack[i] & 0xffffff;
     s->stack[i] |= 0x1000000; // mark as pop ped
     break;
   }
 }
 return ans;
}


void push_local(SBK *s,uint add)
{
   int i;
   uint ans;
   ans = 0;
for (i = 7; i >= 0; i--)
 {
  if (!(s->stack[i] & 0x1000000))
   {
     ans = s->stack[i] & 0xffffff;
     s->stack[i] |= 0x1000000; // mark as pop ped
     break;
   }
 } //return ans;
}
*/

 // TEST !!!
 // s->popaddr = o->addr;


//size = 0;         //  size of args in bytes
//i = 0;            // level of extract 1 = this sub....

//  DBGPRT(1,"%x PUSH @@@@ %x",c->ofst, o->val);

// if pop ped x->pushaddr != 0;
  //or use signature instead ???  but this won't handle jumps very elegantly. But most would match a sig though.....er....

//put this in first, then worry about others...............
//do withins....for loop if jump or call then SCAn that.....
// s->stkptr->nextaddr should be the correct args address.
// no, if matches, simply marrk for emulate, and keep where...............
// if there's another pop in here...........sort out levels as well
//flag ANY change ????
// pop - my args                   //how to do this reliably though....
// pop2  callers args
// pop3 grancallers args  etc.
 /*/ next opcode is ALSO a push...........ignore inner do the two together to get 'level'
 int check_arg_window(uint start,uint end)
 {//forwards....
   uint xofst, ans;
   const OPC *opl;
   xofst = start;                //where pop is
   ans = 1;

 while (xofst && ans)
    {
      xofst = find_opcode(1, xofst, &opl);
      if (xofst >= end) break;
      if (opl->sigix == 15 || opl->sigix == 14) break;    // hit a pop (15) or push (14)
      if (opl->sigix != 12 && opl->sigix != 13) ans = 0;  // only ldx and stx allowed
    }

 if (!xofst) ans = 0;

return ans;
 }



 void find_pop_window (SBK *s, INST *c)
 {
   // called BEFORE the actual push update.
   // find type of arg getter.

   uint   xofst;
   const OPC *opl;
   INST   *xinst;               // malloc so nested calls don't mess up previous level
   AGT    *a;
   xofst = s->curaddr;          // where this push is

  // xinst = (INST*) mem(0,0,sizeof(INST));     // get new instance for code srach

   a = args + s->stkptr;

   // NB. pop clears args struct and sets
   // scb, reg, pop and start addr (pop location)

   a->end = s->curaddr;           // where this push is.  matching pop is at a->start
   a->type = 5;                    //default to fixed args
   a->jmpdest = 0;
    // this reg should match the stack entry. Error if not

   if (c->opr[1].addr != a->reg)
        {

          #ifdef XDBGX
            DBGPRT(1,"&&& %x POP/PUSH reg %x != %x !!!", s->curaddr, c->opr[1].addr, a->reg);
          #endif
            return;
        }

   // check what type of arg extract

   //could go forwards here, as we know where the pop was, and where the push is....
   // may be neater for logic to do this !!

//this little loop here is repeated for pop-push.
// on the basis that the PUSH will be handled innermost FIRST...(but this is reversed from true order)
// mybe SKIP inner loops using start->end ??

// MORE HERE for fixed, variable djnz, ...,  emulate.


   while (xofst)
    {
      xofst = find_opcode(0, xofst, &opl);   // backwards
      if (xofst < s->start)
          {
            a->reg = 0;
            break;
          }
      do_one_opcode(xofst, &tmpscan, xinst);

      //push(immediate) check?  breaks rules (CARD)

// this should become another layer, so can be used for 728x etc.
// have a 'extra layer' type for a pop-push wrapper....

    if (opl->sigix == 14 && !xinst->opcsub)
       {    // PUSH (reg)    //hit another push (register)                //but what about CARD, it does push for immediate
         uint treg;
      // find the matching pop, skipping inner bincode loop
         treg = xinst->opr[1].addr;         //pushed reg

        while (xofst >= s->start)
            {
             xofst = find_opcode(0, xofst, &opl);   // backwards
             if (xofst < s->start) break;

             do_one_opcode(xofst, &tmpscan, xinst);

             if (opl->sigix == 15 && xinst->opr[1].addr == treg) break;
           }
        }




     if (opl->sigix == 15 && !xinst->opcsub)
         {        // POP (reg)                   //done, in effect
            if (xinst->opr[1].addr == a->reg)
             {
                 //check jump
                if (a->jmpdest && (a->jmpdest > a->start || a->jmpdest < a->end)) a->type = 6;
              break;            //return xofst;}
             }
         }

      if (opl->sigix == 18)  a->jmpdest = xinst->opr[2].addr;   // djnz
      else
      if (opl->sigix < 12 || opl->sigix > 15)  a->type = 7;   // emulate

      if (opl->sigix == 12)  a->argr[a->cnt++] = xinst->opr[3].addr;        // multiple over the pop->push (if inc ??? to add a byte ADT ??)

//ldx stx  ...

//NB this doesn't work for 77be, need to track the stb as well..... '++'  may be a useful check ????
// if not an inc, look for stb ?

    // && opl->sigix != 13) args[level].var = 1;          //12 = ldx   mark emu ans = 0;  // only ldx and stx allowed log arg registers here.........
     //no can have encode and others in here must check for actual jumps/rets....
    }

mfree(xinst,sizeof(INST));
// return xofst;
 }


// called only from a PUSH and after that push.

//chk_spf then does actual args assign with ADT.... or emulate ???


// for now, probably give up anything more and just emulate and get it done..........


 void find_args (SBK *s, INST *c)       //uint reg, uint ofst)       //, uint lev)
  {
    //called BEFORE the push,

    int cnt, lev, i;
  //  uint ans;
    SBK *x;
    SPF *f;
    void *d;
    AGT *r;
 //   SUB *t;

    if (!s) return ;


DBGPRT(1," in find args [%d]", s->stkptr);

lev = 0;

i = s->stkptr;

r = args + i;

while (r->scb && r->pop)
{
   if (r->scb != s->stack[i].sc) break;
   if (r->scb) DBGPRT(1,"%d %x %d (%x -> %x)" ,i, r->scb->start, r->pop, r->start, r->end);

 lev++;
 i --;
 i &=7;
 r = args + i;

 if (lev > 5) break;         // safety for any dodgy loop
}

//find_dst(s,1);               //test





  //  ans = 0;

    x = s->stack[s->stkptr].sc;        // correct for current push. BUT not for emulate ??

     if (x)
     {

       r = args + s->stkptr;
    //   t = get_subr(s->subaddr);                  //safety

//but still need to add an spf to 3695 for future calls to it (3654)..


     //  if (r->scb && r->scb->subr) t = r->scb->subr;        //this is right, but need level for 3695 anyway.

       if (r->reg)
         {
          #ifdef XDBGX
            DBGPRT(0,"&&& [%d] %x R%2x found ", s->stkptr, x->start, r->reg);

            if (r->type == 7) DBGPRT(0," EMULATE ");
            if (r->type == 6) DBGPRT(0," Variable ");
            if (r->type == 5) DBGPRT(0," Fixed ");

          #endif

          cnt =  g_word(r->reg) - nobank(x->nextaddr);
          if (cnt > 0 && cnt < 16) { r->cnt = cnt;} else r->cnt = 0;

          #ifdef XDBGX
             if (x) DBGPRT(0," (%x - %x), args=%d",  x->nextaddr, g_word(r->reg), cnt);

//if (r->jmpop == 18)    //djnz



//but even for !r->cnt, add a layer..........





       if (r->cnt) DBGPRT(0," dst=");

             for (cnt = 0; cnt < 16; cnt++)
               {
                if (r->argr[cnt]) DBGPRT(0," R%2x,", r->argr[cnt]);
               }

             DBGPRT(1,0);
          #endif

   //      cnt = 10;                //for an empty pop-push? don't need this if emulate
         if (r->cnt || r->type == 7)
            {

           #ifdef XDBGX
           DBGPRT(1,"[%d] add spf %d to %x lev %d %d args", s->stkptr, r->type, s->start, lev, r->cnt); // OK, works

           DBGPRT(1,"x = %x, %x", x->curaddr,x->start); // OK, works
     //      if (r->jmpop) DBGPRT(1," jto %x,jop %x, jreg %x ", r->jmpdest, r->jmpop, r->jmpreg);
           #endif




//need to queue these in reverse order, to match detection..............
            // y->subr;

           d = get_spf(s);           //get first spf item

           if (!d) d = s;

           f = add_xspf(d, r->type, lev); // was L add_xspf, and add ADT to the spf here ??? this works so far...but not for 3654....
           if (f)
             {
              f->dpars[0] = r->cnt;        //num of args as first par

             }
             s->spf = 1;

             // copy reg destinations ,but reverse the order

              for (cnt = 1; cnt <= r->cnt; cnt++)
                {
                 f->dpars[cnt] = r->argr[r->cnt - cnt];
               }
           }

          }

          r->pop = 0;            //pop not outstanding any more....
     }
   //       i--;
     //     i &= 7;                 //= decstkptr(i);

 //   } while (i != s-> stkptr);         // BUT STKPTR has a valid value !!!
 //  cnt = 0;


 // NOW go back and up the levels for any other args entries ?? how about over 2 subrs ?? matched pairs pop/push ?? maybe...
 // fid = s;

 // while ((s =  get_spf(fid)))
 //   {
 //    prt(0,", [%lx] %lx, %d %d", s , s->fid, s->spf, s->level);
 //    fid = s;

   // }





//return ans;

  }


// A9L now does an encode outside the pop-push ....? how to handle that ???
*/




SBK* pshw (SBK *s, INST *c)
{
  // officially stkptr -=2; [stkptr] = [Rx];
  // so moves stackptr first....


  OPS  *o;
  SBK * newblk;
  int  size, start;
 // RST *g;
 // uint xofst;
 // FSTK *t;
 // RST *r;
  //        SBK *x;

  calc_mult_mode(s,c);



  // does not change PSW

  // if scanning, check for vect pushes first (list or single)
  // immediate value is normally a direct call if valid code addr
  // if indexed or indirect do list check
  // it std register, assume it's a possible argument getter

  // push ( op[1]) or push ( op[4]) if indirect


  o = c->opr + 1;         //source always in [1]

  if (!s->proctype) return s;




  // push can be any of multiple address modes so switch accordingly

  if (s->proctype == 1)            //scanning
    {
     switch(c->opcsub)
      {
        case 1:
          // immediate - treat as a subr call. This is a embedded subr or return address
          // this uses CODE bank, not data. newblk will have caller added in add_scan,
          // but should push it locally as well (for args)

        if (o->imm && val_rom_addr(o->addr))
         {
           newblk = add_scan(s->chn,codebank(o->addr,c), 0, J_SUB, s);
           fake_push(s, 2, newblk);                      //push onto THIS blk as well
         }
       break;


//check for a RET in here to confirm this is a real list...

       case 2:
             // indirect - try to find size and base of list
             // size is same as indexed above, (via cmp)
             // but base must have been added earlier, so look for an ADD, X
             // where x is immediate
             // [4] is saved register
            //  check_vect_list (SBK *caller, uint start, uint sz, INST *c)

            start = find_list_base(c, 10);           //4,10);
            size = find_vect_size(s, c);           //, 4);
            if (start) size = check_vect_list(s, start, size, c);   //if returns 0 not a vect list
            if (!size) fake_push(s,1,0);

           break;

      case 3:

           // indexed - try to find size and 'base' of list
           // ops[0] is base (fixed) addr.  ops[4] (copy of ops[1]) is saved register.
           // go backwards to see if we can find a size via a cmp
           // only look for list if [0] is a valid address,

           // try [1] too ?? a combined address may be the right one

           start = databank(c->opr->addr,c);               // add bank first
           if (val_rom_addr(start))
            {
             size = find_vect_size(s, c);             //, 4);
             size = check_vect_list(s, start, size, c);
            } else size = 0;
           if (!size) fake_push(s,1, 0);

         break;


      default:        // direct register, opcsub zero


//doesn't seem to work for eqe3

      start = check_args(s, o);
      fake_push(s,1,0);               //and does check args and marks emu, args

     #ifdef XDBGX
      DBGPRT(1, "%x ZFZF %d args", s->curaddr,start);
      #endif
      clr_pop(o->addr);
      // g = get_rgstat(o->addr);
      // if (g) g->popped = 0;
      break;
     }    //end switch
    }    // end of if scanning



//probably have to do a fake push for ALL cases, assuming that POP applies everywhere too.



  if (s->proctype == 4)                //emulating)
    {   // similar to push above, but do args if necessary
        // note that as this is done after each scan or rescan,
        // the stack is already built by sj subr....

     if (o->imm && val_rom_addr(o->addr))    //opcsub = 1
       {   // immediate value (address) pushed
          emuscan = add_escan(codebank(o->addr,c),s);          // add imd as a special scan with s as caller
          fake_push(s, 2, emuscan);                            // imd push for local block
       }

   if (o->rgf &&  !c->opcsub)         //same as proctype 1 to start with
         {
          // register - check for argument getter

          //mark arg registers in here somehow ???? use popreg ??

          start = check_args(s, o);  // need original address in popped reg to do the args. or do in check_args

          fake_push(s,1,0);               // and does check args and markes emu, args
         // g = get_rgstat(o->addr);
         // g->popped = 0;                //clear popped but keep other details
         clr_pop(o->addr);

      //    if (i) DBGPRT(1, "%x EMU ZFZF %d args from %x", s->curaddr,i, o->val);

          //if (i) do _args here.............

        }






/*


     else
       {
        for (i=0; i < STKSZ; i++)
            {
       //      t = emulstack+i;
             if (t->type == 1 && t->reg == o->addr)
              { // found
                t->popp ed = 0;        // clear popped.
                t->newadd = o->val | g_bank(t->origadd);
      //          do_args(s,t);
                r = get_rgstat(o->addr);
                if (r)
                  { r->pop ped = 0;
                    if (t->newadd == t->origadd)
                     { // no change, delete it
                      #ifdef XDBGX
                      DBGPRT(1,"delete rgstat R%x", o->addr);
                      #endif
               //       chdelete(CHRGST,get_lastix(CHRGST),1);
                    }
                  }
                break;
              }
            }
       } */
    }


     #ifdef XDBGX
 //   DBG_stack(s->emulating);

    DBGPRT(0,"%x @@@ PUSH [%d] (%d) %x = %x",c->ofst, s->stkptr, c->opcsub, o->addr, o->val);
   // if (s->substart) DBGPRT(0," sub %x", s->substart);
    DBGPRT(1,0);

 #endif


return s;


}



SBK* popw(SBK *s, INST *c)
{
   // does not set PSW
   // Officially,  Rx = [stkptr];  (SP)+=2;
   // done in fake pop() via sbk stack

   OPS  *o;

 #ifdef XDBGX
   uint sp;             //keep ptr before pop
   sp = s->stkptr;
 #endif

 //copy [1] -> [3] for correct print
   calc_mult_mode(s,c);

   cpyop(c,3,1);         //copy op to [3] as write

   if (!s->proctype) return s;


  // cpyop(c,3,1);         //copy op to [3] as write

   o = c->opr+3;      // now a write operand
   o->val = 0;        // safety


//but if indirect,need more here...............or in print.


   fake_pop(s,o);

#ifdef XDBGX
   DBGPRT(0,"@@@ %x ",c->ofst);
   DBGPRT(1," POPW [%d] %x=%x", sp, o->addr, o->val);
#endif
   return s;
}




SBK* clr (SBK *s, INST *c)
  {
    // clear
  calc_1op(s,c);

  c->opr[3].val = 0;
  c->opr[1].val = 0;        //clear src val too

   if (s->proctype == 4)
      {
       pset_psw(s, 0, 0x1a);      // clear bits N,V,C
       pset_psw(s, 1, 0x20);      // set Z
      }
return s;
  }

SBK* neg  (SBK *s, INST *c)
  {
      // negate

    calc_1op(s,c);

    c->opr[3].val = -c->opr[1].val;

    if (s->proctype == 4) set_psw(s, c->opr[3].val, c->opr[3].fend,0x2e);
return s;
 }


SBK* cpl  (SBK *s, INST *c)
  {
    // complement bits
    calc_1op(s,c);
    c->opr[3].val = (~c->opr[1].val) & 0xffff;         // size ?

    if (s->proctype == 4)            //emulating)
     {
      pset_psw(s, 0, 0xa);      // clear bits V,C
      set_psw(s, c->opr[3].val, c->opr[3].fend, 0x30);     // N and Z

     }

  return s;
  }



void check_R11(SBK *s,INST *c)
{
  // extra handler for write to Reg 11 (bank select)
  // done AFTER assignment, but before upd_watch
  // R11 is not written to by upd_watch

  int val;
  OPS *o;

  if (!s->proctype ) return;

  o = c->opr + 3;

  if ((o->addr & 0x3ff) != 0x11) return;

  val = c->opr[1].val & 0xf;          // 0xff if stack_bank too, but not used here

  if (c->opcsub == 1) val++;           //if imd is this default data bank ??

  upd_ram(0x11,c->opr[1].val,7);

  // do this only if it's an imd as this is reliable, else ignore it ???

  if (bkmap[val].bok)
    {              // valid bank
     s->datbnk = val;

     #ifdef XDBGX
       DBGPRT(1,"%x R11 Write, DataBank=%d",c->ofst, val);
     #endif
    }
 }


/*
void do_rgargs(INST *c)
{

   RST *org, *nrg;
   OPS *o;


   if (c->opcsub == 1) return;    //ignore immediates

   o = c->opr+4;   // always source register    >>>>>> IS IT ????

   org = get_rgstat(o->addr);

   if (!org)  return;       //not found

   // found source reg - check if popped or arg
   if (org->arg || org->sarg)
       // reg is flagged as arg, so use size
      {
       // firstly set size, and reflect back to source.
       nrg = get_rgstat(org->sreg);

       // if first level arg (next to popped)
       if (org->sarg)
         {
          // this 2nd level arg, copy sce from first level
          //  how to stop the arg flag when left subr.........
          nrg = add_rgstat(o->addr, o->fend, org->reg, org->ofst);
          if (nrg) {nrg->arg = 1; org->arg = 0;
          if (org->enc) {
             nrg->enc = org->enc;   //src is encoded
             nrg->fend = 15;                   // enc is always a word
             nrg->data  = org->data;     // 'start of data' base reg
#ifdef XDBGX
DBGPRT(1,"ORG ENC");
#endif
} }
               }

             #ifdef XDBGX
              if (org) DBGPRT(1,"RG %x=%x to %x", o->addr, o->val, 0);
             #endif

           }
         else
         if (org->popped)
              // reg flagged as popped so make [wop] an arg - would expect this to be indirect or indexed
           {
            nrg = add_rgstat(dsreg, scfend, org->reg, scaddr);          // [1] for address if indirect
            if (nrg)
             {
              nrg->arg = 1;
              nrg->sarg = 1;                // source level from pop

                 #ifdef XDBGX
              DBGPRT(1,"ADD ARG RG %x at %x sz%d", nrg->reg, c->ofst, nrg->fend);
              #endif
             }
           }

}
*/


void add_spf_register (RPOP *p, uint dest, uint sce, uint popaddr)
{
  SPF *f;
  DPAR *d;
  uint ix;

  if (!p || !dest) return;

 // if (!p->argemu)
 //     {
  //        DBGPRT(0," DADADADADADA");
   //       return;
   //   }


if (!p->argemu)   return;




  if (!get_spf(vconv(p->argemu->nextaddr)))
    {
        f = add_spf(vconv(p->argemu->nextaddr),10,1);
        p->fblk = f;
             #ifdef XDBGX
        DBGPRT(1,"add spf 10 to %x", p->argemu->nextaddr);      //p->tgtscan->start);
        #endif
    }
  else f = p->fblk;     // spf in p->fblk.......



  if (f)
    {

      for (ix = 0; ix <= f->argcnt; ix++)
      {
         d = f->pars + ix;

         if (d->reg == dest)
            {
              if (d->refaddr == popaddr)          // already in list. adding refaddr seems to work.
              return;

              if (d->notarg)
                {
                 d->refaddr = popaddr;           // update refaddr only if a transfer arg
                 return;
                }


            }

      }

    if (sce)
      {
        for (ix = 0; ix <= f->argcnt; ix++)
          {
            d = f->pars + ix;

            if (d->reg == sce)
             {
              d->notarg = 1;       //transfer arg.
              ix = d->refaddr;
              break;
             }
          }

      }

     #ifdef XDBGX
      DBGPRT(1,"add arg R%x to spf", dest);
      #endif

      if (f->argcnt >= 15)
      {
               #ifdef XDBGX
           DBGPRT(1,"arg list full !! R%x ",dest);
           #endif
           return;
      }




    //  if (f)
     //   {
          d = f->pars + f->argcnt;
          d->reg =  dest;        //add destination to args list .. args list should be for caller ???
          if (sce) d->refaddr = ix; else d->refaddr = popaddr;
          d->notarg = 0;
          f->argcnt++;
 //       }


    }
}

/*
void kill_spf_register (RPOP *p, uint addr)
{
  SPF *f;
 // SBK *s;
  DPAR *d;
  uint ix;

  if (!p || !addr) return;

  f = p->fblk;     // spf in p->fblk.......

  if (f)
    {

      for (ix = 0; ix <= f->argcnt; ix++)
      {
         d = f->pars + ix;

         if (d->reg == addr)
           {
             d->notarg = 1;       // replace scaddr with addr
             return;
           }
      }


    }
 }

*/













SBK* stx (SBK *s, INST *c)
  {
   /* NB. this can WRITE to indexed and indirect addrs
    * but addresses not calc'ed in print phase
    * does not change PSW
    *
    * SWOP operands so that everything else works more easily...
    */

  STACK *x;
  OPS  *o;
  RST  *sce, *dst;    //, *h;
  int inx;        //,pp;
  uint pp;
//  SBK *k;
  //  SPF *f;
//  void *fid;
  RPOP *p;




  calc_mult_mode(s,c);

  //copy [1] -> [3] for correct print

  c->opr[1].val = c->opr[2].val;            // opposite to ldx
  cpyop(c,3,1);                             // remake calc to [3] = [2] to match others

 if (!s->proctype) return s;

 //    if (c->opcsub == 1) return s;        //not for immediates beyond here   (STX does NOT have immediates....does it ever happen ???)
//if indecrt stx from argument........bins all seem to do (popped r46
// 8a548: b2,47,3f           ldb   R3f,[R46++]      R3f = [R46++];
// 8a54b: c6,27,3f           stb   R3f,[R26++]      [R26++] = R3f;
// addr[3] (or [1]) is destination
// addr [2] is sce....(3b) which has arg set
//addr[4] is the indirect R16 (and indirect flag and inc) and val[4] is POST INCREMENT, but [3] is NOT

//is this AFTER any increment ??

  dst = get_rgstat(c->opr[3].addr);           // destination (new)
  sce = get_rgstat(c->opr[2].addr);           // source


  if (c->opcsub > 1 && dst && sce)
   {                   //indexed or indirect
      if (sce->sreg)
      {
        if (regstat[sce->sreg].popped)
          {
            p = get_rpop(regstat[sce->sreg].pinx);         // from sce register

            if (!p->fblk)
             { // test for some kind of cockup
     //            DBGPRT(0,0);

             }
            dst->arg = 1;
            dst->sreg = sce->sreg;
            dst->fend = c->opr[3].fend;
            sce->arg = 0;
            add_spf_register(p, c->opr[3].addr, c->opr[2].addr, 0);



 // , c->opr[2].addr, 0);   // dst, sce.  Replace sce with dst.
          }
  //      else sce->sreg = 0;            // sce is not popped any more
      }

   }


  // now check if this is a multibank type stack operation
  // if it this a stack register, treat similarly to a PUSH
  // indirect, indexed always put base in [4] and offset in [0]


  o = c->opr + 2;             // to check inx register (source)

  if (c->opcsub > 1 && valid_reg(c->opr[4].addr) == 4)
    {     //from ldx

     #ifdef XDBGX
      DBGPRT(0,"NEW PUSH via STX at %x", c->ofst);
#endif
      if (c->opcsub == 2) inx = 0;                    // inr
      if (c->opcsub == 3) inx =  c->opr[0].addr/2;    // inx

      x = get_stack(s, inx);              //, &k, &pp);

     #ifdef XDBGX
       DBGPRT(0, " inx %d, R%x type %d ", inx, c->opr[2].addr, x->type);
       if (x->type == 1) DBGPRT(1,"val %x", x->nextaddr);
       if (x->type == 2) DBGPRT(1,"PSW %x", x->start);
#endif


     pp = check_args(s, o);

   //  if (k) k->popreg = 0;


     sce = get_rgstat(o->addr);

     if (sce && sce->popped)
      {
      //    p = get_rpop(sce->pinx);
        #ifdef XDBGX
       if (pp) DBGPRT(1, "%d args", pp);
       #endif
   //    if (s->proctype == 4) k = p->tgtemu;
   //    if (s->proctype == 1) k = p->tgtscan;

       clr_pop(o->addr);
      }
    }


check_R11(s,c);
return s;
 }





SBK* ldx (SBK *s, INST *c)
 {
   // does not change psw

   int inx;
   RST *g, *h;
   OPS *o, *r;    //, *sce;
 //  SBK *z;
    STACK *x;    //  SPF *f;
   RPOP* p;

 //  uint pp;     //, type;

   calc_mult_mode(s,c);

   o = c->opr+3;                  // destination
   r = c->opr+4;                  // read register before ind or inx

   o->val = c->opr[1].val;        // [3] = [1] as default op nned calc for table and func finder.

   if (!s->proctype) return s;


   check_R11(s,c);

   if (s->proctype == 4)         // emulate mode
    {
        //c->opcsub ???       indirects and indiexes ??

      if (c->opcsub == 1) return s;        //not for immediates beyond here

      g = get_rgstat(r->addr);     // register [4], before any inx/ind, source.

      if (r->inc && g->popped)     // inc set on popped reg, op is indirect. How to get an arg....
        {
         p = get_rpop(g->pinx);        // this works for 3695 A9L (direct copy)
         h = get_rgstat(o->addr);        // destination


         if (c->opcix == 46 || c->opcix == 47)
         {  // LDSBW ldzbw          //fix first destination as byte, always.
           h->fendlock = 1;
           h->fend = 7;
         }

         h->sreg = r->addr;              // source register (which was popped)
         h->arg  = 1;                    // mark destination as an arg
         h->fend = o->fend;

     #ifdef XDBGX
         DBGPRT(1,"%x (ldx) R%x(%x) set as arg for %x",s->curaddr, o->addr,o->fend, p->tgtemu->start);
         #endif

          //   z = get_scan(CHSCAN,g->ptarget->start);        //this trick seems to fix multiples
         if (!p->tgtscan)
           {
                           // z = add_scan(CHSCAN,g->ptarget->start,0,J_SUB,0);
     #ifdef XDBGX
             DBGPRT(1,"ZUZU no scan %x", r->addr);
             #endif
           }       //create one ???

         add_spf_register(p, o->addr, 0, c->opr[1].addr);         //o->addr,0);       //dst, no sce, popped directly
        }            //end emulate only

// further use of args.............

// need to check opcsub ?? done above

if (g && g->arg )      // && c->opcsub != 1
 {                 //source [4] is an argument
     h = get_rgstat(o->addr);        // destination
     if (c->opcsub > 1 && valid_reg(r->val) == 1)   h->sreg = r->val; else   h->sreg = r->addr;
     h->arg  = 1;

     #ifdef XDBGX
     DBGPRT(1,"%x (ldx) R%x(%x) set as arg from arg %x(%x)",s->curaddr, o->addr,o->fend, h->sreg, r->fend);
     #endif
     // //if (c->opcsub > 1) g = get_rgstat(r->val); else g = get_rgstat(r->addr);

//can't do this if arg extract done and dusted !!!

  //   add_spf_register(p,o->addr,0);       // drop r->addr ??  dst, no sce, popped directly

 }





    }

  if (c->opcsub > 1 && valid_reg(r->addr) == 4)
    {
  //    inx = 0;               // -1;                  //redundant ??
      if (c->opcsub == 2) inx = 0;                    // inr
      if (c->opcsub == 3) inx =  c->opr[0].addr/2;    // inx (entry num)


//fake_pop(



     x = get_stack(s, inx);       //, &z, &pp);

   //  g = get_rgstat(o->addr);
                  //  p = add_rgpop(o->addr);

     if (valid_reg(o->addr) == 2 ) o->val = 0;          // zero register
     else
     if (x->type == 1)
      {
       o->val = x->nextaddr;
       find_argblk (s,inx,o);
      }

     else
     if (x->type == 3)
       {
        o->val = x->start;   // & 0xff;
       }       // DBGPRT(1, "PUSHP not expected"); return 0;}

#ifdef XDBGX
      DBGPRT(1,"%x NEW POP via LDX R%x=%x [%d] T%d", c->ofst, o->addr, o->val, s->stkptr+inx, x->type);
      #endif
    }
     return s;
}




SBK* orx(SBK *s, INST *c)
 {

  calc_mult_mode(s,c);

  if (!s->proctype) return s;

  if (c->opcix < 32) c->opr[3].val = c->opr[2].val |  c->opr[1].val;   // std or
  else               c->opr[3].val = c->opr[2].val ^  c->opr[1].val;   // xor


//CHECK if mem-expand set with only one bank indicates something screwy
//if rbase sig found, then can try to find bank 1

 //  orb   Ra,10            MEM_Expand = 1;

if (c->opr[3].addr == 0xa && c->opr[1].val & 0x10)
 {        //Mem_expand bit set
    if (numbanks == 0)
    {
             #ifdef XDBGX
        DBGPRT(1,"%x MEM_EXPAND SET WITH SINGLE BANK !!!!", s->curaddr);
        #endif
        wnprt(1,"##        MEM_EXPAND SET WITH SINGLE BANK !!!!");

        //find bank by data..........

    }
 }

  if (s->proctype == 4)                //emulating)
     {
      pset_psw(s, 0, 0xa);      // clear bits V,C
      set_psw(s, c->opr[3].val, c->opr[3].fend, 0x30);     // N and Z
     }
  return s;
 }


SBK* add(SBK *s, INST *c)
 {
      // 2 or 3 op
   calc_mult_mode(s,c);

//must keep calc for finding tableas and funcs
   c->opr[3].val =  c->opr[2].val + c->opr[1].val;

     if (!s->proctype) return s;

    if (s->proctype == 4)                //emulating) if (s->emulating)
   set_psw(s, c->opr[3].val, c->opr[3].fend, 0x3e);

   return s;
 }


SBK* andx(SBK *s, INST *c)
 {
       // 2 or 3 op
  calc_mult_mode(s,c);

    if (!s->proctype) return s;
  c->opr[3].val =  c->opr[2].val & c->opr[1].val;

  if (s->proctype == 4) pset_psw(s, 0, 0xa);        //emulating)
 // set_psw(2, 0x30);
return s;
 }


SBK* sbt(SBK *s, INST *c)
 {      // 2 or 3 op
    calc_mult_mode(s,c);

  if (!s->proctype) return s;
    c->opr[3].val =  c->opr[2].val - c->opr[1].val;

 // sub is a BORROW not a carry
    if (s->proctype == 4)           // if (s->emulating)
      {
       set_psw(s, c->opr[3].val, c->opr[3].fend, 0x3e);
       s->psw ^= 2;       // complement of carry
      }

      return s;
 }


SBK* cmp(SBK *s, INST *c)
 {
   // use curops[0] as temp holder for subtract.
   // sub is a BORROW not a carry

  calc_mult_mode(s,c);


  if (s->proctype == 4)   // if (s->emulating)
    {
     c->opr[0].val =  c->opr[2].val - c->opr[1].val;
     set_psw(s, c->opr[0].val, c->opr[1].fend,0x3e);
     s->psw ^= 2;       // complement of carry
    }
    return s;
 }

SBK* mlx(SBK *s, INST *c)
 {     //psw not clear - assume no changes; 2 or 3 op
    calc_mult_mode(s,c);

      if (!s->proctype) return s;
    c->opr[3].val =  c->opr[2].val * c->opr[1].val;  // 2 and 3 ops
 // PSW ???
   return s;
 }

SBK* dvx(SBK *s, INST *c)
 {
   // quotient to low addr, and remainder to high addr
   // always 2 op ??
    long x;


    calc_mult_mode(s,c);

      if (!s->proctype) return s;

    x = c->opr[3].val;  // keep original
    if (c->opr[1].val != 0) c->opr[3].val = c->opr[2].val / c->opr[1].val;

    x -= (x * c->opr[3].val);             // calc remainder

    if (c->opr[3].fend > 7)              // long/word
        {
         c->opr[3].val  &= 0xffff;
         c->opr[3].val  |= (x << 16);       // remainder
        }
    else
       {          // word/byte
         c->opr[3].val  &= 0xff;
         c->opr[3].val  |= (x << 8);       // remainder
        }

// PSW ??  for /0 at least.........
   return s;
 }


void calc_shift_ops(SBK *s, INST *c)
 {
   int b;
   c->opcsub = 0;                         // no multimode
   b = g_byte(s->nextaddr+1);             // first data byte after (true) opcode is shift amount
   if (s->proctype == 1) set_ftype_data(s->nextaddr+1,7);

   if (b < 16) op_imd(s,c);                 // convert op 1 to absolute (= immediate) shift value
   op_calc (b, 1,s,c);                      // register or immediate






   op_calc (g_byte(s->nextaddr+2),2,s,c);
     if (s->proctype == 1)  set_ftype_data(s->nextaddr+2,7);

  //if b  [1] more than 31 then is INVALID OPCODE..................

   cpyop(c,3,2);

   s->nextaddr += c->numops + 1;

   chk_inv_operands(s,c);
    }

SBK* shl(SBK *s, INST *c)
 {

   /* manual - The right bits of the result are filled with zeroes.
     The last bit shifted out is saved in the carry flag.
      * sticky appears to be last carry ? (not done)
   */

  long mask, v;

  calc_shift_ops(s,c);


   if (!s->proctype) return s;
  v = c->opr[2].val;
  v <<= c->opr[1].val;   // as a 64 long, for Double Word shifts

  if (s->proctype == 4)              //if (s->emulating)
    {
     set_psw(s, v, c->opr[3].fend,0x30);               // Z and N
     mask =  get_sizemask(c->opr[1].fend) + 1;         // 'size + 1' bitno (= i.e. last shifted out)
     // ^^ simpler way ???? 1 << (fend+1)  ??? do as below ????
     if (v & mask) s->psw |= 2;  else s->psw &= 0x3d;   // set or clear carry flag
    }
  c->opr[3].val =  v;

     return s;
 }

SBK* shr(SBK *s, INST *c)
 {
// arith shift vs uns ??

 /* manual  The last bit shifted out is saved in the carry flag.
   The sticky bit flag is set to indicate that, during a right shift, a “1” has been shifted into the
   carry flag and then shifted out.
*/

  calc_shift_ops(s,c);

 if (!s->proctype) return s;
 // c->psw &= 0x3b;         // clear sticky first

  c->opr[3].val = c->opr[2].val >> (c->opr[1].val-1);   // temp keep penultimate bit

 if (s->proctype == 4)          // if (s->emulating)
    {
       if (c->opr[3].val & 1) s->psw |= 2;  else s->psw &= 0x3d;   // set or clear carry flag (last bit shifted out)
    }

 c->opr[3].val >>= 1;

 if (s->proctype == 4)          //if (s->emulating)
    {
     set_psw(s, c->opr[3].val, c->opr[3].fend,0x30);               // Z and N
    }

       return s;
 }

SBK* inc(SBK *s, INST *c)
 {
   calc_1op(s,c);

     if (!s->proctype) return s;
   // ALWAYS 1 (only autoinc is one or two)
   c->opr[3].val++;

   add_dtk(s,c);
   if (s->proctype == 4)           //if (s->emulating)
     {
      set_psw(s, c->opr[3].val, c->opr[3].fend, 0x3e);
     }
   return s;
 }

SBK* dec(SBK *s, INST *c)
 {
    calc_1op(s,c);
   // ignore for scans

  if (!s->proctype) return s;
      c->opr[3].val--;
    if (s->proctype == 4)         //if (s->emulating)
     {

      set_psw(s, c->opr[3].val, c->opr[3].fend, 0x3e);
     }
        return s;
 }


void calc_jump_op(SBK *s, int jofs, INST *c)       //type ??
  {
    // s->nextaddr must correctly point to next opcode.
    // returns jump OFFSET. Address in op[1]
    // add jump details too ??

     OPS *o;

     SPF *f;
     c->opcsub = 0;                           // no multimode allowed (safety)

     o = c->opr+1;                            // set up opr[1] as a jump (= address)
     o->adr = 1;
     o->rgf = 0;
     o->rbs = 0;
     o->val = jofs;       // c->opr->val = jofs;                      // save actual offset.
     o->addr = jofs + s->nextaddr;
     o->addr = codebank(o->addr, c);                              // set CODE bank for jumps


// if unclosed spf of 10, flag the jump

  f = find_spf(s,10,0);

 if (f) {
          #ifdef XDBGX
     DBGPRT(1,"ZJZJ blk %x, %x", s->start, s->curaddr);
     #endif
     f->jmparg = 1;      // this requires spf to be changed at the push...
 }
  //   type = get_oldstack(s, 0, &x, 0); ---- doesn't work

  //   if (type && type < 3)
  //     {
    //     if (x->popreg)
 //s->jmparg = 1;
   //    }

//must scan up the stack to find if anything is popped..............
// s->stkptr  ?? if (get_oldstack(s, 0, sc, 0) < 3);   sc is entry with popreg set.        s->stkptr ?
   //  g = get_rgstat(o->addr);          // o->addr is an ADDRESS !!!
   //  if (g->argsblk) s->jmparg = 1;              // mark a jump for argument emulation
  }


uint check_backw(uint addr, SBK *caller, int type)
{
// check if jump backwards for possible loop condition

  uint ans;
  int jump;

  ans = 0;           // continue

// basic address stuff first................

   jump = addr - caller->curaddr;                         // difference

  if (bankeq(addr, caller->curaddr))                      // bank check
     {                                                    // in same bank
      if (addr == caller->nextaddr) ans = 1;              // dummy jump, ignore for scans
      if (addr == caller->curaddr)  ans = 3;              // loopstop

      // backward jump, scanned already - is a loop   NOT IF IN EMULATE !!!
    if (caller->proctype == 1)
    {
      if (jump < 0 && get_ftype(addr) == 1)
        {
          if (type == J_COND) ans = 2;
          if (type == J_STAT && addr > caller->start)  ans = 2;   //only within same block NOT IF EMULATE !!
        }
    }

      if (type == J_STAT)
       {
        // check for bank start, safety loopstops and opening jumps.
        if (nobank(addr) < 0x2010) end_scan (caller);                // faulty - end block

         if (jump == -1)
           {
            //some bins do a loopstop with di,jmp...............
            if (g_byte(addr) ==  0xfa) ans = 3;
           }
       }
     }

  if (ans > 2 && caller->proctype == 4)            //emulating)
    {    // only check for real loopstops in emulate
        #ifdef XDBGX
     DBGPRT(1,"Loopstop detected (%d) at %x", ans, caller->curaddr);
     #endif
//  caller->stop = 1;   why ?? rejected anyway
    }

  return ans;
}



SBK *do_cond_scan(SBK *caller, INST *c)          //, int type)
{
  /* do conditional jump, new scan
   * necessary for some code sections where scan will not pick up new address,
   * even though current block does not stop
   */

  SBK *newscan;                      // new scan, or temp holder
  int addr;

  if (get_cmdopt(OPTMANL)) return caller;                // no scans, use default
  if (!caller->proctype) return caller;           // just a code search

  addr = c->opr[1].addr;

  add_jump(caller, addr, J_COND);             // add jump BEFORE checks (for loopstops)

  if (check_backw(addr, caller, J_COND)) return caller;              // backward jump probably a loop

  if (get_cmdopt(OPTLABEL)) new_autosym (2, addr);                      // add autolabel if reqd

  newscan =  add_scan (caller->chn, addr, 0, J_COND, caller);    // where cond continues

  scan_blk(newscan,c);


//can't scan without a jcaller...............to return to

/*/always scan instead ??
          if (get_lasterr(C_SCAN) ==  E_DUPSCAN)
              {
                scan_blk(newscan,c);                 //scan forced if stack not match
                add_scan (addr, 0, type, caller);
              }

          // NOT do_spf ?? probably, but cond probably always work ?? or in scan_blk
          break;

*/
 return caller;
}





SBK* cjm(SBK *s, INST *c)
 {    // conditional jump, (short)
    // all types here based upon psw

   s->nextaddr += 2;
   calc_jump_op(s,g_val(s->nextaddr-1,0,39),c);   //signed byte offset

   if (s->proctype == 1)
    {
    set_ftype_data(s->nextaddr-1,39);
    do_cond_scan(s, c);
    return s;
    }

if (s->proctype == 4)             // if (s->emulating) {
 {
// do actual jump if emulating

if (check_backw(c->opr[1].addr,s, J_COND)) return s;       // don't honour infinite loops




switch (c->opcix)
   {

    case 54:            // JNST      STK sticky is 1
       if (!(s->psw & 1))   s->nextaddr = c->opr[1].addr;
       break;

    case 55:            // JST
      if ((s->psw & 1))   s->nextaddr = c->opr[1].addr;
       break;

    case 56:            // JNC       CY carry is 2
       if (!(s->psw & 2))   s->nextaddr = c->opr[1].addr;
       break;

    case 57:            // JC
       if (s->psw & 2)   s->nextaddr = c->opr[1].addr;
       break;

    case 58:            // JNV      OV overflow is 8
       if (!(s->psw & 8))   s->nextaddr = c->opr[1].addr;
       break;

    case 59:            // JV
      if (s->psw & 8)   s->nextaddr = c->opr[1].addr;
       break;

    case 60:            // JNVT  OVT overflow trap is 4
       if (!(s->psw & 4))   s->nextaddr = c->opr[1].addr;
       break;

    case 61:            // JVT
       if (s->psw & 4)   s->nextaddr = c->opr[1].addr;
       break;

    case 62:            // JGTU  C (2) set and Z (0x20) unset
       if ((s->psw & 0x22) == 2)   s->nextaddr = c->opr[1].addr;
       break;

    case 63:            // JLEU  C unset OR Z set
       if ((s->psw & 0x22) != 2)   s->nextaddr = c->opr[1].addr;
       break;

    case 64:            // JGT  N (16) unset and Z (32) unset
       if (!(s->psw & 0x30))   s->nextaddr = c->opr[1].addr;
       break;

    case 65:            // JLE  N set OR Z set
      if (s->psw & 0x30)   s->nextaddr = c->opr[1].addr;
       break;

    case 66:            // JGE  N not set N is 16
       if (!(s->psw & 16))   s->nextaddr = c->opr[1].addr;
       break;

    case 67:            // JLT  N (negative) is set
      if (s->psw & 16)   s->nextaddr = c->opr[1].addr;
       break;

    case 68:            // JE   (Z set)  Z is 32, zero flag
       if (s->psw & 32)   s->nextaddr = c->opr[1].addr;
       break;

    case 69:            // JNE    (Z not set)
      if (!(s->psw & 32))   s->nextaddr = c->opr[1].addr;
       break;

    default:
       break;
   }

#ifdef XDBGX
if (s->nextaddr == c->opr[1].addr)      // debug confirm jump taken
  {
   DBGPRT(1,"cjmpE %x->%x (psw=%x)", c->ofst, c->opr[1].addr, s->psw);
  }
#endif

}   // end of emulating
return s;
 }





SBK* bjm(SBK *s, INST *c)
 {     // Bit Jump.  JB, JNB,
     //  add bit operand but don't change opcode size
    int xofst;
    xofst = s->nextaddr;
    s->nextaddr += 3;

    c->opr[3].bit = 1;           //only place this is set.......// setup bit number in op3
    c->opr[3].rgf = 0;
    c->numops++;                            // local change, NOT opcode size.
    op_calc (g_byte(xofst) & 7, 3,s,c);       // bit number (in opcode)
    op_calc (g_byte(xofst+1), 2,s,c);         // register
     if (s->proctype == 1)   set_ftype_data(xofst+1,7);
    calc_jump_op(s, g_val(xofst+2,0,39),c);   // jump destination, signed byte

    if (s->proctype == 1)         //if (s->scanning)
      {
       set_ftype_data(xofst+2,39);
       do_cond_scan(s,c);          //sjsubr(s, c, J_COND);
       return s;
      }

  //  0 is LS bit .... jnb is 70, jb 71

  if (s->proctype == 4)            //if (s->emulating)
    {

     if (check_backw(c->opr[1].addr, s, J_STAT)) return s;          //NEED THIS !!!!

    xofst = (1 << c->opr[3].val);    // set mask from BIT

    if (c->opcix & 1)
      {
        // JB
        if (c->opr[2].val & xofst)  s->nextaddr = c->opr[1].addr;     // bit set
      }
    else
     { //JNB
      if (!(c->opr[2].val & xofst)) s->nextaddr = c->opr[1].addr;    // / bit not set
     }


  /* bit set and JB
  if ((c->opr[2].val & xofst) && (c->opcix & 1)) s->nextaddr = c->opr[1].addr;      // jb
  // bit not set and JNB
  if (!(c->opr[2].val & xofst) && !(c->opcix & 1)) s->nextaddr = c->opr[1].addr;    //jnb  */

   }
   return s;
 }



SBK * do_stat_scan(SBK *caller, INST *c)
{
  /* do static jump - only called in proctype = 1
   * backwards static jumps are also probably a code loop, but some bins
   * have a common 'return' point, so can't assume this.
   * A static jump can also goto a subroutine, as per signed and
   * unsigned function and table lookups, and so on.
   */

   SBK *newscan;                  // new scan holder

  uint addr, ans;
 // uint agc;

  if (get_cmdopt(OPTMANL)) return caller;                // no scans, use default
  if (!caller->proctype) return caller;

  addr = c->opr[1].addr;

  add_jump(caller, addr, J_STAT);


//check if this jump is inside a cond - if so, it MUST be an ELSE or OR,AND type construct

            //j =

 /*          if (f->jtype)
      {     // static and cond jumps
       tend = g_byte(f->toaddr);
       if (tend == 0xf0 || tend == 0xf1)
         {
          f->retn = 1;
   */


  //       do_subr_end(caller, addr, type);
 ans = check_backw(addr, caller, J_STAT);

 if (ans != 1)  end_scan (caller);                         // END scan of this block unless dummy...

 if (ans) return caller;                                   // backward jump, probably a loop, ignore




         if (get_cmdopt(OPTLABEL)) new_autosym (2, addr);                 // add autolabel if reqd

         newscan = add_scan (caller->chn,addr, 0, J_STAT, caller);          // new or duplicate scan

         scan_blk(newscan,c);     // scan immediately


  if (newscan)
     {
      //  valid block, may be a duplicate
    //  caller->nextaddr += newscan->adtsize;       ??        // for user cmds, adt attached to scan block. not valid for jumps ??

  // reset emu level (every time, in case subr called at different level)

       if (newscan->emureqd)
         {
          caller->emureqd = newscan->emureqd;    // at same level for jumps...........
               #ifdef XDBGX
          DBGPRT(1,"wqwqj set %x emu from %x ", caller->start, newscan->start);
          #endif
         }

     }

//check for anything jumping to here with a spf, and copy it

  check_spf(newscan, J_STAT);        //,type,caller);  needs to be with the scan as well ???

/*jump can also go to an emulate ??  try and see............but this should probably be CALLER ??


      if (newscan && newscan->emulreqd)     // && !caller->emulating)
               {
             //    newscan->caller = caller;         // reset for stack rebuild
                 saveddbnk = datbnk;
                 agc = emulate_blk(newscan);             // and emulate it



      // but should NOT end if goto is in a cond......as this is an else....

//need to check via JUMPS




    //     scan_blk(newscan,c);                              // because of args ???
// }

 // check inheritance ??  for static jumps to funclu AND args...........
 // get subr(addr) ... if certain types....
*/


return caller;
}






SBK* sjm(SBK *s, INST *c)
 { //  a short jump
   //  CANNOT do this as part bit field as it is NOT std.

    int jofs;

    jofs = sjmp_ofst(s->nextaddr);

 //    if (s->proctype == 1)   set_ftypedat(s->nextaddr+1,7);

    s->nextaddr += 2;

    calc_jump_op(s, jofs,c);

    if (s->proctype == 1)              //if (s->scanning)
      {
       set_ftype_data(s->nextaddr-1,7);
       do_stat_scan(s,c);      //sjsubr(s, c, J_STAT);
       return s;
      }

   if (s->proctype == 4)                   //if (s->emulating)
    {
     if (check_backw(c->opr[1].addr, s, J_STAT)) return s;
     s->nextaddr = c->opr[1].addr;      // go direct in emu mode
    }
   return s;
 }


SBK* ljm(SBK *s, INST *c)
 {      //long jump
   int jofs;

   s->nextaddr += 3;
   jofs = g_val(s->nextaddr-2,0,47);        //signed word
   calc_jump_op(s, jofs,c);

   if (s->proctype == 1)          //if (s->scanning)
     {
           set_ftype_data(s->nextaddr-2,47);
       do_stat_scan(s,c);          //jsubr(s, c, J_STAT);
       return s;
     }

  if (s->proctype == 4)          //if (s->emulating)
    {
     if (check_backw(c->opr[1].addr, s, J_STAT)) return s;
     s->nextaddr = c->opr[1].addr;      // go direct in emu mode
    }

       return s;
  }

/*
int cnt_extra_pops(SBK *s)
{
       //   ...for pop-call-push tricks... add to emulevel (e.g. 7aa1)
  int ans, ix;
  STACK *k;

  if (s->stkptr > s->origstkptr)   //testing, move into loop
  {
   ans = 0;

  for (ix = s->origstkptr; ix < s->stkptr; ix++)
    {
     k = s->stack + (ix & 7);  // auto wrap
     if (k->type < 3) ans++;   // skip pushp
    }

  }
  return ans;
}

  //       DBGPRT(1,"wqwq STACK!! +%d (%d,%d)", caller->stkptr - caller->origstkptr,caller->stkptr,caller->origstkptr);
   //         caller->emulevel += (caller->stkptr - caller->origstkptr);
              caller->emulevs <<= (caller->stkptr - caller->origstkptr);


            DBGPRT(1,"wqwq set %x emu=%d", caller->start, caller->emulevs);

//but if caller is tagged, what do we do now? can't carry on............
           }
//if (caller->emulevel == 1) emulate_blk(....um....addr, grandcaller);
         }
*/


SBK* do_sub_scan(SBK *caller, INST *c)           //, int type)
 {
   /* do subroutine or jump
   * new scanned block (or found one) may become holder of this block
   * for subroutine special funcs (and any args)
   * only called in scan type 1
   */

   SBK *newscan;
   int addr;

   if (get_cmdopt(OPTMANL)) return caller;                // no scans, use default
   if (!caller->proctype) return caller;

   addr = c->opr[1].addr;

   #ifdef XDBGX
  //  if (anlpass < ANLPRT)
        DBGPRT(1,"call %x from %x", c->opr[1].addr, c->ofst);
    #endif






   add_jump(caller, addr, J_SUB);

   if (check_backw(addr,caller,J_SUB)) return caller;

   newscan = add_scan (caller->chn, addr,0, J_SUB, caller);          // nsc = new or zero if dupl/invalid

   #ifdef XDBGX
   DBGPRT(1,0);       // add newline to separate scans in debug
   #endif

   scan_blk(newscan,c);     // always scan immediately.

   if (newscan)
     {
      //  valid block, may be a duplicate

      if (newscan->levf)
        {
            #ifdef XDBGX
         DBGPRT(1,"LEVFF! %x", newscan->start);
         DBGPRT(1,"Mark %x as emu", caller->start);
         #endif
         // as a temp fix, mark the caller as emu to see if it works..........seems to work for CARD and A9L !!!
         caller->emureqd = 1;
         caller->estop = 1;
        }

//stop emuls if this is set....

      caller->nextaddr += newscan->adtsize;               // for user cmds, adt attached to scan block.

      if (newscan->getargs)
        {
#ifdef XDBGX
          if (!caller->emureqd) DBGPRT(1,"wqwq set %x emu from args %x", caller->start, newscan->start);
#endif
          caller->emureqd = 1;           //(newscan->emulevs >> 1);                  // - 1;         // TEST !! drop bitmask down by one
          caller->estop = 1;
        }


    //  if (get_ftype(caller->nextaddr) == 4)
    //   {                                         //breaks eqe3
    //       #ifdef XDBGX
    //    DBGPRT(1,"ignore emulate, already done");
    //    #endif
    //    return caller;
   //    }


     if (newscan->emureqd)
       {
        emulate_blk(addr, caller);
       }

     }

   #ifdef XDBGX
        DBGPRT(1,"Resume at %x (%x)",caller->nextaddr, caller->start);
        DBGPRT(1,0);
   #endif

   return caller;
}






SBK* cll(SBK *s, INST *c)
 {     // long call
    SBK *n;

 //   s->rambnk = 0;         //safety

    s->nextaddr += 3;
    calc_jump_op(s,g_val(s->nextaddr-2,0,47),c);         //signed word


 /*  if (s->lscan)
    {
      #ifdef XDBGX
       if (anlpass < ANLPRT) DBGPRT(1,"ignore call %x from %x", c->opr[1].addr, s->curaddr);
      #endif
      return;
    }
*/

   if (s->proctype == 1)          // if (s->scanning)
      {
             set_ftype_data(s->nextaddr-2,47);

       do_sub_scan(s,c);            //sjsubr(s, c, J_SUB);
       return s;
      }

  if (s->proctype == 4)          //if (s->emulating)
    {
  //     SUB *sub;
       n = get_scan(CHSTST,c->opr[1].addr);                         //ubr(c->opr[1].addr);
       if (n && n->adtsize)           //sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opr[1].addr, n->adtsize);
           #endif
           s->nextaddr += n->adtsize;               //sub->size;
           return s;
         }

     #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opr[1].addr, s->curaddr);
     #endif

//but this does NOT WORK if address done with a PUSH (CARD)
     n = add_escan(c->opr[1].addr, s);

     scan_blk(n, &einst);                      // recursive scan - need to fix this

// break here ???  if args set ??
return n;             //new block ??        END_SCAN MUST MATCH !!

    }

      return s;
 }


SBK* scl(SBK *s, INST *c)
 { // short call
   int jofs;
   SBK *n;

 //  s->rambnk = 0;         //safety
   jofs = sjmp_ofst(s->nextaddr);

 //   if (s->proctype)   set_ftypedat(s->nextaddr+1,7);
   s->nextaddr+=2;
   calc_jump_op(s,jofs,c);

 /*  if (s->lscan)
    {
      #ifdef XDBGX
       if (anlpass < ANLPRT) DBGPRT(1,"ignore call %x from %x", c->opr[1].addr, s->curaddr);
      #endif
      return;
    }
*/

   if (s->proctype == 1)          //if (s->scanning)
     {
       set_ftype_data(s->nextaddr-1,7);
   #ifdef XDBGX
 // if (anlpass < ANLPRT)
      DBGPRT(1,"call %x from %x", c->opr[1].addr, s->curaddr);
  #endif

      do_sub_scan(s,c);           //jsubr(s, c, J_SUB);
      return s;
     }

 if (s->proctype == 4)          //if (s->emulating)
    {
  /*     SUB *sub;
       sub = get_subr(c->opr[1].addr);
       if (sub && sub->size)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opr[1].addr, sub->size);
           #endif
           s->nextaddr += sub->size;
           return s;
         }   */

       SBK *blk;
       blk = get_scan(CHSCAN,c->opr[1].addr);
       if (blk && blk->adtsize)
         {
           #ifdef XDBGX
             DBGPRT(1,"Emulation IGNORED, SUB %x has cmd args (%d)", c->opr[1].addr, blk->adtsize);
           #endif
           s->nextaddr += blk->adtsize;
           return s;
         }




   #ifdef XDBGX
     DBGPRT(1,"** enter %x from %x", c->opr[1].addr, s->curaddr);
     #endif



     n = add_escan(c->opr[1].addr, s);
   //  fakepush(s, c->opr[1].addr,1);              // std subr call, push return details onto stack

     scan_blk(n, &einst);                          // recursive (as true core)

 return n;
    }
   return s;
 }


SBK* ret(SBK *s, INST *c)
 {
   // pop and return
   // officially returnaddr = [stkptr];  (stkptr)+=2;
   STACK *n;

   s->nextaddr++;
   if (!s->proctype) return s;

//   calc_jump_op(s, -1,c);      // do as loopstop jump
   add_jump (s, s->curaddr, J_RET);

     #ifdef XDBGX
   DBGPRT(1, "%x RET stk = %d",s->curaddr, s->stkptr);       //temp
     #endif


  if (s->proctype == 1)          // if (s->scanning)
    {
     // NB. Don't do a pop, as this may be reused as source for emulate, but need a pop if arg getter is 2nd call...

     n = s->stack + s->stkptr;
     #ifdef XDBGX
     if (n->type == 1) DBGPRT(1, "would Return to %x (%x)" , n->nextaddr, n->start);
     #endif

     end_scan (s);              // end of this scan block
     #ifdef XDBGX
     DBG_STK(1,s);
     #endif
     return s;
    }


  if (s->proctype == 4)
    {
  //   fakepop(s);                     // check & drop the stack call
       n = s->stack + s->stkptr;
#ifdef XDBGX
       if (n->type == 1) DBGPRT(1, "would Return to %x (%x)" , n->nextaddr, n->start);
       if (n->type == 2) DBGPRT(1, "RETURN TO PUSH imm %x (%x)" , n->nextaddr, n->start);
    //   if (n->type == 3)
 //DBGPRT(1, "%x Invalid RETURN TO PSW", s->curaddr);

       DBG_STK(1,s);
       #endif
       end_scan(s);

       if (n->type == 2) scan_blk(get_scan(CHSTST, n->start), c);       //scan a pushed immediate
    //   s->stkptr++;           //do the pop - should make no diff as exiting anyway.
       if (s == emuscan)
         {
                  #ifdef XDBGX
           DBGPRT(1, "EMU END !!!");
           #endif
         //  end_scan(s);
         //  return n->sc;
         }


  //   if (n) return n;
    }


//   **** have a type  3  stack push for immeditaes !

  //  s->stkptr++;                // move ptr    probably this breaks it ?? don't know why.........
  //  type = get_stxack(s,s->stkptr,&x,&psp);

  //  if (type != 1)
  //   {

      // temp fixup for testing - need a stop emulate point !!





   //  if (n) DBGPRT(1, "would Return to %x" , n->nextaddr); // doesn't work for some reason....
   //  else
//     if (!n)


 //  #ifdef XDBGX
//   if (s->proctype)
//   {
 //    DBGPRT(0,"%x RET", s->curaddr);
 //    if (s->proctype == 4) DBGPRT(0," EM");
 //    DBGPRT(2,0);
 //  }
  // #endif


// always end this block
 //  end_scan(s);

 //}
   return s;
}



SBK* djnz(SBK *s, INST *c)
 {
 // djnz

   int xofst;
 //  RST *g;
   OPS *o;
 //  SPF *f;

   xofst = s->nextaddr;
   s->nextaddr += 3;
   op_calc (g_byte(xofst+1), 3,s,c);              // do register
      if (s->proctype== 1) set_ftype_data(xofst+1,7);

   calc_jump_op(s,g_val(xofst+2,0,39),c);      //signed byte
    if (s->proctype == 1)   set_ftype_data(xofst+2,39);

   cpyop(c,2,3);            // copy to 2 for display

   o = c->opr+3;

 //  g = get_rgstat(o->addr);

  if (s->proctype == 4)       // ||(g->arg && g->ptarget == s) )          //if (s->emulating)
     {
       o->val--;
       if (o->val > 0) s->nextaddr = c->opr[1].addr;
     }

   if (s->proctype == 1)          //if (s->scanning)
     {

   /*    if (g && g->arg && g->ptarget == s)
         {
          DBGPRT(1,"%x djnz (args) popped reg %x = %d", s->curaddr, o->addr, o->val);
    //      s->jmparg = 1;            //does this cover everything ??

//not sure if this is the way to go but does get the no of args out...........
//But getting args out........
          // spf to...er.......
          // need a loop for flagging args....

          // if
//f = get_spf(s);          //this gives r3a level 1 which is first one....make this a 6 ?? or leave to pop ??

// if (f->spf == 10) ???

//f = get_spf(f);          //next one gives level 2 r1a which is 2nd... make this a 5 or a 6 ??  both 10 at the moment....

// if (f->spf == 10) ???

// technically should make the top one a 5, and the lower one a 6.

} */


      do_cond_scan(s,c);            //sjsubr(s, c, J_COND);
      return s;
     }        // proctype = 1

// if popped ?? log (first) value in 3 ??


     return s;
 }



SBK* skj(SBK *s, INST *c)
 {  // skip, = jump(pc+2) as static jump
    s->nextaddr++;

    calc_jump_op(s,1,c);

   if (s->proctype == 1)          // if (s->scanning)
      {
       do_stat_scan(s,c);         //do_sjsubr(s, c, J_STAT);
       return s;
      }

  if (s->proctype == 4)          //if (s->emulating)
   {
      s->nextaddr++;      // skip next byte
   }
      return s;
 }




SBK *pshp(SBK *s, INST *c)
 {
    // push flags (i.e. psw)
    // push is stkptr -=2; [stkptr] = PSW;

    s->nextaddr++;

    if (!s->proctype) return s;

    fake_pushp(s);

    return s;
 }

SBK *popp(SBK *s, INST *c)
 {
   // pop flags (i.e. psw)
   // PSW = [stkptr];  (SP)+=2;
     // ignore in scan
  // SBK *x; //dummy
 //  uint pp;        //, type;
   STACK *x;

   s->nextaddr++;

if (!s->proctype) return s;

   //type =
   x = get_stack(s,0);     //  ,&x,&pp);       // x = s->stack[s->stkptr];    // current top of stack
   s->stkptr++;                                // move ptr

///   fake_pop (....         ??


//if (type != 2)
   // may need to do this , but not right now..
   // c->psw = pp & 0xff;
   // codebank = pp & 0xf0000;
   // rambank =  pp & 0x300;

      #ifdef XDBGX
      if (s->proctype == 1) DBGPRT(1,"%x POPP %x [%d]", c->ofst,x->start, s->stkptr);
  //    DBG_stack(s->emulating);
      #endif


return s;
 }

SBK* nrm(SBK *s, INST *c)
 {
      calc_2op(s,c);

      // from Ford hbook shift left long until b31 = 1
      // and result size is long too....as it's shifted...

      // TO DO
         return s;
 }


SBK* sex(SBK *s, INST *c)
 {
     // sign extend - not necessary in this setup ?

      calc_1op(s,c);
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

SBK* edi(SBK *s, INST *c)
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

   //   According to Ford book, this only sets PSW, so could be cancelled by a POPP
   //   or PUSHP/POPP save/restore,  so may need a PREV_BANK

   s->rambnk = c->opcix - 107;                   // new RAM bank
 //  s->rambnk = bk * 0x100;                 // establish new offset

   op_imd(s,c);                                    // mark operand 1 as immediate
   c->opr[1].val = s->rambnk;
   c->opr[1].fend = 7;                           // byte sized;
   c->numops = 1;                                // fake a single byte [imd] op
   s->nextaddr++;

   #ifdef XDBGX
   if (s->proctype) DBGPRT(1,"New Rambank = %x at %x", s->rambnk, s->curaddr);
   #endif

      return s;
 }





void do_code (SBK *s, INST *c)

{
     /* analyse next code statement from ofst and print operands
      * s is a scan blk, c is holding instance for decoded op.
      * pp-code calls this with a dummy scan block in print phase
      */

  int x, xofst, indx;      //, psw;
  const OPC *opl;
  OPS *o;

  memset(c,0,sizeof(INST));                        // clear entire INST entry

  if (s->stop)
   {
    #ifdef XDBGX
     DBGPRT(1,"Already Scanned s %x c %x e %x", s->start, s->curaddr, s->nextaddr);
    #endif
    return;
   }

    if (s->curaddr > maxromadd(s->start) || s->curaddr < minromadd(s->start))
    {
      #ifdef XDBGX
      if (s->proctype) DBGPRT(1,"Scan > Illegal address %x from blk %x, STOP", s->curaddr, s->start);
      #endif
      s->stop = 1;
      return;
    }


  if (!val_rom_addr(s->curaddr))
    {
     #ifdef XDBGX
     if (s->proctype) DBGPRT(1,"Invalid address %05x (do-code)", s->curaddr);
     #endif
     s->inv = 1;
     s->stop = 1;
     s->nextaddr = s->curaddr+1;
     return;
    }

  xofst = s->curaddr;                             // don't modify curaddr, use temp instead

//----------------------------

  x = g_byte(xofst);

  indx = opcind[x];                       // opcode table index
  opl = opctbl + indx;

 // Ford book states opcodes can have rombank(bank) AND fe as prefix (= 3 extra bytes)

   if (indx == 112)                     // bank swop - do as prefix
     {
      if (!get_cmdopt(OPT8065)) indx = 0;             // invalid for 8061
      else
       {
        int tbk;
        xofst++;
        tbk = g_byte(xofst++);                         // bank number (next byte)
        tbk &= 0xf;
        tbk++;                 //safety, plus 1 for SAD banks.

        #ifdef XDBGX
        if ((s->proctype) && !bkmap[tbk].bok)
                    DBGPRT(1,"ROMBNK - Invalid bank %d (%x)", tbk, c->ofst);
        #endif
        c->bank = tbk;                               // set bank for this opcode
        x = g_byte(xofst);                           // get true opcode
        indx = opcind[x];
       }
     }

  if (indx == 111)
     {                                  // this is the SIGN prefix, find alternate opcode
      xofst++;
      x = g_byte(xofst);
      indx = opcind[x];                 // get opcode
      opl = opctbl + indx;
      if (opl->sign)
        {
         indx += 1;                    // get prefix opcode index instead
         c->sign = 1;                  // mark sign prefix
        }
      else indx = 0;                    // or prefix not valid
     }

  if (opl->p65 && !get_cmdopt(OPT8065)) indx = 0;     // 8065 opcode only

  if (indx > 110) indx = 0;             // safety, if totally lost.....

  opl = opctbl + indx;

  s->nextaddr = xofst;                      // address of actual opcode after prefixes - for eml call


  if (indx == 76 && s->proctype)
   {         // skip
     // if opcode = skip, check that next opcode is only a single byte (i.e. no operands)
     // otherwise skip MUST be an invalid opcode. Others make no sense also (index >= 106 , ret, skip, etc)
     // check operands for SFRs later on.

    const OPC *n;
    uint ix;

    ix = opcind[g_byte(xofst+1)];

    n = opctbl + ix;

    if (ix >= 106 || ix == 76 || n->sigix == 20 || n->nops)     //not just 76 !!
     {
      #ifdef XDBGX
          DBGPRT(0,"[Skip] + %s !INV!", n->name);
       #endif
      indx = 0;
     }
   }

  if (!indx) {set_inv_opcode(s); return;}

  c->scan = s;                                     // link to scan block
  c->opcsub = x & 3;                               // valid only for multimode opcodes
  c->opcde = x;
  c->opcix = indx;                                 // opcode index
  c->sigix = opl->sigix;                           // sig index
  c->ofst  = s->curaddr;                           // original start of opcode

  // These next INST items may be changed by opcode handlers, this is default setup
  // all ops to register, size (+ read/write) from opcode table

  c->numops = opl->nops;

  o = c->opr;                                     // shortcut to operands (4 off)

  for (x = 1; x < 4; x++)
   {
     o[x].fend = opl->fend[x-1];
     o[x].rgf  = 1;
   }

// ---------------------


  if (s->proctype) set_ftype(s->curaddr, 1);           //opcode marker

//  if (s->curaddr == 0x92bb8 && s->proctype == 4)
 // {
 //     DBGPRT(0,0);
 // }



  //opcode marker need in emulate too, as may call a new block....

  // s->nextaddr is address of opcode after any prefixes
  opl->eml(s, c);                           // opcode handler
  // s->nextaddr is now address of next opcode start
  upd_watch(s, c);

 }




int isvtxt(char x)
{  // is valid text for EEC messages ??
   //stricter list of 0x20, 0x2a, 0x2e, 30-39, 41-5a 61-7a........(0xa 0xd cr lf?)


  if (x >= 0x41 && x <= 0x5a) return 1;     // letters (caps)
  if (x >= 0x61 && x <= 0x7a) return 1;     // letters (small)
  if (x >= 0x30 && x <= 0x39) return 1;     // numbers
  if (x == 0x20) return 1;      //space
  if (x == 0x2a) return 1;      // '*'
  if (x == 0x2d) return 1;      // '-'
  if (x == 0x2e) return 1;      // '.'
  if (x == 0xa) return 2;      // CR     //valid but not printable ?
  if (x == 0xd) return 2;      // LF
return 0;
}

int chk_rpt(uint lch, uchar ch, int *rcnt)
{
   // check repeats by char
    if (lch == ch) (*rcnt)++;
    else *rcnt = 0;     //clear count for non repeat

    if (ch == 0x20)
      {
        if (*rcnt > 6) return 1;     // 6 spaces
      }
    else
    if (ch == 0x30)
       {
        if (*rcnt > 3) return 1;     // 3 zeroes
       }
    else
    if (*rcnt > 2) return 1;         // anything else

    return 0;

}


uint find_text (uint *start, uchar *buf)
{
  uint tadd,lch;
  int cnt, rcnt;


  cnt = 0;
  rcnt = 0;
  lch = 0;
  if (isvtxt(buf[*start]))   //now done outside to save calls
    {     //  go backwards, then forwards whilst printable chars..............
      tadd = *start;

  //    if (tadd >= 0xce0)
   //   {
  //        DBGPRT(1,0);
   //   }



      while (tadd > *start - 32 )
       {  // backwards

         if (chk_rpt(lch, buf[tadd], &rcnt)) break;        //too many repeats

          if (tadd == 0)  break;                         // safety for uint
          if (!isvtxt(buf[tadd])) break;
          lch = buf[tadd];
          tadd--;
       }
        // check boundary
    //  if (buf[tadd] != 0xff && buf[tadd] != 0) cnt = -1000;

      tadd++;
      *start = tadd;     // new text start position

      for (; tadd < *start + 64; tadd++)
         {
          //forwards count valid chars
          if (isvtxt(buf[tadd])) cnt++;
          else break;
          if (chk_rpt(lch, buf[tadd], &rcnt)) {cnt = 0; break;}        //too many char repeats
          lch = buf[tadd];
          if (tadd == 0xffff)  break;  //safety fo uint+bank
         }
        // check  boundary
//      if (buf[tadd] != 0xff && buf[tadd] != 0) cnt = -1000;

      if (cnt >= 6 ) return cnt;
     }

 return 0;
 }















void find_cpyrt(uchar *fbuf)
{
 // go down whole file to fldata.flength looking for text
 // outside banks and before fbuf is released, within banks is below

 BANK *b, *l;
 HDATA *fl;
 uint addr, tstart;       //, tstart,tadd;
 int bk,cnt,i;
 uchar val;

 //  banks 3,4,5,6 are always in file order, so use these
 // add end gap in virtual bank 7

 fl = get_fldata();

 l = bkmap + 4 + numbanks;
 l->filstrt = fl->flength-1;          //where next (virtual) bank would start

 l = bkmap;      // bk [0] should always be all zero

 for (bk = 3; bk < 5+numbanks; bk++)
   {
     b = bkmap + bk;
  #ifdef XDBGX
     DBGPRT(1, "textsch file addr %x-%x", l->filend+1, b->filstrt);
     #endif

     for (addr = l->filend; addr < b->filstrt; addr += 4)         //8)
      {
          if (isvtxt(fbuf[addr]))
          {
        tstart = addr;
        cnt = find_text(&tstart, fbuf);

        if (cnt)
        {
            #ifdef XDBGX
          DBGPRT(0,"POSSTXT found at file %x  -  ", tstart);
          #endif
           for (i = 0; i <= cnt; i++)
             {
              val = fbuf[tstart+i];
              #ifdef XDBGX
              if (isvtxt(val) == 1) DBGPRT(0,"%c",val);
              #endif
             }
             addr += cnt;     //skip words
             #ifdef XDBGX
          DBGPRT(1,0);
          #endif
        }
          }
    }

   l = b;          // keep last bank
 }
 #ifdef XDBGX
      DBGPRT(2,0);
      #endif
}


/*

int find_text(BANK *b)
{
  // check for text in end filler block for bank b
  // text blocks have either "Copyright at ff63" or ".HEX* ff06"  in them...
  // check address above ff00 for copyright and then check blocks....
  // works for now, more or less

  //currently called for each bank, so need to change to whole file

// need more than this........................and may have to output separately....

// As "copyright" is 9 chars, could do a check every 8....

// 4DBG has message block outside all banks - see above.

  // 0FAB has no COPYRIGHT string, and no '*' terminator either
  // 1AGB not picked up 9fe0  .... ???

  // can't use fixed addresses....................

  int ans, val, start, ofst, bk, cnt;







  if (nobank(b->maxadd) < 0xdfff)  return 0;          // not full bank (so what ??)

  if (strncmp("Copyright", (char *) b->fbuf + 0xff63, 9)) return 0;  // not found

  // matched Copyright string
  bk = g_bank(b->minadd);

  start = (0xff63 | bk);
  ofst = start;

  ans = 0;
  val = g_byte(ofst);

  while (!val || (val >=0x20 && val <= 0x7f))
   {
      ofst++;
      val = g_byte(ofst);
   }
  add_cmd (start,ofst, C_TEXT |C_SYS,0);
  add_cmd (ofst+1,b->maxadd, C_DFLT|C_SYS,0);    // treat as cmd (cannot override)

  ans = start - 1;

  ofst = (0xff06 | bk);
  cnt = 0;
  val = g_byte(ofst);

  while (ofst < (0xff63 | bk))
   {     // allow a few non printers...6 ?
     if (val < 0x20 || val > 0x7f)
      {
        if (cnt > 5) add_cmd (ofst-cnt,ofst-1, C_TEXT |C_SYS,0);
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

*/




void find_fill_txt(BANK *b)
  {

// look for filler (and text) at end of banks.

   uint ofst, addr, last, minff;
   int tcnt, cnt, val;

   last  = b->maxromadd;
   minff = b->maxromadd;
   cnt = 0;

   for (ofst = b->maxromadd; ofst > b->minromadd; ofst--)
     {
       val = g_byte(ofst);
       if (isvtxt(val))
         {        //text string detect

          addr = nobank(ofst);
          tcnt = find_text(&addr, b->fbuf);

          if (tcnt)
           {
             addr |= g_bank(ofst);
             // add text cmd and continue.............
             // and fill from old end to this end.........but not if unknowns get in the way............

             ofst = addr+tcnt;
             add_cmd (addr,ofst-1, C_TEXT |C_SYS,0);
             add_cmd (ofst, last, C_DFLT|C_SYS,0);    // treat as cmd (cannot override)
             last = addr-1;
             ofst = addr-1;
             cnt = 0;
           }

         }

  // can't just stop after 3, there is dodgy stuff around..16 chars in 22CA........ need to check for a next fill block ???
  //but must have a 'reset' score too ???? could 22CA be a short bank ? probably, see 8d324, 8d32a

  // or skip up 16 and see what happens ???


//ofst-- remember

// need better fill detect in here......................use b->optdat and then clear it ?? maybe.

      if (val != 0xff) cnt++;                  // count non 0xff
      else {if (cnt < 3) minff = ofst; }        // last valid fill.

      if (cnt > 32) break;         // was 3 , temp change 2 non 0xff and no text match // can't break if odd gaps..........
     }

   ofst = minff;                     // +34?  lowest valid ff before counting - seems better

   if (ofst < b->maxromadd)
      {
        // fill at end
       add_cmd (ofst,last, C_DFLT|C_SYS,0);

       //fill can be overridden but XCODE cannot, but need to leave holes where there are not 0xff...............


       add_aux_cmd (ofst,b->maxromadd,C_XCODE|C_SYS,0);       // and xcode to match

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
     b = bkmap + i;
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

  //is it worth prioritizing bank 8 , or doing this BACKWARDS ??



  SBK *s;    //, *saved;
  uint ix;
  CHAIN *x;
  ix = 0;
  x = get_chain(CHSCAN);

  while (ix < x->num)
   {
    s = (SBK*) x->ptrs[ix];
    xscans++;

       if (!s->stop)       // && !s->caller)            //experiment..NO........
         {

// TEST
// need more checks here in case a vect is incorrectly assigned, and other push(imd)
// check that start has not been assigned as a data item or arg ?? or nextaddr for emulates ??

/* skip if opcode set already ??

if (get_ftype(s->nextaddr) == 1)             // start) == 1)
{
      #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(0,"YYYY Chain scan for %x (%x) would be ignored, set stop", s->start, s->nextaddr);
          #endif
 // s->stop = 1;
}
else   */

{




                    // set caller zero as safety for emulate
      //    recurse = 0;                               // reset recurse level
          #ifdef XDBGX
            DBGPRT(1,0);
            DBGPRT(0,"Chain ");
          #endif
          scan_blk(s, &cinst);

          //do_spf ??   maybe here.....but need to find sce and dest reliably....


    //      s->chscan = 1;
 //   if (s->proctxype == 2)    s->caller = saved;                          // and restore caller

          if (tscans > (x->num * 32))               // loop safety check
           {
            wnprt(2,"*********");
            wnprt(1," Abandon - too many scans - a loop somewhere ?");
            wnprt(1,"*********");
            printf("FAILED - too many scan loops\n");
            break;
           }

        }         // end if not scanned
 }
      if (x->lowins <= ix) ix = x->lowins;            // recheck from lowest new scan inserted
      else  ix++;
      x->lowins = x->num;                             // reset to highest value for next scan

   }
 }








/***************************************************************
 *  read directives file
 ***************************************************************/




short init_structs(void)
 {

  // 4 banks of 64k bytess
  ftype = (uchar *) cmem(0,0, 0x40000);         // data for each byte 4 X 65536 (64k)
  regstat = (RST*)  cmem(0,0, 0x400*sizeof(RST));
  regpops = (RPOP*) cmem(0,0, 8* sizeof(RPOP));

// test = (RBST*)  cmem(0,0, 8* sizeof(RPOP));
  memset(bkmap,0,sizeof(bkmap));
  bkmap[3].ftype = ftype;                   // info array start
  bkmap[4].ftype = ftype + 0x10000;         // info array first bank
  bkmap[5].ftype = ftype + 0x20000;
  bkmap[6].ftype = ftype + 0x30000;

  memset(ftype, 0, 0x40000);                 // clear all info



  anlpass  = 0;                              // current pass number
  init_prt();
  set_cmdopt(0);

  tscans    = 0;
  xscans    = 0;

  return 0;
}

/*
void openfiles(void)
{
  int i;

  fldata.error = 0;

  for (i = 0; i < numfiles; i++)
   {
    fldata.fl[i] = fopen(fldata.fn[i], fldata.fpars[i]);      //fd[i].fpars);           // open precalc'ed names
    if(i < 3 && fldata.fl[i] == NULL)
      {
       fldata.error = 1 + i;
       return;   // failed to open a file
      }
   }

}*/


uint check_ihandler(uint taddr)
 {
   // check pointer values (int handler) for valid jump, return etc
   // return 1 for valid 0 otherwise
  int val;
  uint jp, cx;

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

  do_one_opcode(taddr, &tmpscan, &sinst);

  if (sinst.sigix == 16 || sinst.sigix == 23)
    {    // jump, jnb
     jp = sinst.opr[1].addr;
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

    set_cmdopt(OPT8065);                       // set 8065 to allow bank swops

    for (x = (bank|0x2010);  x < (bank|0x205f);  x+=2)
      {
       taddr = g_word(x);             // vect pointer
       if (taddr < 0x2060) break;     // not valid

       b->vc65++;                    // 8065 count (valid address)
       b->ih65 += check_ihandler(taddr|bank);
      }

    set_cmdopt(0);                                     // clear 8065


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
    BNKF  blk, *z, *x1, *x5;
    int fofs;              //file offset
    HDATA *fl;

    fl= get_fldata();

    // temp bank setups so accesses work

    bank = 0x30000;      //bank 3, temp
    b = bkmap+3;
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

    for (addr = 0; addr < fl->flength;  addr += 0x1000)
      {
       // keep all matches and then review best candidates
       // bank->fbuf MUST point to 0x2000 whether real or virtual
       // for missing bytes - mapaddr fixed, PC changes, 0x2002, 0x2001
       // for extra bytes   - mapaddr changes +0 to +3, PC set at 0x2000

       b->filstrt = addr;                       // move file offset
       b->filend  = addr + 0xe000-1;            // max possible end point (64k - 2k)

       if (b->filend >= fl->flength)
         {        // make range safe for file end (adjust end downwards)
          b->filend = fl->flength-1;                         // end of file (no bank)
          b->maxromadd = b->filend - b->filstrt + (PCORG | bank);
         }

       //allow for missing and extra bytes....
       // if bytes are missing, cannot just map backwards, must adjust start address,
       //but if extra bytes, can move buffer up and start at 2000

       for (fofs = -2; fofs < 3; fofs++)   // 2 bytes each way around 'address'
         {
           b->fbuf = fb + addr + fofs - 0x2000;     // overlay buff onto addresses  (minus 0x2000 offset) WRONG !!

        //     if (fofs < 0) b->fbuf = fb + addr - 0x2002;
      //     else   b->fbuf = fb + addr - 0x2000;     // start addr fixed 0x2000, file start moves

           ix = 0x2000|bank;

           if (fofs < 0)
             {
              ix -= fofs;                  // move pcstart up to match so still starts at file offset zero
             }

          skp = skip_ff(ix);        // skips would only be FF and FA, others make NO SENSE max of 15

          ix += skp;
          do_one_opcode(ix, &tmpscan, &sinst);

          if (sinst.sigix == 16 && sinst.opcde)
           {  // jump, long or short, but not skip

           //put in local first to keep candidates low
             memset(&blk,0,sizeof(BNKF));

          // don't allow first file address to be negative

             if (addr < 16 && fofs < 0) blk.filestart = 0;
             else           blk.filestart = addr+fofs;

             blk.pcstart   = ix-skp;
             blk.jdest     = sinst.opr[1].addr;
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

  ch = get_chain(CHBNK);

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
     b = bkmap+ix;
     b->bok = 0;
    }

// run up bank find list and assign banks...

  for (ix = 0; ix < ch->num; ix++)
    {
     z = (BNKF*) ch->ptrs[ix];
     if (z->tbnk)
       {        //bank set (temp)
        bank = z->tbnk;               //reset bank.............
        b = bkmap+bank;

        if (z->ih65 == 40) b->P65 = 1; else  b->P65 = 0;     // mark bank as 8065
        b->bok = 1;                                          // bank is valid
        numbanks++;
        if (z->cbnk) b->cbnk = 1;                            // code bank (8)
        b->filstrt = z->filestart;                           // actual bank start as buffer offset for 0x2000
        b->filend = b->filstrt + 0xe000-1;                   // default (max) end

        // check beyond end of file
        if (b->filend >= fl->flength) b->filend = fl->flength-1;

       // short bank check .............
       l = bkmap + (bank-1);                      // last bank
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
               wnprt(2," WARNING - file has extra %d byte(s) at front of bank",  fofs);
           }

        fofs = nobank(b->minromadd) - PCORG;

        if (fofs)
           {
              #ifdef XDBGX
              DBGPRT(1," WARNING - file has %d byte(s) missing at front of bank", fofs);
              #endif
               wnprt(2," WARNING - file has %d byte(s) missing at front of bank", fofs);
           }

      }
    }

   #ifdef XDBGX

   DBGPRT(2,"- - - - - bank finds - - - - -");

   ch = get_chain(CHBNK);
   for (ix = 0; ix < ch->num; ix++)
   {
      ch->chprt(ch, 1, ix, DBGPRT);
   }
   DBGPRT(2,0);

#endif
}




void clrbkbit(int ix, int bk)
{
  BANK *b;

  b = bkmap+ix;

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
     from = bkmap + i;            // temp bank
     if (from->bok)
      {        // valid to transfer
       from->dbank++;
       to   = bkmap + from->dbank;        // new bank
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


 // codbnk = 0x90000;                         // code start always bank 8
 // rambnk = 0;                               // start at zero for RAM locations
//  if (numbanks) datbnk = 0x20000;           // data bank default to 1 for multibanks (but may not always be true?)
 // else   datbnk = 0x90000;                  // databank 8 for single

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

    bkmap[bk].bkmask = 0xb;         // 9, 1, 0  not 8
    bkmap[bk].bok = 1;

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

         // taddr++;
         // val = g_byte(taddr);             // get opcode after bank prefix

         // if (val == 0xe7 || ( val >=0x20 && val <= 0x27))
         //  {
            // a jump - calc destination
         //   taddr = do_jumpsig (opcind[val], taddr,0x100000);
           // now try to find a valid opcode in one of the other banks.
           // have a guessed order at this point

          /* for (i = 2; i < 6; i++)
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
             } */
       //    }
         }    // end bank check 8065 loop
      }

// now check for an allocated bank after shuffle....



 }

int chk_bank_order (int cbk)
 {
    // cbk = bank location - so check outwards for other banks
    // from int vect pointers

  int i, x, ans;
  BANK *b;

  ans = 1;
  // first use interrupt handlers to identify any bank jumps
  // and know that THIS bank cannot be any that are jumped to
  // b->bkmask is bitmask of possible banks


  for (i = 3; i < 7; i++)
    {
     b = bkmap + i;
     if (i != cbk) drop_banks_via_ints(i);
     else
      {
       b->dbank = 8;       // set up bank 8 directly
       b->bkmask = 4;
       #ifdef XDBGX
         DBGPRT(1, "Bank %d is 8", i);
       #endif
      }
    }

   // now see if we can definitely find more banks
   // if any bank has single bit, can take that as definite ID
   // and drop that bit from all other banks

   for (i = 3; i < 7; i++)
    {
      b = bkmap + i;

      if (i != cbk)
        {  // not for bank 8
         if (b->bkmask == 1 || b->bkmask == 2 || b->bkmask == 8)
         {  // found a single bit, drop from other banks
           for (x = 3; x < 7; x++)
             {
               if (x != i)
                  {
                    #ifdef XDBGX
                   if (bkmap[x].bkmask & b->bkmask)
                     {
                      DBGPRT(1, "Bank %d cannot be %d", x, b->bkmask < 8 ? b->bkmask-1 : 9);
                     }
                   #endif
                   bkmap[x].bkmask &= (~b->bkmask);
                  }
             }
         }
        }
    }

  // now, is there one bit per bank ?? if so have got the
 // bank order.  if not, then we'll have to guess.


 //NB. bin with no vects in data bank 1 ???

   for (i = 3; i < 7; i++)
    {
      b = bkmap + i;
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
        wnprt(2, "Warning - Unable to verify bank order in .bin file");
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
       b = bkmap+i+3;                // 3 to 6
       // b->fbuf points to bank start..................

       // any bank with P65 means all banks are 8065
       if (b->P65) set_cmdopt(OPT8065);              //cmdopts |= 0x8;

       if (b->cbnk)
          {
           if (bank >= 0)
             {
               wnprt(1," Warning - ");
               wnprt(1,"Found multiple Start Code banks [bank 8]");
               wnprt(1,"Choosing first bank found, others ignored - use bank command to override");
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
      wnprt(1,"Cannot find a Start/Code bank [bank 8]");
      wnprt(1,"Is this file an 8061 or 8065 Binary ??");
      return -4;
     }


   #ifdef XDBGX
   DBGPRT(1,"%d Banks, start in %d", numbanks+1, bank+3);
   #endif

  switch (numbanks)
    {

     default:
       wnprt(1,"0 or > 4 banks found. File corrupt?");
       return -2;

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

       //BUT FOMO206 (John) apparently has bank 1 with no vectors.....


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
  prt_banks(DBGPRT);
  DBGPRT(1,0);
  #endif

  return numbanks;
}


int readbin(void)
{
  /* Read binary into main block 0x2000 onwards (ibuf[0x2000])
  *  ibuf  [0]-[2000]     RAM  (faked and used in this disassembly)
  *  ibuf  [2000]-[42000] up to 4 banks ROM, at 0x10000 each
  *  keep file in memory as may have to revisit for data banks (FM20M06)
  */

  int x;


  HDATA *fl;

  fl = get_fldata();

  if (!fl->fh[0]) return -1;


  fl->flength = fseek (fl->fh[0], 0, SEEK_END);
  fl->flength = ftell (fl->fh[0]);
  fseek (fl->fh[0], 0, SEEK_SET);                // get file size & restore to start

  wnprt(2,"# Input file is '%s'",fl->fname[0]);
  wnprt(2,"# File is %dK (0x%x) bytes",fl->flength/1024, fl->flength);

//  fl->error = 0;

  if (fl->flength > 0x40000)
   {
    wnprt(1,"# Bin file > 256k. Truncated to 256k");  // is this always right ??
    fl->flength = 0x40000;
   }

  wnprt(1,0);

  binfbuf = (uchar*) mem(0,0, fl->flength);          // big enough for file
  fread (binfbuf, 1, fl->flength, fl->fh[0]);

  #ifdef XDBGX
   DBGPRT(1,"Read input file (0x%x bytes)", fl->flength);
  #endif

  // file in temp buffer, now read and layout as per banks.

// check for file error ?
//  end = fl->flength;

  find_banks(binfbuf);
  x = do_banks(binfbuf);               // auto, may be modded later by command

  if (x > 0)   find_cpyrt(binfbuf);    // copyright and calibration text, after banks done

  return x;
}



void rstkf(SIG *sx, SBK *b)
{

// mark levels dropped off.....
// stack fiddler subroutine.

 b->levf =  sx->v[14];

}



void rbasp(SIG *sx, SBK *dummy)
{

  //add rbase entries from signature match

  uint raddr, end, daddr;
  uint dbank, reg, i, cnt;   //,xcnt,rcnt;
  RBT *r;
  RST *g;


if (sx->done) return;

   if (numbanks) dbank = 0x20000;           // data bank
   else dbank = 0x90000;

  reg = (sx->v[1] & 0x3ff);
  raddr = nobank(sx->v[3]) | g_bank(sx->start);   // root of rbase in caller (code bank)

  cnt = g_byte(raddr);              // no of entries in list       //wrong if multibank

  #ifdef XDBGX
  DBGPRT(1,"In rbasp - begin reg %x at %x, cnt %d", reg, raddr, cnt);
  #endif

  add_cmd(raddr, raddr+1, C_BYTE|C_SYS,0);             // count + num (2 bytes)

  raddr +=2;                                         // start of pointers
  end = raddr+(cnt*2)-2;                             // where last pointer is

  add_cmd(raddr, end+1, C_WORD|C_SYS, 0);  // rbase entries

  if (get_lasterr(CHCMD))
   {
     #ifdef XDBGX
      DBGPRT(1,"Rbase cmd error");
     #endif
    return ;
   }

//check all pointers

  for (i= raddr; i <= end; i+= 2)
   {

    daddr  = g_word(i);                   // pointer in code bank
    cnt = g_word(daddr | dbank);          // value in there (= next pointer)
    daddr  = g_word(i+2);

    if (i < end && daddr != cnt)           //check match
      {
       // rbases point to different bank
       #ifdef XDBGX
          DBGPRT(1,"Rbase bank error");
       #endif
       return;
     }
   }

   // looks good from here, do the commands and statuses....

   for (i= raddr; i <= end; i+= 2)
    {
      daddr = g_word(i);                        // register pointer
      daddr |= dbank;                           // add + default data bank
      cnt = g_word(daddr | dbank );             //  get next pointer from data;
      cnt   |= dbank;
      add_cmd(daddr, daddr+1, C_WORD|C_SYS,0);    //set word at start of rbase (= next rbase)

        // also valid for last pointer for xcode
     //    if (!valid_reg(daddr)) daddr |= (dbank << 16);     ????            // immediates strip the bank, put it back

      r =   add_rbase(reg,daddr,0,0xfffff);                // dbg message in here....
      if (r) r->usrcmd = 1;                                  // mark as fixed
      g = get_rgstat(reg);
      g->invrbt = 0;
      upd_ram(reg,daddr,15);
      add_aux_cmd(daddr,cnt-1, C_XCODE|C_SYS,0);      // add an XCODE between pointers
      reg +=2;                                        // next register
   }

   sx->done = 1;
   #ifdef XDBGX
   DBGPRT(1,"end rbasp");
   #endif
  // return ;
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


// do branch has a FULL copy of SFIND to ensure that parallel branches have their own
// parameter set established at start of each new branch (= a jump to here)
// this is because the registers may CHANGE in each branch.

// void (*set_str) (SFIND);
          //int anix;            // spf number of func moved to SFIND




void set_struct(SFIND *f)
{
 // check for bank first, and then if truly valid
 if (!val_rom_addr(f->ans[0]))  f->ans[0] |= f->datbank;

 if (!val_rom_addr(f->ans[0]))
     {
   //    #ifdef XDBGX
     //    DBGPRT(1,"Invalid Address %x", f->ans[0]);
  //     #endif
       return ;
      }





   // function
   if (f->spf->spf == 2) set_xfunc(f);
   // table

    if (f->spf->spf == 3) set_xtab(f);
}


void do_list_struct(SFIND *f)
{
 uint m, i, addr;       // temp !!
 uint start, gap;      // for pointer list
 LBK *k;
 ADT *a;


// this sort of works, problem is it proceeds down list and can get size and sign worng.......

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
       if ( i != 0x7f && i != 0xff) break;
       f->ans[0] = addr;        //set func address
       set_struct(f);
       m += 2;
       if (get_cmd(CHCMD,m,0)) break;        // is this any good ? (next ptr not done !)
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
      if (get_cmd(CHCMD,m,0)) break;
       }
    }

   if (m >= start+2) {  k = add_cmd (start,  m-1, C_WORD, 0);

   if (k)
    {
      a = add_adt(vconv(start),0);    // only one
   if (a)
     {
         k->adt = 1;
      a->fend = 15;         //word
  //    a->bsize = 1;
    //  a->foff = 1;
   //   a->data = f->lst[1];
      a->fnam = 1;
      a->cnt = 1;
     }
}}
}


void do_branch(uint addr, int cnt, SFIND f)
 {
    // do a branch from a jump. if jump(s) to here found, do as separate branch
   int xcnt;
   uint xofst, ix;
   JMP *j;
   const OPC *opl;
 //  CHAIN *ch;

   if (cnt <= 0) return;

  // ch = get_chain(CHJPT);


      #ifdef XDBGX
       DBGPRT(1,0);
       DBGPRT(0,"Branch start %x %d [%x %x] (%x %x)", addr,cnt, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
      #endif
   xcnt = cnt;
   xofst = addr;

   while (xcnt)
     {
       // find_opcode just goes down/up list, no checks (find_opcodes does check).
       xofst = find_opcode(0, xofst, &opl);
       if (!xofst) break;
       xcnt --;

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

       match_opnd(xofst,opl,&f);

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

       // see if any jumps in to xofst. if so, launch new branches.

       ix = get_tjmp_ix(xofst);     // start point

       while((j = (JMP*) get_next_item(CHJPT,&ix)))               // < ch->num)
        {
     //     j = (JMP*) ch->ptrs[ix];
          if (j->toaddr != xofst) break;
          #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(1,"JMP %x<-%x %d", j->toaddr, j->fromaddr, cnt);
          #endif

         //j->from-j->cmpcnt to skip the cmp
// copy f for each new branch ??
         do_branch(j->fromaddr,xcnt,f);
         ix++;
        }  // end jump loop

     }        // end find opcode loop

}



// ** abandon this for funcs and do a liear search and verify ???
// MUCH easier than tables are.....................
// but STIL need a decent table recognizer................



void do_calls(SBK *s, SFIND f)
{
  // do the 'base' or 'root' of the tree for each call with spfs
  //(i.e. the subr calls outwards) separately to reset params

 JMP *j, *k;
 LBK *g;
 ADT *a;
 CHAIN *ch;
 uint jx, ix, ofst;


      #ifdef XDBGX
        DBGPRT(1,0);
        DBGPRT(0,"Base calls for %x", s->start);
      #endif

 ch = get_chain(CHJPT);
 ix = get_tjmp_ix(s->start);     // start point

 while(ix < ch->num)
   {
    j = (JMP*) ch->ptrs[ix];

    if (j->toaddr != s->start) break;

    if (j->jtype == J_SUB)              // sub only for from FIRST call
        {
     //     if (f.rg[1] == 1) f.ans[1] = j->fromaddr + j->size;     //func only, for override
     #ifdef XDBGX
  DBGPRT(1,0);
          DBGPRT(0,"To %x - [%x %x] (%x %x) ", j->fromaddr , f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
#endif
        //j->from-j->cmpcnt to skip the cmp
        //j->from+j->size to check for arguments
        // defined args are same as 'args' data command.....

        ofst = j->fromaddr + j->size;
        f.datbank = g_bank(j->fromaddr);

        a = get_adt(vconv(s->start),0);                           //get_subr(s->start),0);
        if (a)
          {    // same as data command, but user specified      IS THIS RIGHT ANY MORE ???
            f.ans[0] = decode_addr(a, ofst);        //address and size.
             #ifdef XDBGX
            DBGPRT(1,"do args at %x = %x", g->start, f.ans[0]);    // assume first parameter ???
             #endif
           // need datbank ?? from the
          }

        else
        {
          g = get_cmd(CHAUX,ofst, C_ARGS);
          if (g)
           {         // assume first parameter ???
            a = get_adt(vconv(g->start),0);
            f.ans[0] = decode_addr(a, ofst);         // address and size

            #ifdef XDBGX
             DBGPRT(1,"do args at %x = %x", g->start, f.ans[0]);    // assume first parameter ???
            #endif
            // need datbank ??
           }
        }

      do_branch(j->fromaddr, 20,f);

          // do_branch scans back from j->from FIRST before checking for jumps,
          // so add an extra check and loop for jumps to j->from

          jx = get_tjmp_ix(j->fromaddr);     // start point
          while(jx < ch->num)
            {
             k = (JMP*) ch->ptrs[jx];
             if (k->toaddr != j->fromaddr) break;
             #ifdef XDBGX
              DBGPRT(1,0);
              DBGPRT(1,"To+ %x - [%x %x] (%x %x) ", j->fromaddr, f.rg[0], f.rg[1], f.ans[0], f.ans[1]);
             #endif
             //j->from-j->cmpcnt to skip the cmp
// copy f for each new branch ??
             do_branch(k->fromaddr,20,f);                //rg1,rg2,rg3,ans1,ans2,ans3);
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
  // no increment in loop as the delete effectively
  // shoves everything down by one...

 LBK *n;
 CHAIN *x;

 x = get_chain(CHCMD);

 n = 0;

 while (1)
     {
      if (ix >= x->num) break;

      n = (LBK *) x->ptrs[ix];
      if (n->usrcmd || n->start >= addr) break;

      // n is spanned by table or func and byte,word, etc.
      // delete it unless it's done by usercmd

      if (n->fcom < C_TEXT) chdelete(CHCMD,ix,1);
      else break;

     }
  return n;       //return next kept command.
 }


//uint getfunceval(uint size)
//{


//}


// MUST allow multiples at top (maxval), multiple at end (minval), and maybe one duplicate value in between.




void extend_func(LBK *k, uint ix)
{
  // extend func to zero or max negative here

  uint eadr, bnkmax,  rowsize, adr, vmatch;     //size
  int val, lastval, endval;
  ADT *a, *b;
  LBK  *n;

 // 4 bytes by default - must have 2 adnl items.

  ix++;         //next block

  bnkmax = maxromadd(k->end);                 // safety - end of bank
  a = get_adt(vconv(k->start),0);          // entry 1
  if (!a) return;
  b = get_adt(a,0);                       // entry 2
  if (!b) return;

  rowsize = bytes(a->fend) + bytes(b->fend);   // whole row size in BYTES
  endval  = get_endval(a->fend);               // end value

  adr   = k->start;

  val = 0xd0000;                       // odd value for start so it gets saved
  vmatch = 0;
  eadr = k->end;

  while (adr < bnkmax)                      // check input value for each row (UNSIGNED)
    {
     adr += rowsize;
     lastval = val;
     val = g_val(adr,0, (a->fend & 31));      // next input value as UNSIGNED
     if (val == endval)
        {
         eadr = adr + rowsize-1;            // hit end input value, set end
         break;
        }

     if (val >= lastval) vmatch++;       // value did not drop, score.
     if (vmatch >= 4) break;              // max 4 matches ??

    }

  k->end = eadr;                        // stage 1 of extend

  // adr is now at a zero row, and val is last value

  n = del_olaps(adr, ix);        //next command.

  if (n)            // && n->start < eadr)
    {                                           // chop at next cmd, if relevant never ture??
      if (n->usrcmd) eadr = n->start;
      if (n->fcom > C_TEXT) eadr = n->start;
    }

  //check for bank end
if (eadr > bnkmax) eadr = bnkmax;


  // set row size for whole row for extend check.
  // word ->long, byte->word

  rowsize = (a->fend & 31) + (b->fend & 31) +1;        // row size (unsigned)
  endval = g_val(adr,0, rowsize);         // and its full row value

  // must check overlap with data HERE

//  eadr = adr +  (bytes(rowsize) * 10);       // allow 10 rows extra  CAN"T DO THIS

//  if (eadr > bnkmax) eadr = bnkmax;          // check bank end

  vmatch = 0;                   //row count
  while (adr < eadr)
    {       // any matching final whole rows
     adr += bytes(rowsize);
     val = g_val(adr,0, rowsize);
     if (val != endval) {
      //   adr -= bytes(rowsize);
         break;
     }
     vmatch++;
     if (vmatch > 8) break;

     if (n && !endval && n->start == adr) break;   // match next cmd for extended zeroes
    }

  // adr is now start of next item (end of extend values)
  // if end value is zero need extra check
  // otherwise need to delete any skipped entries........

//  n = del_olaps(adr, ix);

 //check again for overlaps as del-olaps may have moved n

// if (n && adr > n->start)
 //   {
 //     if (n->usrcmd) adr = n->start;         // can't go past a user command.
 //     if (n->fcom > C_TEXT) adr = n->start;  // or past other funcs or tabs (etc)
 //   }


 k->end = adr-1;             //final end with any extra end rows.


 // now check if all outputs are in upper byte and < 16
 // if so this can be autosized.

 rowsize = bytes(a->fend) + bytes(b->fend);    // whole row size in BYTES


if (rowsize == 4)
  {          //only word func - check for table dim style (top byte only)
    //   int val, lastval, endval;  declared above int lval;
 //  uint chg;
// prob need more rules (like eval should always be <= lval...but val must change also
// and end up at zero ??)

// need to check not signed as well

  adr  = k->start + bytes(a->fend);            // output row
  eadr = k->end;  // ( - rowsize ?);                               // end
  endval = 1;                           // flag
  vmatch = 0;                           // value changed
  lastval = 0x2000;                     // 32 in top byte
  while (adr < eadr)                      // check input value for each row (UNSIGNED)
    {
     val = g_val(adr,0, (b->fend & 31));      // next input value as UNSIGNED
     if (val & 0xff)
        { //bottom byte set
         endval = 0;
         break;
        }
     if (val != lastval) vmatch = 1;

     if (val > 1000) endval = 0;         // > 16 in top byte, so not a scale

      adr += rowsize;
      lastval = val;
    }

  // all lower bytes are zero, values have changed and <= 16
  // so assume it can be reduced
  if (endval && vmatch) add_mcalc(b, 4, 256);          // = (x/256) (upper byte)

 }

}




/*

}
uint test_func(uint start, uint ssize)
{
uint addr, end, rsz;
int val,lval;
if (start == 0x827b4)
{
DBGPRT(1,0);
}



rsz = casz[ssize] * 2;          // one row

addr = start;
end = start + 128;      // safety

lval = g_val(addr, ssize);   //start value


val = (~casg[ssize]) & camk[ssize];     // start value

if (lval != val) return start;


// need row extender in here for some short tables

addr += rsz;

while (addr < end)
{
val = g_val(addr, ssize);
if (val >= lval) break;
lval = val;
addr += rsz;
}

// 3 rows = 6 * size
if ( (addr - start) >= (rsz * 3) && (lval & camk[ssize]) == casg[ssize] )
{
DBGPRT (1,"possible func at %x-%x (%d)", start, addr, ssize);
}

return start;
}




void find_func(uint start, uint end)
{
uint addr;
int val;

addr = start;

while (addr < end)
  {
   val = g_byte(addr);
   if (val == 0xff || val == 0x7f)
    {
     test_func(addr,1);
     test_func(addr,5);      // byte
     if (!(addr &1))
      {
       test_func(addr,2);
       test_func(addr,6);  // word
      }
     }
    addr++;

 }
}
*/




void extend_table(LBK *k,uint ix)
{
  uint apeadr, maxadr, size, val, eval;     //, temp;     //,jx;
  ADT *a;
  LBK *n;

 // attempt to extend by data,use external proc
 // to do best match by data for row and col sizes


  if (!k) return;

 //safety, end of bank

 DBGPRT(1,"new_table_size");

  apeadr = maxromadd(k->end)+1;
  maxadr = apeadr;

 // ix++;             //not with 'get_next'

  n = (LBK*) get_next_item(CHCMD,&ix);

  if (n) apeadr = n->start;       // apparent end from next cmd

  while (n && n->fcom < C_TEXT && !n->usrcmd)
     {  // may be overrideable
        n = (LBK*) get_next_item(CHCMD,&ix);   //  n = (LBK *) chcmd.ptrs[jx];    //skip all byte and word
     }

  if (n) maxadr = n->start;            // max end - not skippable


  a = get_adt(vconv(k->start),0);      // cols in a->cnt

  if (!a) return;
  size = apeadr - k->start;    // size in bytes to apparent end
  eval = size/a->cnt;          // max rows
  val  = eval * a->cnt;        // size in whole rows

 // temp = apeadr;


 if (a->cnt > 2) new_table_sizes(k->start, a->cnt, a->fend);       //works, but WAY too slow for 32x32!!!
 //do_table_sizes(k->start, a->cnt, &temp, maxadr);  // temp, for all tables


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
k->end = k->start+val-1;         ///cathc all
return ;     //don't override table size !!

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

// now extend the end of funcs for the extra filler rows.....
// probably need to check tables too.
// this needs to be moved out if not using the scan branches

void extend_funcs(void)
{
uint ix;
LBK *k;

 ix = 0;
 while ((k = (LBK*) get_next_item(CHCMD,&ix)))
   {
     if (!k->usrcmd && k->fcom == C_FUNC)  extend_func(k,ix);
   }

}

void extend_tabs(void)
{
uint ix;
LBK *k;

 ix = 0;
 while ((k = (LBK*) get_next_item(CHCMD,&ix)))
   {
     if (!k->usrcmd && k->fcom == C_TABLE) extend_table(k,ix);
   }

}









void do_functabs()

 {
   uint ix;
   SBK *s;                    //SUB *sub;

   SFIND f;
   SPF *x;
   void *fid;
   // go down tab and func subroutines and find params (structs)
   //this may be horrid run backwards up the jump/call tree, but it's
   // the only way when scans are done once only....

     #ifdef XDBGX
   DBGPRT(0, "\n\n ***** in do_functabs");
   #endif
   ix = 0;

   while ((s = (SBK*) get_next_item(CHSCAN,&ix)))
    {

   fid = vconv(s->start);

     while ((x = get_spf(fid)))
      {

        if (x->spf > 1 && x->spf < 4)      // 2 for func, 3 for tab
         {
          // func or tab lookup - common to both




          f.datbank = 2;               // safety
          f.lst[0] = 0;
          f.spf    = x;                //->spf;
          f.rg[0]  = x->pars[0].reg;               //addrreg;        // address
          f.ans[0] = 0;

          if (x->spf == 2)
            {     // func subroutines
             f.rg [1] = 0;              // no size reg
             f.ans[1] = 1;              // mark as found
             }

          else
            {   // table subroutines
             f.rg [1] = x->pars[1].reg;            //sizereg;        // cols
             f.ans[1] = 0;
            }




          do_calls(s,f);         // scan call tree
         }
      fid = x;
    }

   }
  // extend_functabs();  moved outside
 }




/***************************************************************
 *  disassemble EEC Binary
 ***************************************************************/
int disassemble (char *fstr)
{
  int ans;

 HDATA *fl;
  // disassemble should be ZERO to end program, or >0  to repeat filename ask

  fl = get_fldata();
  init_structs();

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


  if (fl->error > 0)
    {
     printf("File '%s' not found or cannot open\n",fl->fname[fl->error-1] );
     return 1;
    }

  wnprt(1,0);
  wnprt(1,"# ----------------------------");
  wnprt(1,"# SAD Version %s (%s)", SADVERSION, SADDATE);
  wnprt(2,"# ----------------------------");

  wnprt(1,"# ---------------------------------------------------------------------------------------------");
  wnprt(1,"# NB - All commands and errors are printed in new command format");
  wnprt(3,"# ---------------------------------------------------------------------------------------------");


 // and the debug file....
  #ifdef XDBGX
  DBGPRT(1,0);
  DBGPRT(1,"# ----------------------------");
  DBGPRT(1,"# SAD Version %s (%s)", SADVERSION, SADDATE);
  DBGPRT(2,"# ----------------------------");
  #endif



  ans = readbin();  // anlpass = 0 for read bin file and basic setup

  /* ans is numof banks...read bin file and sort out banks
   but allow getudir in case of manual set banks */

  show_prog(++anlpass);            //anlpass = 1  cmds and setup
  ans = getudir(ans);              // AFTER readbin...

  if (numbanks < 0 || ans < 0 || ans > 1)
  {
     // wrong if no banks or fault
    wnprt(1,"Abandoned !!");
    printf("Abandon - can't process - see warnings file\n");
    return 0;
  }

  show_prog(++anlpass);            //anlpass = 2  signature pass

  // printf("\nstart prescan test");
  prescan_sigs();
// printf ("\nend prescan test\n");

   //check_rbase();                     // check static rbase setup (2020 etc, not as sig)
  wnprt(2,0);
  wnprt(2,"# ----- Start Disassembly phase 1 -----");

  scan_all ();        // do all the analysis

 // ----------------------------------------------------------------------

  show_prog(++anlpass);             //anlpass = 3  scan phase

  do_jumps();

  wnprt(2,"# ----- End   Disassembly phase 1 -----");
  wnprt(2,"# ----- Start Disassembly phase 2 -----");

   // ----------------------------------------------------------------------
   // and do a whole new pass here for the data ??? loops,structs, tables....
   // ----------------------------------------------------------------------



  // scan_dc_olaps();            // check and tidy code and data overlaps


// scan funcs
// scan tabs ??

  // scan_data_gaps();

 //  scan_lpdata();



  show_prog (++anlpass);            //anlpass = 4 ANLFFS

  wnprt(2,"# ----- End   Disassembly phase 2  -----");

  turn_scans_into_code();      // turn all valid scans into code

  do_functabs();                // find structs via funclu and tablu calls  //Split this ??

  scan_gaps();       //after funcs and tabs, before extend

  extend_funcs();

// find_funcs();                // look for extra funcs before extending tables..........

  extend_tabs();              //may be better to try to find all functions FIRST....

  do_dtk_links();               // mark track data items for push, etc.

  // catch 22 here about which order to do these things................


  // probably do need a scan_code_gaps here, before data and structs.

  scan_gaps();                 // BEFORE data, so only code, tabs,vects,funcs (dtk) exist. then data

  turn_scans_into_code();      // do this again after scan gaps -- need to improve this for double run

  turn_dtk_into_data();        // add data found from dtk

//or do all the stuff first, and THEN go back to gap.................

// and lastly, will need a fill checker.

  // ----------------------------------------------------------------------


  show_prog (++anlpass);                 //anlpass = 5 = ANLPRT

  #ifdef XDBGX
    DBG_data();
  #endif

  wnprt(2,"# ----- Output Listing to file %s", fl->fname[1]);

  prt_listing ();
  pstr(2,0);     // flush output file
  pstr(2, " ##########   END of Listing   ##########");



  wnprt(2,0);
  wnprt(1,"# ---------------------------------------------------------------------------------------------");
  wnprt(1,"# The disassembler has scanned the binary and produced the following equivalent command list.");
  wnprt(1,"# This list includes any user commands read in. It can be copied and pasted into a directives file.");
  wnprt(1,"# Commented command lines printed for information but may be uncommented (e.g. bank)");
  wnprt(2,"# ---------------------------------------------------------------------------------------------");

  prt_dirs();
  mfree(ftype, 0x40000);                     // opcode etc
  mfree(regstat, 0x400*sizeof(RST));         // reg status
  mfree(regpops, 8 * sizeof (RPOP));
  mfree(binfbuf, fl->flength);               // original binary file

  free_structs();                      //this has mem debug prt in it


  wnprt(2,0);
  wnprt(2, "# ----- END of disassembly run -----");

  return 0;
}

