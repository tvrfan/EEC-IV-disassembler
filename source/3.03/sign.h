/**************************************************
* Signature defines
* Signatures are like a 'regular expression' things 
* a set of pattern matches to find specific items in the binary.
* for example inline parameter handlers, function lookups, and so on.
*************************/

#ifndef _XSIGNX_H

#define _XSIGNX_H 1


#include "core.h"

#define NSGV      16                 // num sig values, but sig code allows for 32


// OPCODE WORD (top nibble)
#define     OPCOD  0x80000000        // this is an opcode, data pattern if not set.
#define     OPOPT  0x40000000        // optional instruction (single) or 'skip to' subsig if opcode is zero
#define     OPJMP  0x20000000        // keep jump destination
#define     OPRPT  0x10000000        // repeat opcode - repeats must have an index (tval)

// next nibble
#define     OPSPV  0x08000000        // extras - keep bit from JB or JNB, keep carry state (JC,JNC,STC,CLC),
                                     // check opcode subtype against tval (M opcodes) to check imd, inx etc
#define     OPSWP  0x04000000        // allow swop vals to match ldx and stx, and jb,jnb to match other conds (not djnz)

// OPSKP  0x02                      // skip pattern flgs ??
// OPSUB  0x01 


// DATA WORD(s)  

#define     SDINC  0x8000000                 // increment/save index on a repeat
#define     SDLBT  0x4000000                 // drop lowest bit of register (for autoinc and some byte ops)
#define     SDANY  0x2000000                 // is allow any value, save first one
//#define     SDINC  0x100000                 // increment/save index on a repeat

/*************************************
 * DESCRIPTION 
 * 
 * 32 bit unsigned integer - by byte (top down)
 * 
 * First integer (opcode) 
 * NB. if sigindex is zero and OPCODE flag is zero, this means a special subpattern (see further below)
 * 
 * 0xff00 0000     flags (opcode, only 0xfc used do far)
 * 0x001f 0000     index value into params array for saved items, repeat counts, regs  etc

 * 0x0000 f000     pattern size (= num of ints including this int) >> 12 & 0xf
 * 0x0000 003f     if opcode flag set, this is signature index
 *                 if not set, this becomes a data value match (not sig index)
 * 
 * All patterns begin with a first integer 
 * then rest of words are DATA pattern.  (as in OPCODE-DATA-DATA, etc)
 * 
 * DATA integers (follwos first opcode int)
 * 
 * 0x0f00 0000  flags (data)
 * 0x001f 0000  index for saved value (register or data) for matching (ixval)
 * 0x0000 1f00  count value location if SDINC is set, for incremental list
 * 0x0000 00ff  register or data to match (ignore if SDANY is set)
 * 
 * If ixval set, then a param is SAVED into the par array (0-0x1f)
 * normally a register,  but can be bit number, or repeat count, etc. as modded by some flags.
 * First matched occurrence with tval set (ie saved val is zero) is treated as SDANY automatically
 * repeats not set as list therefore need SDANY set
 * 
 * Ignores up to 15 'auto ignore' opcodes (NOP etc)
 * 
 * Special subpatterns
 * 
 * if opcode value is zero, do a SUBSIG or a SKIPSIG pattern, selected by OPOPT flag

 * SUBSIG is an 'any order' match of opcodes.
 
first val is top and bottom byte

  * top byte - no of patterns in subpatt
  * low byte - no of patterns which must match (at least once)

if ix set then matches are stored in this array index 

then following patts can match in ANY ORDER.

 * SKIPSIG
 * 
 * simple single entry which defines 'up to 'n' opcodes (and operands)
 * can be skipped.  If tval set count if stored. not safe to follow with an OPT
 * opcode I think...(not tested)
 **************************************************************

* NOTES
*  NEW - standardize levels at v[1] and no of pars at v[2] (e.g fpar vpar)
*  then use par list later and standardize for consecutive checks at er...
* possibly use 3 and 4 if multilevel thing.

 * sg-val for first opcode is really only ever bottom byte, so can use flags in top.....
 * but keep as full short for data so addresses can be used ?
 * 
 * 
 * 
 may need marker for bnk, prefix etc (by pattern ?) for some of the more clever multibanks
(BWAK)  change main loop so it scans by bank, and keep latest RBNK

 enc24 should be a sub for tables ??

*************************/


typedef struct xsig
{
 struct xptn *ptn;      // pointer to signature pattern
 int ofst;              // start address of sig found
 int xend;              // end of signature - only really used to stop multiple pattern matches and overlaps
 uint done   : 1 ;      // processed marker - for debug really
 uint skp    : 4 ;      // skipped opcodes at front
 int v[NSGV];           // saved values
} SIG;


//  if this goes disc based, then may well need the 'type' field back....and ditch proc addresses

// ptnx is main (static) sig definition
// prep_sig params when -  1st     2nd
// sig is chained       -  SIG*   0
// sig copied/processed -  SIG*   SBK* -> SXT*

typedef struct xptn
{
 uint *sig;                          // actual sig pattern
 const char *name;                   // name (can be null)
 short size;                           // no of instructions in sig
 short vxsz ;                            // size of param array 16 or 32  (could be single bit !!) not used yet...
 void (*sig_prep) (SIG *, SBK*);           // preprocess  (various)
 void (*sig_pr)   (CSIG *, SXT *, SBK *);     // process sig when encountered
} PAT;


// called from sub sign -  

// uint cpu : 2;              // 1 = 8061, 2 = 8065, 3 = both
 // uint funcsp : 1;           // special extra handling for funcs and tabs for sign ?
 //uint prescan : 1;                            // look for sig at beginning

 //short subtype ;                            // for subroutine naming (and other ?)

/*
typedef struct dz
{
 uint opc  : 8 ;
 uint size : 4;
 uint nops : 3;
 uint opcs : 2;
 uint inc  : 1;
 uint imd  : 1;
 uint inx  : 1;
} DSZ;
*/





#endif

