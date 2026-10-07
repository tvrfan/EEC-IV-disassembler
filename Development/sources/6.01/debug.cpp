
//#include  <stdio.h>


#include  "shared.h"                 //declares XDBGX


#include  "core.h"
#include  "sign.h"


#ifdef XDBGX



// names for chains for debug prtouts, and debug block printers...

  cchar *chntxt[] = {     "jmpf"  , "jmpt"     , "symb"    , "rbas"       ,    "sign" ,
                          "cmd"   , "auxc"     , "scan"    , "escan"      ,    "subr" ,
                          "adnl"  ,  "spf"     , "psw"     , "dofst"      , "ddata" ,
                          "bnkf"  , "opcd"     , "rgst"   ,  "rgsta"      , "mname",
                          "mterm" , "link"
                          };



extern int DBGmcalls;            // malloc calls
extern int DBGmrels;             // malloc releases
extern int DBGmaxalloc;          //  TEMP for malloc tracing
extern int DBGcuralloc;
extern int DBGmoves;

extern int tscans;
extern int xscans;

RBT* get_rbt(uint reg, uint pc);

CHAIN *get_chain(uint chn);

DIRS *get_dirs(uint ix);
void list_cmds(uint);

void list_subs (uint);
void list_syms (uint);
void list_banks   (uint);
void list_rbt  (uint);

SYM* get_sym(int , uint , int , int);
DTD * get_dtkd(uint,uint,uint);
MATHX *get_mterm(void *fid, int pix);

DTD *start_dtdk_loop(uint);
DTD *get_next_dtkd(DTD *);
FDATA* get_fldata(uint);

 //POP* get_pop (void *fid)  ;

uint xprt (uint, uint , cchar *, ...);
FSTK*  get_stack(uint proctype, uint ix);

void chk_mx(void *fid, uint ix, uint *chks);

uchar* get_signame(uint patno);

// how to have an array of subroutine pointers.....this works.

// typedef void DBGPB(CHAIN *, int, void*, const char *fmt);

// DBGPB *chdbgpt [4];













uint DBGPRT (uint nl, cchar *fmt, ...)
{  // debug file prt with newlines
  va_list args;
  uint chars;

  FDATA *f;

  f = get_fldata(DBGFILE);

  chars = 0;
  if (fmt)
    {
     va_start (args, fmt);
     chars = vfprintf (f->fh, fmt, args);
     va_end (args);
    }
  while (nl--) fprintf(f->fh, "\n");
  return chars;             // useful for if statements
}


void DBGLBK(uint nl,LBK *b)
{       // debug print for command blk

CSTR *c;

c = get_cstr(CMDSTR, b->fcom);
  DBGPRT(nl," %s %05x %05x ", c->string, b->start, b->end);
}



void DBG_stack (uint proctype)
 {
   FSTK *t;
   int i;

  t = get_stack(proctype,0);

  if (get_anlpass() >= ANLPRT) return;

  if (proctype == 4)  DBGPRT(0,"E"); else DBGPRT(0," ");
  DBGPRT(0,"STK ");
  t--;

  for (i = -1; i < STKSZ; i++)
   {
    if (!t->ftype && i >=0 ) break;
    DBGPRT (0," [%d]", i);
    DBGPRT (0," R%x T%d " , t->popreg, t->ftype);
    if (t->sblk) DBGPRT(0," S %x %x", t->sblk->start, t->sblk->nextaddr);
    t++;
   }
  DBGPRT(2,0);

 }


void DBG_chn(uint ch)
{
  uint size;
  CHAIN *x;

  x = get_chain(ch);

    size =  x->pnum * sizeof(void *);     // number of void* pointers
    size += x->pnum * x->esize;           // plus structs pointed to
    size /= 1024;
 //   DBGPRT(1,"max=%5d tot=%5d cur=%5d allocs=%2d esize=%3d size=%5dK   %s", DBGmax[ch], x->pnum, x->num, DBGallocs[ch], x->esize, size, chntxt[ch]);
      DBGPRT(1,"tot=%6d cur=%5d esize=%3d size=%5dK   %s", x->pnum, x->num, x->esize, size, chntxt[ch]);

 // DBGPRT(2,0);

 }

/*
void DBG_rgstat(RST *r)
{
    DBGPRT(0,"R%x (%d) %x", r->reg, r->fend, r->ofst);         //ssize
    DBGPRT(0," SR%x", r->sreg);
    if (r->arg) DBGPRT(0," Arg");
    if (r->popped) DBGPRT(0," Pop");
    if (r->inr) DBGPRT(0," INR");
  //  if (r->used) DBGPRT(0," Used");
    if (r->sarg) DBGPRT(0," Sarg");
    if (r->inc) DBGPRT(0," ++");
    if (r->enc) DBGPRT(0," enc %d %x", r->enc, r->data);
    DBGPRT(1,0);
}
*/
/*void DBG_rgchain(void)
{

  uint ix;
  RST *r;
  CHAIN *x;

  x = get_chain(CHRGST);

   DBGPRT(1,"# chrgst num = %d", x->num);

   for (ix = 0; ix < x->num; ix++)
    {
     r = (RST*) x->ptrs[ix];
     DBG_rgstat(r);
    }
 DBGPRT(1,0);
}*/


//flags in order of shift 1,2,4,8,10,20,40, 80

cchar *flgdesc[] = {"Spans","Within","Front","Rear",
"Overriden by","Overrides","Merge", "Equal",};



void DBGPRTFLGS(int ans,LBK *newb,LBK *t)
{
  // debug prt for olchk (next)
  int i,f;


  DBGPRT(0,"Olchk %x for ", ans);

  DBGPRT(0,"Insert");

  DBGLBK(0,newb);

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
  DBGLBK(0,t);
  }


// SBK - uint  type       : 3;         // 1 - cond, 2 = stat, 4 = sub

cchar* td[8] = {" 0! ","init","cond", "stat","subr"," !5 "," ESUB ", " !7 "};


void DBG_sbk(cchar *t, SBK *s)
{

 if (t) DBGPRT(0,"%s",t);
 DBGPRT  (0," %05x %05x (%d)", s->start, s->nextaddr, s->proctype);         // nextaddr is end, effectively

 DBGPRT(0," scnt %d %s", s->scnt, td[s->type]);

 if (s->substart)  DBGPRT(0," sub %x", s->substart); else DBGPRT(0," No sub   ");
 if (s->caller)    DBGPRT(0, " caller %x (%x)", s->caller->start, s->caller->nextaddr);else DBGPRT(0," No caller   ");
 if (s->emulreqd)  DBGPRT(0," EMURQ");
 if (s->argsget)      DBGPRT(0," ARGS");
 if (s->pushp)       DBGPRT(0," PSHP");
// if (s->cmd)       DBGPRT(0," cmd");
 //if (!s->logdata)  DBGPRT(0," NDAT");
 if (s->inv)       DBGPRT(0," INV");
 //if (!s->stop)     DBGPRT(0," NOT Scanned");
   if (s->stop)     DBGPRT(0," Scanned");
// if (s->gapchk)    DBGPRT(0," GAP");

 DBGPRT(1,0);

 DBG_stack(s->proctype);
}


void DBG_scans(void)
{
  uint ix;
  CHAIN *x;
  SBK *s;

  x = get_chain(CHSCAN);

   DBGPRT(1,"# dbg scans chnum = %d blk scans = %d chain scans %d", x->num, tscans, xscans);

   for (ix = 0; ix < x->num; ix++)
    {
   // DBG_sbk("Scan",(SBK*) x->ptrs[ix]);
      s = (SBK*) x->ptrs[ix];

      DBGPRT(0,"scan");
 DBGPRT  (0," %05x %05x (%d)", s->start, s->nextaddr, s->proctype);         // nextaddr is end, effectively

 DBGPRT(0," scnt %d %s", s->scnt, td[s->type]);

 if (s->substart)  DBGPRT(0," sub %x", s->substart); else DBGPRT(0," No sub   ");
 if (s->caller)    DBGPRT(0, " caller %x (%x)", s->caller->start, s->caller->nextaddr);else DBGPRT(0," No caller   ");
 if (s->emulreqd)  DBGPRT(0," EMURQ");
 if (s->argsget)      DBGPRT(0," ARGS");
 if (s->pushp)       DBGPRT(0," PSHP");
// if (s->cmd)       DBGPRT(0," cmd");
 if (s->tstvect)  DBGPRT(0," TEST");
 if (s->inv)       DBGPRT(0," INV");
 if (!s->stop)     DBGPRT(0," NOT Scanned");
//   if (s->stop)     DBGPRT(0," Scanned");
// if (s->gapchk)    DBGPRT(0," GAP");

 DBGPRT(1,0);

    }
 DBGPRT(1,0);
}


void DBG_opc(void)
{
  uint ix,i;
  CHAIN *x;
  INST *c;
  OPER *s;

  const OPC *p;

  x = get_chain(CHOPC);

   DBGPRT(1,"# opcodes num = %d", x->num);

   DBGPRT(1,"# addr-nops-opname 0,1,2,3");


   for (ix = 0; ix < x->num; ix++)
    {
      c = (INST*) x->ptrs[ix];
   //   c = h->cinst;
      p = get_opc_entry(c->opcix);
      DBGPRT(0,"%x ",c->ofst);
         DBGPRT(0,"[%d] %d ", c->opcsub, c->opsize);
      DBGPRT(0, "%s ", p->name);

      for (i = 0; i < 4 ; i++)
      {
          s = c->opnd+i;

/* (20 bits)
 uint fend    : 8;         // field size +0x20 (sign) +0x40 (write)

 uint ind     : 1;      // indirect
 uint inx     : 1;      // indexed, use with operand [0]
 uint imd     : 1;      // this is an immediate value
 uint rgf     : 1;      // this is a  register  value
 uint bit     : 1;      // this is a  bit       value
 uint adr     : 1;      // this is an address (includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with sign)
 uint neg     : 1;      // offset address is negative
 uint inc     : 1;      // autoinc (easier for printout....)

 uint bkt     : 1;      // print square brackets
 uint rbs     : 1;      // register is an rbase
 uint sym     : 1;      // 1 if sym found (printout only)
} OPS;  */


          if (s->fend) {

            DBGPRT(0, "[%d] %x %d ", i, s->fend, s->optype);
  /* reg = 0;   (and addr)
 imd = 1
ind = 2;   inx = 3         (as opcsub)
bit = 4;   adr = 5;
off = 6;
       */
       #define  OPIMD  1
#define  OPBIT  2        // with register
#define  OPADDR 3        //
#define  OPOFF  4        //only op [0] ?
#define  OPIND  5
#define  OPINX  6
#define  OPAINC 7       // autoinc and indirect
            if (s->optype == OPIMD)  DBGPRT(0,"I");   //imd
            if (s->optype == OPBIT)  DBGPRT(0,"B");   //bit
            if (s->optype == OPADDR) DBGPRT(0,"A");   //addr
            if (s->optype == OPOFF)  DBGPRT(0,"O");   //offset
            if (s->optype == OPIND)  DBGPRT(0,"I");   //indirect
            if (s->optype == OPINX)  DBGPRT(0,"X");   //indexed
            if (s->optype == OPAINC) DBGPRT(0,"+");   // autoinc (indirect)

                  if (s->neg) DBGPRT(0,"-");
      //            if (s->inc) DBGPRT(0,"+");
                  if (s->rbs) DBGPRT(0,"S");

            DBGPRT(0, "(R %x A %x V %x)  ", s->reg, s->addr, s->val);
          }
      }
        DBGPRT(1,0);
    }



 DBGPRT(3,0);
}








//change to per operand

/*
void DBG_dtkx (TRK *k)
{
 // uint start;  //,fend;
 // CHAIN *x;
  DTD *d;
  uint i;
  const OPC *c;

  c = get_opc_entry(k->opcix);

  DBGPRT(0,"%5x, (%5x), %5s ", k->ofst, k->ofst+k->ocnt, c->name);

  DBGPRT(0,"%-4x ", k->rgvl[0]);
  for (i = 1; i <= 3; i++)
    {





        if (i == 3) DBGPRT(0,"*"); else DBGPRT(0," ");
     DBGPRT(0,"R%-3x ", k->rgvl[i]);
    }
 //  DBGPRT(0,"S%-3x ", k->rgvl[4]);  //saved


  switch(k->opcsub)
    {
      case 0:
        DBGPRT(0, "reg ");
        break;
      case 1:
        DBGPRT(0, "imd ");
        break;
      case 2:
        DBGPRT(0, "inr [R%x]",k->rgvl[1]);
        break;
      case 3:
    //     if (k->rbsx) DBGPRT(0, "rbs "); else
         DBGPRT(0, "inx ");
         if (k->rgvl[0] & 0x8000) DBGPRT(0, "[-%x]", 0x10000 - k->rgvl[0]);
         else DBGPRT(0, "[+%x]", k->rgvl[0]);
        break;
      default:
        break;
    }
    if (k->ainc) DBGPRT(0, " +%x", k->ainc);

 //   x = get_chain(CHDTKO);

    d = (DTD*) chmem(CHDTKO);              //&chdtko);
    if (d) d->ofst = k->ofst;

    while ((d = get_next_dtkd(d)))
      {
        DBGPRT(0," %5x", d->stdat);
    //    if (d->olp) DBGPRT(0,"O");
        if (d->psh) DBGPRT(0," P");
        if (d->gap) DBGPRT(0," G%d",d->gap);
        if (d->olp) DBGPRT(0," O");
        DBGPRT(0,",");
      }
    DBGPRT(1,0);
  }


const char* opct [] = {"reg", "imd", "inr", "inx"};




*/

void DBG_dtd (void)
{


  DTD *d;
  uint ix;
  CHAIN *x;
//  CINST *c;
 // COPER *o;

  x = get_chain(CHDTDO);

  DBGPRT(1,0);
  DBGPRT(1,"# ------------ data tracking list");
  DBGPRT(1,"# num items = %d", x->num);

  DBGPRT(1,"# by ofst");

  for (ix = 0; ix < x->num; ix++)
      {
        d = (DTD*) x->ptrs[ix];

         DBGPRT(0," %5x %5x %d %d (%x)", d->ofst, d->dataptr, d->opcix, d->optype, d->fend);
        if (d->test) DBGPRT(0, " TST");
        DBGPRT(1,0);
    }


  DBGPRT(1,"# by data");

  x = get_chain(CHDTDD);
  for (ix = 0; ix < x->num; ix++)
      {
        d = (DTD*) x->ptrs[ix];

           DBGPRT(0," %5x %5x %d %d (%x)", d->ofst, d->dataptr, d->opcix, d->optype, d->fend);
           if (d->test) DBGPRT(0, " TST");
        DBGPRT(1,0);
      }
   DBGPRT(2,0);
  }




void DBG_adt (ADT *a, uint flag)
 {
   while (a)
   {
    if (!a->xprt)
      {
         DBGPRT(0,"[ fid %lx (%lx) ",  a->fid, a);
         DBGPRT(0,"O %d ", a->cnt);
         DBGPRT(0, "B %x %x ", a->fstart, a->fend);
         DBGPRT(0, "K %d ", a->bank);
         DBGPRT(0, "P %x.%x ", a->pfw, a->pfd);
         DBGPRT(0, "T %d ", a->dptype);
         if (a->newl) DBGPRT(0, " |");
         if (a->fnam) DBGPRT(0, " N");
         DBGPRT(0, "] ");
         if (flag) a->xprt = 1;           //not for command setup
            DBGPRT(1,0);
      }

     //subs ???


    a = get_adt(a,0);
   }

 }




void DBG_adnls (void)
{
 CHAIN *x;    //, *l;
 ADT *a;
 uint ix;
// void *fid;

  x = get_chain(CHADNL);



 DBGPRT(1,"# ---------Additional num = %d", x->num);


 for (ix = 0; ix < x->num; ix++)
   {
     a = (ADT*) x->ptrs[ix];
     DBG_adt(a,1);
   }


 /*
   if ((ulong) a->fid < 0x100000){ fid = a->fid; //  DBG_adt(a);

  // fid = a;
   while ((a = get_adt(fid,0)))
      {
          DBG_adt(a);
       fid = a;
      }
   DBGPRT(1,0);}


   b = get_adt(a,1);          sub queue
if (b)
{
  void *fid;
  //  list_adnl(fno,b, fcom);
    fid = b;
    while ((b = get_adt(fid,0)))
      {
     //  if (b->cnt || fno != MSGFILE)
     //      list_adnl(fno, b, fcom);  // ignore if zero size, and not debug
       fid = b;
      }
   */


   DBGPRT(2,0);
   DBGPRT(2, "ORPHANS ");

 for (ix = 0; ix < x->num; ix++)
   {
   a = (ADT*) x->ptrs[ix];
   if (!a->xprt)
   {
       DBGPRT(0, "ORPHAN ");
       DBG_adt(a,1);
   }
   }
}


void DBG_cmd(uint fno,LBK *k)
{
  DIRS *d;
  uint f;
  CSTR *c;
  ADT *a;

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
       xprt (fno,0,"  $");
       xprt (fno,0," Q");
       if (k->term > 1) xprt(fno,0," %d", k->term);
       f = 1;
      }

   if (d->cmpargs)
    {
        //args option in force
      if (get_cmdopt(OPTCMPA) != k->cptl)
          {
            if (!f) xprt (fno,0,"  $");
            if (k->cptl) xprt (fno,0," C"); else xprt (fno,0," A");
          }
    }

  if (d->cmpdata)
    {
        //args option in force
      if (get_cmdopt(OPTCMPD) != k->cptl)
          {
            if (!f) xprt (fno,0,"  $");
            if (k->cptl)  xprt(fno,0," C"); else xprt(fno,0," A");
          }
    }


/*
   if (!c->cptl)                   //argl)
      {
       if (!f)  prt(MSGFILE,0,"  $");
        prt(MSGFILE,0," A");
      }  */

   a = get_adt(vconvi(k->start),0);
   DBG_adt(a,0);              //fno,vconvi(k->start),k->fcom);

  // if (k->usrcmd) { list_usrcmt(fno);}
   xprt(fno,1,0);
  }












void DBG_cmds(uint fno)
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
         DBG_cmd(fno,aux);
         ixa++;
         if (ixa < caux->num) aux = (LBK*) caux->ptrs[ixa];
         else aux = 0;
        }

     DBG_cmd(fno,cmd);

    }
    xprt(fno,2,0);
}



  cchar *jtxt[] = {"retp","init","jif ","STAT","call", "else"};   // debug text for jump types

void DBG_jmp (uint ch)
{
 JMP *j;
 uint ix;
 CHAIN *x;
 uint cnts[6];

 memset(cnts,0, sizeof(uint)*6);

 x = get_chain(ch);

 for (ix = 0; ix < x->num; ix++)
    {
      j = (JMP*) x->ptrs[ix];
      DBGPRT(0,"%5x->%5x",j->fromaddr, j->toaddr);
      DBGPRT(0," %s  %d   ",jtxt[j->jtype], j->size);

      if (j->test)  DBGPRT (0," test"); else DBGPRT(0,"     ");
      if (j->bswp)  DBGPRT (0," bswp"); else DBGPRT(0,"     ");
      if (j->back)  DBGPRT (0," back"); else DBGPRT(0,"     ");
   //   if (j->uci)   DBGPRT (0," uci");
  //    if (j->uco)   DBGPRT (0," uco");
      if (j->retn)  DBGPRT (0," retn"); else DBGPRT(0,"     ");
      if (j->jelse)   DBGPRT (0," else"); else DBGPRT(0,"     ");

  //    if (j->subloop)   DBGPRT (0," SUB");

//if (j->inloop)   DBGPRT (0," inloop");
//if (j->exloop)   DBGPRT (0," exloop");

      if (j->obkt)  DBGPRT (0," {"); else DBGPRT(0,"  ");
      if (j->cbkt)  DBGPRT (0," }"); else DBGPRT(0,"  ");


         if (j->cmpcnt)  DBGPRT(0,"  cmp %5x",j->fromaddr - j->cmpcnt);
   //   DBGPRT(0," e %5x", j->fromaddr + j->size);

      cnts[j->jtype] ++;
      DBGPRT(1,0);
    }

         DBGPRT (1," counts");
    for (ix = 0; ix < 6; ix++)
    {
           DBGPRT(1," %s, %d",jtxt[ix], cnts[ix]);
    }
    DBGPRT(1,0);
}


void DBG_jumps ()
{
  CHAIN *x;
  x = get_chain(CHJPF);

  DBGPRT(1,"------------ Jump list %s (Num = %d)", "from", x->num);
  DBGPRT(1,"From   To    Type  size   flags");

  DBG_jmp (CHJPF);

  DBGPRT(1,"------------ Jump list %s (Num = %d)", "to", x->num);
  DBGPRT(1,"From   To    Type  size   flags");

  DBG_jmp (CHJPT);
}

void DBG_sigs(uint ch)
{
 uint i, ix;
 SIG *y;
 CHAIN *x;

 x = get_chain(ch);

DBGPRT(1,0);
 DBGPRT(1,"-----------Signatures---------[%d]--", x->num);
 DBGPRT(1,"d ofst  end   HN SN name         pars" );

 for (ix = 0; ix < x->num; ix++)
   {
     y = (SIG*) x->ptrs[ix];
     DBGPRT(0,"%5x %5x, %2d, %2d, %-10s  ", y->start, y->end, y->hix, y->patno, get_signame(y->patno));

     for (i = 0; i < NSGV; i++)
     {
        if (i == 16)  DBGPRT(0,"\n                                 ");
        DBGPRT(0,"%7x," , y->vx[i]);
     }
     DBGPRT(2,0);
    }
 DBGPRT(1,0);
}

extern EMULOG emuargs[];

void DBG_emuargs(void)
{
uint i;
EMULOG *e;

DBGPRT(1,"EMUARGS");
   for (i = 0; i < EMUGSZ; i++)
             {
               e = emuargs + i;
               if (e->cmd)
                 {
                   DBGPRT(1,"%x (%x)", e->cmd->start, e->sblk->start);
                }
             }

DBGPRT(1,0);
        }



void DBG_rst(void)
{
    CHAIN* x;
    uint ix;

    RST* r;
// void *fid;
 x = get_chain(CHRGSTA);

DBGPRT(1,0);
 DBGPRT(1,"----------- RST ---------[%d]--", x->num);
// DBGPRT(1,"bstart  reg   popped modified  jump  paddr");


 for (ix = 0; ix < x->num; ix++)
   {
     r = (RST*) x->ptrs[ix];

/*
   uint enc     : 4;       // encoded type
   uint fend    : 5;       // size
   uint popped  : 1;       // popped reg
   uint pushed  : 1;       // to ignore push-pop saves

   uint arg     : 1;       // this is an argument register
   uint tfr     : 1;       // this is a  transfer reg (A9L 77be) use sig ??
   uint inc     : 1;       // has been incremented   // why?            for emuflag ??
                            //  uint inr     : 1;       // indirect (address)            why ??
                            //  uint used    : 1;       // when used as arg (for size)

   uint reg     : 10;      // register master key
 //  uint popreg    : 10;    // source pop register (i.e. the popped one) (why?)
   uint data    : 20;      // dreg for encode or data
*/

     DBGPRT(0,"R%3x %5x, %5x, P%x A%d SZ %x PTR %d", r->reg, r->argofst, r->cofst, r->popped, r->arg, r->fend, r->ref);

  //   if (p->popblk)  DBGPRT(0, "%x", p->popblk->start); else DBGPRT(0, "0!!");     //, p->sblk->1,0);

    //    fid = p;

 //  while ((p = get_pop(fid)))
 //    {
 //      DBGPRT(0,"  R%2x P%d M%d J%d  ",p->popreg, p->popped, p->modified, p->jump);
  //     if (p->popblk)  DBGPRT(0, "%x", p->popblk->start);      //, p->sblk->1,0);

  //   fid = p;
  //  }



     DBGPRT(1,0);
    }
 DBGPRT(2,0);
}








/*
void DBG_pop(void)
{
    CHAIN* x;
    uint ix;

    POP* p;
// void *fid;
 x = get_chain(CHPOP);

DBGPRT(1,0);
 DBGPRT(1,"----------- POPS ---------[%d]--", x->num);
 DBGPRT(1,"bstart  reg   popped modified  jump  paddr");


 for (ix = 0; ix < x->num; ix++)
   {
     p = (POP*) x->ptrs[ix];


     DBGPRT(0,"%5x %5x, R%2x P%d M%d J%d  ", p->blkstart, p->ofst, p->popreg, p->popped, p->modified, p->jump);

     if (p->popblk)  DBGPRT(0, "%x", p->popblk->start); else DBGPRT(0, "0!!");     //, p->sblk->1,0);

    //    fid = p;

 //  while ((p = get_pop(fid)))
 //    {
 //      DBGPRT(0,"  R%2x P%d M%d J%d  ",p->popreg, p->popped, p->modified, p->jump);
  //     if (p->popblk)  DBGPRT(0, "%x", p->popblk->start);      //, p->sblk->1,0);

  //   fid = p;
  //  }



     DBGPRT(1,0);
    }
 DBGPRT(2,0);
}
*/

/*
void DBG_arg(void)
{
   CHAIN* x;
    uint ix;

    ARG* p;
// void *fid;
 x = get_chain(CHARG);

DBGPRT(1,0);
 DBGPRT(1,"----------- ARGS ---------[%d]--", x->num);
 DBGPRT(1,"addr  reg   sz enc  dat");

 //  void *fid;           // arg->start for first, then arg pointers (like adt)

   uint reg : 10;         // register

   uint fend    : 5;       //size (updated with scan ?)

   uint enc     : 4;       // encoded type
   uint data    : 20;      // dreg for encode or data

//but also need a ofst for reapeated calls with same registers (3654)

   //from blk ??

//others ??  /


 for (ix = 0; ix < x->num; ix++)
   {
     p = (ARG*) x->ptrs[ix];


     DBGPRT(0,"%5lx R%5x, %2x E%d B%d ", p->fid, p->reg, p->fend, p->enc, p->data);

  //   if (p->popblk)  DBGPRT(0, "%x", p->popblk->start); else DBGPRT(0, "0!!");     //, p->sblk->1,0);

    //    fid = p;

 //  while ((p = get_pop(fid)))
 //    {
 //      DBGPRT(0,"  R%2x P%d M%d J%d  ",p->popreg, p->popped, p->modified, p->jump);
  //     if (p->popblk)  DBGPRT(0, "%x", p->popblk->start);      //, p->sblk->1,0);

  //   fid = p;
  //  }



     DBGPRT(1,0);
    }
 DBGPRT(2,0);
}

*/






/*
void DBG_banks(void)
 {
  uint ix;
  BANK *b;

// bank no , file offset,  (opt) pcstart, (opt)  end file offset, (opt) fillstart


  DBGPRT(1,0);
  DBGPRT(1,"-------DEBUG BANKS---------");
  DBGPRT(1,"bank filstart, filend, maxpc,  ## minpc, bkend, maxpc, bok,cmd,bkmask, cbnk, fbuf, opbt" );

 // DBGPRT(1,"ibuf %x, opbt %x",ibuf, opbit);
  DBGPRT(1,"opbt %x",opbit);

  for (ix = 0; ix < BMAX; ix++)
    {
      b = bkmap + ix;
      if (b->bok)
       {
         DBGPRT(0,"%2d, %5x %5x %5x", ix, b->filstrt, b->filend, b->maxpc);
         DBGPRT(0," ## %5x %5x", b->minpc, b->maxbk);
         DBGPRT(0,"  %x %x %x %x  %5x", b->bok, b->cmd,b->bkmask, b->cbnk, b->fbuf);
         DBGPRT(0," %5x", b->opbt-opbit );
         DBGPRT(1,0);}
       }
  DBGPRT(1,0);
  DBGPRT(1," Other non bok ---");

  for (ix = 0; ix < BMAX; ix++)
    {
      b = bkmap + ix;
      if (b->fbuf && !b->bok)
        {
         DBGPRT(0,"[ %2d, %5x %5x %5x", ix, b->filstrt, b->filend, b->maxpc);
         DBGPRT(0," ## %5x %5x", b->minpc, b->maxbk);
          DBGPRT(0,"  %x %x %x %x  %5x", b->bok, b->cmd,b->bkmask, b->cbnk, b->fbuf);
         DBGPRT(1,"]");
        }
     }

 DBGPRT(1,0);
}

*/

void DBG_mx(void *fid, uint ix, uint lev)
{
    // ix is param number.....

// fid ends in 0 for main terms 1,2,3 for param subterms
// ix is always zero ??

  uint i;
  MATHX *a;
  MNUM *n;
  CSTR *c;

  while ((a = get_mterm(fid,ix)))
   {
    fid = a;
    ix = 0;
    for (i = 0; i < lev; i++)  DBGPRT(0,"    ");      //indent
    // DBGPRT (0, "%x %x  (%d)", a, a->fid, a->subf);
    DBGPRT (0, "[%lx] -> %lx (%d)  ", a, a->fid,a->pix);
          if (a->mfirst) DBGPRT(0, "(mf) ");
     if (a->lfirst) DBGPRT(0, "(lf) ");


  //        if (a->noname) DBGPRT(0, "(n) ");
 //    if (a->subt) DBGPRT(0, "(s) ");
      DBGPRT(0,"ctype %d ",a->calctype);
     c = get_cstr(MATHSTR,a->func);

  //   DBGPRT(0, "func= %s ", get_cstr(MATHSTR,a->func);

    if (c) DBGPRT(0, "func= %s ", c->string); else DBGPRT(0, "!func= %d ", a->func);

     DBGPRT (0, "N%d ", a->npars);

     for (i = 0; i < 3; i++)
      {
       n = a->fpar + i;

       DBGPRT(0, "P%d[%d] ",i, n->dptype);

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

    for (i = 0; i < 3; i++)
      {
       n = a->fpar + i;

       if (n->dptype == DP_SUB)
        DBG_mx(fid,i+1,lev+1);   //sub instead of par

      }

     a->prt = 1;    // mark printed


      }

}






void DBG_math(void)

{
  MATHN *x;
  MATHX *a;
  uint ix, chks;
  CHAIN *ch;


  ch = get_chain(CHMATHN);

  DBGPRT(1,0);
  DBGPRT(2,"-----------Math Calcs---------[%d]--", ch->num);


 for (ix = 0; ix < ch->num; ix ++)
 {
    x = (MATHN *) ch->ptrs[ix];
    if (x->nsize) DBGPRT(0, x->mathname); else DBGPRT(0," <0> ");
  //  DBGPRT (0, " [%lx] %lx ", (ulong) x, (ulong) x->fid);
   DBGPRT (0, " [%lx] ", (ulong) x);
    chks = 0;
    chk_mx(x,0, &chks);
    DBGPRT(0, "cks %u ", chks);        //here somewhere....

    if (x->suffix) DBGPRT(0, "%s" ,x->suffix);

    DBGPRT (1, "\nTerms ----  fid, add, func, fval|ref");
    DBG_mx(x,0,0);         //name fk in first math term
    DBGPRT(1,0);

 }



DBGPRT(2,0);

  ch = get_chain(CHMATHX);

  DBGPRT (2, "ORPHANS / NONAMES ----  x FID  subf func fval|ref");

for (ix = 0; ix < ch->num; ix ++)
 {
    a = (MATHX *) ch->ptrs[ix];
     if (!a->prt)
       {
        chks = 0;
        chk_mx(a->fid,0, &chks);
        DBGPRT(0, "cks %d ", chks);        //here somewhere....
        DBG_mx(a->fid,0,0);
       }
  }

DBGPRT (2,0);
/*
 DBGPRT (2, "ORDER (temp)----  x FID ");
for (ix = 0; ix < ch->num; ix ++)
 {
    a = (MATHX *) ch->ptrs[ix];
     DBG_mx(a->fid,0,0);
  }


DBGPRT (3,0);
*/
}

void DBG_links (void)
{

  FKL *l;
  uint ix;

  CHAIN *x;

  x = get_chain(CHLINK);
  DBGPRT(2,0);
  DBGPRT(1,"# links num = %d", x->num);
  DBGPRT(2, "chsce, scekey, chdst, dstkey, type, add");

//  DBGPRT(1, "forwards");

  for (ix = 0; ix < x->num; ix++)
      {
        l = (FKL*) x->ptrs[ix];

// now for multiple queues

   //     DBGPRT(0, "%lx(%2d) -> %lx(%2d)", l->keysce,l->chsce, l->keydst, l->chdst);
        DBGPRT(0, "%s %lx -> %s %lx %d", chntxt[l->chsce], l->keysce, chntxt[l->chdst],l->keydst, l->type);

  //  DBGPRT(0, " %s ", chntxt[l->chsce]);

 //   if (l->chsce == CHDTK)     DBGPRT(0, " %x", ((TRK*)l->keysce)->ofst);
  //  if (l->chsce == CHDTD)     DBGPRT(0, " %x", ((DTD*)l->keysce)->stdat);
    if (l->chsce == CHSCAN)    DBGPRT(0, " %x", ((SBK*)l->keysce)->start);
    if (l->chsce == CHSIG)     DBGPRT(0, " %x", ((SIG*)l->keysce)->start);
    if (l->chsce == CHADNL)    DBGPRT(0, " %x", ((ADT*)l->keysce)->cnt);
    if (l->chsce == CHMATHN)    DBGPRT(0, " 0");       //, ((MATHX*)l->keysce)->cnt);


  //  DBGPRT(0, " %s ",  chntxt[l->chdst]);
 //   if (l->chdst == CHDTK)     DBGPRT(0, " %x", ((TRK*)l->keydst)->ofst);
 //   if (l->chdst == CHDTD)     DBGPRT(0, " %x", ((DTD*)l->keydst)->stdat);
    if (l->chdst == CHSCAN)    DBGPRT(0, " %x", ((SBK*)l->keydst)->start);
    if (l->chdst == CHSIG)     DBGPRT(0, " %x", ((SIG*)l->keydst)->start);
    if (l->chdst == CHADNL)    DBGPRT(0, " %x", ((ADT*)l->keydst)->cnt);
    if (l->chdst == CHMATHN)   DBGPRT(0, " 0");      //, ((MATHX*)l->keydst)->cnt);
     DBGPRT(1,0);

      }


       DBGPRT(2,0);
    }



void DBG_data()
 {

   DBGPRT(1,"max alloc = %d (%dK)", DBGmaxalloc, DBGmaxalloc/1024);
   DBGPRT(1,"cur alloc = %d", DBGcuralloc);
   DBGPRT(1,"mcalls    = %d", DBGmcalls);
   DBGPRT(1,"mrels     = %d", DBGmrels);
   DBGPRT(2,0);
   DBGPRT(1," -- DEBUG INFO --");

    DBG_jumps();

    DBG_scans();
    list_banks(DBGFILE);
  //  DBG_rgchain();
    DBG_opc();

    list_rbt (DBGFILE);
  //  list_subs(DBGFILE);          need DBG subs !
    list_syms(DBGFILE);
    DBG_cmds(DBGFILE);
    DBG_dtd();
    DBG_adnls();
    DBG_rst();
  //  DBG_spf();
    DBG_sigs(CHSIG);
    DBG_math();
    DBG_links();

 }


#endif

