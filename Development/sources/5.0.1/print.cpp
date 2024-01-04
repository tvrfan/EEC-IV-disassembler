

/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version number and notes in shared.h
 ******************************************************/

#include "print.h"

/************************
 * print routines
 **************************/



 //int indent;               // TEST for code print layout
uint  gcol;                   // where main print is at (no of chars = colno)
uint  lastpad;                // last padded to (column)
uint  lastcmt;                // last cmt state for end
uint  wcol;                   // where warning output is at

//CMNT  fcmnt;                  // comments holder from cmnts file

char  nm    [32];             // for floats and string hacking
char  ag    [32];             // separate args string (nm used for comments
uint  plist [64];             // generic param list and column positions for structs
char  cmbuf [256];            // for directives and comment file reads

//CMNT  acmnt;               // auto comments holder - temp removed

LBK   prtblk;                 // dummy LBK block for printing adjustments
CPS   pcmd;                   // command holder for comments
uint  nsyms;                  // and number matched (or a return answer ??

SBK   prtscan;                // scan block for printing
SBK   xscan;                  // for opcode searches

uint rambnk = 0;                 // banks to carry over for print
uint datbnk = 0;




INST  psinst;                  // instance block for searches
INST  pinst;                   // instance block for printing


 // positions of print columns in printout

#define MNEMPOSN   cposn[1]    // 26      opcode mnemonic starts (after header)
#define OPNDPOSN   cposn[2]    // 32      operands start             (+6)
#define PSCEPOSN   cposn[3]    // 49      pseudo source code starts  (+17)
#define CMNTPOSN   cposn[4]    // 86      comment starts             (+37)
#define WRAPPOSN   cposn[5]    // 180     max width of page - wrap here

// [0] is for 'modified' marker for print

uint cposn [6] = {0, 26, 32, 49, 83, 180};               // standard posns

//calc field widths by actual value per radix. [0] is list size, index is the fieldwidth.

uint hexpfw[8]   = {8, 0x10, 0x100,0x1000,0x10000, 0x100000, 0x1000000, 0x10000000};         // hex
uint decpfw[10]  = {10,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000};      // decimal

uint binpfw[30]  = {30, 2,4,8,16,32,64,128,256,1024,2048,4096,8192,16384,32768, 65536, 131072,262144,524288, 1048576, 2097152,4194304,8388608,16777216,
33554432,67108864,134217728,268435456,536870912,1073741824};          //binary          ,2147483648



// extra opcode pseudo sources for replace of R0 opcodes,
// carry, ldx, goto

cchar *scespec[] = {
  "\x2 += CY;",         // R0 + carry replace
  "\x2 -= CY;",
  "\2 = \1;",           // LDX for reduced 3 op, XOR
  "\x9"                 // 'bare' goto
 };

//  for swopping over cmp (R0, x) operands

cchar *swopcmpop[] = {

 "<" ,        // jgtu '>'  (62) to 'jltu' and JC std
 ">=",        // jleu '<=  (63) to 'jgeu' and JNC std
 "<" ,        // jgt  '>'  (64) to 'jlt'
 ">=",        // jle  '<=' (65) to 'jge'
 "<=",        // jge  '>=' (66) to 'jle'
 ">" ,        // jlt  '<'  (67) to 'jgt'
 "=" ,        // je  remains same
 "!=",        // jne remains same
 ">",         // JNC
 "<="         // JC

};

// for the dummy cmp R0,0 or cmp Rx Rx etc

cchar *zerocmpop[] = {

 "false" ,        // jgtu
 "true"  ,        // jleu
 "false" ,        // jgt
 "true"  ,        // jle
 "true"  ,        // jge
 "false" ,        // jlt
 "true"  ,        // je
 "false"          // jne
};


//******************************************************************************







void init_prt(void)
{
  gcol     = 0;
  lastpad  = 0;
}

uint wnprt (uint nl, cchar *fmt, ...)
{  // warning file prt

  uint chars;
  FILE *f;
  chars = 0;

  f = get_file_handle(2);
  if (fmt)
    {
     va_list args;
     va_start (args, fmt);
     chars = vfprintf (f, fmt, args);
     va_end (args);
    }
  wcol += chars;
  if (nl) wcol = 0;         // newline
  while (nl--) fprintf(f, "\n");
  return chars;             // useful for if statements
}



void p_pad(uint c)
{
// List file.  Pad out to required column number if not already there
// and not already padded to. Add spaces if pstr (gcol) > lastpad
uint i;

FILE *f;

f = get_file_handle(1);

if (c > WRAPPOSN) c = 0;             //temp fixup WRAPPOSN;

  if  (c <= gcol && gcol > lastpad) c = gcol+2;          // add 2 spaces...
  for (i = gcol; i < c; i++)  fputc(' ',f);   // fprintf(fldata.fl[1], " ");
  gcol = c;
  lastpad = c;                         // last padded column
}

void wrap()
 {
  // newline and wrap back to comments column if already greater,
  // or lastpad column if not

  FILE *f;

  f = get_file_handle(1);

  fprintf(f, "\n");
  gcol = 0;
  if (lastpad > CMNTPOSN)  lastpad = CMNTPOSN;
  p_pad(lastpad);
 }

uint pstr (uint nl,cchar *fmt, ...)
{   // List file print

  uint chars;
  FILE *f;

  f = get_file_handle(1);
  chars = 0;

  if (fmt)
   {
    va_list argptr;
    va_start (argptr, fmt);
    chars = vfprintf (f, fmt, argptr);
    va_end (argptr);
    if (gcol >= WRAPPOSN) wrap();
   }
  gcol += chars;

  if (nl) gcol = 0;         // newline
  while (nl--) fprintf(f, "\n");
  return chars;
}


uint prtfl(cchar *fmt, float fv, int pfw, int pfd, uint (*prt) (uint,cchar *, ...))
 {
  // separate float print, to control width
  // 'prt' argument allows print to debug or message files

  uint cs;
  char *t, p;
  prt(0,fmt);

  if (pfw) p = ' '; else p = '\0';  // zero or space
  cs = sprintf(nm,"%*.*f", pfw, pfd, fv);
  t = nm + cs-1;

// printf will round the value to pfd, but will add trailing zeroes
// replace extra trailing zeroes with spaces.

  while (*t == '0' && *(t-1) == '0') *t-- = p;
//  if (*t == '.') *t-- = p;
  cs = prt(0,"%s", nm);      //cs = chars printed

  return cs;
}



void p_run(uint nl, char c, uint num)
{
 uint i;
 FILE *f;

 f = get_file_handle(1);

 for (i = 0; i < num; i++) fprintf(f, "%c",c);
 gcol +=num;
 if (gcol >= WRAPPOSN) wrap();
 if (nl) gcol = 0;         // newline
 while (nl--) fprintf(f, "\n");
}



void pchar(char z)
{
  FILE *f;

f = get_file_handle(1);
 fputc(z,f);
 gcol++;
 if (gcol >= WRAPPOSN) wrap();
}


uint pbin(uint val, uint fend)
{  // print val in binary
   // not sure about pfw yet ...
  uint i, tval, pfw;    //, places;


if (val & 0xf000)  pfw = 16;
else if (val & 0x0f00)  pfw = 12;
else if (val & 0x00f0)  pfw = 8;
else pfw = 4;                // temp

 tval = val;
 for (i = 16; i > 0; i--)
  {  // find ms '1' bit
    if (i <= pfw)
      {
       if ((i/4*4) == i) pchar(' ');    //space every 4
       if (tval & 0x8000)  pchar('1'); else pchar('0');
      }
    tval <<= 1;
  }

return pfw;

}


uint paddr(uint addr)
{

 if (valid_reg(addr)) return nobank(addr);
 if (!get_numbanks())  return nobank(addr);         // only one bank

 // bank+1 used internally, leading zeroes
 addr -=  0x10000;
 return addr;
}


void prtaddr(uint addr,uint sp, uint (*prt) (uint,cchar *, ...) )
{
  // printer for addresses with or without bank.
  // if single bank or no bank drop bank
  // For listing or debug output

 if (!get_numbanks() || valid_reg(addr))
     prt(0,"%x", nobank(addr));       // no leading zero
 else
    {    // bank+1 used internally, leading zeroes
     prt(0,"%0x", (addr >> 16) -1);
     prt(0,"%04x", nobank(addr));
    }
  while (sp--) prt(0," ");
}


void prt_banks(uint (*prt) (uint,cchar *, ...))
{
  int i;
  BANK *x;

  // bank no , file offset,  (opt) pcstart, (opt)  pcend  (opt) end file offset, (opt) fillstart

  if (prt == wnprt)
  {
  // prt(2,"# Banks Found.  For information, can uncomment to manually override");
  // prt(1,"# bank_num, file_offset, start_addr, end addr, [filler_start]");

   for (i = 0; i < BMAX; i++)
    {
     x = get_bkdata(i);
     if (x->bprt)
       {
        prt(0,"# bank  %d %5x ", x->dbank-1, x->filstrt);
        prtaddr(x->minromadd,1,prt);
        prtaddr(x->maxromadd,1,prt);
        if (x->usrcmd) prt (0,"  # by user command");
        if (!x->bok) prt (0,"** ignored **");
        prt(1,0);
       }
    }
   prt(1,0);
  }

#ifdef XDBGX
else
  {
 //  debug printout

  DBGPRT(1,0);
  DBGPRT(1,"-------DEBUG BANKS---------");
  DBGPRT(1,"bank filstart, minpc, maxbk, filend. bok,cmd,bkmask, cbnk, P65 dbank" );

  for (i = 0; i < BMAX; i++)
    {
         x = get_bkdata(i);           //x = bkmap + i;
      if (x->bok)
       {
         DBGPRT(0,"%2d %5x %5x %5x (%5x)", i, x->filstrt, x->minromadd, x->maxromadd, x->filend);
         DBGPRT(1,"  %x %x %x %x %x %x", x->bok, x->usrcmd,x->bkmask, x->cbnk, x->P65, x->dbank);
       }
    }
  DBGPRT(1,0);
  DBGPRT(1," Non bok ---");

  for (i = 0; i < BMAX; i++)
    {
         x = get_bkdata(i);        //x = bkmap + i;
      if (!x->bok && x->minromadd)
        {
      DBGPRT(0,"%2d %5x %5x %5x (%5x)", i, x->filstrt, x->minromadd, x->maxromadd, x->filend);
         DBGPRT(1,"  %x %x %x %x %x %x", x->bok, x->usrcmd,x->bkmask, x->cbnk, x->P65, x->dbank);
        }
     }
 DBGPRT(1,0);
}

#endif

}



void prt_cellsize(ADT *a, uint (*prt) (uint,cchar *, ...))
{

  uint sign, fend,c;

  sign = (a->fend & 32);
  fend = a->fend & 31;

  if (a->fstart != fend) prt (0,"%c", sign ? 'S' : 'U');

 c = 0;
 if (!a->fstart)
    {
     if (fend == 7)  c = prt(0,"%c ", 'Y');
     else
     if (fend == 15) c = prt(0,"%c ", 'W');
     else
     if (fend == 23) c = prt(0,"%c ", 'T');
     else
     if (fend == 31) c = prt(0,"%c ", 'L');
    }

 if (!c)
    {
     prt(0,"%c %d ", 'B', a->fstart);
     if (a->fstart != fend) prt(0,"%d" , fend);
    }
}



void prt_pfw(ADT *a, uint (*prt) (uint,cchar *, ...))
{
// will have subfield sizes in here
 if (a->pfw && a->pfw != get_pfw(a)) prt(0,"P %d ",a->pfw);
}


void prt_radix(ADT *a, int dpt, uint (*prt) (uint,cchar *, ...))
{
  // print mode & radix.
  if ((a->dptype != dpt) || prt != wnprt)
    {      // print radix, print mode
     switch (a->dptype)
      {
        default:
          break;

         case DP_HEX :
          prt (0, "X 16 ");     // hex
          break;

        case DP_ADD :
          prt (0, "R ");     // address  (ref) ??
          break;

        case DP_BIN :
          prt(0,"X 2 ");     // binary
          break;

        case DP_DEC :
          prt(0,"X 10 ");    //decimal
          break;

        case DP_FLT :
          prt(0,"X 10");    //decimal F is temp.....
           if (a->pfd) prt(0,".%d", a->pfd);
            prt(0," ");
          break;

         }
    }
}

void prt_calc(void *fid, uint pix, uint (*prt) (uint,cchar *, ...));      //temp...
void prt_calctype(uint ,uint (*prt) (uint,cchar *, ...));



void prt_adnl(ADT *a, int drdx, uint (*prt) (uint,cchar *, ...))
  {

  FKL *l;
  MATHN *n;
 // MATHX *x;
  ADT *b;            //temp...

   //    if (prt != wnprt) prt(0, "{ %lx,%lx}", a, a->fid);
   #ifdef XDBGX
         if (prt == DBGPRT)
           {
             prt(0,"{ fid %lx %lx,",  a->fid,a);
      //       prt (0,"R%x, sz %d}", a->dreg, cellsize(a));
             prt (0," sz %d ", cellsize(a));
           }
        #endif

       prt(0, "[ ");

       if (a->newl) prt(0,"| ");  // newline is for END of this block

       if (a->cnt > 1) prt(0,"O %d ", a->cnt);

       if (a->bank) prt(0,"K %x ", a->bank-1);

       prt_cellsize(a,prt);

       prt_pfw(a,prt);                         // print fieldwidth if not default

       if (a->fnam) prt(0,"N ");

       l = get_link(a, 0, 0);     // adt to math name (by keyout)

       if (l)
        {
            n = (MATHN*) l->keydst;       //to name
            prt(0,"= ");
            if (n->nsize) prt(0,"%s ", n->mathname);   //print only name from ADT
            else
              {
               prt_calctype(n->mcalctype,prt);
               prt_calc(n,0,prt);
               prt(0,") ");
              }
        }

       if (!l) prt_radix(a,drdx,prt);                  // print radix if not default and no calcs




//do subs as recursive loop

       b = get_adt(a,1);
if (b)
{
  void *fid;
    prt_adnl(b, drdx, prt);
    fid = b;
    while ((b = get_adt(fid,0)))
  {
    if (b->cnt || prt != wnprt) prt_adnl(b, drdx, *prt);  // ignore if zero size, and not debug
    fid = b;
  }

}


        prt(0, "]");
     }







void prt_adt(void *fid, int drdx, uint (*prt) (uint,cchar *, ...))
{
  // print additional params for any attached ADT chain, LBK mainly
  // the 'prt' argument allows print to debug or message files
  // dcm is 1 bit

ADT *a;
//FKL *l;
//MATHN *n;
//MATHX *x;
//int nl;

 // nl = 0;

while ((a = get_adt(fid,0)))
  {
    if (a->cnt || prt != wnprt) prt_adnl(a, drdx, *prt);  // ignore if zero size, and not debug
    fid = a;
  }
}


/*
void prt_mnum(MNUM *p, uint (*prt) (uint,cchar *, ...))
{
 // dptype defines print and calc options
 // datatype  0 empty  1 union is integer,  2 union is ref,  3 union is float, (7 subterm/subcalc attached - no print)
 // pmode     Default DECIMAL.  1 = hex , 2 = address (hex+bank) 3 = bin  4 = decimal int  5 = decimal float

* #define DPHEX      1               // hex       integer
#define DPADD      2               // hex       address (uint+bank)
#define DPBIN      3               // binary    integer
#define DPDEC      4               // decimal   integer
#define DPFLT      5               // float     float
#define DPREF      6               // 'X' ref   extract as per ADT definition (= data field)
#define DPSUB      7               //  -        subterm, maths only
*


    switch(p->dptype)
       {
         default :
           break;

        case DP_HEX:
           prt (0, "%x", p->ival);
           break;

        case DP_ADD:
           prtaddr (p->ival,0,prt);
           break;

         case DP_DEC:
           prt (0, "%d", p->ival);
           break;

         case DP_FLT :
           prtfl(0, p->fval, 0,3, prt);
           break;

         case DP_REF:
           prt (0, "x");
        //   if (n->refno > 1)
        //   prt(0, "%d", n->refno);
           break;



       }


}
*/

void prt_calc(void *fid, uint pix, uint (*prt) (uint,cchar *, ...))
{

  MATHX *a;
  MNUM *n;

  uint i;

  while ((a = get_mterm(fid,pix)))
   {

     fid = a;
     pix = 0;              //clear for rest of chain
     if (!a->first)
     prt(0, " %s ", get_mathfname(a->func));

     if (a->fbkt)  prt (0,"(");           //func bkt

for (i = 0; i < a->npars; i++)
 {
    n = a->fpar + i ;

  if (i) prt (0, ", ");

    switch(n->dptype)
       {
         default :
           break;

         case DP_FLT :
           prtfl(0, n->fval, 0,3, prt);
           break;

         case DP_REF:
           prt (0, "x");
        //   if (n->refno > 1)
        //   prt(0, "%d", n->refno);
           break;
         case DP_ADD:
           prtaddr(n->ival, 0, prt);
           break;


         case DP_HEX:
           prt(0, "%x", n->ival);
           break;

         case DP_DEC:
           prt (0, "%d", n->ival);
           break;

         case DP_SUB:
             prt(0,"(");             //temp
             prt_calc(fid,i+1, prt);
             prt(0,")");
           break;
       }
 }
if (a->fbkt)  prt (0,")");            //funcbkt

   }

}


void prt_calctype(uint type,uint (*prt) (uint,cchar *, ...))
{
    switch(type)
    {
        default :
         prt (0, "? ");
          break;

        case DP_ADD:
         prt (0, "address ( ");
         break;

        case DP_DEC:
         prt (0, "integer ( ");
         break;

        case DP_FLT:
         prt (0, "float ( ");
         break;


    }
}

void prt_calcs(uint (*prt) (uint,cchar *, ...))

{
  MATHN *n;
  uint ix;
  CHAIN *ch;
 // FKL *l;

  ch = get_chain(CHMATHN);

 for (ix = 0; ix < ch->num; ix ++)
 {
    n = (MATHN *) ch->ptrs[ix];      //by name in top loop
    if (n->nsize)          //hide null names
     {
      prt (0, "calc \"%s\" =  ", n->mathname);
      prt_calctype(n->mcalctype,prt);
      prt_calc(n,0,prt);
      prt(0," )");
      if (n->sys)      prt(0,"             # Auto Added by SAD ");
      prt(1,0);
     }
 }

prt(2,0);
}



void prtcopts(uint (*prt) (uint,cchar *, ...))
{
  // stop dupls by removing matched flags
 uint i, flgs;
 cchar *s;

 flgs = get_cmdopt(0xffff);        //all cmd opts

 i = 0;
 while (flgs)
  {
    flgs = get_cmdoptstr(flgs, i++, &s);
    if (s) prt(0, "[%s]", s);
  }

}


void prt_opts(void)
{
 // print opt flags to msg file
 wnprt(0,"setopt  " );
 prtcopts(wnprt);
 wnprt(2,0);
}

void prt_layout(void)
{
 // print layout options
 int i;
 if (!cposn[0]) return;    //no changes

 wnprt(0,"layout  " );
 for (i = 3; i < 6; i++)
   {
    wnprt(0," %d",cposn[i]);
   }
 wnprt(2,0);
}





uint prt_spf(void *fid, uint p, uint (*prt) (uint,cchar *, ...))
{
  SPF * s;
  uint f;
 // SBK *x;

  while ((s = get_spf(fid)))
    {
     switch (s->spf)
     {
         default:
            prt(0," {dflt spf %d l=%d args=%d}",s->spf, s->level, s->argcnt);
            if (s->jmparg) prt(0, "JUMP!");
            for (f = 0; f < s->argcnt; f++)  prt(0, " %x," , s->pars[f].reg);
            break;

         case 1:                            // answer
            if (!p) p = prt(0," [$");
            prt(0,"  = %x", s->pars[0]);                //addrreg);
            break;

         case 5:

            if (prt != wnprt)
               {                       //args
                  prt(0," {5 fixed, lev=%d args=%d}",s->level, s->argcnt );
                     if (s->jmparg) prt(0, "JUMP!");
                   for (f = 0; f < s->argcnt; f++)  prt(0, " %x," , s->pars[f].reg);
               }
            break;

         case 6:
            if (prt != wnprt)
               {                       //args
                 prt(0," {6 variable, lev=%d args=%d}",s->level, s->argcnt );
                 for (f = 0; f < s->argcnt; f++) prt(0, " %x," , s->pars[f].reg);
               }
            break;

         case 7:
            if (prt != wnprt)
               {                       //args
                 prt(0," {7 EMULATE, lev=%d args=%d}",s->level, s->argcnt );
                 for (f = 0; f < s->argcnt; f++) prt(0, " %x," , s->pars[f].reg);
               }
            break;

         case 2:            //funclu
          if (!p) p = prt(0," [$");
          f = 0;                        //  base of func names
          if ((s->fendin & 31) > 7)   f = 4;   //  word row
          if (s->fendout & 32) f += 1;
          if (s->fendin & 32)  f += 2;

          prt(0," F %s %x", get_spfstr(f),s->pars[0].reg);           //addrreg);
          break;

       case 3:     // tab lookup
        if (!p) p = prt(0," [$");
        f = 8;
        if ((s->fendout & 31) > 7)   f = 10;   //  word row
         if (s->fendout & 32)        f += 1;

        prt(0," F %s %x", get_spfstr(f),s->pars[0].reg);              //addrreg);

            // tab subroutines need extra par
        prt (0," %x", s->pars[1].reg);                   //sizereg);
        break;

     }

    fid = s;
     }
     return p;
}











void pl_spf (CHAIN *c, uint ix, uint (*prt) (uint,cchar *, ...))

{
  SPF *s;
  DPAR *d;
 // RST *g;            //temp regstat
 // SBK *x;
  void *fid;
  uint f;

  s = (SPF*) c->ptrs[ix];

//only dbgprt at the moment, links to SBK... NOT ALWAYS !!! can link to another spf !!


// if (s->spf < 5)
  // {
 //    x = (SBK*) s->fid;
 //    if (x) prt(0," scan %x (%d)", x->start, x->proctype);  else prt(0, "ZERO!!");
 //  }

 /*  if (s->spf == 10)
   {
    p = (RPOP*) s->fid;
    prt(0," RP %x ", p->dreg);
   }  */

  prt(0," SPF [%lx] %lx, %d %d {%d}", s , s->fid, s->spf, s->level, s->argcnt);
      for (f = 0; f < s->argcnt; f++)
         {
           d = s->pars + f;

           prt(0, "|%x sz%d (%x) " ,d->reg, d->dfend, d->refaddr);
           if (d->notarg) prt(0, "NA ");
        //   if (d->enctype)
         //     {
         //      prt(0, "(%x,%x)," , d->enctype, d->encbasereg);
          //    }
    //       g = get_rgstat(d->reg);                             //no good - not the status NOW !!!
    //       prt(0, "RG sc %x, arg %d  ", g->sreg, g->arg);

         }


/* add some of these for extra checking from regstat
  uint sreg    : 8;       // source register (i.e. the popped one)
  // uint stype : 2 ?        // for a non-pop copy to copy option ??? e.g. 7992 in A9L is ptr, 36a9 is direct...
  //direct (from popped), indirect (via Rx through to pop), (pointer to another register)
  uint fend    : 5;       // size 0-31 for arg sizes
  uint argcnt    : 4;       // mark that popped reg is incremented -> args present.
  uint rbcnt   : 3;       // changed count, for rbases
  uint rbase   : 1;       // valid rbase register
  uint popped  : 3;       // register currently popped = index to RPOP
  uint arg     : 1;       //this register holds an argument (or part of it)


 //       uint reg         : 8;
//  uint encbasereg  : 8;       // encode register (rbase)     // can go here ???
//  uint enctype     : 4;       // encoded type

*/



//more pars to go in here............


   fid = s;

  while ((s =  get_spf(fid)))
    {
     prt(0,", [%lx] %lx, %d %d", s , s->fid, s->spf, s->level);
     for (f = 0; f < s->argcnt; f++)  prt(0, " %x," , s->pars[f]);
     fid = s;

    }


prt (1,0);
}

/*
int pl_rst(CHAIN *,uint, uint (*prt) (uint,cchar *, ...))
{
    //dummy for now
    return 0;
}*/




void prt_sglo(SBK *sc, uint (*prt) (uint,cchar *, ...))
{
uint x;

if (!sc) return;

x = 0;
  if (!get_cmdopt(OPTARGC) && sc->adtcptl)
       {
        x = prt (0," [$ C");
       }
   x = prt_spf(vconv(sc->start),x,prt);

  if (x) prt(0," ]");        // prt close if reqd
}


void pl_rbt(CHAIN *x, uint ix, uint (*prt) (uint,cchar *, ...))
{

  RBT *r;
  RST *v;

    r = (RBT*) x->ptrs[ix];

    v = get_rgstat(r->reg);

        if (prt == wnprt)
          {
           if (!r->inv)
             {      //not invalid or flagged invalid
              prt(0,"rbase %x " , r->reg);
              prtaddr(r->val,1,prt);
              if (r->rstart)
               {
                prt(0,"  ");
                prtaddr(r->rstart,1,prt);
                prtaddr(r->rend,0,prt);
               }
              if (r->usrcmd) prt(0,"       # Auto added by SAD");
              prt(1,0);
            }
         }
        else
          {
           prt(0,"rbase %x %x" , r->reg , r->val);
           prt(0,"  %x %x", r->rstart, r->rend);
           if (v) prt(0," cnt %d, ",v->rbcnt);
           if (r->inv) prt (0, "  INV");
           if (r->usrcmd) prt(0,"       # cmd");
        prt(1,0);
          }

}


void pl_adl(CHAIN *x, uint ix, uint (*prt) (uint,cchar *, ...))
{

  ADT *a ;
 // uint v;


    a = (ADT*) x->ptrs[ix];

    prt_adnl(a, 0, prt);
    prt(1,0);

}


void prt_scans(void)
{
  // not merged with DBG version, as it's very different.
  uint ix;
  SBK *s;
  CHAIN *x;

  x = get_chain(CHSCAN);

   for (ix = 0; ix < x->num; ix++)
    {
     s = (SBK*) x->ptrs[ix];
     if (s->usrcmd) {wnprt(0,"scan ");
     prtaddr(s->start,0,wnprt);
     wnprt(1,0);}
    }
 wnprt(1,0);
}


void prt_cglo(LBK *c,uint (*prt) (uint,cchar *, ...))
{
  uint f;

  f = c->term + c->argl + c->cptl;

  if (f) prt (0,"[$");

   if (c->term)
     {
       prt (0," Q");
       if (c->term > 1) prt(0," %d", c->term);
      }

  if (c->argl)   prt (0," A");
  if (c->cptl)   prt (0," C");

  if (f)  prt (0,"] ");
}


void pl_cmd(CHAIN *x, uint ix, uint (*prt) (uint,cchar *, ...))
{
  LBK *c;

  if (x->lastix > x->num) return;

  c = (LBK*) x->ptrs[ix];

  if (prt == wnprt)
       {         // real printout
   //      if (c->fcom == C_TIMR) prt (0,"# ");
         prt(0,"%-7s ", get_cmd_str(c->fcom));           //cmds[c->fcom]);
         prtaddr(c->start,1,prt);
         prtaddr(c->end  ,0,prt);
       }
     else
      {   // debug printout
          prt(0,"{%lx} ", c);
          prt(0,"%-7s %x %x", get_cmd_str(c->fcom), c->start,c->end);
      }


  // global options here

  prt(0, "     ");
  prt_cglo(c,prt);

  prt_adt(vconv(c->start), (c->fcom == C_TABLE || c->fcom == C_FUNC) ? 1 : 0, prt);

  if (c->sys) prt(0,"             # Auto Added by SAD ");
  prt(1,0);






  //return 1;
}

void pl_bnk(CHAIN *x, uint ix, uint (*prt) (uint,cchar *, ...))
{
  BNKF *c;

  //only for debug ??

  if (x->lastix > x->num) return;

  c = (BNKF*) x->ptrs[ix];

if (c->filestart < 0)prt(0,"   -%x", -c->filestart); else prt(0,"%5x", c->filestart);
  prt(0," %5x  %5x", c->pcstart, c->jdest);
  prt(0," %2d (61 %2d %2d), (65 %2d %2d)", c->skip, c->vc61, c->ih61, c->vc65, c->ih65);
  prt(0," lp %2d, cd %2d tb %2d", c->lpstp, c->cbnk, c->tbnk);
  prt(1,0);


}


void pl_psw (CHAIN* x,uint ix, uint (*prt) (uint,cchar *, ...))
{
  PSW *r ;

    r = (PSW*) x->ptrs[ix];

    prt(0,"pswset ");
    prtaddr(r->jstart,1,prt);
    prtaddr(r->pswop ,0,prt);
    prt(1,0);

}



void prt_chain(uint chn, uint (*prt) (uint,cchar *, ...) )
{
  CHAIN *x;
  uint ix;

// generic chain printer for listing or debug.

  x = get_chain(chn);

  for (ix = 0; ix < x->num; ix++)
   {
     x->chprt(x,ix, prt);
   }
     wnprt(1,0);
}




void pp_hdr (int ofst, cchar *com,  int cnt)
{
  SYM *s;
  int sym;

  if (cnt & P_NOSYM) sym = 0; else sym = 1;
  cnt &= (P_NOSYM-1);         // max count is 255

  if (!cnt) return;

  pstr(1,0);                                         // always start at a new line for hdr
  if (sym)
    {                                              // name allowed
     s = get_sym (ofst,0,7,ofst);                // is there a symbol here (byte address)
     if (s)
      {
       p_pad(3);
       pstr (1,"%s:", s->name);                      // output newline and reset column after symbol
      }
    }

  prtaddr(ofst,0,pstr);
  pstr(0,": ");

  while (--cnt) pstr (0,"%02x,", g_byte(ofst++));
  pstr (0,"%02x", g_byte(ofst++));

  if (com)
   {
    if (gcol > MNEMPOSN) p_pad(gcol+2); else p_pad(MNEMPOSN);
    pstr (0,"%s",com);
    if (gcol > OPNDPOSN) p_pad(gcol+2); else p_pad(OPNDPOSN);
   }

 }


 // print indent line for extra psuedo code lines

void p_indl (void)
{
  if (gcol > PSCEPOSN) pstr(1,0);   //newline if > sce begin col
  p_pad(PSCEPOSN);

}

 // calc type is - 0 none, 1 address, 2 integer,  3 - float

void xplus  (MHLD* h)
{
  // if (!x->calctype) return;
  uint bank;
   if (h->calctype == DP_FLT)  h->total.fval += h->par[0].fval;               // float calc
   else
     {
      bank = g_bank(h->total.ival);         // save bank
      h->total.ival += h->par[0].ival;

      if (h->calctype == DP_ADD)
       {
         h->total.ival &= 0xffff;           // address calc wraps in 16 bits
         h->total.ival |= bank;             // put bank back
       }
     }
}

void xminus (MHLD *h)
{
    // if (!x->calctype) return;

   uint bank;
   if (h->calctype == DP_FLT)  h->total.fval -= h->par[0].fval;               // float calc
   else
     {
      bank = g_bank(h->total.ival);         // save bank
      h->total.ival -= h->par[0].ival;

      if (h->calctype == DP_ADD)
       {
         h->total.ival &= 0xffff;           // address calc wraps in 16 bits
         h->total.ival |= bank;             // put bank back
       }
     }

    }

void xmult  (MHLD *h)   //do these make sense for address calcs ?? should they be shifts ??
{
    // if (!x->calctype) return;

    uint bank;
   if (h->calctype == DP_FLT)  h->total.fval *= h->par[0].fval;               // float calc
   else
     {
      bank = g_bank(h->total.ival);         // save bank
      h->total.ival *= h->par[0].ival;

      if (h->calctype == DP_ADD)
       {
         h->total.ival &= 0xffff;           // address calc wraps in 16 bits
         h->total.ival |= bank;             // put bank back
       }
     }

    }

void xdiv   (MHLD *h)
{
     // if (!x->calctype) return;

   if (h->par[0].ival == 0)
       // set error;
       return;

   uint bank;
   if (h->calctype == DP_FLT)  h->total.fval /= h->par[0].fval;               // float calc
   else
     {
      bank = g_bank(h->total.ival);         // save bank
      h->total.ival /= h->par[0].ival;

      if (h->calctype == DP_ADD)
       {
         h->total.ival &= 0xffff;           // address calc wraps in 16 bits
         h->total.ival |= bank;             // put bank back
       }
     }
  }


void xenc   (MHLD *h)
{

  // Ford encoded address decoder

// h->par[1].ival = enctype
// h->par[2].ival = base register

 int rb,val,off;

  RBT *r;

  val = h->par[0].ival;
  rb = val;
  off = 0;


  // a is in fid..........or maybe not............

  switch (h->par[1].ival)
   {
    case 1:           // 1 and 3 - only if top bit set
    case 3:
     if (! (rb & 0x8000))
       {
        if (valid_reg(val)) val |= g_bank(h->bank);
        h->total.ival = val;
        h->total.dptype = DP_ADD;
        return;              //done, no decode
       }

     off = val & 0xfff;
     rb >>= 12;            // top nibble
     if (h->par[1].ival == 3) rb &= 7;
     rb *=2;
     break;

    case 2:       // 2 and 4 Always decode
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

    r = get_rbase((h->par[2].ival & 0xff) + rb, h->ofst);
    if (r)   val = off + r->val;        // reset op

    else
    {
            #ifdef XDBGX
    DBGPRT(1,"%x No rbase for address decode! %x %x = %x", h->ofst, h->par[2].ival, rb, (h->par[2].ival & 0xff) + rb);

    #endif
 //  }


 // if (valid_reg(val)) val |= g_bank(h->bank);
     // g-bank(a->addr);  ?
   }

// if (val > max_reg()) val |= g_bank(a->data);
   // if (!valid_reg(val)) val -= 0x10000;

 //   ) < 0x400) val = nobank(val) | 0x80000;    //   force bank 8 for regs
    h->total.ival = val;
    h->total.dptype = DP_ADD;
 // return val;
}





//}


void xpwr   (MHLD *h) {

   if (h->calctype >= DP_FLT) h->total.ival = pow(h->total.ival,h->par[0].ival);

else
{ // do an in version ??)
   if (h->calctype == DP_ADD) h->total.ival &= 0xffff;           //address
}

    }
void xroot   (MHLD *){}
void xsqrt  (MHLD *){}
//void xoff   (MHLD *){}
void xvolts  (MHLD *){}
void xbnd   (MHLD *){}







//void xdmy   (MHLD *h)
//{}


void convert_par(MHLD *h, MNUM *p)
{
    //convert parameter types according to calctype

    // if (h->calctype == 1) h->total.ival &= 0xffff;           //address calc wraps in 16 bits
      if (h->calctype < DP_FLT && p->dptype == DP_FLT)  {p->ival = p->fval;  p->dptype = h->calctype; }    //float to int
      if (h->calctype == DP_FLT && p->dptype < DP_FLT)   {p->fval = p->ival;  p->dptype = DP_FLT; }    //int to float

//others ??

}


MHLD* do_tcalc(void *fid, int pix, ADT *a, int ofst)                   //MHLD *h)
  {
   // h->fid is a mathx term, running totals etc all in h
   // ctype is calc type (from name entry at top)

   int i;
   MATHX *m;
   MHLD *h, *s;            //calc holder
   MNUM *p;
   MXF *clc;

// set up calc holder for this level (new one if sub level)

  h = (MHLD*) mem(0,0,sizeof(MHLD)); // zero calc holder
  memset(h,0, sizeof(MHLD));   // mem does not clear alloced memory !!

 // need ofst and original adt block for bank ????

//starts at total = zero.................

 while ((m = get_mterm(fid,pix)))
  {

    fid = m;
     // get params.  if params are subs, call to calculate them
    h->npars = m->npars;
    if (!h->calctype) h->calctype = m->calctype;          // take first term as global calc type
    h->ofst = ofst;                                      //current ofst
    if (!h->bank) h->bank = a->bank;

    for (i = 0; i < h->npars; i++)            // was 3
     {
      p = h->par + i;
      *p = m->fpar[i];                        // transfer fixed pars (MATHX) to variable par in h

      if ( p->dptype == DP_SUB)
          {
           s = do_tcalc(fid,i+1, a,ofst);     // subcalc to do
           *p = s->total;                     // transfer total returned to relevant par
           mfree(s, sizeof(MHLD));            // free subcalc holder
          }

      if  (p->dptype == DP_REF)
      { // this is reference........x1 only at the moment.....uses current field in ADT
         p->dptype = DP_HEX;
         p->ival = g_val (ofst, a->fstart, a->fend);   // get memory value (as int)
      }

      convert_par(h,p);      // so as all are same type for calculation

     }

//    if (m->calc && h->calctype) take away safety for testing
    clc = get_mathfunc(m->func);
    clc->calc(h);
    h->total.dptype = p->dptype;

    //may need to change pfw as well ?????

}


   return h;        //return hold which must be freed
  }






 MHLD *do_adt_calc(ADT *a, uint ofst)
  {
     // initailise and do the calc if there is one
     // return holder so can be called from elswhere

    FKL *l;
  //  MATHN *n;         // DEBUG
 //   MATHX *x;
    MATHN *n;         // calc name (skipped over)
    MHLD *clchdr;   // calc holder

   l = get_link(a, 0, 0);       // fwd link adt to mathx

   // no attached calc, so return plain int value from ADT ofst (default)
   // in calc holder
   if (!l)
     {
      clchdr = (MHLD*) mem(0,0,sizeof(MHLD));     // dummy calc holder
      memset(clchdr,0, sizeof(MHLD));             // and zero it
      clchdr->fid      = a;
      clchdr->calctype   = DP_HEX;                 // plain hex integer
      clchdr->total.ival  = g_val (ofst, a->fstart, a->fend);
      clchdr->total.dptype = DP_HEX;
      return clchdr;
     }


   // calc attached, get and setup details

   n = (MATHN*) l->keydst;                       // this is name

   // calc may reset print mode, also may need to reset pfw....

   clchdr = do_tcalc(n,0,a,ofst);                //x->fid, 0, a, ofst);

   //do tcalc returns an mhld struct

   a->dptype = clchdr->calctype;       // reset print mode

   return clchdr;
  }




int pp_adt (int ofst, ADT *a, uint *pitem)
 {

  // print a SINGLE ITEM from *a, even if a->cnt is set
  // done this way for ARGS printouts.



  int pfw;

  SYM  *sym;
  MHLD *h;

  pfw = 1;                               // min field width
  sym = 0;

  // pfw is still not always right in a struct, especially if names appear.
  // also args always needs min fieldwidths, so allow for a->pfw of zero

  if (pitem) {pfw = plist[*pitem]; (*pitem)++;}          //override preset psw
  else
  {
   if (a->pfw)
    {  // Not set for ARGS, default to preset if too small.
     pfw = get_pfw(a);
     if (a->pfw > pfw) pfw = a->pfw;
    }
  }

  h = do_adt_calc(a, ofst);   // this calc can change print mode

 // print mode & radix.  0 = default (hex) 1 = hex (user), 2 = address (hex+bank) 3 = bin  4 = dec int  5 = dec float (= calc only) (R, X)

    if (a->fnam)      //move this to general at top ???? need rules about when sym printed ???
          {
            sym = get_sym(h->total.ival,a->fstart, a->fend,ofst);         // check for sym (all hexes) this should be a LIST....
            if (sym)  pstr(0,"%*s",-pfw,sym->name);

          }

if (!sym)
{



  switch (a->dptype)
   {
      case DP_ADD :

  //      if (get_numbanks())
          //strip bank off first.....
  //      h->total.ival |= (a->bank << 16);                     // address, add bank  NO, not if single bank !!
  //      else h->total.ival = nobank(h->total.ival);
        // and fall through

          prtaddr(h->total.ival, 0, pstr);
     // OR   void prtaddr(uint addr,uint sp, uint (*prt) (uint,cchar *, ...) )

        break;


      default:          //but no symname find ????

        pstr(0,"%*x", pfw, h->total.ival);                  // default hex  (case 0 and 1
        break;

      case DP_BIN :
        pbin(h->total.ival, a->fend);                        // binary print, need size.
        break;

      case DP_DEC:
        pstr(0,"%*d", pfw, h->total.ival);                    // decimal print, need full width if negative
        break;

      case DP_FLT:

      if (!a->pfd)  a->pfd = 3;                               // TEMP FIX !!
        prtfl(0,h->total.fval,pfw,a->pfd,pstr);               //  float
        break;
   }
}
  mfree(h, sizeof(MHLD));  //free calc holder
  return bytes(a->fend);
}



int pp_lev (void **fid, uint bix, uint ofst, uint *column)
{

  // effectively a compact layout

  // ALWAYS stops at a newline.

// need to add CUTOFF at an end point so it does not overrun
// unless command verifies and fixes size first (better)

//start indent at plist[1] and go up from there.
// in here is Ok as it always does one row

// MUST BECOME RECURSIVE somehow............

 uint i, lx;
 ADT *a;
 void *sfid;      //sub fid

 i = 0;
 sfid = 0;
 lx = bix;

 while ((a = get_adt(*fid,lx)))
  {
   lx = 0;                  //only first item ever has non zero
  // if (i) pstr(0,", ");       // true if looping on ADT

  if (i) { if (bix) pstr(0,"|");  else pstr(0,", "); }  // TEMP

   for (i = 0; i < a->cnt; i++)              // count within each level
    {
     if (i) pstr(0,", ");

     // subfields check
     sfid = a;
     pp_lev (&sfid, 1, ofst, column);       // any sub fields ??
     pp_adt(ofst, a, column);               // field width is plist[column]
     ofst += bytes(a->fend);
    }

   *fid = a;          //done print, move to next fid

   if (a->newl)
    {  //newline flag, stop at this item, reset count for column
      if (column) *column = 1;    // restart column pad markers
      pchar(',');
      break;
    }

  }      // end of while (a)

if (bix && !lx) pstr(0,"|");         //TEMP
  // return size ?
  return 0;
}



//*********************************

uint pp_text (uint ofst, LBK *x)
{
  int cnt;
  short i, v;
  int xofst;

  xofst = x->end;
  cnt = xofst-ofst+1;

  if (cnt > 15) cnt = 15;              // was (15) 16, for copyright notice
  if (cnt < 1)  cnt = 1;

  pp_hdr (ofst, get_cmd_str(x->fcom), cnt);

  xofst = ofst;

  p_pad(CMNTPOSN);
  pstr(0,"\"");
  for (i = cnt; i > 0; i--)
     {
      v = g_byte (xofst++);
      if (v >=0x20 && v <= 0x7f) pchar (v); else  pchar(' ');
     }
  pchar('\"');

  return ofst+cnt;
}

//***********************************
// byte, word print - change name position
//***********************************

uint pp_wdbt (uint ofst, LBK *x)
{
  ADT *a;
  SYM *s;
  // local declares for possible override
  uint com, fend, pfw, val;
  void *fid;

  a = get_adt(x,0);

  fid = x;
  com = x->fcom;

  if (!a)
   {    // no addnl for pp-lev
    fend = 7;                   // set byte default. redundant ??
    if (x->fcom < C_TEXT) fend = (x->fcom *8)-1;   // safe for byte to long
    pfw  = 5;                            // UNS WORD, so that bytes and word values line up
   }
  else
   {
    fend = a->fend;
    pfw   = a->pfw;
   }


  if (((x->end - ofst) +1 ) < bytes(fend))
    {
     // can't be right if it overlaps end, FORCE to byte
     fend = 7;
     com = C_BYTE;
  }

  // no symbol print, do at position 2
  pp_hdr (ofst, get_cmd_str(com), bytes(fend) | P_NOSYM);

  //if (pfw < cnv[cafw[d->defsze])  pfw = cafw[d->defsze]; // not reqd ??

  if (a) pp_lev(&fid, 0, ofst, 0);          //(void **)&x, ofst, 0);       // use addnl block(s)
  else
   {
     // default print HEX
     val = g_val (ofst,0, fend);
     pstr(0,"%*x", pfw, val);
   }

  // add symbol name in source code position
  // this works as ofst only moves by one entry

  s = get_sym(ofst, 0, 7 ,ofst);

  if (s)
    {
     p_pad(PSCEPOSN);
     pstr (0,"%s", s->name);
    }

  val = bytes(fend);
  if (!val) val = 1;       // safety
  ofst += val;

  return ofst;
}

uint pp_dlng (uint ofst, LBK *x)
{
  //for long and triples - NOT DONE YET


  ADT *a;
  SYM *s;
  void *fid;
  //char *tt;
 // DIRS *d;

  // local declares for possible override
  uint com, fend, pfw, val;

  a = get_adt(x,0);

  fid = x;
  com = x->fcom;
 // d = dirs + com;

  if (!a)
   {    // no addnl for pp-lev
    fend = 7;                   // byte, safety.
    if (x->fcom < C_TEXT) fend = (x->fcom *8) -1;   // safe for byte to long
    pfw  = 5;       //get_pfwdef(15);           //cnv[2].pfwdef;                       // UNS WORD, so that bytes and word values line up
   }
  else
   {
    fend = a->fend;
    pfw   = a->pfw;
   }


  if (((x->end - ofst) +1 ) < bytes(fend))
    {
     // can't be right if it overlaps end, FORCE to byte
     fend = 7;
     com = C_BYTE;
  }

  // no symbol print, do at position 2
  pp_hdr (ofst, get_cmd_str(com), bytes(fend) | P_NOSYM);

  //if (pfw < cnv[cafw[d->defsze])  pfw = cafw[d->defsze]; // not reqd ??

  if (a) pp_lev(&fid, 0, ofst, 0);          //(void **)&x, ofst, 0);   // use addnl block(s)
  else
   {
     // default print HEX
     val = g_val (ofst,0, fend);
     pstr(0,"%*x", pfw, val);
   }

  // add symbol name in source code position
  // but this should only be for SINGLE entries ?

  s = get_sym(ofst, 0, 7,ofst);  //(int rw, uint add, uint bitno, uint pc)

  if (s)
    {
     p_pad(PSCEPOSN);
     pstr (0,"%s", s->name);
    }

  val = bytes(fend);
  if (!val) val = 1;       // safety
  ofst += val;

  return ofst;
}




uint pp_vect(uint ofst, LBK *x)
{
  /*************************
  * A9l has param data in its vector list
  * do a find subr to decide what to print
  * ALWAYS have an adnl blk for bank, even if default
  * ***************/

  int val, bsize, bank;
  SYM *s;           //char *n;
  ADT *a;

  s = NULL;
  a = get_adt(vconv(x->start),0);

  bsize = 2;                   // word (for now)

  val = g_word (ofst);          //always word

  if (a) bank = a->bank << 16; else bank = g_bank(x->start);

  val |= bank;               // add bank to value

  if (get_ftype(val) == 1)
    {   // valid code address
      pp_hdr (ofst, get_cmd_str(x->fcom), bsize);
      s = get_sym (val, 0, 7,val);
    }
  else
    {
      if (ofst & 1)  bsize = 1;        // byte, safety
      pp_hdr (ofst, get_cmd_str(bsize), bsize);
    }

  prtaddr(val,0,pstr);

  if (s)
     {
     p_pad(PSCEPOSN);
     pstr (0,"%s", s->name);
     }

  return ofst+bsize;
}

//  print a line as default

uint pp_dmy(uint ofst, LBK *x)
{
 return ofst+1;       // don't print anything.
}


uint pp_dflt (uint ofst, LBK *x)
{
  int i, cnt, sval;       // for big blocks of fill.....
  cchar *s;

  cnt = x->end-ofst+1;

  sval = g_byte(ofst);            // start value for repeats

  for (i = 0; i < cnt; i++)        // count equal values
    {
      if (g_byte(ofst+i) != sval) break;
    }

  s = "???";

  if (sval == 0xff && i >= 6 ) s = get_cmd_str(0);                  // "fill"


  if ((sval == 0xff && i > 12) ||  i > 35)
     {
      if (sval == 0xff) s = get_cmd_str(0);   // allow fill by default if big block

      pstr  (1,0);            // newline first
      pp_hdr (ofst, s, 6);

      p_pad(PSCEPOSN);
      pstr(0, "## ");
      prtaddr (ofst,0, pstr);
      pstr(0," to ");
      prtaddr (ofst+i-1,0, pstr);

      pp_hdr (ofst+i-6, s, 6);

      p_pad(PSCEPOSN);
      pstr(0, "## all bytes = 0x%x", sval);

      return ofst+i;
    }

  if (cnt <= 0) cnt = 1;
  if (cnt > 6 ) cnt = 6;      // 6 max in a line

  if (ofst + cnt > x->end) cnt = x->end - ofst +1;

  pp_hdr (ofst, s, cnt);

  return ofst+cnt;
}



void prt_cmds(uint (*prt) (uint,cchar *, ...))
{
    // command printer for both cmd and auxcmd, debug or real
    // do both chains in one pass in start order

 uint ixc, ixa;
 LBK *cmd, *aux;
 CHAIN *cx, *ax;

 cx = get_chain(CHCMD);
 ax = get_chain(CHAUX);

 ixa = 0;

 aux = (LBK*) ax->ptrs[ixa];           // first aux

 for (ixc = 0; ixc < cx->num; ixc++)
    {
     cmd = (LBK*) cx->ptrs[ixc];

     while (aux && aux->start <= cmd->start)
        {
         ax->chprt(ax,ixa,prt);
       //  prt_cmd(ax,ixa,prt);
         ixa++;
         if (ixa < ax->num) aux = (LBK*) ax->ptrs[ixa];
         else aux = 0;
        }

     cx->chprt(cx,ixc,prt);
   //  prt_cmd(cx,ixc,prt);

    }
    prt(2,0);
}



void pl_sub (CHAIN *c, uint ix, uint (*prt) (uint,cchar *, ...))

{
  SUB *s;
  SYM *x;
  SBK *k;
 uint go;

        s = (SUB*) c->ptrs[ix];
        k = get_scan(CHSCAN,s->start);


        // decide on print
        if (prt == wnprt)
           {
            go = 0;
            if (s->usrcmd)  go = 1;
            if (!go && get_spf(vconv(k->start))) go = 1;         //any spf
            if (!go && get_adt(vconv(k->start),0)) go = 1;
           }
        else go = 1;

        if (go)
          {
           if (prt == wnprt) {prt(0,"sub  ");  prtaddr(s->start,2,prt);}
           else
             {
              prt(0,"sub  [%lx] %x", s, s->start);
              if (s->usrcmd) prt(0," USR");
              if (s->sys) prt(0," SYS");
        //      if (s->emu) prt(0," EMU");
  //            if (s->size) prt(0," ARGS %d",s->size);
       //       if (s->php) prt(0," PHP");
             }
           x = get_sym(s->start,0,7,0);
           if (x)
             {
              prt(0,"  %c%s%c  ", '\"', x->name, '\"');
              if (prt == wnprt) x->noprt = 1;        // printed sym, don't repeat it
             }

           prt_sglo(k, prt);
           prt_adt(vconv(k->start),0,prt);
           if (s->usrcmd)  prt(0," #usr");
           prt(1,0);
          }

  }




void pl_sym (CHAIN *x, uint ix, uint (*prt) (uint,cchar *, ...))
{
 SYM *t;
 uint dbg;

  t = (SYM*) x->ptrs[ix];

  if (prt !=wnprt) dbg =1; else dbg = 0;

   // decide on print
   if (!dbg || !t->noprt)
     {
      if (prt == wnprt)
         {
          prt(0,"sym ");
          prtaddr(t->addr,1,prt);
          if (t->rstart)
           {
            prtaddr(t->rstart,1,prt);
            prtaddr(t->rend,0,prt);
           }
         }
      else
        {         // debug
         prt(0," sym %x ", t->addr);
         prt (0," %5x %5x ", t->rstart, t->rend);
         prt(0, " (%d)", t->symsize);
         prt(0," mask=%x",t->fmask);
        }

     prt(0," \"%s\"",t->name);

     // if (!t->read) prt(0," W");         //temp
     if (t->fend & 64) prt(0, " W");

    // if (t->prtadl || !t->whole || dbg)
    if (!t->whole || dbg)
        {
          prt(0," [");
    //      if (!t->whole || dbg)
           {
            prt(0,"B %d" , t->fstart);
            if (t->fend != t->fstart || dbg) prt(0, " %d", t->fend);
           }


    //      if (t->names) prt(0," N");
          if (t->immok) prt(0," I");
          prt (0,"]");
        }
     if (t->sys) prt(0,"               #Auto Added by SAD");
     prt(1,0);

 }

}


void prt_dirs()
{                 // print all directives for possible re-use to msg file
  prt_opts ();
  prt_layout();
  prt_banks(wnprt);
  prt_chain (CHBASE, wnprt);                //rbt  (wnprt);
  prt_calcs(wnprt);
  prt_chain (CHPSW, wnprt);           //psw  ();
  prt_scans();
  prt_cmds (wnprt);


  wnprt(1,"# ------------ Subroutine list----------");
  prt_chain(CHSUBR, wnprt);          //subs (wnprt);

  wnprt(1,"# ------------ Symbol list ----");
  prt_chain (CHSYM, wnprt);           // prt_syms (wnprt);


}

//operand prints

void p_sc (INST *x, int ix)
{
// single op print for source code for supplied instance index
// note that cannot use same print as bits, bitno and reg stored
// in separate ops


  int v;
  OPS *o;
  SYM *s;
  o = x->opr + ix;

 // ->bit  set ONLY by bit jump, and always one bit.

  if (o->sym)
   {      // if symbol was set/found earlier
     if (o->bit)   s = get_sym (o->addr, o->val, o->val,x->ofst);   // to get bit
     else          s = get_sym (o->addr, 0, o->fend ,x->ofst);

     if (s)
      {
        if (o->off) pchar('+');                    // if offset with symname

        // if name not a bit name, add Bx_
        if (o->bit && (s->fstart != o->val || s->fend != o->val))  pstr (0,"B%d_", o->val);
        pstr(0,"%s", s->name);
   //   p_sym(s);
        return;
      }
    }

 if (o->imm)                               // immediate operand
    {
     pstr(0,"%x", o->val & get_sizemask(o->fend));
     return;
    }

 if (o->rgf)
    {
     if (valid_reg(o->addr) == 2 && (o->fend & 64) == 0) pstr (0,"0");           //zero reg (read only)
     else      pstr (0,"R%x", nobank(o->addr));

     if (o->inc) pstr (0,"++");
     return;
    }

 if (o->bit)
     {            //bit but name not found.............
       pstr (0,"B%d_", o->val);
       return;
     }

 if (o->adr)
    {
     prtaddr(o->addr,0,pstr);
     return;
    }

if (o->off)
    {            //offset address - signed
     v = o->addr;
      //     pchar(0x20);
     if (v < 0)
     {
      v = -v;
      pchar('-');
     }
     else pchar('+');
     //     pchar(0x20);
     // may need more smarts here
     // if single bank or < register, otherwise add bank
     if (!get_numbanks() || nobank (v) < 0x400) v &= get_sizemask(15);
     else   v |= ((x->scan->datbnk-1)<< 16);    // put bank in
     pstr (0,"%x", v);
     return;
    }
}







void p_op (OPS *o)
{
 // bare single operand - print type by flag markers

 if (o->imm) pstr (0,"%x", o->val & get_sizemask(o->fend));     // immediate (with size)
 if (o->rgf) pstr (0,"R%x",nobank(o->addr));                    // register
 if (o->inc) pstr (0,"++");                                     // autoincrement
 if (o->bit) pstr (0,"B%d",o->val);                             // bit value
 if (o->adr) prtaddr(o->addr,0,pstr);                           // address - jumps
 if (o->off) pstr (0,"+%x", o->addr & get_sizemask(o->fend));   // address, as raw offset (indexed)

}


void p_oper (INST *c, int ix)
{
  // bare op print by instance and index - top level
  // if indexed op. append "+op[0]" with op[1]

  OPS *o;

  ix &= 0x3;                      // safety check

  o = c->opr+ix;

  if (o->bkt)  pchar ('[');
  p_op (o);
  if (o->inx)  p_op (c->opr);
  if (o->bkt)  pchar (']');
  return;
}



void fix_sym_names(cchar **, INST *);




void p_opsc (INST *x, int ix)
{
 // source code printout - include sign and size markers.
 // ws = if set, use write size (equals not printed yet)
 // ix+0x10 flag specifies extra sizes

  OPS *o;
  int i;
   if (!x)
    {
       pchar('0');
       return;
    }

   if (ix >= 0x11 && ix <= 0x13)
      {                 // extra size/sign requested
       i = ix & 3;      // safety
   //    if (i == 1 && x->opcsub > 1)  i = 0;
       if (x->opr[i].sym) i = 0;
       if (!x->opr[i].rgf) i = 0;

       if (i)
         {
           i = x->opr[i].fend;
           if (i & 32)  pstr(0,"s");
           i &= 31;
           if (i < 8)  pstr(0,"y");
           else if (i < 16)  pstr(0,"w");
           else if (i < 24)  pstr(0,"t");
           //if (i == 4)
           else pstr(0,"l");
         }
      }

  ix &= 3;
  o = x->opr + ix;                            // set operand

  //    if (x->opr[ix].fend & 64) pstr(0,"~");     // TEMP for write !!

  /* add these back later...........
  // check if index is to a register
  if (ix == 1 && x->opcsub == 3 && nobank(o->addr) <= max_reg() && !o->rgf)
      {     // present as register - seems to work OK
       o->bkt = 0;
       o->rgf = 1;
       p_sc (x, ix);
       return;
      }

// push check, turn immediate into address printout
//probably need better check here, to see if 'codebank(addr)' is a code command

  if (o->imd && x->sigix == 14)
        {  // immediate - assume current code bank for printout
           // if valid address, and change print to address
          o->addr = codebank(o->addr,x);
          if (val_rom_addr(o->addr))
            {
             o->imd = 0;
             o->adr = 1;
             o->sym = 1;       //look for symname too
            }
        }        */

  if (o->bkt) pchar ('[');
  p_sc (x, ix);
  if (o->inx) p_sc (x, 0);  // indexed, extra op
  if (o->bkt)  pchar (']');
  return;
}
















//and comment stuff below here





void cmtwrapcheck(char *c)
{
  char *t;
  uint col;
  // check if next 'word' would wrap after a space

  if (*c != ' ' && *c != ',') return; // not at space

  t = c+1;         // next char
  while (*t)
   {
     if (*t == ' ' || *t == ',' || *t == '\n' || *t == '\r' ) break;
     t++;
   }

// but this could be end of line and still overlap....
  if (!(*t)) return;         // no space
  col = t - c;
  if (col > 40) return;      // safety ....
  col += gcol;   // where next 'word' would end

  if (col >= WRAPPOSN)
    {
     wrap();
    }
}



uint calc_pfw(MHLD *h, ADT *a, uint cptl)
{
// calc actual field width for aligned structs
 //cptl from parent LBK command
  int val ;
  uint ans, neg, i;
uint *z;


  if (h->total.dptype < DP_FLT) val = h->total.ival;
  else val = h->total.fval;
  neg = 0;

  switch(h->total.dptype )
   {

      default:
        ans = a->pfw;
        z = 0;
        break;

      case DP_BIN:
        z = binpfw;
        break;

      case DP_ADD:
       z = 0;
       if (get_numbanks()) ans = 5;   // with bank
       else ans = 4;
       break;

      case DP_HEX:
        z = hexpfw;
        break;

      case DP_DEC:
        z = decpfw;
       // fall through
      case DP_FLT:
//need extra here probably for decimal points ??

      if (val < 0) {val = -val; neg = 1;}
      break;
   }

//fieldwidth...

  if (z)
  {
      for (i = 1; i < z[0]; i++)
      {
        if (val < (int) z[i]) break;
      }

      ans = i;
      if (neg) ans ++;

   }
    // if not compact, use wider default settings
    if (!cptl && ans < a->pfw) ans = a->pfw;

    return ans;

}




void calchdrsize(LBK *x)
{
  // calc max hdrsize (in bytes) of a data struct in x and place in pitem[0]
  // allows for newlines. Bytes only for header - item widths next subr

   ADT  *a;
   uint sz;
   void *fid;

   memset(&plist,0,sizeof(plist)); // zero the whole thing

   plist[0] = 0;
   if (x->argl) return;           // ignore if arg list.

   sz = 0;
   fid = vconv(x->start);

   while ((a = get_adt(fid,0)))
     {
       sz += cellsize(a);    // not in any subs, only main string
       if (a->newl)
        {  // keep size, but restart for next line
         if (sz > plist[0])  plist[0] = sz;
         sz = 0;
        }
     fid = a;
   }


 if (sz > plist[0])  plist[0] = sz;     // for last (or only) segment
 if (sz == 0)
 {               //emergency fix for loop
 #ifdef XBDGX
     DBGPRT(0,"SIZE ZERO !!!!");
     DBGLBK(0,x,0);
     #endif
     sz = 2;
 }

  // now add up for pad position.
  // ADD a comment column here ?

 plist[0] = (plist[0]*3) + 7;      //first pad posn, allow for addr
 if (get_numbanks()) plist[0]++;         // for exta address digit

 if (plist[0] < MNEMPOSN) plist[0] = MNEMPOSN;   // minimum at mnemonic col

}






void maxcols(void *fid, uint bix, uint cptl, uint *col, uint *ofst)
{
 ADT  *a;
 SYM  *sym;
 MHLD *h;

 uint i,j,z;

  z = bix;
  while ((a = get_adt(fid,z)))
     {   // work out data column widths - sym or calc or default by size
       z = 0;                              // only first ADT ever has a one
       for (i = 0; i < a->cnt; i++)          // count within each level
        {

         maxcols(a, 1, cptl,col,ofst);             // GOES HERE..before parent cell

         h =  do_adt_calc(a,*ofst);          // value after calc

         sym = 0;
         if (a->fnam && h->total.dptype < DP_FLT)               //== DPADD)
           {
             sym = get_sym(h->total.ival,0, 7,*ofst);        // READ sym AFTER any decodes - may be only addresses ????
           }

         if (sym)    j = sym->symsize;              // sym name length
         else        j = calc_pfw(h,a, cptl);    // data or default

         if (j > plist[*col]) plist[*col] = j;      // keep largest width

         (*col)++;
        if (!bix) *ofst += bytes(a->fend);
        }

       if (!bix && !a->cnt) (*ofst)++;                         // safety to stop infinite loop
       if (a->newl) *col = 1;                       // restart items if newline
       fid = a;
     }            //end while a

}






int maxcolsizes(LBK *x)
{
 // calc max column sizes of actual data for each line
 // used for all structs

// ADT  *a;
// SYM  *sym;
 //MHLD *h;

 uint i, ofst, end, item;
 void *fid;

 ofst = x->start;
 memset(&plist,0,sizeof(plist)); // zero the whole thing

 if (x->argl) return 0;                     //ouch.....
 end = x->end - x->term ;  // looks OK

 calchdrsize(x);         // in plist[0],  bytes to print.

 while (ofst < end)
   {
    item = 1;
    fid = vconv(x->start);
    maxcols(fid,0,x->cptl,&item, &ofst);
   }

  // Now add up col widths for comment pad position.

 for (i = 0; i <= item; i++)
   {
     plist[31] += plist[i]+2;           // space and comma for each item
   }

 plist[31] += 5;                        // and add cmnd string

 return item;
}




void parse_cmnt(CPS *c, uint *flags)
{
  // process any special chars in a comment line
  // use CPS struct here for getpx and getpd etc.

  int chars, ans;
  uint col;
  char *t;
  SYM *x;

   if (!c->cmpos) return;
   if (*c->cmpos == '\0') return;        // safety

   if (*c->cmpos != '\\' ) p_pad(CMNTPOSN);  //no special chars, tab to cmnt position

   while (c->cmpos < c->cmend)
    {
     if ((*flags) & 4) break;

     switch (*c->cmpos)
       {   // top level char check

         default:
           pchar(*c->cmpos);                     // print anything else - auto wraps
           cmtwrapcheck(c->cmpos);               // wrap check for whole word.
           c->cmpos++;
           break;

         case 1:                          //  for autocomment embedded - NOTE not chars !!
         case 2:
         case 3:
           p_opsc (c->minst,*c->cmpos);
           break;

         case '\n' :                      // '\n',  ignore it
         case '\r' :
         c->cmpos++;
           break;


         case '\\' :                // special sequences
           c->cmpos++;                     // skip the backslash, process next char.

     //      rs = toupper(*c->cmpos);
           switch (toupper(*c->cmpos))
             {  // special sequences

               default:
                 pchar(*c->cmpos);             // print anything else
                 break;

// more opcode/operand options ??

               case 0x30:                  //allow zero - NOT TESTED !!
               case 0x31:
               case 0x32:                 // for user embedded operands 1-3
               case 0x33:

                 ans = *c->cmpos - 0x30;   // char to number
                 if (ans >= 0 && ans <= 3)
                   {
                    p_opsc(c->minst,ans);
                   }
                   c->cmpos++;
                 break;

               case 'N' :
                 pstr(1,0);
                 c->cmpos++;
                 break;

               case 'S' :                                // 'S' symname with no padding

                 col = gcol;                             // where print is now.

                 c->cmpos++;            //skip the 's'

                 x = NULL;
                 t = c->cmpos;         // remember where
                 c->p[7] = 0;           //default read

                 if (toupper(*c->cmpos) == 'W') {c->p[7] |= 64; c->cmpos++;}  // write sym

                 //allow other letters, e.g. no default Bx_Ry

                 ans = getpx(c,1,1);
                 if (fix_input_addr_bank(c,1)) ans = 0;
                 if (ans > 0)
                   {
                    if (*c->cmpos == ':')
                     {
                      c->cmpos++;                         //skip the colon
                      ans = getpx(c,2,2);                 //bit number
                      if (c->p[2] > 31) c->p[2] = 31;
                      if (c->p[3] > 31) c->p[3] = 31;
                      if (ans < 2) c->p[3] = c->p[2];        //one number, end = start
                      //else
                      //if (c->p[3] > 31) c->p[3] = 32;     //  to fix for start and end
                     }
                    else
                    {
                     c->p[2] = 0;             //no bits, default to byte
                     c->p[3] = 7;
                    }
                    c->p[3] |= c->p[7];                       // write flag

                    c->p[4] = 0;
                    if (*c->cmpos == '@')
                     { //range address
                       ans = getpx(c,4,1);                       //range
                       if (!ans) c->p[4] = 0;
                     }

                    if (!c->p[4])
                      {
                       if (c->minst) c->p[4] = c->minst->ofst;    //get range ofst from code
                       else c->p[4] = c->p[0];       //address of the comment
                      }

                    x =  get_sym (c->p[1], c->p[2], c->p[3], c->p[4]);

           //    should be     x =  get_sym (addr, sbit, ebit, ofst);

                 if (x)       // and need extra check somehow............to stop B2...but B0 won't work here
                     {
                      if (c->pf[2] && x->whole) pstr(0,"B%d_",c->p[2]);   //   not right for WORDS ?
                      pstr(0,"%s",x->name);
                  //    p_sym(x);
                      if (col > gcol) p_pad(col);           // pad to col (from above)
                     }
                   }         //end of first number read in

                 if (!x)
                   {        //no sym or addr incorrect

                     pchar('?');  // failed sym read, print ??? and sequence ??for Bx_Rn dsiplay
                     while (t < c->cmpos) pchar(*t++);
                     pchar('?');
                   }  //


//if field read, print ??

              //   c->cmpos--;         //= c+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'T' :
         //      case 't' :
                                // tab to 'x'
                 c->cmpos++;            //next char
                 chars = 0;

                 ans = getpd(c,1,1);         //sscanf(c,"%u%n",&addr,&chars);
                 if (ans > 0) { if (c->p[1] < (int) WRAPPOSN) p_pad(c->p[1]); else wrap();}

          //       c->cmpos = c->cmpos+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'R' :        // runout
                 c->cmpos++;
                 chars = 0;
                 c->p[2] = ' ';     //space is default

                 if (!isdigit(*c->cmpos))
                   {
                    c->p[2] = *c->cmpos;   //new runout char
                    c->cmpos++;
                   }
                 ans = getpd(c,1,1); //ans = sscanf(c,"%u%n",&addr,&chars);
                 if (ans <= 0) c->p[1] = 1;
                 p_run(0, c->p[2], c->p[1]);
             //    c->cmpos = c->cmpos+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'M' :      //next tab posn
           //      chars = gcol;       //current col
                 chars = gcol/4 * 4;     //nearest prev tab
                 chars += 4;
                 p_pad(chars);
                 c->cmpos++;
                 break;


               case '\n' :                               // '\n',  ignore it
               case '\r' :
               c->cmpos++;
                 break;


              case 'W' :     // wrap comment, or pad to comment column
                 if (lastpad >= CMNTPOSN) wrap();
                 else p_pad(CMNTPOSN);
                 c->cmpos++;
                 break;

              case '|'  :        // multiple comment lines ? this seems to work............
                 if ((*flags) & 2) (*flags) |= 4;
                 c->cmpos++;
                 break;

             }   // end switch for '\' sequences



// case '[' :
// new sequences  - swapped to command reader






       }        // end switch for char
    // if (*c->cmpos) c->cmpos++;
    }

// save char position if in opcode,  else clear string

 // if ((flags & 2) && *c->cmpos)  m->ctext = c;             ??
  //else *c->cmpos = '\0';    //m->ctext = NULL;         // clear string for next cmt
}


int kill_cmnt(CPS *c, int ans)
{
    //ans = 1 is 'get next comment line', 0 is kill forever.

    if (!ans) c->p[0] = 0xfffff;    // max possible address with max bank - finishes ALL comment print
    cmbuf[0] = '\0';                // safety
    c->cmpos = c->cmend;            // this comment used, get next one.
    return ans;
}





int get_cmnt (CPS *c, uint *flags)
{
  int ans;
  char *t;
  HDATA *fl;


  if ((*flags) & 4)
  {                  // '|'  detected. Stop until next pp-comment
    (*flags) &= 3;   // clear the flag
    return 0;
  }

  if (c->cmpos < c->cmend) return 0;    // more comment to do

  fl = get_fldata();

  ans = 0;

  c->cmpos = cmbuf;

  if (fl->fh[4] == NULL || feof(fl->fh[4])) return kill_cmnt(c,0);  // none, or end of cmt file

  memset(c,0,sizeof(CPS));   //clear the command structure  before any new read.

  while (ans < 1 && fgets (cmbuf, 255, fl->fh[4]))
    {
       cmbuf[255] = '\0';                  // safety

       t = strchr(cmbuf, '\n');            // check for LF
       if (t) *t = '\0';                   // shorten line

       t = strchr(cmbuf, '\r');            // check for CR
       if (t)   *t = '\0';        //c

       t = cmbuf;
       while (*t == ' ') t++;              // skip leading spaces
       if (*t == '#') *t = '\0';           // line commented out (a first #)
       if (*t == '/' && *(t+1) == '/') *t = '\0';    // and a '//' is valid as per 'C'

       ans = strlen(t);
       c->cmend = t + ans;                // end of line
       c->cmpos = t;                      // start of string
    }

  if (ans <= 0) return kill_cmnt(c,1);  // invalid, get next line

  ans = getpx(c,0,1);

  if (fix_input_addr_bank(c,0)) return kill_cmnt(c,1);

  readpunc(c);
  c->argl = 0;
  while (*c->cmpos == 0x20) c->cmpos++;     // consume any spaces after number
  t = c->cmpos;
  if (*t == '\\' && *(t+1) == 'n') c->argl = 1;    // newline at front of cmt

  return 1;

}



void pp_comment (uint ofst, uint flags)
{
  // ofst is the START of an opcode statement so comments
  // less than ofst should be printed.

 // if (s->proctype) return;        // safety check


 // if (flags&1) set, then add a newline.
 // if (flags&2) set, stop on a '|' char (split comment for bit names)
 // BUT newline should be AFTER trailing comments (i.e. not beginning with newline)
 // but BEFORE ones that do. Therefore, if end set, break at first cmnt with newline at front
 // and remember if newline printed so multiple 'end' set doesn't cause multiple newlines
 // clear flag if end not set

   while ((int) ofst > pcmd.p[0])
       {
        if ((flags & 1) && pcmd.argl) break;       // do on NEXT call
        parse_cmnt(&pcmd,&flags);                  // print it
        if (!get_cmnt(&pcmd, &flags)) break;       // get next comment
       }

   if (flags & 1)
      {                 //print newline
       if (!lastcmt)
         {
           pstr(1,0);
           lastcmt = 1;    // remember
         }
       }
   else lastcmt = 0;      //clear if not end
}












int pp_cmpl(uint ofst, void *fid, int fcom)
{
  // print 'compact' layout, all args on one line
  // unless | char, when it's split

  uint tsize, size, end, itemno;

  tsize = adn_totsize(fid);
  end = ofst + tsize;

// this is stct format, need a different one for subr (with args afterwards)

 itemno = 1;

 while (ofst < end)
  {      //for newline split............
   size = adn_listsize(fid);

  if (fcom != C_ARGS)
   {
    pp_hdr (ofst,0,size);        // no title
    p_pad(plist[0]);
    pstr (0,"%s", get_cmd_str(fcom));
    if (gcol < OPNDPOSN) p_pad(OPNDPOSN); else p_pad(gcol+2);
    pp_lev(&fid,0, ofst, &itemno);    // continue from newline

    ofst += size;
    if (gcol >= CMNTPOSN) p_pad(plist[31]);
    if (ofst < end) pp_comment (ofst,0);    // NOT last item
   }
  else
   { // arguments for subr on same line, no split
    pp_lev(&fid,0 ,ofst, 0);
    ofst += size;
   }
  }

 if (!tsize) return 1;

 return tsize;
}



int pp_argl (uint ofst, void *fid, int fcom, int *argno)
{

// Print 'argument' layout, 1 arg per line
// can get SBK or LBK as fid

int i;
uint tend;
ADT *a;

 pp_comment (ofst,0);            //for any comments BEFORE args

 tend = ofst + adn_totsize(fid)-1;  // where this 'row' ends

 while ((a = get_adt(fid,0)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     if (fcom == C_ARGS) sprintf(ag, "      #%s %d", "arg", (*argno)++);
     else                sprintf(ag, "      #%s %d", get_cmd_str(fcom), (*argno)++);

     pp_hdr(ofst, ag, bytes(a->fend));
     p_pad(PSCEPOSN+3);
     a->pfw = 0;                         // no print field in argl
     pp_adt(ofst, a,0);

     ofst += bytes(a->fend);
     if (!bytes(a->fend)) ofst++;     // safety

     if (ofst <= tend)
       {
         pchar(',');
         pp_comment (ofst,0);
       }
    }

   fid = a;
   if (a->newl) break;
  }

  return ofst;
}



int pp_subargs (INST *c, int ofst)
{
  // ofst is effectively nextaddr

  // change pp-lev for pp_args to do
 // a line by line print of attached arguments

 // SUB  *xsub;
  SBK *sc;
  LBK  *k;
  int size, xofst, argno, postprt;


  size = 0;
  argno = 1;
  postprt = 0;

  if (get_cmdopt(OPTSSRC))   pstr(0," (");   // args open bracket

  xofst = ofst;

  while(1)           //look for args first.........in a loop
   {
    k = get_cmd(CHAUX,xofst, C_ARGS);

    if (!k || !k->size) break;
    if (get_cmdopt(OPTSSRC))
      {
       if (xofst > ofst)   pchar(',');
       if (k->size)
       {
        if (k->cptl)
           {
            pp_cmpl(xofst,vconv(k->start), k->fcom);
            postprt = 1;
           }
        else  pp_argl(xofst, vconv(k->start), k->fcom, &argno);
       }
      }
    xofst += k->size;                     // look for next args
    size += k->size;
    }

  if (!size)
    {
     // didn't find any ARGS commands - look for subr arguments

     sc = get_scan(CHSCAN,c->opr[1].addr);          //            xsub = get_subr(c->opr[1].addr);

     if (sc)                      //xsub)
      {
       size = sc->adtsize;                    //xsub->size;
       if (get_cmdopt(OPTSSRC) && size)
         {
          if (sc->adtcptl)                        //xsub->cptl)
             {
              pp_cmpl(xofst,vconv(sc->start), C_ARGS);          //xsub, C_ARGS);
              postprt = 1;
             }
          else pp_argl(xofst, vconv(sc->start), C_ARGS, &argno );            //xsub, C_ARGS, &argno);
         }
      }
    }

  if (get_cmdopt(OPTSSRC))
     {
      if (size) pchar(' '); // add space if args
      pstr(0,");");       // args close bracket

     }

  if (size)
    {
     pp_comment (ofst,0);                               // do any comment before next args print
     if (postprt) pp_hdr (ofst, "#args ", size);        // do args as sep line if compact
    }

  return size;
}
// print subroutine params - simpler version
//func, table, struct go here

uint pp_stct (uint ofst, LBK *x)
{
  uint size, end;
  int argno;

  end =  ofst + x->size;
  argno = 1;                               // can't reset argno !!

  if (end == ofst) end++;     // safety

  if (ofst == x->start) maxcolsizes(x);  // do only once.

  size = x->end - x->term + 1;    // where terminator starts

  if (x->term && end > size)
    {      // print terminator
     pp_hdr (ofst, 0, x->term);
     p_pad(OPNDPOSN);
     pstr(0,"## terminator");
     ofst += x->term;
     return ofst;
    }

  while (ofst < end)
    {
      if (x->argl)
        {
         ofst = pp_argl(ofst,vconv(x->start),x->fcom,&argno);
         pp_comment (ofst, 1);                  // last item
         pstr(1,0);
        }
      else
        {
         ofst += pp_cmpl(ofst,vconv(x->start),x->fcom);
         if (gcol >= CMNTPOSN) p_pad(plist[31]);    //before comments.....
        }
    }

 return ofst;
}


/*
void prt_def_fld(uint pmask, SYM **list, uint addr, uint vmask, uint shifts)
{
  // print default feilds as "Bx_name"

  uint bit, tmk;
  SYM *s;
         if (pmask & 0xff)
          {    // unresolved bits in pmask

            if (list[1]) s = list[1];
            else
            if (list[0]) s = list[0];
            else s = 0;

            for (bit = 0; bit < 8; bit++)
             {
               tmk = 1 << bit;
               if (tmk & pmask)
                 {
                  p_indl();
                  pstr(0,"B%d_", bit + (shifts*8));
                  if (s) pstr(0,s->name); else pstr(0,"R%x",addr);
                  pstr(0," = %x;", (vmask & tmk) >> bit);

           //       pmask -= d->fmask;           //and remove bit for tracking
                 }
             }
          }
}
*/







void bitwise_replace(cchar **tx, INST *c)

// Now works for fields wider than one bit, adjusting val as well as print maks
//how to switch on global settings.... ???

{

  uint sig;
  uint pmask, vmask, addr, shf;
  int i;
  SYM **list;
  OPS  *o;
  SYM *d;

  sig = c->sigix;                 // opcode sigix

  // only continue if either imd instruction or clr

  if (c->opcsub != 1 && sig != 1) return;      // only if an immediate or clr opcode.
  if (c->opr[4].rbs) return;                   // NOT if a shuffled out rbs operand (why?)

  if (sig == 9 && !c->opr[1].val)
    {                                          // OR, XOR with zero mask - a timewaster ?
      c->opr[1] = c->opr[2];                   // make operands equal
      *tx = scespec[2] - 1;                    // add a "redundant" comment ??
      return;
    }

  vmask = c->opr[1].val;                       // value of read for bits
  o = c->opr + 3;                              // o is write op, [1] is read op

  if (sig == 12)
    {                                            // ldb, ldw, etc
      pmask = fmask(0,c->opr[1].fend);           // for all bits in load
    }
  else  pmask = vmask;                           // only for set bits

  if (sig == 5 || sig == 1)
    {                                            // AND and CLR . Reverse print mask if not a 3 op
        pmask = ~pmask;
    }

  list = get_syms(o->addr, 0, o->fend, c->ofst);  // size via o->fend (write) ....

  // if (sig = 12 && (!list[3])) return;              // at least 2 bit field symbols ?? v4 was 2 minimum

  //next bit only works if at least one field name found....may need more for single bits ??
  // v4 prints single bits as Bn_Rx ....

 if (list[2])                                    // if more than default matches */
   {
     addr = o->addr;

     // need to pick out bit fields versus defaults.............
     // we KNOW that syms occur in correct bit order so can sort this out....

     i = 2;
     shf = 0;
     while (i < 65 && pmask)
      {
       d = list[i];
       if (!d) break;         // end of list

       if (d->addr > addr)
        {                   //  shuffle up to next bit set
          // if (?)    prt_def_fld(pmask, list, addr, vmask, shf);       //prints out of order ?? global option "all names" ?
          addr = d->addr;
          pmask >>= 8;
          vmask >>= 8;
          shf++;
        }

       if (d->fmask & pmask)
        {
          p_indl();
          pstr(0,"%s = %x;", d->name, (vmask & d->fmask) >> d->fstart);
          pmask ^= d->fmask;           //and remove bit(s) for printing
        }

       i++;
      }
    *tx = 0;
   }

 }



void shuffle_ops(cchar **tx, INST *c, int ix)
{
  // shuffle ops down by one [1]=[2],[2]=[3] acc. ix
  // change pseudo code to ldx string "[2] = [1]"

  c->opr[4] = c->opr[ix];  // save original
  while (ix < 3)
    {
     c->opr[ix] = c->opr[ix+1];
     ix++;
    }
  if (tx) *tx = scespec[2];   //opctbl[LDWIX].sce;
}

int find_last_psw(INST *c, INST *out)
 {
   // finds last PSW changing opcode for printout, always in print phase
   // c is current place/opcode, out is found instance
    int ans, cnt,chkclc;
    uint ofst;
    const OPC *opl;
    PSW *p;


    ans = 0;
    cnt = 0;
    ofst = c->ofst;   //from current instance

    // is a psw setter defined by user ??
    p = get_psw(c->ofst);
    if (p)
       {
          ans = 1;            // found !!
          ofst = p->pswop;
       }

    /// ignore clc,stc as psw setters unless JLEU, JGTU, JC, JNC
    chkclc = 0;
    if (c->opcix > 61 && c->opcix < 64) chkclc = 1;  // JLEU/JGTU
    if (c->opcix > 55 && c->opcix < 58) chkclc = 1;   // JC/JNC

    // This is used by carry and ovf as well

    // what about a popp ??

    while (!ans)
     {
      ofst = find_opcode(0, ofst, &opl);

      if (cnt > 32) break;
      if (!ofst)    break;

      if (opl->sigix == 17) break;

      /* {
        // CALL - find jump and go to end of subr
        JMP *j;
        SUB *sub;

        j = get_fjump(ofst);

        if (j && j->jtype == J_SUB)
           {
            sub = get_subr(j->toaddr);
      //      if (sub)  ofst = sub->end; else break;
           }
        else break;
       }  */

   if (opl->sigix == 20)  break;      // STOP if ret

 /*  if (opl->sigix == 6)
   {

    do_one_opcode(ofst);
     if (sinst.opr[1].addr == 0)
     {
       ofst = find_opcode(0, ofst, &opl);    //go back another opcode
        if (!ofst)    break;

// can't go back through rets, or stat jumps ??
     }
   }  */




      if (opl->pswc)
        {    // FOUND PSW setter, if set/clear carry, check flag.
           if (opl->sigix == 21)
            {
             if (chkclc) ans = 2;
            }
         else
            {
             ans = 1;
            }
        }
        cnt++;
     }

 if (ans)
       {
        do_one_opcode(ofst, &xscan, &psinst);
 //       if (syms)
        fix_sym_names(0,out);
       }
   else
      {
       clr_inst(&psinst);
      }

    return ans;
 }


void prt_ovf(INST *c, cchar **tx)          //, int ainx)
{

// Replace if (OVF) with various sources
// possible opcodes add, shift L, cmp, inc, dec, neg, sub, div.

 //  int ans;
 //  const OPC *opl;

   pstr (0,"if (");
   // get last psw changer

return;            //temp ignore extras

/* old one
   ans = find_last_psw(c,1);  // returns 1 if found...op in psinst
   if (!ans) return;          // not found

   opl = opctbl + psinst.opcix;              // mapped to found inst


   if (opl->sigix == 7)
        {
          // subtract. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];
          p_opsc(&psinst,opl->wop,0);
          pstr(" %s ",*tx);
          p_opsc(0,1,0);
          pstr(") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


  if (opl->sigix == 10)
        {
          // compare. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];

          if (!nobank(psinst.opr[2].addr))
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(&psinst,1,0);                        // operand 1 of compare
              pstr(" %s ", swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(&psinst,2,0);                        // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(&psinst,2,0);
              pstr(" %s ",*tx);
              p_opsc(&psinst, 1,0);
            }
          pstr(") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 6  || opl->sigix == 25)
        {       //  add or inc STANDARD CARRY -  have wop ?
          p_opsc(&psinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<=");
          pstr(" %x) ", camk[psinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


   if (opl->sigix == 3)
        {       //  shl STANDARD CARRY all have wop ?
          // need to find out how many shifts
          // psinst.opr[1].val is shift amount.  wop is shifted value cant do it for other than imm (opr[1].imd)

          if (!psinst.opr[1].imd)
            {
             clr_inst(&psinst);       // can't do variables
             return;
            }

          //shl

          ans = casz[psinst.opr[opl->wop].wsize] * 8;          // number of bits
          ans -= psinst.opr[1].val;                            //shift from MSig

         // dl shifts ??

         if (ans > 16)
          {
            ans -= 16;
            psinst.opr[opl->wop].addr+=2;
          }

          //fiddle as a bit
          pstr("B%d_", ans);
          p_opsc(&psinst,opl->wop,0);
          pstr(" = %d) ", ainx & 1);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 11 || opl->sigix == 8)
        {       //  mult,div STANDARD OVERFLOW all have wop ?
          p_opsc(&psinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<=" );
          pstr(" %x) ", camk[psinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


     if (opl->sigix == 24)
      {       // dec
          p_opsc(&psinst,opl->wop,0);
          pstr(" %s 0) ", (ainx & 1) ?  ">=" : "<" );
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
      }
*/
}

void prt_cond_jmp(INST *c, cchar **tx)          //, int ainx)
{
   // Conditional ops replacements.
   // last is ZERO and lwop < 0 if no candidate found.
   // lwop is zero ONLY for R0 as wop

   // note that tx is ALREADY SWOPPED for "if{}" or "goto" NOT ANY MORE.....driven by j->swl now

   const OPC *opl;
   cchar *t, *s, *x;
   int ans, ix;
   JMP *j;


   j = get_fjump(c->ofst);

  // if (!j) return ;              //too strict. default to opcix ??

  // if (j->swl) ix = c->opcix ^ 1; else ix = c->opcix;

    if (j && j->swl) ix = c->opcix ^ 1; else ix = c->opcix;



   opl = get_opc_entry(ix);  // mapped to found instance

   *tx = opl->sce;      // select correct version

   t = *tx+1;    // skip '\7', t points to operator string

   ans = find_last_psw(c,&psinst);  // returns 1 if found...op in psinst

//if (j->swl) pstr (0, "S ");
   pstr (0,"if (");

   // if (!ans) return;          // not found







   if (opl->fend[2])            // fend3)
     {            // not a compare
      if (!nobank(psinst.opr[3].addr))
        {
         // wop is R0, style"(op) = 0"
         // does not work for shortened ops, i.e. "+="

         if (nobank(psinst.opr[2].addr))
           {  // print only [1] and op if zero register in [2]
            s = opl->sce;               // source code of psw op
            x = strchr(s, '=');
            if (x)
             {  // do opcode after = in brackets
              x += 3;                                // should now be op
              pstr(0,"(");
              p_opsc(&psinst,2);                    // operand 2 of compare (write)
              while (*x > 0x1f) pchar(*x++);         // operator (space & up)
              p_opsc(&psinst,1);                    // operand 1 of compare
              pstr(0,")");
             }
           }
         else
           {
             p_opsc(&psinst,1);                 // operand 1 of compare (write)
           }
         pstr(0," %s ",t);                      // orig compare op
         p_opsc(0,1);                         // orig compare value
        }
      else
       {   // no compare real wop , style [wop] == 0
        p_opsc(&psinst,3);            //opl->wop,1);         // write
        pstr(0," %s ",t);
        p_opsc(0,1);
       }
     }


   if (!opl->fend[2])                     // fend3)         //wop
     {      // compare found, style R2 = R1
      if (!nobank(psinst.opr[2].addr))
       {
         if (!nobank(psinst.opr[1].addr) && ans)
          {       //both ops zero ??
            pstr(0," %s ", zerocmpop[ix - 62]);       //c->opcix - 62]);
          }
         else
          {
           // cmp has zero first, so swop operands over via special array.
           // NB. NOT same as 'goto' to {} logical swop
           // so R1 <reversed> 0
           p_opsc(&psinst,1);                        // operand 1 of compare
           pstr(0," %s ", swopcmpop[ix - 62]);        //c->opcix - 62]);
           p_opsc(&psinst,2);                        // operand 2 of compare (zero)
          }
       }
     else
       {
         p_opsc(&psinst,2);                        // operand 2 of compare (reads)
         pstr(0," %s ",t);
         p_opsc(&psinst,1);                        // operand 1 of compare
       }
     }

    pstr(0,") ");
    *tx = scespec[3] -1; //opctbl[JMPIX].sce - 1;             //set \9 for 'goto' processing - or call subr....
}


void pp_answer(INST *c)
{
  // print answer, if there is one
  uint addr;
  SUB *sub;
  SPF *a;
  SYM *sym;
  OPS *o;
  void *fid;

  addr = c->opr[1].addr;
  sub = get_subr(addr);

  if (!sub) return;
 // a = get_spf(s,0);


 fid = sub;

  while ((a = get_spf(fid)))          //if (!a) return;
  {
    //if (a->spf > 1) break;           //no answer found

    if (a->spf == 1)
    {

  // found an answer. set up a phantom opr[3] as write op... (rgf set)
     o = c->opr+3;
     sym = 0;

     o->addr =  a->pars[0].reg;               //addrreg;

      if (!valid_reg(o->addr))
       {
         o->rgf = 0;
         o->adr = 1;
         o->bkt = 1;
       }

     sym = get_sym(o->addr,0,7,c->ofst);
     if (sym) {o->sym = 1; o->bkt = 0;}

     p_opsc(c,3);
     pstr(0," = ");
    }
    fid = a;
  }
}




void prt_carry(INST *c, cchar **tx)     //, int ainx)
{

// Replace "if (CY)" with various sources
// possible opcodes add,shift R, shift L, cmp, inc, dec, neg, sub

   int ans, ainx;
   const OPC *opl;

   pstr (0,"if (");
ainx = c->opcix ^1;

   // get last psw changer
   ans = find_last_psw(c,&psinst);  // returns 1 if found...op in psinst
   if (!ans) return;          // not found, revert to "if (CY"   style

   opl = get_opc_entry(psinst.opcix);              // mapped to found inst

   if (opl->sigix == 7)
        {
          // subtract. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];
          p_opsc(&psinst,3);                  //opl->wop,1);           // write
          pstr(0," %s ",*tx);
          p_opsc(0,1);
          pstr(0,") ");
          *tx = scespec[3] -1;     //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

  if (opl->sigix == 10)
        {
          // compare. Same as subtratc, but no wop
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];

          if (valid_reg(psinst.opr[2].addr) == 2)                // psinst.opr[2].addr == 0)
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(&psinst,1);                   // operand 1 of compare
              pstr(0," %s ", swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(&psinst,2);                   // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(&psinst,2);
              pstr(0," %s ",*tx);
              p_opsc(&psinst, 1);
            }
          pstr(0,") ");
          *tx = scespec[3] -1;    //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 6  || opl->sigix == 25)
        {       //  add or inc STANDARD CARRY -  have wop ?
          p_opsc(&psinst,3);                     //opl->wop,1);
          pstr(0," %s", (ainx & 1) ? ">" : "<=");
          pstr(0," %x) ", get_sizemask(psinst.opr[3].fend));       //opl->wop].wfend));
          *tx = scespec[3] -1;         //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


   if (opl->sigix == 3 || opl->sigix == 4)
        {   //  shl (3)   or shr (4)

          // check no of shifts is imd, otherwise return 0=0 style.
           if (!psinst.opr[1].imm)
            {
             clr_inst(&psinst);       // can't do variables
             return;
            }

           if (opl->sigix == 3)
            {   //shl
             ans =  psinst.opr[3].fend + 1;       //opl->wop].wfend + 1;              // number of bits
             ans -= psinst.opr[1].val;                            //shift from MSig

             if (ans > 16)
               { // dl shifts ??
                 ans -= 16;
                 psinst.opr[3].addr+=2;          //opl->wop].addr+=2;
               }
            }
          else
            {  //shl
              ans = psinst.opr[1].val-1;
            }

          pstr(0,"B%d_", ans);

          p_opsc(&psinst,3);               //opl->wop,1);
          pstr(0," = %d) ", ainx & 1);
          *tx = scespec[3] -1;         //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

 /*  if (opl->sigix == 11 || opl->sigix == 8)
        {       //  mult,div STANDARD OVERFLOW all have wop ?
          p_opsc(&psinst,opl->wop,0);
          pstr(" %s", (ainx & 1) ? ">" : "<=" );
          pstr(" %x) ", camk[psinst.opr[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }
*/

     if (opl->sigix == 24)
      {       // dec
          p_opsc(&psinst,3);             //opl->wop,1);
          pstr(0," %s 0) ", (ainx & 1) ?  ">=" : "<" );
          *tx = scespec[3] -1;             //opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
      }

}


void prt_jump(INST *c, cchar **tx)     //, int ainx)prt(
{

//        j =


/*
JMP *find_fjump (uint ofst, int *x)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 8 for 'else', 4 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JMP *j;

 // if (get_anlpass() < ANLPRT) return 0;

  *x = 0;

  j = get_fjump(ofst);

  if (!j) return 0;


  if (!j->jtype)  *x = 0;   // true return

  if (j->retn)  *x = 2;      // jump to a return

  if (j->jtype == J_ELSE) *x = 4;
  if (j->obkt) *x = 1;     // bracket - reverse condition in source

// if (j->jelse) *x |= 4;          //pstr(" } else { ");
//if (j->jor)   pstr(0," ** OR **");

  return j;
}

*/



/*           from print block below
        find_fjump (c->ofst, &jf);  */

    JMP *j;


    j = get_fjump(c->ofst);

    if (!j) return ;

     /*   if ((jf & 1) && *tx)
         {                             // invert jump logic for source printout
           if (inx > 53 && inx < 72)   // all conditional jumps (except djnz)
           {
             const OPC *s;
             ainx = inx ^ 1;           // get alternate opcode index if jump found.
             s = get_opc_entry(ainx);
             tx = s->sce;    // and source code
//indent++;
           }
           else jf = 0;
        }
*/
             // if (j<3)  always true at present
/* make this bitmap instead of switch ???
 *  if (!j->jtype)  *x = 0;   // true return

  if (j->retn)  *x = 2;      // jump to a return

  if (j->jtype == J_ELSE) *x = 4;
  if (j->obkt) *x = 1;     // bracket - reverse condition in source

// if (j->jelse) *x |= 4;          //pstr(" } else { ");
//if (j->jor)   pstr(0," ** OR **");

            switch (jf)    //jump handling
             {
              case 1 :
               *
               *
               */


        //         if (j->swl) { //this must go in other jump prints......
                 // swop sense over with c->opcix ^ 1;
                 //   s = get_opc_entry(ainx);
                 // tx = s->sce;    // and source code
          //       pstr (0," S");       // temp
             //    }

    //   if (j->jtype == J_ELSE) { pstr (0, "} else { "); return;}

           //    if (j->cbkt)  pstr (0,"C ");   // [if] case
               if (j->obkt)  pstr (0,"{");   // [if] case
               if (j->retn)   {pstr (0,"return;"); return;}  // static jump to a return


/*
      break;
              case 3:
                pchar(';');
                // fall through
              case 4:
                pstr(0,"} else { ");
                break;
case 8:
break;
              default:
               *
               */
                 if (!j->obkt) {
                pstr (0,"goto ");
                p_opsc (c,1);
                 pchar (';'); }
             /*   break;
             }

/ if (j && j->retn)
    {
 //     if (jf == 1) { pstr(" }");  j->cbkt = 0; }


           acmnt.minst = c;
                acmnt.ctext = nm;
                acmnt.ofst = prtscan.nextaddr;
                sprintf(nm, "## -> Return");

}   */


}


void prt_jbit(INST *c, cchar **tx)
{
         // "if (\3\2 = 0) \x9"  effectively



  JMP *j;

  const OPC *alt;

  pstr (0,"if (");

  j = get_fjump(c->ofst);

  if (!j) return;

  if (j->swl)
    {
     alt = get_opc_entry(c->opcix ^ 1);

     *tx = alt->sce;

     //and don't do the jump....so print all of it ??
     //could do *tx = 0;

}

}











void fix_sym_names(cchar **tx, INST *c)
{
  // adjust operands for correct SOURCE code printout, rbases, indexes etc.
  // This CHANGES addresses for indexed and RBS for printout purposes

  OPS *b, *o;
  RBT *r;
  SYM *s;
  int i;
  int addr;
  const OPC* opl;


  opl = get_opc_entry(c->opcix);

  // get and flag all sym names FIRST, then can drop them out later as reqd

  for (i = 1; i <= 3; i++)               //opl->nops; i++)
   {
    o = c->opr+i;
    o->sym = 0;            // no sym
    s = NULL;

    s = get_sym(o->addr, 0, o->fend,c->ofst);      // write sym or read sym
    if (s)
      {
       o->sym = 1;  // symbol found
      }
   }

  if (c->opr[3].bit)            // bit jump - if bitname, drop register name
     {
        o = c->opr+2;
        b = c->opr+3;               // b = bit number entry, get bit/flag name
        b->sym = 0;                 // clear bit name flag
        s = get_sym (o->addr, b->val, b->val, c->ofst);  // read sym  WHY READ TWICE ????
        if (s)
          {
           o->rgf = 0;
           o->sym = 0;
           b->sym = 1;
           b->addr = o->addr;
          }
     }

  //  Shifts, drop sym if immediate
  if (c->opcix < 10 && c->opr[1].val < 16)  c->opr[1].sym = 0;

  // indexed - check if RBS, or zero base (equiv of = [imd])

  if (c->opcsub == 3)           //indexed, use address in cell [0] for sym
     {
     b = c->opr;                // probably redundant, but safety - cell [0]
     o = c->opr+1;

     if (valid_reg(o->addr) == 2) o->val = 0;            // sero register safety

     if (valid_reg(o->addr) == 2 || o->rbs)
      {
        // Rbase+off  or R0+off
        // merge the two operands to fixed addr  and drop register operand
        // can't rely on o->val always being correct (pop screws it up...) so reset here

        if (o->rbs)
          {           // rbase - get true value from rb chain
           r = get_rbase(o->addr, c->ofst);
           o->val = r->val;                     // but already done ?
          }

        addr = c->opr->addr + o->val;          // addr may be NEGATIVE at this point !!
        if (addr < 0) addr = -addr;                // effective wrap around ? never true ?.......

//        o->addr = o->val + c->opr->addr;                 // add [0] to get true address
        o->addr = databank(addr, c);                  // need bank for index addition

        o->rgf = 0;
        o->sym = 0;
        o->adr = 1;                                      // [o] is now a bare address
        b->off = 0;                                      // no following offset reqd

        s = get_sym(o->addr, 0,7,c->ofst);       // and sym for true inx address

        if (s)     //symbol found at (true) addr
         {
          o->bkt = 0;       // drop brackets (from [1+0])
          o->sym = 1;       // mark symbol found
         }

      } // end merge (rbs or zero)
     else
      {
        // this is an [Rx + offset]. Sort op[0] symbol if not a small offset
        if (valid_reg(b->addr) == 1 && !b->neg)
          {
            // not negative, not reserved register, add databank if > PCORG

            s = get_sym(databank(b->addr,c), 0,7,c->ofst);
            if (s)
              {
               if (nobank(b->addr) > PCORG || s->immok)  b->sym = 1;
              }
            if (get_numbanks()) b->addr = databank(b->addr,c);         // do bank for printout
          }
      }
    }         // end inx

  if (c->opcsub == 1)       // immediate - check opcodes etc. for removing symbol names
     {
      // not small values (register names ??)
      if (valid_reg(c->opr[1].addr))  c->opr[1].sym = 0;      // not (normally) names for registers
      // sub ??
      // not 5 and, 8 mult, 9 or,xor, 10 cmp, 11 div  - all must be an abs value
      // ldx 12 with rbase - no symname
      else if (opl->sigix == 5)  c->opr[1].sym = 0;
      else if (opl->sigix > 7 && opl->sigix < 12)  c->opr[1].sym = 0;
      else if (opl->sigix == 12 && c->opr[2].rbs)  c->opr[1].sym = 0;
     }


  // 3 operands.  look for rbase or zero in op[1] or op [2] and convert or drop
  if (opl->nops > 2)
    {    //rbase and R0
     if (c->opr[1].rbs && valid_reg(c->opr[2].addr) == 2)  shuffle_ops(tx,c,2);

     if (c->opr[2].rbs && c->opcsub == 1 && c->opcix > 23 && c->opcix < 29)
      {   // RBASE + offset by addition or subtraction, only if imd and ad3w or sb3w
       b = c->opr+1;
       addr =  b->val + c->opr[2].val; // addr may be NEGATIVE at this point !!
       if (addr < 0) addr = -addr;

       b->addr = addr;         //b->val + c->opr[2].val;              // convert to true address....
       // databank ?
       b->imm = 0;
       b->adr = 1;
       s = get_sym(b->addr,0,7,c->ofst);

       if (s) b->sym = 1;
       shuffle_ops(tx,c,2);                  // drop op [2]
      }

    if (valid_reg(c->opr[2].addr) == 2)   shuffle_ops(tx,c,2);      // x+0, drop R0
    if (valid_reg(c->opr[1].addr) == 2)   shuffle_ops(tx,c,1);      // 0+x, drop R0



   }   // end op 3 op checks

 //     R3 = R2 + R1 style.  is anything rbase ?? replace with address as an imd
  if (!c->opcsub && (c->opr[1].rbs || c->opr[2].rbs))
      {
        for (i = 1; i < c->numops; i++)
          {  //  op 1 for 2 ops, 1 & 2 for 3 ops
            b = c->opr + i;
            if (b->rbs)
              {
               b->imm = 1;
               b->rgf = 0;
               b->sym = 0;
              }
          }
       }

    // CARRY opcodes - can get 0+x in 2 op adds as well for carry
    // "\x2 += \x1 + CY;"   nobank is for immediates... adc and subb

  if (nobank(c->opr[1].addr) == 0 && c->opcix > 41 && c->opcix < 46)
    {
     if (tx) *tx = scespec[opl->sigix-6];     // drop op [1] via special array
    }
}

void save_banks(SBK* s)
{
  if (!datbnk)
    {   //first use of banks for print
      rambnk = 0;
      if (get_numbanks()) datbnk = 2;           // data bank 1 for multibanks
      else   datbnk = 9;                        // 8 for single
    }
  else
  {
   rambnk = s->rambnk;                 // banks to carry over for print
   datbnk = s->datbnk;
  }
}

void set_prt_banks(SBK* s)
{

  s->codbnk = (s->start >> 16) & 0xf;          // code bank from start addr               //but not updated immediately !!

  s->rambnk = rambnk;
  s->datbnk = datbnk;
}


/**************************************************************************
* opcode printout - calls do_code
***************************************************************************/

uint pp_code (uint ofst, LBK *k)
{
  int cnt;

  cchar  *tx ;
  const OPC* opl;
  INST *c;
  SBK *p;

  p = &prtscan;
//this code worked when banks were separate..............now need a save/restore ??
  save_banks(p);

  memset(p,0, sizeof(SBK));     // clear it first   NO!! need banks.....
  c = &pinst;                          // print instance
  p->curaddr = ofst;
  p->start = ofst;

  set_prt_banks(p);

  do_code(p, c);

  pcmd.minst = c;           // for later comments
//  preset_comment(ofst);  // begin and end reqd ??

  if (p->inv)
    {
     pp_hdr (ofst, "!INV!", 1);   // single invalid opcode (= !INV!)
     return p->nextaddr+1;
     }

  // BANK - For printout, show bank swop prefix as a separate opcode
  // This is neatest way to show addresses correctly in printout
  // internally, keep 'ofst' at original address, but update curaddr for printout.

  if (c->bank)
    {
     pp_hdr (ofst, "rombk", 2);
     pstr (0,"%d", c->bank-1);
     p->curaddr +=2;
    }

  opl = get_opc_entry(c->opcix);

  cnt = p->nextaddr - p->curaddr;
  pp_hdr (p->curaddr, opl->name, cnt); // header + opcode

  cnt = c->numops;             //change this ??

  while (cnt > 0)
     {
      p_oper (c, cnt);              // operands printout
      if (--cnt) pchar (',');
     }

  /**********************************************
  * source code generation from here
  * check and do substitutes and cleanups, name resolution etc.
  *********************************************/

  if (get_cmdopt(OPTSSRC))
    {
      if (opl->sce)
       {               // skip if no pseudo source
          cnt = find_tjump (p->nextaddr);         // add any reqd close bracket(s)

          // if (cnt & 0x1000) print an else....
          /// also for do...while.....

        p_pad(PSCEPOSN);
   //     pstr("|%d ", indent);         // experiment, for lining up and new brackets

        tx = opl->sce;                 // source code, before any mods

        // any mods to source code go here

        fix_sym_names(&tx,c);           // for rbases, 3 ops, names, indexed fixes etc.
    //    shift_comment(c);               // add comment to shifts with mutiply/divide  - if (curinst.opcix < 10 && ops->imd)

       //  do 'psuedo source code' output

       while (tx && *tx)
        {

         switch (*tx)   // 1-12 used  1-31 available (0x1 to 0x1f)
          {
           case 1:
           case 2:
           case 3:
           case 0x11:           // 17, 18, 19. (+ 0x10)  = print size & sign
           case 0x12:
           case 0x13:

             p_opsc (c, *tx);
             break;

           case 4:
             // replace bit masks with flag names - ANDs and ORs etc     //MORE//  if (ix >  9 && ix < 14) ||  (ix > 29 && ix < 34)
             bitwise_replace(&tx, c);
             break;

           case 5:
             // subr call, may have answer attached.
             pp_answer(c);
             p_opsc (c, 1);
             p->nextaddr += pp_subargs(c,p->nextaddr);
             break;

           case 6:                  // overflow JV JNV - not used (yet)
             prt_ovf(c, &tx);
             break;

           case 7:                  // all conditional jumps with 'get last PSW reqd'
             prt_cond_jmp(c, &tx);
             break;

           case 8:                 // carry  JC and JNC
             prt_carry(c, &tx);
             break;

           case 9:
             prt_jump(c, &tx);   // general goto (+ else,and,or, bracket etc)
             break;

           case 10:
             prt_jbit(c, &tx);     // bit jumps   JB, JNB
             break;

           case 11:
             p_indl();             // 0xb  print new line with indent (DJNZ)
             break;

           default:
             pchar(*tx);
          }         // end switch tx
          if (!tx) break;
          tx++;
        }           // end while tx
      }             // end if opl->sce
   }                // end if PSSRC
  else
     {      // subr arguments without source code option - must still print and skip operands
     // use c->sigix == 17 insteadf ??
      if (!get_cmdopt(OPTSSRC) && (c->opcix == 73 || c->opcix == 74 || c->opcix == 78 || c->opcix == 79))          // CALL opcodes
         {
           p->nextaddr += pp_subargs(c,p->nextaddr);
         }
     }


   //  i = find_tjump (p->nextaddr);         // add any reqd close bracket(s) at top
cnt &= 0xf;

     while (cnt > 0)
      {               // i = number of brackets (=jumps) found
       pstr (0," }");
       cnt--;
      }

  //   if (PACOM) parse_comment(&acmnt,0);                   // print auto comment if one created, but need to check ofst !!

/*
  {
    OPS *s;                              // extra debug printout

  //  pp_comment(ofst+cnt);              // do comment first
    if (opctbl[curinst.opcix].nops)
     {
      p_pad(cposn[3]);                          //CDOSTART);
      pstr(" SVA ");

      for (i=curinst.numops;  i>=0; i--)
        {
         s = curops+i;
         if (s->addr + s->val)
          {
   //        if(curinst.opl->wop == i && i)  pstr("*");
           pstr("%c[",i+0x30);

           if (s->sym) pstr("%s ",get_syxxm_name(0,s->addr,0));       // temp fix.....

           pstr("%x %x",s->val, s->addr);
//           if (!i) pstr (" T%x", curinst.tval);          //new temp value
           pstr("]%x, ",s->ssize);
          }
        }
     }
  }                 // end dubug         */

  // add an extra blank line after END BLOCK opcodes, via comments
  // 75, 81 are jumps 76->79 are rets

 if (p->nextaddr <= k->end)
  {
   if (c->opcix > 79 && c->opcix < 83) pp_comment(p->nextaddr,1);   // Rets c->sigix == 20
   if (c->opcix == 75 || c->opcix == 77)  pp_comment(p->nextaddr,1); //jumps c->sigix == 16 ?? not skip ??
  }
 return p->nextaddr;
}




 void prt_listing (void)
{
  uint ofst;
  int i, lastcom;
  LBK *x;
  BANK *b;
  DIRS *last,*cmd;
  HDATA *fl;

  fl = get_fldata();

  memset(&pcmd,0,sizeof(CPS));            //safety if there's no cmt file.

  pstr(1,0);            //newline
  p_run(1,'#', 50);
  pstr(0,"# SAD Version %s     (%s)", SADVERSION, SADDATE);
  p_pad(49);
  pchar('#');
  pstr(1,0);            //newline
  p_run(1,'#',50);
  pstr(0,"# Disassembly listing of file '%s%s'.",fl->barename,fl->suffix[0]);
  p_pad(49);
  pchar('#');
  pstr(1,0);
  //pstr(0,"  806%d%c %d bank%c ", P8065 ? 5 : 1, PSTPD ? 'D' : ' ',  numbanks+1, numbanks ? 's' : ' ');
  pstr(0,"# Appears to be %d %s, 806%d", get_numbanks()+1, get_numbanks() ? "banks" : "bank", get_cmdopt(OPT8065) ? 5 : 1);
  p_pad(49);
  pchar('#');
  pstr(1,0);

  pstr(0,"# See '%s%s' file for more info", fl->barename, fl->suffix[2]);
  p_pad(49);
  pchar('#');
  pstr(1,0);

 // pstr(1,"# Overview of extra flags and formats - ");
 // pstr(1,"#   R = general register.  '++' increment register after operation, e.g. R30++; .");
 // pstr(1,"# Extra prefix letters are shown for mixed size opcodes (e.g DIVW)");
 // pstr(1,"#   l = long (4 bytes), w = word (2 bytes), y = byte, s = signed. Unsigned is default.");
 // pstr(1,"#   [ ] = use value as an address. Addresses are always word. Can be [R30] (contents of R30)");
 // pstr(2,"#   or [3456] as a direct address.   '++' increment register after operation." );                //  @=value is an address (to symbol),
 // pstr(1,"# Processor status flags (for conditional jumps)" );
 // pstr(1,"#   CY=carry, STC=sticky, OVF=overflow, OVT=overflow trap");
#ifdef  XDBGX

  pstr(0,"# - - - -  DEBUG SET - - - - - ");
  p_pad(49);
  pchar('#');
  pstr(1,0);
#endif
  p_run(2,'#',50);



 for (i = 0; i < BMAX; i++)
    {
     b = get_bkdata(i);            //bkmap+i;

     if (!b->bok) continue;

//     if (get_numbanks())
      {
       pstr (2,0);
       p_run (1,'#',75);
       if (get_numbanks()) pstr (0,"# Bank %d", i-1); else pstr (0,"# Single Bank");
       pstr (0,"  file offset %x-%x,   bank ", b->filstrt, b->filend);
       prtaddr(b->minromadd,0,pstr);  pstr(0," - "); prtaddr(b->maxromadd,0,pstr);
//       if (i == 9) pstr(0, " CODE begins HERE");
       p_pad(74);
       pchar('#');
       pstr(1,0);
       p_run (2,'#',75);
      }

  //   indent = 0;
     lastcom = 0;

     for (ofst = b->minromadd; ofst < b->maxromadd; )
      {
         // first, find command for this offset

       x = get_prt_cmd(&prtblk, b, ofst);

       cmd  = get_cmd_def(x->fcom);
       last = get_cmd_def(lastcom);

       // add newline if changing block print types NB lastcom == 0 is fill....

       // if (dirs[x->fcom].prtcmd != dirs[lastcom].prtcmd || dirs[x->fcom].prtcmd == pp_stct) {pp_comment(ofst,1);}

       if (cmd->prtcmd != last->prtcmd || cmd->prtcmd == pp_stct) {pp_comment(ofst,1);}
       lastcom = x->fcom;

       while (ofst <= x->end)
         {
          show_prog (get_anlpass());
          pp_comment(ofst,0);

          ofst = cmd->prtcmd (ofst, x);
         }
       }      // ofst loop

     pp_comment(b->maxromadd,0);     //+1,0);             // any trailing comments at bank or listing end
    }        // bank
}

