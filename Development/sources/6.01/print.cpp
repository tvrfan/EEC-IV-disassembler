
#include  "print.h"


//calc field widths by actual value per radix. [0] is list size, index is the fieldwidth. not used yet.....

uint decpfw[10]  = {10 ,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000};      // decimal
uint hexpfw[8]   = {8, 0x10, 0x100,0x1000,0x10000, 0x100000, 0x1000000, 0x10000000};          // hex


//010 is octal 8
uint binpfw[30]  = {30, 2,4,8,16,32,64,128,256,1024,2048,4096,8192,16384,32768, 65536, 131072,262144,524288, 1048576, 2097152,4194304,8388608,16777216,
33554432,67108864,134217728,268435456,536870912,1073741824};          //bin


uint  plist [64];             // generic param list and column positions for structs,prob needs to be bigger !!!

uint  lastcmt;                 // last cmt state for end and multiple comment checking

CPS   pcmd;                     // command holder for comments


PP_OP csym[4];                // 4 print holders for cinst operands
PP_OP ssym[4];                // 4 print holders for sinst operands

INST pinst;                   // printing
INST linst;                   // for last psw searches

LBK prtblk;





uint get_pfwdef(ADT *a)
{   // default print field width
 uint ans, sz, sign;

 sign = (a->fend & 32);
 sz = (a->fend & 31) - a->fstart;

//need more here for

 ans = 0;          // min default size

 if (sz == 7) ans = 3;
 if (sz == 15) ans = 5;
 if (sz == 23) ans = 7;
 if (sz == 31) ans = 9;

 if (sign) ans += 1;      // for -ve sign
 if (!a->pfd) a->pfd = get_cmdopt(OPTDPL);
 return ans;
}




uint xprt (uint fno, uint nl, cchar *fmt, ...)
{  // warning file prt

  FDATA *f;
  uint chars;

  f = get_fldata(fno);

  if (fmt)
    {
     va_list args;
     va_start (args, fmt);
     chars = vfprintf (f->fh, fmt, args);
     va_end (args);
    }
  else chars = 0;

  if (nl) f->pcol = 0;         // newline
  else f->pcol += chars;

  while (nl--) fprintf(f->fh, "\n");

  return chars;
}




void paddr(uint fno, uint addr,uint pfw)
{
  // printer for addresses with or without bank.
  // if single bank or no bank drop bank
  // For listing or debug output

  // NOTE pfw used for listing layout. only in only ONE place !!

 uint chars;
 chars = 0;

 if (!get_numbanks() || g_bank(addr) == 0)
     chars = xprt(fno, 0,"%x", nobank(addr));       // no leading zeroes, or bank
 else
    {    // bank+1 used internally, and leading zeroes
     xprt(fno,0,"%0x", (addr >> 16) -1);
     xprt(fno,0,"%04x", nobank(addr));
     chars = 5;
    }

 while (chars < pfw)        // pad if required
     {
       xprt(fno,0," ");
       chars++;
     }

}





void list_cellsize(uint fno, ADT *a)
{

  uint fend;




if (!a->dptype) return;

  fend = a->fend;

  xprt (fno,0," %c", (fend & 0x20) ? 'S' : 'U');
  fend &= 0x1f;

  if (!a->fstart)
    {
      switch (fend)
      {
       case 7:
         xprt (fno,0," %c", 'Y');
         break;

       case 15:
         xprt (fno,0," %c", 'W');
         break;

       case 23:
         xprt (fno,0," %c", 'T');
         break;

      case 31:
         xprt (fno,0," %c", 'L');
         break;

       default:

        xprt(fno,0, " B %d ",0);
        if (a->fend) xprt(fno,0," %d", fend);
        break;
      }
    }
  else
  {
    xprt(fno,0, " B %d ",a->fstart);
    if (fend != a->fstart) xprt(fno,0," %d", fend);
   }

}

void prt_pfw(uint fno, ADT *a)
{
// will have subfield sizes in here
 if (a->pfw && a->pfw > get_pfwdef(a)) xprt(fno,0,"P %x ",a->pfw);
}



void prt_radix(uint fno, ADT *a, int fcom)
{
  // print mode & radix.
  DIRS *c;
  c = get_dirs(fcom);

  if ((a->dptype != c->defdptype) || fno != MSGFILE)
    {      // print radix, print mode always if debug
     switch (a->dptype)
      {
        default:
          break;

         case DP_HEX :
          xprt (fno,0, "X %d ", 16);     // hex
          break;

        case DP_DADD :
          xprt (fno,0, "R ");     // address  (ref) ??
          break;

        case DP_BIN :
          xprt(fno,0,"X %d ", 2);     // binary
          break;

        case DP_DEC :
          xprt(fno,0,"X %d ", 10);    //decimal
          break;

        case DP_FLT :     //float, assume pfd always set

          xprt(fno,0,"X %d.%d ",10, a->pfd);
          break;

         }
    }
}


//note this V5 replaces below with auto reounded *f param
//this seems to work fine

uint prtfl(uint fno, float fv, uint pfw, uint pfd)
 {
  // separate float print, to control width
  // 'prt' argument allows print to debug or message files
  // note that pfd is not regarded as part of pfw.
  // system call printf automatically round to places specified (=pfd)

  uint cs;
  FDATA *f;

  f = get_fldata(fno);

  //check fv for Nan (not a valid number) and infinity.

  if (isnan(fv)) { }
  if (isinf(fv)) { }

  cs = fprintf(f->fh,"%*.*f", pfw, pfd, fv);
  f->pcol += cs;

  return cs;
}




void p_pad(uint fno, uint c)
{
// List file.  Pad out to required column number if not already there
// and not already padded to. Add 2 spaces if pcol > lastpad
  uint i;
  FDATA *f;

  f = get_fldata(fno);

  if  (c <= f->pcol && f->pcol > f->lastpad) c = f->pcol+2;          // add 2 spaces if beyond tab.
  for (i = f->pcol; i < c; i++)  fputc(' ',f->fh);


  f->pcol = c;
  // f->lastpad = c;                         // KEEP last padded to column unchanged
}

/*
void save_pad(uint fno)
{
  // save current col position - for wraps when col > CMNTPOS
    FDATA *f;

  f = get_fldata(fno);

  if (f->pcol < CMNTPOSN) f->savedcol = CMNTPOSN;
  else
      f->savedcol = f->pcol;

}
*/

void xwrap(uint fno, uint tocol)
 {
  // newline and wrap back to supplied column if already greater,
  // or lastpad column if not

  FDATA *f;

  f = get_fldata(fno);

  fprintf(f->fh, "\n");
  f->pcol = 0;
  if (f->lastpad > tocol)  f->lastpad = tocol;
  p_pad(fno,f->lastpad);
 }

void pchar(uint fno, char z)
{
 FDATA *f;
 f = get_fldata(fno);

 fputc(z,f->fh);
 f->pcol++;
 if (f->pcol >= WRAPPOSN) xwrap(fno,CMNTPOSN);
}


uint pbin(uint fno, uint val, uint fend)
{  // print val in binary
   // not sure about pfw yet ...
  uint i, tval, pfw;

// simple approach

if (val & 0xf000)  pfw = 16;
else if (val & 0x0f00)  pfw = 12;
else if (val & 0x00f0)  pfw = 8;
else pfw = 4;                // temp

 tval = val;
 for (i = 16; i > 0; i--)
  {  // find ms '1' bit
    if (i <= pfw)
      {
       if ((i/4*4) == i) pchar(fno,' ');    //space every 4
       if (tval & 0x8000)  pchar(fno,'1'); else pchar(fno,'0');
      }
    tval <<= 1;
  }

return pfw;

}

// print indent multiple psuedo code lines

void p_indl (uint fno)
{
  FDATA *fl;

  fl = get_fldata(fno);
  if (fl->pcol > PSCEPOSN) xprt(fno,1,0);   //newline  if > sce col
  p_pad(fno,PSCEPOSN);

}

void p_run(uint fno, uint nl, char c, uint num)
{
 uint i;
   FDATA *f;

  f = get_fldata(fno);

 for (i = 0; i < num; i++) fprintf(f->fh, "%c",c);
 f->pcol +=num;
 if (f->pcol >= WRAPPOSN) xwrap(fno,CMNTPOSN);
 if (nl) f->pcol = 0;         // newline
 while (nl--) fprintf(f->fh, "\n");
}


void prtcmdopts(uint fno, uint state)
{
    //prints in sets of 5

  CSINX *s;
  uint i, j;
  CSTR *c;

  s = get_strinx(OPTSTR);
  c = s->strlist;
  j = 0;

  i = 0;

  while (i < s->size)
    {
       if (state && c[i].par2)
         {            //has param attached, print with set
          if (!j) xprt(fno,0, state ? "setopts " : "clropts " );
          xprt(fno,0, "[ %s ", c[i].string);
          if (c[i].par2) xprt(fno, 0, "%d ", c[i].par1);          //param
          pchar(fno,']');
          j++;
         }
       else
        {
         if (state == c[i].par1)
           {            // no params just on or off
             if (!j) xprt(fno,0, state ? "setopts " : "clropts " );
             xprt(fno,0, "[ %s ]", c[i].string);   // 1 if set values, 0 if not
             j++;
           }
        }
       i++;

       if (j >= 5)
         {
          xprt(fno,1,0);
          j = 0;
         }
    }


   }


void list_usrcmt(uint fno)
{
   // MSGFILE
     p_pad(fno,CMNTPOSN);
     xprt(fno,0," #user cmd");
}

void list_syscmt(uint fno)
{
   // MSGFILE
     p_pad(fno,CMNTPOSN);
     xprt(fno,0," # auto added");
}





void list_opts(uint fno)
{
 // print opt flags to msg file

  prtcmdopts(fno,1);
  xprt(fno,1,0);
  prtcmdopts(fno,0);

 xprt(fno,2,0);
}


int abank(ADT *a, int val)
{
   if (!valid_reg(val) && a->bank) val |= (a->bank << 16);
 return val;
}


void convert_par(MATHX *m, MNUM *p)
{
    //convert parameter types according to calctype

    // if (h->calctype == 1) h->total.ival &= 0xffff;           //address calc wraps in 16 bits
      if (m->calctype < DP_FLT && p->dptype == DP_FLT)  {p->ival = p->fval;  p->dptype = m->calctype; }    //float to int
      if (m->calctype == DP_FLT && p->dptype < DP_FLT)   {p->fval = p->ival;  p->dptype = DP_FLT; }    //int to float

//others ??

}

uint calc_pfw(MHLD *h)        //, ADT *a)
{
// calc actual field width for aligned structs
 //cptl from parent LBK command 1 if compact 0 if extended
  int val ;
  uint ans, neg, i;
  uint *z;


  if (h->total.dptype < DP_FLT) val = h->total.ival;
  else val = h->total.fval;
  neg = 0;

  switch(h->total.dptype )
   {

      default:
    //    ans = a->pfw;
        z = 0;
        break;

      case DP_BIN:
        z = binpfw;
        break;

      case DP_DADD:
       z = 0;
       if (get_numbanks()) ans = 5;   // with bank
       else ans = 4;
       break;

      case DP_HEX:
        z = hexpfw;
        break;

      case DP_DEC:
  //      z = decpfw;
       // fall through
      case DP_FLT:
      z = decpfw;
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

      ans = i;      // +1;                  //  add a space NO !! done outside
      if (neg) ans ++;            //  add '-'

   }
    // if not compact, use wider default settings
 //  if (ans < a->pfw) ans = a->pfw;
   //  if (a->fend & 0x20) ans += 1;           //with sign
    return ans;

}













MHLD* do_tcalc(void *fid, int pix, ADT *a, int ofst)                   //MHLD *h)
  {
    // fid as fed in at top level is a NAME id. lower levels are mathx.
   // h->fid is a mathx term, running totals etc all in h
   // ctype is calc type (from name entry at top)

   int i;
   MATHX *m;
   MHLD *h, *s;            //calc holder
   MNUM *p;
   MXF *clc;

// set up calc holder for this level (new one if sub level)

  h = (MHLD*) cmem(0,0,sizeof(MHLD));     // alloc and clear calc holder
//  memset(h,0, sizeof(MHLD));             // mem does not clear alloced memory !!

 // need ofst and original adt block for bank ????

// starts at total = zero.................



 while ((m = get_mterm(fid,pix)))
  {

    fid = m;
     // get params.  if params are subs, call to calculate them
    h->npars = m->npars;
    h->calctype = m->calctype;
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

      convert_par(m,p);      // so as all are same type for calculation

     }

    clc = get_mathfunc(m->func);
    clc->calc(h);
    h->total.dptype = p->dptype;

    //may need to change pfw as well ?????

    //databank if DP_ADD ???

}


   return h;        //return hold which must be freed
  }




 MHLD *do_adt_calc(ADT *a, uint ofst)
  {
     // initailise and do the calc if there is one
     // return holder so can be used outside

    FKL *l;
    MATHX *x;
    MHLD *clchdr;     // calc holder

   l = get_link(CHADNL, a, CHMATHX,0, 0);       // fwd link adt to mathx

   // no attached calc, so return plain int value from ADT ofst (default)
   // in calc holder and use a->dptype as calctype

   if (!l)
     {
      clchdr = (MHLD*) mem(0,0,sizeof(MHLD));     // dummy calc holder
      memset(clchdr,0, sizeof(MHLD));             // and zero it
      clchdr->fid      = a;
      clchdr->calctype   = a->dptype;           //  default to adt type
      clchdr->total.ival  = g_val (ofst, a->fstart, a->fend);
      clchdr->total.dptype = a->dptype;
      return clchdr;
     }

  // calc attached, get and setup details

   x = (MATHX*) l->keydst;                       // this is first calc term

   // calc may reset print mode, also may need to reset pfw....

   clchdr = do_tcalc(x->fid,0,a,ofst);

   //do tcalc returns an MHLD struct

   if (clchdr->calctype) a->dptype = clchdr->calctype;       // reset print mode

   //pfw ??

   return clchdr;
  }





void maxcols(void *fid, uint bix, uint *col, uint *ofst)
{
    //called for compact layout only.

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

         maxcols(a, 1,col,ofst);             // SUB  GOES HERE..before parent cell

         h =  do_adt_calc(a,*ofst);          // value after calc

         sym = 0;
         if (a->fnam && h->total.dptype < DP_FLT)               //== DPADD)
           {
             j = h->total.ival | (a->bank << 16);  // add bank for sym
             sym = get_sym(j ,0, 7,*ofst);        // READ sym AFTER any decodes - may be only addresses ????
           }

         if (sym)    j = sym->symsize;              // sym name length (but already has space ??
         else        j = calc_pfw(h);    // data or default  /buit is this what gets printed ???

       //  if (ans < a->pfw) ans = a->pfw;
       //   if (a->fend & 0x20) ans += 1;           // move outside calc to here...not rqd ??...


         if (j > plist[*col]) plist[*col] = j;      // keep largest width

         (*col)++;
        if (!bix) *ofst += bytes(a->fend);

        mfree(h,sizeof(MHLD));     // free calc holder


        }

       if (!bix && !a->cnt) (*ofst)++;                         // safety to stop infinite loop
       if (a->newl) *col = 1;                       // restart items if newline
       fid = a;
     }            //end while a

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
   if (!x->cptl) return;           // ignore if arg list.

   sz = 0;
   fid = vconvi(x->start);

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

// recalce sizes for hdr size and pad position.
  // ADD a comment column here ?

  // calc - *3 for byte + comma, Plus 5 address+colon plus 2 spaces

 plist[0] = (plist[0]*3) + 7;
 if (get_numbanks()) plist[0]++;         // for exta bank digit

 if (plist[0] < MNEMPOSN) plist[0] = MNEMPOSN;   // minimum at mnemonic col

}




int maxcolsizes(LBK *x)
{
 // calc max column sizes of actual data for each line
 // used for all structs

// ADT  *a;
// SYM  *sym;
 //MHLD *h;
 CSTR *c;

 uint i, ofst, end, item;
 void *fid;

 ofst = x->start;
// memset(&plist,0,sizeof(plist)); // done in caller

 if (!x->cptl) return 0;            // extended layout

 end = x->end - x->term ;  // looks OK

 calchdrsize(x);         // in plist[0],  bytes to print.

item = 0;          //safety

 while (ofst < end)
   {
    item = 1;
    fid = vconvi(x->start);
    maxcols(fid,0,&item, &ofst);
   }

  // Now add up col widths for comment pad position,
  // including header at [0]

 for (i = 0; i <= item; i++)
   {
     plist[31] += plist[i]+2;           // add space and comma for each item
   }

c = get_cstr(CMDSTR, x->fcom);

 plist[31] += c->len;                  // cmnd string len

// assign value to fl  somewhere !!
 return item;
}




void pp_hdr (int ofst, cchar *com,  int cnt)
{
  SYM *s;
  int sym;

  if (cnt & P_NOSYM) sym = 0; else sym = 1;
  cnt &= (P_NOSYM-1);         // max count is 255

  if (!cnt) return;

  xprt(LSTFILE,1,0);                                         // always start at a new line for hdr
  if (sym)
    {                                                   // name allowed
     s = get_sym (ofst,0,C_NOBIT|7,ofst);                 // is there a symbol here (HDR)?
     if (s)
      {
       p_pad(LSTFILE,3);
       xprt(LSTFILE,1,"%s:", s->name);                      // output newline and reset column after symbol
      }
    }

  paddr(LSTFILE,ofst,0);       //caters for bank and no bank
  xprt(LSTFILE,0,": ");

  while (--cnt) xprt(LSTFILE,0,"%02x,", g_byte(ofst++));
  xprt(LSTFILE,0,"%02x", g_byte(ofst++));

  if (com)
   {
 //  FDATA *fl;

 //  fl = get_fldata(LSTFILE);
//    if (fl->pcol >= MNEMPOSN)  p_pad(LSTFILE,fl->pcol+2); else
  p_pad(LSTFILE,MNEMPOSN);
    xprt(LSTFILE,0,"%s",com);
//    if (fl->pcol >= OPNDPOSN)  p_pad(LSTFILE,fl->pcol+2); else
  p_pad(LSTFILE,OPNDPOSN);
   }

 }




uint pp_vect(uint ofst, LBK *x)
{
  /*************************
  * A9l has param data in its vector list
  * do a find subr to decide what to print
  * ALWAYS have an adnl blk for bank, even if default
  * ***************/

  int val, bytes, bank;
  SYM *s;           //char *n;
  ADT *a;
  CSTR *cmd;

  cmd = get_cstr(CMDSTR,0);       //x->fcom);
  s = NULL;
  a = get_adt(vconvi(x->start),0);

  bytes = 2;                   // word (for now)

  val = g_word (ofst);          //always word

  if (a) bank = a->bank << 16; else bank = g_bank(x->start);

  val |= bank;               // add bank to value

  if (get_copcode(val))                //get_opstart(val))
    {   // valid code address
      pp_hdr (ofst, cmd[x->fcom].string, bytes);
      s = get_sym (val, 0, C_NOBIT|7, val);           //vect
    }
  else
    {
      if (ofst & 1)  bytes = 1;        // byte, safety
      pp_hdr (ofst, cmd[bytes].string, bytes);    //else 'word'
    }

  paddr(LSTFILE,val,0);

  if (s)
     {
     p_pad(LSTFILE,PSCEPOSN);
     xprt(LSTFILE,0,"%s", s->name);
     }

  return bytes;
}





uint pp_dmy(uint ofst, LBK *x)
{
 return 1;       // don't print anything.
}


uint pp_dflt (uint ofst, LBK *x)
{
  int i, cnt, sval;       // for big blocks of fill.....
  CSTR *cmd;

  cmd = get_cstr(CMDSTR,0);

  cnt = x->end-ofst+1;

  sval = g_byte(ofst);            // start value for repeats

 for (i = 1; i < cnt; ++i)        // count equal values
    {
      if (g_byte(ofst+i) != sval) break;
    }

  if (i > 32)   // more than 2 rows of fill - compress
     {
      xprt  (LSTFILE,1,0);            // newline first
      paddr (LSTFILE,ofst,0);
      xprt  (LSTFILE,0," -> ");
      paddr (LSTFILE,ofst+i-1,0);
      xprt  (LSTFILE,1," = 0x%x  ## fill ## ", sval );
      return i;
    }

  if (cnt <= 0 ) cnt = 1;
  if (cnt > 16) cnt = 16;      // 16 max in a line ?  6 better ?


  if (ofst + cnt > x->end) cnt = x->end - ofst +1;

  if (x->size)             //set if cmd not found
    pp_hdr (ofst, "???", cnt);
  else
    pp_hdr (ofst, cmd->string, cnt);     //fill

  return cnt;
}

// this works for actual structure - need to sort up/down etc

uint pp_timer(uint ofst, LBK *x)
{
  int val,cnt;
  int xofst;

  SYM *sym;
  short bit, i;
  ADT *a;
  CSTR *cmd;

  cmd = get_cstr(CMDSTR,0);

  sym = 0;
  a = get_adt(vconvi(x->start),0);

 // if (!a)  a = append_adt(vconvi(x->start),0);     // safety

  xofst = ofst;

  val = g_byte (xofst++);  // first byte

  if (!val)
    {
      pp_hdr (ofst,cmd[C_BYTE].string,1);    // end of list
      xprt(LSTFILE,0,"# Terminator", val);
      return 1;
     }

 // a->fnam = 1;                      // TEMP TEST !!

  cnt = (val & 1) ? 3 : 1;           // long or short entry
  pp_hdr (ofst, cmd[C_TIMR].string, bytes(a->fend) + cnt);
  xprt(LSTFILE,0,"%2x, ", val);
  val = g_val (xofst,0, a->fend);
//val = fix_sbk_addr(val);                    //2nd entry.
  if (a->fnam) sym = get_sym(val,a->fstart, a->fend,xofst);       // syname Timer AFTER any decodes
  if (sym) xprt(LSTFILE,0,"%s, ",sym->name);
  else xprt(LSTFILE,0,"%5x, ",val);

  xofst += bytes(a->fend);

  if (cnt == 3)
   {                // int entry
    i = g_byte (xofst++);
    bit = 0;
    while (!(i&1) && bit < 16) {i /= 2; bit++;}          // LOOP without bit check !!
    val = g_byte (xofst);
//val = fix_sbk_addr(val);
     if (a->fnam) {sym = get_sym(val,bit,bit,xofst);
     if (!sym) sym = get_sym(val,a->fstart, a->fend,xofst); }       // timer
    if (sym) xprt(LSTFILE,0,"%s, ",sym->name);
    else  xprt(LSTFILE,0," B%d_%x",bit, val);
  //  xofst++;                            // more later ??

   }

  return cnt+bytes(a->fend);
}



void prt_calctype(uint fno, uint type)
{
    // NOT the same as data type
    switch(type)
    {
        default :
         xprt (fno,0, "? ");
          break;

        case DP_DADD:
         xprt (fno,0, "address ( ");
         break;

        case DP_DEC:
         xprt (fno,0, "integer ( ");
         break;

        case DP_FLT:
         xprt (fno,0, "float ( ");
         break;


    }
}



void prt_calc(uint fno, void *fid, uint pix)
{

  MATHX *a;
  MNUM *n;
  CSTR *c;

  uint i;

  while ((a = get_mterm(fid,pix)))
   {

     fid = a;
     pix = 0;              //clear for rest of chain

     if (a->mfirst)
       {             // print calctype instead of function
                     // first term always a '+'
         prt_calctype(fno, a->calctype);
       }

     if (!a->lfirst)
      {
        c = get_cstr(MATHSTR,a->func);
        if (c) xprt (fno,0, " %s ", c->string);
      }
     if (a->fbkt)  xprt (fno,0,"(");           //func bkt

     for (i = 0; i < a->npars; i++)
       {
        n = a->fpar + i ;

        convert_par(a,n);   //  convert to calctype

        if (i)
          {
           xprt (fno,0, ", ");
          }

       switch(n->dptype)
       {
         default :
           break;

         case DP_FLT :
           prtfl(fno, n->fval, 0,3);
           break;

         case DP_REF:
           xprt (fno,0, "x");
        //   if (n->refno > 1)
        //   xprt (fno,0, "%d", n->refno);
           break;
         case DP_DADD:
           paddr(fno, n->ival, 0);
           break;


         case DP_HEX:
           xprt (fno,0, "%x", n->ival);
           break;

         case DP_DEC:
           xprt (fno, 0, "%d", n->ival);
           break;

         case DP_SUB:
             xprt (fno,0,"(");             //temp
             prt_calc(fno,fid,i+1);
             xprt (fno,0,")");
           break;
       }
 }
if (a->fbkt)  xprt (fno, 0,")");            //funcbkt

   }

}




void list_adnl(ADT *a, uint start, uint fcom)        ///void *fid
  {

  FKL *l, *j;
  MATHN *n;
  MATHX *x;
  ADT *b;            //temp...


   /// while ((a = get_adt(fid,0)))      if merged with below
   ///{

 //      if (a->fid == vconvi(0x93a12))
 //      {
   //        DBGPRT(0,0);
   //    }

         xprt(MSGFILE,0, " [");

       if (a->newl) xprt(MSGFILE,0,"| ");  // newline is for END of this block

       if (a->cnt > 1) xprt(MSGFILE,0,"O %d ", a->cnt);

       if (a->bank && a->bank != (start >>16)) xprt(MSGFILE,0,"K %x ", a->bank-1);

       list_cellsize(MSGFILE,a);

       prt_pfw(MSGFILE, a);                         // print fieldwidth if not default

       if (a->fnam) xprt(MSGFILE,0,"N ");

       l = get_link(CHADNL, a, 0, 0, 0);     // adt to calc by name or direct (by keyout)

       if (l)
        {
          xprt(MSGFILE,0," = ");

          if (l && l->chdst == CHMATHX)
           {
             x = (MATHX*) l->keydst;       // calc link  print calc

             j = get_link(CHMATHX, x, CHMATHN,0,0);     // adt to calc by name or direct (by keyout)
             //look for name here first
             if (j)
               {
                 n = (MATHN*) j->keydst;       // name link, print name
                 if (n) xprt(MSGFILE,0,"\"%s\" ", n->mathname);   //print only name from ADT
               }
             else
              {
                prt_calc(MSGFILE, x->fid,0);      //   print calc
                xprt(MSGFILE,0,") ");  }
              }
        }

       else prt_radix(MSGFILE,a,fcom);               //     if (!l)     // print radix if not default and no calcs

       // do subs as recursive loop

       b = get_adt(a,1);
       if (b)
         {
           void *fid;
           list_adnl(b, start, fcom);
           fid = b;
            while ((b = get_adt(fid,0)))
               {
                 list_adnl(b, start, fcom);  // ignore if zero size, and not debug
                 fid = b;
               }

         }


       xprt(MSGFILE,0, "]");

      ///     fid = a;        //if merged with below
///  }


    }


void list_adt(uint start, int fcom)
{
  // print additional params for any attached ADT chain, LBK mainly but also SUBs
  // the 'prt' argument allows print to debug or message files
  // fcom is command for default print types

ADT *a;
void *fid;

fid = vconvi(start);
//at this point, fid is always start address, could merge into above, with a small change


while ((a = get_adt(fid,0)))
  {
    list_adnl(a, start, fcom);  // ignore if zero size, and not debug
    fid = a;
  }
}



















void list_glo(uint fno, SUB *sub)
{
   SPF *s;
   int x;
   CSTR *cmd;

  cmd = get_cstr(SPFSTR,0);
  // print special func in style of adt
  x = 0;

  if (!get_cmdopt(OPTCMPA) && sub->cptl)
       {
        xprt(fno,0," $ C");
        x = 1;
       }

  s = get_spf(vconvi(sub->start));

  if (!s) return;          // no special funcs

  if (!x) xprt(fno,0," [$");

if (fno == MSGFILE)
{
   if (s->spf == 4)
   {
    x = 1;                        //  base of func names
    if ((s->fendin  & 31) > 7)   x += 4;   //  word row
   }
   if (s->spf == 5)
   {
   x =  9;
   if ((s->fendin  & 31) > 7)   x += 2;   //  word row
   }
   if (s->fendout & 32) x += 1;          // sign out
   if (s->fendin & 32)  x += 2;          // sign in

  xprt(fno,0," F %s %x", cmd[x].string      , s->addrreg);

  // tab subroutines need extra par
  if (s->spf == 5) xprt (fno,0," %x", s->sizereg);
   xprt(fno,0," ]");
}
else
{     // debug
xprt(fno,0, "F %d,  %x, %x  ", s->spf, s->addrreg, s->sizereg);
xprt(fno,1, "I %x, O %x,  (%x)",s->fendin, s->fendout, s->fromadd);

}



}


void list_rbt (uint fno)
{
  RBT *r ;
  uint ix;
  CHAIN *x;

  x = get_chain(CHBASE);
 if (fno != MSGFILE)
   {
    xprt(fno,0,"# ------------ Rbase list----------");
    xprt(fno,0, "%d items", x->num);
    xprt(fno,1,0);
   }


  for (ix = 0; ix < x->num; ix++)
   {
    r = (RBT*) x->ptrs[ix];


// if (get_flag(o->addr, rbinv)) return;    // already marked as invalid

        if (fno == MSGFILE)
          {
           if (!r->oinv)
             {      //not invalid or flagged invalid
              xprt(fno,0,"rbase %x " , r->reg);
              paddr(fno,r->val,0);
              if (r->rstart)
               {
                xprt(fno,0,"  ");
                paddr(fno,r->rstart,0);
                xprt(fno,0,"  ");
                paddr(fno,r->rend,0);
               }
              if (r->usrcmd) list_usrcmt(fno);
              xprt(fno,1,0);
            }
         }
        else
          {

           xprt(fno,0,"rbase %x %x" , r->reg , r->val);

   //        prt(0, "flg %d", get_rbflg(r->reg));
       //    prt(0,"  %x %x  (%x %x %x)", r->rstart, r->rend, r->chaddr[0], r->chaddr[1], r->chaddr[2]);
           xprt(fno,0,"  %x %x  (%d)", r->rstart, r->rend, r->ucnt);
           xprt(fno,0, "setat %x, invat %x", r->setat, r->invat);
           if (r->oinv) xprt (fno,0, " INV");
           if (r->usrcmd) xprt(fno,0," cmd");
            if (r->lock)  xprt(fno,0," rb sig");
        xprt(fno,1,0);
          }
   }
    xprt(fno,1,0);
}

void list_scans(uint fno)
{
  // not merged with DBG version, as it's very different.
  uint ix;
  SBK *s;
  CHAIN *x;

  x = get_chain(CHSCAN);

   for (ix = 0; ix < x->num; ix++)
    {
     s = (SBK*) x->ptrs[ix];
     if (s->usercmd) {xprt(fno, 0,"scan ");
     paddr(fno,s->start,0);
     xprt(fno,1,0);}
    }
 xprt(fno,1,0);
}

void list_psw (uint fno)
{
  PSW *r ;
  uint ix;
  CHAIN *x;

  x = get_chain(CHPSW);

  for (ix = 0; ix < x->num; ix++)
   {
    r = (PSW*) x->ptrs[ix];

    xprt(fno,0,"pswset ");
    paddr(fno,r->jstart,0);
    xprt(fno,0,"  ");
    paddr(fno,r->pswop ,0);
    xprt(fno,1,0);
   }
     xprt(fno,1,0);
}



void list_banks(uint fno)
{
  int i;
  BANK *x;

  // bank no , file offset,  (opt) pcstart, (opt)  pcend  (opt) end file offset, (opt) fillstart

  if (fno == MSGFILE)
  {
   xprt(fno,2,"# Banks Found.  For information, can uncomment to manually override");
  // prt(1,"# bank_num, file_offset, start_addr, end addr, [filler_start]");

   for (i = 0; i < BMAX; i++)
    {
     x = get_bankmap(i);                  // bkmap+i;
     if (x->bprt)
       {
        xprt(fno,0,"# bank  %d %5x ", x->dbank-1, x->filstrt);
        paddr(fno,x->minromadd,0);
        xprt(fno,0," ");
        paddr(fno,x->maxromadd,0);
        if (x->usrcmd) list_usrcmt(fno);

        if (!x->bok) xprt (fno,0,"** ignored **");
        xprt(fno,1,0);
       }
    }
   xprt(fno,1,0);
  }

#ifdef XDBGX
else
  {
 //  debug printout

  DBGPRT(1,0);
  DBGPRT(1,"-------DEBUG BANKS---------");
  DBGPRT(1,"bank filstart, minpc, maxbk, maxpc, filend. bok,cmd,bkmask, cbnk, P65 dbank" );

  for (i = 0; i < BMAX; i++)
    {
      x = get_bankmap(i);              //bkmap + i;
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
      x = get_bankmap(i);                    //bkmap + i;
      if (!x->bok && x->minromadd)
        {
      DBGPRT(0,"%2d %5x %5x %5x  (%5x)", i, x->filstrt, x->minromadd, x->maxromadd, x->filend);
         DBGPRT(1,"  %x %x %x %x %x %x", x->bok, x->usrcmd,x->bkmask, x->cbnk, x->P65, x->dbank);
        }
     }
 DBGPRT(1,0);
}

#endif

}


void list_cmd(uint fno,LBK *k)
{
  DIRS *d;
  uint f;
  CSTR *c;

  d = get_dirs(k->fcom);
  c = get_cstr(CMDSTR,k->fcom);

  // print single LBK command

     if (fno == MSGFILE)
       {         // real printout
         if (nobank(k->start) < PCORG) return;    // safety
         if (k->fcom == C_TIMR) xprt (fno,0,"# ");
         xprt(fno,0,"%-7s ", c->string);
         paddr(fno,k->start,0);
         xprt(fno,0,"  ");
         paddr(fno,k->end  ,0);
       }
     else
      {   // debug printout
          xprt(fno,0,"%-7s %x %x", c->string, k->start, k->end);
      }

   // global options here

   f = 0;
   if (k->term)
      {
       xprt (fno,0,"  [$");
       xprt (fno,0," Q");
       if (k->term > 1) xprt(fno,0," %d", k->term);
       f = 1;
      }

   if (d->cmpargs)
    {
        //args option in force
      if (get_cmdopt(OPTCMPA) != k->cptl)
          {
            if (!f) xprt (fno,0,"  [$");
            if (k->cptl) xprt (fno,0," C"); else xprt (fno,0," A");
          }
    }

  if (d->cmpdata)
    {
        //args option in force
      if (get_cmdopt(OPTCMPD) != k->cptl)
          {
            if (!f) xprt (fno,0,"  [$");
            if (k->cptl)  xprt(fno,0," C"); else xprt(fno,0," A");
          }
    }


   if (f) xprt (fno,0," ]");
   list_adt(k->start,k->fcom);

   if (k->usrcmd) { list_usrcmt(fno);}
   xprt(fno,1,0);
  }



void list_cmds(uint fno)
{
    // command printer for both cmd and auxcmd, debug or real
    // do both chains in one pass in start order

 uint ixc, ixa;
 LBK *cmd, *aux;
 CHAIN *ccmd, *caux;

 ixa = 0;
 aux = 0;

 ccmd = get_chain(CHCMD);
 caux = get_chain(CHAUX);

if (caux->num > 0) aux = (LBK*) caux->ptrs[ixa];           // first aux (what if there isn't any ??

 for (ixc = 0; ixc < ccmd->num; ixc++)
    {
     cmd = (LBK*) ccmd->ptrs[ixc];

     while (aux && aux->start <= cmd->start)
        {
         list_cmd(fno,aux);
         ixa++;
         if (ixa < caux->num) aux = (LBK*) caux->ptrs[ixa];
         else aux = 0;
        }

     list_cmd(fno,cmd);

    }
    xprt(fno,2,0);
}



void list_subs (uint fno)
{
  SUB *s;
  SYM *x;
CHAIN *c;
 // ADT *a;
  uint ix, go;

  xprt(fno,1,"# ------------ Subroutine list----------");

  c = get_chain(CHSUBR);

  for (ix = 0; ix < c->num; ix++)
      {
        s = (SUB*) c->ptrs[ix];

        // decide on print
        if (fno == MSGFILE)
           {
            go = 0;
            if (s->usrcmd)  go = 1;
            if (!go && get_spf(vconvi(s->start))) go = 1;
            if (!go && get_adt(vconvi(s->start),0)) go = 1;
       //     if (!go && get_adnl(&chans,s->start,1))  go = 1;
           }
        else go = 1;

        if (go)
          {
           if (fno == MSGFILE) {
               xprt(fno,0,"sub  ");
               paddr(fno,s->start,0);
               xprt(fno,0,"  ");
           }
           else xprt(fno,0,"sub  %x ", s->start);
// xprt(fno,0,"sub  %x %x  ", s->start, s->end);
           x = get_sym(s->start,0,C_NOBIT|7,0);             //subname
           if (x)
             {
              xprt(fno,0,"%c%s%c  ", '\"', x->name, '\"');
              if (fno == MSGFILE) x->noprt = 1;        // printed sym, don't repeat it
             }

           list_glo(fno,s);
           list_adt(s->start,C_ARGS);
           if (s->usrcmd)  list_usrcmt(fno);
           xprt(fno,1,0);
          }
      }

   xprt(fno,2,0);
  }



void list_syms (uint fno)       //void)
{
 SYM *t;
 CHAIN *x;
 uint ix, b;

 xprt(fno,1,"# ------------ Symbol list ----");

 x = get_chain(CHSYM);

 for (ix = 0; ix < x->num; ix++)
   {
   t = (SYM*) x->ptrs[ix];

   // decide on print
   if (fno == !MSGFILE || !t->noprt)
     {

      if (fno == MSGFILE)
         {
          xprt(fno,0,"sym ");
          paddr(fno,t->addr,0);
          xprt(fno,0,"  ");
          if (t->rstart)
           {
            paddr(fno,t->rstart,0);
              xprt(fno,0,"  ");
            paddr(fno,t->rend,0);
           }
         }
      else
        {         // debug
         xprt(fno,0,"sym %4x ", t->addr);
         xprt(fno,0," %5x %5x ", t->rstart, t->rend);
         xprt(fno,0,"  B %2x %3x" , t->fstart, t->fend);
     //    if (t->nobitf) xprt(fno,0, " NOBIT"); else xprt(fno,0,"      ");
         if (t->pbkt)  xprt(fno,0," bkt"); else xprt(fno,0,"    ");
         if (t->fend & C_NOBIT)  xprt(fno,0," NoBIT"); else xprt(fno,0,"      ");
     //    if (t->olap)  xprt(fno,0," OLP"); else xprt(fno,0,"    ");
                 if (t->test)  xprt(fno,0," TST"); else xprt(fno,0,"    ");
         xprt(fno,0," mk %x", t->fmask);


        }

     xprt(fno,0," \"%s\"",t->name);

     if (t->pbkt ) xprt(fno,0," [");    //|| t->pbits

     b = t->fend & 0x1f;
     if (t->fstart || b != 7)              //!(t->fend & C_WHOLE))
       {
        xprt(fno,0," B %d" , t->fstart );
        if (b != t->fstart || fno != MSGFILE) xprt(fno,0, " %d", b);
       }
  //   else

     if ((t->fend & 0x40))  xprt(fno,0," W");
 //    if (t->pbkt)  xprt(fno,0," bkt"); else xprt(fno,0,"    ");
 //  if (t->flags) xprt(fno,0," F");
   //  if (t->names) xprt(fno,0," N");
     if (t->immok) xprt(fno,0," I");
      if (t->pbkt ) xprt(fno,0," ]");
  //   if (t->usrcmd) list_usrcmt(fno);
  if (t->sys) list_syscmt(fno);
     xprt(fno,1,0);
     }
   }
  xprt(fno,1,0);
 }

void list_calctype(uint fno, uint type)
{
    switch(type)
    {
        default :
         xprt (fno,0, "?");
          break;

        case DP_DADD:
         xprt (fno,0, "address");
         break;

        case DP_DEC:
        xprt (fno,0, "integer");
         break;

        case DP_FLT:
         xprt (fno,0, "float");
         break;
    }

    xprt (fno,0, " ( ");
}


void list_calc (uint fno, void *fid, uint pix)
{

  MATHX *a;
  MNUM *n;
  CSTR *c;

  uint i, dp;

  a = (MATHX*) fid;

 // while ((a = get_mterm(fid,pix)))

 while (a)
   {

     fid = a;
     pix = 0;              //cleared for rest of chain

     if (a->mfirst)
     {
       list_calctype(fno,a->calctype);
     }

     if (!a->lfirst)
       {
         c = get_cstr(MATHSTR, a->func);
         if (c) xprt(fno, 0, " %s ", c->string);
       }
     if (a->fbkt)  xprt (fno,0,"(");           //func bkt

for (i = 0; i < a->npars; i++)
 {
    n = a->fpar + i ;

    convert_par(a,n);   //  convert to calctype

  if (i)
    {
      xprt (fno,0, ", ");
    }
    switch(n->dptype)
       {
         default :
           break;

         case DP_FLT :

         // get default decimal points ....
            dp = get_cmdopt(OPTDPL);
           prtfl(fno,n->fval, 0,dp);
           break;

         case DP_REF:
           xprt (fno,0, "x");
        //   if (n->refno > 1)
        //   prt(0, "%d", n->refno);
           break;
         case DP_DADD:
           paddr(fno, n->ival, 0);
           break;


         case DP_HEX:
           xprt(fno,0, "%x", n->ival);
           break;

         case DP_DEC:
           xprt (fno,0, "%d", n->ival);
           break;

         case DP_SUB:
             xprt(fno,0,"(");             //temp
        //   (a = get_mterm(fid,pix)))        //must get subterm first !!
         //        list_calc(fno,a,i+1);           //??
             list_calc(fno,get_mterm(a,i+1), i+1);
             xprt(fno,0,")");
           break;
       }
 }          //npars
if (a->fbkt)  xprt (fno,0,")");            //funcbkt



     a = get_mterm(a,pix);
   }        //end while

}




void list_calcs(uint fno)

{
  MATHN *n;
  uint ix;
  CHAIN *ch;
  FKL *l;

  ch = get_chain(CHMATHN);

  xprt(fno, 1, "-- calcs list --");   //works for add_encode....

 for (ix = 0; ix < ch->num; ix ++)
 {
    n = (MATHN *) ch->ptrs[ix];      //by name in top loop

    xprt (fno, 0, "calc \"%s\" =  ", n->mathname);

    l = get_link(CHMATHN,n,CHMATHX,0,1);
    if (l) list_calc(fno,l->keydst,0);              //else ??
    xprt(fno, 0," )");
    if (n->sys)      xprt(fno, 0,"             # Auto Added by SAD ");
    xprt(fno,1,0);
 }

xprt(fno, 2,0);
}






void list_dirs(uint fno)
{                 // print all directives (msg file)
  list_opts (fno);
  list_banks(fno);
  list_calcs(fno);
  list_rbt  (fno);
  list_psw  (fno);
  list_scans(fno);
  list_cmds (fno);
//  prt_cmds (&chaux, wnprt);
  list_subs (fno);
  list_syms (fno);



}


// print routines (mostly) called by command.

void cmtwrapcheck(char *c)
{
  char *t;
  uint col;

 FDATA *fl;

  fl = get_fldata(LSTFILE);

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
  col += fl->pcol;   // where next 'word' would end

  if (col >= WRAPPOSN)
    {
     xwrap(LSTFILE,CMNTPOSN);
    }
}









void xplus  (MHLD* h)
{
  // if (!x->calctype) return;
  uint bank;
   if (h->calctype == DP_FLT)  h->total.fval += h->par[0].fval;               // float calc
   else
     {
      bank = g_bank(h->total.ival);         // save bank
      h->total.ival += h->par[0].ival;

      if (h->calctype == DP_DADD)
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

      if (h->calctype == DP_DADD)
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

      if (h->calctype == DP_DADD)
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
      bank = g_bank(h->total.ival);         // save bank (but what if no bank ?)
      h->total.ival /= h->par[0].ival;

      if (h->calctype == DP_DADD)
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
        if (!valid_reg(val)) val |= (h->bank << 16);
        h->total.ival = val;
        h->total.dptype = DP_DADD;
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

    r = get_rbt((h->par[2].ival & 0xff) + rb, h->ofst);
    if (r)   val = off + r->val;        // reset op

    else
    {
            #ifdef XDBGX
    DBGPRT(1,"%x No rbase for address decode! %x %x = %x", h->ofst, h->par[2].ival, rb, (h->par[2].ival & 0xff) + rb);

    #endif
 //  }


 // if (valid_reg(val)) val |= (h->bank << 16);
     // g-bank(a->addr);  ?
   }

// if (val > max_reg()) val |= g_bank(a->data);
   // if (!valid_reg(val)) val -= 0x10000;

 //   ) < 0x400) val = nobank(val) | 0x80000;    //   force bank 8 for regs
    h->total.ival = val;
    h->total.dptype = DP_DADD;
 // return val;
}





//}


void xpwr   (MHLD *h) {

   if (h->calctype >= DP_FLT) h->total.ival = pow(h->total.ival,h->par[0].ival);

else
{ // do an in version ??)
   if (h->calctype == DP_DADD) h->total.ival &= 0xffff;           //address
}

    }

 // temp empty procs
void xroot   (MHLD *){}
void xsqrt  (MHLD *){}
//void xoff   (MHLD *){}
void xvolts  (MHLD *){}
void xbnd   (MHLD *){}



void p_sign(COPER *o, PP_OP *p)
{
  uint i;

  if (p->sign)
    {                        //do sign and size
      i = o->fend;

      if (i & 0x20)  xprt(LSTFILE,0,"s");
      i &= 31;
      if (i < 8)  xprt(LSTFILE,0,"y");
      else if (i < 16)  xprt(LSTFILE,0,"w");
      else if (i < 24)  xprt(LSTFILE,0,"t");
      else xprt(LSTFILE,0,"l");
    }

}


void p_sc (COPER *o, PP_OP *p)           //CINST *x, int ix)
{
// single op print for source code for instance 'c' and ix
// symbol first, then by type

  int v;
 // COPER *o;
 // PP_OP *p;

 // o = c->opnd + ix;
 // p = psym + ix;

  if (p->noprt) return;

  if (p->opsym)
    {
     if (o->optype == OPOFF) pchar(LSTFILE,'+');                    // if offset with symname

     if (o->optype == OPBIT && (p->opsym->fend & C_NOBIT))  xprt(LSTFILE,0,"B%d_", o->addr);
     xprt(LSTFILE,0,"%s", p->opsym->name);
     if (o->optype == OPAINC) xprt(LSTFILE,0,"++");
    return;
      }


// switch ?


 if (p->poptype == OPIMD)                               // immediate operand
    {
     xprt(LSTFILE,0,"%x", o->addr & get_sizemask(o->fend));
     return;
    }



 if (p->poptype == OPBIT)          //and no sym
     {
       xprt(LSTFILE,0,"B%d_R%x", o->addr, o->reg);
       return;
     }

 if (p->poptype == OPADDR)
    {
     paddr(LSTFILE,o->addr,0);
     return;
    }

if (p->poptype == OPOFF)
    {            //offset address - signed
     v = o->val;
      //     pchar(LSTFILE,0x20);
     if (v < 0)
     {
      v = -v;
      pchar(LSTFILE,'-');
     }
     else pchar(LSTFILE,'+');
     //     pchar(LSTFILE,0x20);
     // may need more smarts here
     // if single bank or < register
     if (!get_numbanks() || nobank (v) < 0x400) v &= get_sizemask(15);
     paddr(LSTFILE,v,0);
     return;
    }

 //   default            if (o->rgf)
  //  {

     if (valid_reg(o->reg) != 2 || (o->fend & 0x40)) xprt(LSTFILE,0,"R");           // non zero reg, or write op

     xprt(LSTFILE,0,"%x", o->reg);
      if (o->optype == OPAINC) xprt(LSTFILE,0,"++");
  //   return;
 //   }



}




void p_opsc (CINST *c, PP_OP *x, uint ix)
{
 // source code printout
 // prefix sign and size markers if flag set.
 // and add term for indexed

 //ix is 0-3 or 10-13 ?

  COPER *o;
  PP_OP *p;      //, *x;

  if (!c) return;
  if (!x) return;


  ix &= 3;

  o = c->opnd + ix;                            // set operand
  p = x + ix;

  p_sign(o,p);

  if (p->bkt)  pchar(LSTFILE,'[');
  p_sc (o,p);
  if (o->optype == OPINX) p_sc (c->opnd, x);  // indexed, extra op
  if (p->bkt)  pchar(LSTFILE,']');
  return;
}










void prt_cmnt(CPS *c, uint flags)
{
  // process any special chars in a comment line
  // use CPS struct here for getpx and getpd etc.

  int chars, ans;
  char *t;
  SYM *x;
  FDATA *fl;

  fl = get_fldata(LSTFILE);

   if (!c->cmpos) return ;
   if (*c->cmpos == '\0') return ;        // safety

   /*check for split comment or newline here ??
   // mark a '|' or '\|' here for split comment ?? c->bits ?? but need it in ptr_cmt as well for multiples.

    c->argl = 0;          //newline
      while (*c->cmpos == 0x20) c->cmpos++;     // consume any spaces after number
      t = c->cmpos;
      if (*t == '\\' && *(t+1) == 'n') c->argl = 1;    // newline at front of cmt
      // mark a '|' or '\|' here for split comment ?? c->bits ?? but need it in ptr_cmt as well for multiples.
      //end move

// c->argl for newline ??
*/

   if (*c->cmpos != '\\' )  p_pad(LSTFILE,CMNTPOSN);  // no special chars at front, tab to cmnt position

   while (c->cmpos < c->cmend)
    {
     if (c->split) break;           // a '|' has happened

     switch (*c->cmpos)
       {   // top level char check

         default:
           pchar(LSTFILE,*c->cmpos);                     // print anything else - auto wraps
           cmtwrapcheck(c->cmpos);               // wrap check for whole word.
           c->cmpos++;
           break;

         case 1:                          //  for autocomment embedded - NOTE not chars !!
         case 2:
         case 3:
           p_opsc (c->cinst, csym, *c->cmpos);            // links to cinst  (c->cinst,*c->cmpos);
           break;

         case '\n' :                      // '\n',  ignore it
         case '\r' :
         c->cmpos++;
         break;


         case '\\' :                // special sequences

           if (c->cmpos == c->cmend) pchar(LSTFILE,*c->cmpos);        //print backslash if end of line

           c->cmpos++;                     // skip the backslash, process next char.
           if (c->cmpos >= c->cmend) break;


           //rs = toupper(*c->cmpos);
           switch (toupper(*c->cmpos))
             {  // special sequences

               default:
                 pchar(LSTFILE,*c->cmpos);
                 break;

// more opcode/operand options ??

               case 0x30:                  //allow zero - NOT TESTED !!
               case 0x31:
               case 0x32:                 // for user embedded operands 1-3
               case 0x33:

                 ans = *c->cmpos - 0x30;   // char to number
                 if (ans >= 0 && ans <= 3)
                   {
                    p_opsc(c->cinst, csym, ans);
                   }
                   c->cmpos++;
                 break;

               case 'N' :                //newline
                 xprt(LSTFILE,1,0);
                 c->cmpos++;
                 break;

               case 'S' :                                // 'S' symname with no padding

             //    col = gcol;                             // where print is now.

                 c->cmpos++;            //skip the 's'

                 x = NULL;
                 t = c->cmpos;         // remember where

                 if (toupper(*c->cmpos) == 'W') {c->p[7] |= 64; c->cmpos++;}  // write sym

                 ans = getpx(c,1,1);
                 if (fix_input_addr_bank(c,1)) ans = 0;
                 if (ans)
                   {
                    c->p[2] = 0;
                    if (*c->cmpos == ':')
                     {
                      c->cmpos++;         //skip the colon
                      ans = getpd(c,2,1);                       //bit number
                      if (c->p[2] > 31) c->p[2] = 31;
                      if (ans < 2) c->p[3] = c->p[2];        //one number, end = start
                     }
                    else
                    {
                     c->p[2]  = 0;             //no bits, default to byte
                     c->pf[2] = 0;             //reset size to zero
                     c->p[3]  = 7;              //|C_NOBIT ??;
                    }

                    c->p[3] |= c->p[7];

                   c->p[4] = 0;
                    if (*c->cmpos == '@')
                     { //range address
                       ans = getpx(c,4,1);                       //range
                       if (!ans) c->p[4] = 0;
                     }

                    if (!c->p[4])
                      {
                       if (c->cinst) c->p[4] = c->cinst->ofst;    //get range ofst from code
                       else c->p[4] = c->p[0];       //address of the comment
                      }

                    x =  get_sym (c->p[1], c->p[2], c->p[3], c->p[4]);

                 if (x)
                     {
                      if (c->pf[2] && (x->fend & C_NOBIT)) xprt(LSTFILE,0,"B%d_",c->p[2]); // not right for WORDS ?
                      xprt(LSTFILE,0,"%s",x->name);
                //      if (col > gcol)  p_pad(LSTFILE,col);           // pad to col (from above)
                     }
                   }         //end of first number read in

                 if (!x)
                   {        //no sym or addr incorrect

                     pchar(LSTFILE,'?');  // failed sym read, print ??? and sequence ??for Bx_Rn dsiplay
                     while (t < c->cmpos) pchar(LSTFILE,*t++);
                     pchar(LSTFILE,'?');
                   }  //


//if field read, print ??

              //   c->cmpos--;         //= c+chars-1;                                   // skip the sequence + whatever read
       //        }
//}
                 break;

               case 'T' :
         //      case 't' :
                                // tab to 'x'
                 c->cmpos++;            //next char
                                  if (c->cmpos >= c->cmend) break;
                 chars = 0;

                 ans = getpd(c,1,1);         //sscanf(c,"%u%n",&addr,&chars);
                 if (ans > 0) { if (c->p[1] < (int) WRAPPOSN)  p_pad(LSTFILE,c->p[1]); else xwrap(LSTFILE,CMNTPOSN);}

          //       c->cmpos = c->cmpos+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'R' :        // runout
                 c->cmpos++;
                 if (c->cmpos >= c->cmend) break;
                 chars = 0;
                 c->p[2] = ' ';     //space is default

                 if (!isdigit(*c->cmpos))
                   {
                    c->p[2] = *c->cmpos;   //new runout char
                    c->cmpos++;
                   }
                 ans = getpd(c,1,1);
                 if (ans <= 0) c->p[1] = 1;
                 p_run(LSTFILE,0,c->p[2], c->p[1]);
             //    c->cmpos = c->cmpos+chars-1;                                   // skip the sequence + whatever read
                 break;

               case 'M' :      //next tab posn
           //      chars = gcol;       //current col
                 chars = fl->pcol/4 * 4;     //nearest prev tab
                 chars += 4;
                 p_pad(LSTFILE,chars);
                 c->cmpos++;
                 break;


               case '\n' :                               // '\n',  ignore it
               case '\r' :
               c->cmpos++;
                 break;


              case 'W' :     // wrap comment, or pad to comment column
                 if (fl->lastpad >= CMNTPOSN) xwrap(LSTFILE,CMNTPOSN);
                 else  p_pad(LSTFILE,CMNTPOSN);
                 c->cmpos++;
                 break;



              case '|'  :        // multiple comment lines with bitnames

                 // break loop, wait for next call
                 if (flags & 2) c->split = 1;
                 c->cmpos++;
                 break;

             }   // end switch for '\' sequences



// case '[' :
// new sequences  - swapped to command reader






       }        // end switch for char

    }


}





//void xdmy   (MHLD *h)
//{}


void pp_comment (uint ofst, uint flags)
{
  // ofst is the START of an opcode statement so comments
  // less than ofst should be printed.

  //cmnd struct cleared only at start of listing....

//  if (anlpass < ANLPRT) return;        // safety check

 // flags = 0 is default - print whole line after opcode
 // if (flags&1) set, then add a newline.
 // if (flags&2) set, stop on a '|' char  (split comment for bit names)

 // NB. a newline between commands should be AFTER any trailing comments (i.e. not beginning with newline)
 // but BEFORE ones any that do. Therefore, if end set, break at first cmnt with newline at front
 // and remember if newline printed so multiple 'end' set doesn't cause multiple newlines
 // clear flag if end not set




   if (flags & 2) ofst++;                         // for intra opcode (bit names)

   while ((uint) pcmd.p[0] < ofst)
       {
        if ((flags & 1) && pcmd.newline) break;   // newline + flag - do on NEXT call
        prt_cmnt(&pcmd, flags);                   // print it
        if (!get_cmnt(&pcmd)) break;              // get next comment if current one done
       }

   if (flags & 1)
      {                 //print newline
       if (!lastcmt)
         {
           xprt(LSTFILE,1,0);
           lastcmt = 1;    // remember
         }
       }
   else lastcmt = 0;      //clear if not end
}











int pp_adt (int ofst, ADT *a, uint *pitem)
 {

// print a SINGLE ITEM in listing from *a, even if a->cnt is set
// done this way for ARGS printouts.

  int pfw;
  SYM* sym;
  MHLD *h;

  pfw = 1;                               // min field width
  sym = 0;

  // pfw is still not always right in a struct, especially if names appear.
  // also args always needs min fieldwidths, so allow for a->pfw of zero
  // can't do this if calc involved !!

  if (pitem)
    { pfw = plist[*pitem]; (*pitem)++; }  //get from preset list
  else
   {
    if (a->pfw)
     {  // Not set for ARGS, default to preset if too small.
      pfw = get_pfwdef(a);

        if (a->pfw > pfw) pfw = a->pfw;

  //    if (a->pfw < val) pfw = val;
  //    else pfw = a->pfw;
     }
  }

  h = do_adt_calc(a, ofst);                          // this calc can change print mode

  if (a->fnam)
     {
      uint j;
      j = h->total.ival | (a->bank << 16);       //add bank for sym names

      sym = get_sym(j,a->fstart, a->fend,ofst);             // READ sym AFTER any decodes (ADT)
      if (sym)
        {
         xprt(LSTFILE,0,"%*s",pfw,sym->name);
         mfree(h, sizeof(MHLD));                 //must free calc holder.
         return bytes(a->fend);
        }
     }

  if (!sym)
    {

  switch (a->dptype)
   {
      case DP_DADD :
      case DP_CADD :

           // need pfw to line up columns

          paddr(LSTFILE, h->total.ival, pfw);


        break;


      default:          //but no symname find ????

        xprt(LSTFILE, 0,"%*x", pfw, h->total.ival);                  // *pfw, testdefault hex  (case 0 and 1
     //    pstr(0,"%-5x", h->total.ival);                  // *pfw, testdefault hex  (case 0 and 1
        break;

      case DP_BIN :
        pbin(LSTFILE, h->total.ival, a->fend);                        // binary print, need size.
        break;

      case DP_DEC:
        xprt(LSTFILE, 0,"%*d", pfw, h->total.ival);                    // decimal print, need full width if negative
        break;

      case DP_FLT:

   //   if (!a->pfd)  a->pfd += 3;                               // TEMP FIX !!
        prtfl(LSTFILE, h->total.fval,pfw,a->pfd);               //  float
        break;
   }
}
  mfree(h, sizeof(MHLD));  //free calc holder
  return bytes(a->fend);
}






int pp_lev (void **fid, uint bix, uint ofst, uint *colno)
{


  // ALWAYS stops at a newline.

// need to add CUTOFF at an end point so it does not overrun
// unless command verifies and fixes size first (better)

//start indent at plist[1] and go up from there.
// in here is Ok as it always does one row

// RECURSIVE for sub chains

 uint i, lx;
 ADT *a;
 void *sfid;      //sub fid

 i = 0;
 lx = bix;

 while ((a = get_adt(*fid,lx)))
  {
   lx = 0;                  //only first item ever has non zero

   if (i) { if (bix) xprt(LSTFILE,0,"|");  else xprt(LSTFILE,0,", "); }  // TEMP

   for (i = 0; i < a->cnt; i++)              // count within each level
    {
     if (i) xprt(LSTFILE,0,", ");

     // subfields check
     sfid = a;
     pp_lev (&sfid, 1, ofst, colno);       // any sub fields ??
     pp_adt(ofst, a, colno);               // field width is plist[column]

     if (!bix) ofst += bytes(a->fend);   //don't inc for subfields
    }

   *fid = a;          //done print, move to next fid

   if (a->newl)
    {  //newline flag, stop at this item, reset count for column
      if (colno) *colno = 1;    // restart column pad markers
      pchar(LSTFILE,',');
      break;
    }

  }      // end of while (a)

if (bix && !lx) xprt(LSTFILE,0,"|");         //TEMP
  // return size ?
  return 0;
}



//***********************************
// byte, word, long  - change name position
//***********************************




uint pp_wdbl (uint ofst, LBK *x)
{
  ADT *a;
  SYM *s;
 CSTR *cmd;



 //char *tt;
 // DIRS *d;

  // local declares for possible override
  uint com, fend, pfw, val;
 // uint itn;
  void* fid;


 //itn = 1;

  a = get_adt(vconvi(x->start),0);

  cmd = get_cstr(CMDSTR,0);       //x->fcom);
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
  pp_hdr (ofst, cmd[com].string, bytes(fend) | P_NOSYM);

  //if (pfw < cnv[cafw[d->defsze])  pfw = cafw[d->defsze]; // not reqd ??
 fid = vconvi(x->start);
  if (a) pp_lev(&fid, 0, ofst, 0);       //&itn);        //0, (uint)1);       // use addnl block(s)
  else
   {
     // default print HEX
     val = g_val (ofst,0, fend);
     xprt(LSTFILE,0,"%*x", pfw, val);
   }

  // add symbol name in source code position
  // but this should only be for SINGLE entries ?

  s = get_sym(ofst, 0 , C_NOBIT|fend, ofst);      //word/byte

  if (s)
    {
      p_pad(LSTFILE,PSCEPOSN);
     xprt(LSTFILE,0,"%s", s->name);
    }

  return bytes(fend);           //ofst += bytes(fend);

 // return ofst;
}



int pp_cmpl(uint ofst, void *fid, int fcom)
{
  // print 'compact' layout, all args on one line
  // unless | char, when it's split

  uint tsize, size, end, itemno;
  FDATA *fl;
  CSTR *cmd;
  fl = get_fldata(LSTFILE);
  cmd = get_cstr(CMDSTR,0);
  tsize = totsize(fid);
  end = ofst + tsize;

// this is stct format, need a different one for subr (with args afterwards)

 itemno = 1;

 while (ofst < end)
  {      //for newline split............
   size = listsize(fid);

  if (fcom != C_ARGS)
   {
    pp_hdr (ofst,0,size);        // no title
    p_pad(LSTFILE,plist[0]);
    xprt(LSTFILE,0,"%s", cmd[fcom].string);
    p_pad(LSTFILE,OPNDPOSN);
    pp_lev(&fid,0,ofst, &itemno);    // continue from newline
    ofst += size;

    if (fl->pcol >= CMNTPOSN)  p_pad(LSTFILE,plist[31]);

    if (ofst < end) pp_comment (ofst,0);    // NOT last item
   }
  else
   { // arguments for subr on same line, no split or fieldwidth
    pp_lev(&fid,0,ofst, 0);
    ofst += size;
   }
  }

 if (!tsize) return 1;

 return tsize;
}




int pp_argl (uint ofst, void *fid, int fcom, int *argno)
{

// Print 'argument' layout, 1 arg per line
// can get SUB or LBK as fid

  int i, size;
  uint tend;
  ADT *a;
 CSTR *cmd;

 cmd = get_cstr(CMDSTR,0);
 pp_comment (ofst,0);            //for any comments BEFORE args

 size = totsize(fid);
 tend = ofst + size - 1;  // where this 'row' ends

 while ((a = get_adt(fid,0)))             //next_adnl(&chadnl,a)))
  {
   for (i = 0; i < a->cnt; i++)             // count within each level
    {
     pp_hdr(ofst, 0, bytes(a->fend));
     p_pad(LSTFILE,MNEMPOSN+6);

     xprt(LSTFILE, 0, "#%s", fcom == C_ARGS ? "arg" : cmd[fcom].string);
     xprt(LSTFILE, 0, " %d", (*argno)++);

     p_pad(LSTFILE,PSCEPOSN+2);
     a->pfw = 0;                         // no print field in argl
     pp_adt(ofst, a,0);

     ofst += bytes(a->fend);
     if (!bytes(a->fend)) ofst++;     // safety

     if (ofst <= tend)
       {
         pchar(LSTFILE,',');
         pp_comment (ofst,0);
       }
    }
   fid = a;
   if (a->newl) break;       //this breaks seq no ??
  }

  return size;
}



int pp_subargs (CINST *c)      //, int ofst)
{
  // ofst is effectively nextaddr (from INST)

  // change pp-lev for pp_args to do
 // a line by line print of attached arguments

  SUB  *xsub;
  LBK  *k;

  int size, ofst, xofst, argno,postprt;


  size = 0;
  argno = 1;
  postprt = 0;

  if (get_cmdopt(OPTSRC))   xprt(LSTFILE,0," (");   // args open bracket

  ofst = c->ofst + c->opsize;
  xofst = ofst;

  while(1)           //look for args first.........in a loop
   {
    k = get_aux_cmd(xofst, C_ARGS);

    if (!k || !k->size) break;

//if (k->start == 0x28683)
  //  DBGPRT(0,0);



    if (get_cmdopt(OPTSRC))
     {
       if (xofst > ofst)   pchar(LSTFILE,',');
       if (k->size)
        {
         if (k->cptl)
           {
            pp_cmpl(xofst,vconvi(k->start), k->fcom);
            postprt = 1;
           }
         else  pp_argl(xofst, vconvi(k->start), k->fcom, &argno);
        }
     }






    xofst += k->size;                     // look for next args
    size += k->size;
    }

  if (!size)
    {
     // didn't find any ARGS commands - look for subr arguments

     xsub = get_subr(c->opnd[1].addr);

     if (xsub)
      {
       size = xsub->size;
       if (get_cmdopt(OPTSRC) && size)
         {
          if (xsub->cptl)
             {
              pp_cmpl(xofst,vconvi(xsub->start), C_ARGS);
              postprt = 1;
             }
          else pp_argl(xofst, vconvi(xsub->start), C_ARGS, &argno);
         }
      }
    }

  if (get_cmdopt(OPTSRC))
   {
    if (size) pchar(LSTFILE,' '); // add space if args
    xprt(LSTFILE,0,");");       // args close bracket
   }

  if (size)
    {
     pp_comment (ofst,0);                               // do any comment before next args print
 //    if (postprt) pp_hdr (ofst, "#args ", size);        // do args as sep line if compact
     if (postprt) {pp_hdr (ofst, 0, size);
     p_pad(LSTFILE, CMNTPOSN);
     xprt(LSTFILE,0,"#args");    }                         // do args as comment
    }

  return size;
}
// print subroutine params - simpler version
//func, table, struct go here



uint pp_stct (uint ofst, LBK *x)
{
  uint size, end;
  int argno;
  void * fid;

  FDATA *fl;

  fl = get_fldata(LSTFILE);

//  if (x->start == 0x927b4)
//  {
//  DBGPRT(1,0);
//  }


  fid = vconvi(x->start);
  end =  ofst + x->size;
  argno = 1;                               // can't reset argno !!

  if (end == ofst) end++;     // safety

  if (ofst == x->start) maxcolsizes(x);  // do only once.

  size = x->end - x->term + 1;    // where terminator starts

  if (x->term && end > size)
    {      // print terminator
     pp_hdr (ofst, 0, x->term);
     p_pad(LSTFILE,OPNDPOSN);
     xprt(LSTFILE,0,"## terminator");
     ofst += x->term;
     return x->term;             //ofst;
    }

  while (ofst < end)
    {
      if (x->cptl)
        {               //compact
         ofst += pp_cmpl(ofst,fid,x->fcom);
         if (fl->pcol >= CMNTPOSN)  p_pad(LSTFILE,plist[31]);    //before comments.....
        }
      else
        {        //argument
         ofst += pp_argl(ofst,fid,x->fcom,&argno);
         pp_comment (ofst, 1);                  // last item
         xprt(LSTFILE,1,0);
        }
    }

 return x->size;            //ofst;
}

uint pp_text (uint ofst, LBK *x)
{
  int cnt;
  short i, v;
  int xofst;
  CSTR *cmd;

  cmd = get_cstr(CMDSTR,0);
  xofst = x->end;
  cnt = xofst-ofst+1;

  if (cnt > 15) cnt = 15;              // was 16, for copyright notice
  if (cnt < 1)  cnt = 1;

  pp_hdr (ofst, cmd[x->fcom].string, cnt);

  xofst = ofst;

   p_pad(LSTFILE,CMNTPOSN);
  xprt(LSTFILE,0,"\"");
  for (i = cnt; i > 0; i--)
     {
      v = g_byte (xofst++);
      if (v >=0x20 && v <= 0x7f) pchar(LSTFILE,v); else  pchar(LSTFILE,'.');
     }
  pchar(LSTFILE,'\"');

  return cnt;
}


void p_op (COPER *o)
{
 // bare single operand - print type by flag markers

 switch(o->optype)
  {
    case OPIMD:
       xprt(LSTFILE,0,"%x", o->addr & get_sizemask(o->fend));    // immediate (with size)
       break;
    case OPBIT:
       xprt(LSTFILE,0,"B%d,R%x",o->addr,o->reg);                //inline bit for JB,JNB
       break;
    case OPADDR :
       paddr(LSTFILE,o->addr,0);                               // address - jumps
       break;
    case OPOFF :
       xprt(LSTFILE,0,"+%x", o->val & get_sizemask(o->fend));            //addr & get_sizemask(o->fend));     // address, as raw offset (indexed)
       break;
    default:
       xprt(LSTFILE,0,"R%x",nobank(o->reg));                      // register
       if (o->optype == OPAINC) xprt(LSTFILE,0,"++");             // autoincrement
   }

}


void p_bareop (CINST *c, int ix)
{
  // bare op print by instance and index - top level
  // if indexed op. append "+op[0]" with op[1]
  //only called from 1 place ??

  COPER *o;
  PP_OP *p;

  ix &= 0x3;                      // safety check

  o = c->opnd+ix;
  p = csym+ix;

  if (o->optype == OPIND || o->optype == OPINX || o->optype == OPAINC) p->bkt = 1;
  else p->bkt = 0;

  if (p->bkt)  pchar(LSTFILE,'[');
  p_op (o);
  if (o->optype == OPINX) p_op (c->opnd);
  if (p->bkt)  pchar(LSTFILE,']');
  return;
}







void do_opsym(CINST *c, PP_OP *x, uint ix)        //, uint ofst)
{

   COPER *o;
   PP_OP *p;     //, *x;

  if (!c) return;
  if (!x) return;

  ix &= 3;

  o = c->opnd + ix;                            // set operand
  p = x + ix;

  if (!o->fend) return;

    p->noprt = 0;
    p->poptype = o->optype;
    p->opsym = NULL;
    p->sign = bool (o->fend & 0x100);
    p->bkt = 0;

    // indirect ops have brackets

    switch (p->poptype)
     {
       default:
         p->opsym = get_sym(o->reg,0, C_NOBIT|o->fend, c->ofst);      // default to register
       break;

       case OPBIT :            // bit jump - if bitname, drop any register name
         p->opsym = get_sym (o->reg, o->addr, o->addr, c->ofst);
         break;

      case OPIND:
      case OPAINC:
         p->opsym = get_sym(o->reg,0, C_NOBIT|o->fend, c->ofst);
         p->bkt = 1;
         break;

      case OPINX:
         p->opsym = get_sym(o->reg,0, C_NOBIT|o->fend, c->ofst);
         p->bkt = 1;

         if (valid_reg (o->reg) == 2 || o->rbs)
           {       // zero reg or rbs in index
             p->opsym = get_sym(o->addr,0, C_NOBIT|o->fend, c->ofst);
             x->noprt = 1;    // suppress offset print
             if (!p->opsym) p->poptype = OPADDR;    // set to address if no sym
             else p->bkt = 0;
           }
     //     if (p->opsym) p->bkt = 0;         //clear brackets for symname
         break;


     case OPOFF:

     //    p->opsym = get_sym(o->addr,0, C_WHOLE|o->fend, ofst);
     //    break;

     case OPADDR:
         if (!valid_reg (o->addr)) p->opsym = get_sym(o->addr,0, C_NOBIT|o->fend, c->ofst);
         break;

     case OPIMD:
         //  EXCLUSIONS are outside (need INST)
         p->opsym = get_sym(o->addr,0, C_NOBIT|o->fend, c->ofst);
         p->sign = 0;             //no signs with immediate
         break;
     }

  // Note that index and indirect already have the correct calc'ed address, assuming rbase is correct ....

// MUST RECHECK RBASE !!

 if (p->opsym) p->sign = 0;
}




void chk_ad3x(INST *c)
{
  uint addr;
 // 3 operands.  look for rbase or zero in op[1] or op [2] and convert or drop     - NEEDED....

   if (c->sigix == 6 || c->sigix == 7)

    {    // rbase and R0 checks for ad3x and sb3x


     //RBASE may be found AFTER initial scan, so recheck here...........or do an extra pass.....

     //switch if need all 4 opcsubs.

     if (c->opcsub == 1 )
       {      // immediate for rbs index address x = Rbase + off

         if (c->opnd[2].rbs)
           {     // RBASE + offset by addition or subtraction, only if imd and ad3w or sb3w
             OPER *b;
             b = c->opnd+1;      //change address for op[1] (the imediate)

             addr =  b->val + c->opnd[2].val; // addr may be NEGATIVE at this point !!
             if (addr < 0) addr = -addr;              //not convinced this is right way... what about bank ??

             b->addr = addr;         //b->val + c->oper[2].val;              // convert to true address....
             // databank ?
             b->optype = OPADDR;
             csym[1].opsym = get_sym(b->addr,0,C_NOBIT|7,c->ofst);       //indexed rbs
             csym[2].noprt = 1;                                  //and drop op [2] (the rbase)
             csym[1].bkt = 0;
           }
       }

       //if (opcsub == 3)  ??  indexed
           // index R0 already done in do_sym   -  noprt 1, bkt 0

            // need an extra checks for 3 ops with indexed R0 to make name/address reappear, as both [0] and [1] get noprt

//may need an opcsub = 2  ?
         else
          {   // not immediate , check for R0 for psw ...??



            if (valid_reg(c->opnd[2].reg) == 2)   csym[2].noprt = 1;      // x+0, drop R0 (and op)

            if (valid_reg(c->opnd[1].reg) == 2 && c->opnd[1].optype != OPINX)   csym[1].noprt = 1;      // 0+x, drop R0       HERE !!
          }



   } // end ad3x

}








uint pp_sce (INST *c)
{

  /**********************************************
  * generic default pseudo code printer
  * ops still catered for
  *********************************************/

 // int ans;
  cchar  *tx ;
  const OPC* opl;

 // ans = 0;
  opl = get_opc_entry(c->opcix);

  if (!opl->sce) return 0;              // skip if no pseudo source
   p_pad(LSTFILE,PSCEPOSN);

    //    shift_comment(c);               // add comment to shifts with mutiply/divide  - if (curinst.opcix < 10 && ops->imd)

  tx = opl->sce;

//  do_symx_names(c);             // for rbases, 3 ops, names, indexed fixes etc.    done in pp code

       while (*tx)
        {

         switch (*tx)
          {
           case 1:
           case 2:
           case 3:

             p_opsc (c, csym,*tx);                  //c, *tx);
             break;
/*
           acmnt.minst = c;
                acmnt.ctext = nm;
                acmnt.ofst = prtscan.nextaddr;
                sprintf(nm, "## -> Return");

}   */

           case '\n':
             p_indl(LSTFILE);             // print new line with indent (DJNZ, NORM etc)
             break;

             default:
             pchar(LSTFILE,*tx);
          }         // end switch tx

          if (!tx) break;
          tx++;
        }           // end while tx



 return 0;         //ans;
}





uint pp_3op(INST *c)
 {
   const OPC *opl;

   opl = get_opc_entry(c->opcix);

    p_pad(LSTFILE,PSCEPOSN);

   chk_ad3x(c);       // adapt print for rbases and R0

   p_opsc(c, csym,3);
   xprt(LSTFILE,0," = ");
   p_opsc(c, csym,2);
   if (!csym[2].noprt && !csym[1].noprt) xprt(LSTFILE,0," %s ", opl->sce);
   p_opsc(c, csym,1);
   pchar(LSTFILE,';');
   return 0;
 }




uint pp_2op(INST *c)
 {
  const OPC *opl;

   opl = get_opc_entry(c->opcix);

    p_pad(LSTFILE,PSCEPOSN);
         /*
if (opl->nops == 2 && c->opnd[3].fend && valid_reg(c->opnd[3].reg) == 2 && c->opcix > 9)
   {   //   R0 = R0 + something;       NOT IF SHIFT !!
      if (tx) *tx = scespec[2];           //opctbl[LDWIX].sce;
    }

 */

   p_opsc(c, csym,3);
   xprt(LSTFILE,0," %s ", opl->sce);
   p_opsc(c, csym,1);
   pchar(LSTFILE,';');
   return 0;
  }







uint pp_an3x(INST *c)

// purely for an3x, with a = b & c


//!! wrong !!

{

  uint cnt, vmask, i, ix, symmask, ofst;
  int spos;
  SYMLIST *list;
  OPER  *o;
  CHAIN *x;
  SYM *d;
  FDATA *fl;

  fl = get_fldata(LSTFILE);


  if (c->opcsub != 1) return pp_3op(c);       // not immediate


//if (c->ofst == 0x928a7)
//{
// DBGPRT(0,0);
//}


  // if (!o->addr) return;                        // ignore if R0 (why ?)

  ofst = c->ofst;

  vmask = c->opnd[1].addr;                       // value of read/write for bits OR and AND

  o = c->opnd + 2;                              // mask is op[1], but symbol source is op[2]

  list = get_symlist(o->addr, o->fend, ofst);   // size via o->fend (write) ....


  if (!list) return pp_3op(c);
  if (!list->bcnt) return pp_3op(c);     // no part word fields - must have subnames !!

   p_pad(LSTFILE,PSCEPOSN);
  x = get_chain(CHSYM);

 cnt = 0;

 // cut down version of bitwise for an3x
// NOT CORRECT !!!

  i = list->startix;                     //2;

  while (i <= list->endix && vmask)              //i < 65 && vmask)
    {
      d = (SYM*) x->ptrs[i];

      if (d->addr > o->addr)
        {
          ix =  (d->addr - o->addr) * 8;     // bit shift necessary
          symmask = d->fmask << ix;          // to get to correct sym mask for pmask , vmask
         }
      else
        {
          ix = 0;
          symmask = d->fmask;
        }

//but not if not bit flags specified in op[3]
      if (symmask & vmask)
        {

          if (cnt == 0)
           {
             p_opsc(c, csym,3);
             xprt(LSTFILE,0," = (");
             spos = fl->pcol;            // keep this position
           }

          xprt(LSTFILE,0,"%s ", d->name);  // shift val down to correct bit.

          cnt++;
          vmask ^= symmask;                                    // remove bit(s) after printing

          if (vmask)
           {
             pchar(LSTFILE,'|');
             pp_comment(ofst,2);
             xprt(LSTFILE,1,0);
              p_pad(LSTFILE,spos);         //pad to saved column
           }
          else  xprt(LSTFILE,0,");");

      //    if (vmask)   pp_comment(c->ofst+1,2);               // check for comment THIS BUGGERS UP the list !!!
        }
       i++;
    }










  //end of list, but pmask may still have bits set, so do defaults


 // if (pmask)      pmask = do_bitdeflt(c, o->addr, 0, pmask, "|", list);




   mfree(list,sizeof(SYMLIST));

   if (!cnt)  return pp_3op(c);

   return 0;

}












uint do_bitnames(CINST *c, uint addr, uint vmask, uint *pmask, SYMLIST *list)
 {
    //do named fields

  uint i, ix, cnt, symmask, ofst;
  CHAIN *x;
  SYM *d;
  cchar *op;

  if (!(*pmask)) return 0;      // nothing to print
  if (!list)  return 0;       //safety

  if (c->opcix > 31 && c->opcix < 34)  op = "^="; else op = "=";    // xor

  x = get_chain(CHSYM);

  ofst = c->ofst;

  i = list->startix;
  cnt = 0;
  while (i <= list->endix && *pmask)
    {
      d = (SYM*) x->ptrs[i];

      if (!(d->fend & C_NOBIT))
        {
          //whole syms

         if (d->rstart <= ofst && d->rend >= ofst )
           {
            if (d->addr > addr)
              {
               ix =  (d->addr - addr) * 8;        // bit shift necessary
               symmask = d->fmask << ix;          // to get to correct sym mask for pmask , vmask
              }
            else
             {
              ix = 0;
              symmask = d->fmask;
             }

            if (symmask & *pmask)    //names can have multiple bit fields
             {
              p_indl(LSTFILE);
              xprt(LSTFILE,0,"%s %s %x;", d->name, op, (vmask & symmask) >> (d->fstart+ix));  // shift val down to correct bit.
              cnt++;
              *pmask ^= symmask;                                    // remove bit(s) after printing
              if (*pmask)   pp_comment(ofst,2);                    // check for comment
             }
           }
        }
       i++;
    }
  return cnt;
}


uint do_bitdeflt(CINST *c, uint addr, uint vmask, uint *pmask, SYMLIST *list)
   {
      //  Bx_Rn  printouts. Single bits
     cchar *op;
     uint i, cnt, symmask, ofst;
     CHAIN *x;
     SYM *d;

     if (!(*pmask)) return 0;      // nothing to print

     x = get_chain(CHSYM);

     if (c->opcix > 31 && c->opcix < 34)  op = "^="; else op = "=";    // xor
     ofst = c->ofst;

      // sort out symname first - always write by preference

     d = 0;
     if (list)
       {
         if (list->fend & 40)
           {       //write
            if (list->defwr < x->num) d = (SYM*) x->ptrs[list->defwr];
           }

        if (!d && list->defrd < x->num) d = (SYM*) x->ptrs[list->defrd];      //read is default
       }

    cnt = 0;
     i = 0;
      while (i < 32)
        {
          symmask = (1 << i);

          if (symmask & *pmask)
            {
              if (!(*pmask)) break;

             p_indl(LSTFILE);

             if (d)
             xprt(LSTFILE,0,"B%d_%s %s %x;", i, d->name, op, (vmask & symmask) >> i);              // shift val down to correct bit.
             else
             xprt(LSTFILE,0,"B%d_R%x %s %x;", i, addr, op, (vmask & symmask) >> i);              // shift val down to correct bit.
             cnt++;
             *pmask ^= symmask;                                    // remove bit(s) after printing

             if (*pmask)   pp_comment(ofst,2);               // check for comment
            }
           i++;
        }
   return cnt;
  }







void do_sym_names(INST *c, PP_OP *x)    //, uint ix)                 //INST *c)
{
  // adjust operands for correct SOURCE code printout, rbases, indexes etc.
  // This CHANGES addresses for indexed and RBS for printout purposes

  OPER *b, *o;
  PP_OP *p;
  int i;
  const OPC* opl;

  opl = get_opc_entry(c->opcix);

  // get all sym names FIRST, then drop them later as reqd
  // get by optype

  //if (c == &cinst)  p = psym else....


  for (i = 0; i < 4; i++)     // always check ops 1,2,3
   {
    o = c->opnd + i;
    p = x + i;

    do_opsym(c,x, i);         //, c->ofst);       //so can do individual ops (div) outside

    if (o->optype == OPIMD)
        {
         //  EXCLUSIONS for immediates, no sym if ...

      //   can't trust rbs

         if (valid_reg (o->addr)) p->opsym = NULL;                  // valid reg (RAM and ROM OK)
         if (c->opcix < 10)   p->opsym = NULL;                      // all shifts
         if (opl->sigix == 5) p->opsym = NULL;                      // AND
         if (opl->sigix > 7 && opl->sigix < 12)  p->opsym = NULL;   // MULT, XOR, OR, CMP, DIV
         if (opl->sigix == 12 && c->opnd[2].rbs) p->opsym = NULL;   // LDX with rbase
        }
   }

// RECHECK RBASE  ?? but can't go in main chain....unless separate pass....



/*
if (opl->nops == 2 && c->opnd[3].fend && valid_reg(c->opnd[3].reg) == 2 && c->opcix > 9)
   {   //   R0 = R0 + something;       NOT IF SHIFT !!
      if (tx) *tx = scespec[2];           //opctbl[LDWIX].sce;
    }

 */

 //     R3 = R2 + R1 style.  is anything rbase ?? replace with address as an imd, but NOT if it's traget

// can't trust rbs ?



  if (!c->opcsub && (c->opnd[1].rbs || c->opnd[2].rbs))    //   register only
      {
        for (i = 1; i < c->numops; i++)
          {  //  op 1 for 2 ops, 1 & 2 for 3 ops
            b = c->opnd + i;
            if (c->opnd[2].rbs && i != 3)        //not target
              {
               b->optype = OPIMD;
         //      b->rgf = 0;
              //  psym[i].opsym = NULL;
                  x[i].opsym = NULL;
              }
          }
       }

    /* CARRY opcodes - can get 0+x in 2 op adds as well for carry
    // "\x2 += \x1 + CY;"   nobank is for immediates.../

  if (valid_reg(c->opnd[1].addr) == 2 && c->opcix > 41 && c->opcix < 46)
    {
      psym[1].noprt = 1;
     //if (tx) *tx = scespec[opl->sigix-6];     // drop op [1] via special array
    }
*/
}





//push->pop as ldx....
//push 14, pop 15

uint pp_pop(INST *c)
{
      CINST *x;

    x = find_opcode(c->ofst,0);            // previous opcode in loop



   if (x && x->sigix == 14)
     {   //for push->pop combination, to show x[3] = c[1] ;

       memcpy(&linst, x, sizeof(INST) );       //&linst....
       do_sym_names(&linst, ssym);      //0x10);   //  (or dest)
       p_pad(LSTFILE,PSCEPOSN);
       p_opsc(c, csym,1);
       xprt(LSTFILE,0," = ");
       p_opsc(&linst, ssym,1);
       pchar(LSTFILE,';');
       return 0;
  }

//not match for push before pop

      return pp_sce(c);                //temp
}



uint pp_psh(INST *c)
 {
//  const OPC *opl;
  CINST *x;


 //  opl = get_opc_entry(c->opcix);

   x = find_opcode(c->ofst,1);            // forwards to next opcode in loop

   if (x->sigix == 15)
   {
  return  0;            // print nothing, in next pop

   }

  return pp_sce(c);


    p_pad(LSTFILE,PSCEPOSN);

 //for push->pop combination, to show a = b;

// x[3] = c[1] ....but must suppress the pop....


         /*
if (opl->nops == 2 && c->opnd[3].fend && valid_reg(c->opnd[3].reg) == 2 && c->opcix > 9)
   {   //   R0 = R0 + something;       NOT IF SHIFT !!
      if (tx) *tx = scespec[2];           //opctbl[LDWIX].sce;
    }

 */

   p_opsc(c, csym,3);
 //  xprt(LSTFILE,0," %s ", opl->sce);
   p_opsc(c, csym,1);
   pchar(LSTFILE,';');
   return 0;
  }





uint pp_btws (INST *c)

// Now works for fields wider than one bit, adjusting val as well as print mask
//how to switch on global settings.... ???


// prob need subr for each opcode type ??? opcode and sigs...
// AND (5), OR (9), LDX (12), CLR (1)............STX ?




{

  uint cnt, sig, pmask, vmask;
  SYMLIST *list;
  COPER  *o;
  uint (*p_op) (INST *);            // for 2 or 1 op

  if (c->numops == 2) p_op = pp_2op; else p_op = pp_sce;   // 2 or 1 op

  // must be immediate op, or clr
  if (c->opcsub != 1 && c->sigix != 1) return p_op(c);      //pp_sce(c);

  sig = c->sigix;                 // opcode sigix

  if (c->opnd[1].rbs) return p_op(c);                          // NOT if an rbs operand (...why?)

  if (sig == 9 && !c->opnd[1].addr)
    {        // OR, XOR with zero mask - a timewaster ?
       p_pad(LSTFILE,PSCEPOSN);
      p_opsc(c, csym,3);
      xprt(LSTFILE,0," = ");
      p_opsc(c, csym,3);
      pchar(LSTFILE,';');
      return 0;
   }

  vmask = c->opnd[1].val;                       // value of read/write mask (OR and AND)

  o = c->opnd + 3;                              // o is write op, [1] is read op

  if (sig == 12 || sig == 1)
    {                                             // ldb, ldw, clr. etc
      pmask = fmask(0,c->opnd[3].fend);           // set all bits in WRITE operand
    }
  else  pmask = vmask;                           // only for bits in value given

  if (sig == 5)
    {                                            // AND. Reverse the print mask.
       pmask = ~pmask;
       pmask &= fmask(0,c->opnd[3].fend);        // cut back down to right size
    }

 //but may need an 'all bits/flags' option, so would be same as ldb and clr above....

  list = get_symlist(o->addr, o->fend, c->ofst);

  if (!list && __builtin_popcount(pmask) != 1)  return p_op(c); // allow single flag

  //list->bcnt;   list->wcnt;
 //  list->bcnt = 0;                // count of bitfield syms found (not pmask'd)
 // list->wcnt = 0;                // count of 'whole' syms found

  // either names or a single bit field to get here

  cnt = 0;

  cnt = do_bitnames(c, o->addr, vmask, &pmask, list);

  //end of list, but pmask may still have bits set, so do defaults
// ** may need more options here !!

  if (pmask && !cnt && (sig == 5 || sig == 9))      // AND, OR only if no names ?
    {
      cnt += do_bitdeflt(c, o->addr, vmask, &pmask, list);
    }


   //   if (pmask)   xprt(LSTFILE,0, " **P %x **", pmask);  /// DEBUG

   mfree(list,sizeof(SYMLIST));

   if (!cnt) return p_op(c);

return 0;
 }




uint get_swop_inx(CINST *c, int *j)
 {
    if (get_cmdopt(OPTBKT))
      {
     find_fjump (c->ofst, j);               // used for static jumps too.
     if (c->opcix > 53 && c->opcix < 72)   // 54 - 71 all conditional jumps (except djnz)
       {
         if ((*j) & 1) return c->opcix ^ 1;
       }
      }
     return c->opcix;
 }









uint pp_cy(INST *c)
{
    // for carry opcodes adcb, adcw, sbbb, sbbw
    // do as 2 op
    // \3 <op>= \1 <op> CY
    // \3 <op>= CY

  const OPC *opl;

   opl = get_opc_entry(c->opcix);

     p_pad(LSTFILE,PSCEPOSN);

  // reduce op if op[1] is R0  ( R += 0 + CY)

  if (valid_reg(c->opnd[1].addr) == 2) csym[1].noprt = 1;

     p_opsc(c, csym,3);

     if (csym[1].noprt)
      {          // r1 is zero
        xprt(LSTFILE,0," %s= ", opl->sce);
      }
     else
      {
   //     xprt(LSTFILE,0, " = ");
  //      p_opsc(3);
        xprt(LSTFILE,0, " %s= ", opl->sce);
        p_opsc(c, csym,1);
        xprt(LSTFILE,0, " %s ", opl->sce);
      }


     xprt(LSTFILE,0, "CY");
     pchar(LSTFILE,';');

 return 0;
}




uint pp_goto(INST *c)
 {
   int jf;
   PP_OP *p;

   jf = 0;

   if (get_cmdopt(OPTBKT))   find_fjump (c->ofst, &jf);       // as used here.

   switch (jf)    //jump handling
             {
              case 1 :
                xprt(LSTFILE,0,"{");   // [if] case
                break;
              case 2 :
                xprt(LSTFILE,0,"return;");  // static jump to a return
                break;
              case 3:
                pchar(LSTFILE,';');
                // fall through
              case 4:
                xprt(LSTFILE,0,"} else {");
                break;
              default:
                xprt(LSTFILE,0,"goto ");
                p = csym + 1;
                p-> bkt = 0;      // overlap with lookback indexed
                p->poptype = OPADDR;

                p_opsc (c, csym,1);
                pchar(LSTFILE,';');

                if (c->opcix == 76)  xprt(LSTFILE,1,0);        //skip fakes a newline
                break;
             }

   return 0;
 }

uint pp_pswf(INST *c)
{
   // default "if <flagname> goto  style
   int inx, jf;
   const OPC *opl;

   inx = get_swop_inx(c, &jf);      // both cond and static jumps,  not djnz
   opl = get_opc_entry(inx);              // c->opcix;

    p_pad(LSTFILE,PSCEPOSN);
   xprt(LSTFILE,0,"if (%s = %d) ", opl->sce, (inx & 1));

   pp_goto(c);
   return 0;
}




INST* find_last_psw(CINST *c, int cnt)
 {
   // finds last PSW changing opcode for printout, always print phase
   //goes backwrds by cnt

    JMP *j;
    PSW *p;
    CINST *x;
  //  uint ans;
    uint ofst, ix;
    const OPC *opl;

  //  ans = 0;

    ofst = c->ofst;   //from current instance

 //   if (ofst == 0x92b1e)
 //   {
  //      DBGPRT(0,0);

  //  }

    // is a psw setter defined by user cmd ??
    p = get_psw(c->ofst);
    if (p)
       {   // found user coommand
           linst.ofst = p->pswop;
           if (get_mopcode(&linst)) return &linst;
   //        return ans;
       }

    /// ignore clc,stc as psw setters unless JLEU, JGTU, JC, JNC
  //  chkclc = 0;
  //  if (c->opcix > 61 && c->opcix < 64) chkclc = 1;  // JLEU/JGTU
  //  if (c->opcix > 55 && c->opcix < 58) chkclc = 1;   // JC/JNC

    // This is used by carry and ovf as well

    // what about a popp ??

x = c;  //just make non zero
    while (x)
     {
      x = find_opcode(ofst,0);            // go backwards

      if (cnt <= 0) break;
      if (!x) break;
      cnt--;

      ofst = x->ofst;

      if (x->sigix == 17)
       {              // hit a CALL - find RET jump after this CALL
                      // and go back from there

         j = get_tjump(ofst+x->opsize, &ix);    // next opcode

         if (j && j->jtype == J_RET) ofst = j->fromaddr;
         else break;                             //stop if not found
       }

      if (x->sigix == 20)
       {                        //RET, this seems to work for aa, BUT NOT FOR TWO CALLS !!
          x = find_opcode(ofst,1);            // forwards to next opcode in loop
          j = get_tjump(x->ofst, &ix);        // get where CALL is for this opcode

          //but this might have multiple callers !!

          if (j && j->jtype == J_SUB) ofst = j->fromaddr;
          else break;                             //stop if not found
       }

      opl = get_opc_entry(x->opcix);
      if (opl->pswc)
      //if (opctbl[x->opcix].pswc)
        {       // FOUND a PSW setter,
          memcpy(&linst, x, sizeof(INST) );  //copy to dest
    //      ans = 1;
          break;
        }

      //  {    // FOUND PSW setter, if set/clear carry, check flag.
      //     if (x->sigix == 21)
      //      {
      //       if (chkclc) ans = 2;
      //      }
      //   else
      //      {
      //       ans = 1;
      //      }
      //  }



     }         //end while !ans

/*if get_scan(c->ofst)
    {         //search begins at block start , find jump ?

     ix = get_tjmp_ix(ofst);   // find first 'to' jump
get_chain
if valid, search back from here instead.


}
*/



    if (x)  {do_sym_names(&linst, ssym);      //0x10);   //  (or dest)       // 0x10 = use sinst
    return &linst; }

    return 0;
 }








uint pp_pswc(INST *c)
{

  // Carry - jnc,jc opcodes (56,57)

  // Replace "if (CY)" with various source opcodes
  // may be bad idea....similar rules with write and non_write.

// possible opcodes add,shift R, shift L, cmp, inc, dec, neg, sub

   int ans, inx, jf;
   const OPC *opl;
   INST *x;

//if (c->ofst == 0x92b1e)
//{
// DBGPRT(0,0);
//}




   inx = get_swop_inx(c, &jf);      // both cond and static jumps,  not djnz

   // get last psw changer

   x = find_last_psw(c,32);        // returns inst if found (in linst)

   if (!x) return pp_pswf(c);          // not found, revert to "if (CY"   style

   opl = get_opc_entry(x->opcix);              // map to found inst



   switch (x->sigix)
     {
       case 7:

          // subtract. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JGE (66) JC -> JLT (67)

          //sb3b not right? can use op3 ?

      //    if (inx & 1)   tx = opctbl[66].sce;      // JNC (56) -> JGE
       //   else  tx = opctbl[67].sce;                 // JC (57) -> JLT
          p_pad(LSTFILE,PSCEPOSN);
         xprt(LSTFILE,0,"if (");

         if (x->opnd[3].addr)  //not register in cvase indexed
          {
            p_opsc(x,ssym,3);
            xprt(LSTFILE,0," %s 0) ",(inx & 1) ? ">=" : "<");
            break;
          }

         p_opsc(x,ssym,1);
         xprt(LSTFILE,0," %s ",(inx & 1) ? ">=" : "<");
         p_opsc(x,ssym,2);
         xprt(LSTFILE,0,") ");

         break;

      case 10:

          // compare. Same as subtract, but no wop

          // JNC (56) -> JLE (65) JC -> JGT (660

           p_pad(LSTFILE,PSCEPOSN);
          xprt(LSTFILE,0,"if (");

          if (valid_reg(x->opnd[2].addr) == 2)     // zero register
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(x,ssym,1);
              xprt(LSTFILE,0," %s ",(inx & 1) ? ">=" : "<");
              p_opsc(x,ssym,2);
            }
          else
            {
              p_opsc(x,ssym,2);
              xprt(LSTFILE,0," %s ",(inx & 1) ? ">=" : "<");
              p_opsc(x,ssym,1);
            }
          xprt(LSTFILE,0,") ");
          break;


      case 6:
      case 25:
          //  add or inc   STANDARD CARRY
            p_pad(LSTFILE,PSCEPOSN);
          xprt(LSTFILE,0,"if (");
          p_opsc(x,ssym,3);
          xprt(LSTFILE,0," %s", (inx & 1) ? ">" : "<=");
          xprt(LSTFILE,0," %x) ", get_sizemask(x->opnd[3].fend));
          break;


      case 3:
      case 4:

   //if (opl->sigix == 3 || opl->sigix == 4)
     //   {   //  shl (3)   or shr (4)

          // check no of shifts is imd, otherwise return 0=0 style.
           if (x->opnd[1].optype != OPIMD)
            {
              return pp_pswf(c);        //memset(&sinst,0,sizeof(INST));        // set all zero      clr_inst(&sinst);       // can't do variables
            // return 0;
            }
   p_pad(LSTFILE,PSCEPOSN);
           if (opl->sigix == 3)
            {   //shl
             ans =  (x->opnd[3].fend & 0x1f) + 1;           // total number of bits
             ans -= x->opnd[1].val;                         // shifts from opcode

             if (ans > 16)
               { // dl shifts ??
                 ans -= 16;
                 x->opnd[3].addr+=2;
               }
            }
          else
            {  //shl
              ans = x->opnd[1].val-1;
            }

          xprt(LSTFILE,0,"if (");
          xprt(LSTFILE,0,"B%d_", ans);

          p_opsc(x,ssym,3);
          xprt(LSTFILE,0," = %d) ", inx & 1);
          break;

     case 11:
     case 8:
     default:   //  mult,div. use OVF

          return pp_pswf(c);
break;
 /*  if (opl->sigix == 11 || opl->sigix == 8)
        {
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," %s", (ainx & 1) ? ">" : "<=" );
          xprt(LSTFILE," %x) ", camk[sinst.oper[opl->wop].wsize]);
         }
*/

    case 24:       // if (opl->sigix == 24)
             // dec
                p_pad(LSTFILE,PSCEPOSN);
          p_opsc(x,ssym,3);           //&sinst,3);
          xprt(LSTFILE,0," %s 0) ", (inx & 1) ?  ">=" : "<" );

      break;

     }  //end switch
pp_goto(c);
return 0;
}

/*
uint pp_pswv(INST *c)
{

  // Overflow - jnv,jv opcodes

  // Replace "if (OVF) with various source opcodes
// possible opcodes add,shift R, shift L, cmp, inc, dec, neg, sub

   int ans, inx, jf;
   const OPC *opl;
   cchar *tx;

if (c->ofst == 0x92b1e)
{
 DBGPRT(0,0);
}




   inx = get_swop_inx(c, &jf);      // both cond and static jumps,  not djnz
   tx = opctbl[inx].sce;            // output string, c->opcix mostly.

    p_pad(LSTFILE,PSCEPOSN);

   // get last psw changer

   ans = find_last_psw(c,&sinst,32);        // returns 1 if found...op in linst

   if (!ans) return pp_pswf(c);          // not found, revert to "if (CY"   style

   opl = opctbl + sinst.opcix;              // remap to found inst



   switch (sinst.sigix)
     {
       case 7:

          // subtract.
          //overflow set if a-B is greater than range or sign changes
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE
    //      xprt(LSTFILE,0,"if (");
          if (inx == 56)   tx = opctbl[67].sce;      // JNC -> JLT     swopcmpop[ainx&1];
          else  tx = opctbl[66].sce;                 // JC -> JGE

         xprt(LSTFILE,0,"if (");
         p_opsc(0x13);                  //WRONG !!
                  xprt(LSTFILE,0," ");
         p_opsc(0x12);
         xprt(LSTFILE,0," %s ",tx);
        //         xprt(LSTFILE,0," %s 0) ",tx);
         p_opsc(0x11);
         xprt(LSTFILE,0,") ");

         break;

      case 10:

          // compare. Same as subtract, but no wop

          // JNV (58) -> JLT  JC -> JGE

          if (inx == 58)   tx = opctbl[67].sce;      // JNC/JNV -> JLT
          else  tx = opctbl[66].sce;                 // JC/ -> JGE



          xprt(LSTFILE,0,"if (");
          if (valid_reg(sinst.opnd[2].addr) == 2)     // zero register
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(0x11);            //&sinst,1);                   // operand 1 of compare
              xprt(LSTFILE,0," %s ",    opctbl[inx^1].sce);       //      swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(0x12);           //&sinst,2);                   // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(0x12);         //&sinst,2);
              xprt(LSTFILE,0," %s ",tx);
              p_opsc(0x11);             //&sinst, 1);
            }
          xprt(LSTFILE,0,") ");
          break;


      case 6:
      case 25:
          //  add or inc   STANDARD CARRY
          xprt(LSTFILE,0,"if (");
          p_opsc(0x13);            //&sinst,3);
          xprt(LSTFILE,0," %s", (inx & 1) ? ">" : "<=");
          xprt(LSTFILE,0," %x) ", get_sizemask(sinst.opnd[3].fend));
          break;


      case 3:
      case 4:

   //if (opl->sigix == 3 || opl->sigix == 4)
     //   {   //  shl (3)   or shr (4)

          // check no of shifts is imd, otherwise return 0=0 style.
           if (sinst.opnd[1].optype != OPIMD)
            {
              return pp_pswf(c);        //memset(&sinst,0,sizeof(INST));        // set all zero      clr_inst(&sinst);       // can't do variables
            // return 0;
            }

           if (opl->sigix == 3)
            {   //shl
             ans =  (sinst.opnd[3].fend & 0x1f) + 1;       //  (sinst.oper[opl->wop].wsize & 7) * 8;        //   casz[sinst.oper[opl->wop].wsize] * 8;          // number of bits
             ans -= sinst.opnd[1].val;                            //shift from MSig

             if (ans > 16)
               { // dl shifts ??
                 ans -= 16;
                 sinst.opnd[3].addr+=2;        //opl->wop
               }
            }
          else
            {  //shl
              ans = sinst.opnd[1].val-1;
            }
          xprt(LSTFILE,0,"if (");
          xprt(LSTFILE,0,"B%d_", ans);

          p_opsc(0x13);              //&sinst,3);               //opl->wop
          xprt(LSTFILE,0," = %d) ", inx & 1);
          break;

     case 11:
     case 8:
     default:

          return pp_pswf(c);
break;
 /-  if (opl->sigix == 11 || opl->sigix == 8)
        {       //  mult,div STANDARD OVERFLOW all have wop ?
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," %s", (ainx & 1) ? ">" : "<=" );
          xprt(LSTFILE," %x) ", camk[sinst.oper[opl->wop].wsize]);
         }
-/

    case 24:       // if (opl->sigix == 24)
             // dec
          p_opsc(0x13);           //&sinst,3);
          xprt(LSTFILE,0," %s 0) ", (inx & 1) ?  ">=" : "<" );

      break;

     }  //end switch
pp_goto(c);
return 0;
}
*/
/* Not used ??
void do_ovf(CINST *c, const char **tx, int ainx)
{

// Replace if (OVF) with various sources
// possible opcodes add, shift L, cmp, inc, dec, neg, sub, div.

 //  int ans;
 //  const OPC *opl;

   xprt(LSTFILE,0,"if (");
   // get last psw changer

return;            //temp ignore extras

//  ainx = get_swop_inx(c, &jf);          // both cond and static jumps,  not djnz
//  *tx = opctbl[inx].sce;
*
   ans = find_zlast_psw(c,1);  // returns 1 if found...op in sinst
   if (!ans) return;          // not found

   opl = opctbl + sinst.opcix;              // mapped to found inst


   if (opl->sigix == 7)
        {
          // subtract. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," %s ",*tx);
          p_opsc(0,1,0);
          xprt(LSTFILE,") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


  if (opl->sigix == 10)
        {
          // compare. Borrow (Carry) cleared if a>=b
          // use swopcmpop to remap aix to get correct arith operator
          // JNC -> JLT  JC -> JGE

          *tx = swopcmpop[ainx&1];

          if (!nobank(sinst.oper[2].addr))
            {  // jc/jnc after zero first compare, swop order via swopcmpop 8 and 9
              p_opsc(&sinst,1,0);                        // operand 1 of compare
              xprt(LSTFILE," %s ", swopcmpop[ainx - 48]);   // 56 maps to 8
              p_opsc(&sinst,2,0);                        // operand 2 of compare (zero)
            }
          else
            {
              p_opsc(&sinst,2,0);
              xprt(LSTFILE," %s ",*tx);
              p_opsc(&sinst, 1,0);
            }
          xprt(LSTFILE,") ");
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 6  || opl->sigix == 25)
        {       //  add or inc STANDARD CARRY -  have wop ?
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," %s", (ainx & 1) ? ">" : "<=");
          xprt(LSTFILE," %x) ", camk[sinst.oper[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


   if (opl->sigix == 3)
        {       //  shl STANDARD CARRY all have wop ?
          // need to find out how many shifts
          // sinst.oper[1].val is shift amount.  wop is shifted value cant do it for other than imm (oper[1].imd)

          if (!sinst.oper[1].imd)
            {
             clr_inst(&sinst);       // can't do variables
             return;
            }

          //shl

          ans = casz[sinst.oper[opl->wop].wsize] * 8;          // number of bits
          ans -= sinst.oper[1].val;                            //shift from MSig

         // dl shifts ??

         if (ans > 16)
          {
            ans -= 16;
            sinst.oper[opl->wop].addr+=2;
          }

          //fiddle as a bit
          xprt(LSTFILE,"B%d_", ans);
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," = %d) ", ainx & 1);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }

   if (opl->sigix == 11 || opl->sigix == 8)
        {       //  mult,div STANDARD OVERFLOW all have wop ?
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," %s", (ainx & 1) ? ">" : "<=" );
          xprt(LSTFILE," %x) ", camk[sinst.oper[opl->wop].wsize]);
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
        }


     if (opl->sigix == 24)
      {       // dec
          p_opsc(&sinst,opl->wop,0);
          xprt(LSTFILE," %s 0) ", (ainx & 1) ?  ">=" : "<" );
          *tx = opctbl[JMPIX].sce - 1;   //jump sce code to follow this - allow for increment
      }
}
*/



 // "\3--;\nif (\3 != 0) \x9" },
uint pp_djnz(INST *c)
{
// do a two line print for divides
// manual says jumps if non zero, else next opcode
   int j;

    p_pad(LSTFILE,PSCEPOSN);

   p_opsc(c,csym,3);
   xprt(LSTFILE,0,"--;");

   p_indl(LSTFILE);                 //next line
   xprt(LSTFILE,0,"if (");
   p_opsc(c,csym,3);

   // need to flip this as with other jumps

   find_fjump (c->ofst, &j);

   if (j & 1)  xprt(LSTFILE,0," = 0) ");
   else        xprt(LSTFILE,0," != 0) ");
   pp_goto(c);


   return 0;
}

uint pp_nrm(INST *c)
{
// "\1 = normalize(\2);\n\3 <<= \1;" ,       original

// "Ra = 0;  while (B31_Rb = 0 and Ra < 32)   { Rb << 1;  Ra++;}  "

///drop curly brackets if no brackets ?? different layout ?

   p_pad(LSTFILE,PSCEPOSN);


   p_opsc(c,csym,1);
   xprt(LSTFILE,0," = 0;");

   p_indl(LSTFILE);                 //next line

   //pstr(0,"while (B31_");
    xprt(LSTFILE,0,"while ");
   //drop o-fend & 100 to ditch sign
   p_opsc(c,csym,3);
   xprt(LSTFILE,0," >= 0 and ");
   p_opsc(c,csym,1);
   xprt(LSTFILE,0," < 32");

   p_indl(LSTFILE);                 //next line

   xprt(LSTFILE,0,"{ ");
   p_opsc(c,csym,3);
   xprt(LSTFILE,0," << 1; ");
   p_opsc(c,csym,1);
   xprt(LSTFILE,0,"++;}");            //Rb << 1;  Ra++;}

   // need to flip this as with other jumps

 //  find_fjump (c->ofst, &j);

 //  if (j & 1)  xprt(LSTFILE,0," = 0) ");
//   else        xprt(LSTFILE,0," != 0) ");
 //  pp_goto(c);



return 0;

}





uint pp_divprt(INST *c)
{
// do a two line print for divides

   OPER *o;
   OPER x;
   PP_OP p;
   p_pad(LSTFILE,PSCEPOSN);

   p_opsc(c,csym,3);                //c,3);
   xprt(LSTFILE,0," = ");
   p_opsc(c,csym,2);                  //c,2);
   xprt(LSTFILE,0," / ");
   p_opsc(c,csym,1);           // c,1);
   pchar(LSTFILE,';');
   p_indl(LSTFILE);                 //next line

   o = c->opnd + 3;

   memcpy(&x,o,sizeof(OPER));         //copy oper and print to local
   memcpy(&p,csym+3, sizeof(PP_OP));

   // change target
   x.reg++;
   if ((o->fend & 0x1f) == 0xf) x.reg ++;   // long div


   p_sign(&x,&p);             // print local items
   p_sc (&x,&p);

   xprt(LSTFILE,0," = ");
   p_opsc(c, csym,2);                //c,2);
   xprt(LSTFILE,0,"%s", " % ");
   p_opsc(c, csym,1);                 //c,1);
   pchar(LSTFILE,';');
   return 0;
}



uint pp_jmp(INST *c)
{
   p_pad(LSTFILE,PSCEPOSN);
return pp_goto(c);
}

uint pp_cond(INST *c)
{
   /* conditonal jumps (goto)

   jgtu (62), jleu (63),   jgt (64), jle (65)
   jge  (66), jlt  (67),   je  (68), jne (69)

based on previous opcodes rules are
if (3) != R0    <Local op> 0  i.e. if a write op
else if (2 <lop> 1)  if no write op and 2 op
if 1 op (30 always set ??

*/

   int inx, jf, wrt;
   cchar *tx;
   INST *x;
   const OPC* opl;

   inx = get_swop_inx(c, &jf);      // both cond and static jumps,  not djnz

  opl = get_opc_entry(inx);
  tx = opl->sce;                 // 'psuedo source code' output string, c->opcix mostly.

   x = find_last_psw(c,32);         // returns inst if found...op in linst

    p_pad(LSTFILE,PSCEPOSN);
   if (!x) return pp_pswf(c);


   xprt(LSTFILE,0,"if (");


//if pp_sce, need to search for op as below, otherwise  p-whatever with bkts....
//SYMBOLS !!

 //  opf = opctbl + x->opcix;           // mapped to found instance

   wrt = x->opnd[3].fend & 0x40;         // is this a write op ?

   if (wrt)
    {            // real write op (not compare)
      if (x->opnd[3].addr)         // don't use reg  - could be indexed op
        {                           // style [3] op 0
         p_opsc(x,ssym,3);
         xprt(LSTFILE,0," %s 0",tx);
        }
      else
       {
         // R0 = <op>;  style, print  "if (( 2 op 1) = 0)"
         // but can be 1,2, or 3 ops....

         if (x->opnd[2].reg)
           {    // print "(2 <op> 1) = 0" 2op, or zero with 3 op


              opl = get_opc_entry(x->opcix);
              xprt(LSTFILE,0,"(");
              p_opsc(x,ssym,2);
              xprt(LSTFILE,0," %s ",opl->sce);   //           while (*x > 0x1f) ppchar(LSTFILE,*x++);         // operator (space & up)
              p_opsc(x,ssym,1);
              xprt(LSTFILE,0,")");
              xprt(LSTFILE,0," %s 0",tx);
          }
         else

         xprt(LSTFILE,0," %s 0",tx);

        }

     }

// where are the symbol finds ??

 if (!wrt)
     {      // non write - a compare style,  R2 = R1
      if (!x->opnd[2].reg)
       {
         if (!x->opnd[1].addr)    //not reg as may be indexed
          {
            xprt(LSTFILE,0," %s ", tx);    //  ?? true ?
          }
         else
          {
           // cmp has zero first, so swop operands over
            //tx cannmot be right if swopped over....
           // "1 = 0"
           p_opsc(x,ssym,1);
           opl = get_opc_entry(inx ^ 1);
           xprt(LSTFILE,0," %s ", opl->sce);
           p_opsc(x,ssym,2);        //zero
          }
       }
     else
       {  // "2 <op> 1"
         p_opsc(x,ssym,2);
         xprt(LSTFILE,0," %s ", tx );
         p_opsc(x,ssym,1);
       }
     }

    xprt(LSTFILE,0,") ");
    return pp_goto(c);


}

void pp_answer(INST *c)
{
  // print answer, if there is one
  uint addr;
  SUB *sub;
  SPF *a;
  void *fid;

  OPER x;
  PP_OP p;

  addr = c->opnd[1].addr;
  sub = get_subr(addr);

  if (!sub) return;
 // a = get_spf(s,0);


 fid = vconvi(sub->start);

  while ((a = get_spf(fid)))          //if (!a) return;
  {
    //if (a->spf > 1) break;           //no answer found

    if (a->spf == 1)
    {

  // found an answer. set up a phantom oper[3] as write op... (rgf set)
  // CHNAGE TO local op ! like pp_div

  //   o = c->opnd+3;
  //   p = psym + 3;
     p.opsym = 0;

     x.addr =  a->addrreg;      //pars[0].reg;               //addrreg;

      if (!valid_reg(x.addr))
       {
     //    o->rgf = 0;
         x.optype = OPADDR;
         p.bkt = 1;            // address destination
       }

     p.opsym = get_sym(x.addr,0,7,c->ofst);        //pp_answer
     if (p.opsym) { p.bkt = 0;}   //             o->sym = 1; o->bkt = 0;}

     p_sc(&x,&p);               //p_opsc(c,3);                  //p_sc ??
     xprt(LSTFILE,0," = ");
    }
    fid = a;
  }
}







uint pp_call(INST *c)
{
   // subr call, may have answer and args attached.
  uint ans;
  ans = 0;
   p_pad(LSTFILE,PSCEPOSN);

  pp_answer(c);
  p_opsc (c, csym,1);                 //c, 1);
  ans += pp_subargs(c);
  return ans;
}


uint pp_bitjmp(INST *c)
{
// this seems to work..........

  int jf, inx;

  p_pad(LSTFILE,PSCEPOSN);
  xprt(LSTFILE,0,"if (");
  p_opsc(c,csym,2);

  inx = get_swop_inx(c, &jf);          // both cond and static jumps,  not djnz

  xprt(LSTFILE,0, " = %d) ", inx & 1);

  return pp_goto(c);
//return 0;
}


/**************************************************************************
* opcode printout - uses stored opcodes
***************************************************************************/



uint pp_code (uint ofst, LBK *k)
{

  // print code.  NOTE this COPIES from INST chain
  // into modifiable INSTANCE to fiddle with

  int i, next, size;
  const OPC* opl;
  INST *c;

  c = &pinst;
  c->ofst = ofst;          //to find the opcode

   if (!get_mopcode(c))
     {
         pp_hdr (c->ofst, "???", 1);
         return 1;        // not found
     }

  ofst = c->ofst;
  pcmd.cinst = c;           // for any later comments

//  preset_comment(ofst);  // begin and end reqd ??

  size = c->opsize;

  // BANK - For printout, show bank swop prefix as a separate opcode
  // This is neatest way to show addresses correctly in printout

  opl = get_opc_entry(c->opcix);

  if (c->bank)
    {
     pp_hdr (c->ofst, "rombk", 2);          //print rombank first, separate line
     xprt(LSTFILE,0,"%d", c->bank-1);
     pp_hdr (c->ofst+2, opl->name, c->opsize-2); // then rest of opcode
    }
  else

  pp_hdr (c->ofst, opl->name, c->opsize);

// stx is only opcode which does not have op[3] as destination
// stx() handler swops ops to make it same as ldx for easy processing
// so print bare ops backwards here (1,3)
// otherwise standard order (3,2,1)

if (c->sigix == 13)
  {
    p_bareop(c,1);
    pchar(LSTFILE,',');
    p_bareop (c,3);
  }
else
{
  i = c->numops;
  while (i)
     {
      p_bareop (c, i);              // code printout
      if (--i) pchar(LSTFILE,',');
     }
}


//check rbs here ? No...



  opl = get_opc_entry(c->opcix);

  do_sym_names(c,csym);             // cinst)

  if (get_cmdopt(OPTSRC))
    {                         //  source print
      size += opl->pp(c);
    }
  else
    {   // must still print and skip arguments for subroputines
      if (c->sigix == 17)                             // CALL opcodes
         {
           size += pp_subargs(c);
         }
    }

     next = c->ofst+size;

     i = get_tjump_bkts (next);                              // add any reqd close bracket(s)
     if (i) pchar(LSTFILE,' ');

     while (i > 0)
      {               // i = number of brackets (=jumps) found
       xprt(LSTFILE,0,"}");
       i--;
      }

  //   if (PACOM) parse_comment(&acmnt,0);                   // print auto comment if one created, but need to check ofst !!


  // add an extra blank line after END BLOCK opcodes, via comments

 if (next <= k->end)
  {
   if (c->opcix > 79 &&  c->opcix < 84)   pp_comment(next,1);        // Rets
   if (c->opcix == 75 || c->opcix == 77)  pp_comment(next,1);        // jumps
  }

return size;     //opcode size + any args
}



 void do_listing (void)
{
  uint ofst, size;
  int i, lastcom;
  LBK *x;
  BANK *b;

 // anlpass = ANLPRT;                   // safety

DIRS *dirs;

{
HDATA *fh;                 // filepath, bare names, bin size (declared in shared.h)
FDATA *fl;



  fh = get_flhdr();
  fl = get_fldata(2);


  xprt (LSTFILE,1,0);
  p_run(LSTFILE,1, '#', 80);
  xprt (LSTFILE,1,"   SAD Version %s (%s)", SADVERSION, SADDATE);
  p_run(LSTFILE,1, '#', 80);
  xprt (LSTFILE,0,"#\n# Disassembly listing of file '%s'  ",fh->bare);
  i = get_numbanks();

  xprt(LSTFILE,2,"Appears to be %d bank%c,  806%d CPU", i + 1, i ? 's' : ' ', get_cmdopt(OPT8065) ? 5 : 1);

//  xprt(LSTFILE,1,"# See '%s%s' for warnings, results and other information", flhdr.bare, fd[2].suffix);

  xprt(LSTFILE,1,"# See '%s' for warnings, results and other information", fl->fn);
}


  xprt(LSTFILE,0,"\n\n# Explanation of extra flags and formats - ");
  xprt(LSTFILE,0,"\n#   R general register. Extra prefix letters shown for mixed size opcodes (e.g DIVW)");
  xprt(LSTFILE,0,"\n#   l=long (4 bytes), w=word (2 bytes), y=byte, s=signed. Unsigned is default.");
  xprt(LSTFILE,0,"\n#   [ ]=use value as an address   (addresses are always word) ");
  xprt(LSTFILE,0,"\n#   '++' increment register after operation." );
  xprt(LSTFILE,0,"\n# Processor status flags (for conditional jumps)" );
  xprt(LSTFILE,1,"\n#   CY=carry, STC=sticky, OVF=overflow, OVT=overflow trap");
#ifdef  XDBGX
xprt(LSTFILE, 1,"\n# - - - -  DEBUG SET - - - - - ");
#endif
   p_run(LSTFILE,1, '#', 80);

 dirs = get_dirs(0);
 memset(&pcmd, 0, sizeof(CPS));          // clear entire struct, also used for comments

 for (i = 0; i < BMAX; i++)
    {
     b = get_bankmap(i);                       //bkmap+i;

     if (!b->bok) continue;

     if (get_numbanks())
      {
       xprt(LSTFILE,2,0);
       xprt(LSTFILE,1,"###########################################################################");
       xprt(LSTFILE,0,"# Bank %d  file offset %x-%x, ", i-1, b->filstrt, b->filend);
       paddr(LSTFILE,b->minromadd,0);  xprt(LSTFILE,0," - "); paddr(LSTFILE,b->maxromadd,0);
       if (i == 9) xprt(LSTFILE,0, "  CODE/BOOT starts HERE");
       xprt(LSTFILE,1,0);
       xprt(LSTFILE,2,"###########################################################################");
      }

  //   indent = 0;
     lastcom = 0;

     pp_comment(b->minromadd,0);          //may be redundant ?

     for (ofst = b->minromadd; ofst < b->maxromadd; )
      {
         // first, find command for this offset

       x = get_prt_cmd(&prtblk, b, ofst);

       // add newline if changing block print types
 //      xprt = dirs[x->fcom].prtcmd;
       if (dirs[x->fcom].prtcmd != dirs[lastcom].prtcmd || dirs[x->fcom].prtcmd == pp_stct) pp_comment(ofst,1);
       lastcom = x->fcom;

       while (ofst <= x->end)
         {
          show_prog ();
          pp_comment(ofst,0);                      // comments for last opcode - note the check above for swop commands
          size = dirs[x->fcom].prtcmd (ofst, x);
     //     save_pad(LSTFILE);                      // for comments
          if (!size) ofst++;                      // stop infinite loop, safety check
          ofst += size;
         }
       }      // ofst loop

     pp_comment(g_bank(b->maxromadd) + 0x10000,0);    // next bank + 0 - need this for any trailing comments
    }        // bank

  }


