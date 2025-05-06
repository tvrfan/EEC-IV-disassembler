/*****************************************************
 *  Signature Module version 5
 ******************************************************/
#include "sign.h"          //calls shared.h


uchar *savepstart[32];      // pattern start pointers holder for subpatterns (x of y), zero if matched


//use offsets for separate file ???

// uchar * index [] ??  but must be done as offsets....not direct char* array


/*each sig starts with header which is -

 if (mh->sx[0] > 1)
    {
      if (mh->sx[1] & 1) mh->novlp   = 1;               // no overlaps.
      if (mh->sx[1] & 2) mh->dbgflag = 1;
    }

  if (mh->sx[0] > 2)     mh->hix  = mh->sx[2];                     // handler index
  if (mh->sx[0] > 3)     mh->par1 = mh->sx[3];                     // extra (fixed) param     }
  if (mh->sx[0] > 4)     mh->sname = (char *)(mh->sx + 4);         // name




 * size of header
 -----    size of parameter array ?

 * flags

 * handler subroutine (index)
 * extra global parameter
 * name
 * zero to terminate name


*/

//Rbase loop for 2020 or 2060

uchar rbase [] = {
 10, 0, 6, 0, 'r','b','a','s','e',0,    // handler 6 rbase lookup
 0x80, 0x0 , 0x0 , 0xa1,   0x10, 0x0 , 0x1 , 0xf0,   0x10, 0x0,  0x0,  0x0 ,   0x10, 0x0 , 0x2 , 0x18,
 0x80, 0x0 , 0x0 , 0xb3,   0x10, 0x0 , 0x0 , 0x1 ,   0x10, 0x1 , 0x3 , 0x20,   0x10, 0x0 , 0x3 , 0x20,  0x10, 0x0 , 0x5 , 0x1a,
 0x80, 0x0 , 0x0 , 0xa2,   0x10, 0x0 , 0x6 , 0x15,   0x10, 0x0 , 0x7 , 0x1c,
 0x80, 0x0 , 0x0 , 0xc2,   0x10, 0x0 , 0x2 , 0x19,   0x10, 0x0 , 0x7 , 0x1c,                // STW or LDX
 0x80, 0x0 , 0x0 , 0xe0,   0x10, 0x0 , 0x5 , 0x1a,   0x10, 0x0 , 0x8 , 0xf7,                 //djnz
 0xff
};


// Avct using stkptr2 as task pointer for background task list - handler 1

uchar avct [] = {                //bwak3n2 82477

 9, 0, 1, 0 ,   'a', 'v','c', 'l',  0,

 0x80, 0x0 , 0x0 , 0xa1 ,   0x10, 0x1 , 0x4 , 0x8a,  0x10, 0x0, 0x4, 0xaf,  0x10, 0x0 , 0x0 , 0x22,      // ldw   R22,af8a
 0x82, 0x3 , 0x0 , 0x0  ,                                                                     // skip up to 3 opcodes
 0x80, 0x0 , 0x0 , 0xb1 ,   0x10, 0x0 , 0x0 , 0x11,   0x10, 0x0 , 0x7 , 0x11,                            // R11 = 11  bank select
 0x82, 0xf , 0x0 , 0x0 ,                                                                      // skip up to 15 opcodes
 0x80, 0x0 , 0x0 , 0xc9,     0x10, 0x2, 0x0, 0x0,   0x10, 0x2, 0x0, 0x0,                      // push (word)
 0x80, 0x0 , 0x0 , 0xf0,                                                                      // ret or reta
 0xff
};



uchar times [] = {
  10,0,  2, 1,    't','i','m','e','r',0,            // handler 2 - timer list early AA, A9L

  0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x1 , 0x30,   0x10, 0x0 , 0x2 , 0x3c,    // ldb R3c, [R30++];      (AA 3e73)
  0x80, 0x0 , 0x0 , 0x99,   0x10, 0x0 , 0x0 , 0x0 ,   0x10, 0x0 , 0x2 , 0x3c,    // cmpb  R3c,0
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0x0 , 0x55,                              // je    3ed0

  0x81, 0x01, 0xe,  0x02,                                                        // repeat for word (1 or 2 bytes)
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


 // func lookup - byte or word handler 3

uchar fnlu [] = {
 10, 0, 3, 0, 'f','n','l','u','1', 0,

 0x80, 0x0 , 0x0 , 0x9b,   0x10, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x2 , 0x2 ,   0x10, 0x0 , 0x8 , 0x34,
 0x80, 0x8 , 0xb , 0x31,   0x10, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x4 ,


 0x80, 0x8 , 0x0 , 0xdb,   0x10, 0x0 , 0x0 , 0xa ,
 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x2 ,

 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x0 , 0x0 , 0x6 ,

 0x80, 0x0 , 0x0 , 0x65 ,  0x10, 0x0 , 0x2 , 0x2 ,   0x10, 0x0, 0x0, 0x0,  0x10, 0x0 , 0x4 , 0x32,
 0x80, 0x0 , 0x0 , 0x27,   0x10, 0x2 , 0x0 , 0xed,

 0x81, 0x0 , 0x0,  0x1,                                                        // optional -  min, ix, max
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


ORIGINAL - note skip and only mult
uint  ttrps[] = {     // table interpolate - signed. flag is SET for signed

0x980e2001,  0x2000033,                  // multiple clrb   (optional ?)
0x880a3017,  0x3002d,   0x7001a,         // jnb   B7,R2d, x    table signed flag [10];
0x40098000,                            // skip up to 8 opcodes, (cnt in [9])
0x86003008,  0x2003a,   0x5003c         // sml2w R3c,R3a     MUST HAVE fe
};

xdt2

   Sub_83087:
83087: 11,34              clrb  R34              R34 = 0;
83089: 11,36              clrb  R36              R36 = 0;
   Sub_8308b:
8308b: 11,38              clrb  R38              R38 = 0;
8308d: 37,99,22           jnb   B7,R99,830b2     if (B7_R99 = 1)  {
83090: 08,01,38           shrw  R38,1            R38 >>= 1;
83093: a0,34,3e           ldw   R3e,R34          R3e = R34;
83096: fe,4c,36,38,34     sml3w R34,R38,R36      slR34 = swR38 * R36;
8309b: a0,3e,34           ldw   R34,R3e          R34 = R3e;
8309e: 0a,01,3e           asrw  R3e,1            swR3e >>=  1;
830a1: 64,36,3e           ad2w  R3e,R36          R3e += R36;
830a4: fe,6c,38,34        sml2w R34,R38          slR34 = swR34 * R38;
830a8: 68,36,3e           sb2w  R3e,R36          R3e -= R36;
830ab: 09,01,3e           shlw  R3e,1            R3e <<= 1;
830ae: 09,01,38           shlw  R38,1            R38 <<= 1;
830b1: f0                 ret                    return; }


*/

// this works for A9L type tblu,. but not the later one..........
// table interpolate - no specific handler - for signed/unsigned checks
//no hanlder - maybe there should be one ?

uchar ttrps [] = {
 12, 0, 0, 0,  't','a','b','i','n','t','p',0,

 0x81, 0x0, 0x0, 0x4 ,                                                         // 0 to 4 repeats of ...
 0x80, 0x0, 0x0, 0x1 ,   0x10, 0x2 , 0x0 , 0x33,                               // clr

 0x80, 0x8 , 0xa , 0x37,   0x10, 0x0 , 0x3 , 0x2d,   0x10, 0x0 , 0x7 , 0x1a,   // jnb   B7,R2d,373c      if (Tblsflg = 1)

 0x82 ,0x8, 0x0, 0x0,                                                          // skip up to any 8 opcodes to a
 0x80, 0x2 , 0x2,  0x6c,   0x10, 0x2 , 0x0 , 0x3a,   0x10, 0x2 , 0x5 , 0x3c,   // (0xfe) sml2w R3c,R3a     slR3c = swR3c * R3a;  for sign- ness

 // (what about sml3w ??) make 2 and 3 ops interchangeable ??



//temp cut here
0xff };

/*maybe chop here ??  only need jb/jnb really.
 0x80, 0x0 , 0x0 , 0xbc,   0x10, 0x0 , 0x1,  0x33,   0x10, 0x0 , 0x2 , 0x3a,  // ldsbw R3a,R33          swR3a = yR33;
 0x80, 0x0 , 0x0 , 0xbc,   0x10, 0x0 , 0x4,  0x31,   0x10, 0x0 , 0x5 , 0x3c,  // ldsbw R3c,R31          swR3c = yR31;
 0x80, 0x0 , 0x0 , 0x68,   0x10, 0x0 , 0x5,  0x3c,   0x10, 0x0 , 0x2 , 0x3a,  // sb2w  R3a,R3c          R3a -= R3c;
 0x80, 0x0 , 0x0 , 0xac,   0x10, 0x0 , 0xc,  0x30,   0x10, 0x0 , 0x5 , 0x3c,  // ldzbw R3c,R30          wR3c = yR30;
 0x80, 0x6 , 0x0 , 0x6c,   0x10, 0x0 , 0x2 , 0x3a,   0x10, 0x0 , 0x5 , 0x3c,  // (0xfe) sml2w R3c,R3a     slR3c = swR3c * R3a;  for sign- ness
 0xff
};  */

//need 22CA one, and xdt2 one, and ....
/************** word tables ->
  table addr   = s->v[4]
  table cols   = s->v[3]
   * from interpolate sig
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

 // table lookup, byte, early      - hanlder 4

uchar tblu [] = {
 10, 0, 4,0, 't','b','l','u','1' ,0,

// 0x81, 0x0,  0x0,  0x1 ,                  //optional
// 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x2 , 0x0 , 0xfe,   0x10, 0x2 , 0x0,  0xf8,                              // an2b  Rf8,fe

 0x80, 0x0 , 0x0 , 0x5c,   0x10, 0x0 , 0x8 , 0x33,   0x10, 0x0 , 0x3 , 0x34,   0x10, 0x0 , 0x9 , 0x36,     // ml3b  R36,R34,R33 ml3b
 0x80, 0x0 , 0x0 , 0x74,   0x10, 0x0 , 0x6 , 0x31,   0x10, 0x0 , 0x9 , 0x36,                               // ad2b  R36,R31

 0x80, 0x40, 0x0 , 0xd3,   0x10, 0x0 , 0x0 , 0x2 ,                                                         // jnc
 0x80, 0x40, 0x0 , 0x17,   0x10, 0x0 , 0x5 , 0x37,                                                         // incb  R37
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x9 , 0x36,   0x10, 0x0 , 0x4 , 0x38,                               // ad2w  R38,R36
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x39,   0x10, 0x0 , 0x6 , 0x31,                               // ldb   R31,[R38++]
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x38,   0x10, 0x0 , 0x8 , 0x33,                               // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x1 , 0x1c,                                                         // call  (interpolate)
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x3 , 0x34,   0x10, 0x0 , 0x4 , 0x38,                               // ad2w  R38,R34
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x38,   0x10, 0x0 , 0x8 , 0x33,                               // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0x05,   0x10, 0x0 , 0x4 , 0x38,                                                         // decw  R38
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x38,   0x10, 0x0 , 0x6 , 0x31,                               // ldb   R31,[R38]
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x7 , 0x3a,   0x10, 0x0 , 0x3 , 0x34,                               // ldb   R34,R3b
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0xc ,                                                         // call
 0xff
};

 /* table lookup, later multibank
  table addr   = s->v[4]
  table cols   = s->v[3]
   * from interpolate sig
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

uchar tbl2 [] = {                         //handler 4
 10, 0, 4,0, 't','b','l','u','2',0,

// 0x81, 0x0,  0x0,  0x1 ,                  //optional
// 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x2 , 0x0 , 0xfe,   0x10, 0x2 , 0x0,  0xf8,                              // an2b  Rf8,fe

 0x80, 0x0 , 0x0 , 0x5c,   0x10, 0x0 , 0x8 , 0x37,   0x10, 0x0 , 0x3 , 0x38,   0x10, 0x0 , 0x5 , 0x3a,    // ml3b  R36,R34,R33
 0x80, 0x0 , 0x0 , 0x74,   0x10, 0x0 , 0x9 , 0x35,   0x10, 0x0 , 0x5 , 0x3a,                              // ad2b  R36,R31
 0x80, 0x0 , 0x0 , 0xb4,   0x10, 0x0 , 0x0 , 0x0 ,   0x10, 0x0 , 0x2 , 0x3b,                              // adcb  R37,R0
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x5 , 0x3a,   0x10, 0x0 , 0x4 , 0x3c,                              // ad2w  R38,R36
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x9 , 0x35,                              // ldb   R31,[R38++]
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x8 , 0x37,                              // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0x64,   0x10, 0x0 , 0x3 , 0x38,   0x10, 0x0 , 0x4 , 0x3c,                              // ad2w  R38,R34
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0x7 , 0x34,   0x10, 0x0 , 0x6 , 0x39,                              // ldb   R35,R30
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0xe , 0x36,   0x10, 0x0 , 0x2 , 0x3b,                              // ldb   R37,R32
 0x80, 0x20, 0x0 , 0x28,   0x10, 0x2 , 0x1 , 0x1c,                                                        // call
 0x80, 0x0 , 0x0 , 0xb2,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x8 , 0x37,                              // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0xb3,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x0 , 0xff,   0x10, 0x0 , 0x9 , 0x35,    // ldb   R31,[R38+ff]
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0x4 , 0x3c,                              // ldw   R38,R3a
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x1 , 0xc ,                                                        // call

 /*
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0xe , 0x36,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x7 , 0x34,
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0x2 , 0x3b,   0x10, 0x0 , 0x6 , 0x39,
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0x1c ,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0x4 , 0x3c,
 0x80, 0x8 , 0xa , 0x37,   0x10, 0x0 , 0xf , 0xd8,   0x10, 0x2 , 0x0 , 0x1a,      //jnb is sign marker   */
 0xff
};

/* tbl3 ??
5548: ad,07,34            ldzbw R34,7            wR34 = 7;
554b: 45,fa,00,88,38      ad3w  R38,R88,fa       R38 = 3792;

 / table lookup, later multibank
  table addr   = s->v[4]
  table cols   = s->v[3]
   * from interpolate sig
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

uchar tbl3 [] = {                         //handler 4          (1AGB)

10, 0, 4,0, 't','b','l','u','3',0,

// 0x81, 0x0,  0x0,  0x1 ,                  //optional
//0x80, 0x00, 0x00, 0x71,   0x10, 0x00, 0x08, 0xfe,    0x10, 0x00, 0x02, 0xf5,                              // an2b  Rf5,fe

0x80, 0x00, 0x00, 0x5c,   0x10, 0x00, 0x08, 0x33,    0x10, 0x00, 0x03, 0x34,    0x10, 0x00, 0x05, 0x36,   // ml3b  R36,R34,R33
0x80, 0x00, 0x00, 0x74,   0x10, 0x00, 0x06, 0x31,    0x10, 0x00, 0x05, 0x36,                              // ad2b  R36,R31
0x80, 0x00, 0x00, 0xb4,   0x10, 0x02, 0x00, 0x00,    0x10, 0x00, 0x07, 0x37,                              // adcb  R37,R0
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x05, 0x36,    0x10, 0x00, 0x04, 0x38,                              // ad2w  R38,R36
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x04, 0x38,    0x10, 0x00, 0x06, 0x31,                              // ldb   R31,[R38++]
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x04, 0x38,    0x10, 0x00, 0x08, 0x33,                              // ldb   R33,[R38]
0x80, 0x20, 0x00, 0x28,   0x10, 0x02, 0x01, 0x39,                                                         // scall 2181
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x03, 0x34,    0x10, 0x00, 0x04, 0x38,                              // ad2w  R38,R34
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x04, 0x38,    0x10, 0x00, 0x08, 0x33,                              // ldb   R33,[R38]
0x80, 0x00, 0x00, 0x05,   0x10, 0x00, 0x04, 0x38,                                                         // decw  R38
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x04, 0x38,    0x10, 0x00, 0x06, 0x31,                              // ldb   R31,[R38]
0x80, 0x00, 0x00, 0xa0,   0x10, 0x00, 0x0a, 0x3a,    0x10, 0x00, 0x03, 0x34,                              // ldw   R34,R3a
0x80, 0x00, 0x00, 0x28,   0x10, 0x02, 0x00, 0x29,                                                         // scall 2181
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
//par1 set for word interpolate

uchar wtbl [] = {
 10, 0, 4, 1, 'w', 't','b','l','u',0,                                    // 4  table lookup, par 1 for word sized

0x80, 0x00, 0x00, 0x5c,   0x10, 0x00, 0x0b, 0x37,    0x10, 0x00, 0x03, 0x38,    0x10, 0x00, 0x02, 0x3e,   // ml3b  R3e,R38,R37
0x80, 0x00, 0x00, 0xb0,   0x10, 0x00, 0x0a, 0x34,    0x10, 0x00, 0x05, 0x3a,                              // ldb   R3a,R34
0x80, 0x00, 0x00, 0xb0,   0x10, 0x00, 0x06, 0x36,    0x10, 0x00, 0x07, 0x3b,                              // ldb   R3b,R36
0x80, 0x00, 0x00, 0xac,   0x10, 0x00, 0x08, 0x35,    0x10, 0x00, 0x0a, 0x34,                              // ldzbw R34,R35
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x0a, 0x34,    0x10, 0x00, 0x02, 0x3e,                              // ad2w  R3e,R34
0x80, 0x00, 0x00, 0x09,   0x10, 0x00, 0x09, 0x01,    0x10, 0x00, 0x02, 0x3e,                              // shlw  R3e,1
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x02, 0x3e,    0x10, 0x00, 0x08, 0x3c,                              // ad2w  R3c,R3e
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x08, 0x3c,    0x10, 0x00, 0x0a, 0x34,                              // ldw   R34,[R3c++]
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x08, 0x3c,    0x10, 0x00, 0x06, 0x36,                              // ldw   R36,[R3c]
0x80, 0x00, 0x00, 0x28,   0x10, 0x02, 0x01, 0x25,                                                         // scall 92e8e
0x80, 0x00, 0x00, 0x09,   0x10, 0x00, 0x09, 0x01,    0x10, 0x00, 0x03, 0x38,                              // shlw  R38,1
0x80, 0x00, 0x00, 0x64,   0x10, 0x00, 0x03, 0x38,    0x10, 0x00, 0x08, 0x3c,                              // ad2w  R3c,R38
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x04, 0x3c,    0x10, 0x00, 0x06, 0x36,                              // ldw   R36,[R3c]
0x80, 0x00, 0x00, 0x05,   0x10, 0x00, 0x04, 0x3c,                                                         // decw  R3c
0x80, 0x00, 0x00, 0x05,   0x10, 0x00, 0x04, 0x3c,                                                         // decw  R3c
0x80, 0x00, 0x00, 0xa2,   0x10, 0x00, 0x04, 0x3c,    0x10, 0x00, 0x0a, 0x34,                              // ldw   R34,[R3c]
0x80, 0x00, 0x00, 0xa0,   0x10, 0x00, 0x02, 0x3e,    0x10, 0x00, 0x03, 0x38,                              // ldw   R38,R3e
0x80, 0x00, 0x00, 0x28,   0x10, 0x02, 0x01, 0x10,                                                         // scall 92e8e

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

/* next bunch not used with emulate........
//Simple fixed arg getter single bank (e.g. A9L 3695)
//level in [f]
// arg count in [e]
//arg list in [6] onwards

uchar gparfix[] = {

11, 0, 8, 0, 'g', 'p', 'a', 'r','f', 's', 0,

0x81, 0x1 , 0xf,  0x4,                                                           //       repeat between 1 and 4 times - pop level
0x80, 0x00, 0x00, 0xcc,   0x10, 0x08, 0x10, 0x38,                                // pop   Rx location and match cnt in index[2].
0x81, 0x1 , 0xe,  0x8,                                                           //       repeat between 1 and 8 times
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x2, 0x3a,    0x10, 0x08, 0x8, 0x3c,       // ldb   R3c,[Rx++] list in [8] onwards
0x80, 0x00, 0x00, 0xc8,   0x10, 0x02, 0x00, 0x3a,                                // push  Rx

0xff
};



// optional setbank in all multibanks, after mbpop - flag the register [1] if found, need two list pointers...

uchar setbank[] = {
10, 0, 1, 0, 's', 't', 'b', 'n','k', 0,                              //calls setbnk (= 1)

0x80, 0x00, 0x00, 0x18,   0x10, 0x00, 0x00, 0x02,    0x10, 0x04, 0x01, 0x37,                              // shrb  R37,2 (saved as R36)
0x80, 0x00, 0x00, 0xb0,   0x10, 0x04, 0x01, 0x37,    0x10, 0x00, 0x00, 0x11,                              // ldb   R11,R37

0xff
};


// child sig for a single 'pop' equivalent with optional bank select following, for multibanks
// setbank will FLAG registers as bank selects and keep offsets....
// ldw set as multiple because called as multiple from "gpar...m"  patterns

uchar mbpop [] = {
10, 0, 2, 0, 'm', 'b', 'p', 'o','p', 0,                         // calls cpoph (= 2)

0x80, 0x00, 0x00, 0xa3,   0x10, 0x00, 0x00, 0x20,    0x10, 0x08, 0x0c, 0x04,    0x10, 0x08, 0x10, 0x3a,   // ldw   R3a,[R20+x] (reg from 16, offset from 12]

0x51, 0x0, 0xc, 0x1, 0x0, 0x10,          // optional child (setbank), (shrb+ldb R11) (2 and 5 as list indexes to match above pat=setbank).

0xff
};









// xdt2 multibank fixed arg getter

//not that 'pops + bankl select' won't ever be more than 2 (bank select + address) it's not required with [r20+x] style
// but could be a problem with R20+x style ???
//Simple fixed arg getter single bank (e.g. A9L 3695)

// single bank version has
// level in [f]
// arg count in [e]
// arg list in [6]

// This version
// level in [f] but needs processing
// arg count in [e]

//arg list in [16] onwards
// [1] for mbpop register, [2] is save offset, [3] is param address register [4] temp count ?

uchar gparfixm[] = {

11, 1, 8, 1, 'g', 'p', 'a', 'r','f', 'm', 0,   //calls fixpar

0x51, 0x1, 0xf, 0x4, 0x1, 0x10,             // 1 to 4 repeats of sig 'mbpop' (1) as child sig INDEXES ARE IMPORTANT !!  indexes should be in [2] and [5]

0x81, 0x1 , 0xe,  0x8,                                                                                    // 1 to 8 args
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x02, 0x3a,    0x10, 0x08, 0x6, 0x36,                               // ldb   R36,[R3a++], save list from 6

0x80, 0x00, 0x00, 0xb1,   0x10, 0x00, 0x00, 0x11,    0x10, 0x00, 0x00, 0x11,                              // ldb   R11,11
0x80, 0x00, 0x00, 0xc3,   0x10, 0x00, 0x00, 0x20,    0x10, 0x00, 0x03, 0x04,    0x10, 0x00, 0x02, 0x3a,   // stw   R3a,[R20+4]  //where pars are written back
0xff
};



// variable arg getter  (e.g. A9L  77d2)  - may need repeat on first pop ?

//no of args via register in [2], drawn from binary directly
//need to sort out param list !!! and multiple pops probably
// levels [f] is first (param extracted), [e] is seconfd, (level is [f] + [e])

uchar gparvar[] = {

11, 0, 9, 0,'g', 'p', 'a', 'r','v', 's' , 0,

0x81, 0x1 , 0xf,  0x4,                                                           //       repeat between 1 and 4 times - pop level for no of args
0x80, 0x00, 0x00, 0xcc,   0x10, 0x08, 0x10, 0x3c,                                                         // pop   R3c (list from [16]
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x01, 0x3c,    0x10, 0x00, 0x02, 0x3a,                              // ldb   R3a,[R3c++]
0x81, 0x1 , 0xe,  0x4,                                                           //       repeat between 1 and 4 times - pop level for args
0x80, 0x00, 0x00, 0xcc,   0x10, 0x08, 0x18, 0x42,                                                         // pop   R42 (list from [24]
0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x03, 0x42,    0x10, 0x00, 0x05, 0x3b,                              // ldb   R3b,[R42++]
0x80, 0x00, 0x00, 0xc6,   0x10, 0x00, 0x04, 0x16,    0x10, 0x00, 0x05, 0x3b,                              // stb   R3b,[R16++]
0x80, 0x00, 0x00, 0xe0,   0x10, 0x00, 0x02, 0x3a,    0x10, 0x00, 0x06, 0xf7,                              // djnz  R3a,77c9       //loop for args
0x80, 0x00, 0x00, 0xc8,   0x10, 0x00, 0x03, 0x42,                                                         // push  R42
0x80, 0x00, 0x00, 0xc8,   0x10, 0x00, 0x01, 0x3c,                                                         // push  R3c

0xff
};

// xdt2 variable arg getter same as single bank above ....

 //9->3

uchar gparvarm[] = {

11, 1, 9, 1,'g', 'p', 'a', 'r','v', 'm', 0,
//child pat    --fake pop + optional bank select

0x51, 0x1, 0xf, 0x4, 0x1, 0x10,             // 1 to 4 repeats of sig 'mbpop' as child sig after bank select ones dropped.

0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x01, 0x40,    0x10, 0x00, 0x02, 0x3e,                              // ldb   R3e,[R40++]
0x80, 0x00, 0x00, 0xc3,   0x10, 0x00, 0x00, 0x20,    0x10, 0x00, 0x09, 0x02,    0x10, 0x00, 0x01, 0x40,   // stw   R40,[R20+2]

//child pat    --fake pop + optional bank select

0x51, 0x1, 0xe, 0x2, 0x1, 0x18,             // 1 to 4 repeats of sig 'mbpop' as child sig  - but this overwrites !!

0x80, 0x00, 0x00, 0xb2,   0x10, 0x00, 0x03, 0x46,    0x10, 0x00, 0x0b, 0x3f,                              // ldb   R3f,[R46++]
0x80, 0x00, 0x00, 0xc6,   0x10, 0x00, 0x04, 0x26,    0x10, 0x00, 0x0b, 0x3f,                              // stb   R3f,[R26++]
0x80, 0x00, 0x00, 0xe0,   0x10, 0x00, 0x02, 0x3e,    0x10, 0x00, 0x07, 0xf7,                              // djnz  R3e,8a548
//0x80, 0x00, 0x00, 0xb1,   0x10, 0x00, 0x05, 0x11,    0x10, 0x00, 0x05, 0x11,                              // ldb   R11,11

0xff };


*/
 // encoded address type 4 and 2, (xdt2)      hanlder 5

uchar enc24 [] = {
 10, 0, 5 ,4,  'e','n','c','2','4',0,
 0x80, 0x0 , 0x0 , 0xac,    0x10, 0x4 , 0x8 , 0x37,   0x10, 0x0 , 0xa , 0x3a,    // ldzbw R3a,R37;
 0x80, 0x0 , 0x0 , 0x71 ,   0x10, 0x0 , 0x3 , 0xf,    0x10, 0x4 , 0x8 , 0x37,    // an2b  R37,0x1f (t4) or 0xf (t2)
 0x80, 0x0 , 0x0 , 0x08 ,   0x10, 0x0 , 0x0 , 0x4 ,   0x10, 0x0 , 0xa , 0x3a,    // shrw  R3a,4

 0x81, 0x0 , 0x0,  0x1,                                                        // optional -  min, ix, max
 0x80, 0x40, 0x0 , 0x71 ,   0x10, 0x0 , 0x0 , 0xe ,   0x10, 0x0 , 0xa , 0x3a,    // an2b  R3a,e     not in type 2

 0x80, 0x0 , 0x0 , 0x67 ,   0x10, 0x0 , 0xa , 0x3a,   0x10, 0x0 , 0x5 , 0xf0,  0x10, 0x0,0x0, 0x0,   0x10, 0x0 , 0x9 , 0x36,   // R36 += [R3a+f0];
 0xff
};

 // encoded address types 3 and 1    - handler 5

uchar enc13 [] = {
 10, 0, 5, 3, 'e','n','c','1','3', 0,
 0x80, 0x0 , 0x0 , 0x37,   0x10, 0x4 , 0x8 , 0x42,   0x10, 0x2 , 0x0 , 0x14,    // jnb   B7,R43,3cda  (drop bit)
 0x80, 0x0 , 0x0 , 0xac,   0x10, 0x4 , 0x8 , 0x42,   0x10, 0x0 , 0x4 , 0x3a,      // ldzbw R3a,R43      (drop bit)

 0x83, 0x3 , 0x0 , 0x3 ,                                                          // 3 of 3 must match, any order
 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x0 , 0x0 , 0xf ,   0x10, 0x4 , 0x8 , 0x42,  // an2b  R43,f
 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x0 , 0x3 , 0x70,   0x10, 0x0 , 0x4 , 0x3a,  // an2b  R3a,70  (0xfe is a t1, 0x70 is a t3)
 0x80, 0x0 , 0x0 , 0x18,   0x10, 0x0 , 0x0 , 0x3 ,   0x10, 0x0 , 0x4 , 0x3a,    // shrw  R3a,3

 0x80, 0x0 , 0x0 , 0x67,   0x10, 0x0 , 0x4 , 0x3b,   0x10, 0x0 , 0x5 , 0xf0,   0x0, 0x0, 0x0, 0x0, 0x10, 0x0 , 0x8 , 0x42, // ad2w  R42,[R3a+f0]
 0x81, 0x0 , 0x0,  0x1 ,                                                            // 1 optional opcode
 0x80, 0x40, 0x1 , 0xc2 ,  0x10, 0x0 , 0x9 , 0x3e,   0x10, 0x0 , 0x8 , 0x42,     // stw   R42,[R3e]    optional
 0xff
};





/* A9L stack fiddler, call with one param inside a pop, push- a'special' probably (spf 7, containing a 6)
not used with emulate model

uchar stkfld [] = {
 11,0, 7,0, 'S','T','K','f','d','l', 0,                      // 7 stack fiddle (A9L)
 //maybe more than one ?? need some repeats here....
 0x81, 0x01, 0xe,  0x03,                                  //1 to 3 repeats, store in 14
 0x80, 0x0 , 0x0 , 0xcc,   0x10, 0x0 , 0x2 , 0x1e,        //   pop   R1e           # back up stack by one call

 0x80, 0x0 , 0x0 , 0x2d,   0x10, 0x2 , 0x0 , 0x17,        //   scall (sub)
 0x20, 0x0 , 0x6 , 0x04,                                  //  one data value ,flag ignore after test (maybe this should be a skip ?)
 0x80, 0x0 , 0x0 , 0xc8,   0x10, 0x0 , 0x2 , 0x1e,        //  push   R1e
 0xff
};

*/
// defines in shared.h

// handlers

void fnplu  (SIG*, SBK*);
void tbplu  (SIG*, SBK*);
void encdx  (SIG*, SBK*);
void ptimep (SIG*, SBK*);
void avcf   (SIG*, SBK*);
void rbasp  (SIG*, SBK*);
//void stckf  (SIG*, SBK*);
//void fixpar  (SIG*, SBK*);
//void varpar  (SIG*, SBK*);
void sdummy  (SIG*, SBK*);

//void xdummy   (MMH*);

//void csetbnk  (MMH*);
//void cpoph    (MMH*);

// void sigdummy  (SIG*, SBK*);


// for indexed lookup list

//could assemble pattern lists on the fly, but need to have subpatts identified somehow, or put them in main list ??
//last two outside std scan range

uchar *patterns[] = {rbase, avct, times, fnlu, tblu, tbl2, tbl3, enc13, enc24, wtbl, ttrps, wintp };   //stkfld,  wtbl, gparvar, gparvarm, gparfix, gparfixm,  wintp };

uchar *subpatts[] = {};      //setbank, mbpop };

// indexed sig handlers 1     2     3      4      5       6      7     8        9     10
SIGP sigproc[] = {0, avcf, ptimep,fnplu, tbplu, encdx, rbasp, sdummy };     //stckf, fixpar, varpar,sdummy };          // sigdummy };

CHLDSUB  chlproc[] = {0};  // csetbnk, cpoph, xdummy, xdummy};          //child pattern handlers  (none yet)


#define PATRBSE         0    // rbase lookup
#define SCANPATSTART     1
#define SCANPATEND       9               //14


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
   #ifdef XDBGX



void dbg_sig(MMH *ch)
{
    // debug
  uint i;

  if (!ch) return;

  DBGPRT(0,"[%2d %2x %2x]", ch->mc, ch->cinx1, ch->cinx2);
  for (i = 0; i < 32; i++) DBGPRT(0," %2x", ch->lvals[i]);
  DBGPRT(1, "  %s", ch->sname);
}



void dbg_sigs(MMH *ch)
{

    //debug
  MMH* c;

  c = ch;

  while (c)
   {
    dbg_sig(c);
    c = c->caller;
   }
  DBGPRT(2,0);
}

   #endif




uint sigfail(MMH *m, uint ix)
 { //uint so that can do 'return sigfail(..)' and IX for tracking where

  m->sigmatch = 0;

  if (m->dbgflag)
     {
           #ifdef XDBGX
       DBGPRT(0, "%d sig failed %s at %x - ",ix, m->sname, m->c->ofst);
       dbg_sig(m);
       #endif
     }
  return 1;
}



void sdummy  (SIG* s, SBK* b)
{

 #ifdef XDBGX
 DBGPRT(1,"** in dummy sig handler for %s", s->name );
 #endif
}


void xdummy  (MMH *ch)
{

   #ifdef XDBGX
      DBGPRT(1,"** in dummy child handler");
      dbg_sigs(ch);

 #endif
}



/*

void cpoph  (MMH *ch)
{
  uint i, j, ix; //, dd; //, sx, cnt;     //, j;     //, k;

  MMH* caller;

  caller = ch->caller;

 DBGPRT(1,"\n\n** in cpoph for %x, %x",ch->sigstart, ch->scan->curaddr);

 if (ch->lvals[1])
  {          //only if reg found by child sig (via csetbnk subr)

    DBGPRT(1,"flag R%x", ch->lvals[1]);


  // lists in [12] indexes, and [16] registers. local so no cinx used.
  // ch->mc is stored in local [2] ix for each loop. saved at END in caller index ....

  for (i = 16; i < (uint) 16 + ch->mc; i++)
     {
       if (ch->lvals[i] == ch->lvals[1])
         {
          ch->lvals[i] |= 0x1000;
          ch->lvals[i-4] |= 0x1000;               // mark register AND index from [12]
          break;
         }
     }

  }


//need to do straight copy to caller (additive though), and now should have bank selects (pushp)
// to match with stack (I hope)


if (ch->cinx1 == 0xe)
{
    DBGPRT(0,0);
}





 // copy to caller, must check where caller list is at, as this is ADDITIVE , called in a loop.
 // so caller must hold a temp count somwhere.
 // caller->cinx1 (==f) and 2(=11)...final count and list.
 //inx1 for count, 2 for list.
 // caller->inhmc to inhibit count being written....cinx1


//this is an append to list which seems right.

// ch->cinx2.....

ix = ch->cinx2;           //where the list goes

 j = 0;                   //caller->lvals[4];
 for (i = 16; i < (uint) 16 + ch->mc; i++)
     {
       if (1)        //ch->lvals[i] < 0x1000)
       {
        caller->lvals[j+ix] = ch->lvals[i];
        caller->lvals[j+ix+4] = ch->lvals[i-4];
        j++;
       }
     }
  caller->lvals[ch->cinx1] = ch->mc;            //   caller->lvals[4] = j;

 /-  caller->lvals[4]; i < caller->lvals[4] + ch->mc; i++)


//dd = 0;              // deduct offset - must be saved in caller somewhere....
 j = caller->lvals[4];
 for (i = 16; i < (uint) 16 + ch->mc; i++)
     {
       if (ch->lvals[i] < 0x1000)
       {
        caller->lvals[j+16] = ch->lvals[i];
        caller->lvals[j+20] = (ch->lvals[i-4] - caller->lvals[1])/2 + 1;
        j++;
       }
      else
      {
        caller->lvals[1] += 2;        //seems to work....dropped index(es)...


      }
     }
     caller->lvals[4] = j;

-/





// dbg_sigs(ch);




//NB. this is called AFTER the loop, so CAN change totals in caller indexes safely, applies for caller loop, BUT
// changes are done INSIDE that caller loop


// copy valid ones up to caller, but must shuffle the index ...  can't verify where first ldw index starts.
// so for every flagged index, drop and subtract 2 from all subsequent entries.  THEN convert with /2 + 1







  //    if (!j && ch->lvals[i] == ch->lvals[1]) { j = i;}    // k = ch->lvals[i-4];}

  //    if (j && ch->lvals[i])          { ch->lvals[i] |= 0x1000;        //flag instead         ch->lvals[i+1];   // kill destination CAN'T DO THIS IF REGISTER REUSED !!!   (XDT@)

//             }
//     }

/-
 if (j)
  {
  for (i = 12; i < 16; i++)
        {

    ch->lvals[i] = ch->lvals[i+1]; // - 2;             // adjust indeces
        }
  }

if (j) {ch->caller->lvals[caller->cinx1]--;            //caller repeat index for this sub pattern
caller->mc--; }
//ch->lvals[12] -=k;
ch->caller->lvals[caller->cinx2] = ch->lvals[12];                // /2 + 1;   //test - first index used       WRONG!!!

/


}




void csetbnk  (MMH *ch)
{
  MMH* caller;
  uint i,j;           //, j;

  caller = ch->caller;

  DBGPRT(1,"\n\n** in csetbnk for %x, %x",ch->sigstart, ch->scan->curaddr);
  dbg_sigs(ch);

if (ch->lvals[1])
  {
     DBGPRT(1," pshp reg is %x", ch->lvals[1]);

    //{ DBGPRT(1,"no pshp");  return;}    // optional pattern still gets called if no match...
  j = ch->cinx2;

if (j)
{
  for (i = j; i <= j + ch->mc; i++)
     {
       if (caller->lvals[i] == ch->lvals[1])
         {
          caller->lvals[i] |= 0x1000;
    //      ch->lvals[i-4] |= 0x1000;               // mark index from [12]
          break;
         }
     }

}





// cant just use [1] if multiple loop calls !!! add flag to register in caller



 // caller->lvals[caller->cinx1] = ch->lvals[1];       //copy register up for caller handling
}

}


*/



uchar *skip_operands(uchar *start)
{
    if (*start == SIGOCOD) start += 4;

    while (*start == SIGOPER) start += 4;     // skip operands
   return start;
}


MMH* set_sigholder(uchar *pat, MMH *caller)
{           //set up local holder for pattern match, child or caller sig

  MMH *mh;
  char *x;
  size_t sz;

// set pointers,  hold struct + inst + scan + 2 uint arrays (save+local)

//make pars variable in size via the header pattern of each sig....


  sz = sizeof(MMH) + sizeof(INST) + sizeof(SBK) + (sizeof(uint) * (NSGV*2));        //add lvals

  x = (char*) cmem (0,0, sz);

  mh          = (MMH*)   x;
  mh->c       = (INST*) (x + sizeof(MMH));
  mh->scan    = (SBK*)  (x + sizeof(MMH) + sizeof(INST));

  //make these variable and size in the header pattern of each sig....


  mh->svals   = (uint*) (x + sizeof(MMH) + sizeof(INST) + sizeof(SBK));   // saved values
  mh->lvals   = mh->svals + NSGV;                                         // local values
  mh->msize   = sz;

  mh->sigmatch = 1;

  mh->sx = pat;

 // process header, all or part

  if (caller)
   {           //inherit flags first
    mh->skipcod = caller->skipcod;
    mh->dbgflag = caller->dbgflag;
    mh->caller  = caller;
    mh->sigstart = caller->sigstart;            // debug
   }

  if (mh->sx[0] > 1)
    {
      if (mh->sx[1] & 1) mh->novlp   = 1;               // no overlaps.
      if (mh->sx[1] & 2) mh->dbgflag = 1;
    }

  if (mh->sx[0] > 2)     mh->hix  = mh->sx[2];                     // handler index
  if (mh->sx[0] > 3)     mh->par1 = mh->sx[3];                     // extra (fixed) param     }
  if (mh->sx[0] > 4)     mh->sname = (char *)(mh->sx + 4);         // name

  mh->sx += mh->sx[0];                                             // skip header block

 // if begins with a repeat
  if (mh->sx[0] ==  SIGORPT) mh->novlp = 1;        // begins with repeat, set no overlap.

  return mh;
}


void free_sigholder(MMH *mh)
{
  mfree (mh, mh->msize);
}









void set_sigscan(uint xofst, SBK* ts)
  {
   memset(ts,0,sizeof(SBK));    // safety clear
   ts->curaddr = xofst;
   ts->start   = xofst;
   ts->nextaddr = xofst;


   // ts->stop    = 0;           //redundant

//   if (get_numbanks()) ts->datbnk = 2;          // data bank 1 for multibanks
//   else  ts->datbnk = 9;                        // 8 for single

 //  ts->codbnk = (xofst >> 16) & 0xf;          // code bank from start addr
  }



uint match_operand(MMH *m, OPS *o)
{
    //match and save a single operand.

  uint val, ix, mp, mv;
//  uint *v;

//  v   = m->lvals;        // easier to read....
  ix  = m->sx[2];        // save index

  if (!ix && (m->sx[1] & SIGIGN)) {m->sx += 4; return 1;}         // ignore value, skip, unless we want to save it....

  if (m->sx[1] & SIGINX)
    {
      ix += m->mc;                        // index is a LIST
      if (ix > 31) ix =  m->sx[2];        // safety wrap, ignore other variable
    }

  mp = m->sx[3];               // pattern value (use if no index)

 if (o->imd) val = o->val; else val = o->addr;        // this can be a WORD, and also a jump destination

// drop lowest bit in pattern and value
 if ((o->inc || o->inx) || (m->sx[1] & SIGLBT)) {val &= 0xfe; mp &= 0xfe;}

 if (m->sx[1] & SIGBNK) val = databank(val, m->c);    // done already ?  may be wrong for calls ?? breaks rbase without it - use imm ?

 if (ix && !m->lvals[ix]) m->lvals[ix] = val;

 if (m->highb)
   {
     mv = (val >> 8) & 0xff ;                       //  match high byte value
     if (ix)  mp = (m->lvals[ix] >> 8) & 0xff;      //  patt, or saved value
   }
 else
 {
     mv = val & 0xff;                           //  match value
     if (ix)  mp = m->lvals[ix] & 0xff;
 }

  if (mp != mv && !(m->sx[1] & SIGIGN)) return sigfail(m, 1);

  m->highb = 0;

  if(m->sigmatch) m->sx += 4;

  return 0;
}



void do_operand(MMH *m)
{
 // do single operand

OPS *o;
INST *c;


  if (!m->sx) return;           // safety

  if (m->sx[0] == SIGOCOD) m->sx += 4;

  if (m->sx[0] != SIGOPER)
     {
      sigfail(m, 2);           //m->smatch = 0;             // not valid data sig
      return;
     }

  c = m->c;


  if (m->opno == 1 && c->opcix > 9 && c->opcix < 54) o = c->opr + 4;    // 1, copied to 4 for multiple mode
  else o = c->opr + m->opno;           // 2 & 3


  match_operand(m,o);         // current ix

  if(!m->sigmatch) return;

  //extra terms of bytes for word/long ops etc

  // immediate, if word do high byte - are these by ->val though ???

  if (o->imd && (o->fend & 31) > 7)
   {
    m->highb = 1;                 // match high byte as extra
    match_operand(m, o);
    if (!m->sigmatch) return;
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
    match_operand(m,m->c->opr);      //  op [0]

    if(!m->sigmatch) return;

    if ( (m->c->opr->fend & 31) > 7)            // long indexed
     {
       m->highb = 1;                   // match high byte as extra
       match_operand(m,m->c->opr);
       if (!m->sigmatch) return;
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
       if (opc == 0xfe) break;           // sign or alt prefix

       foff++;
       ans++;
       if (ans >= 15 ) break;            // skip up to 15 opcodes
       if (!val_rom_addr(foff)) break;
      }
    return ans;                          // number of skips
   }




uint do_jump_operand(MMH *m)
{
 // do operands for JUMP opcodes

 OPS *o;
 uint ix, im;

 if (!m->sx) return 0;           // safety

 //cix = 0;                       // safety
 im  = 1;                         // saves - default to jump offset

 if (m->sx[0] == SIGOCOD)
   {
      // cix = m->sx[2];    // remember index from opcode patt  NOT REQD ?? maybe if need to flag -ve offset

     if (m->sx[1] & SIGADD) im = 0;  // swop offset / address save

     m->sx += 4;
   }
// now in first data item, register or destination

 if (m->sx[0] != SIGOPER) return sigfail(m,3);         // not valid data sig

 if (m->c->numops > 1)
    {             // djnz and jb/jnb etc - match register address first
     o = m->c->opr + 2;
     match_operand(m,o);
    }

 if (!m->sigmatch) return 0;

 if (m->sx[0] != SIGOPER) return sigfail(m,4);            // not valid data sig

   ix = m->sx[2];
   o  = m->c->opr + 1;

   // call has destination address default, not offset (swop over)
   if (m->c->sigix == 17) im ^= 1;

   if (im) o->imd = 1;               // match (and save) offset value, not address (default)
   match_operand(m,o);


   if (!m->sigmatch) return 0;
       // match opnd checks offset.
       //  save actual destination if tval set, and set optype for conditionals

     if (ix)
        {
         if (m->c->sigix == 19 || m->c->sigix == 21)
          {        //cond jump or set/clr carry. For address or offset, no good for negatives...
           if (m->c->opcix & 1)  m->lvals[ix] |= 0x1000000;
          }
        }
    return 0;
 }



uint match_opcode(MMH *m)
 // verify opcode match, save bit number, check for ldx versus stx
 {
  uint ix, bit;
  const OPC *opl;
//  uint *v;

//  v = m->lvals;
//  ix = m->sx[2];                //ix used for opcode lookup
  bit = m->c->opcode & 7;

 // t = get_opc_inx(m->sx[3]);                // last is actual opcode
 // opl  = get_opc_entry(t);                  // opcode table entry for sig

  ix = opcind[m->sx[3]];                       // opcode table index
  opl = opctbl + ix;

  ix = m->sx[2];



// ldx versus stx. allow but swop operands
  m->oswp = 0;
  if (m->c->sigix == 12 && opl->sigix == 13) m->oswp = 1;
  else
  if (m->c->sigix == 13 && opl->sigix == 12) m->oswp = 1;
  else
  if (!m->c->sigix || m->c->sigix != opl->sigix) return sigfail(m,5);           //m->smatch = 0;

  if (m->c->sigix == 23)
   {        //jb, jnb                  //not right if specified....
    if (ix && (m->sx[1] & SIGBIT))
     {
          // not quite same as standard operand match
      if (!m->lvals[ix]) m->lvals[ix] = bit;      // save bit number

      if (m->lvals[ix] != bit) sigfail(m,6);                   // must match bit num

      if ((m->c->opcix & 1))  m->lvals[ix] |= 0x1000000;     // save bit jump type  JNB=0, JB=1
     }
   }


  if (m->sx[1] & SIGMOPC)
     {     // address mode specified in [2]
           // if [2] == 3 (indexed) allow 2 as well, as a [Rx+0]

        if (m->sx[2] == 3)
          {
            if (m->c->opcsub < 2) sigfail(m,7);     //allow 2 or 3

            // NB. if 2 (ind) instead of 3 (inx), already handled in match operand

          }
        else
         {         //must match
            if (m->c->opcsub != m->sx[2]) sigfail(m,8);
         }
     }

   if (ix && (m->sx[1] & SIGFE))
     {                //save sign bit
       m->lvals[ix] = m->c->feflg;
     }


   if (ix && (m->sx[1] & SIGSOPC))
     {                     // save opcode address mode
       m->lvals[ix] = m->c->opcsub;
     }

   m->patsub = m->sx[3] & 3;              // keep reqd address mode for operand checks

 m->skipcod = 1;                          // skips allowed after first opcode processed
 return 1;
}



void match_data(MMH *m)
{
    // plain to start with
  uint ix, mp, mv;

  SBK *dsc;

  dsc = m->scan;

  if (m->sx[1] & SIGIGN)
    {
     m->sx += 4;
     dsc->curaddr++;              // next byte
     dsc->nextaddr++;
     return;
    }         // ignore value

  ix = m->sx[2];           // and its index
  mp = m->sx[3];                 // pattern value (use if no index

  mv = g_byte(dsc->curaddr);      // match value

  if (ix)
    {
      if (!m->lvals[ix])  m->lvals[ix] = mv;          // save data value
      if (m->lvals[ix] != mv) sigfail(m,9);           //m->smatch = 0;   // must match
    }
  else
    {
      if (mp != mv) sigfail(m,10);           //m->smatch = 0;      // must match
    }

  if (m->sigmatch)
    {
     m->sx += 4;                 // next pattern
     dsc->curaddr++;              // next byte
     dsc->nextaddr++;
    }

}




void get_sigopcode(MMH *m)
{
    // get the next opcode and operands, only
  int skp;
  SBK *dsc;

  dsc = m->scan;

// could do a if datapatt return datamatch()....

  if (!m->sx) return;           // safety

 if (m->sx[0] != SIGOCOD)
     {
      sigfail(m,11);           //m->smatch = 0;   // not valid opcode sig - redundant ???
      return;
     }

  skp = 0;
  if (m->skipcod)
    {
      skp = do_opcskp(dsc->curaddr);
      dsc->curaddr += skp;
    }

  if (!val_rom_addr(dsc->curaddr) || skp > 16)
     {
      sigfail(m,12);           //m->smatch = 0;
      return;          // too many skips or hit end bank
     }

  do_code (m->scan, m->c);                     // one opcode at a time,like a scan
}



uint match_operands(MMH *m)
 {                 // do operands for one opcode

  int ix;

  if (!m->sx) return 0;           // safety
  if (!m->sigmatch) return 0;




// match operands. Different jump logic

  if (m->c->opcix > 53 && m->c->opcix < 80) return do_jump_operand(m);

 if (m->sx[0] == SIGOCOD) m->sx += 4;       // skip opcode pattern


    // swop operands 1 and 2 for stx,ldx
  if (m->oswp)
    {
      for (ix = 2; ix > 0; ix--)
      {
       if (!m->sigmatch) break;
       m->opno = ix;
       do_operand(m);
      }
    }

 else
    // do operands in normal order
    {
  for (ix = 1; ix <= m->c->numops; ix++)
    {
       if (!m->sigmatch) break;
       m->opno = ix;
       do_operand(m);
    }
    }

  return 0;

 }






 void do_stdpat(MMH *m)
  {  // split up for multipatterns get/match opcode, then operands.

  SBK *dsc;

  dsc = m->scan;

     get_sigopcode(m);
     match_opcode(m);
     if (m->sigmatch) dsc->curaddr = dsc->nextaddr;      // next opcode

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
  SBK *dsc;

  dsc = m->scan;

  min  = m->sx[1];          // num of opcode patts which must match (once each)
  num  = m->sx[3];          // num of patterns
  ix   = m->sx[2];          // index where to save count

  m->sx += 4;
  start = m->sx;          // start of patterns

 // assemble pat starts in an array. Keep one too many for resume after pattern....
 // don't know where opcode patterns are so scan for them

  for (i = 0; i <= num; i++)
   {
    savepstart[i] = start;                // next opcode start (or end)
    if (*start != SIGOCOD) break;
    start = skip_operands(start);
   }

  score = 0;
  start = savepstart[0];


  memcpy(m->svals, m->lvals, NSGV*sizeof(int));     // save pars

// check ALL patterns for one opcode and operands

  while (score < num  && m->sigmatch)
    {
      get_sigopcode(m);        //from bin file

      for (i = 0; i < num; i++)
       {
        m->mc = 0;

        if (savepstart[i])
          {
           m->sx = savepstart[i];
           m->sigmatch = 1;                             // reset match
           memcpy(m->svals, m->lvals, NSGV*sizeof(int));   // save pars if match

           match_opcode(m);
           match_operands (m);

           if (m->sigmatch)
            {
             score++;
             m->mc++;
             savepstart[i] = 0;          // matched
             dsc->curaddr = dsc->nextaddr;
             break;             // inner loop, next opcode
            }
           else
            {
             memcpy(m->lvals, m->svals, NSGV*sizeof(int));       //restore pars if no match
            }
          }    // x[i]
       }      // while i



      if (!m->mc)
         {
           sigfail(m,13);           //m->smatch = 0;   // at least one patt must match for each pass
           break;            //outer loop
         }
    }  // while score

      if (score >= min)
         {                // pass
          m->sigmatch = 1;
          if (ix)   m->lvals[ix] = score;
          }

   m->sx = savepstart[num];         // next pattern after this subpattern
 }


 void do_datsub(MMH *m)
  {


  }

 uint do_skppat(MMH *m)
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
     m->scan->curaddr = m->scan->nextaddr;       // next opcode

     if (m->sigmatch) break;    // found opcode and operands

     m->mc++;                    // skip count
     m->sigmatch = 1;           // reset match
   }

//check skip count.......??
 if (m->mc >= max) return sigfail(m,15);

 if (ix) m->lvals[ix] = m->mc;

return 1;
 }



 uint do_rptpat(MMH *m)
  {
    // to match between x and y repeats, for single pattern
    //  where x can be zero and y is one (= optional)
   int ix, min, max;
   uchar *start;

  min   = m->sx[1];          // min repeats
  max   = m->sx[3];          // max repeats
  ix    = m->sx[2];          // index where to save count
  start = m->sx + 4;

  m->mc = 0;                     // repeat count

  while (m->mc < max)
   {
     m->sx = start;        // reset for pattern
     get_sigopcode(m);
     match_opcode(m);
     match_operands(m);    //  has m->match check inside

     if (!m->sigmatch)
       {
         m->sx = skip_operands(m->sx);   // skip rest of pattern
         break;
       }
     m->mc++;                              // match count
     m->scan->curaddr = m->scan->nextaddr;        //next opcode
   }


 //  if (m->mc < min || m->mc > max) m->sigmatch = 0; else m->sigmatch = 1;
  m->sigmatch = 1;               // can fail once in a repeat
  if (m->mc < min || m->mc > max) return sigfail(m,15);

  if (ix) m->lvals[ix] = m->mc;

//would call a handler here ??

 return 1;
 }

uint match_sig (MMH *m, uint ofst);


uint do_childsig (MMH *caller)
{
   // true child signature
   // MUST HAVE A HANDLER PROC, even if it's a dummy

 MMH *lhold;                    // must have local MMH structure for recursive call.

 lhold =  set_sigholder(subpatts[caller->sx[1]], caller);

 match_sig(lhold, caller->scan->curaddr);

 if (!lhold->sigmatch) return sigfail(caller,14);           //caller->smatch = 0;

  //update caller struct, address and pattern
 caller->scan->curaddr  = lhold->scan->curaddr;
 caller->scan->nextaddr = lhold->scan->nextaddr;
 caller->sx += 2;             // skip child sig entry
 chlproc[lhold->hix].pr_sig(lhold);

 free_sigholder(lhold);
return 1;
}

 uint do_rptchild(MMH *caller)
  {
    // match childsig between x and y repeats, for one child sig
    //  where x can be zero and y is one (= optional)

    // MUST HAVE A SIG HANDLER, or dummy

   int min, max;
   uchar *start;

   //6 byte header

//need to add handler maybe, e.g. for paramter changes after child sig (e.g. bank select drops a par from 'pop'


  MMH *lhold;           //local sig holder for recursive call.

  lhold =  set_sigholder(subpatts[caller->sx[4]], caller);    // child sig to call is in [4]

  min   = caller->sx[1];             // min repeats
  max   = caller->sx[3];             // max repeats

  lhold->cinx1 = caller->sx[2];     // remote index for value list or count, in caller
  lhold->cinx2 = caller->sx[5];     // second remote index

  caller->sx += 6;                   // skip entry in caller pattern


  start = lhold->sx;                 // to restart the pattern

  lhold->mc = 0;                     // repeat count

  while (lhold->mc < max)
   {
     lhold->sx = start;
     match_sig(lhold, caller->scan->curaddr);

     if (!lhold->sigmatch)  break;             // can fail match if not repeat

    // update caller struct, address and pattern

    lhold->mc++;

    caller->scan->curaddr  = lhold->scan->curaddr;
    caller->scan->nextaddr = lhold->scan->nextaddr;

   }

  lhold->sigmatch = 1;                      // can fail once and still match
  if (lhold->mc < min || lhold->mc > max)
      {
           free_sigholder(lhold);
          return sigfail(caller,15);
      }

  // matched, run handler, once for this loop

           //   if (lhold->cinx1) caller->lvals[lhold->cinx1] = lhold->mc;                   // save loop count upwards, BEFORE handler
  chlproc[lhold->hix].pr_sig(lhold);                            // handler can change count


//for mbpop handler,

// remove popp entry, adjust count, then copy count and destination to caller sig.



 //#ifdef XDBGX
 //   repfail(caller,3);

 //  #endif


  caller->skipcod = 1;      //can skip opcodes once passed

 free_sigholder(lhold);

 return 1;

 }


 void do_datrpt(MMH *m)
  {


  }












uint match_sig (MMH *m, uint ofst)
{
  uchar fl;                        // flags

  SBK *dsc;

  dsc = m->scan;

  set_sigscan(ofst, dsc);         // set up scan block for do_code calls

  while (m->sigmatch)
   {
     if (!m->sx)  sigfail(m,16);           //m->smatch = 0;             //no pattern, safety`

     if (!val_rom_addr(dsc->curaddr)) sigfail(m,17);           //m->smatch = 0;   // fail if drop off bank end


/*
if (dsc->curaddr == 0x92fb2)
{
    DBGPRT(0,0);
}
*/

     if (m->sigmatch == 0) break;    // failed

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

      case SIGCHILD:        // full sig pattern as child sig         [1] is flags [2] is subpattern index
        do_childsig(m);
        break;

      case SCHORPT:
        do_rptchild(m);
        break;



      case SIGDATA:         // single data pattern
        do_datpat (m);
        break;
/*
      case SIGDRPT:         // data pattern - optional or repeat
        do_datrpt (m);
        break;

      case SIGDSUB:         // data pattern - match x of y in any order
        do_datsub (m);
        break;      */

      default:
        sigfail(m,18);           //m->smatch = 0;
        break;              // failed (safety)
     }        // end of switch
    }       // end of while smatch

  if (dsc->curaddr <= ofst) sigfail(m,19);           //m->smatch = 0;      // must have at least one matched patt


  if (m->sigmatch == 0) return 0;

  return ofst;
}











SIG* do_sig (uint pat, uint ofst)
{
 SIG *z, *r;
 MMH *mhold;

  if (anlpass >= ANLPRT) return 0;        //get_anlpass() >= ANLPRT) return 0;
  if (!val_rom_addr(ofst))     return 0;

  // should check 'pat' is valid...............

  r = 0;

  mhold = set_sigholder(patterns[pat], 0);

//if (pat > NC(patterns))
//{
//DBGPRT(0,0);
//}

  mhold->sigstart = ofst;               //debug

 // if (ofst != 0x9a526) mhold->dbgflag = 0;

  // check if sig previously found and overlaps already ?? only if same sig and no overlaps set ??


  match_sig (mhold, ofst);

  if (mhold->sigmatch)
    {
      // match - copy details and add valid sig

      z = (SIG *) chmem(&chsig);
      z->start = ofst;                      // update start and end addresses
      z->end   = mhold->scan->curaddr-1;
      z->hix   = mhold->hix;
      z->par1  = mhold->par1;
      z->novlp = mhold->novlp;
      strncpy(z->name, mhold->sname,14);      // copy name
      memcpy( z->vx,   mhold->lvals, sizeof(uint) * NSGV);

      r = add_sig(z);                    // insert sig into chain

      #ifdef XDBGX
      if (r)  DBGPRT(1, "%x %x *** add sig %s",z->start,z->end, z->name);
      else   DBGPRT(1, "%x %x *** add sig FAIL %s",z->start,z->end, z->name);
      #endif

    }

   free_sigholder(mhold);
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

  if (PMANL) return 0;              //get_cmdopt(OPTMANL)) return 0;
  if (!PSIG) return 0;              //!get_cmdopt(OPTSIG)) return 0;


//if (ofst == 0x92f9f)
//{
 //   DBGPRT(0,0);
//}




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

  if (PMANL) return ;             //get_cmdopt(OPTMANL)) return ;             // manual stops it ??
 // if (!get_cmdopt(OPTSIG)) return ;          //not if rbases, must ALWAYS look for it ?.

  #ifdef XDBGX
   DBGPRT(1,0);
   DBGPRT(1," -- Scan bank 8 for pre sigs --");
  #endif


 // for (i = 0; i < BMAX; i++)
 //  {
     b = bkmap + 9;                   //get_bkdata(9);           //i);
   //  if (!b->bok) continue;

     for (ofst = b->minromadd; ofst < b->maxromadd; ofst++)
       {
         show_prog (anlpass);               //get_anlpass());

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
