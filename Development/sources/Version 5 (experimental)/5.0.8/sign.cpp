/*****************************************************
 *  Signature Declarations and includes
 ******************************************************/
#include "sign.h"          //calls shared.h



SBK  dsc;                   // dummy scan uses curaddr, nextaddr in here
INST dnst;                  // one instance

// duplicate for subpatterns ??

uchar *savepatts[32];        // pattern pointers holder for subpatterns
uint   lv[NSGV];             // local signature values

#ifdef XDBGX
uint dbgflag = 0;
#endif

//use offsets for separate file ???

// uchar * index [] ??  but must be done as offsets....not direct char*


//Rbase loop for 2020 or 2060

uchar rbase [] = {
 10,0, 6, 0, 'r','b','a','s','e',0,    // 6 rbase lookup (A9L)
 0x80, 0x0 , 0x0 , 0xa1,   0x10, 0x0 , 0x1 , 0xf0,   0x10, 0x0,  0x0,  0x0 ,   0x10, 0x0 , 0x2 , 0x18,
 0x80, 0x0 , 0x0 , 0xb3,   0x10, 0x0 , 0x0 , 0x1 ,   0x10, 0x1 , 0x3 , 0x20,   0x10, 0x0 , 0x3 , 0x20,  0x10, 0x0 , 0x5 , 0x1a,
 0x80, 0x0 , 0x0 , 0xa2,   0x10, 0x0 , 0x6 , 0x15,   0x10, 0x0 , 0x7 , 0x1c,
 0x80, 0x0 , 0x0 , 0xc2,   0x10, 0x0 , 0x2 , 0x19,   0x10, 0x0 , 0x7 , 0x1c,                //STW !!!!
 0x80, 0x0 , 0x0 , 0xe0,   0x10, 0x0 , 0x5 , 0x1a,   0x10, 0x0 , 0x8 , 0xf7,                 //djnz
 0xff
};


// Avct using stkptr2 as task pointer

uchar avct [] = {                //bwak3n2 82477

 9, 0, 1, 0 ,   'a', 'v','c', 'l',  0,

 0x80, 0x0 , 0x0 , 0xa1 ,   0x10, 0x1 , 0x4 , 0x8a,  0x10, 0x0, 0x4, 0xaf,  0x10, 0x0 , 0x0 , 0x22,      // ldw   R22,af8a
 0x40, 0x3 , 0x0 , 0x0,                                                                 //skip up to 3 opcodes
 0x80, 0x0 , 0x0 , 0xb1 ,   0x10, 0x0 , 0x0 , 0x11,   0x10, 0x0 , 0x7 , 0x11,                            // R11 = 11  bank select
 0x40, 0xf , 0x0 , 0x0,                                                                 //skip up to 15 opcodes
 0x80, 0x0 , 0x0 , 0xc9,     0x10, 0x2, 0x0, 0x0,   0x10, 0x2, 0x0,0x0,                 //push (word)
 0x80, 0x0 , 0x0 , 0xf0,                                                                // ret or reta
 0xff
};



uchar times [] = {
  10,0,  2, 1,    't','i','m','e','r',0,            // 2  timer list early AA, A9L

  0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x1 , 0x30,   0x10, 0x0 , 0x2 , 0x3c,    // ldb R3c, [R30++];      (AA 3e73)
  0x80, 0x0 , 0x0 , 0x99,   0x10, 0x0 , 0x0 , 0x0 ,   0x10, 0x0 , 0x2 , 0x3c,    // cmpb  R3c,0
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0x0 , 0x55,                              // je    3ed0

  0x30, 0x01, 0xe,  0x02,                                                        // repeat for word (1 or 2 bytes)
  0x80, 0x10, 0x0 , 0xae,   0x10, 0x0 , 0x1 , 0x30,   0x10, 0x4 , 0x7 , 0x32,    // ldzbw R32,[R30++]

  0x80, 0x0 , 0x0 , 0x30,   0x10, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x12,    // jnb   B0,R3c,3e93
  0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x1 , 0x30,   0x10, 0x0 , 0x3 , 0x3d,    // ldb   R3d,[R30++]
  0x80, 0x0 , 0x0 , 0xae,   0x10, 0x0 , 0x1 , 0x30,   0x10, 0x0 , 0x4 , 0x34,    // ldzbw R34,[R30++]
  0x80, 0x0 , 0x0 , 0x72,   0x10, 0x0 , 0x4 , 0x34,   0x10, 0x0 , 0x3 , 0x3d,    // an2b  R3d,[R34]
  0x80, 0x0 , 0x0 , 0x33,   0x10, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x4 ,    // jnb   B3,R3c,3e91
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x0 , 0x0 , 0x4 ,                              // je    3e93
  0x80, 0x0 , 0x0 , 0x27,   0x10, 0x2 , 0x0 , 0xe2,                              // sjmp  3e73
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0x0 , 0xe0,                              // je    3e73
  0x80, 0x0 , 0x0 , 0x50,   0x10, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x5 , 0x3e,   0x10, 0x0 , 0x0 , 0x0,    // an3b  R0,R3e,R3c
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0x0 , 0xda,                               // je    3e73
  0x80, 0x0 , 0x0 , 0x31,   0x10, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x5 ,     // jnb   B1,R3c,3ea1
  0x80, 0x0 , 0x0 , 0xa2,   0x10, 0x0 , 0x7 , 0x32,   0x10, 0x0 , 0x6 , 0x36,     // ldw   R36,[R32]
  0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x3 ,                               // sjmp  3ea4
  0x80, 0x0 , 0x0 , 0xbe,   0x10, 0x0 , 0x7 , 0x32,   0x10, 0x0 , 0x6 , 0x36,     // ldsbw R36,[R32]
  0x80, 0x0 , 0x0 , 0x32,   0x10, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x4 ,     // jnb   B2,R3c,3eab
  0x80, 0x0 , 0x0 , 0x03 ,  0x10, 0x0 , 0x6 , 0x36,                               // negw R36
 0xff
};


 // func lookup - byte or word

uchar fnlu [] = {
 10, 0, 3, 0, 'f','n','l','u','1', 0,

 0x80, 0x0 , 0x0 , 0x9b,   0x10, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x2 , 0x2 ,   0x10, 0x0 , 0x8 , 0x34,
 0x80, 0x8 , 0xb , 0x31,   0x10, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x4 ,


 0x80, 0x8 , 0x0 , 0xdb,   0x10, 0x0 , 0x0 , 0xa ,
 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x2 ,

 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x0 , 0x0 , 0x6 ,

 0x80, 0x0 , 0x0 , 0x65 ,  0x10, 0x0 , 0x2 , 0x2 ,   0x10, 0x0, 0x0, 0x0,  0x10, 0x0 , 0x4 , 0x32,
 0x80, 0x0 , 0x0 , 0x27,   0x10, 0x2 , 0x0 , 0xed,

 0x30, 0x0 , 0x0,  0x1,                                                        // optional -  min, ix, max
 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x2 , 0x0 , 0xdf,   0x10, 0x0 , 0x3 , 0xf6,  // optional A9L clear signed flag



 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x6 , 0x36,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x33,   0x10, 0x0 , 0x7 , 0x38,
 0x80, 0x0 , 0x0 , 0x7a,   0x10, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x6 , 0x36,
 0x80, 0x0 , 0x0 , 0x7a,   0x10, 0x0 , 0x4 , 0x33,   0x10, 0x0 , 0x8 , 0x34,
 0x80, 0x0 , 0x0 , 0x7a,   0x10, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x7 , 0x38,
 0x80, 0x8 , 0xa , 0x31,   0x10, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x4 ,
 0x80, 0x8 , 0x0 , 0xdb,   0x10, 0x0 , 0x0 , 0x9 ,

 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x2 ,
 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x0 , 0x0 , 0x5 ,
 0x80, 0x0 , 0x0 , 0x91,   0x10, 0x2 , 0x0 , 0x1 ,   0x10, 0x0 , 0x3 , 0xf6,
 0x80, 0x0 , 0x0 , 0x13,   0x10, 0x0 , 0x7 , 0x38,
 0x80, 0x0 , 0x0 , 0x7c,   0x10, 0x0 , 0x8 , 0x34,   0x10, 0x0 , 0x7 , 0x38,
 0x80, 0x0 , 0x0 , 0x9c,   0x10, 0x0 , 0x6 , 0x36,   0x10, 0x0 , 0x7 , 0x38,
 0x80, 0x0 , 0x0 , 0x30,   0x10, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x2 ,
 0x80, 0x0 , 0x0 , 0x13,   0x10, 0x0 , 0x7 , 0x38,
 0xff
};

/* INTERPOLATE may be better to look for the signed conversion - A9L ??
3722: bc,33,3a            ldsbw R3a,R33          swR3a = yR33;                     # SIGNED interpolate calc
3725: bc,31,3c            ldsbw R3c,R31          swR3c = yR31;
3728: 68,3c,3a            sb2w  R3a,R3c          R3a -= R3c;
372b: ac,30,3c            ldzbw R3c,R30          wR3c = yR30;
372e: fe,6c,3a,3c         sml2w R3c,R3a          slR3c = swR3c * R3a;
 * unsigned does a direct multiply ml3b.....
*/

// this works for A9L type tblu,. but not the later one..........
// table interpolate - no specific handler - for signed/unsigned checks

uchar ttrps [] = {
 12, 0, 0, 0,  't','a','b','i','n','t','p',0,
// 0x80, 0x18, 0xe , 0x1 ,   0x10, 0x2 , 0x0 , 0x33,                 //optional ??
 0x80, 0x8 , 0xa , 0x37,   0x10, 0x0 , 0x3 , 0x2d,   0x10, 0x0 , 0x7 , 0x1a,  // jnb   B7,R2d,373c      if (Tblsflg = 1)


 0x80, 0x0 , 0x0 , 0xbc,   0x10, 0x0 , 0x1,  0x33,   0x10, 0x0 , 0x2 , 0x3a,  // ldsbw R3a,R33          swR3a = yR33;
 0x80, 0x0 , 0x0 , 0xbc,   0x10, 0x0 , 0x4,  0x31,   0x10, 0x0 , 0x5 , 0x3c,  // ldsbw R3c,R31          swR3c = yR31;
 0x80, 0x0 , 0x0 , 0x68,   0x10, 0x0 , 0x5,  0x3c,   0x10, 0x0 , 0x2 , 0x3a,  // sb2w  R3a,R3c          R3a -= R3c;
 0x80, 0x0 , 0x0 , 0xac,   0x10, 0x0 , 0xc,  0x30,   0x10, 0x0 , 0x5 , 0x3c,  // ldzbw R3c,R30          wR3c = yR30;
 0x80, 0x6 , 0x0 , 0x6c,   0x10, 0x0 , 0x2 , 0x3a,   0x10, 0x0 , 0x5 , 0x3c,  // (0xfe) sml2w R3c,R3a     slR3c = swR3c * R3a;ml
 0xff
};


/************** word tables ->
  table addr   = s->v[4]
  table cols   = s->v[3]
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

 // table lookup, byte, early

uchar tblu [] = {
 10, 0, 4,0, 't','b','l','u','1' ,0,
 0x80, 0x0 , 0x0 , 0x5c,   0x10, 0x0 , 0x8 , 0x33,   0x10, 0x0 , 0x3 , 0x34,   0x10, 0x0 , 0x9 , 0x36,    //
 0x80, 0x0 , 0x0 , 0x74,   0x10, 0x0 , 0x6 , 0x31,   0x10, 0x0 , 0x9 , 0x36,
 0x80, 0x40, 0x0 , 0xd3,   0x10, 0x0 , 0x0 , 0x2 ,
 0x80, 0x40, 0x0 , 0x17,   0x10, 0x0 , 0x5 , 0x37,
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x9 , 0x36,   0x10, 0x0 , 0x4 , 0x38,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x39,   0x10, 0x0 , 0x6 , 0x31,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x38,   0x10, 0x0 , 0x8 , 0x33,
 0x80, 0x20, 0x0 , 0x28,   0x10, 0x2 , 0x1 , 0x1c,
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x3 , 0x34,   0x10, 0x0 , 0x4 , 0x38,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x38,   0x10, 0x0 , 0x8 , 0x33,
 0x80, 0x0 , 0x0 , 0x05,   0x10, 0x0 , 0x4 , 0x38,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x38,   0x10, 0x0 , 0x6 , 0x31,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x7 , 0x3a,   0x10, 0x0 , 0x3 , 0x34,
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0xc ,
 0xff
};

 // table lookup, later multibank

uchar tbl2 [] = {
 10, 0, 4,0, 't','b','l','u','2',0,

 0x80, 0x0 , 0x0 , 0x5c,   0x10, 0x0 , 0x8 , 0x37,   0x10, 0x0 , 0x3 , 0x38,   0x10, 0x0 , 0x1 , 0x3a,
 0x80, 0x0 , 0x0 , 0x74,   0x10, 0x0 , 0x9 , 0x35,   0x10, 0x0 , 0x1 , 0x3a,
 0x80, 0x0 , 0x0 , 0xb4,   0x10, 0x0 , 0x0 , 0x0 ,   0x10, 0x0 , 0x2 , 0x3b,
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x1 , 0x3a,   0x10, 0x0 , 0x4 , 0x3c,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x9 , 0x35,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x8 , 0x37,
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x3 , 0x38,   0x10, 0x0 , 0x4 , 0x3c,
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0x7 , 0x34,   0x10, 0x0 , 0x6 , 0x39,
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0xe , 0x36,   0x10, 0x0 , 0x2 , 0x3b,
 0x80, 0x20, 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0x1c,
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x8 , 0x37,
 0x80, 0x0 , 0x0 , 0xb3,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x0 , 0xff,   0x10, 0x0 , 0x9 , 0x35,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0x4 , 0x3c,
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0xc ,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0xe , 0x36,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x7 , 0x34,
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0x2 , 0x3b,   0x10, 0x0 , 0x6 , 0x39,
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0x1c ,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0x4 , 0x3c,
 0x80, 0x8 , 0xa , 0x37,   0x10, 0x0 , 0xf , 0xd8,   0x10, 0x2 , 0x0 , 0x1a,      //jnb is sign marker
 0xff
};



/* word table lookup
  table addr   = s->v[4]
  table cols   = s->v[3]
  bit register = interp->v[3]
  bit mask     = interp->v[10]
*/

// R38 [3] is no of word cols  R3c [4] is tab address
// [1] is jump address

uchar wtbl [] = {
 10, 0, 4, 2, 'w', 't','b','l','u',0,                                    // 4  table lookup, later multibank

0x80, 0x00, 0x00, 0x5c,   0x10, 0x00, 0x0b, 0x37,    0x10, 0x00, 0x03, 0x38,    0x10, 0x00, 0x02, 0x3e,   // ml3b  R3e,R38,R37
0x80, 0x00, 0x00, 0xb0,   0x10, 0x00, 0x0a, 0x34,    0x10, 0x00, 0x05, 0x3a,                              // ldb   R3a,R34
0x80, 0x00, 0x00, 0xb0,   0x10, 0x00, 0x06, 0x36,    0x10, 0x00, 0x07, 0x3b,                              // ldb   R3b,R36
0x80, 0x00, 0x00, 0xac,   0x10, 0x00, 0x08, 0x35,    0x10, 0x00, 0x0a, 0x34,                              // ldzbw R34,R35
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x0a, 0x34,    0x10, 0x00, 0x02, 0x3e,                              // ad2w  R3e,R34
0x80, 0x00, 0x00, 0x09,   0x10, 0x00, 0x09, 0x01,    0x10, 0x00, 0x02, 0x3e,                              // shlw  R3e,1
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x02, 0x3e,    0x10, 0x00, 0x04, 0x3c,                              // ad2w  R3c,R3e
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x04, 0x3c,    0x10, 0x00, 0x0a, 0x34,                              // ldw   R34,[R3c++]
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x04, 0x3c,    0x10, 0x00, 0x06, 0x36,                              // ldw   R36,[R3c]
0x80, 0x00, 0x00, 0x28,   0x10, 0x02, 0x01, 0x25,                                                         // scall 92e8e
0x80, 0x00, 0x00, 0x09,   0x10, 0x00, 0x09, 0x01,    0x10, 0x00, 0x03, 0x38,                              // shlw  R38,1
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x03, 0x38,    0x10, 0x00, 0x04, 0x3c,                              // ad2w  R3c,R38
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x04, 0x3c,    0x10, 0x00, 0x06, 0x36,                              // ldw   R36,[R3c]
0x80, 0x00, 0x00, 0x05,   0x10, 0x00, 0x04, 0x3c,                                                         // decw  R3c
0x80, 0x00, 0x00, 0x05,   0x10, 0x00, 0x04, 0x3c,                                                         // decw  R3c
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x04, 0x3c,    0x10, 0x00, 0x0a, 0x34,                              // ldw   R34,[R3c]
0x80, 0x00, 0x00, 0xa0,   0x10, 0x00, 0x02, 0x3e,    0x10, 0x00, 0x03, 0x38,                              // ldw   R38,R3e
0x80, 0x00, 0x00, 0x28,   0x10, 0x02, 0x00, 0x10,                                                         // scall 92e8e

0xff

};

//interpolate for word table. B7 R96 is sign bit.
// bit register = interp->v[3]
//  bit mask     = interp->v[10]

uchar wintp[] = {

9, 0, 0, 0, 'w', 't', 'b', 'p', 0,

0x80, 0x00, 0x00, 0xac,   0x10, 0x00, 0x01, 0x3a,    0x10, 0x00, 0x02, 0x3e,                              // ldzbw R3e,R3a
0x80, 0x08, 0x0a, 0x37,   0x10, 0x00, 0x03, 0x96,    0x10, 0x00, 0x00, 0x01,                              // jnb   B7,R96,92e95
0x80, 0x00, 0x00, 0x4c,   0x10, 0x00, 0x06, 0x36,    0x10, 0x00, 0x02, 0x3e,  0x10, 0x00, 0x07, 0x44,    //  sml3w R44,R3e,R36                                                       // sml3w R44,R3e,R36
0x80, 0x00, 0x00, 0x37,   0x10, 0x00, 0x03, 0x96,    0x10, 0x00, 0x00, 0x01,                              // jnb   B7,R96,92e9d
0x80, 0x00, 0x00, 0x4c,   0x10, 0x00, 0x08, 0x34,    0x10, 0x00, 0x02, 0x3e,  0x10, 0x00, 0x09, 0x40,   // sml3w R40,R3e,R34
0x80, 0x00, 0x00, 0x68,   0x10, 0x00, 0x09, 0x40,    0x10, 0x00, 0x07, 0x44,                              // sb2w  R44,R40

// matchint interp for tblu2 ??

0xff
};


//A9L simple fixed arg getter 3695

uchar gparfix[] = {

10, 1, 7, 0, 'g', 'p', 'a', 'r','f', 0,

0x30, 0x1 , 0xf,  0x3,                                                           //     repeat between 1 and 3 times - pop level
0x80, 0x00, 0x00, 0xcc,   0x10, 0x08, 0xf1, 0x38,                                // pop   Rx location and match cnt in index[2].
0x30, 0x1 , 0xe,  0x8,                                                           //     repeat between 1 and 8 times
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x2, 0x3a,    0x10, 0x08, 0xe4, 0x3c,      // ldb   R3c,[Rx++]
0x80, 0x00, 0x00, 0xc8,   0x10, 0x02, 0x00, 0x3a,                                // push  Rx

0xff
};

// xdt2 multibank fixed arg getter

uchar gparfixm[] = {

11, 3, 7, 0, 'g', 'p', 'a', 'r','f', 'm', 0,

//0x80, 0x00, 0x00, 0xa3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x02, 0x02,    0x10, 0x00, 0x03, 0x36,   // ldw   R36,[R20+2]

0x80, 0x00, 0x00, 0xa3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x04, 0x04,    0x10, 0x00, 0x05, 0x3a,   // ldw   R3a,[R20+4]
//0x40, 0x3 , 0x0 , 0x0,                                                                 //skip up to 3 opcodes ??
//0x30, 0x0 , 0x0,  0x1,     //optional
0x80, 0x00, 0x00, 0x18,   0x10, 0x00, 0x02, 0x02,    0x10, 0x00, 0x06, 0x37,                              // shrb  R37,2

//0x30, 0x0 , 0x0,  0x1,     //optional

0x80, 0x00, 0x00, 0xb0,   0x10, 0x00, 0x06, 0x37,    0x10, 0x00, 0x07, 0x11,                              // ldb   R11,R37
0x30, 0x1 , 0xe,  0x8,                                                                                    // up to 8 args
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x05, 0x3a,    0x10, 0x08, 0xe4, 0x36,                              // ldb   R36,[R3a++]
0x80, 0x00, 0x00, 0xb1,   0x10, 0x00, 0x07, 0x11,    0x10, 0x00, 0x07, 0x11,                              // ldb   R11,11
0x80, 0x00, 0x00, 0xc3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x04, 0x04,    0x10, 0x00, 0x05, 0x3a,   // stw   R3a,[R20+4]
0xff
};



// A9L variable arg getter 77d2

uchar gparvar[] = {

10, 1, 8, 0,'g', 'p', 'a', 'r','v', 0,

0x80, 0x00, 0x00, 0xcc,   0x10, 0x00, 0x01, 0x3c,                                                         // pop   R3c
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x01, 0x3c,    0x10, 0x00, 0x02, 0x3a,                              // ldb   R3a,[R3c++]
0x80, 0x00, 0x00, 0xcc,   0x10, 0x00, 0x03, 0x42,                                                         // pop   R42
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x03, 0x42,    0x10, 0x00, 0x04, 0x3b,                              // ldb   R3b,[R42++]
0x80, 0x00, 0x00, 0xc6,   0x10, 0x00, 0x05, 0x16,    0x10, 0x00, 0x04, 0x3b,                              // stb   R3b,[R16++]
0x80, 0x00, 0x00, 0xe0,   0x10, 0x00, 0x02, 0x3a,    0x10, 0x00, 0x06, 0xf7,                              // djnz  R3a,77c9
0x80, 0x00, 0x00, 0xc8,   0x10, 0x00, 0x03, 0x42,                                                         // push  R42
0x80, 0x00, 0x00, 0xc8,   0x10, 0x00, 0x01, 0x3c,                                                         // push  R3c

0xff
};

// xdt2 variable arg getter

uchar gparvarm[] = {

11, 1, 8, 0,'g', 'p', 'a', 'r','v', 'm', 0,

0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x02, 0x40,                              // ldw   R40,[R20]
0x80, 0x00, 0x00, 0x18,   0x10, 0x00, 0x03, 0x02,    0x10, 0x00, 0x04, 0x41,                              // shrb  R41,2
0x80, 0x00, 0x00, 0xc4,   0x10, 0x00, 0x05, 0x11,    0x10, 0x00, 0x04, 0x41,                              // stb   R41,R11
0x80, 0x00, 0x00, 0xa3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x03, 0x02,    0x10, 0x00, 0x02, 0x40,   // ldw   R40,[R20+2]
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x02, 0x40,    0x10, 0x00, 0x06, 0x3e,                              // ldb   R3e,[R40++]
0x80, 0x00, 0x00, 0xc3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x03, 0x02,    0x10, 0x00, 0x02, 0x40,   // stw   R40,[R20+2]
0x80, 0x00, 0x00, 0xa3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x07, 0x04,    0x10, 0x00, 0x02, 0x40,   // ldw   R40,[R20+4]
0x80, 0x00, 0x00, 0x18,   0x10, 0x00, 0x03, 0x02,    0x10, 0x00, 0x04, 0x41,                              // shrb  R41,2
0x80, 0x00, 0x00, 0xb0,   0x10, 0x00, 0x04, 0x41,    0x10, 0x00, 0x05, 0x11,                              // ldb   R11,R41
0x80, 0x00, 0x00, 0xa3,   0x10, 0x00, 0x01, 0x20,    0x10, 0x00, 0x08, 0x06,    0x10, 0x00, 0x09, 0x46,   // ldw   R46,[R20+6]
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x09, 0x46,    0x10, 0x00, 0x0a, 0x3f,                              // ldb   R3f,[R46++]
0x80, 0x00, 0x00, 0xc6,   0x10, 0x00, 0x0b, 0x26,    0x10, 0x00, 0x0a, 0x3f,                              // stb   R3f,[R26++]
0x80, 0x00, 0x00, 0xe0,   0x10, 0x00, 0x06, 0x3e,    0x10, 0x00, 0x0c, 0xf7,                              // djnz  R3e,8a548
0x80, 0x00, 0x00, 0xb1,   0x10, 0x00, 0x05, 0x11,    0x10, 0x00, 0x05, 0x11,                              // ldb   R11,11

0xff };

 // encoded address type 4 and 2, (xdt2)

uchar enc24 [] = {
 10, 0, 5 ,4,  'e','n','c','2','4',0,
 0x80, 0x0 , 0x0 , 0xac,    0x10, 0x4 , 0x8 , 0x37,   0x10, 0x0 , 0xa , 0x3a,    // ldzbw R3a,R37;
 0x80, 0x0 , 0x0 , 0x71 ,   0x10, 0x0 , 0x3 , 0xf,    0x10, 0x4 , 0x8 , 0x37,    // an2b  R37,0x1f (t4) or 0xf (t2)
 0x80, 0x0 , 0x0 , 0x08 ,   0x10, 0x0 , 0x0 , 0x4 ,   0x10, 0x0 , 0xa , 0x3a,    // shrw  R3a,4

 0x30, 0x0 , 0x0,  0x1,                                                        // optional -  min, ix, max
 0x80, 0x40, 0x0 , 0x71 ,   0x10, 0x0 , 0x0 , 0xe ,   0x10, 0x0 , 0xa , 0x3a,    // an2b  R3a,e     not in type 2

 0x80, 0x0 , 0x0 , 0x67 ,   0x10, 0x0 , 0xa , 0x3a,   0x10, 0x0 , 0x5 , 0xf0,  0x10, 0x0,0x0, 0x0,   0x10, 0x0 , 0x9 , 0x36,   // R36 += [R3a+f0];
 0xff
};

 // encoded address types 3 and 1

uchar enc13 [] = {
 10, 2, 5, 3, 'e','n','c','1','3', 0,
 0x80, 0x0 , 0x0 , 0x37,   0x10, 0x4 , 0x8 , 0x42,   0x10, 0x2 , 0x0 , 0x14,    // jnb   B7,R43,3cda  (drop bit)
 0x80, 0x0 , 0x0 , 0xac,   0x10, 0x4 , 0x8 , 0x42,   0x10, 0x0 , 0x4 , 0x3a,      // ldzbw R3a,R43      (drop bit)

 0x20, 0x3 , 0x0 , 0x3 ,                                                          // 3 of 3 must match, any order
 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x0 , 0x0 , 0xf ,   0x10, 0x4 , 0x8 , 0x42,  // an2b  R43,f
 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x0 , 0x3 , 0x70,   0x10, 0x0 , 0x4 , 0x3a,  // an2b  R3a,70  (0xfe is a t1, 0x70 is a t3)
 0x80, 0x0 , 0x0 , 0x18,   0x10, 0x0 , 0x0 , 0x3 ,   0x10, 0x0 , 0x4 , 0x3a,    // shrw  R3a,3

 0x80, 0x0 , 0x0 , 0x67,   0x10, 0x0 , 0x4 , 0x3b,   0x10, 0x0 , 0x5 , 0xf0,   0x0, 0x0, 0x0, 0x0, 0x10, 0x0 , 0x8 , 0x42, // ad2w  R42,[R3a+f0]
 0x30, 0x0 , 0x0,  0x1 ,                                                            // 1 optional opcode
 0x80, 0x40, 0x1 , 0xc2 ,  0x10, 0x0 , 0x9 , 0x3e,   0x10, 0x0 , 0x8 , 0x42,     // stw   R42,[R3e]    optional
 0xff
};





// A9L stack fiddler, call inside a pop, push

uchar stkfld [] = {
 11,0, 7,0, 'S','T','K','f','d','l', 0,                      // 7 stack fiddle (A9L)
 //maybe more than one ?? need some repeats here....
 0x30, 0x01, 0xe,  0x03,                                  //1 to 3 repeats, store in 14
 0x80, 0x0 , 0x0 , 0xcc,   0x10, 0x0 , 0x2 , 0x1e,        //   pop   R1e           # back up stack by one call

 0x80, 0x0 , 0x0 , 0x2d,   0x10, 0x2 , 0x0 , 0x17,        //   scall (sub)
 0x81, 0x0 , 0x6 , 0x04,                                  //  one data value ,flag ignore after test
 0x80, 0x0 , 0x0 , 0xc8,   0x10, 0x0 , 0x2 , 0x1e,        //  push   R1e
 0xff
};


// defines in shared.h

// handlers

void fnplu  (SIG*, SBK*);
void tbplu  (SIG*, SBK*);
void encdx  (SIG*, SBK*);
void ptimep (SIG*, SBK*);
void avcf   (SIG*, SBK*);
void rbasp  (SIG*, SBK*);
void rstkf  (SIG*, SBK*);
void sdummy  (SIG*, SBK*);

// void sigdummy  (SIG*, SBK*);

//uchar *subpatterns[] {

// for indexed lookup list
uchar *patterns[] = {rbase, avct, times, fnlu, tblu, tbl2, enc13, enc24, ttrps,  stkfld,  wtbl, gparfix, gparfixm, gparvar, gparvarm, wintp };

uchar *subpatts[] = {rbase };

// indexed sig handlers
SIGP sigproc[] = {0, avcf, ptimep,fnplu, tbplu, encdx, rbasp, rstkf, sdummy, sdummy };          // sigdummy };




#define SCANPATSTART     1
#define SCANPATEND       14


//************************************************************************



/* order of operands is 1 [0], 2, 3 where 1 and 0 might be a word (not 2 or 3 ??)
// for STX, swop order to  2, [0], 1  //  CHECK THIS.....

2b4f: 67,    01,5c,01,32      ad2w  R32,[R0+15c]     R32 += Tot_Inj_L;
2b54: a3,    01,5e,01,34      ldw   R34,[R0+15e]     R34 = Tot_Inj_H;

2b5c: c3,    01,5e,01,34      34, 01,15e,01          stw   R34,[R0+15e]  -> ldw  [0+15e] ,R34   [15e] = R34
2b61: c3,    01,5c,01,32      stw   R32,[R0+15c]     [0+15c] = R32;


 * by bytes ->  01,5e,01,34         ldw 34.... 0+15e 1 [0], 2  1 [+inx], 2
 *              34,01,5e,01         stw   34, 0+15e  2,1,[0]   2, 1[+inx]
 *
 * */





//************************************************************************


void sdummy  (SIG*, SBK*)
{
      #ifdef XDBGX
 DBGPRT(1,"** in sig dummy handler ");
 #endif
}

uchar *skip_operands(uchar *start)
{
    if (*start == SIGOCOD) start += 4;

    while (*start == SIGOPER) start += 4;     // skip operands
   return start;
}


void set_sigscan(uint xofst, SBK* ts)
  {
   memset(ts,0,sizeof(SBK));    // safety clear
   ts->curaddr = xofst;
   ts->start   = xofst;
   ts->stop    = 0;
  }



void match_operand(MMH *m, OPS *o)
{
    //match or save a single operand.

  uint val, ix, mp, mv;
  uint *v;

  v   = m->sgn->v;             // easier to read....

  if (m->sx[1] & SIGIGN) {m->sx += 4; return;}         // ignore value

if (m->mc)
{
    DBGPRT(0,0);
}

  ix  = m->sx[2] & 0xf;        // save index

  if (m->sx[1] & SIGINX)
    {
      ix += m->mc;                 // moving index
      ix += 16;          // count index for repeats;
      if (ix > 31) ix =  m->sx[2] & 0xf;        // safety, ignore other variable
    }

  mp = m->sx[3];               // pattern value (use if no index)

 if (o->imm) val = o->val; else val = o->addr;        //this can be a WORD

// drop lowest bit in pattern and value
 if ((o->inc || o->inx) || (m->sx[1] & SIGLBT)) {val &= 0xfe; mp &= 0xfe;}

 if (m->sx[1] & SIGBNK) val = databank(val, &dnst);

 if (ix && !v[ix]) v[ix] = val;

 if (m->highb)
   {
     mv = (val >> 8) & 0xff ;                   //  match value
     if (ix)  mp = (v[ix] >> 8) & 0xff;         // patt, or saved value
   }
 else
 {
     mv = val & 0xff;                           //  match value
     if (ix)  mp = v[ix] & 0xff;
 }

if (mp != mv)
  {
  m->smatch = 0;
  }
  m->highb = 0;
  if(m->smatch) m->sx += 4;
}



void do_operand(MMH *m)
{
 // do single operand

OPS *o;

  if (!m->sx) return;           // safety

  if (m->sx[0] == SIGOCOD) m->sx += 4;

  if (m->sx[0] != SIGOPER)
     {
      m->smatch = 0;             // not valid data sig
      return;
     }

  if (m->opno == 1 && dnst.opcix > 9 && dnst.opcix < 54) o = dnst.opr + 4;    // 1, copied to 4 for multiple mode
  else o = dnst.opr + m->opno;           // 2 & 3


  match_operand(m,o);         // current ix

  if(!m->smatch) return;

  //extra terms of bytes for word/long ops etc

  // immediate, if word do high byte - are these by ->val though ???

  if (o->imm && (o->fend & 31) > 7)
   {
    m->highb = 1;
    match_operand(m, o);
      if(!m->smatch) return;
   }

// indirect but allow for an indexed in the pattern to be treated as [Rx + 0]
// mainly for arg getters...

  if (o->ind && m->patsub == 3)           // ind for inx
  {
     m->sx += 4;      //skip this operand
     return;
  }





  //indexed, should match op[0]. but in two bytes if a word/long

  if (o->inx)
   {
    match_operand(m,dnst.opr);      //  op [0]

    if(!m->smatch) return;

    if ( (dnst.opr->fend & 31) > 7)            // long indexed
     {
       m->highb = 1;         // match high byte as well
       match_operand(m,dnst.opr);
       if (!m->smatch) return;
     }
   }
 }



uint do_opcskp(uint foff)
  {
    // skip any 'optional' opcodes.  max of 16 allowed.
    // skippable are f2-f7, fa-ff,

    int ans, opc;

    ans = 0;

    while(1)
      {
       opc = g_byte(foff);                // code file value

       if (opc < 0xf2)  break;           // not skippable (all up to pushp)
       if (opc == 0xf8) break;           // clear carry
       if (opc == 0xf9) break;           // set   carry

       foff++;
       ans++;
       if (ans >= 15 ) break;            // skip up to 15 opcodes
       if (!val_rom_addr(foff)) break;
      }
    return ans;                          // number of skips
   }




void do_jump_operand(MMH *m)
{
 // do operands for JUMP opcodes

 OPS *o;
 uint ix;

 if (!m->sx) return;           // safety

 if (m->sx[0] == SIGOCOD) m->sx += 4;

// now in first data item, register or destination

 if (m->sx[0] != SIGOPER)
     {
      m->smatch = 0;             // not valid data sig
      return;
     }

  if (dnst.numops > 1)
    {             // djnz and jb/jnb etc - match register address
     o = dnst.opr + 2;
     match_operand(m,o);

    }

 if (!m->smatch) return;

 if (m->sx[0] != SIGOPER)
     {
      m->smatch = 0;             // not valid data sig
      return;
     }

   ix = m->sx[2] & 0xf;
   o  = dnst.opr + 1;
   o->imm = 1;               //match offset value, not address
   match_operand(m,o);


   if (!m->smatch) return;
       // match opnd checks offset.
       //  save actual destination if tval set, and set optype for conditionals

     if (ix)
        {
         m->sgn->v[ix] = o->addr;
         if (dnst.sigix == 19 || dnst.sigix == 21)
          {
           if (dnst.opcix & 1)  m->sgn->v[ix] |= 0x1000000;
          }
        }
 }



void match_opcode(MMH *m)
 // verify opcode match, save bit number, check for ldx versus stx
 {
  uint t, ix, bit;
  const OPC *opl;
  uint *v;

  v = m->sgn->v;
  ix = m->sx[2] & 0xf;
  bit = dnst.opcde & 7;

  t = get_opc_inx(m->sx[3]);                // last is actual opcode
  opl  = get_opc_entry(t);                  // opcode table entry for sig


// ldx versus stx.
  m->oswp = 0;
  if (dnst.sigix == 12 && opl->sigix == 13) m->oswp = 1;
  else
  if (dnst.sigix == 13 && opl->sigix == 12) m->oswp = 1;
  else
  if (!dnst.sigix || dnst.sigix != opl->sigix) m->smatch = 0;

  if (dnst.sigix == 23)
   {        //jb, jnb
    if ((m->sx[1] & SIGBIT) && ix)
     {
          // not quite same as standard operand match
      if (!v[ix])
        {
         v[ix] = bit;      // save bit number
        }

      if (v[ix] != bit) m->smatch = 0;       // must match bitnum

      if ((dnst.opcix & 1))  m->sgn->v[ix] |= 0x1000000;    // save bit jump type  JNB=0, JB=1
     }
   }







         //end bit

   if (m->sx[3] & SIGOPC && ix)
     {                     // save address mode
       v[ix] = dnst.opcsub;
     }
 m->patsub = m->sx[3] & 3;              // keep address mode for operand checks

 m->skip = 1;         // skips allowed after first opcode processed
}



void match_data(MMH *m)
{
    // plain to start with
  uint ix, mp, mv;
  uint *v;

  if (m->sx[1] & SIGIGN)
    {
     m->sx += 4;
     dsc.curaddr++;              // next byte
     dsc.nextaddr++;
     return;
    }         // ignore value

  v = m->sgn->v;                 // param array
  ix = m->sx[2] & 0xf;           // and its index

  mp = m->sx[3];                 // pattern value (use if no index

  mv = g_byte(dsc.curaddr);      // match value

  if (ix)
    {
      if (!v[ix])  v[ix] = mv;          // save data value
      if (v[ix] != mv) m->smatch = 0;   // must match
    }
  else
    {
      if (mp != mv) m->smatch = 0;      // must match
    }

  if (m->smatch)
    {
     m->sx += 4;                 // next pattern
     dsc.curaddr++;              // next byte
     dsc.nextaddr++;
    }

}




void get_sigopcode(MMH *m)
{
    // get the next opcode and operands, only
  int skp;

// could do a if datapatt return datamatch()....

  if (!m->sx) return;           // safety

 if (m->sx[0] != SIGOCOD)
     {
      m->smatch = 0;   // not valid opcode sig - redundant ???
      return;
     }

  skp = 0;
  if (m->skip)
    {
      skp = do_opcskp(dsc.curaddr);
      dsc.curaddr += skp;
    }

  if (!val_rom_addr(dsc.curaddr) || skp > 16)
     {
      m->smatch = 0;
      return;          // too many skips or hit end bank
     }

  do_code (&dsc, &dnst);                     // one opcode at a time,like a scan
}



void match_operands(MMH *m)
 {                 // do operands for one opcode

  int ix;

  if (!m->sx) return;           // safety
  if (!m->smatch) return;


  if (m->sx[0] == SIGOCOD) m->sx += 4;       // skip opcode pattern

// match operands. Different jump logic

  if (dnst.opcix > 53 && dnst.opcix < 80) do_jump_operand(m);

  else

    // swop operands for stx,ldx
  if (m->oswp)
    {
      for (ix = 2; ix > 0; ix--)
      {
       if (!m->smatch) break;
       m->opno = ix;
       do_operand(m);
      }
    }

 else
    // do operands

  for (ix = 1; ix <= dnst.numops; ix++)
    {
       if (!m->smatch) break;
       m->opno = ix;
       do_operand(m);
    }

 }






 void do_stdpat(MMH *m)
  {  // split up for multipatterns get/match opcode, then operands.
     get_sigopcode(m);
     match_opcode(m);
     if (m->smatch) dsc.curaddr = dsc.nextaddr;      // next opcode

     if (dbgflag)
     {
         DBGPRT(0,0);
     }


     match_operands (m);
  }


 void do_datpat(MMH *m)
  {
    match_data(m);

  }

void do_subpat(MMH *m)
 {
   // subpattern - "x of y must match" in any order.
   // max of 32 patts.

  int score, ix, i, min, num;
  uchar *start;

  min  = m->sx[1];          // num of opcode patts which must match (once each)
  num  = m->sx[3];          // num of patterns
  ix   = m->sx[2];          // index where to save count

  m->sx += 4;
  start = m->sx;          // start of patterns

 // assemble pat starts in an array. Keep one too many for resume after pattern....
 // don't know where opcode patterns are so scan for them

  for (i = 0; i <= num; i++)
   {
    savepatts[i] = start;                // next opcode (or end)
    if (*start != SIGOCOD) break;
    start = skip_operands(start);
   }

  score = 0;
  start = savepatts[0];


  memcpy(lv, m->sgn->v, NSGV*sizeof(int));     // save pars

// check ALL patterns for one opcode and operands

  while (score < num  && m->smatch)
    {
      get_sigopcode(m);        //from bin file

      for (i = 0; i < num; i++)
       {
        m->mc = 0;

        if (savepatts[i])
          {
           m->sx = savepatts[i];
           m->smatch = 1;                             // reset match
           memcpy(lv, m->sgn->v, NSGV*sizeof(int));    //save pars

           match_opcode(m);
           match_operands (m);

           if (m->smatch)
            {
             score++;
             m->mc++;
             savepatts[i] = 0;          // matched
             dsc.curaddr = dsc.nextaddr;
             break;             // inner loop, next opcode
            }
           else
            {
             memcpy(m->sgn->v, lv, NSGV*sizeof(int));       //restore pars
            }
          }    // x[i]
       }      // while i



      if (!m->mc)
         {
           m->smatch = 0;   // at least one patt must match for each pass
           break;            //outer loop
         }
    }  // while score

      if (score >= min)
         {                // pass
          m->smatch = 1;
          if (ix)   m->sgn->v[ix] = score;
          }

   m->sx = savepatts[num];         // next pattern after this subpattern
 }


 void do_datsub(MMH *m)
  {


  }

 void do_skppat(MMH *m)
 {
   // 'skip to defined pattern'
// min - max

  int ix, max;
  uchar *start;

  max  = m->sx[1];          // num of skips
  ix   = m->sx[2];          // index where to save count

  start = m->sx + 4;        // start of next pattern, assume ONE to match

  m->mc = 0;                   //skip count

  if (max < 1) max = 1;       //safety

  while (m->mc < max)
   {
     m->sx = start;         // reset to match
     get_sigopcode(m);
     match_opcode(m);
     match_operands(m);
     dsc.curaddr = dsc.nextaddr;       // next opcode

     if (m->smatch) break;    // found opcode and operands

     m->mc++;                    // skip count
     m->smatch = 1;           // reset match
   }

//check skip count.......??

 if (ix && m->smatch) m->sgn->v[ix] = m->mc;

 }



 void do_rptpat(MMH *m)
  {
    // to match between x and y repeats, for single pattern
    //  where x can be zero and y is one (= optional)
   int ix, min, max;
   uchar *start;

  min   = m->sx[1];          // min repeats
  max   = m->sx[3];          // max repeats
  ix    = m->sx[2] & 0xf;    // index where to save count
  start = m->sx + 4;

  m->mc = 0;                     // repeat count
  if (ix) m->sgn->v[ix] = 0;     // count in array - set to zero

  while (m->mc < max)
   {
     m->sx = start;        //reset for pattern
     get_sigopcode(m);
     match_opcode(m);
     match_operands(m);    //  has m->match check inside

     if (!m->smatch)
       {
         m->sx = skip_operands(m->sx);
         break;
       }

     m->mc++;                              // match count
     if (ix) m->sgn->v[ix] = m->mc;        // save count for next loop
     dsc.curaddr = dsc.nextaddr;        //next opcode
   }


 if (m->mc < min || m->mc > max) m->smatch = 0; else m->smatch = 1;

 }

uint match_sig (MMH *m, uint ofst);

void do_childsig (MMH *master)
{
    //true subpattern ?? what, multiple list ?? optional ? another match_sig(m, dsc->curaddr ?);
//what about options? need to save in case of no match.


/* need to have LOCAL MMH struct, separate from master,
 * and use uchar* pointer for new sig.
 */
 // SIG *z, *r;
 MMH *lhold;          //local

 lhold = (MMH*) cmem(0,0, sizeof(MMH));         // have own mhold

 lhold->sx = subpatts[master->sx[1]];
 lhold->sgn = master->sgn;
 lhold->smatch = 1;                                 // set match to start with

 match_sig(lhold, dsc.curaddr);

 if (!lhold->smatch) master->smatch = 0;

 mfree(lhold,sizeof(MMH));

}

 /* fl = *(m->sx);                // next pattern flag
  //   if (fl == 0xff) break;        // end of master pattern - prob not reqd ?? safety

 // m->sx[0] is 0x50....
 // m->sx[1] is index ??   0-255 ??
  *
  * from top level do_sig
  z = (SIG *) chmem(CHSIG,1); //do we need this for a subsig ??
  mhold->sgn = z;

     z->hix  = mhold->sx[2];                            // handler index ?? for subsig ??
  z->v[0] = mhold->sx[3];                            // extra (fixed) param
  strncpy(z->name, (char *)(mhold->sx + 4),14);      // copy name
   *
   *
   *
   *
   * *
 // min  = m->sx[1];          // number of subpatts
 //   allows for 2 subpatts, or extra 4 for up to 6 subpatts. or have a zero for 3, 7 ....
 // max  = m->sx[3];          // max repeats
 // ix   = m->sx[2];          // index where to save count

// so this could PART match and still fail... need alternate scan and inst ???

  //SBK  dsc;                   // dummy scan uses curaddr, nextaddr in here
 // INST dnst;                  // one instance
  // no headers in subpatterns

  lhold = (MMH*) cmem(0,0, sizeof(MMH));         // have own mhold

  lhold->sx = patterns[pat];
  lhold->sgn = master->sgn;
  lhold->smatch = 1;                                 // set match to start with

 * from top level do_sig
 z = (SIG *) chmem(CHSIG,1);
  mhold->sgn = z;

  z->hix  = mhold->sx[2];                            // handler index
  z->v[0] = mhold->sx[3];                            // extra (fixed) param
  strncpy(z->name, (char *)(mhold->sx + 4),14);      // copy name

  if (mhold->sx[1] & 1) z->novlp = 1;               //no overlaps.

  match_sig (lmhold, ofst);      match_sig(m, dsc.curaddr ?);  // seems right....

  if (mhold->smatch)
    {
      // add vaid sig to sig chain WRONG !!

      r = add_sig(z);                               // insert sig into chain

      if (r != z) return r;                         // duplicate, return original
           #ifdef XDBGX
           DBGPRT(1, "%x add sig %s",z->start,z->name);
           #endif
      return z;
    }

  mfree(mhold,sizeof(MMH));
  return 0;

 }*/


















 void do_datrpt(MMH *m)
  {


  }





uint match_sig (MMH *m, uint ofst)
{
  uchar fl;                        // flags

  set_sigscan(ofst, &dsc);         // set up scan block for do_code calls

  while (m->smatch)
   {
     if (!m->sx)  m->smatch = 0;             //no pattern, safety`

     if (!val_rom_addr(dsc.curaddr)) m->smatch = 0;   // fail if drop off bank end

     #ifdef XDBGX
         if (!m->smatch && dbgflag) DBGPRT(1, "sig failed %s at %x , pat %x, %x, %x %x", dsc.curaddr, m->sgn->name, m->sx[0], m->sx[1], m->sx[2], m->sx[3]);
     #endif

if (dsc.curaddr == 0x92fb2)
{
    DBGPRT(0,0);
}

     if (m->smatch == 0) break;    // failed

     fl = *(m->sx);                // next pattern flag
     if (fl == 0xff) break;        // end of pattern - safety

     switch(fl)
     {       //should only ever be one value set

      //opcode patterns, match opcode + operands

      case SIGOCOD :        // std opcode pattern - do one entry. NB. used in multi patterns
        do_stdpat(m);
        break;

      case SIGOSKP:         // 'skip to opcode X' pattern
        do_skppat (m);
        break;

      case SIGOSUB:         // subpattern  (any order match x of y)
        do_subpat (m);
        break;

      case SIGORPT:         // opcode repeat and optional
        do_rptpat(m);
        break;

      case SIGCHILD:     // full pattern as child sig         [1] is flags [2] is subpattern index
        do_childsig(m);
        break;

      case SIGDATA:         // single data pattern
        do_datpat (m);
        break;

      case SIGDRPT:         // data pattern - optional or repeat
        do_datrpt (m);
        break;

      case SIGDSUB:         // data pattern - match x of y in any order
        do_datsub (m);
        break;

      default:
        m->smatch = 0;
        break;              // failed (safety)
     }        // end of switch
    }       // end of while smatch

  if (dsc.curaddr <= ofst) m->smatch = 0;      // must have at least one matched patt

   #ifdef XDBGX
      if (!m->smatch && dbgflag)
       DBGPRT(1, "sig failed %s at %x , pat %x, %x, %x %x", m->sgn->name, dsc.curaddr, m->sx[0], m->sx[1], m->sx[2], m->sx[3]);
   #endif

  if (m->smatch == 0) return 0;

  m->sgn->start = ofst;                  // update start and end addresses
  m->sgn->end   = dsc.curaddr;

  return ofst;
}











SIG* do_sig (uint pat, uint ofst)
{
 SIG *z, *r;
 MMH *mhold;

  if (get_anlpass() >= ANLPRT) return 0;
  if (!val_rom_addr(ofst))     return 0;

  // should check 'pat' is valid...............

  mhold = (MMH*) cmem(0,0, sizeof(MMH));

  mhold->sx = patterns[pat];

  r = 0;
  z = (SIG *) chmem(CHSIG,1);
  mhold->sgn = z;

  z->hix  = mhold->sx[2];                            // handler index
  z->v[0] = mhold->sx[3];                            // extra (fixed) param
  strncpy(z->name, (char *)(mhold->sx + 4),14);      // copy name

  if (mhold->sx[1] & 1) z->novlp = 1;               //no overlaps.

  #ifdef XDBGX
   if ((mhold->sx[1] & 2) && ofst == 0x92f93) dbgflag = 1;               //no overlaps.V
   else dbgflag = 0;
 #endif

  mhold->sx += mhold->sx[0];                         // skip header block
  mhold->smatch = 1;                                 // set match to start with

  match_sig (mhold, ofst);

  if (mhold->smatch)
    {
      // add vaid sig to sig chain
      r = add_sig(z);                               // insert sig into chain

      #ifdef XDBGX
      if (r)  DBGPRT(1, "%x add sig %s",z->start,z->name);
      #endif

    }

   mfree(mhold,sizeof(MMH));
  return r;
 }


void do_sigproc (SIG* g, SBK* s)
{
  if (g && g->hix)
    {
     sigproc[g->hix].pr_sig(g,s);      // call signature processor
    }
}


SIG* scan_sigs (int ofst)
{
// scan ofst for signatures from list supplied. EXTERNALLY visible

  uint j;
  SIG *s;

  if (get_cmdopt(OPTMANL)) return 0;
  if (!get_cmdopt(OPTSIG)) return 0;

//allow for some to be dropped off.........
  for (j = SCANPATSTART; j <= SCANPATEND; j++)
      {

          //for this with do_one_opcode, is it more efficient to do the opcode once and THEN check all the sigs ??

        s = do_sig (j, ofst);        // code list
        if (s) break;                       // stop as soon as one found
      }

 return s;
}

/*
 *
 * only bank 8 and rbase sig to find ???
 */
void prescan_sigs (void)
{
 // scan whole binary file (exclude fill areas) for signatures first.
 // do preprocess if reqd

  uint ofst;
 // uint i, j;
  SIG *s;
  BANK *b;

  if (get_cmdopt(OPTMANL)) return ;             // manual stops it ??
 // if (!get_cmdopt(OPTSIG)) return ;          //not if rbases, must ALWAYS look for it ?.

  #ifdef XDBGX
   DBGPRT(1,0);
   DBGPRT(1," -- Scan whole bin for pre sigs --");
  #endif


 // for (i = 0; i < BMAX; i++)
 //  {
     b = get_bkdata(9);           //i);
   //  if (!b->bok) continue;

     for (ofst = b->minromadd; ofst < b->maxromadd; ofst++)
       {
         show_prog (get_anlpass());

    //     for (j = SCANPATSTART; j <= SCANPATEND; j++)
     //       {
             s = do_sig (PATRBSE, ofst);                  // prescan list
             if (s)
              {
               ofst = s->end;                          //sigs can't overlap
               if (s->hix)  sigproc[s->hix].pr_sig(s,0);    // call handler for signature
               break;
              }
         //   }
       }
  // }

 #ifdef XDBGX
   DBGPRT(2," -- End Scan whole bin --");
  #endif
}



/// END of sign.cpp
