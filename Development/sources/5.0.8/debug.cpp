
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version number and notes in shared.h
 ******************************************************/


#include  "debug.h"

#ifdef XDBGX




 cchar *jtxt[] = {"retp","init","jif","STAT","call", "else"};   // debug text for jump types
 cchar *csub[] = {"", "imd", "inr", "inx"};                     //  0 = register (default) 1=imd, 2 = ind, 3 = inx

// names for chains for debug prtouts, and debug block printers...

  cchar *chntxt[20] = {"",

 "SYM"  , "RBS"  , "SIG"  , "CMD"  , "AUX"  ,
 "SCAN" , "TSCN" , "SUBR" , "ADNL" , "SPF"  ,
 "PSW"  , "TKOP" , "TKDT" , "MTHN" , "MTHX" ,
 "LINK" , "JPFR" , "JPTO" , "BKFD" , // "RGST"
 };


// how to have an array of subroutine pointers.....this works.
// could have by struct type, and by key....

// typedef void DBGPB(CHAIN *, int, void*, cchar *fmt);
// DBGPB *chdbgpt [21];



cchar *get_jtext(uint ix)
{
    return jtxt[ix];
}




cchar *get_chn_name(uint ch)
{
    return chntxt[ch];
}









uint DBGPRT (uint nl, cchar *fmt, ...)
{  // debug file prt with newlines
  va_list args;
  FILE *f;

  f = get_file_handle(5);

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (f, fmt, args);
     va_end (args);
    }
  while (nl--) fprintf(f, "\n");
//  fflush(f);
  return 1;             // useful for if statements
}


void DBGLBK(int nl, LBK *b, cchar *fmt, ...)
{       // debug print for command blk
  va_list args;

  if (!b) return;

  FILE *f;

  f = get_file_handle(5);

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (f, fmt, args);
     va_end (args);
    }

  DBGPRT(0," %05x %05x %s ", b->start, b->end, get_cmd_str(b->fcom));        //       cmds[b->fcom]);
  va_end (args);
  while (nl--) fprintf(f, "\n");
}


/* bit flags
  * 1  new block spans  t  (t within newb)
  * 2  new block within t (t spans newb)
  * 4  front overlap  (newb overlaps front of t)
  * 8  rear  overlap  (newb cannot be inserted)
  * 10 front adjacent
  * 20 rear  adjacent
  * 40 merge allowed (flag set and commands match)
  * 80 t overrides newb
  * 100 newb overrides t (can overwrite it)

  * next flags in order of shift 1,2,4,8,10,20,40, 80
*/



cchar *flgdesc[] = {"Spans","Within","OlapF","OlapR",
"AdjF", "AdjR","Merge","Overriden","Overrides"};

void DBGPRTFGS(int ans,LBK *newb,LBK *t)
{
  // debug prt for olchk (next)
  int i,f;
 DBGPRT(1,0);
 DBGPRT(0,"{DBG ");
  DBGPRT(0,"Olbchk %x for ", ans);
  DBGLBK(0,newb,"Insert");
  i = 0;
  f = 0;
  while (ans)
    {
     if (ans & 1)
     {
       if (f) DBGPRT(0,"|");
       DBGPRT(0, flgdesc[i]);
       f = 1;
     }
     i++;
     ans >>= 1;
    }
  DBGLBK(0,t,0);
  DBGPRT(1,"}");
  }



void DBG_STK(uint nl, SBK *s)
{
  STACK *x;
 // SBK *k;
  uint i;

  DBGPRT(0, " STK (S,C,N)");
  for (i = 0; i < 8; i++)
  {
    if (s->stack[i].type)
     {
      DBGPRT(0, " [%d",i);
      if (i == s->stkptr) DBGPRT(0,"*"); //else DBGPRT(0," ");
      DBGPRT(0,"]");

      x = s->stack+i;

   //   if (x->type == 1 || x->type == 2)
   //     {
         DBGPRT(0,"%d,%x,%x,%x", x->type, x->start, x->curaddr, x->nextaddr);
 //       }

  //    if (x->type == 3)
  //      {
                      //   psp = x->psp;
   //       DBGPRT(0,"{P} %x ^%x",x->start, x->curaddr);
                       //DBGPRT(0,"PSW %x CBNK %d RBNK %d", d->psw, d->codebank, d->rambank);
   //     }

    //    if (x->type == 4)
   //     {
                      //   psp = x->psp;
    //      DBGPRT(0,"{H} S%x N%x ^%x", x->start, x->nextaddr, x->curaddr);
                       //DBGPRT(0,"PSW %x CBNK %d RBNK %d", d->psw, d->codebank, d->rambank);
     //   }


     }

   }

//   if (s->jfrom)
  //    {
   //     k = s->jfrom;
   //     DBGPRT(0, " JMP %x (%x)", k->start, k->nextaddr);
  //    }
   if (nl) DBGPRT(nl,0);
}








void DBG_sbk(cchar *t, SBK *s)
{

 if (t) DBGPRT(0,"%s ", t);
 if (s->proctype == 4) DBGPRT(0,"E");
 DBGPRT  (0,"Scan %05x %05x", s->start, s->nextaddr);         // nextaddr is end, effectively
 //if (s->jcaller) DBGPRT(0," Caller %x", s->jcaller->nextaddr);
 if (s->ssjp)    DBGPRT(0," J"); else DBGPRT(0,"  ");
 if (s->ssub )   DBGPRT(0," U"); else DBGPRT(0,"  ");

 if (s->stop)    DBGPRT(0," X"); else DBGPRT(0,"  ");
// if (s->ignore)  DBGPRT(0," I"); else DBGPRT(0,"  ");
 if (s->emureqd) DBGPRT(0," E"); else DBGPRT(0,"  ");
 if (s->getargs)    DBGPRT(0," A"); else DBGPRT(0,"  ");
 if (s->levf)    DBGPRT(0," L%d", s->levf);  else DBGPRT(0,"   ");


 DBG_STK(0,s);


 if (s->adtsize)   DBGPRT(0," FARGS %d", s->adtsize);
 if (s->usrcmd)       DBGPRT(0," usr");
 if (s->inv)       DBGPRT(0," INV");
// if (s->spf)       DBGPRT(0," SPF");

 prt_spf(s,1,DBGPRT);
 DBGPRT(1,0);
}


void DBG_scans(void)
{

  uint ix;
  CHAIN *x;

  x = get_chain(CHSCAN);

   DBGPRT(1,"# dbg scans chnum = %d blk scans = %d chain scans %d", x->num, tscans, xscans);

   for (ix = 0; ix < x->num; ix++)
    {
        DBGPRT(0,"[%4d] ", ix);
    DBG_sbk(0,(SBK*) x->ptrs[ix]);

    }
 DBGPRT(1,0);
}


void DBG_links (void)
{

  FKL *l;
  uint ix;

  CHAIN *x;

  x = get_chain(CHLINK);
  DBGPRT(2,0);
  DBGPRT(1,"# links num = %d", x->num);

//  DBGPRT(1, "forwards");

  for (ix = 0; ix < x->num; ix++)
      {
        l = (FKL*) x->ptrs[ix];

// now for multiple queues

   //     DBGPRT(0, "%lx(%2d) -> %lx(%2d)", l->keysce,l->chsce, l->keydst, l->chdst);
        DBGPRT(0, "%lx -> %lx %d", l->keysce,l->keydst, l->type);

    DBGPRT(0, " %s ", get_chn_name(l->chsce));

    if (l->chsce == CHDTK)     DBGPRT(0, " %x", ((TRK*)l->keysce)->ofst);
    if (l->chsce == CHDTD)     DBGPRT(0, " %x", ((DTD*)l->keysce)->stdat);
    if (l->chsce == CHSCAN)    DBGPRT(0, " %x", ((SBK*)l->keysce)->start);
    if (l->chsce == CHSIG)     DBGPRT(0, " %x", ((SIG*)l->keysce)->start);
    if (l->chsce == CHADNL)    DBGPRT(0, " %x", ((ADT*)l->keysce)->cnt);
    if (l->chsce == CHMATHN)    DBGPRT(0, " 0");       //, ((MATHX*)l->keysce)->cnt);


    DBGPRT(0, " %s ", get_chn_name(l->chdst));
    if (l->chdst == CHDTK)     DBGPRT(0, " %x", ((TRK*)l->keydst)->ofst);
    if (l->chdst == CHDTD)     DBGPRT(0, " %x", ((DTD*)l->keydst)->stdat);
    if (l->chdst == CHSCAN)    DBGPRT(0, " %x", ((SBK*)l->keydst)->start);
    if (l->chdst == CHSIG)     DBGPRT(0, " %x", ((SIG*)l->keydst)->start);
    if (l->chdst == CHADNL)    DBGPRT(0, " %x", ((ADT*)l->keydst)->cnt);
    if (l->chdst == CHMATHN)   DBGPRT(0, " 0");      //, ((MATHX*)l->keydst)->cnt);
     DBGPRT(1,0);

      }


       DBGPRT(2,0);
    }



void DBG_dtkx (TRK *k)
{
  DTD *d;
  FKL *l;
  const OPC *c;
  uint wop, i, ix;

  wop = 3;

  c = get_opc_entry(k->opcix);

  DBGPRT(0,"%5x, (%5x), %5s ", k->ofst, k->ofst+k->ocnt, c->name);

  for (i = 0; i <= 4; i++)
    {
     if (i && wop == i) DBGPRT(0,"*"); else DBGPRT(0," ");
     DBGPRT(0,"R%4x ", k->rgvl[i]);
    }

  DBGPRT(0," BS %5x", k->base);
  DBGPRT(0," OFF %d", k->offset);

  DBGPRT(0," data ");

  l = get_link(k,0,0);
  ix = get_lastix(CHLINK)+1;



  while (l)
    {
       if (l->keysce != k) break;
       d = (DTD*) l->keydst;               //dst

        DBGPRT(0,"D %5x", d->stdat);
    //    if (d->olp) DBGPRT(0,"O");
     //   if (d->psh) DBGPRT(0," P");
    //    if (d->gap) DBGPRT(0," G%d",d->gap);
     //   if (d->olp) DBGPRT(0," O");
        DBGPRT(0,",");
        l = (FKL*) get_next_item(CHLINK, &ix);
      }
 //   }
    DBGPRT(1,0);
  }


cchar* opct [] = {"reg", "imd", "inr", "inx"};


void DBG_dtk (void)
{

  TRK *k;
  DTD *d;
  FKL *l;
  uint ix, f,i;
 // void *fid;
  CHAIN *x;

  x = get_chain(CHDTK);

  DBGPRT(3,0);
  DBGPRT(1,"# ------------ data tracking list");
  DBGPRT(1,"# num items = %d", x->num);

  DBGPRT(1,"from, next, addr, size");
  DBGPRT(1,"# by opcode");

  for (ix = 0; ix < x->num; ix++)
      {

        DBG_dtkx ( (TRK*) x->ptrs[ix] );
    }


  DBGPRT(1,"# by data");

  x = get_chain(CHDTD);
  for (ix = 0; ix < x->num; ix++)
      {
        d = (DTD*) x->ptrs[ix];
        DBGPRT(0,"D %5x (%x)", d->stdat, d->modes);

        f = d->modes;
        i = 0;
  while (f)
    {
       if (f & 1)  DBGPRT(0, "%s,", opct[i]);
       i++;
       f >>= 1;
    }

      //   DBGPRT(0," BS %5x", d->base);
   //      DBGPRT(0," OF %d", d->offset);

   if (d->bsze) DBGPRT (0," S%d", d->bsze);
   if (d->gap1) DBGPRT (0," G%x", d->gap1);
   if (d->gap2) DBGPRT (0,",%x", d->gap2);
   if (d->inc)  DBGPRT (0," +%d", d->inc);
   if (d->psh)  DBGPRT (0," PSH");
   if (d->pop)  DBGPRT (0," POP");
   if (d->olp)  DBGPRT (0," OLP");
   if (d->rbs)  DBGPRT (0," RBS");
   if (d->bcnt) DBGPRT (0," BCNT %d", d->bcnt);
   if (d->icnt) DBGPRT (0," ICNT %d", d->icnt);

  DBGPRT(0,"   opcodes ");


  l = get_link(d,0,0);     //data <- opcode
  i = get_lastix(CHLINK) + 1;

    while (l)
      {
        k = (TRK*) l->keydst;  //to opcode

        if (l->keysce != d ) break;

        DBGPRT(0," %5x", k->ofst);
   //     DBGPRT(0, "[%d", k->opcsub);
   //     if (k->psh) DBGPRT(0," PSH");
     //   if (k->pop) DBGPRT(0," POP");
     //   if (k->ainc) DBGPRT(0," +%d",k->ainc);
       // DBGPRT(0, "]");
        l = (FKL*) get_next_item(CHLINK, &i);
        DBGPRT(0,",");
      }


DBGPRT(1,0);
      }

   DBGPRT(2,0);
  }




void DBG_patt(uint *scores)
{

// debug printout
uint rowno, rowsize, scix;


 #ifdef XDBGX
     DBGPRT (1, "Patt analysis Result");


     DBGPRT(0,"\nrows    ");

     for (rowno = 0; rowno < 32; rowno++)
            {
              DBGPRT(0, "%3d, ", rowno);
            }
DBGPRT(1,0);

          for (rowsize = 0;  rowsize < 32; rowsize++)
          {    // by colsize
            DBGPRT(0,"\nsize %2d ", rowsize);
       //     aix = rowsize*32;

           // scores[aix] = 0;
            for (rowno = 0; rowno < 32; rowno++)
             {  // score by rowno
               scix = (rowsize*32)+rowno;     //  to get to right cell


  //     if (rowno > 0 && scores[scix] > scores[aix]) //   <= scores[aix])    break;    //TEST
          //     scores[aix] = scores[scix];        //     ans = scores[scix];

               if (scores[scix]) DBGPRT(0, "%3d, ", scores[scix]);
               else DBGPRT(0,"   , ");
             }
           }
DBGPRT(1,0);

   #endif
}


void DBG_jumps (uint chn, cchar *z)
{
 JMP *j;
 CHAIN *x;
 uint ix, sb;

  x = get_chain(chn);

  DBGPRT(1,"------------ Jump list %s", z);
  DBGPRT(1,"num jumps = %d", x->num);



  sb = 0;
  for (ix = 0; ix < x->num; ix++)
    {
      j = (JMP*) x->ptrs[ix];

      if (j->jtype == J_SUB) sb++;
    //  DBGPRT(0,"%5x->%5x sub %5x",j->fromaddr, j->toaddr, j->fsubaddr);
      DBGPRT(0,"%5x->%5x ",j->fromaddr, j->toaddr);

      if (j->cmpcnt)  DBGPRT(0," cmp %5x",j->fromaddr - j->cmpcnt); else DBGPRT(0,"          ");
      DBGPRT(0," e %5x", j->fromaddr + j->size);
      DBGPRT(0," %s",jtxt[j->jtype]);
      if (j->bswp)  DBGPRT (0," bswp");
      if (j->back)  DBGPRT (0," back");
   //   if (j->uci)   DBGPRT (0," uci");
  //    if (j->uco)   DBGPRT (0," uco");
      if (j->retn)  DBGPRT (0," retn");

  //    if (j->subloop)   DBGPRT (0," SUB");
  //    if (j->jelse)   DBGPRT (0," ELSE");
//if (j->inloop)   DBGPRT (0," inloop");
//if (j->exloop)   DBGPRT (0," exloop");

      if (j->obkt)  DBGPRT (0," {");
      if (j->cbkt)  DBGPRT (0," }");
      DBGPRT(1,0);
    }

    DBGPRT(1, "%d subr calls", sb);
    DBGPRT(1,0);
}


void DBG_jumps ()
{
DBG_jumps (CHJPF,"Forwards");
DBG_jumps (CHJPT,"To ");
}

void DBG_sigs(void)
{
 uint i, ix;
 SIG *y;
 CHAIN *x;

 x = get_chain(CHSIG);


 DBGPRT(1,0);
 DBGPRT(1,"-----------Signatures---------[%d]--", x->num);
 DBGPRT(1,"d ofst  end   HN name         pars" );

 for (ix = 0; ix < x->num; ix++)
   {
     y = (SIG*) x->ptrs[ix];
     DBGPRT(0,"%d %5x %5x %2d %-10s  ", y->done, y->start, y->end, y->hix, y->name);

     for (i = 0; i < NSGV; i++)
     {
        if (i == 16)  DBGPRT(0,"\n                             ");
        DBGPRT(0,"%5x," , y->v[i]);
     }
     DBGPRT(2,0);
    }

 DBGPRT(1,0);
}



void DBG_mx(void *fid, uint ix, uint lev)
{
    // ix is param number.....

// fid ends in 0 for main terms 1,2,3 for param subterms
// ix is always zero ??

  uint x;
  MATHX *a;
  MNUM *n;

  while ((a = get_mterm(fid,ix)))
   {
    fid = a;
    ix = 0;
    for (x = 0; x < lev; x++)  DBGPRT(0,"    ");      //indent
    // DBGPRT (0, "%x %x  (%d)", a, a->fid, a->subf);
    DBGPRT (0, "[%lx] -> %lx (%d)  ", a, a->fid,a->pix);
     if (a->first) DBGPRT(0, "(f) ");
  //        if (a->noname) DBGPRT(0, "(n) ");
 //    if (a->subt) DBGPRT(0, "(s) ");
   //   DBGPRT(0,"ctype %d ",a->calctype);
     DBGPRT(0, "func= %s ", get_mathfname(a->func));

     DBGPRT (0, "N%d ", a->npars);

     for (x = 0; x < 3; x++)
      {
       n = a->fpar + x;

       DBGPRT(0, "P%d[%d] ",x, n->dptype);

// 0 empty  1 union is integer,  2 union is ref,  3 union is float, 7 subterm/subcalc attached
       switch(n->dptype)
         {
          default:
            break;

          case DP_DEC:
           DBGPRT (0, "%d ", n->ival);
           break;

          case DP_REF:
           DBGPRT (0, "x ");
           break;

         case DP_FLT :
           DBGPRT (0, "%.3f ", n->fval);
           break;

         case DP_HEX:
           DBGPRT (0, "%x ",n->ival);
      //   DBGPRT(0, "%d ", n->refno);
           break;
        }
      }

     DBGPRT(1,0);

    for (x = 0; x < 3; x++)
      {
       n = a->fpar + x;

       if (n->dptype == DP_SUB)
        DBG_mx(fid,x+1,lev+1);   //sub instead of par

      }

     a->prt = 1;    // mark printed


      }

}






void DBG_math(void)

{
  MATHN *x;
  MATHX *a;
  uint ix;
  CHAIN *ch;


  ch = get_chain(CHMATHN);

  DBGPRT(1,0);
  DBGPRT(2,"-----------Math Calcs---------[%d]--", ch->num);


 for (ix = 0; ix < ch->num; ix ++)
 {
    x = (MATHN *) ch->ptrs[ix];
    if (x->nsize) DBGPRT(0, x->mathname); else DBGPRT(0," <0> ");
    DBGPRT (0, " [%lx] %lx ", (ulong) x, (ulong) x->fid);


    //DBGPRT(0,"ctype %d, sys %d ",x->calctype, x->sys);
       DBGPRT(0,"ctype %d ",x->mcalctype);
    if (x->suffix) DBGPRT(0, "%s" ,x->suffix);

    DBGPRT (1, "\nTerms ----  fid, add, func, fval|ref");
    DBG_mx(x,0,0);         //name fk in first math term
    DBGPRT(1,0);

 }



DBGPRT(2,0);

  ch = get_chain(CHMATHN);

  DBGPRT (2, "ORPHANS / NONAMES ----  x FID  subf func fval|ref");

for (ix = 0; ix < ch->num; ix ++)
 {
    a = (MATHX *) ch->ptrs[ix];
   if (!a->prt) DBG_mx(a->fid,0,0);
  }

DBGPRT (2,0);
 DBGPRT (2, "ORDER (temp)----  x FID ");
for (ix = 0; ix < ch->num; ix ++)
 {
    a = (MATHX *) ch->ptrs[ix];
     DBG_mx(a->fid,0,0);
  }


DBGPRT (3,0);
}


void DBG_rgstat(void)
{
    uint i;
    RST *g;
    RPOP *p;

    for (i = 0; i <= 0x3ff; i++)
     {
      g = get_rgstat(i);

if (g)
{
      DBGPRT(0,"%x ", i);
      DBGPRT(0,"sz=%d ", get_fsize(i));
      DBGPRT(1,"sreg=%d", g->sreg);
      }

     }

DBGPRT(2,0);

    for (i = 0; i < 8; i++)
     {
      p = get_rpop(i);

      if (p)
      {
         DBGPRT(0,"%x ", i);
         DBGPRT(0,"tgt  %x %x", p->tgtscan, p->tgtemu);
         DBGPRT(0,"arg  %x %x", p->argscan, p->argemu);

      }

     }

DBGPRT(2,0);


}


void DBG_data()
 {

   DBGPRT(1," -- DEBUG INFO --");

    DBG_jumps();

    DBG_scans();
    prt_banks(DBGPRT);
    DBGPRT(1,"rgstat data");
    DBG_rgstat();


    DBGPRT(1,"rbase chain");
    prt_chain(CHBASE, DBGPRT);       //rbt (DBGPRT);

        DBGPRT(1,"subs chain");
    prt_chain (CHSUBR, DBGPRT); // subs(DBGPRT);
        DBGPRT(1,"symbols chain");
    prt_chain(CHSYM,DBGPRT);
    DBGPRT(1,"command chain");
    prt_cmds(DBGPRT);
    DBGPRT(1,"adnl chain");
    prt_chain(CHADNL,DBGPRT);
      DBGPRT(1,"spf chain");
    prt_chain(CHSPF,DBGPRT);
    DBG_links();
    DBG_dtk();
    DBG_sigs();
    DBG_math();

    DBGPRT(1, "alloced %d", DBGalloced);
 }


#endif

