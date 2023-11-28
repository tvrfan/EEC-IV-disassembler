/**************************************************
* Signature defines
* Signatures are like a 'regular expression' matcher 
* a set of pattern matches to find specific items in the binary.
* for function lookups, rbase lists, and so on.
*************************/

#ifndef _XSIGNX_H

#define _XSIGNX_H 1

#include "core.h"

// OPCODE WORD (top nibble)
#define     OPCOD  0x80000000        // this is an opcode, data pattern if not set.
#define     OPOPT  0x40000000        // optional instruction (single) or 'skip to' subsig if opcode is zero
#define     OPJMP  0x20000000        // keep jump destination
#define     OPRPT  0x10000000        // repeat opcode - repeats must have an index (tval)

// next nibble - more flags

#define     OPSPV  0x08000000        // special extras
#define     OPSPX  0x04000000        // match stx with ldx, sign prefix
#define     OPSVL  0x02000000        // prefix for signed present  
#define     OPSST  0x01000000        // save opcode subtype 

/* flags
 * OPSPV      special func 
 *    keep jump bit bit and state from JB or JNB (into params[tval])
 *    keep carry state from a JC,JNC,STC,CLC
 *    allow zero repeats (i.e. optional + repeat) with OPRPT
 * 
 * OPSPX 
 *    allow ldx to match stx (and swop register data i.e LDX a, b is same as STX b, a)
 *    with 'sign prefix' opcodes, specify whether prefix is there (0xFE)
 * OPSVL
 *    a flag to specify prefix present or not with OPSPX. used for mults and divs (i.e. signed or unsigned)
 * OPSST
 *    save opcode subtype in tval for multitype opcodes
*/

// DATA WORD(s)  

#define     SDINC  0x8000000                 // increment/save index on a repeat
#define     SDLBT  0x4000000                 // drop lowest bit of register (for autoinc and some byte ops)
#define     SDANY  0x2000000                 // is allow any value, save first one
//#define     SDDAT  0x1000000                 // save data from register

/*************************************
 * DESCRIPTION 
 * 
 * 32 bit unsigned integer - by byte (top down)
 * 
 * First integer (opcode) 
 * NB. if sigindex is zero and OPCODE flag is zero, this means a special subpattern (see further below)
 * 
 * 0xff00 0000     flags (opcode, all 8 used)
 * 0x000f 0000     index value into params array for saved items, repeat counts, regs  etc

 * 0x0000 f000     pattern size (= num of ints including this int) [>> 12 & 0xf]
 * 0x0000 003f     if opcode flag set, this is opcode signature index
 *                 if not set, this becomes a data value match (not sig index)
 * 
 * All patterns begin with a first integer 
 * then rest of words are DATA pattern.  (as in OPCODE-DATA-DATA, etc)
 * 
 * DATA/operands (follows opcode match)
 * 
 * 0x0f00 0000  flags (data - 3 used)
 * 0x000f 0000  index for saved value (register or data) for matching (ixval)
 * 0x0000 0f00  count value location if SDINC is set, for incremental list
 * 0x0000 0f00  actual register value location if SDDAT is set
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



#endif