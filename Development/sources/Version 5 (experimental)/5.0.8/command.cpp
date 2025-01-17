
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version number and notes in shared.h
 ******************************************************/


#include "command.h"

 char  flbuf [256];            // for directives and comment file reads
 char  cnm   [300];            // string hacking (and main cmd line as temp)

// command processors

 uint set_vect  (CPS *); uint set_code  (CPS *); uint set_scan (CPS *); uint set_xcode (CPS *);
 uint set_args  (CPS *); uint set_sdata (CPS *); uint set_rbas (CPS *); uint set_sym  (CPS *);
 uint set_subr  (CPS *); uint set_bnk   (CPS *); uint set_time (CPS *); uint set_tab  (CPS *);
 uint set_func  (CPS *); uint set_stct  (CPS *); uint set_opts (CPS *); uint clr_opts (CPS *);
 uint set_cpsw  (CPS *); uint set_lay   (CPS *); uint set_dmy  (CPS *); uint set_ldata (CPS *);


// print processors

 uint pp_dflt  (uint, LBK *); uint pp_wdbt  (uint, LBK *); uint pp_text  (uint, LBK *);
 uint pp_vect  (uint, LBK *); uint pp_code  (uint, LBK *); uint pp_stct  (uint, LBK *);
 uint pp_dmy   (uint, LBK *); uint pp_dlng  (uint, LBK *); uint pp_timer (uint, LBK *);

// main command definition structure
// Params -
// Processor subr, Print subr,
// <gap> min pars, max pars expected,  4 par types, <gap> name allowed,
// <gap> min addnl levels, max addnl levels, default size (=fend),
// <gap>  merge allowed, string type opts,
// <gap>  override mask/calc_type,  global options , main print options string, subopts string

// max sublevels ??


DIRS dirs[23] = {

{ set_sdata, pp_dflt,  2, 2,  4,5,0,0,   0,    0,  0,  1,   1, 0,  0,    0,       0              , 0    },      // fill  (default) MUST be entry zero
{ set_sdata, pp_wdbt,  1, 2,  4,5,0,0,   1,    0,  1,  1,   1, 0,  1,    0,       "PSUXVY=" , "BUS="  },        // byte
{ set_sdata, pp_wdbt,  1, 2,  4,5,0,0,   1,    0,  1,  2,   1, 0,  3,    0,       "PSUXV="  , "BUSY="  },       // word
{ set_ldata, pp_dlng,  2, 2,  4,4,0,0,   1,    0,  1,  3,   0, 0,  7,    0,       "PSUXV="  , 0},               // triple
{ set_ldata, pp_dlng,  2, 2,  4,4,0,0,   1,    0,  1,  4,   0, 0,  15,   0,       "PSUXV="  , 0},               // long
{ set_sdata, pp_text,  2, 2,  4,5,0,0,   0,    0,  0,  1,   1, 0,  1,    0,        0        , 0} ,              // text
{ set_vect, pp_vect,   1, 2,  1,2,0,0,   1,    0,  1,  2,   1, 0,  7,    0,       "DKQ"   , 0},                 // vect
{ set_tab,  pp_stct,   2, 2,  4,5,0,0,   1,    1,  1,  1,   0, 0,  0x1f,    "C",     "LOPSUVWXY=", 0 },            // table
{ set_func, pp_stct,   2, 2,  4,5,0,0,   1,    1,  2,  1,   0, 0,  0x1f,    "C",     "LPSUVWXY=", 0 } ,            // func
{ set_stct, pp_stct,   2, 2,  4,5,0,0,   1,    1,  15, 1,   0, 0,  7,    "QAC",   "DELNOPRSUVWXY|=", "BUS=" },  // struct


{ set_code, pp_code,   2, 2,  1,2,0,0,   0,    0,  0,  0,   1, 0,  2047, 0,        0  , 0} ,                    // code

//these next two go in aux chain
{ set_args, pp_dflt,   1, 2,  1,2,0,0,   0,    1, 15, 1,   0, 0,  0,    0,       "DELNOPSUVWXY|=",0 },         // args
{ set_xcode,pp_dmy,    2, 2,  4,5,0,0,   0,    0,  0, 0,   1, 0,  0,    0,        0  , 0} ,                   // xcode


{ set_subr, pp_dmy,    1, 1,  1,0,0,0,   1,    0, 15, 0,   0, 0,  0,    "ACF=",  "DELNOPSUVWXY|", 0 },          // subr (no equals)
{ set_scan, pp_dmy ,   1, 1,  1,0,0,0,   0,    0,  0, 0,   0, 0,  0,    0,        0  ,0 },                    // scan
{ set_rbas, pp_dmy,    2, 4,  3,4,6,7,   0,    0,  0, 0,   0, 0,  0,    0,        0  ,0 },                    // rbase

{ set_sym,  pp_dmy,    1, 3,  4,6,7,0,   1,    0,  1, 0,   0, 0,  0,    0,       "BFIWN"  ,0  },              // sym
{ set_bnk,  pp_dmy,    2, 4,  0,0,0,0,   0,    0,  0, 0,   0, 0,  0,    0,        0 ,0},                      // bank

{ set_opts, pp_dmy,    0, 0,  0,0,0,0,   0,    0,  0, 0,   0, 1,  0,    0,        0 ,0},                  // set options (separate strings array)
{ clr_opts, pp_dmy,    0, 0,  0,0,0,0,   0,    0,  0, 0,   0, 1,  0,    0,        0 ,0},                  // clear options
{ set_cpsw, pp_dmy,    2, 2,  1,1,0,0,   0,    0,  0, 0,   0, 0,  0,    0,        0 ,0},                  // psw setter for '0=0' jumps

{ set_dmy,  pp_dmy,    0, 0,  0,0,0,0,   1,    0,  0, 0,   0, 2,  0,    0,        0 ,0},                  // set math formula 'calc' (name =)
{ set_time, pp_timer,  2, 3,  4,5,0,0,   1,    0,  0, 0,   0, 0,  0,    0,        0, 0}   //    "NTUWY" },            // timer
//need to put timer back....

};


// and add a struct GROUP to do nice layout.


/****************************************************
 * LOOKUP strings for use in many places in commands.
 * indexed below for master index to save lots of pointers
 *****************************************************/

// main command strings

cchar *cmds[23] =

{"fill"   , "byte"  , "word"  , "triple", "long" , "text"   , "vect" ,
 "table"  , "func"  , "struct", "code"  , "args" , "xcode"  ,
 "subr"   , "scan"  , "rbase" , "symbol", "bank" , "setopts",
 "clropts", "pswset", "calc"  , "timer"    // " group"
 };

// special function strings for subroutines

cchar *spfstrs[] =

{ "uuyflu" , "usyflu" , "suyflu", "ssyflu" ,
  "uuwflu" , "uswflu" , "suwflu", "sswflu" ,
  "uytlu"  , "sytlu"  , "uwtlu",  "swtlu"
};

// string options for SETOPTS and CLROPTS


cchar *optstrs[] =

{ //"default"
   "sceprt", "tabnames"  , "funcnames", "ssubnames" ,
  "labelnames" ,"manual", "intrnames" , "subnames" , "signatures",
  "acomments",  "sympresets" ,"8065"  , "compact"   ,"extend" };


// will probably need shift and mask as well.....
//NB . CANNOT have a '/0' for dummy entries, they MATCH !!

cchar * mathfs [] = {"\5\0", "+", "-", "*", "/", "enc", "pwr", "root", "sqrt", "volts", "bound" };

//  n.B. cube root = pwr (= pow) 1/3;   scalers used a lot, allow for a percentage setup ???

cchar *clcstr[] = {"\5\0", "address","integer","float" };  //hex, bin ??

uint clctp[] = {0,DP_ADD,DP_DEC,DP_FLT,0};

// ******************* indexes, size, min math in chars

int     ssizes[6]   = {0, NC(cmds), NC(spfstrs), NC(optstrs), NC(mathfs), NC(clcstr)};
cchar **sindex[6]   = {0,cmds,spfstrs,optstrs, mathfs, clcstr};
//uint    minmatch[6] = {0, 3,    3,    3,         1,      3  };

#define  LUCMDS    1
#define  LUSPF     2
#define  LUOPTS    3
#define  LUMTHF    4           //may as well do a special for this one instead of minmtah
#define  LUCLC     5




// add param options ??

uint mopts [] = {
   // OPTDEFLT  ,     // default  - only one with multiple bits
  1       ,     // sceprt  print source code
  2       ,     // auto table names
  0x200   ,     // auto func names
  4       ,     // auto special func subnames
  0x10    ,     // auto labelnames
  0x20    ,     // full manual operation
  0x40    ,     // auto interrupt handler names
  0x80    ,     // auto subroutine names
  0x100   ,     // signatures/fingerprint
  0x400   ,     // do autocomments
  0x800   ,     // preset symbols
  0x8     ,     // set 8065
  0x2000  ,     // set arg compact layout
  0x4000        // set data extend layout
};





const char *htxt[] = {"\0","Error ", "Warning ", "Note "};   //more ?

struct
{
    uint err : 7;
    uint wix : 1;            //do 'with' index block
    const char *txt;
} especs[] = {

{ 0, 0, "\0"},
{ 1, 1, "Duplicate Command"},                           // E_DUPL
{ 1, 0, "Invalid Address"},                             // E_INVA
{ 1, 0, "Banks Don't Match"},                             // E_BKNM
{ 1, 0, "Odd address Invalid"},                         // E_ODD
{ 1, 1, "Command Overlaps"},                            // E_OVLP
{ 1, 1, "Range Overlaps"},                              // E_OVRG
{ 2, 0, "XCODE bans Command"},                          // E_XCOD
{ 2, 1, "New Symname replaces"},           // E_NAMREP

};


/*/error numbers
#define E_DUPL  1           // duplicate
#define E_INVA  2           // invalid address
#define E_INVSA  3           //inv start range ??
#define E_INVEA  4          // invalid address  end
#define E_BKNM   5          // banks no match
#define E_ODDB   6          // odd address (WORD etc)
#define E_OVLP   7          // overlap (addresses or bit fields)
#define E_OVRG   8          // overlapping ranges
#define E_XCOD   9          // XCODE bans
#define E_SYMNAM   10       // syname replaced
#define E_DUPSCAN  11       // dupl and orig not scanned
//#define MAX_CHERR  11     // max embedded chain error
*/






/*****        Auto SFR names.      *******
  fend is size,  + 0x20 (32) for signed, +0x40 (64) for write op,  giving -
 * fixed names for special regs as Ford Handbook
 * 8061 set followed by 8065 set
 *
 * Params in order -
 * start bit num (0-7)
 * end   bit num (0-31) +0x20 (32) for sign, +0x40 (64) for write
 * address
 * no overlap (for SFRs
 * name

*/

 // 8061 SFR list
DFSYM d61syms [] = {

 { 6,    6, 0x2, 0, "CPU_OK"        },     // 2->6 are byte only
 { 0,    7, 0x2, 0, "LSO_Port"      },
 { 0,    7, 0x3, 0, "LIO_Port"      },
 { 0,    7, 0x4, 0, "AD_Result_Lo"  },     //read
 { 0, 0x47, 0x4, 0, "AD_Cmd"        },     // Write

 //    {0, 0, 3, 0x4, "AD_Channel"      },
    // {0, 32, 32, 0x4, "AD_Low"      },
    // {1, 0, 3, 0x4, "AD_Cmd"       },          // Write symbols from here test with 4 bits
    // {0, 6, 15, 0x4, "AD_Result"      },       // TEST !! crosses bytes

 { 0,    7, 0x5, 0, "AD_result_Hi"  },     // read
 { 0, 0x47, 0x5, 0, "WDG_Timer"     },     // write
 { 0,  0xf, 0x6, 0, "IO_Timer"      },     // word only, 7  not valid by itself
 { 0,    7, 0x8, 0, "INT_Mask"      },
 { 0,    7, 0x9, 0, "INT_Pend"      },

 { 0,    0, 0xa, 0, "HSO_OVF"       },
 { 1,    1, 0xa, 0, "HSI_OVF"       },
 { 2,    2, 0xa, 0, "HSI_Ready"     },
 { 3,    3, 0xa, 0, "AD_Ready"      },
 { 4,    4, 0xa, 0, "Int_Servicing" },
 { 5,    5, 0xa, 0, "Int_Priority"  },
 { 0,    7, 0xa, 0, "IO_Status"     },

 { 0,    7, 0xb, 0, "HSI_Sample"    },
 { 0,    7, 0xc, 0, "HSI_Mask"      },
 { 0,    7, 0xd, 0, "HSI_Data"      },     // read
 { 0, 0x47, 0xd, 0, "HSO_Cmd"       },     // write
 { 0,  0xf, 0xe, 0, "HSI_Time"      },     // word only, 0xf not valid by itself
 { 0,  0xf, 0x10,0, "StackPtr"     }

};


//8065 SFR list

//NB we meed some kind of marker here to stop runover into next byte for certain SFR registers.

// 7 F 11 13 15 17 19 1D


DFSYM d65syms [] = {

 { 6,    6, 0x2, 0, "CPU_OK"              },
 { 0,    7, 0x2, 0, "LSO_Port"            },
 { 0,    7, 0x3, 0, "LIO_Port"            },
 { 0,    7, 0x4, 0, "AD_Imm_Result_Lo"    },      // as byte
 { 0,  0xf, 0x4, 0, "AD_Imm_Result"       },      // as word
 { 0, 0x47, 0x4, 0, "AD_Cmd"              },      // Write
 { 0,  7,   0x5, 0, "AD_Imm_Result_High"  },
 { 0, 0x47, 0x5, 0, "WDG_Timer"           },      // write

 { 0,    7, 0x6, 0, "IO_Timer_Lo"         },      // as byte
 { 0,  0xf, 0x6, 1, "IO_Timer"            },      // as word, no olap
 { 0,    7, 0x7, 0, "AD_Timed_Result_Lo"  },      // read
 { 0, 0x47, 0x7, 0, "AD_Timed_Cmd"        },      // write

 { 0,    7, 0x8, 0, "INT_Mask"            },
 { 0,    7, 0x9, 0, "INT_Pend"            },

 { 0,    0, 0xa, 0, "HSO_OVF"             },
 { 1,    1, 0xa, 0, "HSI_OVF"             },
 { 2,    2, 0xa, 0, "HSI_Ready"    },
 { 3,    3, 0xa, 0, "AD_Imm_Ready"     },
 { 4,    4, 0xa, 0, "MEM_Expand"    },
 { 5,    5, 0xa, 0, "HSO_Read_OVF"  },
 { 5,    6, 0xa, 0, "HSO_Pcnt_OVF"  },
 { 6,    6, 0xa, 0, "Timed_AD_Ready"},
 { 7,    7, 0xa, 0, "HSO_Port_OVF"  },

 { 0,    7, 0xa, 0, "IO_Status"     },
 { 0,    7, 0xb, 0, "HSI_Sample"    },
 { 0, 0x47, 0xb, 0, "IDDQ_Test Mode"    },       //write
 { 0,    7, 0xc, 0, "HSI_Trans_Mask"  },
 { 0,    7, 0xd, 0, "HSI_Data"  },
 { 0, 0x47, 0xd, 0, "HSO_Cmd"      },

 { 0,  0xf, 0xe,  1, "HSI_Time_Hold"  },
 { 0,  0x7, 0xf, 0, "AD_Timed_Result_Hi" },

 { 0,  0xf, 0x10, 1, "HSO_Intpend1"  },
 { 0,    7, 0x11, 0, "BANK_Select"  },         //mem expand only

 { 0,    3, 0x11, 0, "Data_Bank"  },// mem expand only
 { 4,    7, 0x11, 0, "Stack_Bank"  },

 { 0,  0xf, 0x12, 1, "HSO_IntMask1" },
 { 0,    7, 0x13, 0, "IO_Timer_Hi"  },
 { 0,  0xf, 0x14, 1, "HSO_IntPend2" },
 { 0,    7, 0x15, 0, "LSSI_A"       },       // read
 { 0, 0x47, 0x15, 0, "LSSO_A"       },       // write
 { 0,  0xf, 0x16, 1, "HSO_IntMask2" },
 { 0,    7, 0x17, 0, "LSSI_B"       },
 { 0, 0x47, 0x17, 0, "LSSO_B"       },       // write
 { 0,  0xf, 0x18, 1, "HSO_State"    },
 { 0,    7, 0x19, 0, "LSSI_C"       },
 { 0, 0x47, 0x19, 0, "LSSO_C"       },      // write
 { 0,    7, 0x1a, 0, "HSI_Mode"     },
 { 0,    7, 0x1b, 0, "HSO_UsedCnt"  },
 { 0,  0xf, 0x1c, 1, "HSO_TimeCnt"  },
 { 0,    7, 0x1d, 0, "LSSI_D"       },
 { 0, 0x47, 0x1d, 0, "LSSO_D"       },       // write
 { 0,    7, 0x1e, 0, "HSO_LastSlot" },
 { 0,    7, 0x1f, 0, "HSO_SlotSel"  },

 { 0,  0xf,  0x20, 0, "StackPtr"     },
 { 0,  0xf,  0x22, 0, "AltStackPtr"    }

};


//mem_expand syms...



 /* interrupt subroutine names array, for auto naming
  * "HSI_" and default name is output as prefix in code
  * switched on/off with Opt
  * params -
  * start, end   address range of interrupt name (+ 0x2010)
  * CPU          0 is both (65 and 61)  1 = 8065 only
  * num          1 = name has address appended (8065 only)
  * name
  */


INAMES inames[] = {
{ 0,   0, 0, 0, "HSO_2" },            //  8061 ints - 2010- 201f
{ 1,   1, 0, 0, "Timer_OVF" },
{ 2,   2, 0, 0, "AD_Rdy" },
{ 3,   3, 0, 0, "HSI_Data" },
{ 4,   4, 0, 0, "External" },
{ 5,   5, 0, 0, "HSO_1" },
{ 6,   6, 0, 0, "HSI_1" },
{ 7,   7, 0, 0, "HSI_0" },            // end of 8061 list


{ 0,  15, 1, 1, "HSO_" },            // 8065 ints - 2010-201f (plus int num)
{ 16, 16, 1, 0, "HSI_FIFO" },
{ 17, 17, 1, 0, "External" },
{ 18, 18, 1, 0, "HSI_0" },
{ 19, 19, 1, 0, "HSI_Data" },
{ 20, 20, 1, 0, "HSI_1" },
{ 21, 21, 1, 0, "AD_Imm_Rdy" },
{ 22, 22, 1, 0, "AD_Timed_Rdy" },
{ 23, 23, 1, 0, "ATimer_OVF" },
{ 24, 24, 1, 0, "AD_Timed_Start" },
{ 25, 25, 1, 0, "ATimer_reset" },
{ 26, 29, 1, 1, "Counter_" },          // plus num
{ 30, 39, 1, 1, "Software_" }          // plus num   2040-205f    end 8065
};


// math functions.  5 onwards are unary and expected to have an open bracket. but may have 2 pars ?


// functions one set per calctype

void xplus  (MHLD *);
void xminus (MHLD *);
void xmult  (MHLD *);   //do these make sense for address calcs ?? should they be shifts ??
void xdiv   (MHLD *);
void xdmy   (MHLD *);
void xenc   (MHLD *);

void xpwr   (MHLD *);
void xroot  (MHLD *);
void xsqrt  (MHLD *);
void xoff   (MHLD *);
void xvolts (MHLD *);
void xbnd   (MHLD *);





// params = num pars, func has brackets, (default) calctype, nextnamenum, calc subr

// a force calc mode ???   float ?? single letter for some funcs ?? (E,D,V,O ???)

// default param types too ?????      useful for system additions........


MXF mathc [] = {

{ 0,  0, 0, 0     },             // dummy
{ 1,  0, DP_FLT, 1, xplus  },    // integer
{ 1,  0, DP_FLT, 1, xminus },
{ 1,  0, DP_FLT, 1, xmult  },
{ 1,  0, DP_FLT, 1, xdiv   },
{ 3,  1, DP_ADD, 1, xenc   },      // enc (x,n,b) address,type,base reg   may not need 3 ...
{ 2,  1, DP_FLT, 1, xpwr   },      // pwr(x,n)   float
{ 1,  1, DP_FLT, 1, xroot   },      // root
{ 1,  1, DP_FLT, 1, xsqrt   },      // sqrt

{ 1,  1, DP_FLT, 1, xvolts   },      // a to d (volts)
{ 1,  1, DP_FLT, 1, xbnd   }       // bnd) ???  bound(x) for funcs (=x/128) or whatever
     // { 1,  1, 0, xdmy }  last entry

};


CPS  cmnd;                   // user command holder variables for parse and processing

uint cmdopts = 0;          // command global options


// ---------------------------------------------------------------------------------------------




uint get_cmdopt(uint x)
{
    return cmdopts & x;
   // if (cmdopts & x) return (cmdopts & x);
   // return 0;
}



//get(notcmdopt ?)



void set_cmdopt(uint x)
{
    if (!x) cmdopts = 0;
    else  cmdopts |= x;
}






//command definition

DIRS *get_cmd_def(uint cmd)
{
 if (cmd >= NC(dirs)) return NULL;

 return dirs + cmd;
}


cchar *get_cmd_str(uint cmd)
{
 if (cmd >= NC(cmds)) return NULL;

 return cmds[cmd];
}


cchar *get_spfstr (uint ix)
{
 if (ix >= NC(spfstrs)) return NULL;
 return spfstrs[ix];
}


uint get_cmdoptstr(uint flgs, uint ix, cchar **s)
{ // have to modify flgs in here, as 'default' overlaps with others.

   *s = NULL;
   if (ix >= NC(mopts)) return 0;

   if ((flgs & mopts[ix]) == mopts[ix])
     {
       *s = optstrs[ix];
       flgs &= ~mopts[ix];   // clear bit(s)
      }
   return flgs;
}

 cchar *get_mathfname(uint ix)
{
    // add bound checks...
if (ix < NC(mathfs))  return mathfs[ix];
return NULL;
}


MXF * get_mathfunc(uint ix)
{
    if (ix < NC(mathc)) return mathc + ix;
    return NULL;
}

uint get_defcalctype(uint ix)
{
  if (ix < NC(mathc)) return mathc[ix].defctype;
    return 0;
}





/*******************command stuff ***************************************************/



int do_error(CPS *c, uint flags, uint err, const char *fmt, ...)
  {
    // cmd string in flbuf....
    // use vfprintf directly as it does not work
    // if called via wnprt

   va_list args;
   char *t;
   FILE *f;

   if (!err) return 0;

   f = get_file_handle(2);

   if (!c->firsterr)
    {
      wnprt (1,"## %s",flbuf);     // print cmd string once
      c->firsterr = 1;
     }

   wnprt(0,"   ## %s",htxt[err]);    // print error header

  if (c->cmpos < c->cmend)
     {
      t = c->cmpos-6;
      if (t < flbuf) t = flbuf;
      wnprt (0,"at \"%.6s\"",t);       // print where
     }

   wnprt(0," - ");

   if (fmt)
    {
     va_start (args, fmt);
     vfprintf (f, fmt, args);   // print error
     va_end (args);
    }

if (flags)
{
 if (flags == 1) wnprt(0," with - ");

}
else  fputc('\n', f);

   if (err == 1)  c->error = err;
   return err;
  }



int do_ch_error(CPS *c,uint ch)
{
  uint err;
  CHAIN *x;

  x = get_chain(ch);

  err = get_lasterr(ch);


  err = do_error(c, especs[err].wix, especs[err].err, especs[err].txt);

  if (especs[err].wix) x->chprt(x, 0, x->lastix, wnprt);

// void pl_spf (CHAIN *c, uint ix, uint (*prt) (uint,cchar *, ...))

return err;


}










int bktchar(CPS *c, cchar *t)
{
 if (*t == '(' || *t == ')' || *t == '[' || *t == ']' ) return 1;
 if (t > c->cmend) return 1;
 return 0;
}


int ismypunc(cchar *c)
{
  if (*c == ' ')  return 1;
  if (*c == ',')  return 1;
  if (*c == '\t') return 1;
  return 0;
}

int readpunc(CPS *c)
 {
  // read punctuation between commands and numbers etc.
  // keeps track of position (for errors)

    while (c->cmpos < c->cmend)
     {
       if (ismypunc(c->cmpos)) c->cmpos++;
       else break;
     }

  if (c->cmpos >= c->cmend) return 0;            // hit end of line

  return 1;
 }








int get_optchar(CPS *c, cchar *lt)
{
   // check option letters against supplied array,
   // count brackets up/down as 'level' delimiters

   char t;
   int ans;

   readpunc(c);

   if (c->cmpos > c->cmend) return -1;      //end of line

   ans = *c->cmpos;

   switch(ans)
    {
      case '(' :
         c->rbcnt++;    //track round bracket counts....
         break;

      case ')' :
         c->rbcnt--;    //track round bracket counts....
         break;

      case '[' :
         c->sqcnt++;    //track square bracket counts....
         break;

      case ']' :
         c->sqcnt--;    //track square bracket counts....
         break;


      default:                // check against supplied list of chars
        if (!lt) return 0;    // zero opt list (invalid)
        ans = 0;

        t = toupper(*c->cmpos);     // where we are in cmd string

        while (*lt)
         {
          if (t == *lt)
           {
            ans = *lt;
            break;
           }
          lt++;
         }
         break;
    } // end switch

  if (ans) c->cmpos++;            // and skip the char in cmd string
  return ans;
}


int chkstr(CPS *c,cchar *tst)
{
 char *t;

 readpunc(c);
 t = c->cmpos;

 while (*tst)
  {
   if (*t != *tst) break;
   t++;
   tst++;
   while (ismypunc(t))  t++;     // skip any punc
  }

 if (*tst == '\0')
   {  // success - reached end of supplied string
     c->cmpos = t;
     return 1;
   }
 return 0;
}




int gethex(char *x, uint *n, int *rlen)
{
    /* separate hex reader, used for comments and commands
    * max 0xfffff, and flag rlen with extra items for decode of addresses
    * defines are
    HXRL      read length (max 15, bottom 4 bits)
    HXLZ      leading zero read (for bank 0 detection)
    HXNG      negative '-' before number (for offsets in hex)
*/

  if (sscanf(x, "%5x%n", n, rlen) > 0)
       {
        if (*x == 0) (*rlen) |= HXLZ;      // leading zero(es)
        if (*x == '-') (*rlen) |= HXNG;    // negative
        return 1;
       }
 return 0;
}


void clear_pars(CPS *c, int ix, int limit)
{

  limit += ix;

  if (limit > 8) limit = 8;

  while (ix < limit)
   {
     c->p[ix]  = 0;
     c->pf[ix] = 0;
     ix++;
   }

}

int getpx(CPS *c, int ix, int limit)
 {
  // multiple hex params reader
  // ix is where to start in p params array,
  // but can also be ptr to types and flags...
  // for max of 8-ix parameters
  // answer is params read OK

  int ans, rlen;
  uint n;

  clear_pars(c,ix,limit);

  ans = 0;

  limit += ix;

  if (limit > 8) limit = 8;

  while (ix < limit)
   {
     readpunc(c);
     if (!gethex(c->cmpos,&n, &rlen)) break;
     c->p[ix] = n;
     c->pf[ix] = rlen;
     c->cmpos += (rlen & HXRL);
     ans++;
     ix++;
   }

// zero any remaining slots
   while (ix < limit)
    {
      c->p[ix] = 0;
      c->pf[ix] = 0;
      ix++;
    }

  return ans;
 }



int getpd(CPS *c, int ix, int limit)
 {
  // multiple decimal reader
  // ix is where to start in p params array, 2 max decimal pars
  // answer is params read - only ever 1 param ??
   int ans, rlen;

  clear_pars(c,ix,limit);
  ans = 0;

 limit += ix;

  if (limit > 8) limit = 8;

  while (ix < limit)
   {
    if (sscanf(c->cmpos, "%d%n", c->p+ix, &rlen) <= 0) break;
    c->cmpos += rlen;
    c->pf[ix] = rlen;
    ans++;
    readpunc(c);
    ix++;
   }

// zero any remaining slots
   while (ix < limit)
    {
      c->p[ix] = 0;
      c->pf[ix] = 0;
      ix++;
    }

  return ans;
 }



uint read_str(CPS *c)
{
  // get string - any std chars
   uint chars;
   cchar *t;

   chars = 0;
   t = c->cmpos;

   while (t <= c->cmend)
    {
     if (ismypunc(t)) break;
     if (bktchar(c,t) ) break;
     cnm[chars++] = *t++;      // copy char
    }
   cnm[chars] = 0;             // terminate string

   return chars;

}


int match_str(CPS *c, cchar *tst, uint tstlen, uint strix)
{
  // 3 char minimum, max size is lookup string size

  int j, size;
  uint matchlen;
  cchar *t, **str;

  str    = sindex[strix];
  size   = ssizes[strix];

  for (j = 0; j < size; j++)
    {
     t = str[j];                                             // map to test array
     matchlen = strlen(t);                                   // length of command

     if (matchlen < 3)
       { // short match specified, string can be longer than preset match (math funcs)
         if (!strncasecmp (t, tst, matchlen) )
           {
             c->cmpos += matchlen;    // move along by match
             break;
           }
       }

     if (tstlen >= 3 && tstlen <= matchlen)
       {  // default 3 minimum, string cannot be longer than preset match
         if (!strncasecmp (t, tst, tstlen) )
           {
             c->cmpos += tstlen;      // move along by match
             break;
          }
       }
    }

    if (j >= size) return -1;       // no match

    return j;
}


int get_str(CPS *c, uint strix)
 {
   // get string and compare to string array indexed by 'strix'

   int ans, chars;

   readpunc(c);

   chars = read_str(c);

   if (!strix)
    {       // no check array, any string is valid
      if (!chars) return -1;
      ans = chars;
      c->cmpos += chars;
      return ans;
    }

   ans = match_str(c, cnm, chars, strix);         //s, size);             // skip if string matched

   return ans;
 }




uint set_code (CPS *c)
{


    //check olap with Xcode.............


  add_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);   // by cmd
  return do_ch_error(c,CHCMD);
}

uint set_scan (CPS *c)
{
  add_scan (CHSCAN,c->p[0], 0, J_STAT|C_USR,0);

 return do_ch_error(c,CHSCAN);

}


uint set_xcode (CPS *c)
{
    // xcode commands
  add_aux_cmd(c->p[0],c->p[1],c->fcom|C_USR,0);
  return do_ch_error(c,CHAUX);
}


uint set_rbas (CPS *c)
{
 RBT *x;

//addresses checked already in parse cmd

  if (c->npars < 3)
   {
    c->p[2] = 0;         // default address range to ALL (zeroes) otherwise done by cmd
    c->p[3] = 0xfffff;   // max possible address
   }

 x = add_rbase(c->p[0], c->p[1], c->p[2], c->p[3]);
 if (x) x->usrcmd = 1;              // by command

 return do_ch_error(c,CHBASE);


}


uint set_dmy (CPS *c)
{      //dummy command setter
return 0;
}


uint set_opts (CPS *c)
{
  //opts bitmask in p[0]
 // int x;
 // if (P8065) x = 1; else x = 0;

  cmdopts |= c->p[0];

  if (get_numbanks())  cmdopts |= 0x8;

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(0,"SetOpts = ");
    prtcopts(DBGPRT);
    DBGPRT(2,0);
  #endif
return 0;

}

uint clr_opts (CPS *c)
{
  cmdopts &= (~c->p[0]);

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(0,"ClrOpts = ");
    prtcopts(DBGPRT);
    DBGPRT(2,0);
  #endif
return 0;

}



int chk_csize(CPS *c)
 {
  int bsize;
  DIRS *d;

  d = dirs + c->fcom;

// check any extra levels and base sizes

  bsize = c->p[1] - c->p[0];
  bsize += (1 - c->term);    // byte size of cmd entry as specified

  //minumim default size

  if (d->dsize > bsize)
    {              // cmd is less than minimum size. Extend it
     c->p[1] = c->p[0] + d->dsize - 1;
     c->p[1] += c->term;
     do_error(c, 0,2, "Strat->End too small, increased to %x", c->p[1]);
    }


  if (c->adtsize)
    {       // if additional data
      int row, newbsize;

      row = bsize/c->adtsize;           // number of whole rows
      newbsize  = row * c->adtsize;     // number of bytes for whole rows

      if (newbsize != bsize)
        {
            //not whole rows.
          c->p[1] = c->p[0] + newbsize -1;
          c->p[1] += c->term;
          do_error(c, 2,2, "End inconsistent with size, reset to ");
          prtaddr(c->p[1],0,wnprt);
       }
    }
   return 0;
 }



void upd_cadt(CPS *c, void *newfid)
{
 // copy adt blocks from cmd chain to std chain with new fid

  ADT *a;
  uint fix, ix;

  fix = 0;

  if (c->fcom == C_ARGS)  fix = 1;
  if (c->fcom == C_SUBR)  fix = 1;
  if (c->fcom == C_TABLE) fix = 2;
  if (c->fcom == C_FUNC)  fix = 2;


//fid for a itself doesn't change - conneted stuff OK

   a = get_adt(&cmnd,0);    // get first block (dummy fid)

  if (a)
    {
     ix = get_lastix(CHADNL);         // save index of block
     a->fid = newfid;
     chupdate(CHADNL,ix);             // rechain after new FK
    }


  // set pfw zero for args and subr
  // set print radix to decimal for tab and func

  if (!fix) return;

  while ((a = get_adt(newfid,0)))
    {
      if (a->cnt)
       {          // use cnt as null checker
        if (fix == 1) a->pfw = 0;    // clear all pfw
        if (fix == 2 && a->dptype == 0) a->dptype = DP_DEC;  //set decimal if not set
       }
      newfid = a;
    }

     #ifdef XDBGX
    DBGPRT(0,"      with addnl params");
    prt_adt(newfid, 0, DBGPRT);
    DBGPRT(1,0);
  #endif

}




uint set_func (CPS *c)
{
  // for functions.  2 levels only

  int val, fend, startval;
  LBK *blk;
  ADT *a, *b;
  // size row vs. start>end check first

  a = get_adt(&cmnd,0);                //  first item - dummy fid
  b = get_adt(a,0);                    //  get second item

  if (!b)
    {   // add another level if only one specified

     b = add_adt(a,0);
     *b = *a;               // copy all data
     b->fid = a;            // restore fid
    }

  c->adtsize = adn_totsize(&cmnd);

  if (chk_csize(c)) return 1;

  fend = a->fend;                   //size and sign as specified

  // check start value consistent with sign

  val   = g_val(c->p[0], 0, fend);
  startval = get_startval(fend);              // start value from fend SIGN

  if (val != startval)  // try alternate sign....does not change command though
    {
     fend ^= 32;                             // swop sign flag
     startval = get_startval(fend);          // new start value
     if (val != startval) do_error(c, 0,2, "Function Start value invalid");
     else
      {
        do_error(c, 0,2, "First func value indicates wrong sign specified");
     }
    }

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);  // start,end, cmnd by cmd
  if (do_ch_error(c,CHCMD)) return 1;

  upd_cadt(c,vconv(blk->start));
  blk->size = adn_totsize(vconv(blk->start));

  return 0;
}

uint set_tab (CPS *c)
{
   // tables, ONE level only

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);  // start,end, cmnd by cmd
  if (do_ch_error(c,CHCMD)) return 1;

  upd_cadt(c,vconv(blk->start));
  blk->size = adn_totsize(vconv(blk->start));

 if (c->argl) blk->argl = 1;     // default is argl OFF
 return 0;
}



void set_data_vect(LBK *blk)
{
 int ofst, xofst, end, pc;
 int bank;
 ADT *a;
 void *fid;


 // for structs only -loops around and looks for addr refs
 if (get_cmdopt(OPTMANL))  return;
 if (!blk || blk->fcom != C_STCT) return;

 bank = g_bank(blk->start);      // what if bank is NOT same as struct - WRONG!!!  databank ???
 fid = blk;

 for (ofst = blk->start; ofst < blk->end; ofst += blk->size)
  {

   end = ofst + blk->size-1;
   xofst = ofst;
   while ((a = get_adt(fid,0)) && xofst < end)
    {
      if (a->dptype == DP_ADD)    // address
      {
       pc = g_word(xofst);
       pc |= bank;                  // this assumes vect is always in same bank ?
       add_scan (CHSCAN,pc, 0, J_SUB, 0);     // auto adds subr
      }
     xofst += cellsize(a);        // always 2 No........
     fid = a;
    }
  }
}


uint set_stct (CPS *c)
{
   // structures

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);  // start,end, cmnd by cmd
  if (do_ch_error(c,CHCMD)) return 1;

  upd_cadt(c,vconv(blk->start));
  blk->size = adn_totsize(vconv(blk->start));

  blk->term = c->term;                    // terminating byte flag (struct only)
  blk->argl = c->argl;                   // default is arg layout
  blk->cptl = c->cptl;

  set_data_vect(blk);   // for Refs out in structs
  return 0;
}

/*
 *  for move into auto timer above.
 */
 uint set_time(CPS *c)
 {
  LBK *blk;
//  int val,type,bank;
//  int xofst;
 // short b, m, sn;
 // SYM *s;

 // char *z;

  blk = add_cmd (c->p[0],c->p[1],c->fcom,0);

//  set_adnl(cmnd,blk);
 // DBGPRT(1,0);
//  new_sym(2,cmnd->symname,cmnd->start, -1);  // safe for null name
return 0;
 }

/*
  // up to here is same as set_prm...now add names

  if (!blk) return 1;          // command didn't stick

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


uint set_args(CPS *c)
{
  LBK *blk;

 if (chk_csize(c)) return 1;

  blk = add_aux_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);  // by cmd

  if (do_ch_error(c,CHAUX)) return 1;

  upd_cadt(c,vconv(blk->start));
  set_data_vect(blk);

  blk->size = adn_totsize(vconv(blk->start));

  #ifdef XDBGX
  DBGPRT(1,0);
  #endif
return 0;
}


uint set_vect (CPS *c)
{                          // assumes vect subrs in same bank as pointers
  uint  ofst, bank;
  int i;
  LBK *blk;
  ADT *a;
  BANK *b;

 // if (chk_csize(c))   return 1;         // fixed size

  b = get_bkdata(0);

  blk = add_cmd (c->p[0],c->p[1], C_VECT|C_USR,0);   // by cmd

  if (get_cmdopt(OPTMANL)) return 0;
   if (do_ch_error(c,CHCMD)) return 1;

  a = get_adt(&cmnd,0);
  if (a)
   {
           // check addn entry
     bank = a->bank << 16;
     if (!b[a->bank].bok) return do_error(c, 0,1,"Invalid Bank");      //should be verified in parse cmd
     upd_cadt(c,blk);
   }
  else bank = g_bank(c->p[0]);

  for (i = c->p[0]; i < c->p[1]; i += 2)
    {
      ofst = g_word (i);
      ofst |= g_bank(bank);
      add_scan (CHSCAN,ofst, 0, J_SUB,0);      // adds subr

    }
  return 0;
}


uint set_sdata (CPS *c)
{
   // for word, byte, text etc,  triple and long are different
   // no addnl is fine


   // *******  but may need 2 addresses for triple and long !!  *********
   // this needs new format around c->p params..........

  LBK *blk;
 // ADT *a;

 // if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);  // start,end, cmnd by cmd
  if (do_ch_error(c,CHCMD)) return 1;

 // a = get_adnl(CHADNL,&c->spf);  moved to do_adnl_item in command read....

 // if (a && bytes(a->fend) < c->fcom)  a->fend = (c->fcom * 8) -1;     // safety for command sizes
if (blk)
{
  upd_cadt(c,vconv(blk->start));
  blk->size = adn_totsize(vconv(blk->start));
}
  return 0;
}


uint set_ldata (CPS *c)
{
   // for triple and long
   // this uses 2 addresses for the two locations
   // word+byte or word+word


// *** needs more work ****
  LBK *blk;
//  ADT *a;

  //if (chk_csize(c)) return 1;

//how to do this in add_cmd ???

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USR,0);  // start,end, cmnd by cmd
  if (do_ch_error(c,CHCMD)) return 1;

 // a = get_adnl(CHADNL,&c->spf);

 // if (a && bytes(a->fend) < c->fcom)  a->fend = (c->fcom * 8) -1;     // safety for command sizes
if (blk)
{
  upd_cadt(c,vconv(blk->start));
  blk->size = adn_totsize(vconv(blk->start));
}
  return 0;
}


uint set_sym (CPS *c)
{
  int fstart,fend;
 // uint error;
  ADT *a;
  SYM  *s;

  // can only have zero or ONE adt level

  if (c->npars < 3)
   {
    c->p[1] = 0;         // default address range to ALL, otherwise done by cmd
    c->p[2] = 0xfffff;   // max possible address
   }

   // only one adt level allowed

    a = get_adt(&cmnd,0);
    if (a)
     {
      fstart = a->fstart;
      fend   = a->fend;
     }
    else
     {
       // set default size if no bits specified
       // byte for odds, words for evens so overlaps allowed
       fstart = 0;
       if (c->p[0] & 1) fend = 7; else fend = 15;
     }

  //NB.  C_REN must also be set to allow rename

  s = add_sym(c->string, c->p[0], fstart, fend| C_USR, c->p[1], c->p[2]);

 // if (!(c->p[7] & 16)) s->prtbit = 1;           // do bit field printout
 // if (c->p[6])  s->smask = c->p[6];             // not yet.....
 // if (c->symflags & 2)  s->names = 1;           // with flags

  if (c->p[7] & 4)  s->immok = 1;             // immediate

  if ((c->p[0] & 1) && (fend & 0x1f) > 7)
     do_error(c, 0,2, "Implies word operation for odd address");


//instead of do_ch_error.........

  return do_ch_error(c, CHSYM);






  //return 0;
}


uint set_subr (CPS *c)
{

  SUB *xsub;
  SBK *s;
  SPF *f;
  AUTON *an;

  xsub = add_subr(c->p[0]);

  s = add_scan (CHSCAN,c->p[0], 0, J_SUB,0);
  // can do name here, even if a duplicate.............

  if (!xsub || xsub->sys)
    {   // allow redefinition of existing system one
     if (do_ch_error(c,CHSUBR)) return 1;
     }
  else xsub->usrcmd = 1;

  // do any special functions first

//  f = 0;

//  fid = get_last_fid(CHSPF,xsub);


// everything attached to the SCAN BLOCK

  if (c->strix)
     {            // special func is marked
      an = get_autonames();
      an = an + c->strix;
      f = add_spf(vconv(s->start), an->spf, 1);           // add to the SCAN block
      f->fendin  = an->fendin;
      f->fendout = an->fendout;
      f->pars[0].reg = c->adreg;            // register
      f->pars[1].reg = c->szreg;             // cols

      #ifdef XDBGX
        DBGPRT(0, "  with spf = %x ", f->spf);
      #endif
    }  // end special func

   if (c->ansreg)
   {
       f = add_spf(vconv(s->start), 1, 1);
       f->pars[0].reg = c->ansreg;
        #ifdef XDBGX
        DBGPRT(0, "  with ans = %x ", f->pars[0].reg);
      #endif
   }


  upd_cadt(c,vconv(s->start));              // add args to SCAN BLOCK, not - xsub);

  s->adtsize = adn_totsize(vconv(s->start));            //xsub->size = adn_totsize(xsub);
  if (c->cptl) s->adtcptl = 1;         //xsub->cptl = 1;     // default is argl ON
 return 0;
 }


uint val_bank(uint bk)
{
 //for bank number
  BANK *b;

  b = get_bkdata(bk);
  if (b->bok) return 1;
  return 0;
}


uint fix_input_addr_bank(CPS *c, int ix)
 {
     // answers 0 is OK
     // 1 is invalid bank

   uint rlen, bk, addr;

   rlen = c->pf[ix] & HXRL;      // read length
   bk = g_bank  (c->p[ix]);     // 0 - 15 in 0xf0000
   addr = nobank(c->p[ix]);     // address part

   if (valid_reg(addr))
     {
       if (bk) return 1;       // registers don't ever have a bank
       return 0;               // register valid
     }

   if (!get_numbanks())
     {
       if (rlen > 4 && bk != 0x80000 ) return 1;      // only bank 8 allowed
       bk = 0x90000;                          // single bank force 8 (9)
     }
   else
     {          // multibank
       if (bk & 0x60000) return 1;       // invalid bank

       if (rlen < 5)
         {
          if (addr > 0x1fff) return 1;    // 0x2000 on MUST have a bank.
          bk = 0x20000;                   // set bank 1
         }
       else
         {
          bk += 0x10000;                   // add one to valid bank
         }
     }
   c->p[ix] = addr | bk;
   return 0;
 }




uint validate_input_addr(CPS *c, int ix, uint type)
{
  /* validate/fix input addresses according to type
   * 0 none,
   * 1 start address with ROM validation
   * 2 end address (same bank as 1, and can be zero)
   * 3 register
   * 4 start address (0 - 0xfffff)
   * 5 end address   (0 - 0xfffff)
   * 6 range start
   * 7 range end (can cross banks)
   */


  switch(type)

  {
    default:           //do nothing
      break;

      case 2:       // end address within valid ROM

         if (ix)
          {

           if (!c->p[ix]) c->p[ix] = c->p[ix-1];
           else
            {
             if ((c->pf[ix] & HXRL) < 5) c->p[ix] |= g_bank(c->p[ix-1]);
             else if (fix_input_addr_bank(c,ix))    return do_error(c, 0,1, "Invalid bank");

             if (!val_rom_addr(c->p[ix]))       return do_error(c, 0,1,"Invalid address");
            }
         // extra end validation against start

           if (!bankeq(c->p[ix-1], c->p[ix]))   return do_error(c, 0,1, "Banks must match");
           if (c->p[ix] < c->p[ix-1])           return do_error(c, 0,1, "End is less than Start");

           break;
          }
       // else fall through as a type 1

      case 1:       // start address within valid ROM

         if (get_numbanks() < 0)              return do_error(c, 0,1, "No banks defined");    //but what if bank command ???
         if (fix_input_addr_bank(c,ix)) return do_error(c, 0,1, "Invalid bank");
         if (!val_rom_addr(c->p[ix]))   return do_error(c, 0,1, "Invalid address");
         break;



      case 3:       //register

        c->p[ix] = nobank(c->p[ix]);
        if (!valid_reg(c->p[ix]) )      return do_error(c, 0,1, "Invalid Register ");
        break;

     case 5:       // end address (0-0xffff) without val_rom

         if (ix)
          {

           if (!c->p[ix]) c->p[ix] = c->p[ix-1];
           else
            {
             if ((c->pf[ix] & HXRL) < 5) c->p[ix] |= g_bank(c->p[ix-1]);
             else if (fix_input_addr_bank(c, ix))    return do_error(c, 0,1, "Invalid bank");
            }
         // extra end validation against start

           if (!bankeq(c->p[ix-1], c->p[ix]))   return do_error(c, 0,1, "Banks must match");
           if (c->p[ix] < c->p[ix-1])           return do_error(c, 0,1, "End is less than Start");

           break;
          }
       // else fall through as a type 4


   case 7:       // full range end address across banks

         if (ix)
          {

           if (!c->p[ix]) c->p[ix] = c->p[ix-1];
           else
            {
             if ((c->pf[ix] & HXRL) < 5) c->p[ix] |= g_bank(c->p[ix-1]);
             else if (fix_input_addr_bank(c,ix))    return do_error(c, 0,1, "Invalid bank");;
            }

           if (c->p[ix] < c->p[ix-1])           return do_error(c, 0,1, "End is less than Start");

           break;
          }
       // else fall through as a type 6

      case 4:       // start address (0-0xffff) without val_rom
      case 6:       // full range start address (same as 4)

         if (get_numbanks() < 0)              return do_error(c, 0,1, "No banks defined");    //but what if bank command ???
         if (fix_input_addr_bank(c,ix)) return do_error(c, 0,1, "Invalid bank");
         break;



  }
return 0;
}







int chk_cmdaddrs(CPS *c, DIRS* d)
 {
   // check and fixup any address issues (bank etc)
   // validate according to type (previous subr)

  int ans;
  int i;

  if (c->npars < d->minpars) return do_error(c, 0,1,"Require at least %d parameters",d->minpars);
  if (c->npars > d->maxpars) return do_error(c, 0,1,"Require %d parameters or fewer",d->maxpars);

  for (i = 0; i < d->maxpars; i++)
   {
      ans = validate_input_addr(c,i, d->ptype[i]);
      if (ans) break;
   }

    // NB. do error returns 1

 return 0;
 }



uint set_cpsw (CPS *c)
{
    // psw setter p[0] checked, but not p[1]

  add_psw(c->p[0],c->p[1]);
  return do_ch_error(c,CHPSW);

}



uint get_pfw(ADT *a)
{   // default print field width, based on field size.
// may need more for different dptypes
 uint ans, sz, sign;

 sign = (a->fend & 32);
 sz = (a->fend & 31) - a->fstart;

 if (sz < 4) ans = 1;
 else
 if (sz < 7) ans = 2;
 else
 if (sz < 10) ans = 3;
 else
 if (sz < 14) ans = 4;
 else
 if (sz < 17) ans = 5;
 else
 if (sz < 20) ans = 6;
 else
 if (sz < 24) ans = 7;
 else
 if (sz < 27) ans = 8;
 else
 if (sz < 30) ans = 9;
 else  ans = 10;

 if (sign) ans += 1;      //for -ve sign
// if (a->pfd) ans+= (a->pfd+1);           // floating point

return ans;

}





 void set_pfwdef(ADT *a)
{   // default print field width

if (a->pfw) return;                   // never modify a user set pfw ???

a->pfw = get_pfw(a);
}


// next few for math strings





MATHN *add_encode(uint chn, void *fid,uint type, uint reg)
{
  // add encode type to adt cell.
  // defined as func as used in core by argument code

  //add a dumy name ???   may be easier....

  MATHX *x;
  MATHN *n;

  n = add_sys_mname(fid,20);
  n->mcalctype = DP_ADD;
  n->sys = 1;

  add_link(chn,fid,CHMATHN,n,1);                // mterm -> adt (fwds)

  x = add_mterm (n,0);
  if (!x) return NULL;

  x->calctype = DP_ADD;
  x->func = 5;
  x->fbkt = 1;
  x->npars = 3;


  x->fpar[0].dptype = DP_REF;
  x->fpar[1].ival = type;    //enc type
  x->fpar[1].dptype = DP_DEC;
  x->fpar[2].ival = reg;    //register address
  x->fpar[2].dptype = DP_ADD;



  return n;
}


MATHX *add_mcalc(ADT *a, uint func, uint par1)
{
    //to add single math term, e.g. divide
    //this for D (offsets) must modify for V

  MATHX *x;
  MATHN *n;

  n = add_mname(a,0);

  n->mcalctype = DP_DEC;
  add_link(CHADNL,a,CHMATHN,n,1);                // mterm -> adt (fwds)
  x = add_mterm (n,0);

  if (!x)  return NULL;

  // first term. add the 'x' reference


  x->func = 1;
  x->npars = 1;
  x->fpar[0].dptype = DP_REF;
  x->calctype = DP_DEC;
  x->first = 1;
 // x->noname = 1;
 // add_link(CHADNL,a,CHMATHX,x,1);               // mterm -> adt (fwds)

  // 2nd term

  x = add_mterm (x,0);              // 2nd -> 1st term

  x->calctype = DP_DEC;           //default
  x->func = func;
 // x->calc = &xplus;
  x->npars = 1;
  x->fpar[0].ival = par1;    //offset value
  x->fpar[0].dptype = DP_DEC;

  return x;
 }











int get_float(CPS *c, float *num)
 {
  // ix is where to start in p params array, 2 max decimal pars
  // answer is params read - only ever 1 param at a time

 // isnan()

   int rlen, ans;

   *num = NAN;

  //now read number

   if (c->cmpos > c->cmend) return -1;

   ans = sscanf(c->cmpos,"%f%n", num, &rlen);
   if (ans > 0)
     {
       c->cmpos += rlen;
       readpunc(c);
     }

   return ans;

 }





uint do_math_term(CPS *c, void *fid, int pix)
{
  int opt, cmdread, bktlev;
  char *lastpos;
  float fx;
  MATHX *a;
  MNUM *n;
  MXF *mr;       // reference for func details

/* fid starts out as name reference, or ADT if no name

 *
 * each term is func (par1, par2, par3).  Read func (name) OR float OR ref each time,
 * and if it's first term in this level and a number first, add a dummy '+' func to start.
 *
 * each parameter has a type, int/float etc, but also includes SUB and a REF so that a
 * subterm can be hung anywhere there is a parameter
 *
 * to do.... SET flag somewhere in C to mark as single term only for short formula - and terminate on end ')'
 */

  readpunc(c);

  a = 0;
  mr = 0;

  bktlev = c->rbcnt;              //round bracket count

  while (c->cmpos < c->cmend)
    {

     cmdread = 0;              // read type
     readpunc(c);

     if (c->cmpos > c->cmend) break;         //  end of cmd line
     lastpos = c->cmpos;                     //  to check something has been read

    // attempt to read a number, reference, function or special char (e.g. open bracket)

    // should always be a func first, then pars/numbers expected.
    // special if first term is number, add an implied '+' to make func, par -> func, par(s) etc.

    //TO DO - if func is + or - and func already set, assume attached to number.....
    // also no space after the -/+

    // recursive for bracketed subterms (treated as subs of params)

     c->mfunc = get_str(c, LUMTHF);

     if (c->mfunc > 0)
       {
         cmdread = 4;               // command read
         a = add_mterm(fid,pix);    // this is new function - always add a new term
         fid = a;                   // new term is now fid
         pix = 0;                   // and param is zero
         c->mcnt++;
         a->func = c->mfunc;
         mr = mathc + c->mfunc;       // func details
    //     a->calc = mr->calc;
         a->fbkt = mr->bkt;          // and mark in ADT
         a->calctype = c->mcalctype;  //master calc type

         if (mr->bkt)        // skipobkt = 1;   // set marker to skip one bracket.....
           {    // skip one bracket (for function) mark close
            opt = get_optchar(c,0);      //read next
            if (opt != '(' ) return do_error(c, 0,1," '(' expected");
           }
       }
       else
       {
          c->mfunc = 0;
          if (!a)
            {
             // this is first term of a new level and number read in, not func
             // so create a dummy '+' function
             a = add_mterm(fid,pix);
             fid = a;
             pix = 0;
             c->mcnt++;
             a->func = 1;
             mr = mathc + 1;
          //   a->calc = mr->calc;
             a->first = 1;                 // flag as first to stop print of '+'
             a->calctype = c->mcalctype;       //mr->defctype;   //not always !!
            }
       }


     opt = get_optchar(c,0);

     if (opt == '(') // open bkt flag
       {       // nested bkt '('
         n = a->fpar + a->npars;
         n->dptype = DP_SUB;                 //subterm
         a->npars++;
         do_math_term (c,fid, a->npars);     //recursive for subterm(s)
         continue;                           // this seems to work....??
       }

   // return if all bkts (for this term level) closed
     if (opt == ')' && c->rbcnt < bktlev) return 0;

     readpunc(c);

     if (toupper(*c->cmpos) == 'X')
       {  // a reference - links to relevant ADT entry, including a subfield....
   //      opt = get_ref(c);
      //   if (opt)
          {
            c->p[5] = 1;           //opt;         // ref num may be redundant.
            cmdread = 2;            //reference
      //      n->dptype = DPREF;    // see BELOW....
            c->cmpos++;  //skip X (no numbers for now)
          }
       }
     else
   //  if (isdigit(*c->cmpos))      //can just call get_float ?? Yes.
       {

         //calctype must define whether float int or adress - c->mastercalc or mr->defctype (float or add) ??
        opt = 0;

        if (mr && c->mcalctype <= DP_ADD)         // mr->defctype == DPADD)
        {
          opt = getpx(c,5,1);
          if (opt > 0) cmdread = 3;     //hex (address) OK
        }

        if (!opt)          //float is default
        {
          opt = get_float(c,&fx);    //  term must begin null func with number...........THEN func/opt and number

          if (opt > 0)
           {
             if (!isnormal(fx)) {do_error(c, 0,1, "Invalid floating point number"); return 0;}
             cmdread = 1;            // float OK
           }
        }



       }


     if (cmdread > 0 && cmdread < 4)
        { // number or ref read in

          n = a->fpar + a->npars;

          if (cmdread == 1) {n->fval = fx; n->dptype = DP_FLT;}     //float - no not always............

          if (cmdread == 2) n->dptype = DP_REF;              //n->refno = c->p[0]; if numbers with X
          if (cmdread == 3) {n->ival = c->p[5]; n->dptype = DP_HEX;}     //float - no not always............

          a->npars ++;
        }


     if (a) mr = mathc + a->func;      // func lookup details - set here in case of auto func



     if (c->error) break;
     if (lastpos == c->cmpos) do_error(c, 0,1, "Parse fail");    // catchall safety check - nothing has been read

    }  // end while chars to read


//then copy local term to a valid structure............

  return 0;
}

int read_calctype(CPS *c)
{

  c->ansreg = get_str(c, LUCLC);

  // do check OUTSIDE, as = 'name' doesn't have a calctype.

   readpunc(c);

   if (c->cmpos > c->cmend) return do_error(c, 0,1, "Line incomplete");

return 0;
}





int do_math_str (void *fid, CPS *c)
{
   MATHN *n;
 //  MATHX *x;
   uint ix;
  // int ans;

  // name is in c->string if fid = cmnd.
  // for both named and unnamed, should be at first '('
  // named via 'calc' command, unnamed via embedded '=' in an ADT

  // need to consider SUFFIX string as well....e.g. volts, RPM, degrees, percent

   readpunc(c);

   if (c->cmpos > c->cmend) return do_error(c, 0,1, "Line incomplete");

   // calctype,   = float ( ), = integer ( ), = address( ) already in c->ansreg

   if (fid == &cmnd)
      {
        n = add_mname(fid, c->string);
        n->mcalctype = clctp[c->ansreg];
        c->mcalctype = n->mcalctype;            //master calc set this for following terms
        fid = n;
      }

 readpunc(c);

 ix = get_optchar(c,0);

 if (ix != '(') do_error(c, 0,1, "Calc must begin with '('");

 do_math_term(c,fid,0);

return 0;

}

/*
if (fid != &cmnd) return 0;   //this doesn't do a link to name .....


 //relink to new name

 x = get_mterm(fid,0);
 ix = get_lastix(CHMATHX);    // keep index

if (x) {
 x->fid = n;                  // relink first math term to n
 chupdate(CHMATHX,ix);        // and rechain in correct place
}


// AND suffix string ??

return 0;
} */





int do_adnl_item (CPS *c, void **fid, uint *ix)
{
   // options by command letter in one 'item'

  ADT *a;
  MATHX *x;
  MATHN *n;
//  FKL *z;                 // debug

  int ans, rlen, opt;

  void *subfid;     // for subcommands, need a LOCAL fid for next level
  uint subix;

  subix = 1;               // next level down if get a '['

   // first open bracket is already read in, so start new item

  if (c->adtcnt > 32) return do_error(c, 0,1, "Too Many data items");
  c->adtcnt++;                  //number of terms in main level
  a = add_adt(*fid, *ix);     // ix allows for subfields

  *ix = 0;              //  only first term will ever have ix > 0, and that is a '1' at this time
  *fid = a;

  subfid = a;

 // if (a->fend < dirs[c->fcom].dfend)  a->fend = dirs[c->fcom].dfend;     // check preset for some commands is this right ????

  // now scan down cmd string - use sqcnt to sort out subs....(subix is no good if it's reset.........)

  while (c->cmpos < c->cmend)
    {
      if (c->error) break;

      if (c->sqcnt > 1)  opt = get_optchar(c, dirs[c->fcom].subopts);
      else     opt = get_optchar(c,dirs[c->fcom].mainopts);


    switch(opt)            //toupper done in get_optchar
     {

        case '[' :         // new data term inside current one....

         do_adnl_item (c,&subfid, &subix);  // nested open - fkey = ADT block 'a' + level via bkts
         if (!c->error) a->sub = 1;         // mark subflags, not used anywhere yet
         break;

      case -1:           //  end of line
      case ']' :         // end of field
         return 0;
         break;

       case '|' :
         //   split printout for large structs, ignore if arg layout set
         if (!c->argl) a->newl = 1;
         break;

      case '=' :

         // formula name or a short calc if in ()

         read_calctype(c);

         if (c->ansreg > 0)
          {   // recognised system calc_type - do ad hoc calc with no name.
            n = add_mname(a,0);         //add zero name
            if (n)
              {
                add_link(CHADNL,a,CHMATHN,n,1);               // mname (dummy) <-> adt
                n->mcalctype = clctp[c->ansreg];
                c->mcalctype = n->mcalctype;
                do_math_str(n,c);
              }
          }
         else
          {    //expect a calc name
            if (rlen > 0)
             {
              c->cmpos += rlen;
              n = get_mname(0,cnm);                             // name only search
              if (n) add_link(CHADNL,a,CHMATHN,n,1);          // mname <-> adt
             }

            if (!n)   return do_error(c, 0,1, "Calc name not found");
           }
         readpunc(c);
         break;


        // 'A':  args ??  one or other is default


       case 'B' :                 // Bit for subfields and syms.
         ans = getpd(c,4,2);
         if (!ans) return do_error(c, 0, 1, "Bit Number Required");
         //but this doesn't work with signs etc...........
         if (c->p[4] < 0 || c->p[4] > 31) return do_error(c, 0, 1, "Invalid Start Bit");
         if (c->p[5] < 0 || c->p[5] > 31) return do_error(c, 0, 1, "Invalid End Bit");

         a->fend &= 0x60;         // keep sign and write
         a->fstart = c->p[4];
         a->fend   |= c->p[5];
         // 'W' for write and 'S' sets a->fend already.

         if (ans < 2)
            {
              if (a->fend & 32) return do_error(c, 0, 1, "Single bit cannot be signed");
              a->fend |= a->fstart;  // single bit
            }





         break;

// C compact ?

       case 'D' :

         // 1 hex value (address offset with bank) deprecated but should still work....
         ans = getpx(c,4,1);
         if (!ans) return do_error(c, 0, 1, "Address Required");

         //  need name + 2 terms, +x (first,noname), + offset
         x = 0;
         n = add_mname(a,0);         //add zero name
         if (n)
            {
             add_link(CHADNL,a,CHMATHN,n,1);               // mterm -> adt (fwds)
             n->mcalctype = DP_ADD;
             x = add_mterm (n,0);
            }

         if (x)
          {
           x->calctype = DP_ADD;
           x->func = 1;
      //     x->calc = &xplus;
           x->npars = 1;
           x->fpar[0].dptype = DP_REF;
           x->first = 1;

           //2nd term

           x = add_mterm (x,0);              // 2nd -> 1st term
    //       x->calctype = DP_ADD;
           x->func = 1;
     //      x->calc = &xplus;
           x->npars = 1;
           x->fpar[0].ival = c->p[4];    //offset value
           x->fpar[0].dptype = DP_HEX;
          }


         set_pfwdef(a);  //word sized for offsets
         break;

       case 'E' :     // ENCODED - always size 2 deprecated
         ans = getpx(c,4,2);
         if (ans != 2 )
            {
              return do_error(c, 0, 1, " 2 values Required");
            }

         n = add_encode(CHADNL, a,c->p[4], c->p[5]);

         if (!n) return do_error(c, 0, 1, " encode calc failed!");
//safety checks....
  a->fend = 15;            // encoded implies unsigned word
  set_pfwdef(a);
  if (!a->cnt) a->cnt   = 1;

         break;

       case 'F' :

         // flags for symbol  (mask ??
         if (c->fcom == C_SYM)
          {             // flags word/byte for SYM.
      //     ans = getpx(6);
        //   if (ans)
      //      {
        //     if (ans != 1 ) return do_error(c, 0, 1, " Value Required");

             c->p[7] |= 8;              // 'F' set
           // }
    //       else cmndp[6] = 0;
         }
        break;



  // G H J

       case 'I' :               // immediate fixup for syms

        if (c->fcom == C_SYM)  c->p[7] |= 4;       //immediate
        break;

       case 'K' :

         // 1 decimal value (bank, 0,1,8,9), but plus 1 for internal
         ans = getpd(c,4,1);
         if (!ans) return do_error(c, 0, 1, "Bank number Required");

        c->p[4]++;
         if (!val_bank(c->p[4])) return do_error(c, 0, 1, "Bank Number Invalid ");
         a->bank  = c->p[4];
         if (!a->cnt) a->cnt = 1;     // but size stays at zero
         break;

       case 'L' :               // long int (32 bit)

         a->fend &= 0x60;        // keep sign and maybe write
         a->fend |= 31;
       //  a->bsize = 4;
         if (!a->cnt) a->cnt = 1;
         set_pfwdef(a);
         break;

 // case 'M':                // mask pair ?, use esize to decide word or byte
 //    cadd->mask = 1;
 //    break;

       case 'N' :

         a->fnam = 1;       // Name lookup in structs
          if (c->fcom == C_SYM) c->p[7] |= 2;
         break;

       case 'O' :                // Cols or count - 1 decimal value

// count may not fit with subfields  [o 5 [b 5 6]] causes 10 items ?? probably not, but DOES work for calcs (e.g. funcs and tabs)

         ans = getpd(c,4,1);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, 0, 2, " Repeat value Missing, 1 assumed");
           }
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c, 0, 1, " Repeat value Invalid (1-32)");

         a->cnt = c->p[4];
         break;

       case 'P' :               // Print fieldwidth (in spaces, not digits) 1 decimal
                                // float handling in 'X'
         ans = getpd(c,4,1);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, 0,2, "Field Width Missing, 1 assumed");
           }
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c, 0, 1, " Field width Invalid  (1-32)");

         a->pfw = c->p[4];

         break;

       case 'R' :               // indirect pointer to subroutine (= vect)

         a->fend &= 0x60;
         a->fend = 15;           // safety, lock to WORD, but not always true for offsets
         a->dptype = DP_ADD;      // address mode

         //and cross check against calcs

     //    a->name  = 1;          // with name by default  NO !!
         if (!a->cnt) a->cnt   = 1;

         set_pfwdef(a);
         break;


       case 'S':              // Signed
          a->fend |= 0x20;
          break;

       case 'T':                 // bit -> Triple
         a->fend &= 0x60;           // keep sign
         a->fend |= 23;
         if (!a->cnt) a->cnt   = 1;
         set_pfwdef(a);
         break;


       case 'U' :               //unsigned (default so optional)
         a->fend &= 0x60;         // clear sign
         break;


// variable size struct ??  how to specify ?? V st field, end field/relative address to row/value ??

       case 'V':                  // diVisor DEPRECATED to calc
       {
         float fx;
         // read one FLOAT val
         ans = sscanf(c->cmpos, "%f%n ", &fx, &rlen);       // FLOAT
         if (ans <=0) return do_error(c, 0, 1, "Divisor value Required");
         c->cmpos += rlen;

        //  need name + 2 terms, +x 1st, / divisor 2nd
         x = 0;

         n = add_mname(a,0);         //add zero name
         if (n)
           {
            add_link(CHADNL,a,CHMATHN,n,1);               // mterm -> adt (fwds)
            n->mcalctype = DP_FLT;
            x = add_mterm (n,0);
           }

         if (x)
          {
           x->calctype = DP_FLT;
           x->func = 1;
     //      x->calc = &xplus;
           x->npars = 1;
           x->fpar[0].dptype = DP_REF;
           x->first = 1;

           //2nd term

           x = add_mterm (x,0);              // 2nd -> 1st term
           x->calctype = DP_FLT;
           x->func = 4;
    //       x->calc = &xdiv;
           x->npars = 1;
           x->fpar[0].fval = fx;           // divsior
           x->fpar[0].dptype = DP_FLT;
          }

       }
         break;

       case 'W':                  // Word   - or write for syms
            if (c->fcom == C_SYM)    a->fend |= C_WRT;            // write for syms
            else
            {
               a->fend &= 0x60;           // keep sign
               a->fend |= 15;
               if (!a->cnt) a->cnt   = 1;
               set_pfwdef(a);
            }


          break;

       case 'X':                  // Print radix  ( Hex default)
         ans = getpd(c,4,2);
         if (!ans)
           {
             a->dptype = DP_HEX;
             do_error(c, 0,2, "Radix value missing, default to HEX");
           }

// change to :x 10.z  for float, where z is num places.
// t = flbuf+cmndposn;       // where we are in cmd string
// if (t = '.')    go float

  // cross check for R which will set pmode at 1
  // print mode & radix.  0 = default (hex) 1 = hex (user), 2 = address (hex+bank) 3 = bin  4 = dec int  5 = dec float (= calc only) (R, X)


// allow last digit as shortcut (0,2,6) ?? so X0.2 for 2 decimal places...

//switch ??

         else if (c->p[4] == 2 ) a->dptype = DP_BIN;
         else if (c->p[4] == 10 || c->p[4] == 0 ) a->dptype = DP_DEC;
         else if (c->p[4] == 16 || c->p[4] == 6 ) a->dptype = DP_HEX;
         else return do_error(c, 0, 1, "Radix value Invalid  (2,10,16)");

         if (ans == 2)
          {       //decimal place
            if (a->dptype != DP_DEC)  return do_error(c, 0, 1, "Float Invalid unless x=10");
                a->dptype = DP_FLT;            // set float print mode.
                a->pfd = c->p[5];        // number of digits field width (after .)
                // check pfd is valid ...

          }
          // reset pfw according to new print type.
                          // CANNOT add pfd to pfw....need the '.' to line up.....



         break;

       case 'Y':                  // bYte
         a->fend &= 32;           // keep sign, set size to 1
         a->fend |= 7;
         if (!a->cnt) a->cnt   = 1;
         set_pfwdef(a);
         break;

         // Z

       default:
       c->cmpos++;       //move to failed char
           return do_error(c, 0, 1, "Invalid Option");

      }          // end switch

    }  // end while

 return 0;
    }





int do_global_opts(CPS *c)
{
   int ans, opt;

   if (get_cmdopt(OPTARGC))   c->cptl = 1;     // set compact argument layout default
   if (get_cmdopt(OPTSTCA))   c->argl = 1;     // set data struct argument layout default

   if (!dirs[c->fcom].gopts) return 0;         //global opts not allowed


   // check the "[$' first, otherwise no globals

   readpunc(c);

   if (!chkstr(c,"[$")) return 0;

   // now read allowed options...
   c->sqcnt++;                         //have read a square bkt...

   while (c->cmpos < c->cmend)
    {
      if (c->error) break;

      // get next letter and check, main options

      opt = get_optchar(c,dirs[c->fcom].gopts);        // allowed letter main options

      switch(opt)          // toupper done already
     {

       case ']' :              // close global opts
         return 0;
         break;

       case 'Q' :              // terminator byte

         // read optional number of bytes max 3.
         ans = getpd(c,4,1);
         if (!ans) c->term = 1;
         else
          {
            if (c->p[4] < 1 || c->p[4] > 3) return do_error(c, 0, 1, "Size Invalid  (1-3)");
            c->term = c->p[4];
          }
         break;

       case  'A' :
         c->argl = 1;        // set args layout
         c->cptl = 0;
         break;

       case  'C' :

         c->cptl = 1;        // set compact layout
         c->argl = 0;
         break;


       case  'F' :
        // f <str> <reg1> <reg2>    func add/ tab add, cols

         ans = get_str(c, LUSPF);     //    spfstrs, NC(spfstrs),0);

         if (ans < 0) return do_error(c, 0, 1, " Invalid Subr type");

         // func lookup =2, tablu = 3
   //      if (ans < 8) c->spf = 2; else c->spf = 3;

         c->strix = (ans & 0xff) + 5;
         ans = getpx(c,5,2);

          // register check ?? what about size ??

         if (c->strix < 13)
          {
           if (ans < 1)   return do_error(c, 0, 1, "At least 1 value Required");
           if (ans > 1)   do_error(c, 0,2, "Extra Values Ignored");
          }
         else
          {

           if (ans < 2)   return do_error(c, 0, 1, "At least 2 values Required");
           if (ans > 2)   do_error(c, 0,2, "Extra Values Ignored");
          }


//check for size data ??

         c->adreg = c->p[5];                  // struct address register
         c->szreg  = c->p[6];                  // tab column register
         break;

      case '=' :       // an ANSWER definition.

         ans = getpx(c,5,1);
         if (!ans) return do_error(c, 0, 1, "Address Required");

         c->ansreg = c->p[5];
         break;

       default:
         do_error(c, 0, 1, "Invalid Option");
         break;
     }       //end switch

    readpunc(c);
  //  if (c->cmpos > c->cmend) go = 0;

    }
   return 0;
}


int do_adnl_data (CPS *c)
{
   // shell for adnl_item
  int opt;
  void *fid;
  uint ix;
 // get next delimiter and then options

  fid = &cmnd;           // dummy to start with
  ix = 0;
  while (c->cmpos < c->cmend)
    {
     readpunc(c);      //test

     if (c->cmpos >= c->cmend) break;

     opt = get_optchar(c,0);         // look for open bracket

     if (opt == '[') do_adnl_item (c,&fid,&ix);

     if (!opt) return do_error(c, 0, 1, "'[' expected");      // only 'open' expected here

     if (c->error) break;
    }



return 0;
}












int do_opt_str (CPS *c)
{
 // same as get_str, but strings for setopt, clropt, but need brackets around them
 // get_str does not account for optchars []()

// allow for a following number ??

   int ans, opt;

   c->p[0] = 0;

   while (c->cmpos < c->cmend)
    {
     // readpunc(c);       //get optchar does this

      opt = get_optchar(c,0);      //read next
      if (opt != '[' ) return do_error(c, 0,1," '[' expected");

      ans = get_str(c, LUOPTS);     //    optstrs, NC(optstrs),1);

      if (ans < 0)  return do_error(c, 0, 1, "Invalid Option");       // not found

      get_optchar(c,0);       // get close bracket

      if (c->sqcnt && c->cmpos < c->cmend) return do_error(c, 0,1," ']' expected");
      c->p[0] |= mopts[ans & 0xff];
    }

return 0;

}




void get_qstring(CPS *c)
{
   // return quoted string as terminated string for a name.
   // if CALC command check name and verify following '='

  uint rlen;

  readpunc(c);

  c->strsze = 0;

  if (c->cmpos > c->cmend || *c->cmpos != '\"') return;

  // allow and skip multiple quotes (as typo)
  // replace as spaces ?

  while (*c->cmpos == '\"') c->cmpos++;   // skip quote(s)

  readpunc(c);                    // safety check

  rlen = 0;

  while (c->cmpos < c->cmend)
     {
      if (rlen >= SYMSZE) break;

      if (*c->cmpos == '\"') break;        // end of name
      if (*c->cmpos == ' ' ) break;        // space is end of name ??
      c->string[rlen] = *c->cmpos;

      c->cmpos++;
      rlen++;
     }

// need extra check if space within quotes.



 if (rlen > SYMSZE)
   {
   do_error(c, 0,1, " Name too long");
   return;
   }

    c->strsze = rlen;
    c->string[rlen] = 0;
    c->cmpos++;                       // and skip quote


 // allow and consume multiple quotes (as typo)

    while (*c->cmpos == '\"') c->cmpos++;   // skip quote(s)

 if (c->fcom == C_CALC)
   {
     if (match_str(c, c->string, c->strsze, LUCLC) > 0)
        {
          do_error(c, 0,1,"cannot use a calc type as a name");
          return;
        }

     readpunc(c);
     if (*c->cmpos != '=') do_error(c, 0,1,"calc must start with = ");

     c->cmpos++;
   }



}


void replace_delims(void)
{

    // temp convertor for old->new

    // need to be able to spot math formulae in = ( ) and not change them............

    //END colon fails !!

 char *s, *d;              //source, dest
 int obkt, skip, name;

 s = flbuf;
 d = cnm;
 obkt = 0;
 name = 0;
 skip = 0;

 while (*s)
  {

    if (*s == '\"')
      {
        name ^= 1;                         // in a "name" sequence
        while (*(s+1) == '\"') s++;        // skip multiple quotes (as typo)
      }


    if (*s == '[')          skip += 1;    // skip any new style format pairs
    if (*s == ']')          skip -= 1;     // for nested setups



 if (!name && !skip)
   {
    if (*s == ':' || *s == '|')    // : is start or end
     {
       if (*s == '|') *d++ = '|';   // put pipe back at end of 'previous' item

       if (obkt) *d++ = ']';
       *d++ = '[';
       obkt = 1;                  // in '[ ]' from ':'

       while (*s == ':' || *s == '|') s++;               // skip the :, |, and any multiples
     //this may be wrong....



     }

//not if new format already ????

    if (*s == '$')
       {                //global options
        *d++ = '[';
        *d++ = *s++;          // change '$' to '[$'
        obkt = 1;
       }

   }           // end !name


  if (!name && *s == '#' )
      {        // if comment add close bkts if necessary
        if (obkt)
         {
          *d++ = ']';
          obkt = 0;
         }
      }


    if (*s == '\r' || *s == '\n' )
        {             //if line end add close bkts if colon
          if (obkt)
            {
             *d++ = ']';
      //       obkt = 0;
            }
          break;              //ALWAYS break if line end.
        }

    *d++ = *s++;       //otherwise, just copy
 }


*d = '\0';

strcpy(flbuf,cnm); // and copy new one over old one.
}



int parse_cmd(CPS *c, char *flbuf)
{
  // parse (and check) cmnd return error(s) as bit flags
  // return 1 for next line (even if error) and 0 to stop (end of file)

  int ans;
  char *t;                            // where we are up to
  DIRS* d;
  ADT *a;
  void *fid;
  HDATA* fd;

  memset(c,0,sizeof(CPS));      //         clr_cmd(c);                          // clear entire command holder
  fd = get_fldata();

  // if any ADT left connected, must be killed first so they don't hang on to a shorter ADT string.
  // but this does not tidy up any subs.....need subr to do properly

  fid = &cmnd;
  while ((a = get_adt(fid,0)))    // get first block (dummy fid)
  {
     fid = a;                          // next.....
     ans = get_lastix(CHADNL);         // save index of block
     chdelete(CHADNL, ans, 1);         // and delete it
   }


  if (!fgets (flbuf, 255, fd->fh[3])) return 0;

  flbuf[255] = '\0';            // safety

  replace_delims();                     // * temp -- replace colons etc with new format

  c->cmend = flbuf + strlen(flbuf);      // safety

  t = strchr(flbuf, '\n');                 // check for LF
  if (t) c->cmend = t;                     // mark end

  t = strchr(flbuf, '\r');                 // check for CR
  if (t) c->cmend = t;                     // mark it

  if (c->cmend == flbuf) return 1;         // this is an empty line;

  // check for comments AFTER name, below.

  c->cmpos = flbuf;

  readpunc(c);

  if (c->cmpos >= c->cmend) return 1;       // empty line after punc removed (at front)

  if (*c->cmpos == '#')  return 1;          // '#' comment at front

  if (!strncmp(c->cmpos,"//",2)) return 1;  // double slash comment at front

  // OK, now process the line


  ans = get_str(c, LUCMDS);          //cmds, NC(cmds),0);     // string match against cmds array

  if (ans < 0) return do_error(c, 0,1, "Invalid Command");             // cmd not found

  d = dirs+ans;
  c->fcom = ans;

  readpunc(c);

  if (d->maxpars)       // read up to 8 following addresses into p[0-n] (if any)
    {
     c->npars = getpx(c,0, d->maxpars);

     if (c->npars < 1) return do_error(c, 0,1, "Parameters Required");        // params reqd
     if (c->npars > d->maxpars) do_error(c, 0,2, "Extra Parameters Ignored"); // extra pars ignored (continue)

     // verify addresses, single and/or start-end pair

     chk_cmdaddrs(c,d);

     if (c->error > 1) return 0;

     if (c->error) return 1;

//cmnderror is 0 for warning, 1 for error

// return 0 if serious bank problem
// "no banks defined, cannot continue" ..................


    }

  // read name here, if allowed and present - sort out spaces in names ??

  readpunc(c);

  if (d->namex) get_qstring(c);

   // NOW check for comments, AFTER name read in

  readpunc(c);

  t = strchr(c->cmpos, '#');
  if (t)  c->cmend = t;              // shorten line as reqd

  //drop spaces at end of string for clean errors etc.

  while (ismypunc(c->cmend)) c->cmend--;

  *c->cmend = '\0';

  // now read addnl levels

  switch(d->stropt)
    {
      default:                  // case 0 (safety)
         do_global_opts(c);
         do_adnl_data(c);
      break;

      case 1:
        do_opt_str(c);         // option strings (setopt)
        break;

      case 2:
         read_calctype(c);
         if (c->ansreg <= 0) return do_error(c, 0,1, "Invalid Calctype");
         do_math_str(c,c);        // math formula
        break;
    }

    if (c->error) return 1;

    if (c->adtcnt > d->maxadt) return do_error(c, 0,1,"too many [data] items specified\n");
    if (c->adtcnt < d->minadt) return do_error(c, 0,1,"More [data] items required\n");


 // any further error reporting is responsibility of each handler command

  c->adtsize = adn_totsize(&cmnd);          //safety.

  d->setcmd (c);                  // do setup procedure

  // add sym name for cmds other than SYM itself, but not maths ones
  // change to < C_SYM

  if (d->namex && c->fcom < C_SYM && c->string)
   {

     add_sym(c->string,c->p[0],0, 7|C_USR, 0,0xfffff);

     if (get_lasterr(CHSYM)) do_error(c, 0,2,"Embedded symname ignored");     //don't stop, but report

   }

 if (c->firsterr) wnprt(1,0);    // extra LF after any errors
 show_prog(0);
 return 1;                   // next line
}



void add_iname(int from, int ofst)
{
  // add interrupt autonames
  uint i, x;
  char *z;

  // ignore_int can be at the end of a jump in multibanks....

 // if (!PINTF) return;      // moved out to call
  from &= 0xffff;            // drop bank
  from -= 0x2010;
  from /= 2;                 // correct index for table lookup

  if (get_cmdopt(OPT8065)) x = 1; else x = 0;;  // 8061 or 8065
  z = cnm;

  if (get_numbanks())               // sort out bank number
   {
    i = ofst >> 16;             // add bank number after the I
    z += sprintf(cnm, "I%u_",i-1);
   }
 else
    z += sprintf(cnm, "I_");         // no bank number

 i = g_byte(ofst);

 if ( i == 0xf0 || i == 0xf1)       // this is 'ignore int'
    {
      sprintf(z,"Ignore");
      add_sym(cnm, ofst, 0, 7|C_SYS, 0,0xfffff);

      return;
    }

 for (i = 0; i < NC(inames); i++)
    {
    if (from <= inames[i].end && from >= inames[i].start && inames[i].p5 == x)
       {
       z += sprintf(z,"%s",inames[i].name);                      // number flag set
       if (inames[i].num) z+=sprintf(z, "%u", from-inames[i].start);
       add_sym(cnm, ofst, 0, 7|C_SYS, 0,0xfffff);                //|C_NOFLD,0,0xfffff);
       break;    // stop loop once found for single calls
       }
    }
return;
 }



void do_preset_syms(void)
 {
   uint i;
   DFSYM *z;

   if (get_cmdopt(OPT8065))
    {
     for (i = 0; i < NC(d65syms); i++)
     {
       z = d65syms+i;
       add_sym(z->symname,z->addr,z->fstart,(z->fend | C_SYS),0,0xfffff);
     }
    }
   else
    {
     for (i = 0; i < NC(d61syms); i++)
     {
       z = d61syms+i;
       add_sym(z->symname,z->addr,z->fstart,(z->fend | C_SYS),0,0xfffff);
     }
    }

  }

uint set_bnk (CPS *c)
{
  int i, bk, bank, addr;
  BANK *b, *bkmap;
  HDATA *fd;


  fd = get_fldata();
  bkmap = get_bkdata(0);

  // bank no , file offset,  (opt) pcstart, (opt)  bkend  , (opt) fillstart

   // use bk[2].cmd as marker to indicate bank command used already for
   // following bank commands, as this would not get set otherwise.

   // validate all params before overwriting anything

   if (c->npars < 2) return do_error(c, 0,1, "Need at least two params");

   bk = c->p[0];          // first param is BANK

   if ((bk & 6) || bk > 15) return do_error(c, 0,1,"Invalid Bank Number");

   if (bkmap[bk].usrcmd)   return do_error(c, 0,1, "Bank already defined");

   bk++;
   bank = bk << 16;          // bank in correct form

   // 2nd is file offset
   if (c->p[1] < 0 || c->p[1] > (int) fd->flength) return do_error(c, 0,1, "Invalid File Offset");

   if (bk > 0 && bkmap[bk-1].bok)
      {        //check overlap in file offset
       if (c->p[1] <= (int) bkmap[bk-1].filend) return do_error(c, 0,1, "File offsets Overlaps with");
      }

   if (c->npars > 2)
     {     // pc start specified (for non std offset) (p[0] bank, p[1] start file offset)
      addr = nobank(c->p[2]);
      if (addr < PCORG-2 ||  addr > PCORG+3 ) return do_error(c, 0,1, " Bank Start");
     }

   if (c->npars > 3)
     {     // bank end specified. p[0] bank, p[1] start file offset)
      addr = nobank(c->p[3]);
      if (addr > 0xffff) return do_error(c, 0,1, "Bank End");

      addr -= nobank(c->p[2]);       // size, end-start.
      addr++;

      if ((c->p[1] + addr) > (int) fd->flength) return do_error(c, 0,1,"Bank end > File Size");

     }


//  if (c->npars > 4)      //fill specified zero otherwise


// then sort fillstart

// NEED MORE CHECKS HERE TO STOP CRASHES and set up data flag markers ..............


// clear valid map entries

   if (!bkmap[3].usrcmd)
      {    //first use of 'bank' command - clear whole map.
  #ifdef XDBGX
     DBGPRT(1,"Banks CLEARED ***************");
   #endif

   do_error(c, 0,2, "Detected Banks replaced with user command");

       for (i = 0; i < BMAX; i++)
          {
           b = bkmap+i;
           b->bok = 0;
           b->cbnk = 0;
           b->bprt = 0;
           b->bkmask = 0;
           //but keep others !!
          }
       set_numbanks(-1);                // no banks
       bkmap[3].usrcmd = 1;
      }

   b = bkmap+bk;
   b->usrcmd = 1;
   b->bok = 1;
   b->bprt = 1;    // print in msg file
   b->filstrt = c->p[1];

   if (c->npars > 2) b->minromadd = nobank(c->p[2]) | bank;
   else  b->minromadd = PCORG | bank;

   if (c->npars > 3) b->maxromadd = nobank(c->p[3]) | bank;
   else  b->maxromadd = 0xffff;

   b->filend =  b->filstrt + (b->maxromadd - b->minromadd);

   // fldata is still valid...
   if (b->filend > fd->flength)
      {
       b->filend = fd->flength-1;
       b->maxromadd = (b->filend-b->filstrt) + b->minromadd;
      }

  //  b->maxpc = b->maxbk;

// file read ???     anything ??

  b->fbuf = (uchar*) mem(0,0, 0x10000);          // big enough for bank

  fseek (fd->fh[0], b->filstrt,SEEK_SET);        // set file start - CHECK ERROR

  fread (b->fbuf + 0x2000, 1, b->filend-b->filstrt+1, fd->fh[0]);

  set_numbanks(get_numbanks()+1);

    #ifdef XDBGX
     DBGPRT(0,"Bank Set %d %x %x ",bk, b->minromadd,b->maxromadd);
     DBGPRT(1," #(%x - %x)",b->filstrt, b->filend);
   #endif
   return 0;
 }






int getudir (int ans)
{
  int addr,addr2, i, j, bank;
  BANK *b, *bkmap;
  HDATA *fd;

  // cmdopts will be zero or P8065 here (before dir read)
  // and banks should be sorted (auto detect anyway)

  cmdopts |= OPTDEFLT;            // add default
  fd = get_fldata();
  bkmap = get_bkdata(0);



  if (fd->fh[3] == NULL)
    {
     wnprt(2,"# ----- No directive file. Set default options");
    }
  else
    {
      wnprt(2,"# Read commands from directive file '%s'", fd->fname[3]);
      while (parse_cmd(&cmnd,flbuf));
    }



  if (get_cmdopt(OPTRSYM)) do_preset_syms();

  if (get_cmdopt(OPTMANL)) return 0;         // entirely manual

  // now setup each bank's first scans and interrupt subr cmds


  for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;
     if (!b->bok) continue;

     #ifdef XDBGX
      DBGPRT(1,"--- auto setup for bank %d ---",i);
     #endif

     bank = i << 16;
     addr = b->minromadd;
     addr |= bank;

     add_scan (CHSCAN,addr, 0, J_STAT,0);                  // inital scan at PCORG (cmd)

     // add if not filler
     addr2 = 0;
     for (j = 0x200a; j < 0x200f; j+=2)
         if (g_word(j|bank) != 0xffff) addr2 = 1;

     if (addr2)
          add_cmd (0x200a|bank, 0x200f|bank, C_WORD|C_SYS,0);     // from Ford handbook


     j = (get_cmdopt(OPT8065)) ? 0x205f : 0x201f;
     j |= bank;

     add_cmd (0x2010|bank, j, C_VECT|C_SYS,0);               // interrupt vects with cmd

     for (j -= 1; j >= (0x200f|bank); j-=2)      // counting DOWN to 2010
        {
         addr2 = g_word (j);
         addr2 |= bank;
         if (get_cmdopt(OPTINTF)) add_iname(j,addr2);
         add_scan (CHSCAN,addr2, 0, J_SUB,0);      // interrupt handling subr

        }
     }

   #ifdef XDBGX
      DBGPRT(1," END auto setup");
     #endif

return 0;
 }



