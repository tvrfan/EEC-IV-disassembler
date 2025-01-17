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
 * 0x00 00 ff 00   [2]        Indexes 0-15 into params array for saved and rematched items
 * 0x00 00 00 ff   [3]        Value to match - opcode or operand or data (register address or immediate value etc.)
 */

// First byte - opcodes, operands
#define  SIGOCOD  0x80     // standard opcode pattern, optionally with data pats following.

#define  SIGOSKP  0x40     // opcode skip pattern 'skip to x'
#define  SIGORPT  0x30     // opcode repeat/optional  min max (opt = 0 or 1 repeat)
#define  SIGOSUB  0x20     // opcode order independant pattern (match x of y)
#define  SIGOPER  0x10     // operand pattern

#define SIGCHILD  0x50     //true subpattern as another embedded sig ??

// First byte -  data (straight match) , single 4 byte patterns (data value is where opcode would be, byte [3])
#define  SIGDATA  0x81     // standard data pattern
#define  SIGDRPT  0x31     // data repeat / optional   min max
#define  SIGDSUB  0x21     // data sub pattern (match x of y)

// Second byte -  operand patterns (0x10)

//save operand size in bytes ??? funcs, tabs...

#define  SIGLBT  0x4         // drop lowest bit of register (for autoinc and some byte ops)
#define  SIGIGN  0x2         // allow any value (ignore)
#define  SIGBNK  0x1         // add current bank (for addresses and jumps)

// Second byte -  opcode patterns (0x80)
#define  SIGOPC  0x40        // save 'mode' of address of opcode.
#define  SIGBIT  0x8         // save bit number for bit jump (and others).


// sign bit ??

typedef struct match
{       //current state holder for passing state between subroutines


SIG     *sgn;        // current SIG block
uchar   *sx;         // pointer within current pattern

uint  smatch  : 1;
uint  skip    : 1;   // redundant opcodes can be skipped
uint  highb   : 1;   // match high byte only
uint  opno    : 2;   // operand number

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