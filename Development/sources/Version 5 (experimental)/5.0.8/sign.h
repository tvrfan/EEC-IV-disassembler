/**************************************************
* Signature defines
* Signatures are like a 'regular expression' matcher
* a set of pattern matches to find specific items in the binary.
* for function lookups, rbase lists, and so on.
*************************/

#ifndef _XSIGNX_H

#define _XSIGNX_H 1

// #include "core.h"
#include "shared.h"




/*************************************
 * DESCRIPTION
 *
 * a 'match pattern' is 4 bytes
 *
 * bytes                      For all patterns
 * 0xff ff 00 00   [0], [1].  Flags,types, first and second bytes
 * 0x00 00 ff 00   [2]        2 Indexes 0-15, 16-31, into params array for saved and rematched items
 * 0x00 00 00 ff   [3]        Value to match - opcode or operand or data (register address or immediate value etc.)
 *
 */


// First byte [0] - pattern def - opcodes, data, operands
#define  SIGOCOD  0x80     // standard opcode pattern, optionally with data pats following. [1] flags, [2] index, [3] value to match

#define  SIGCHILD 0x50     // full pattern as child sig         [1] is flags [2] is subpattern index
#define  SIGOSKP  0x40     // opcode skip pattern 'skip to x'   [1] is max skips
#define  SIGORPT  0x30     // opcode repeat/optional            [1] is min rpt/match, [2] is save index, [3] is max
#define  SIGOSUB  0x20     // opcode order independant pattern  [1] is min match, [2] is index [3] is no of patterns
#define  SIGOPER  0x10     // operand pattern  [1] flags, [2] index, [3] value to match

// Second byte [1] -  opcode patterns (0x80)
#define  SIGOPC  0x40        // save 'mode' of address of opcode.
#define  SIGBIT  0x8         // save bit number for bit jump (and others).

// Second byte [1] -  operand patterns (0x10)

//save operand size in bytes ??? funcs, tabs...

#define  SIGINX  0x8         // alternate indexing for repeats.
#define  SIGLBT  0x4         // drop lowest bit of register (for autoinc and some byte ops)
#define  SIGIGN  0x2         // allow any value (ignore)
#define  SIGBNK  0x1         // add current bank (for addresses and jumps)


// First byte [0] -  data (straight match) , single 4 byte patterns (data value is where opcode would be, byte [3])
#define  SIGDATA  0x81     // standard data pattern
#define  SIGDRPT  0x31     // data repeat / optional   min max
#define  SIGDSUB  0x21     // data sub pattern (match x of y)



// sign bit ??

typedef struct match
{       //current state holder for passing state between subroutines


SIG     *sgn;        // current SIG block
uchar   *sx;         // pointer within current pattern

// may need ofst in here for subsigs....

uint  smatch  : 1;
uint  skip    : 1;   // redundant opcodes can be skipped
uint  highb   : 1;   // match (extra) high byte only - word and long
uint  opno    : 2;   // operand number
uint  patsub  : 2;   // opcsub from pattern
uint  oswp    : 1;   // swop operands (stx <-> ldx)

uint  mc      : 8;          //match count ??? repeats and optionals... ??

} MMH;


//*********************************************external declarations

int    g_byte       (uint);

uint   get_cmdopt   (uint);
uint   get_anlpass  (void);
void   show_prog    (int);
void   do_code      (SBK *, INST *);

void* cmem (void *ptr, size_t osize, size_t nsize);
void  mfree(void *addr, size_t size);
void*  chmem        (uint, uint);          //CHAIN *,uint);
SIG*   add_sig      (SIG *);

uchar  get_opc_inx  (uchar);

uint   val_rom_addr (uint addr);
uint   databank     (uint addr, INST *);

BANK* get_bkdata  (uint ix);

const OPC*   get_opc_entry (uchar);

#ifdef XDBGX
uint DBGPRT        (uint, cchar*, ...);
#endif



#endif