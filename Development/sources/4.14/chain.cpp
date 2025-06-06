
#include  "chain.h"               // this calls shared.h



void *shuffle[16];                // for chain reorganises (and deletes), a block of void pointers



/**********************************************************************************
* CHAIN declarations, used ordered storage with for binary chop searches
*
* CHAIN structures use common find and insert mechanisms with defined subroutines for
* each chain type, Each type has its own structure associated with it.
* searched with 'binary chop' type algorithm.
* CHAIN params = num entries, num ptrs, allocsize, entrysize, maxptrs, num mallocs, ptr array,
* subroutines - free, compare, equals, (debug)
***********************************************************************************/

// Free block

void fsym(void *); void fsub(void *);
void fcmd(void *); void fblk(void *);

// compare, for binary chop searches

int cpjmp  (CHAIN *,uint, void *);   int cpbase (CHAIN *,uint, void *);
int cpcmd  (CHAIN *,uint, void *);   int cpscan (CHAIN *,uint, void *);
int cpsub  (CHAIN *,uint, void *);   int cpsym  (CHAIN *,uint, void *);
int cpsig  (CHAIN *,uint, void *);   int cpsscn (CHAIN *,uint, void *);
int cprgs  (CHAIN *,uint, void *);   int cpadt  (CHAIN *,uint, void *);
int cpspf  (CHAIN *,uint, void *);   int cppsw  (CHAIN *,uint, void *);
int cpdtk  (CHAIN *,uint, void *);   int cpdtka (CHAIN *,uint, void *);
 int cpbnk (CHAIN *,uint, void *);   int cpopcd (CHAIN *,uint, void *);

// chains for holding data

CHAIN chjpf   = {0,0,200,sizeof(JMP),   0,0,0,0,0, 0    ,cpjmp  };      // jump, indexed by 'from'
CHAIN chjpt   = {0,0,200,0          ,   0,0,0,0,0, 0    ,cpjmp  };      // jump, indexed by 'to' [REMOTE]

CHAIN chsym   = {0,0,200,sizeof(SYM),   0,0,0,0,0, fsym ,cpsym  };      // symbols

CHAIN chbase  = {0,0,200,sizeof(RBT),   0,0,0,0,0, 0    ,cpbase };      // rbases
CHAIN chsig   = {0,0, 20,sizeof(SIG),   0,0,0,0,0, 0    ,cpsig  };      // signatures

CHAIN chcmd   = {0,0,200,sizeof(LBK),  0,0,0,0,0, fcmd ,cpcmd  };       // main code and data commands
CHAIN chaux   = {0,0, 50,sizeof(LBK),  0,0,0,0,0, fcmd ,cpcmd  };       // auxilliary commands (arg, xcode, ...)

CHAIN chscan  = {0,0,200,sizeof(SBK),   0,0,0,0,0, 0    ,cpscan };      // code scans
CHAIN chemul  = {0,0, 20,sizeof(SBK),   0,0,0,0,0, 0    ,cpscan };      // code emulations
CHAIN chsbcn  = {0,0, 20,0          ,   0,0,0,0,0, 0    ,cpsscn };      // subroutine scans - [REMOTE]

CHAIN chsgap  = {0,0, 20,0          ,   0,0,0,0,0, 0    ,cpsscn };      // gap scans - [REMOTE]

CHAIN chsubr  = {0,0, 30,sizeof(SUB),   0,0,0,0,0, fsub ,cpsub  };      // subroutines

CHAIN chadnl  = {0,0, 50,sizeof(ADT),   0,0,0,0,0, 0    ,cpadt  };      // additional data, linked by FK

CHAIN chrgst  = {0,0, 20,sizeof(RST),   0,0,0,0,0, 0    ,cprgs  };      // register statuses (for arg calcs)

CHAIN chspf   = {0,0, 10,sizeof(SPF),   0,0,0,0,0, 0    ,cpspf  };      // special funcs (subroutines, tablu, funclu etc.)

CHAIN chpsw   = {0,0, 20,sizeof(PSW),   0,0,0,0,0, 0    ,cppsw  };      // psw setters

CHAIN chdtk   = {0,0,500,sizeof(TRK),   0,0,0,0,0, 0    ,cpdtk  };      // data opcode tracking.  index by ofst
CHAIN chdtkr  = {0,0,500,0          ,   0,0,0,0,0, 0    ,cpdtk  };      // data opcode tracking.  index by register


CHAIN chdtko  = {0,0,200,sizeof(DTD),   0,0,0,0,0, 0    ,cpdtka  };      // data address tracking.  index by fk (ofst)
CHAIN chdtkd  = {0,0,500,0           ,  0,0,0,0,0, 0    ,cpdtka  };     // data address tracking.  index by start [REMOTE]

//opcodes ??   decoded for all data ???



CHAIN chbkf   = {0,0, 10,sizeof(BNKF),  0,0,0,0,0,  0     ,cpbnk};      // bank finds



CHAIN chopcd  = {0,0,200,sizeof(TOPC),   0,0,0,0,0, 0    ,cpopcd  };      // opcodes index by ofst.

CHAIN mblk    = {0,0, 50,sizeof(MBL),   0,0,0,0,0, fblk ,0     };      // malloc tracking (for cleanup) ** must be last **


// CHAIN Pointer array for neater admin and external routines.

CHAIN *chindex[] = { &chjpf, &chjpt , &chsym,  &chbase, &chsig,
                     &chcmd, &chaux , &chscan, &chemul, &chsbcn, &chsgap, &chsubr , &chadnl,   &chrgst,
                     &chspf,  &chpsw,  &chdtk, &chdtkr, &chdtko, &chdtkd, &chbkf, &chopcd, &mblk};

#ifdef XDBGX
// names for chains for debug prtouts, and debug block printers...

  const char *chntxt[] = {"jump from", "jump to", "symbol", "rbase", "sign", "cmd", "aux",
                            "scan", "emulscan", "subscan", "gapscan","subroutines", "adnl data",   "regstat", //"adnl ans","adnl cmd",
                            "spf" , "psw"     , "dtrack" , "dtrack r", "dtrackd ofst", "dtrackd start"     , "bank find","opcodes", "monblk"};

#endif



CHAIN *get_chain(uint chn)
{
    if (chn >= NC(chindex) ) return NULL;
    return chindex[chn];
}


   uint adds [10];
   uint szes [32];







void mfree(void *addr, size_t size)
 {
     // free malloc with tracking
  if (!addr) return;     // safety

  #ifdef XDBGX
   DBGcuralloc -= size;    // keep track of mallocs
   DBGmrels++;
  #endif

  free (addr);
 }


void* mem(void *ptr, size_t osize, size_t nsize)
{  // malloc with local tracking
  void *ans;
  if (nsize < 1) wnprt(1,"Invalid malloc size");

  #ifdef XDBGX
  if (ptr)  DBGcuralloc -= osize;
  DBGmcalls++;
  DBGcuralloc += nsize;
  if (DBGcuralloc > DBGmaxalloc) DBGmaxalloc = DBGcuralloc;
  #endif

  ans = realloc(ptr, nsize);

  if (!ans) wnprt(1,"Run out of malloc space");
  return ans;
}


void* cmem(void *ptr, size_t osize, size_t nsize)
{  // malloc with clear
  void *ans;
  ans = mem(ptr,osize,nsize);
  memset(ans,0,nsize);
  return ans;
}










void adpch(CHAIN *);    // declared for logblk

void logblk(void *blk, size_t size)
{
 MBL *b;

 if (!blk) return;

 if ((mblk.pnum - mblk.num) < 2) adpch(&mblk);    // need more pointers

 b = (MBL*) mblk.ptrs[mblk.num];  // no point in using chmem
 b->blk = blk;
 b->size = size;
 mblk.num ++;
 if (mblk.num > mblk.DBGmax) mblk.DBGmax = mblk.num;      // for debug really
}

 //----------------------------------------------------------------
 // add pointers for chains. malloced entries
 // pointer block is realloced, so is continuous...
 // but data blocks are NOT....
 //----------------------------------------------------------------

void adpch(CHAIN *x)
 {   // add pointers for more chain pointers (malloced entries)
     // pointer block is realloced, so is always contiguous.
     // data blocks are not.

    int old, nw, msz,i;
    char *z, *n;

    // grow pointer array - always as single block for arrays
    old = x->pnum;
    x->pnum += x->asize;
    x->DBGallocs++;

    nw = x->pnum;            // new size
    x->ptrs = (void**) mem(x->ptrs, old*sizeof(void*), nw*sizeof(void*));

    // allocate new entries if not a remote chain, and save in mbl chain

    n = NULL;
    msz = 0;

    if (x->esize)
     {
      msz = x->esize*x->asize;           // size of new data block to go with ptrs
      n =  (char *) mem(0,0,msz);
      if (!n) wnprt(0,"FATAL - MALLOC ERROR ");
     }

    z = n;
    for (i = old;  i < nw; i++)
     {
      x->ptrs[i] = z;
      z += x->esize;
     }

   logblk(n, msz);

 }


void* chmem(CHAIN *x)
 {
   // get a free entry of the chain for search or insert.
   // this is from x->num onwards (next free entry) + ix for nested use

  void *ptr;

  if (!x->esize) return NULL;               // can't have memory on a remote chain...

  if (x->pnum < (x->num+2)) adpch(x);       // need more pointers (unsigned check)

  ptr = x->ptrs[x->num];                    // use next free block (at end of list)
  memset(ptr,0, x->esize);                  // and clear it
  return ptr;
 }


void chinsert(CHAIN *x, int ix, void *blk)
{
  // insert new pointer (= blk) at index ix
  // relies on main pointer block being contiguous and blk being x->num;

 if (!blk) return ;

 if ((x->pnum - x->num) < 2) adpch(x);    // need more pointers

 // move entries up one from ix to make a space.
 memmove(x->ptrs+ix+1,x->ptrs+ix, sizeof(void*) * (x->num-ix));  // fastest for contiguous

 x->ptrs[ix] = blk;                              // insert new block (x->num) at ix
 x->num++;

 x->lastix = x->num;                              // invalidate any get_next
 #ifdef  XDBGX
 if (x->num > x->DBGmax) x->DBGmax = x->num;     // for debug
 #endif


}


void chdelete(CHAIN *x, uint ix, int num)
{
 // drop num block(s) from list from ix, shuffle down rest of
 // list and restore ptr(s) after x->num.
 // relies on main pointer block being contiguous
 // shuffle array to hold pointers (max 16)
 // extra +1 in shuffle is for x->num block (used by chmem)

 uint i;

 if (ix >= x->num) return;

 if (x->cfree)
   {     //free any additional items
    for (i = ix;  i < ix+num; i++) x->cfree(x->ptrs[i]);
   }

 // save pointers about to be 'deleted' (released) in temp array
 memcpy(shuffle, x->ptrs+ix, sizeof(void*) * num);

 x->num -= num;   //drop no of defined entries

 // move entries down by 'num' from ix+num to overwrite released entries, with extra chmem(0) pointer.
 memmove(x->ptrs+ix, x->ptrs+ix+num, sizeof(void*) * (x->num-ix+1));

 // put released pointers back after new x->num position so they become unused
 memcpy(x->ptrs + x->num+1, shuffle, sizeof(void*) * num);
 x->lastix = x->num;                              // invalidate any get_next
}




uint bfindix(CHAIN *x, void *blk)         //, int (*cmp) (CHAIN *x, uint ix, void *blk))
{
  // generic binary chop.
  // send back an INDEX value to allow access to adjacent blocks.
  // use a compare subr as supplied or default for chain struct.
  // Candidate block is void* and cast to reqd type inside cmp (compare subroutine).
  // answer (min) is always min <= blk, or blk is always >= min (= answer)
  // and min+1 is > blk.   Answer is therefore INSERT point for new block, or FOUND.

  uint tst, min, max;

//  if (!cmp) cmp = x->comp;   // use std chain compare subr if not given

  // NB. this loop could work for subsearches too.....

  min = 0;
  max = x->num;

  while (min < max)
    {
      tst = min + ((max - min) / 2);               // safest way to do midpoint of (min, max);
      if(x->comp(x,tst,blk) < 0) min = tst + 1;        // blk > tst, so new minimum is (tst+1)
      else  max = tst;                             // blk <= tst, so new maximum is tst
    }

  x->lastix = min;                                 // keep last index (even if x->num)
  return min;
}











void chupdate(CHAIN *x, uint ix)
{
  // block at index 'ix' has been updated - rechain in correct place
  // may need extra check here if ix = x->num

//  CHAIN *x;
  void *blk;
  uint newix;

 // x = chindex[ch];
  blk = x->ptrs[ix];                                                // save blk pointer

  newix = bfindix(x,blk);                                           // find new insert point

  if (newix == ix) return;                                          // nothing to do

  if (newix > ix)
   {   // move ptrs down one, over ix. Reinsert block at newix.
      newix--;    // don't move newix entry.

     if (newix >= x->num) newix = x->num-1;                           // safety, should never happen ??
     memmove(x->ptrs+ix, x->ptrs+ix+1, sizeof(void*) * (newix-ix));    // move ptrs in gap down
     x->ptrs[newix] = blk;
   }

  if (newix < ix)
   {
      // move ptrs up one, over ix, reinsert block at newix.
      memmove(x->ptrs+newix+1, x->ptrs+newix, sizeof(void*) * (ix-newix));    // move ptrs in gap UP
      x->ptrs[newix] = blk;
   }

 // if (newix < x->lowins) x->lowins = newix;             // reset lowest insert index (for loops)

  x->lastix = x->num;                                   // invalidate any get_next

}



void fblk(void *x)
{           // malloced blocks tracker (free'd last)
  MBL *k;
  k = (MBL *) x;
  mfree(k->blk, k->size);
}

void clearchain(CHAIN *x)
{
  // clear pointer list contents, but not pointers themselves. Free any attached structures
  //  NB.  count DOWN so that for mblks first entry (which points to itself) is released last.

 int i;

 for (i = x->num-1; i >=0; i--)
    {
     if (x->cfree)  x->cfree(x->ptrs[i]);
    }

 x->num = 0;
}


void freeptrs(CHAIN *x)
{
    // free the master pointer block of chain x (after clearchain does entries)
 if (x->pnum)
   {
    mfree(x->ptrs, x->pnum * sizeof(void*));
    x->ptrs = NULL;
    x->pnum = 0;
   }
}

void freechain(int ix)
{
  CHAIN *x;

  x = chindex[ix];

  #ifdef XDBGX
    int size;
    size =  x->pnum * sizeof(void *);     // number of void* pointers
    size += x->pnum * x->esize;           // plus structs pointed to
    DBGPRT(1,"max=%5d tot=%5d allocs=%2d esize=%3d size=%6d   %s", x->DBGmax, x->pnum, x->DBGallocs, x->esize, size, chntxt[ix]);
  #endif

  clearchain(x);
  freeptrs(x);
}

void free_structs ()
{                        // complete tidyup for closing or restart
 uint i;

 for (i = 0; i < NC(chindex); i++) freechain(i);           // this includes mblock

   #ifdef XDBGX
    DBGPRT(1,"max alloc = %d (%dK)", DBGmaxalloc, DBGmaxalloc/1024);
    DBGPRT(1,"cur alloc = %d", DBGcuralloc);
    DBGPRT(1,"mcalls = %d", DBGmcalls);
    DBGPRT(1,"mrels  = %d", DBGmrels);
    DBGPRT(1,"Max recurse = %d", DBGrecurse);
    DBGPRT(1,"Num Opcodes = %d", DBGnumops);
   #endif

   fflush (fldata.fl[1]);            // flush output files (safety)
   fflush (fldata.fl[2]);


}



void *vconv(uint addr)
{
   // allows any chain item to link either via a bin address (uint) or by
   // a void* pointer. Converts uint to void*

   // Convert a uint of 32 bits (e.g. address) into void* (64 bits) and avoid compiler warnings.
   // It doesn't matter how compiler aligns this,  it's consistent within this program.

  union
   {
    void *fid;
    uint address;
   } cv;

  cv.fid = 0;            // clear whole field
  cv.address = addr;     // 'OR' address (in effect)

  return cv.fid;
}





//---------------------------------------------------------------------------------------/

// CHAIN Compare  procs used by bfindix for binary chop chain searches
// Straight subtract works nicely for chain ordering, even for pointers
// < 0 less than, 0 equals, > 0 greater than



int cpbnk  (CHAIN *x, uint ix, void *newb)
{
   // temp banks from find_bank....

 BNKF *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (BNKF*) x->ptrs[ix];
 nw = (BNKF*) newb;

   ans = ch->filestart - nw->filestart;                 // file start address
   if (ans) return ans;
   ans = ch->pcstart - nw->pcstart;                     // program start

   return ans;
 }





int cpopcd  (CHAIN *x, uint ix, void *newb)
{
   // temp banks from find_bank....

 TOPC *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (TOPC*) x->ptrs[ix];
 nw = (TOPC*) newb;

   ans = ch->ofst - nw->ofst;                 // start address

   return ans;
 }





int cpjmp (CHAIN *x, uint ix, void *d)
{
  int ans;
  JMP *j, *t;

  if (ix >= x->num) return -1;

  j = (JMP*) x->ptrs[ix];
  t = (JMP*) d;

  // 'fromaddr' is unique, 'toaddr' is not.

  if (x == &chjpt)
   { // to chain
     // includes fromaddr (0 = first from)
     ans = j->toaddr - t->toaddr;
     if (ans) return ans;
     ans = j->fromaddr - t->fromaddr;
   }
  else
   {  // from chain.  fromaddr is unique
     ans = j->fromaddr - t->fromaddr;
   }

  return ans;
}

int cpsym (CHAIN *x, uint ix, void *newb)
{
 // symbols - multiple items in effect make the key

 int ans;
 SYM *ch, *nw;

  if (ix >= x->num) return -1;

  ch = (SYM*) x->ptrs[ix];
  nw = (SYM*) newb;

// byte address
 ans = ch->addr - nw->addr;
 if (ans) return ans;

// lowest bit first
 ans = ch->fstart - nw->fstart;
 if (ans) return ans;

//Highest bit second (larget field first, write first)

 ans = nw->fend - ch->fend;        //  reverse order, largest field first, write first
 if (ans) return ans;



//check range

 return nw->rstart  -  ch->rstart;    // by range start, in REVERSE order

}




int cpbase (CHAIN *x, uint ix, void *d)
{
 RBT *b, *t;
 int ans;

 if (ix >= x->num) return -1;
 b = (RBT*) x->ptrs[ix];
 t = (RBT*) d;

  ans = b->reg - t->reg;
  if (ans) return ans;

  return t->rstart - b->rstart;  // by start, in reverse order

//  return ans;
}


int cpsig (CHAIN *x, uint ix, void *newb)
{
  SIG *ch, *nw;
  int ans;

  if (ix >= x->num) return -1;

  ch = (SIG*) x->ptrs[ix];
  nw = (SIG*) newb;

  ans = ch->start - nw->start;

  return ans;

}


int cprgs (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

  RST  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (RST*) x->ptrs[ix];
 t = (RST*) d;

 ans = s->reg - t->reg;

 return ans;

}

int cpspf (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   //only ever one spf per subr, so unique

 SPF  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (SPF*) x->ptrs[ix];
 t = (SPF*) d;

 ans = (char *) s->fid - (char *) t->fid;

 return ans;

 //return (long) s->pkey - (long) t->pkey;

}

int cpadt (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

 ADT  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (ADT*) x->ptrs[ix];
 t = (ADT*) d;

// ans = (long) s->fid - (long) t->fid;

// ans =

// if (ans) return ans;

// ans = s->seq - t->seq;

ans = (char*) s->fid - (char*) t->fid;

 return ans;

}


int cpscan (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

 SBK  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (SBK*) x->ptrs[ix];
 t = (SBK*) d;

 ans = s->start - t->start;
 if (ans) return ans;

 // and subroutine for scans

 if (t->substart) ans = s->substart - t->substart;
// if (ans) return ans;

return ans;
}


int cpsscn (CHAIN *x, uint ix, void *d)
{
   // subscan  - d (t) is candidate blk

   // use subroutine address to ensure
   // all branches are scanned

 SBK  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (SBK*) x->ptrs[ix];
 t = (SBK*) d;

 ans = s->substart - t->substart;

 return ans;

}

int cppsw (CHAIN *x, uint ix, void *d)
{
   // subscan  - d (t) is candidate blk

 PSW  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (PSW*) x->ptrs[ix];
 t = (PSW*) d;

 ans = s->jstart - t->jstart;

 return ans;

}

int cpdtk (CHAIN *x, uint ix, void *d)
{
 TRK *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (TRK*) x->ptrs[ix];
 t = (TRK*) d;


//if (x == &chdtk)
 {
  //  ofst for main entries

   ans = s->ofst - t->ofst;
   return ans;
 }

/*
if ( x == &chdtkr)
  {
    ans = s->?
*/

// return ans;
}









/* if (x == &chdtkd)
   { // index by data address - not unique - jmp example.........

   if (t->fromaddr) ans = j->fromaddr - t->fromaddr;
     else
       {    // get to minimum matching 'to' if no 'from' specified
        if (ix > 0)
          {
           j = (JMP*) x->ptrs[ix-1];
           if (j->toaddr == t->toaddr) ans = 1; //go back
          }
       }






 ans = s->start - t->start;
 if (ans) return ans;

 if (t->rreg)
 ans = s->rreg - t->rreg;
 if (ans) return ans;

 if (t->ofst)
ans = s->ofst - t->ofst;

 return ans;
}



 if (x == &chdtkr)
   { // index by register and offset, not unique ?

    ans = s->reg1 - t->reg1;
    if (ans) return ans;

//    ans = s->start - t->start;
//    if (ans) return ans;

    if (t->off)
    ans = s->off - t->off;
    if (ans) return ans;

    ans = s->ofst - t->ofst;


}

 return ans;
}
// */

int cpdtka (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

 DTD  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (DTD*) x->ptrs[ix];
 t = (DTD*) d;

 if (x == &chdtko)
  {       //key first
   ans = s->ofst - t->ofst;
   if (ans) return ans;

      //ofst first.
  ans = s->stdat - t->stdat;
 return ans;
 }

  ans = s->stdat - t->stdat;
  if (ans) return ans;

  ans = s->ofst - t->ofst;
 return ans;

}








int cpsub (CHAIN *x, uint ix, void *d)
{
 // add a check here for pushp subrs ?.
 SUB *s, *t;

 if (ix >= x->num) return -1;

 s = (SUB*) x->ptrs[ix];
 t = (SUB*) d;

 //if (s->psp && t->start == s->start+1)  return 0;     // pushp match

 return s->start - t->start;
}


int cpcmd (CHAIN *x, uint ix, void *d)
{
 // this compare checks END with start for address ranges
 // therefore [ix].end <= d->start.

 LBK *b, *t;
 int ans;

 if (ix >= x->num) return -1;
 b = (LBK*) x->ptrs[ix];
 t = (LBK*) d;

 ans = b->end - t->start;

// if (ans) return ans;

// reverse order for command (code at very front)
// ans = t->fcom - b->fcom;

 return ans;

}

/*
int eqsig (CHAIN *x, uint ix, void *d)
{
//cpsig with extra check for skips

// CHANGE THIS to use END addresses, like cmd, to allow for skips at front

 SIG *g, *t;
 int ans;

 if (ix >= x->num) return -1;
 g = (SIG*) x->ptrs[ix];
 t = (SIG*) d;

 ans = 1;

 // allow for skips at front.........
 if (g->start <= t->start && (g->start + g->skp) >= t->start) ans = 0;
 if (!ans && t->ptn) ans = g->ptn - t->ptn;
 return ans;

}
*/


int eqcmd (CHAIN *x, uint ix, void *d)
{
 // make sure candidate block falls within
 // indiexed block start and end.

 LBK *b, *t;

 if (ix >= x->num) return -1;

 b = (LBK*) x->ptrs[ix];
 t = (LBK*) d;

 if (t->start >= b->start && t->start <= b->end)
    {
     if (!t->fcom || t->fcom == b->fcom) return 0;
    }
 return 1;
}

uint mergecheck(LBK *t, LBK *newb)
  {
    //split out logic to make simpler...
    // return 1 if merge OK, 0 if not

     // can always merge code and xcode
     if (newb->fcom == C_CODE  && t->fcom == C_CODE)  return 1;
     if (newb->fcom == C_XCODE && t->fcom == C_XCODE) return 1;

     if (newb->usrcmd)  return 0;
     if (t->usrcmd)     return 0;
     if (newb->nomerge) return 0;
     if (t->nomerge)    return 0;

     if (dirs[t->fcom].merge && t->fcom == newb->fcom)   return 1;

     return 0;
  }





// also check here for adnl bl

int olchk(CHAIN *x, uint ix, LBK *newb)
 {

   /***** overlap and overwrite check if - commands can be merged/overwrite etc
   * b is 'candidate' (new) block , t is existing (test) block at ix
   * set answer as bit mask -
   * 1  new block spans  t  (t within newb)
   * 2  new block within t (t spans newb)
   * 4  front overlap or front adjacent
   * 8  rear  overlap or rear  adjacent


no overrides, just overlaps and fail if not same command ????



   * 10 t overrides newb (newb cannot be inserted)
   * 20 newb overrides t (can overwrite it)
   * 40 merge allowed (flag set and commands match)
   ************/

   LBK *t;
   int ans;

   if (ix > x->num) return 0;

   t = (LBK*) x->ptrs[ix];

   if (t == newb) return 0;                     // safety check

   ans = 0;   //clear all flags;

   // NB.  check for equal already done in inscmd

   if (newb->start <= t->start && newb->end >= t->end)  ans |= 1; // newb spans t
   else
   if (newb->start >= t->start && newb->end <= t->end)  ans |= 2; // newb is spanned by t
   else
    {      // overlap, front or back
     if (newb->start < t->start && newb->end   >= t->start && newb->end   <= t->end) ans |= 4;        // overlap front
     if (newb->end   > t->end   && newb->start >= t->start && newb->start <= t->end) ans |= 8;        // overlap rear
    }

 //  if (ans)
 //   {       // check overwrite
 //    if (!t->cmd    && dirs[newb->fcom].maxovr >= t->fcom) ans |= 0x20;  // newb can overwrite t
 //    if (!newb->cmd && dirs[t->fcom].maxovr > newb->fcom) ans |= 0x10;  // newb cannot overwrite t  - relevant for part overlaps
 //   }

// Gribble


   //only merge matching commands,  user and adt checks in inschk.

  // if (!t->usrcmd  && !newb->usrcmd && !t->nomerge && !newb->nomerge)       //  NO!! not if CODE !!
   //  {
   //if (dirs[t->fcom].merge && t->fcom == newb->fcom)       //    && !newb->usrcmd && !t->usrcmd && !t->size && ! newb->size)

   if (mergecheck(t,newb))
    {
     if (ans) ans |= 0x40;                            // overlap set already, merge allowed
     if (newb->end == t->start-1)  ans |= 0x44;       // adjacent to front of test block (merge+front)
     if (newb->start == t->end+1)  ans |= 0x48;       // adjacent to end of test block (merge+rear)
    }

// also check here for adnl blocks which would break any merges being allowed.


// do the dbg print HERE for both blocks

 #ifdef XDBGX
  if (ans) DBGPRTFGS(ans,newb,t);
 #endif

 return ans;
}


uint find_olap(CHAIN *x, uint ix, void *newb, uint up)     //upwards

{
//up 1 = up, 0 = down



 // std compare checks END address with start, for address ranges
 // therefore [ix].end >= d->start ALWAYS.
 // this is a special version for checking OVERLAPS.

 // If d.start is specified, find the MINIMUM ix where d.start
 // is overlapped or adjacent, where [ix].end > d.start.

 // If d.end is specified, find the MAXIMUM ix where d.end is
 // is overlapped or adjacent, where [ix].end > d.end.
 // for LBK structs (data and code).

  LBK *b, *t;
 // int ans;
 // uint max;

//  ans = -1;
  if (ix >= x->num) return x->num-1;

  b = (LBK*) x->ptrs[ix];
  t = (LBK*) newb;

// ans = b->start - t->start;      is std cmp

  if (up)
     {
      // get to maximum matching block, overlap or adjacent
      // t->start always <= b->start
      while (ix < x->num)
          {
           if (t->end <= (b->start-1)) break;
           ix++;
           b = (LBK*) x->ptrs[ix];
          }
      if (ix >= x->num) ix = x->num - 1;
     }

  if (!up)
     {     // down to minimum extra adjacent check (different logic to END check)
           //skip back one first so in front of insert
      if (ix == 0) return ix;
       ix--;
       while (ix < x->num)
        {            // handles wrap..............
          b = (LBK*) x->ptrs[ix];
          if (b->end < t->start-1) break;
          ix--;
        }
       //if (ix >= 0) ix = 0;
     }
 return ix;

}


int inschk(CHAIN *x, int ix, LBK *newb)
{

   /* check if insert is allowed, fix any overlaps later.
   *  newb is insert candidate. check range of possible overlaps.
   * ix is where insert would be (if going recursive....)
   *  probably better to delete blocks because of ADT issues....but causes issue with A9L
   * answer is bit mask from olchck
   * 1  new block spans  t  (t within newb) must check for overlaps........
   * 2  new block within t  (t spans newb)
   * 4  front overlap or front adjacent
   * 8  rear  overlap or rear  adjacent
   * 10 t overrides newb (newb cannot be inserted) for part overlaps
   * 20 newb overrides t (can overwrite t)
   * 40 merge allowed (flag set and commands match)
   */

  LBK *t;
  uint chkf, min, max;
  int ans;

  // OK, find min and max overlap

  if (x->num == 0) return 1;        //always ok to insert

  if (newb->fcom & C_NOMERGE) return 1;

 // ix is where insert would be


  max = find_olap(x,ix,newb,1);     //upwards
  min = find_olap(x,ix,newb,0);     //downwards

  ans = 1;                            // set 'insert OK'

  while (min <= max && ans)
    {                                 // scan range of possible overlaps
     if (max >= x->num) break;
     if (min >= x->num) break;         // safety for deleted items - perhaps should restart............

     chkf = olchk(x, min, newb);      // check chained block for overlap

     if (chkf)
      {
       t = (LBK*) x->ptrs[min];

       if (newb->fcom > t->fcom)               // chkf & 0x20)   TEMP !!    need more here
        {                    // new block overrides chain block
         if (chkf & 1)
          { //new block spans the existing one (check cmd ?)
            #ifdef XDBGX
            DBGPBK(0,t,"delete1 (%d)", min);
            #endif
            chdelete(x, min, 1);
            //probably better to redo the whole thing ?
            min--;
            max--;                // match is now 1 shorter (but is it ?  min won't be less ??)
          }
         else
         if (chkf & 2)
             { // new block is contained within chain block, but overrides it
               // if front overlaps (4), move front of chain block
               // if end overlaps (8), move end of chain block
            //   NO !!!
        //           ans = 0;          // no insert
       //            return ans;

               if (chkf & 4) t->start = newb->end+1;
               else
               if (chkf & 8) t->end = newb->start-1;
                          // panic here
             }
          else
              { // overlap, can overwrite
               if (chkf & 4)            //F)
                 {    // front of new block overlaps chain block
                  t->end = newb->start-1;     // change end
                 }
              if (chkf & 8)                //R)
                 { // end of new block overlaps start of of chain block
                   t->start = newb->end+1;
                 }
               }
              // more cases ???








          }      // end 0x20
         else
         if (chkf & 0x10)                  //V)
          {
           // chained block overrides new block
             if (chkf & 4) newb->start = t->end+1;
               else
            if (chkf & 8)               //R)
               {
                 newb->end = t->start-1;
               }

           // check start and end ....

            if (chkf & 2 || newb->start < newb->end)
              {      // contains, (or now empty). Reject insert
               ans = 0;          // no insert
       //        return ans;
              }

         }
         else
         if (chkf & 2)                     //C)
           {
            ans = 0;          // no insert
  //          return ans;
           }
         else

       if (chkf & 0x40)         // TEST for span as well ? fewer errors, but....word over byte ??
        {
         // merge allowed, if multiple in this loop,
         // merge new block with chain then stop insert ? drop chain entry

      //    if (t->usrcmd  || newb->usrcmd) ans = 0;
     //     if (t->nomerge || newb->nomerge) ans = 0;

    //      if (ans)
               {
                 if (t->start < newb->start) newb->start = t->start;
                 if (t->end   > newb->end)   newb->end   = t->end;
                 #ifdef XDBGX
                   DBGPRT(0,"(%d) ", min);
                   DBGPBK(0,t,"delete2");
                 #endif
                 chdelete(x, min, 1);
                 min--;
                 max--;                // match is now 1 shorter
               }

        }
       else

// but if TWO merges (i.e. new block spans gap between two chains,
// MUST drop one......



// where is s ?
        {
          // default catch
          #ifdef XDBGX
          // can ignore issues for same command
        //  if (newb->fcom != t->fcom)              no !!
            {
              DBGPRT(0,"(%d) ",min);
              DBGPBK(0,newb, "DFLT Reject");
            }
           #endif
          ans = 0;           // temp stop insert
        }
         #ifdef XDBGX
        DBGPRT(1,0);
         #endif
      }          // end if chk set
     min++;
    }         // end loop

 return ans;                    // insert if not zero
}




LBK* inscmd(CHAIN *x, LBK *blk)
 {
   /* need extra checks the for address ranges (start->end)

    * at this call 'blk' is always x->ptrs[x->num] ....
    * do a simple check,   THEN do more complex merging as necessary

   this works but does not merge anything yet.  THis assumes you never get an END overlap ?
   probably true for current scan setup, but not safe.....

   eqcmd only checks that start falls between start&end of chained block....

   NB.   Probably don't care about overlaps right now, can do later, but must capture
   any extended END markers.

   so have to check for exact match, and non_allowed overlaps/overrides ?

    */

  int ans;
  uint ix;
  LBK *k;

//  ans = 1;

  ix = bfindix(x, blk);                 // where to insert (from cpcmd)

  k = (LBK*) x->ptrs[ix];

  ans = 1;                      //temp allow overlaps
  // check for exact match
  if (ix < x->num && k->start == blk->start && k->end == blk->end && k->fcom == blk->fcom)
    {
//      if (!k->sys)
 x->lasterr = E_DUPL;
 //else x->lasterr = 0;
     // ans = 0;
       return 0;       //no insert
    }

  ans = inschk(x,ix,blk);              // found - is insert allowed ?
//this may CHANGE IX !!!

  if (ans)
      {
       ix = bfindix(x, blk);          // find again in case stuff deleted
       chinsert(x, ix, blk);          // do insert at ix
       x->lasterr = 0;                // lasterr
      }
  else
     {
      blk = (LBK*) x->ptrs[ix];       // overlap. (duplicate above)
      x->lasterr =  E_OVLP;           // what about overlaps ??
     }
 return blk;

}

// match for signature blocks

SIG* add_sig(SIG *blk)
 {

  int ans;
  uint ix;
  SIG *s;

  //no chemem as done externally for data to be copied in.

  ix = bfindix(&chsig, blk);

  ans = -1;

  if (ix < chsig.num)
   {       // new sig overlaps ix (which is next block in chain)
    s = (SIG*) chsig.ptrs[ix];

    if (s->start == blk->start) ans = 0;       // duplicate
    if (ix > 0)
     {
       // if novlp, no overlaps allowed    // for sig handler of same type
      s = (SIG*) chsig.ptrs[ix-1];             // previous entry

      if (s->novlp &&            // s->hix == blk->hix &&
          s->start <= blk->start && s->end >= blk->start) ans = 0;
     }

    if (!ans)
     {
      chsig.lasterr = E_DUPL;
      return 0;
     }
   }

  chinsert(&chsig, ix, blk);      // do insert
  return blk;

}


LBK* add_cmd(uint start, uint end, uint com, uint from)
 {

   LBK *b, *x;
   uint tcom;

   chcmd.lasterr = E_INVA;           // set "invalid address"
   tcom = com & 0x1f;                // drop any extra flags

   if (end < start) end = start;             // safety

   //only code at 0x2000 in each bank
   if (nobank(start) == 0x2000 && tcom != C_CODE) return NULL;


   if  (tcom == C_DFLT || tcom == C_TEXT)
    {  // special for fill and text cmd
     if (start > maxadd (start)) return NULL;
     if (end   > maxadd (end))   return NULL;
    }
   else
    {
     if (!val_rom_addr(start)) return NULL;
     if (!val_rom_addr(end))   return NULL;
    }

   if (g_bank(start))
     {
      if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

     // can get requests for 0xffff with bank, which will break boundary....

      if (!bankeq(end, start))
       {
        chcmd.lasterr = E_BKNM;           // set "banks don't match"
        return NULL;
       }
     }


 //  chcmd.lasterr = 0;      //done in insert


   b = (LBK *) chmem(&chcmd);
   b->start = start;
   b->end = end;
 //  b->from = from;
   b->fcom  = tcom;             // max 31 as set
   if (com & C_USR)     b->usrcmd = 1;
   if (com & C_NOMERGE) b->nomerge = 1;
   if (PARGC)           b->cptl = 1;

// gribble NOMERGE


   x = inscmd(&chcmd,b);

 //  if (!chdata.lasterr) set_opdatar(start,end);       // mark data

// note that b may be WRONG here after merges etc.

   #ifdef XDBGX
   if (!chcmd.lasterr) DBGPBK (0,b,"add cmd");
   else   DBGPBK (0,b,"FAIL add cmd (%d)", chcmd.lasterr);
   if (from) DBGPRT(0, "from %x", from);
   DBGPRT(1,0);
   #endif

   return x;
}





TOPC *add_opc(uint start)
{
  TOPC *x;
  uint ix;
  int ans;

  chopcd.lasterr = E_INVA;
  if (!val_rom_addr(start))  return 0;

  x = (TOPC *) chmem(&chopcd);
  x->ofst = start;

  ix = bfindix(&chopcd, x);
  ans = chopcd.comp(&chopcd,ix, x);

  if (ans)
   {
    chinsert(&chopcd, ix, x);
    chopcd.lasterr = 0;
    return x;
   }

 //  #ifdef XDBGX
 //     if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail");
 //     DBGPRT(0," opcode %x", start);
 //     DBGPRT(1,0);
 //    #endif


//if (ans) return x;

chopcd.lasterr = E_DUPL;       //duplicate
return 0;
}

TOPC *get_opc(uint addr)
{
  TOPC *r;
  uint ix;

  r = (TOPC*) chmem(&chopcd);
  r->ofst = addr;

  ix = bfindix(&chopcd, r);
  if (chopcd.comp(&chopcd,ix,r)) return NULL;    // no match
  return (TOPC*) chopcd.ptrs[ix];
}




// general chain finder for all chains



PSW *add_psw(uint jstart, uint pswaddr)
{
  PSW *x;
  uint ix;
  int ans;

  chbase.lasterr = E_INVA;
  if (!val_rom_addr(jstart))  return 0;
  if (!val_rom_addr(pswaddr)) return 0;

  x = (PSW *) chmem(&chpsw);
  x->jstart = jstart;
  x->pswop  = pswaddr;

  ix = bfindix(&chpsw, x);
  ans = chpsw.comp(&chpsw,ix, x);

  if (ans)
   {
    chinsert(&chpsw, ix, x);
    chpsw.lasterr = 0;
   }

   #ifdef XDBGX
      if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail");
      DBGPRT(0," pswset %x = %x", jstart, pswaddr);
      DBGPRT(1,0);
     #endif


if (ans) return x;

chpsw.lasterr = E_DUPL;       //duplicate
return 0;
}

PSW *get_psw(uint addr)
{
  PSW *r;
  uint ix;

  r = (PSW*) chmem(&chpsw);
  r->jstart = addr;

  ix = bfindix(&chpsw, r);
  if (chpsw.comp(&chpsw,ix,r)) return NULL;    // no match
  return (PSW*) chpsw.ptrs[ix];
}


RBT *add_rbase(uint reg, uint addr, uint rstart, uint rend)
{
  RBT *x, *z;     // val includes bank
  uint ix;
  int ans;

  chbase.lasterr = E_INVA;                  // set "invalid addr"

  if (reg  & 1)              return 0;          // can't have odd registers (word addressed)
  if (!val_general_reg(reg)) return 0;          // not any special regs
  if (addr > maxadd(addr))   return 0;          // anywhere in address range

  if (rstart && !val_rom_addr(rstart)) return 0;  // range must be within ROM

  x = (RBT *) chmem(&chbase);
  x->reg    = reg;
  x->val    = addr;
  x->rstart = rstart;
  x->rend   = rend;

 // if (fend & 0x60)  s->pbkt   = 1;       // sign or write set
 // if (fend & C_USR) s->usrcmd = 1;       // by user command
 // if (reg & C_SYS) s->sys    = 1;       // system generated




  ix = bfindix(&chbase, x);
  ans = chbase.comp(&chbase,ix, x);

// if ans=0 duplicate


  // check for overlap here

  z = (RBT *) chbase.ptrs[ix];         // chain block found, nearest below

  // ranges - can be within existing range, or outside as a new one
  // check for overlaps CHANGE <= and >= to < and >  FROM OLCHK COMMANDS

  if (ix < chbase.num)
    {
     int err;

     err = 0;

     if (x->rstart < z->rstart && x->rend   >= z->rstart && x->rend   <= z->rend) err = 1;   //overlaps rstart
     if (x->rend   > z->rend   && x->rstart >= z->rstart && x->rstart <= z->rend) err = 1;   // overlap rear

     if (err)
      {
       chbase.lasterr = E_OVRG;          // overlap ranges

       #ifdef XDBGX
        if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail OLAP");
        DBGPRT(0," rbase R%x = %05x", reg, addr);
        if (rstart) DBGPRT(0,"(%05x - %05x)", rstart, rend);
        DBGPRT(1,0);
       #endif
       return 0;
      }
    }


  if (ans)
   {
    chinsert(&chbase, ix, x);
    chbase.lasterr = 0;
   }
  else
   {
     chbase.lasterr = E_DUPL;       //duplicate
   }

 #ifdef XDBGX
  if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"DUPL");
  DBGPRT(0," rbase R%x = %05x", reg, addr);
  if (rstart) DBGPRT(0,"(%05x - %05x)", rstart, rend);
  DBGPRT(1,0);
 #endif

 if (ans) return x;
 return 0;

}


RBT* get_rbt(uint reg, uint pc)
{
  RBT *r, *x;
  uint ix;

  r = (RBT*) chmem(&chbase);
  r->reg    = reg;
  r->rstart = pc;
  r->rend   = pc;

  ix = bfindix(&chbase, r);

  if (ix < chbase.num)
    {
     x = (RBT*) chbase.ptrs[ix];
     if (x->inv) return NULL;
     if (x->reg == r->reg && r->rstart >= x->rstart && r->rstart <= x->rend) return x;          //   match
    }

return NULL;

}


RST* get_rgstat(uint reg)
{
  uint ix;
  RST *b, *t;

 t = (RST *) chmem(&chrgst);
 t->reg  = reg;

 ix = bfindix(&chrgst, t);
 b = (RST*) chrgst.ptrs[ix];

 if (ix < chrgst.num)
   {       // exact match reqd
    if (b->reg != t->reg) return NULL;
    return b;    // must match to get here
   }

 return NULL;

}


void set_rgsize(RST *r, uint fend)
{
    // need ix anyway so may as well use it
    // only allow byte and word sizes....

   if (!r) return;

   if ((fend & 15) > (r->fend & 15))
    {
     r->fend = fend;              // just do up to word size
     #ifdef XDBGX
       DBGPRT(1,"Reg R%x Fend = %d", r->reg, r->fend);
     #endif
    }
}


RST* add_rgstat(uint reg, uint fend, uint sreg, uint ofst)
{

 RST *x;
// RBT *b;
 int ix, ans;

 if (!val_general_reg(reg))
   {
    #ifdef XDBGX
    DBGPRT (0,"REJECT Reg Inv");
    DBGPRT  (0," rgstat %x fend %d from %x", reg, fend, ofst);
    DBGPRT(1,0);
   #endif
    return NULL;
}

 if (fend == 0) return NULL;

 x = (RST *) chmem(&chrgst);
 x->reg  = nobank(reg);

 ix  = bfindix(&chrgst, x);
 ans = chrgst.comp(&chrgst,ix, x);

 if (ans)
   {
    chinsert(&chrgst, ix, x);
   }
 else
   {
    x = (RST*) chrgst.ptrs[ix];
    memset(x,0, sizeof(RST));                 // and clear it ?
    x->reg = reg;
   }

  x->ofst = ofst;
  x->fend = fend;
  x->sreg = sreg;

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT  (0," rgstat %x fend %d ofst %x", x->reg, x->fend, x->ofst);
    DBGPRT(1,0);
   #endif

 return x;

}


SPF *get_spf(void* fid)
{
 int ix;
 SPF *s;

 s = (SPF*) chmem(&chspf);
 s->fid = fid;


 ix = bfindix(&chspf, s);

 if (!chspf.comp(&chspf,ix,s))
 {
   return (SPF*) chspf.ptrs[ix];
 }

chspf.lastix = chspf.num;   // invalid lastix
return NULL;
}

SPF *find_spf(void *fid, uint spf)
{

 SPF *s;
 uint m ;

  while ((s = get_spf(fid)))
    {
     m = 0;
     if (!spf || s->spf == spf) m++; // i.e. zero is 'any'
     //in case need more pars....
     if (m) return s;
     fid = s;
    }

 return NULL;
}

SPF* append_spf (void *fid, uint spf, uint from)
  {
   // add special func block
   // subrs can have multiple special functions.
   // adt may be attached to an spf for remote arg getters
   // using vconv, fid can be a bin address ot a void* blk pointer

   // unique by spf type and address

  SPF *a, *s;
  uint ix;
  int ans;

  if (!spf) return NULL;            //safety
//  if (!lev) return NULL;            //safety


//look for last item in chain, and at same time check for duplicates

  s = get_spf(fid);

  while (s)
  {
 //   if (s->spf == spf && s->level == lev && s->fromadd == from)
    if (s->spf == spf && s->fromadd == from)
      {
       chspf.lasterr = E_DUPL;
       #ifdef XDBGX
          DBGPRT(1, "DUPL spf %d to %lx from %x",  spf,  fid, from);
       #endif
       return 0;
       }

    a = get_spf(s);         // get next item
    if (!a) break;          // a is end of chain
    s = a;                  // last valid item
  }

  a = (SPF *) chmem(&chspf);

//last valid id of chain

  if (!s) a->fid = fid;
  else
    {
        a->fid = s;
    }

  a->spf = spf;
 // a->level = lev;
  a->fromadd = from;

  ix = bfindix(&chspf,a);
  ans = cpspf(&chspf,ix, a);     //chspf.cheq(

  if (!ans)
   {
            #ifdef XDBGX
     DBGPRT(0, "add spf PANIC!!");
     #endif
   }
  else
  {
    chinsert(&chspf, ix, a);

    #ifdef XDBGX
      DBGPRT(1, "Add spf %d to %lx from %x",  spf,  fid, from);
    #endif

  }

  return a;
}

ADT *get_adt(void *fid, uint bix)
{
    // bix is for subchains on same fid - change to bottom bit of the void ??
 uint ix;
 ADT *s;
 CHAIN *x;
 x = &chadnl;

 s = (ADT*) chmem(&chadnl);
 s->fid = fid;
 s->sbix = bix;

 ix = bfindix(x, s);

 if (!cpadt(x, ix,s)) return (ADT*) x->ptrs[ix];

 x->lastix = x->num;   //invalidate if not found
 return NULL;
}


void free_adt(void *fid)
 {

  ADT *a;
  uint ix;

 while ((a = get_adt(fid,0)))
  {
    //but what about subfields ??? copy from print ??
    ix = chadnl.lastix;        //of a
    chdelete(&chadnl,ix,1);         // a->cnt = 0;
    fid  = a;
  }


 }



ADT* append_adt (void *fid, uint fix)
  {
   // add special func block
   // subrs can have multiple special functions.
   // adt may be attached to an spf for remote arg getters
   // using vconv, fid can be a bin address ot a void* blk pointer

  ADT *a, *s;
  uint ix;
  int ans;

  if (!fid) return NULL;            //safety
//  if (!lev) return NULL;            //safety


//look for last item in chain, and at same time check for duplicates

  s = get_adt(fid, fix);

  while (s)
  {
 /*   if (s->spf == spf && s->level == lev && s->fromaddr == from)
      {
       chspf.lasterr = E_DUPL;
            #ifdef XDBGX
       DBGPRT(0, " DUPL SPF ");
       #endif
       return s;
       }  */

    a = get_adt(s,0);         // get next item (always zero)
    if (!a) break;          // a is end of chain
    s = a;                  // last valid item
  }

  a = (ADT *) chmem(&chadnl);
  a->cnt    = 1;
  a->fend   = 7;               // single byte default

 // a->sbix = fix;              // fid and index     maybe later...

//last valid id of chain

  if (!s) a->fid = fid;
  else
    {
        a->fid = s;
    }


  ix = bfindix(&chadnl,a);
  ans = cpadt(&chadnl,ix, a);

  if (!ans)
   {
            #ifdef XDBGX
     DBGPRT(0, "PANIC!!");
     #endif
   }
  else
  {
    chinsert(&chadnl, ix, a);
  }

  return a;
}




LBK *get_cmd (uint start, uint fcom)
{
  // returns match if ofst within range of a data block (start-end)
  LBK *blk;
  int ix;

  blk = (LBK*) chmem(&chcmd);
  blk->start = start;
  blk->fcom =  fcom;

  ix = bfindix(&chcmd, blk);

  // if command SPANs this address, will not always be selected.

//  while (ix < chcmd.num)
//    {
   //  blk = (LBK*) chcmd.ptrs[ix];
//     if (p->end < start) return NULL;



  //    if (p->start <= start && p->end >= start) return start = 0;
    //            }
      //        if (start) DBGPRT(1,"push %x from %x",start, d->ofst);
        //     }





  if (eqcmd(&chcmd,ix,blk)) return NULL;    // no match

  return (LBK *) chcmd.ptrs[ix];
}

LBK *get_aux_cmd (uint start, uint fcom)
{
  // returns match if ofst within range of a data block (start-end)
  LBK *blk;
  int ix;

  blk = (LBK*) chmem(&chaux);
  blk->start = start;
  blk->fcom =  fcom;

  ix = bfindix(&chaux, blk);

  if (eqcmd(&chaux,ix,blk)) return NULL;    // no match

  return (LBK *) chaux.ptrs[ix];
}







RST *get_rgarg(uint reg, uint addr)
{
  uint i;
  RST *r, *ans;

  ans = 0;
  for (i = 0; i < chrgst.num; i++)
  {
    r = (RST*) chrgst.ptrs[i];
//    if (r->arg && r->sreg == reg)
      if (r->ofst == addr)
    {
        ans = r;
        break;
    }
  }
        return ans;
}


SBK *get_scan(uint addr)
{
 SBK *x, *ans;
 int ix;

  x = (SBK*) chmem(&chscan);       // dummy block for search
  x->start = addr;                  // symbol addr

  ans = NULL;
  ix = bfindix(&chscan, x);
  if (!chscan.comp(&chscan,ix,x))
 ans = (SBK *) chscan.ptrs[ix];

 return ans;
}




SUB *get_subr(uint addr)
{
  SUB *s;
  uint ix;

  if (!addr) return 0;         // shortcut..
  s = (SUB*) chmem(&chsubr);
  s->start = addr;

 ix = bfindix(&chsubr, s);
 if (chsubr.comp(&chsubr,ix,s)) return NULL;    // no match
 return (SUB*) chsubr.ptrs[ix];
}


SBK *get_scan(uint addr, uint subaddr)
{
 int ix;
 SBK *s;

 s = (SBK*) chmem(&chscan);
 s->start = addr;
 s->substart = subaddr;

 ix = bfindix(&chscan, s) ;

 if (chscan.comp(&chscan,ix,s)) return NULL;

 return (SBK*) chscan.ptrs[ix];

}



/*
ADT *get_adt(void *fid, uint bix)
{
    // bix is for subchains on same fid - change to bottom bit of the void ??
 uint ix;
 ADT *s;
 CHAIN *x;
 x = &chadnl;

 s = (ADT*) chmem(CHADNL);
 s->fid = fid;
 s->sbix = bix;

 ix = bfindix(x, s);

 if (!x->cheq(x, ix,s)) return (ADT*) x->ptrs[ix];

 x->lastix = x->num;   //invalidate if not found
 return NULL;
}
*/







TRK *get_dtk(uint ofst)
{
 uint ix;
 TRK *s;

 s = (TRK*) chmem(&chdtk);     //always master chain
 s->ofst = ofst;

 ix = bfindix(&chdtk, s);

if (ix < chdtk.num)
 {
  s = (TRK*) chdtk.ptrs[ix];
  if (ofst == s-> ofst) return s;
 }

chdtk.lastix = chdtk.num;   //invalidate if not found
return NULL;             //(DTKD*) x->ptrs[ix];

}

DTD *get_dtkd(CHAIN *x, uint ofst, uint start)
{
 uint ix;
 DTD *s;

// chdtko   index by ofst
// chdtkd   index by start [REMOTE]
 s = (DTD*) chmem(&chdtko);     //always master chain
 s->ofst = ofst;
 s->stdat = start;

 ix = bfindix(x, s);

if (ix < x->num)
 {
  s = (DTD*) x->ptrs[ix];
  if (ofst && s->ofst == ofst) return s;
  else if (start == s->stdat) return s;
 }


// x->lastix = x->num;   //invalidate if not found
return NULL;             //(DTKD*) x->ptrs[ix];

}

/*
DTD *start_dtdk_loop(uint ofst)
{
 DTD *a;
  a = (DTD*) chmem(&chdtko);
  if (a) a->ofst = ofst;
 return a;
}
*/

DTD *get_next_dtkd(DTD *d)
{
 DTD *a;
 uint ix;

// speedup shortcut - FAULTY !!
// NB> can't use dtka as it tests stdat as well...

// ix = chdtko.lastix + 1;

// if (ix < chdtko.num)
//   {  // if valid lastix - speedup trick for get_next
 //   a = (DTD*) chdtko.ptrs[ix];

 //   if (a->fk == d->fk)
  //    {  // next item in sequence
  //     chdtko.lastix = ix;         // last valid
 //      return a;
//      }
//   }

 // otherwise, just look for it

 a = (DTD*) chmem(&chdtko);
 a->ofst = d->ofst;
 if (d->stdat) a->stdat = d->stdat+1;

 ix = bfindix(&chdtko, a);

 if (ix >= chdtko.num) return NULL;   // not found

   a = (DTD*) chdtko.ptrs[ix];
   if (a->ofst == d->ofst)
      {  // next item in sequence
       chdtko.lastix = ix;         // last valid
       return a;
      }



 //if (!cpdtka(&chdtko,ix,a)) return (DTD*) chdtko.ptrs[ix];

 return NULL;
}

DTD* add_dtkd (TRK *k, INST *c, int start)
  {
   // add additional data block to fid
   // if seq >= MAXSEQ (255) then add at end of chain (=seq+1)
   // else add actual sequence number

  DTD *a, *x;
  uint ix;
  int ans;

  a = (DTD *) chmem(&chdtko);

  a->ofst    = k->ofst;
  a->stdat = start;

  a->psh   = k->psh;
  a->inc   = k->ainc;
  a->bsze = bytes(opctbl[c->opcix].fend1);

 //if (c->opcsub == 1)  a->reg = c->opr[c->wop].addr;

 //if (c->opcsub == 2)  a->reg = c->opr[4].addr;

  if (c->opcsub == 3) a->off = c->opr[0].addr;


  if (get_cmd(start,C_CODE)) a->olp = 1;

  a->opcsub = k->opcsub;

  //more to go here
  if (c->opcsub == 3)
    {
     if (c->opr[4].addr == 0) a->opcsub = 1;    // R0, make an imd
  //   if (c->opr[4].rbs) a->hq = 1;            // not valid here !!
    }

 // a->bsze  = k->bsze;
 // a->modes = (1 << k->opcsub);





  ix = bfindix(&chdtko,a);

  ans = chdtko.comp(&chdtko,ix, a);

// before insert, check for same fk and calc gap

 if (ix > 0 && ix < chdtko.num)
   {
     x = (DTD*) chdtko.ptrs[ix-1];

     if (x->ofst == k->ofst)
      {
       if ((start- x->stdat) < 32) x->gap = start-x->stdat;
      }

   }


  if (ans)
   {
    chinsert(&chdtko, ix, a);
    ix = bfindix(&chdtkd,a);
    chinsert(&chdtkd, ix, a);
   }
  else
  { // exists already.  Clear it and reset for use. may not be necessary ?
   return NULL;
  }

  chdtko.lastix = chdtko.num;        //must reset any queries
  return a;
}












TRK* add_dtk(SBK *s,INST *c)
 {
  TRK *x;
 // DTD *a;
  int ans;
  uint ix, start;  //, size;

  OPS *o;         //, *w;

  // c instance should have all info required............. BUT NOT FOR ARGS !!


  if (anlpass >= ANLPRT)  return 0;

  if (!s->proctype) return 0;   //may be only scanning ??

  // would need to sort out qualifying ops (LDX, AD2B, imd etc) here
  // normally, source would be [1] which is only mutliple mode, except for stx

  //  ([2] = [1]), but not always........

 /*Signature index value for like instructions - MAX 63
 0  special   1  clr    2  neg/cpl   3 shift l   4 shift R   5 and
 6  add       7  subt   8  mult      9 or        10 cmp     11 divide
 12 ldx       13 stx    14 push     15 pop       16 sjmp    17 subr
 18 djnz      19 cjmp   20 ret      21 carry     22 other
 24 dec       25 inc

*/


//not for compares ???

  // base block setup

  x = (TRK *) chmem(&chdtk);      // next free block
  x->ofst = c->ofst;
  x->ocnt = s->nextaddr-c->ofst;

  x->opcix = c->opcix;

  o = c->opr+1;

  memset(x->rgvl, 0, sizeof(x->rgvl));

  for (ix = 0; ix < 5; ix++) x->rgvl[ix] = c->opr[ix].addr;

  x->opcsub = c->opcsub;
  x->numops = c->numops;

//  x->bsze = bytes(opctbl[c->opcix].rfend1);
  if (c->sigix == 14) x->psh = 1;

  ans = 0;
  start = 0;

  switch (c->opcsub)
    {

      case 0:               // all register op

         if (c->sigix == 25)
           {            //incb or incw
        //    start = databank(o->val,c);
            x->ainc = 1;
            ans = 1;

           }

         if (c->sigix == 14)
           {
            // pushw        //not always code bank ?
            start = codebank(o->val,c);
            ans = 1;
           }

//but probably for size checking, need EVERTYHING...........


        break;

      case 1:    // immediate op

        //need to keep immediate ldx to register as an rbase..............

//start = o->addr;

         if (c->sigix == 14)
           {
            // pushw
            start = codebank(o->addr,c);
            break;
           }

         if (c->sigix == 12)       // || c->sigix == 6)
           {   // ldx
            start = o->addr;
            break;
           }

         if (c->sigix == 6)
           {
            // add - check if answer is valid address.
        // need subtract too !!!!

      //      xofst =  c->opr[2].val + c->opr[1].val;   //this could be right too ? ...........

          //    xofst = o->addr;
           start = o->addr;           //xofst;
            break;
           }

         if (c->sigix == 10)
           {    //cmp
            start = o->addr;
            break;
           }









        break;

      case 2:        //indirect - beware of stx ???

        x->rgvl[1] = c->opr[4].addr;
        start = o->addr;

        if (o->inc) x->ainc = bytes(o->fend);

        break;

     case 3:
        // if R0 or rbase treat as indirect

        x->rgvl[1] = c->opr[4].addr;
        x->rgvl[4] = c->opr[4].val;

        start = o->addr;  // combined address

        start = databank(start,c);

        break;

    }


// if (val_rom_addr(start)) ans = 1;
//indexed, check special reg, but zero is OK
//if (x->reg1 && x->opcsub == 3 && is_special_reg(x->reg1)) return 0;           //inx

// not for print, but may need these for loops
if (start && is_special_reg(start)) return 0;
/// if (start <= max_reg()) return 0;

if (!c->opcsub && !ans) return 0;




//if (x->ofst == 0x9342a)
//{
//DBGPRT(1,0);
//}

  ix = bfindix(&chdtk, x);
  ans = cpdtk(&chdtk,ix, x);


  if (ans)
    {          // no match
     chinsert(&chdtk, ix, x);            // insert main chain

//  ix = bfindix(&chdtkr, x);
//   chinsert(&chdtkr, ix, x);            // insert main chain




//if (c->opcsub) {                     // not reg to reg
if (val_rom_addr(start))
    {
     x = (TRK*) chdtk.ptrs[ix];
     add_dtkd(x,c,start);          // and start address
    }

 //    ix = bfindix(&chdtkr, x);        // find for register index
 //    chinsert(&chdtkr, ix, x);           // inser224e: 07,30               incw  R30              R30++;                            # incr ptr to save addresst reg chain
   //  ix = bfindix(&chdtkd, x);        // find for data index can't do this ?? search dtdkd may work....
  //   chinsert(&chdtkd, ix, x);           // insert data chain
 //    return x;
    }
  else
    {
      // add data address.

     if (val_rom_addr(start))
      {
       x = (TRK*) chdtk.ptrs[ix];
       add_dtkd(x,c,start);
      }
    }

 if (!ans) return 0;
return x;
}
/*
// THIS ONE  to change to TRACKING data


 // LBK *k;
  int addr, sig;             //x

  if (anlpass >= ANLFFS) return;
 // if (s->nodata) return;

  addr = 0;
  sig = opctbl[c->opcix].sigix;

  if (sig == 14)  return;    // not push



 / opcsub == 1 for immediates with valid ROM / RAM address ??
 // how to tell what's real though ? Assume it always calls a indirect ??


  if (c->opcsub == 1)
  {     // ldx or add with immediate only. ALSO check it's not a cmp limit check ?
    if (sig == 12 || sig == 6)
      {
       o = c->opr+1;

    //   if (val_rom_addr(o->addr))  addr = o->addr ;

    //   if (c->wop == 1) addr = o->addr;

       if (nobank(addr) > PCORG)
         {
          x = find_last_psw(c->ofst,0);
          if (x && sinst.opcix > 33 && sinst.opcix < 36)
           {  // found  a cmp
              if (sinst.opr[2].addr == c->opr[2].addr
               && sinst.opr[2].val == c->opr[2].val)
             {  // cmp matches the ldx, i.e. a LIMIT CHECK
              addr = 0;
             }
           }
         }
       }
    }         /


// indirect but NOT for writes (STX) or zero wops

  if (c->opcsub == 2)
   {
     o = c->opr+1;
     if (c->wop > 1) addr = o->addr;

  //   if (c->inc)
 //    { //struct ?
 //    DBGPRT(1,"INC++ STRUCT?");
 //    }
   }

 // indexed

  if (c->opcsub == 3)
  {
   o = c->opr+4;                         // = saved opr[1]

 // if (!o->wsize)
 // {
  // if [0] is a valid ROM address, then use that.
  // if R[1] is a valid ROM address with small offset, then use R[1]
  // if R[1] is an rbase, use combined address in [1]

   if (c->opr[1].rbs) addr = c->opr[1].addr;    // combined address
   else
    {
     addr = c->opr->addr;                 // address of offset [0]
     if (addr >= (PCORG+0x8))
       {
        addr = databank(addr,c);       // use fixed offset
       }
     else
      {
       addr = o->addr; // fixed offset is too small, use register ?
       if (addr >= (PCORG+0x8)) addr = databank(addr,c); else addr = 0;
      }
    }
  // }
  }


  if (addr)
     {            // addr must be val_rom_addr
//      k =
//add_data_cmd (addr,  addr+cnv[o->ssize].bytes-1, cnv[o->ssize].datcmd, c->ofst);       // add data entry
 //     if (k)
 //      {    // add offset and type for later checks
    //    k->opcsub = c->opcsub;
    //    if (c->opcsub == 3 && c->opr[4].addr)
    //    if (c->opr[4].val > 0 && c->opr[4].val < 17)
    //    k->soff = c->opr[4].val;                // offset 0-16
  //     }
     }
 }

*/

BNKF *add_bnkf(uint add, uint pc)
{
  CHAIN *x;
  BNKF *b;
  uint ix;
  int ans;

  x = &chbkf;         //chindex[CHBNK];

  b = (BNKF *) chmem(&chbkf);
  b->filestart = add;
  b->pcstart   = pc;

  ix = bfindix(x,b);

  ans = x->comp(x, ix,b);

  if (!ans)
    { // exists already.
     return NULL;
    }
  else
   {
    chinsert(&chbkf,ix,b);
   }

return b;
  }



void fsym(void *x)
{            // free symbol
 SYM *s;

 s = (SYM*) x;
 if (s->symsize)  mfree (s->name, s->symsize);              // free symbol name
 s->name = NULL;
 s->symsize = 0;
}

void fsub(void *x)
 {
  free_adt (x);         //&(s->adnl));
 }

void fcmd(void *x)
{       // free cmd
  free_adt (x);         //&(k->adnl));
}


int adtchnsize(void *fid, uint nl)
{
 // continue from fid to next newline
 // save lastix in case it hits end

 ADT *a;
 int sz;
 sz = 0;

 while ((a = get_adt(fid,0)))
   {
    sz +=  cellsize(a);                             // size in bytes
    if (nl && a->newl) break;
    fid = a;
   }

 return sz;
}


void set_retn(JMP *x)
 {
 // set retn flag for anything which jumps to addr
 // bfindix on to chain will always get FIRST jump
 // called recursively

  uint ix;
  JMP *t;

  if (x->jtype != J_STAT) return;

  t = (JMP*) chmem(&chjpf);     //use forward chain, can't use backward
  t->toaddr = x->fromaddr;
  ix = bfindix(&chjpt, t);     // TO chain, finds first match

  while (ix < chjpt.num)
   {
    t = (JMP *) chjpt.ptrs[ix];     // match
    if (t->toaddr != x->fromaddr) break;
    // only if 't' (from) is static
    if (t->jtype == J_STAT)
       {
        t->retn = 1;
        set_retn(t);      // check jumps to here
       }
    ix++;
   }
  }


/*
int fwdadds(JMP *j, uint *from, uint *to)
 {
    // return valid to and from, always forwards

  if (!j->jtype) return 1;
  if (j->bswp)   return 1;
  if (j->jtype == J_SUB) return 1;      // ignore subr and return jumps

  if (j->retn) return 1;             // TEST !!

  if (j->back)
     {  // backwards jump
      *from = j->toaddr;
      *to   = j->fromaddr;
     }
  else
     {
      *from = j->fromaddr;
      *to   = j->toaddr;
     }
 return 0;
 }


uint faddr(JMP *j)
 {
    // return valid to and from, always forwards

  if (j->back) return j->toaddr;
  return j->fromaddr;
 }

int taddr(JMP *j)
 {
    // return valid to and from, always forwards

  if (j->back) return j->fromaddr;
  return j->toaddr;
}

*/
/*
void outside (JMP* b, JMP *t)
 {
     // jump overlap tester.  b is base, t is test
      // could flag b directly

   uint bfrom, bto;
   uint tfrom, tto;       // need these in case jumps are backwards

   if (b == t)        return;     // don't compare same jump...
   if (fwdadds(b, &bfrom, &bto)) return;
   if (fwdadds(t, &tfrom, &tto)) return;

// outside jumps in

   if (tto > bfrom && tto < bto)
     {
      if (tfrom < bfrom)  b->uci = 1;
      if (tfrom > bto)    b->uci = 1;
     }

/ inside jumps out - allow this as experiment

  if (tfrom > bfrom && tfrom < bto)
     {
      if (!t->jelse) {
      if (tto < bfrom)  b->uco = 1;
      if (tto > bto)    b->uco = 1;
} //  else t->cbkt = 0;
     }


 }

   if (f->jtype == J_COND && !f->retn)
zx = ix+1;                                  // next jump
          t = (JMP*) chjpf.ptrs[zx];
          while (t->fromaddr <= f->toaddr && zx < chjpf.num)
            {
             outside(f, t);
             zx++;
             t = (JMP*) chjpf.ptrs[zx];
            }

void check_jump_span(JMP *j)
 {
  // check if j is within a cond jump (and possibly other cases too)
  // j is candidate for insert
  // get minimum overlap candidate and go forwards

  JMP *x;
  uint ix, oix;

  if (j->jtype != J_STAT) return;

  x = (JMP*) schmem();
  x->toaddr = j->fromaddr;                 // to get to first of overlaps (to >= from)

  oix = bfindix(&chjpt, x, 0);      // TO chain

  if (oix >= chjpt.num)  return;  // not found

  ix = oix;

  // ix is minimum possible match (as  to >= from)

  while (ix < chjpt.num)
   {
      x = (JMP*) chjpt.ptrs[ix];

      if (x != j)
       {
         if (x->jtype == J_COND || x->jtype == J_ELSE)
          {
             // ignore backwards jumps for now
           if (!x->back)
            {
              if (x->fromaddr > j->fromaddr) break;      // no more overlaps
              if (x->fromaddr < j->fromaddr && x->toaddr > j->fromaddr)     // && !x->back
                {          // static is inside a conditional
                   // BUT not if cond only jumps a single goto............... (xdt2)
                 // if (x->to == j->from+j->size) then only jumps over another jump,
                 //  so j remains a STAT

 //                 if (x->to == j->from+j->size && x->from+x->size == j->from )
 //DBGPRT(0,"Ignore ");
//else
                  j->jtype = J_ELSE;
                      #ifdef XDBGX
                  DBGPRT(1,"CJUMP %x-%x spanned by %x-%x", j->fromaddr, j->toaddr, x->fromaddr, x->toaddr);
                  #endif
                  break;
                }
            }
          }
       }
      ix++;
   }
 }
*/

void do_jumps (void)
{

/******* jumps checklist ***************
 set conditions flags for various things
 whether jumps overlap, multiple end points etc etc.
*****************************************/


  uint ix, zx;
 // uint initofst;             // where initialise is.
//  uint tstart, tend, fstart, fend;    // have to use indexes here as interleaved scan

//  uint excnt;
  uint tend;
  JMP *f, *t;

  // first, check for jumps to return opcodes
  // true ret has jtype 0

  for (ix = 0; ix < chjpf.num; ix++)
    {         // down list of forward jumps
     f = (JMP*) chjpf.ptrs[ix];

     //assume brackets OK for all conds except backwards jumps

     if (f->jtype == J_COND && !f->back)   //f->bkt = 1;
      {
       f->obkt = 1;         //assume bkts OK at first
       f->cbkt = 1;
   //    f->done = 0;
      }

     if (f->jtype)
      {     // static and cond jumps
       tend = g_byte(f->toaddr);
       if (tend == 0xf0 || tend == 0xf1)
         {
          f->retn = 1;
          set_retn(f);    // recursive for STAT jumps only
         }
      }
    }

// check for overlaps in jumps

  for (ix = 0; ix < chjpf.num; ix++)          //unsigned !!
    {
     f = (JMP*) chjpf.ptrs[ix];                // down full list of forward jumps


   //  fend   = f->fromaddr + f->size;           // end of jump
   //  fstart = f->fromaddr - f->cmpcnt;         // start, from cmp if present

   //   if (f->back)
   //      {
    //        f->jdo = 1;
   //         f->bkt = 0;         // later !!
      //      f->cbkt = 0;
    //        f->done = 1;
   //      }

     if (f->jtype == J_COND && !f->back) // && !f->done)
        { // only for conditional jumps (as base)
          // try to sort out heirarchy for whether brackets can be used
          // fromaddr is unique, but toaddr is not
          // in order of FROM...

//findix instead ??

           zx = ix+1;                                  // next jump
      //     t = (JMP*) chjpf.ptrs[zx];

           while (zx < chjpf.num)
             {      // inside -> outside (same chain)

              t = (JMP*) chjpf.ptrs[zx];
              if (t->fromaddr >= f->toaddr)  break;

            //  if (t->fromaddr < f->toaddr)
              //  {
                 if (t->toaddr > f->toaddr) t->obkt = 0;
                 if (t->toaddr < f->fromaddr) t->obkt = 0;
             //   }

    /*   skip these for now...
              //  check for 'else' and 'or' etc.
              tend   = t->fromaddr + t->size;           // end of jump
              tstart = t->fromaddr - t->cmpcnt;

              if (t->jtype == J_STAT && tend == f->toaddr && tstart > fend && !t->back)
               {
                t->jelse = 1;          //type = J_ELSE;
               t->cbkt = 1;
               f->cbkt = 0;
         //    if (

               }

              if (tstart == fend && f->toaddr == tend)   //   && tstart > fend)
               {
               DBGPRT(1,0);
               if (t->jtype == J_COND) t->jor = 1;
               }
//*/

             zx++;
           //  t = (JMP*) chjpf.ptrs[zx];

          //   if (t->fromaddr > f->toaddr)  break;      moved up

            }  //end while (inside->outside)








//          if (f->jtype == J_COND && !f->back && !f->bswp && !f->uci && !f->uco) f->cbkt = 1;      // conditional, forwards, clean.

        }
    }


// need to check for loops within loops here ....NO, move to dtk processing.
// do an additional check for bins which jump to initialise..... (jump zero ?) No, bank 8

//bfindix on 82000 and find nearest one (find or after)


 // f = (JMP*) chmem(&chjpf,0);
 // f->fromaddr = 0x92000;             //first/initial from jump
 // ix = bfindix(&chjpf, f);
 // f = (JMP*) chjpf.ptrs[ix];

 // initofst = f->toaddr;

/*  j = (JMP*) find(&chjpf,j);         // jump from
void *find(CHAIN *x, void *b)
{
 // find item with optional extra pars in b

 int ix;
 ix = bfindix(x, b, 0);
 if (x->equal(x,ix,b)) return NULL;    // no match
 return x->ptrs[ix];
}

 if (j->back && !j->retn && (j->jtype == J_STAT || j->jtype == J_COND))

STILL needs more 8481 A9L .....


excnt = 0;

  for (ix = 1; ix < chjpf.num; ix++)
    {
     f = (JMP*) chjpf.ptrs[ix];                // down full list of forward jumps

     excnt = f->fromaddr - f->toaddr;
//drop init check, but may need something more...........

     if (f->back && !f->retn && !f->bswp && (f->jtype == J_STAT || f->jtype == J_COND)
   //      && f->toaddr != initofst
          && excnt < 256 && excnt > 0)
       {


//technically ALL backward jumps not ret or loopstop should qualify for a loop check, including
// loops inside loops.............


//must verify a loop HERE.   .....how ?? ..... must have an increment ???




    // f is probably a short loop jump.........

       //find toaddr in from chain, for any contained jumps..................or go backwards....


if (f->jtype == J_COND) f->exloop = 1;     // conditional IS already an exit

//DBGPRT(1,0);

       // if f->back set, fromaddr > toaddr.....

     //  if (ix > 0)
       //  {
//go DOWN the chain..........


           zx = ix-1;                                  // prev jump

           while (zx > 0)
             {
              t = (JMP*) chjpf.ptrs[zx];
              if (t->fromaddr < f->toaddr) break;     // outside bounds for backwards jump



              // t->fromaddr ALWAYS < f->fromaddr AND >= f->toaddr to get here

      //         if (t->back)         //for loops
      //          {
            //      if (t->toaddr < f->fromaddr && t->toaddr >= f->toaddr )
             //        {
            //          t->subloop = 1;
           //          }
          //      }
            //but exit jumps could be here too...........................
          //    else
                {  //std jump
                  if (t->fromaddr > f->toaddr)
                    {

if (t->fromaddr == 0x92042)
{
DBGPRT(1,0);
}

                     if (t->toaddr <= f->fromaddr) {t->inloop = 1;}    // t->exloop = 0;}
                     if (t->toaddr == f->fromaddr+f->size) t->exloop = 1;          //> f->fromaddr) t->exloop = 1;
                    }
                }

             zx--;
            }
        // }
       }
    }
*/
}






LBK* add_aux_cmd (uint start, uint end, uint com, uint from)
 {
  LBK *n;

  if (anlpass >= ANLPRT)    return NULL;

  //  verify start and end addresses

  if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

  // can get requests for 0xffff with bank, which will break boundary....

  if (!bankeq(end, start))
    {
      #ifdef XDBGX
     // if (nobank(start) < 0xffff) to drop warning
      chaux.lasterr = E_BKNM;
      DBGPRT(1,"Invalid - diff banks %05x - %05x", start, end);
      #endif
      return NULL;
    }

  if (end < start) end = start;

  // check start and end within bounds - no, need full range

 // chaux.lasterr = E_INVA;       // invalid address

 // if (!val_rom_addr(start)) return NULL;

 // if  ((com &0x1f) == C_XCODE)
 //  {  // special for xcode
 //   if (end > maxadd (end)) return NULL;
 //  }
//  else
//  if (!val_rom_addr(end))   return NULL;

  n = (LBK *) chmem(&chaux);
  n->start = start;
  n->end   = end;
  n->fcom  = com & 0x1f;    // max 31 as set
  if (com & C_USR) n->usrcmd = 1;
 // if (com & C_SYS) n->sys = 1;

  n = inscmd(&chaux,n);            //is this reqd with only aux cmds ? (xcode and args)

  if (chaux.lasterr) return 0;                        // fail

  #ifdef XDBGX
  DBGPBK(1,n, "Add aux");
  #endif

  return n;   // successful
 }

SUB * add_subr (uint addr)
{
  // type is 1 add name, 0 no name
  // returns valid pointer even for duplicate

  int ans, ix;
  SUB *x;

  if (anlpass >= ANLPRT) return NULL;
  if (!val_rom_addr(addr)) return NULL;

  if (get_aux_cmd(addr, C_XCODE))
     {
       #ifdef XDBGX
       DBGPRT(1,"XCODE bans add_subr %x", addr);
       #endif
      return NULL;
     }


// insert into subroutine chain

  x = (SUB *) chmem(&chsubr);
  x->start = addr;

  ix = bfindix(&chsubr, x);
  ans = cpsub (&chsubr,ix,x);

  if (ans)
   {
    chinsert(&chsubr, ix, x);       // do insert
    chsubr.lasterr = 0;
    if (PARGC) x->cptl = 1;         // set compact layout
   }
  else
  {
   // match - do not insert. Duplicate.
   x = (SUB*) chsubr.ptrs[ix];
   chsubr.lasterr = E_DUPL;
  }

  #ifdef XDBGX
    if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Dup");
    DBGPRT(0," sub %x",  addr);
    DBGPRT(1,0);
  #endif
  return x;
  }




 SBK *add_scan (uint add, int type, SBK *caller)
{

 /* add code scan.  Will autocreate a subr for type "SUBR"
  * updates type etc even for duplicates to fit 'rescan' type model
  * C_CMD (+32) = by user command (can't be changed)
  * caller is only set in calls from do-sjsubr and taks lists (vect)
  */

   SBK *s;

   int ans;
   uint ix;

   if (anlpass >= ANLPRT)   return 0;

   // TEMP add a safety barrier ...
   if (chscan.num > 20000) return 0;

   if (!val_rom_addr(add))
    {
     chscan.lasterr = E_INVA;
     #ifdef XDBGX
       ix  = (nobank(add));   // reserved addresses
       if (ix >= 0xd000 && ix <= 0xd010) return NULL;
       if (ix == 0x1f1c || ix == 0x1000 || ix == 0x1800) return NULL;
       DBGPRT (1,"Invalid scan %x", add);
     #endif

     return NULL;
    }

 /*  if (get_opdata (add))  breaks stuff. Stops dupls
    {
     #ifdef XDBGX
     DBGPRT (1,"Scan OLAP %x", add);
     #endif
     chscan.lasterr = E_INVA;
     return 0;
    }   */









   if (get_aux_cmd (add, C_XCODE))
     {
      #ifdef XDBGX
      DBGPRT(0,"XCODE bans scan %x", add);
      DBGPRT(1,0);
      #endif
      chscan.lasterr = E_XCOD;
      return 0;
     }

   if (PMANL)
      {
       #ifdef XDBGX
       DBGPRT(1,"no scan %x, manual mode", add);
       #endif
       return 0;
      }



   s = (SBK *) chmem(&chscan);           // new block (zeroed)
   s->start    = add;
   s->curaddr  = add;                    // important !!
   s->nextaddr = add;
   s->proctype = 1;                     //scanning = 1;                      // scan processing on (default)
   s->logdata  = 1;                      // and log data found

   if (type & C_USR) s->cmd = 1;         // added by user command
   if (type & C_GAP) s->gapscan = 1;     // gap scanning
   s->type     =  type & 0x1f;

   if (caller)
      {
       s->substart = caller->substart;
       s->caller   = caller->caller;         // default to same level
       s->logdata  = caller->logdata;        // keep nodata flag
       s->gapscan   = caller->gapscan;         // keep gap check flag too
      }

   if (s->type == J_SUB)
    {
      if (caller) s->caller = caller;     // new level
      add_subr(s->start);
      s->substart =  s->start;            // new subr
      new_autosym(1,s->start);            // add name (autonumbered subr)
    }

   if (s->caller && s->start == s->caller->start)  return 0;          // safety check

   ix = bfindix(&chscan, s);
   ans = cpscan(&chscan,ix,s);                     // checks start address ONLY, not range

   if (!ans)
    {                                     // match - duplicate
     SBK *t;
     t = (SBK*) chscan.ptrs[ix];          // block which matches

     if (s->caller && t->caller == 0)
       {   // was a user or system added scan,and now has caller
        *t = *s;       // replace
       #ifdef XDBGX
       DBGPRT (0,"Replace");
       DBGPRT  (0," scan %05x %s", s->start, jtxt[s->type]);
       if (s->substart)  DBGPRT (0," sub %x", s->substart);
       if (s->caller) DBGPRT(1, " caller %x", s->caller->start);
       #endif
       if (ix < lowscan)  lowscan = ix;       // reset lowest scan index
       return t;
       }
     s = (SBK*) chscan.ptrs[ix];          // block which matches
     chscan.lasterr = E_DUPL;
    }
   else
    {        // no match - do insert
//check flags here ??
     chinsert(&chscan, ix, s);
     if (ix < lowscan)  lowscan = ix;       // reset lowest scan index

     if (s->substart)                       // scan all blks within subr
       {
        ix = bfindix(&chsbcn, s);           // and insert in subscan chain
        chinsert(&chsbcn, ix, s);
       }

     if (s->gapscan)
       {
        ix = bfindix(&chsgap, s);           // and insert in gap chain
        chinsert(&chsgap, ix, s);
       }
     chscan.lasterr = 0;
    }


 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT  (0," scan %05x %s", s->start, jtxt[s->type]);
    if (s->substart)  DBGPRT (0," sub %x", s->substart);
    if (s->caller) DBGPRT(0, " caller %x", s->caller->start);

    if (!ans)
    {      //dupl
//    if (!s->stop) DBGPRT(0," Not");
    if (s->stop) DBGPRT(0, " Scanned");
    if (s->args) DBGPRT(0," ARGS!");
    if (s->chscan) DBGPRT(0," CHAIN");
    }
    DBGPRT(1,0);
   #endif
 return s;
}



SBK *add_escan (uint add, SBK *caller)
{

 /* add emu scan.
  if addr is zero, COPY caller, else work as 'standard' scan
  */

   SBK *s;

   int ans, ix;

   if (anlpass >= ANLPRT)   return 0;

   s = (SBK *) chmem(&chemul);           // new block (zeroed)
   if (add) s->start = add;                // standard add emu
   else s->start    = caller->start;       // copy of SBK to new emu

   ix = bfindix(&chemul, s);
   ans = cpscan(&chemul,ix,s);             // checks start address ONLY

   if (!ans)
    {                                     // match - duplicate
     s = (SBK*) chemul.ptrs[ix];          // block which matches
    }
   else
    {        // no match - do insert (s is valid pointer)
     chinsert(&chemul, ix, s);
    }

   if (!add)
    {
      // copy caller block
      *s = *caller;                          // copy whole block and pars
  //    s->scanning = 0;                       // clear scanning flag
    }
   else
    {
     // this add will ALWAYS be the a subr call in EMU, caller will always be set
      s->caller = caller;
      s->substart =  s->start;               // new subr
    }

   s->curaddr = s->start;
   s->nextaddr = s->start;
   s->proctype = 4;                         // emulating = 1;
   s->stop = 0;
   s->inv = 0;                             // and always set for emu rescan


 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT (0," EMUscan %x", s->start);
    if (add) DBGPRT(0," New");  else DBGPRT(0," Copy");
    if (caller) DBGPRT(0," from %x" , caller->curaddr);
    DBGPRT(1,0);
   #endif
 return s;
}


void fixbitfield(uint *add, uint *fstart, uint *fend)
{
 uint i, flags;

// move fstart to byte addr, and fend to match
// keep sign,write,nobit flags (0xe0)

  flags = (*fend) & 0xe0;   // sign,write, nobit

  *fend   &= 0x1f;
  *fstart &= 0x1f;

  if (*fstart > 7)
    {
      i = (*fstart)/8;     // this works for doubles too....
      *add += i;           // add extra bytes
      i *= 8;              // bits to subtract
      (*fstart) -= i;
      (*fend)   -= i;
    }

  (*fend) |= flags;        // restore flags

}





/*
uint fix_bare_addr(uint addr)
{
  // force single bank addrs to bank 9,
  // add databank if there isn't one
  // this is internal so NO BANK ADDITION

  uint x;
  x = nobank(addr);

  // < 0x400 is a register, no bank

  if (x <= max_reg()) return x;

  // single banks always 9 (databank)
  if (!numbanks)      return x | 0x90000;

  // no bank and multibank, default to databank 2 - is this right ??

  if (!g_bank(addr))  return x | 0x20000;         //basepars.datbnk;

  return addr;

}*/


//v5 find sym

/*
void mark_olap_syms(uint ix)
{
    // done after insert, so all checks passed.
    // check if

SYM *ch, *t;

if (!ix) return;

ch = (SYM *) chsym.ptrs[ix];         // chain block to check
t =  (SYM *) chsym.ptrs[ix-1];       // previous (larger?) entry

if (ch->addr & 1)  return;           // odd addresses must be byte anyway
if (t->addr != ch->addr) return;      // addresses don't match. OK

if ((ch->fend & 0x60) != (t->fend & 0x60)) return;    //sign or write don't match

//if (t->whole && ch->whole)
 // {
 //   if (t->fend > ch->fend) ch->pbits = 1;
 //   else t->pbits = 1;
//  }

}
*/





uint check_sym_overlaps(SYM* xnew, SYM* t)
{
   // t is chained, xnew is new insert
   // bit fields can overlap so this is a duplicate check
   // followed by a range check

   chsym.lasterr = 0;           // no error yet

   if (t->addr != xnew->addr)  return 0;                     // address, OK

   if ((t->fend & 0xe0) != (xnew->fend & 0xe0)) return 0;    // write, sign, nobit

   // fields can overlap but not be identical

   if (xnew->fstart != t->fstart) return 0;
   if (xnew->fend   != t->fend)   return 0;


// so this is a duplicate sym if it gets here, do range check

   if (!xnew->rstart && !t->rstart && xnew->rend == 0xfffff && t->rend == 0xfffff)
    {
     chsym.lasterr = E_DUPL;     // No ranges for either sym
     return E_DUPL;              // this is a duplicate
    }

   if (xnew->rstart < t->rstart && xnew->rend > t->rend) return 0;         // spans.
   if (xnew->rstart > t->rstart && xnew->rend < t->rend) return 0;         // contained
   if (xnew->rend   < t->rstart) return 0;                            // before
   if (xnew->rstart > t->rend)   return 0;                            // after

   chsym.lasterr = E_OVRG;                          // set range overlaps
  return E_OVRG;

}




uint fmask(uint fstart, uint fend)
{
   uint ans, i;
   ans = 0;

   fend &= 0x1f;                       // safety
   for (i = fstart; i <= fend; i++)
     {
       ans |= (1 << i);
     }
   return ans;
}




uint olpadd(uint add)

{
    if (!(8065))     return 0;    //only 8065 has overlap SFRs
    if (add > 0x1c)  return 0;
    if (add < 0x6)   return 0;   // shortcuts

    if (add == 0x6)  return 1;
    if (add == 0xE)  return 1;
    if (add == 0x10) return 1;
    if (add == 0x12) return 1;
    if (add == 0x14) return 1;
    if (add == 0x16) return 1;
    if (add == 0x18) return 1;
    if (add == 0x1c) return 1;
    return 0;

}




void fixbitsym(SYM *s, uint add, uint fstart, uint fend)
{
 // uint lfend;

// move start to correct byte address for syms
// fend is field end plus sign (0x20) plus write (+x40)
// and extra flags upwards, WHL,SYS, CMD.
// CHANGE RULES for byte defaults
//map into byte addressed unless it's an overlapped SFR

  if (fend & 0x60)  s->pbkt   = 1;       // sign or write set
  if (fend & C_USR) s->usrcmd = 1;       // by user command
  if (fend & C_SYS) s->sys    = 1;       // system generated


//if whole ??    7,15,23,31 ?   0x7,f, 17, 1f set NBT ?

  if (!fstart && (fend & 7) == 7) s->fend |= C_WHL;

  if (s->fend & C_WHL)

    {                        // no bits flag
     s->fstart = 0;          // force zero start
     s->fend  = fend ;      // 0-7, with flags.
     s->addr   = add;
     return;
    }

  s->pbkt = 1;            //has bit field if it gets here

if (!olpadd(add))   fixbitfield(&add,&fstart, &fend);

  s->addr   = add;
  s->fstart = fstart;
  s->fend   = fend;
  s->fmask  = fmask(fstart, fend);         // set up field mask for later use

}




SYM *add_sym  (cchar *fnam, uint add, uint fstart, uint fend, uint rstart, uint rend)

 {
   // chain for symbols bitno = -1 for no bit
   // fend as OPS as field end plus sign (+32) plus write (+64)

   SYM *s, *t;
   int ans;
   uint ix;


   // if (anlpass >= ANLPRTget_anlpass() >= ANLPRT) return 0;

   if (!fnam || !*fnam) return 0;          // must have a symname.

   s = (SYM *) chmem(&chsym);

   //map into byte addressed unless it's an overlapped SFR
  // NB check is in fixbitsym

   fixbitsym (s, add, fstart, fend);

   s->rstart  = rstart;                    //range start and end
   s->rend    = rend;

   ix = bfindix(&chsym, s);

   ans = chsym.comp(&chsym, ix, s);              // zero if matches

   t = (SYM *) chsym.ptrs[ix];         // chain block found

// !ans is an exact duplicate, including addr,fstart,fend, start range, but not end range.


chsym.lasterr = 0;

    if (!ans)
      {       //duplicate

        #ifdef XDBGX
          t = (SYM*) chsym.ptrs[ix];
          DBGPRT(0,"Dup sym %x",add);
          DBGPRT(0," %c%s%c " ,'"',fnam, '"');

          if (fend & C_REN) DBGPRT(0, " ren"); else DBGPRT(0, "NO REN");
          DBGPRT(0," with ");
          DBGPRT(0," %c%s%c " ,'"',t->name, '"');
          DBGPRT(1,0);
        #endif

       s = (SYM*) chsym.ptrs[ix];
       chsym.lasterr = E_DUPL;
       if (fend & C_REN)  new_symname (s, fnam);     // allow name replace, will reset error
       return s;
     }

   if (ix < chsym.num)
     {

    //     error checking is not right !!
      check_sym_overlaps(s, t);      // sets last err if any overlaps.

   //   if (!chsym.lasterr) check_sym_range(s,t);  // OK if no range error

      if (chsym.lasterr)             // may need multiple syms checked ??
       {
          // x->lastix

       #ifdef XDBGX
        DBGPRT(0,"add sym %x",add);
        DBGPRT(0," %c%s%c " ,'"',fnam, '"');
        if (chsym.lasterr ==  E_OVRG)  DBGPRT(0,"range"); else DBGPRT(0,"field");
        DBGPRT(0," overlap %d with ", chsym.lasterr);
        DBGPRT(0," %c%s%c " ,'"',t->name, '"');
        DBGPRT(1,0);
        #endif
        return 0;
       }

    }

     // do insert
     new_symname (s, fnam);

//not right - if syms overlap, THEn check for
  //   mark_olap_syms(ix);

      if (!chsym.lasterr)  chinsert(&chsym, ix, s);



     #ifdef XDBGX
      DBGPRT(0,"add sym %x",add);
      if (fnam) DBGPRT(0," %c%s%c " ,'"',s->name, '"');
      if (s->rstart)      DBGPRT (0," %05x - %05x" , s->rstart, s->rend);
      DBGPRT (0," B%d %2x", s->fstart, s->fend);
      if (s->fend & 0x40)  DBGPRT(0," write");        //s->writ
      if (s->fend & 0x80)  DBGPRT(0," no_bit");
      if (s->pbkt) DBGPRT(0," pbkt");
      if (s->immok) DBGPRT(0," imm");
      if (s->usrcmd)   DBGPRT(0,"  USR");

      DBGPRT(1,0);
     #endif

 return s;

}




SYMLIST * get_symlist(uint add, uint fend, uint pc)
{
    // get ALL possible address matches.
    // fstart used for field mode, fend used against access size

    // list [0] is read default, [1] is write

 // don't need list, just have r/w defaults and start and end.

  SYMLIST *list;
  SYM *s, *t;
  uint ix, end;

  list = (SYMLIST*) mem (0, 0, sizeof(SYMLIST));
  list->defwr = chsym.num+1;
  list->defrd = chsym.num+1;
  list->startix = chsym.num+1;
  list->endix   = chsym.num+1;

  list->fend = fend;

  list->bcnt = 0;                // count of bitfield syms found
  list->wcnt = 0;                // count of 'whole' syms found

 // add = fix_bare_addr(add);         //  for bank, not bit



if  (olpadd(add))  end = add+1;
else  end = add + bytes(fend);          //  end address

  s = (SYM*) chmem(&chsym);         // block for search

  s->addr   = add;
  s->fend   = 0xff;                  // largest possible fend (31 + write + sign + nob)
  s->rstart = 0;
  s->rend   = 0xfffff;                   // range required for binary search..........

  ix = bfindix(&chsym, s);

  list->startix = ix;              //first matching entry

  while (ix < chsym.num)
    {

     t = (SYM*) chsym.ptrs[ix];        // first entry found

     if (t->addr >= end)
        {
         break;          //return list;             // address not match, or end of chain
        }

     if ((fend & C_WHL) && !(t->fend && C_WHL)) break;    //no bit fields allowed.

     if (pc >= t->rstart && pc <= t->rend)
       {  // sym range covers pc, deflt read in [0] , deflt write in [1]

           // in order in list -
           //'whole' write,  whole read,
           // largest field write (from zero) , then read
           // start bit write, read


           if (t->fend & C_WHL)
             {  // whole symbol
                list->wcnt++;           //whole sym found
                if (t->fend & C_WRT)
                   {     // write
                     if (list->defwr > chsym.num) list->defwr = ix;
                   }
                else
                  {     // read
                     if (list->defrd > chsym.num) list->defrd = ix;
                  }
             }
           else  list->bcnt++;     //bitfield
       }

     list->endix = ix;
     ix++;

    }

if (list->endix > chsym.num)
     {       //no matches
         mfree(list,sizeof(SYMLIST));
         return NULL;
     }
    // read exists but no write ?? assign write to read as default
    if (list->defwr > chsym.num && list->defrd < chsym.num) list->defwr = list->defrd;

  return list;
}


SYM* get_sym(uint add, uint fstart, uint fend, uint pc)
{
    uint i, ans;
    SYM *s;
    SYMLIST *list;

   ans = chsym.num;

//get_list must operate before fixbit if overlap - how to do this ??
//or is this OK, and it's only bitwise....

  if (!olpadd(add)) fixbitfield(&add, &fstart, &fend);     // adjust pars to byte based if not SFR or flagged

   list = get_symlist(add, fend, pc);

   if (!list) return NULL;

   // defaults in defwr, defrd, not set is >chsym.num

    i = list->startix;

    while (i <= list->endix)
      {
        s = (SYM*) chsym.ptrs[i];

          // range and field and address not just addr
          // writes always first, reads follow

        if (s->addr == add && pc >= s->rstart && pc <= s->rend &&
            s->fstart == fstart)
          {  //  matches so far.

//if (fend & 0x80)  whole only.........

            if (s->fend == fend)
              {   //write+sign+whole as exact match
               ans = i;
               break;         // perfect match first (write, sign)
              }

            if (fend & 0x40)
                { // match read to write if above fails and write selected.
                  // drop sign and write. (0x60)
                   if ((s->fend & 0x9f) == (fend & 0x9f) )
                     {
                       ans = i;
                       break;         // read for write
                     }

                }
          }
       i++;
      }

     if (ans >= chsym.num)
       {
          // no match if it got here, so use default
        if ((fend & 0x40) && list->defwr < chsym.num) ans = list->defwr;   // default write sym
        else if (list->defrd < chsym.num) ans = list->defrd;               // default sym (read or write)
       }


   if (ans < chsym.num) s = (SYM*) chsym.ptrs[ans];   else s = NULL;

   mfree(list,sizeof(SYMLIST));
   return s;
 }




/*
void scan_dc_olaps(void)
{
  // check for data commands inside code
  // could be an indexed offset ldx etc.
  // revise to use opdata

  uint ix,i, j, ofst, size;

  LBK *d;            //, *n;
  SBK *s, *p;

  if (!chscan.num) return;


  return;

  for (i = 0; i < chdata.num; i++)
   {
     d = (LBK *) chdata.ptrs[i];

     if (d->fcom > C_DFLT && d->fcom < C_ARGS)
      {         // real data pointer
        s = (SBK*) chmem(&chscan,1);              // dummy block for scan search
        s->start = d->start;              // addr
        ix = bfindix(&chscan, s);
        s = (SBK*) chscan.ptrs[ix];

        if (s->start <= d->start && s->nextaddr >= d->start )
          {     // scan before and after data start - assume spanned entirely by code
                // delete data
            #ifdef XDBGX
               LBK *x;
               DBGPRT(1,"DATA OVERLAP %d %x %x %s-> %x %x %s (from %x)",1, s->start, s->nextaddr, "SPAN", d->start, d->end, cmds[d->fcom], d->from);       //(from %x, %d) ,d->from, d->opcsub);
               x = (LBK *) chdata.ptrs[i];
               DBGPBK(1,x,"delete (%d)", i);
            #endif
            chdelete(&chdata,i,1);
            i--;
          }
        else
          {
           ofst = 0;
           size = 0;
           p = (SBK*) chscan.ptrs[ix-1];  // previous scan

           if (p->start <= d->start && p->nextaddr >= d->start )
            {   // previous scan olaps at end, but may have 'hole'
              #ifdef XDBGX
              DBGPRT(1,"DATA OVERLAP %d %x %x %s-> %x %x %s (from %x)", 2, p->start, p->nextaddr, "PREV", d->start, d->end, cmds[d->fcom],d->from);       //(from %x, %d), d->from, d->opcsub);
              #endif

             if ((p->nextaddr - d->start) < 16)
              {   // within 16 bytes of end




                for (j = 0; j < 64; j++)
                  {
                   if (1)       //(!get_opdata(d->start+j))
                     {      // gap found
                      if (!ofst) ofst = j;
                      size++;
                     }
                   else if (ofst) break;
                  }
              }

           if (ofst)
              {
               do_one_opcode(d->from);               // get details of opcode
               j = bytes(sinst.opr[1].rfend);        // size in bytes
               if (j > 1 && (ofst & 1))
                 { ofst++; size--;}  // can't have word etc on add bound.

               if (sinst.opcsub == 3 && size >= j)
                 {      //indexed, and will fit............
                  d->start += ofst;
                  d->end  += ofst;

                  if (((size/j)*j) == size)
                    {  //consistent with multiple entries...expand
                      d->end = d->start+size-1;
                      #ifdef XDBGX
                        DBGPRT(1,"move+expand data to %x-%x ", d->start, d->end);
                      #endif
                    }
                  #ifdef XDBGX
                    else
                    DBGPRT(1,"move data to %x-%x", d->start, d->end);
                  #endif
          //        set_opdatar(d->start, d->end);     // and mark it

//but should now check if this has more data after it................2a36 A9L




                 }
               }
           else
               {
                #ifdef XDBGX
                  LBK *x;
                  x = (LBK *) chdata.ptrs[i];
                  DBGPBK(1,x,"delete (%d)", i);
                #endif
                chdelete(&chdata,i,1);
                i--;
               }
           }
        }
     }
  }
}


*/

void scan_sgap(uint addr)
 {
  // add a scan for the detected gap, and add_scan keeps
  // scans also in extra chain for tracking

  SBK *s, *x;
  uint ix, num;

  // check not a 'fill' gap first

  if (check_sfill(addr))
  {
    #ifdef XDBGX
     DBGPRT(1,"Ignore gscan for %x, fails rpt check", addr);
    #endif
    return;
  }

  x = add_scan(addr, J_STAT | C_GAP,0);
  if (!x) return;
//  if (chscan.lasterr) return;

 // x->gapscan = 1;              //set here for
  scan_blk(x, &cinst);

  if (!x->inv)
  {
  // this may have added new scans, will be in gap chain

//  wnprt(1,"# scan gap at %x as code",cmdaddr(addr));
  ix = 0;

  num = chsgap.num;

  while (ix < chsgap.num)
   {
    s = (SBK*) chsgap.ptrs[ix];
    if (!s->stop && !s->inv && s->scnt < 10)
      {
        #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(0,"Sgap Scan (%d) ", ix);
        #endif
          scan_blk(s, &cinst);
          if (chsgap.num != num)
            {
             #ifdef XDBGX
             DBGPRT(1,"NEW SCANS %d -> %d", num, chsbcn.num);
             #endif
             ix = -1;   // rescan if changed
             num = chsgap.num;
            }
         }
    ix++;
   }
  }

 }


uint get_jump(CHAIN *x, uint addr)
{
  JMP *j;
  j = (JMP*) chmem(&chjpf);

  if (x == &chjpf)  j->fromaddr = addr;
  else j->toaddr = addr;

  return bfindix(x, j);
}


JMP * get_fjump(uint addr)
{
  JMP *j;
  uint ix;

  j = (JMP*) chmem(&chjpf);
  j->fromaddr = addr;

  ix = bfindix(&chjpf, j);


  if (chjpf.comp(&chjpf,ix,j)) return NULL;    // no match
  return (JMP*) chjpf.ptrs[ix];
}










JMP * add_jump (SBK *s, int to, int type)
{
  JMP *j;
  OPC *x;
  uint ix;
int ans;

  if (anlpass >= ANLPRT) return NULL;

  if (!val_rom_addr(to))
    {
     #ifdef XDBGX
       ix  = (nobank(to));   // don't want calcons
       if (ix >= 0xd000 && ix <= 0xd010) return NULL;
       if (ix == 0x1f1c || ix == 0x1000 || ix == 0x1800) return NULL;
       DBGPRT(1,"Invalid jump to %x from %x", to, s->curaddr);
     #endif
     return NULL;
    }

  j = (JMP *) chmem(&chjpf);
  j->toaddr     = to;
  j->fromaddr   = s->curaddr;
  j->jtype  = type;
  j->size   = s->nextaddr-s->curaddr;   // excludes compare

  // is this preceeded by a compare ?
  // get size of cmp as well

  ix = find_opcode(0, s->curaddr, &x);
  if (x && x->sigix == 10)  j->cmpcnt = s->curaddr - ix;   // count back for cmp


  if (!bankeq(j->toaddr, j->fromaddr))  j->bswp = 1;        // bank change
  else
  if (j->jtype && j->fromaddr >= j->toaddr)
     {
      j->back = 1;
     }

  // insert into the two jump chains

  ix = bfindix(&chjpf, j);      // FROM chain
  ans = chjpf.comp(&chjpf, ix, j);

  if (ans)
    {
     chinsert(&chjpf, ix, j);                    // do insert in FROM chain
     ix = bfindix(&chjpt, j);                 // and in TO chain
     chinsert(&chjpt, ix, j);
    }
  else  j =  (JMP *) chjpf.ptrs[ix];             // match - duplicate

//  check_jump_span(j);

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT (0," jump %x-%x %s", j->fromaddr, j->toaddr, jtxt[j->jtype]);
    if (j->retn)  DBGPRT(0," ret");
    DBGPRT(1,0);
   #endif

  if (!ans) return 0;             // for detection of duplicate (sjsubr)
  return j;
}










uint get_tjmp_ix(uint ofst)
{
  // find to-jump - as this is multiple,
  // bfindix (via cpjump) will find first one

  JMP * j;
  uint ix;

  if (!chjpt.num) return 1;

  j = (JMP*) chmem(&chjpf);
  j->toaddr = ofst;
  ix = bfindix(&chjpt, j);

  // bfindix WILL find first to jump (code in cpjmp)

  if (ix >= chjpt.num) return ix;

  j = (JMP*) chjpt.ptrs[ix];

  if (j->toaddr != ofst) return chjpt.num;

  return ix;
}

SIG * get_sig (uint addr)
{
  SIG *t, *s;
  uint ix;

  t = (SIG*) chmem(&chsig);
  t->start = addr;

  ix = bfindix(&chsig, t);

 if (ix >= chsig.num) return  NULL;    // no match
  //check it's right address ....
  s = (SIG*) chsig.ptrs[ix];

  if (s->start != addr) return NULL;


  return s;


//  if (eqsig(&chsig,ix,t)) return  NULL;    // no match
//  return (SIG*) chsig.ptrs[ix];

}

LBK *get_prt_cmd(LBK *dflt, BANK *b, uint ofst)
  {
    // find next command for printout from cmd either code or data chain
    // if none found use a default (fill) block
    // code always overrides data

    LBK *c  ;      //, *d;
    int ixc ;      //, ixd;

    dflt->start = ofst;
    dflt->end = 0;
    dflt->dflt = 1;

    ixc = bfindix(&chcmd,dflt);             // search for code entry first
    c = (LBK*) chcmd.ptrs[ixc];

    if (!eqcmd(&chcmd,ixc,dflt)) return c;    // match

  //  ixd = bfindix(&chdata,dflt);             // search for data entry
  //  d = (LBK*) chdata.ptrs[ixd];

    //check code block in case found data block overlaps it
  //  if (c->start < d->end && c->start >= d->start && c->fcom == C_CODE)
  //      d->end = c->start-1;

  //  if (!eqcmd(&chdata,ixd,dflt)) return d;    // match

    // not found either one, look for next blocks to get an end address

    dflt->end = b->maxromadd;

    if (dflt->end > c->start && ofst < c->start)  dflt->end = (c->start-1);       // c is valid code block

   // if (d->start < dflt->end && ofst < d->start)  dflt->end = (d->start-1);

    dflt->fcom = C_DFLT;

    if (dflt->end < dflt->start && dflt->end < b->maxromadd)
     {
       #ifdef XDBGX
        DBGPRT(0,"PANIC for %x  %x!!! ", dflt->start, dflt->end);
        DBGPBK(0,c,"CD");
     //   DBGPBK(1,d,"DAT");
       #endif
       dflt->end = dflt->start+1;
     }

return dflt;

}

int find_tjump (uint ofst)
{
  // find trailing jump, for brackets, matches fjump
  // prints close bracket for each jump found

 JMP * j;
 uint ix, ans;

 if (anlpass < ANLPRT) return 0;
 if (!PSSRC) return 0;

 ans = 0;
 ix = get_tjmp_ix(ofst);   // find first 'to' jump

 while (ix < chjpt.num)
  {
   j = (JMP*) chjpt.ptrs[ix];
   if (j->toaddr == ofst)
    {
     if (j->cbkt) ans++;
     ix++;
    }
   else break;    //stop as soon as different address
  }
  return ans;
}


JMP * find_fjump (uint ofst, int *x)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 8 for 'else', 4 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JMP *j;

  if (anlpass < ANLPRT) return 0;

  *x = 0;

  j = get_fjump(ofst);

  if (!j) return 0;


  if (!j->jtype)  *x = 0;   // true return

  if (j->retn)  *x = 2;      // jump to a return

//  if (j->jtype == J_ELSE) *x = 4;
  if (j->obkt) *x = 1;     // bracket - reverse condition in source

//if (j->jelse) *x |= 4;          //pstr(" } else { ");
//if (j->jor)   pstr(0," ** OR **");

  return j;
}

/*
old original
  ix = bfindix(&cmdch, ofst, 0);            // index for command block
       x = (LBK*) cmdch.ptrs[ix];
                                                      // if (eq(&cmdch,ix,ofst,0))
       if (ix >= cmdch.num || cmdch.eq(&cmdch,ix,ofst,0))    // was x
         {
         x = (LBK*) chmem(&cmdch);   // NOT found, set up a dummy block
         clrblk(x,sizeof(LBK));      //clear  block
         x->start = ofst;
         x->end   = end;   // safety catch
         x->fcom  = C_DFLT;

         if (ix < cmdch.num)
             {                     // set end up to next block
              y = (LBK*) cmdch.ptrs[ix];
              if (x->end >= y->start) x->end = y->start -1;
             }
         ix = cmdch.num;
         }













LBK *get_prt_cmd(LBK *dflt, BANK *b, uint ofst)
  {
    // find next command for printout from cmd either code or data chain
    // if none found use a default (fill) block
    // code always overrides data

    LBK *c, *d;
    int ixc, ixd;

    dflt->start = ofst;
    dflt->end = 0;
    dflt->dflt = 1;

    ixc = bfindix(&chcode,dflt);             // search for code entry first
    c = (LBK*) chcode.ptrs[ixc];

    if (!eqcmd(&chcode,ixc,dflt) && c->fcom != C_XCODE) return c;    // match

    ixd = bfindix(&chdata,dflt);             // search for data entry
    d = (LBK*) chdata.ptrs[ixd];

    //check code block in case found data block overlaps it
    if (c->start < d->end && c->start >= d->start && c->fcom == C_CODE)
        d->end = c->start-1;

    if (!eqcmd(&chdata,ixd,dflt)) return d;    // match

    // not found either one, look for next blocks to get an end address

    dflt->end = b->maxbk;

    if (c->fcom != C_XCODE && dflt->end > c->start && ofst < c->start)  dflt->end = (c->start-1);  // c is valid code block

    if (d->start < dflt->end && ofst < d->start)  dflt->end = (d->start-1);

    dflt->fcom = C_DFLT;

    if (dflt->end < dflt->start && dflt->end < b->maxbk)
     {
       #ifdef XDBGX
        DBGPRT(0,"PANIC for %x  %x!!! ", dflt->start, dflt->end);
        DBGPBK(0,c,"CD");
        DBGPBK(1,d,"DAT");
       #endif
       dflt->end = dflt->start+1;
     }

return dflt;

}


void scan_loopdata(void)
{
  // try to find data structs from access loops...........
  // may add more later on ...... simple first.

  uint ix,jx,sx;         //,i;
  uint  tinc, reg, ofst, xofst;
//  uint tstart, tend, fstart, fend;    // have to use indexes here as interleaved scan
//  uint tend;
  JMP *j;
  OPC *x;
  DTK *k;
  SBK *emu;

//loop pass, for lists of things.
  // first, check for jumps to return opcodes
  // true ret has jtype 0


       #ifdef XDBGX
         DBGPRT(2,"\n\n ************** SCAN LOOPDATA");
       #endif

  for (jx = 0; jx < chjpf.num; jx++)
    {         // down list of forward jumps
     j = (JMP*) chjpf.ptrs[jx];

  //   if (j->back && !j->retn && !j->subloop && (j->jtype == J_STAT || j->jtype == J_COND))
if (j->back && !j->retn && (j->jtype == J_STAT || j->jtype == J_COND))
      {
        // backwards jump, check if this is a loop.......

//   data tracking
       k = (DTK*) chmem(&chdtk,0);
       k->ofst = j->toaddr;
       sx = bfindix(&chdtk, k);         //keep start in sx

       ix = sx;
       tinc = 0;
       reg = 0;

       while (ix < chdtk.num)
        {
         k = (DTK*) chdtk.ptrs[ix];

         if (k->ofst >= j->fromaddr) break;

         if (k->ainc)
            {
             if (!reg) reg = k->reg1;
             if (k->reg1 == reg)  tinc += k->ainc;
            }

   //      #ifdef XDBGX
    //     DBGPRT(1,"found %x", k->ofst);
   //      #endif
         ix++;

        }
      if (tinc)
        {     //need register

       #ifdef XDBGX
         DBGPRT(0,"\nBJUMP %x-%x", j->fromaddr,j->toaddr);
       #endif

         #ifdef XDBGX
         DBGPRT(1," found INC %d for R%x", tinc, reg);
         #endif

// find orig ldx for register first................


     ofst  = j->toaddr;
     xofst = j->toaddr;         //ofst;

  //   ofst = find_opcode(0, j->toaddr, &x);    //backwards

     while (xofst)
       {
        xofst = find_opcode(0,xofst, &x);    //backwards
        if (!xofst || x->sigix != 12) break;
        ofst = xofst;
       }

      if (ofst) {             //break;  //safety
// emulate loop here ??  would need to import a new scan ??

       emu = add_escan(ofst, 0);
       emu->lscan = 1;               // mark loop scan

   //    emu->logdata = 1;

       scan_loop(emu, &cinst, j->fromaddr);

}
else {
   #ifdef XDBGX
         DBGPRT(1," ofst = 0");
         #endif
}
        }




      }






    }


//pass for [Rx+offset]  items


       #ifdef XDBGX
         DBGPRT(2,"\n\n ************** END LPDATA");
       #endif


 }

   //  OPCODE chaser
      ofst = find_opcode(0, j->toaddr, &x);
  while (ofst && ofst < j->fromaddr)
         {
          ofst = find_opcode(1, ofst, &x);

//if (ofst == 0x823b6)
//{
//DBGPRT(1,0);
//}
void *find(CHAIN *x, void *b)
{
 // find item with optional extra pars in b

 uint ix;
 ix = bfindix(x, b, 0);
 if (x->equal(x,ix,b)) return NULL;    // no match
 return x->ptrs[ix];
}

  x = (DTK*) chmem(&chdtk);
  x->ofst = ofst;
//  r->start = pc;

  x = (DTK*) find(&chdtk, x);




          if (ofst)
            {
              do_one_opcode(ofst);
            }


       #ifdef XDBGX
         DBGPRT(0,"BLoop %x", ofst);
         if (!ofst) DBGPRT(1,0);
         else
           {
            DBGPRT(0," (%s) %d", x->name, sinst.opcsub);
            for (i = 0; i <= sinst.numops; i++)
              {
               DBGPRT(0, "R%x," , sinst.opr[i].addr);
               }
            DBGPRT(1,0);
           }
       #endif

         }
*/

// DTK chaser..................


// init block here
//  #ifdef XDBGX
 //        DBGPRT(1,"BLoop %x", j->toaddr);
 // #endif





/*
void scan_code_gaps(void)          //TEMP !!
{
  uint ix, jx;
  LBK *b, *c;
  SBK *s, *n;

  // when called, code blocks yet, only scans
  // use scans for this  still experimental !!
  // scan -> scan gaps only

  //b = (LBK*) chmem(&chdata,0);

  ix = 0;
  while (ix < chscan.num)
  {
    s = (SBK*) chscan.ptrs[ix];
    if (s->inv) chdelete(&chscan,ix,1);  // drop invalids
    else ix++;
  }

  ix = 0;
  while (ix < chscan.num)
   {
    s = (SBK*) chscan.ptrs[ix];

    if (ix+1 >= chscan.num) break;

    n = (SBK*) chscan.ptrs[ix+1]; // next scan block

    if (nobank(s->start) <= 0x201e) { ix++; continue;}

    // scan blocks can overlap, so make sure it's a true gap

    if ((n->start - s->nextaddr) > 1)
      {
       b->start = s->nextaddr;      // find nearest data block to end of s
       jx = bfindix(&chdata, b);   //   all together now in cmdch............

       if (jx < chdata.num)
         {       // a valid block
           c = (LBK*) chdata.ptrs[jx];
           if (c->start > n->start)
            {
             // and should check it's not XCODE !!
             // code gap with no data in it
             #ifdef XDBGX
              DBGPRT(1,"************************ Code GAP %x-%x", s->nextaddr+1, n->start-1); // not if found something before it.
             #endif
      //       scan_sgap(s->nextaddr+1);
            }
         }
      }
     ix++;
   }

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(1,"- - -END Code gap Scan");
  #endif

 } // end func

*/

/*

void sniff_func(uint start, uint end)
{
// see if we can find a func in this block .

// just do start addr first...................
int sval, val, size, rsize, sz;
uint addr;

size = 0;

sval = g_word(start);

if (sval == 0xffff) size = 2;
else
if (sval == 0x7fff) size = 10;      //signed word
else
 {
   sval = g_byte(start);
   if (sval == 0xff) size = 1;
   else
   if (sval == 0x7f) size = 9;
}

if (size <= 0) return;

sz = get_signmask(size);


rsize = (size & 7) * 2;             // row size of function

sval = g_val(start,0,size);          // signed value

addr = start+rsize;          // 2nd row

while (addr < end)
{
// fix auto sizes later !
 val = g_val(addr,0, size);
 if (val > sval) break;
 sval = val;
 if (val == sz) break;          //cnv[size].signmask) break;
//but check continued ends here ?? or not ??
 addr += rsize;
}

if (val != sz) return;       //cnv[size].signmask) return;

// passed first test.
#ifdef XDBGX
DBGPRT(1,"found func size %d", size);
#endif
}



//establish a median/mean value first ???
//use preset patterns ??   No use for init lists or tables ??

*/
// int  rscore[16][32];                  // row, gap, can be negative
 int score [16];
 uint mtype[32];                      // temp for now whilst testing
 int  vald   [32];               // value difference

uint do_patt_template(uint start, uint rowsize)
 {
   uint i, addr;
   int a,b;

   for (i = 0; i < rowsize; i++)
     {
       // establish base set of match types from first two rows of elements
       // of possible structure.

      addr = start+i;
      a = g_val(addr,0,7);
      b = g_val(addr+rowsize,0,7);
   //   c = g_val(addr+(gap*2), 0,7);

      vald[i] = a-b;                             // set value difference.
      mtype[i] = 0;

      if (a == b)  mtype[i] |= 1;      //         && a == c)          // match

      //  else if ((c-b) == (b-a))  mtype[i] |= 32;       //value difference match

      if (__builtin_popcount(a) == 1 && __builtin_popcount(b) == 1 ) mtype[i] |= 2;  // && __builtin_popcount(c) == 1 bit mask positive
      if (__builtin_popcount(a) == 7 && __builtin_popcount(b) == 7 ) mtype[i] |= 16;  // && __builtin_popcount(c) == 7 bit mask negative
      if (a < b )  mtype [i] |= 4;        // && b < c increment
      if (a > b )  mtype [i] |= 8;        // && b > c decrement
    }


return 1;

}


int score_row_match(uint *start, uint rowsize)
  {
    uint i, p, addr;
    uint m;               //matches, for debug

    int a,b, score;

// perhaps this should be DOWNWARDS ??

       for (i = 0; i < rowsize; i++)       // compare this row to the 'base'
          {
              //what about words ??
           addr = *start + i;
           a = g_val(addr,0,7);           // or should this be against FIRST row ???
           b = g_val(addr+rowsize,0,7);

           m = 0;
           score = -2 * rowsize;          // negative by no of elements

        //   if (mtype[i] & 32)

           if ((a-b) == vald[i])  {score += 5;  m++; }       //  matched by value difference

           if ((mtype[i] & 1) && a == b)  {score += 5;  m++; }       // match by value

           if ((mtype[i] & 4) && a < b )  {score += 2;   m++; }     // increment

           if ((mtype[i] & 8) && a > b )  {score += 2;   m++; }     // decrement

           if (a != b)
            {
              p = __builtin_popcount(b);
              if ((mtype[i] & 2)  && p == 1) {score += 5;  m++; }   // bit mask +ve
              if ((mtype[i] & 16) && p == 7) {score += 5;   m++; }  // bit maks -ve
            }
//others ??    Word ?? constant difference ?? all valid

          }


     #ifdef XDBGX
//       if (score >0)
 DBGPRT(1,"sz %d, score %d m %d (%x-%x)", rowsize, score, m, *start, *start+rowsize);
     #endif

*start = addr+rowsize ;
  return score;

}







uint do_data_patt (uint *start, uint *end)    //uint gap
{
  uint addr, rowsize, rownum;
  uint highest, ansrow;

// int tscore[32];                  // can be negative -total struct score

//  uint mtype[32];                  // temp for now whilst testing
//  int vl   [32];               // value difference
//  int val, a, b, c;

 #ifdef XDBGX
 DBGPRT (1, "start patt analysis for %x-%x", *start,*end);
 #endif



/*copy table size trick........................

// can't do this as may be several structs in here - may or may not be imd...........

// end not necessarily reliable either..............

 size = end - start + 1;      // max size
 term = 0;                   // no terminator

      val = g_byte(end);
       if (val == 0xff)
         {  // could be terminator
           term = 1;
           #ifdef XDBGX
            DBGPRT(1,"possible terminator");
           #endif
}

 tries = 0;
  for (i = 2; i < 32; i++)
    {
      val = size/i;
      if (val*i == size) tries |= (1<<i);
      if (term && val*i == size-1) tries |= (1<<i);                      // add suggested col
    }


//all byte ??   this needs expansion to words.

 memset(rscore,0, sizeof(rscore));
// memset(tscore,0, sizeof(tscore));

*/

//   if ( (tries >> gap) & 1)  temp ignore...................

 // try -gap ??? (2286 A9L ??


highest = *start + 1;

for (rowsize = 2; rowsize < 32; rowsize++)
{

//if (rowsize == 16 && *start == 0x92286)
//{
//DBGPRT(1,0);
//}




   do_patt_template(*start,rowsize);

   for (rownum = 0;  rownum < rowsize; rownum++)
   {
    #ifdef XDBGX
   DBGPRT(0," %x", mtype[rownum]);
   #endif
   }
   #ifdef XDBGX
 DBGPRT (1,0);
 #endif

   addr = *start;
   rownum = 0;

//if rowsize vs size check to go here



while (rownum < 16)
  {
   addr = *start+(rowsize*rownum);              //no point scoring first row.......

 //  DBGPRT(0,"rownum = %d size %d start %x ", rownum, rowsize, addr);
   score[rownum] = score_row_match(&addr, rowsize);

  // if (score[rownum] > 0)   DBGPRT(1,"OK %d", score[rownum]); else
  // DBGPRT(1,"ABANDON %d", score[rownum]);

 if (score[rownum] <= 0) break;
   rownum++;

// check addr if highest   match for end address




addr += rowsize;

  if (addr > *end)
     {
           #ifdef XDBGX
      DBGPRT(1,"beyond end");
      #endif
      addr = 0;
      break;
     }




if (addr > highest)
  {
   highest = addr;
   ansrow = rowsize;
     #ifdef XDBGX
   DBGPRT(1,"highest addr = %x for size %x",highest, ansrow);
   #endif
  }

}

}

if (highest == *end) {
      #ifdef XDBGX
    DBGPRT (1, "matches end ! size %d" ,ansrow );
    #endif
    highest = 0;}

if (highest == *end-1)
  {
    addr = g_byte(highest+1);
    if (addr == 0xff)

 {
       #ifdef XDBGX
       DBGPRT (1, "matches end with term size %d", ansrow); ansrow |= 0x1000;
       #endif
        highest = 0;}
  }



//if (match) break;    //break from above....



if (highest)

{ /// no perfect match
   // ansrow has best fit     = rowsize;
  *end = highest;

}


return ansrow;

}

/*wrong way around..................

 for (row = 3; row < 16; row ++)
   {



   addr = start + (gap * row);  // 16 rows ??          // 512 bytes max

   if (addr > maxpcadd(start)) break;

//   for (j = start+gap; j < term; j+=gap)      // (start+127)-gap; j+=gap)       // row by row until 128 max not big enough !!
  //   {
    //    if (j > term) break;

        for (i = 0; i < gap; i++)       // compare this row to the 'base'
          {
              //what about words ??
           addr = start + i;
           a = g_val(addr,0,7);
           b = g_val(addr+gap,0,7);

           if (mtype[i] & 32)
             {
               val = a-b;                                         // value difference.
               if (val == vl[i])  rscore[row][gap] += 5;   else rscore[row][gap] -= 5;       //  matched by value difference
             }

           if (mtype[i] & 1)
             {
               if (a == b)  rscore[row][gap] += 5; else rscore[row][gap] -= 5;        // match by value
             }
           if (mtype[i] & 4)
             {
              if (a < b)  rscore[row][gap] += 2; else rscore[row][gap] -= 2;       // increment
             }
           if (mtype[i] & 8)
           {
           if (a > b)  rscore[row][gap] += 2; else rscore[row][gap] -= 2;       // decrement
           }
           if (mtype[i] & 2)
           {            //bit mask, drop if any fails
            val = __builtin_popcount(b);
     //       if (val != 1) mtype[i] &= 0xfd;         //from type 2
//            else
 if (a != b && __builtin_popcount(a) == 1) rscore[row][gap] += 3; else rscore[row][gap] -= 3;
           }

           if (mtype[i] & 16)
           {
 //           val = __builtin_popcount(b);
   //         if (val != 7) mtype[i] &= 0xef;         //drop type 16          //bit mask
     //       else
             if (a != b && __builtin_popcount(a) == 7) rscore[row][gap] += 3; else rscore[row][gap] -= 3;
           }
//others ??    Word ?? constant difference ?? all valid

          }

       #ifdef XDBGX
       DBGPRT(1,"row %d gap %d, score %d", row, gap, score);
       #endif
}

} //end master for gap

//table divedes by number of ITEMS

 val = 0;                         // use for highest score
 gap = 0;                          // where it is
 row = 0;
 for (a=3; a < 16; a++)
 {
 for (j=1; j < 32; j++)
  {
   b = a*j;

rscore[a][j] /= b;

   if (rscore[a][j] > val)
   { val = b;
      gap = j;
      row = a;
   }
 }
}
// terminator ??

 if (gap) { val = size/gap;
  if (val*gap == size-1) gap |= 0x1000;
}

#ifdef XDBGX
DBGPRT(1,"end analysis at %x", end+1);
DBGPRT(1,"highest score is %d for row %d gap %d", a, row, gap & 0xfff);
#endif
return 1;
}



DTK* get_bdtk(uint addr)
{
  DTK  *x;
  SBK  *t;
  uint ix;

  x = (DTK*) chmem(&chdtk);
  x->start = addr;

// temp change for nearest
 ix = bfindix(&chdtkd, x, 0);
// if (x->equal(x,ix,b)) return NULL;    // no match
 if (ix < chdtk.num)
  {
    x = (DTK*) chdtkd.ptrs[ix];
    DBGPRT(0," Next DTK for %x = %x R%x(%d)",addr, x->start, x->reg, ix);
    if (ix > 0)
     {
      ix--;
      x = (DTK*) chdtkd.ptrs[ix];
      DBGPRT(0," Prev DTK for %x = %x (%d)",addr, x->start, ix);
     }

// find scan
      // t = find_scan(addr);
  t = (SBK*) schmem();              // dummy block for search
  t->start = addr;                  // symbol addr

  ix = bfindix(&chscan, t, 0);

  if (ix > 0)
  {
  t = (SBK*) chscan.ptrs[ix-1];
  if (t) DBGPRT(0," Prev code for %x = %x-%x",addr, t->start,t->nextaddr, ix);   // not if within xcode area.............
}

   return (DTK*) chdtkd.ptrs[ix];
  }
return 0;

//  x = (DTK*) find(&chdtk, x);

return x;

}

*/


/*
uint find_imd_dtk(uint jx, uint forb)
 {
   uint ix, addr, ans;
   DTK *x;

   x = (DTK*) chdtkd.ptrs[jx];
   addr = x->start[0];
   ans = 0;

   if (forb) ix = jx+1; else ix = jx-1;

   while (ix < chdtkd.num)
      {
       x = (DTK*) chdtkd.ptrs[ix];

       if (forb)
          { if ((x->start[0] - addr) > 256) break;
          }
       else  if ((addr - x->start[0]) > 256) break;   // gap too big


       if (x->opcsub == 1)      //imd)
         {
          ans = x->start[0];
          break;
         }
        if (forb) ix++; else ix--;
       }

return ans;

}
*/

/*
void sniff_loop(uint ix)

{
  DTK *x;
  uint start, end;
  // and inc is set for x

  x = (DTK*) chdtkd.ptrs[ix];
  //DBGPRT(1," assume loop ");

  start = find_imd_dtk(ix, 0);       // backwards
  end   = find_imd_dtk(ix, 1);       // forwards

 // DBGPRT(0,"IMD at %x (%x) %x", start, x->start, end);

  // now (hopefully have limits of data struct (may not be right...)

//  DBGPRT(0," ofst %x ", x->ofst);

  /// roll back for ofst ???




}

*/




void add_unique(uint *x, uint val, uint max)
 {
   uint j;
   // count in x[0];
   max--;    //drop count in [0] from size

   for (j = 1; j <= x[0]; j++) if (val == x[j]) break;

   if (j > x[0] && j < max)
       {
        x[0]++;
        x[x[0]] = val;

       }
}

/*   imd is NOT a guaranteed start for a struct.......need to know if
it's also accessed by an inr or inx, which IS a valid access....
2284 in A9L ...
ad2w imm 2680, 2c56
push inr  2c5a              **THIS confirms....***

2286 has only an IMD and no adjacent following inx so no confirm
2296 has imd + inx confirm but dodgy but following have a G8 ...

a G is ALWAYS a multiple, so *MUST* be a reliable indicator of size ???
even for inx as well as inr ????

22a6 has imd and following adj inx but also has G12 and G22 markers....
*/




void discover_struct(uint start, uint end)
 {
 //  TRK *k;
   DTD *d, *s;
 //  JMP *j;
   LBK *x;
   ADT *a;
   uint ix, jx, kx, row, gap, type;        //term, gap;    //, stype;
 //  uint i,j;

   #ifdef XDBGX
    DBGPRT(1," in discover");
   #endif

  //  szes[0] = 0;          // counter

   // look for nearest track (stdat) item

   get_dtkd(&chdtkd,0, start);
   kx = chdtkd.lastix;

   if (kx >= chdtkd.num) return;        //not found

   s = (DTD *) chdtkd.ptrs[kx];

   jx = chdtkd.lastix+1;
   while (jx < chdtkd.num)
    {
           d = (DTD *) chdtkd.ptrs[jx];
           if (d->stdat > end) break;
           if (d->opcsub == 1)   // d->modes & 2)
             {        //immediate
              add_unique(adds,d->stdat,NC(adds));
             }
           jx++;
       }


// debug
 #ifdef XDBGX
        DBGPRT(1,"start = %x", start);
        #endif

          jx = 1;

          while (jx <= adds[0])
           {
               #ifdef XDBGX
            DBGPRT(1," imd = %x", adds[jx]);
            #endif
            jx++;
           }
#ifdef XDBGX
        DBGPRT(1,"possible end = %x", end);
        #endif



// end of temp debug

  //  k = get_dtk(s->fk);            //for debug print

    jx = 1;
         while (jx <= adds[0])
           {
            if (adds[jx] > start && adds[jx] < end)
              {
               end = adds[jx]-1;
               #ifdef XDBGX
               DBGPRT(1,"next imd = %x", adds[jx]);
               #endif
               break;
              }
            jx++;
           }

gap = 0;
type = 0;
   jx = chdtkd.lastix+1;

   memset(szes,0,sizeof(szes));

   if (s->opcsub > 1)   szes[0] = s->bsze;

   while (jx < chdtkd.num)
    { // get everything between start and end
        // and try to make sense out of it..............
      d = (DTD *) chdtkd.ptrs[jx];
      if (d->stdat > end) break;        // finished, maybe

#ifdef XDBGX
      DBGPRT(1,"%x from %x,%d, %d, %d) ", d->stdat, d->ofst, d->opcsub, d->bsze, d->gap);
      #endif
      if (d->opcsub == 2)      // d->modes & 4)
        { // indirect, check gap
          if (d->gap ) gap |= (1 << d->gap);

   //       add_unique(szes,d->bsze,NC(szes));
          if (d->inc) type = 2;             //probably a loop
        }

    //  if (d->opcsub == 3)        // d->modes & 8)
        {        //immediate
   //      add_unique(szes,d->bsze,NC(szes));

           row =  d->stdat - start;
           if (start & 1)  row++;          //always keep row index even
           if (row < 32 && d->opcsub > 1)
             {
              if (d->bsze > szes[row])
                {
                  if ((row & 1) && szes[row-1] > 1) szes[row] = 0;
                  else szes[row] = d->bsze;
                }
             }

          type |= 1;
          if (d->gap ) gap |= (1 << d->gap);

        }

         jx++;
       }

// fix up sizes...............



#ifdef XDBGX
 DBGPRT(0,"sizes");
 for (row = 0; row < 32; row++)
   DBGPRT(0," %d",szes[row]);
 DBGPRT(1,0);




 DBGPRT(1,"gap = %x", gap);
 DBGPRT(1,"type = %d", type);
#endif

   if (end < start)  return;

//only if entry checks out as not being there...................

//   term = end - start + 1;

   if (end < (start+3))
    {         // could be a vect, or a word
       // if s-> mattches....
     add_cmd(start,end, s->bsze,0);
     return;
    }


         #ifdef XDBGX
 DBGPRT(1,"*** investigate %x to %x", start,end);
         #endif

   row = do_data_patt(&start,&end);         // likely struct size

  if (row & 0x1000) {kx = 1;} else kx = 0;
  row &= 0xfff;

  x = add_cmd(start,end,C_STCT,0);




  if (x)
    {
      a = append_adt(vconv(start),1);
     if (a)
      {
       a->fend = 7; //byte for now
       if (row > 2 && row < 32)
         {
          a->cnt = row;
          if (kx) { x->term = 1;
 #ifdef XDBGX
  DBGPRT(1,"with terminator");
  #endif
  }
         }
else
{
 #ifdef XDBGX
    DBGPRT(1,"reject rowsize = %x", row);
    #endif
}
    }


    }

  get_dtkd(&chdtkd,0, start); //may not match, use lastix for nearest after start
  ix = chdtkd.lastix;
  d = (DTD *) chdtkd.ptrs[ix];

  if (ix > chdtkd.num || d->stdat > end) return;          // no data, safety

 // k = get_dtk(d->fk);                // main entry

 // #ifdef XDBGX
 //   DBGPRT(1," earliest ref at %x", k->ofst);
//  #endif

// get all data inside start-end, or go to ofst based TRK ??
// multiple addresses and gaps ??

// jx = get_tjmp_ix(k->ofst + k->ocnt);

// if (jx < chjpt.num) j = (JMP*) chjpt.ptrs[jx]; else j = 0;


/*

         d = (DTD *) chdtkd.ptrs[ix];

         // if (3) then use indexed finder
         // if (1) use struct findeer ??

         if (kx < chdtkd.num)
           {     //get parent track entry
            k = get_dtk(d->fk);
            stype = d->opcsub;
            if (d->opcsub == 3)
             {
     //         if (k->off > start) start = k->off;   //k->base ??
              #ifdef XDBGX
               DBGPRT(1,"inx %x (%x,%d) from %x", start, d->fk, d->opcsub,k->ofst);
              #endif
             }
            else
             {
    //          if (d->stdat > start)     start = d->stdat;
              #ifdef XDBGX
               DBGPRT(1,"imd %x (%x,%d) from %x", start, d->fk, d->opcsub,k->ofst);
              #endif
             }
if (stype ==1)
{ // imd to imd - use data tracking to find struct




*/


}











uint turn_gapscans_into_code (void)
 {

  // turn gapscan list into code commands.
  // if any are invalid, ignore the whole lot

  uint ix, ans;
  SBK *s;

ans = 0;
  for (ix = 0; ix < chsgap.num; ix++)
   {
    s = (SBK*) chsgap.ptrs[ix];
    if (s->inv)
     {
  //       DBGPRT(1,"gapscans invalid");
      clearchain(&chsgap);       // clear all temp scan blocks, do nothing
      return 0;
     }
   }

// DBGPRT(1,"gapscans OK");
   for (ix = 0; ix < chsgap.num; ix++)
   {                                 //for all valid blocks completed
    s = (SBK*) chsgap.ptrs[ix];
    if (s && s->stop)
      {
       add_cmd (s->start,s->nextaddr,C_CODE,0);
       if (!chcmd.lasterr) ans = 1;
      }
   }            // for loop
 clearchain(&chsgap);       // clear all temp scan blocks
 return ans;
 }


void check_dtk_links(void)
{   //check links for push (and others?)
uint ix, jx;

DTD *d, *x;

for (ix = 0; ix  < chdtkd.num; ix++)
  {
    d = (DTD*) chdtkd.ptrs[ix];


//if (d->stdat == 0x9225f)
//{
//DBGPRT(1,0);
//}



    // d should be lowest ofst occurence of any new start addr.

    jx = ix+1;

    while (jx < chdtkd.num)
     {
      x = (DTD*) chdtkd.ptrs[jx];
      if (x->stdat != d->stdat) break;
  //    d->modes |= x->modes;            //get all access modes
      if (x->psh && x->bsze == 2) d->psh = 1;

      //immediate with confirmed inx or imd after it
      if (d->opcsub == 1 && x->opcsub > 1)  d->hq = 1;

   //   if (get_rbt(
      jx++;
     }

    // no previous instances

    jx = ix+1;

    while (jx < chdtkd.num)
     {
      x = (DTD*) chdtkd.ptrs[jx];
      if (x->stdat != d->stdat) break;
      if (d->psh && x->bsze == 2) x->psh = 1;
  //    x->modes = d->modes;
      jx++;
      }
    ix = jx-1;      // skip forward entries

}

}



uint do_cgap(uint start)
{
   #ifdef XDBGX
      DBGPRT(1,"Gap Code scan %x", start);
   #endif
   scan_sgap(start);
   if (turn_gapscans_into_code ())

   //but this skips some stuff, so must probably go back and refind the start point..........
   // dup scan is OK....but..........
  //   if (chscan.lasterr) return ix;
  {
   get_cmd(start,0);
   return 1;
  }
   return 0;
}











void scan_cmd_gaps(uint (*pgap) (LBK *s, LBK *n))
{
   // COMMAND CHAIN gaps.

  LBK *s, *n;
  uint ix;

 #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(2, "Gap analysis");
  #endif


 // while (ix < chcmd.num)

  for (ix = 0; ix < (chcmd.num-1); ix++)
   {
    s = (LBK*) chcmd.ptrs[ix];
    if (nobank(s->end) <= 0x201e) continue;

    n = (LBK*) chcmd.ptrs[ix+1]; // next scan block

    if ((n->start-1) < (s->end+1)) continue;

    if (g_bank(s->start) != g_bank(n->start) ) continue;       //safety

    #ifdef XDBGX
      DBGPRT(0,"\n\nGAP %x-%x", s->end-1, n->start-1);
      DBGPRT(1, "  %s <-> %s", cmds[s->fcom].str, cmds[n->fcom].str);
    #endif

    if (pgap (s,n)) ix--;    //ix -= pgap (s,n);         ??
   }

}



uint scan_cgap(LBK *s, LBK *n)
{
   LBK *cb;
   DTD *d;
   uint jx, start, end, val;  //, cnt;

   start = s->end+1;
   end  = n->start-1;

   #ifdef XDBGX
      DBGPRT(1,"code scan %x-%x", start, end);
   #endif


// XCODE ??

   cb = get_aux_cmd(start, 0);
   if (cb && cb->fcom == C_XCODE)
     {
       #ifdef XDBGX
        DBGPRT(1,"Ignore, XCODE set");
       #endif
       return 0;

       //if end < gap end ??
     }

   //data ?? get next item from start

   get_dtkd(&chdtkd,0, start);
   jx = chdtkd.lastix;
   d = 0;
   if (jx < chdtkd.num)
      {
       d = (DTD *) chdtkd.ptrs[jx];
       if (d->stdat <= end && !d->psh)
         {
          #ifdef XDBGX
            DBGPRT(1,"Data found at %x, STOP", d->stdat);
          #endif

          // if 0xf0 at previous to stdat then can still scan....


          return 0;
        }
      }
   //code scan

   //change this to scan between f0/f1 backwards ? for gap which have data at end.
   //also if start-1 is an f0 or not.

   for (jx = start; jx <= end; jx++)
     {
      val = g_byte(jx);
      if (val == 0xf0 || val == 0xf1 ) break;
     }

   if (jx > end) return 0;


//   if (s->fcom == C_CODE)
  //   {
       //if (n->fcom == C_CODE) return do_cgap(start);

//only if gap ends with a ret.

        // code, not code. does last byte end in jump or ret ??
        //may need to scan backwards for f0 here...
    //    if (val == 0xf0 || val == 0xf1 )
      //    {
            return do_cgap(start);
        //  }

   //  }
  //  else
  //  {
        //not code -> code
     //if next command is code...end gap is code, and need previous is f0 consider scanning code...
    // but may get a couple together....
   //   if (n->fcom == C_CODE)
     //   {
       //  if (val == 0xf0 || val == 0xf1 )
         //  {
           //  return do_cgap(start-1);          //start BEFORE the gap..
           //}
        //}
    // }

    return 0;
}





uint scan_fillgap(LBK *s, LBK *n)
{
    // not code, so look for data structs
    // check for fill block first.  Must not overlap anything data.
   uint i,jx, start, end, flag;
   DTD *d;

   if (n->fcom <= C_BYTE) return 0; // n bigger than byte

   start = s->end+1;
   end  = n->start-1;        i = s->end+1;

   get_dtkd(&chdtkd,0, start);
   jx = chdtkd.lastix;
   d = 0;
   if (jx < chdtkd.num)
      {
       d = (DTD *) chdtkd.ptrs[jx];
         if (d && d->stdat < end) end = d->stdat-1;
      //   flag = 0;
           flag = 1;

         while (i <= end)
          {
           jx = g_byte(i);
           if (jx != 0xff)
            {
             flag = 0;
             break;
            }
           i++;
          }

         if (flag)
           {
            add_cmd(start, end,0,0);      // all fill
            if (!chcmd.lasterr) return 1;   // new command OK
           }
        }



return 0;
}




/*

data -> data gaps only
   // done AFTER code added

  uint ix, jx, kx, start, end, xofst;   //, xdata;
  LBK *s, *n, *b, *c;
  DTK *x, *z;
  SBK *t;
  JMP *j;

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(2, "Data gaps analysis");
  #endif

//  sniff_func(0x98de2, 0x98e5c);  // test

//DBGPRT(0,"*****Test 1");

//do_data_pat(0x8225f, 0x82284);

//DBGPRT(0,"*****Test 2");

//do_data_pat(0x82286, 0x822a5);

//DBGPRT(0,"*****Test 3");

//do_data_pat(0x822a6, 0x82357);


  b = (LBK*) chmem(&chcode,0);
  ix = 0;
  while (ix < chdata.num)
   {
    if (ix+1 >= chdata.num) break;    //end of chain
    s = (LBK*) chdata.ptrs[ix];

  if (nobank(s->end) <= 0x201e) { ix++; continue;}

    n = (LBK*) chdata.ptrs[ix+1]; // next scan block

    start = s->end+1;
    end  = n->start-1;

    // is this a single filler byte = 0xff ??

    if ((n->start - s->end) == 2)
      {
       if (n->fcom > C_BYTE)
        {
         jx = g_byte(start);
         if (jx == 0xff)   add_data_cmd(start,start,0,0);      //fill
        }
      }

    if ((n->start - s->end) > 2)
      {

         #ifdef XDBGX
           DBGPRT(0,"\nDATA GAP %x-%x", start, end);
           #endif



       b->start = s->end;         //find nearest code block to n->start
       jx = bfindix(&chcode, b);

       if (jx < chcode.num)
         {       // a valid block
          c = (LBK*) chcode.ptrs[jx];

          if (c->start > start)
            {
              // possible gap before code block
           if (c->start <= n->start && c->end >= n->end)
             {
              // HERE would check for offset < 16 fixer
            //also not for ARGS ....
    #ifdef XDBGX
           DBGPRT(0,"\n*** delete data entry ? %x-%x o%d[%x %x]", n->start, n->end, n->opcsub, c->start, c->end);
           #endif
// and go back ??
}

           if (c->start < end && c->end  >= end)
             {  //overlaps end
           //   end = c->start-1;      //if (xofst) end = xofst-1;         //still not right.....................
           #ifdef XDBGX
           DBGPRT(0,"\n Mod gap to %x-%x (1)   code[%x %x]", start, c->start-1 , c->start, c->end);
           #endif

           // if n is completely spanned by code, ditch it...................


           }
else DBGPRT(0,"\n Catch 1 %x %x", c->start, c->end  );
          }

         if (c->start <= start)
            {
              // overlaps start of gap
            if (c->end < end)
              {
           start = c->end+1;
           #ifdef XDBGX
           DBGPRT(0,"\n  mod gap to %x-%x (2) code[%x %x]", start, end, c->start, c->end);
           #endif
           }
           else DBGPRT(0,"\n Ignore - Code spans 2 %x %x ", c->start, c->end  );
          }

      } }
    ix++;
   }
}



--- move to another subr
       if (end > addr)      // && !get_opdatar(addr, end))
         {
           #ifdef XDBGX
           DBGPRT(0,"\n******* DATA GAP %x-%x ", addr, end);
           DBGPBK(0,s,"Pre");
        #endif

    x = (DTK*) chmem(&chdtk,0);
    x->start = addr;

    jx = bfindix(&chdtkd, x);  // find nearest dtk (after) 's' by DATA addr

    x = (DTK*) chdtkd.ptrs[jx];

if (x) {
    if (x->start != addr)
      {       // not found, try last address (in 's')
       jx--;             //previous blk

       if (jx < chdtk.num)
         {
            x = (DTK*) chdtkd.ptrs[jx];
         }
      }

    #ifdef XDBGX
       DBGPRT(0,"DTK %x R%x @%x", x->start, x->rreg, x->ofst);
       if (x->inc) DBGPRT(0," INC");     // implies list already
       if (x->imd) DBGPRT(0," IMD");     // direct
       if (x->imd) DBGPRT(0," INX");     // list or struct
    #endif

// if it's an INC already, chances are it's a loop -- need to find the loop !!!!

  //  if (x->inc) sniff_loop(jx);

}}
 // if (x->imd) sniff_structs();    or inx then investigate struct ??
 // if (x->inx) sniff_structs();    or inx then investigate struct ??
/// do_data_pat(s->end+1, n->start-1);


// prob need to find the nearest previous imd flagged ?
// or investigate entries with subr.....

//but first, need nearby DTKs with their attributes ???
//if it's found as an IMD, then can condsider if a func or table ??
// if it's an INX, then it might be a list...............or a struct.



           jx--;             //previous blk

           if (jx < chdtk.num)
             {
               x = (DTK*) chdtkd.ptrs[jx];
               #ifdef XDBGX
               DBGPRT(0," Prev %x R%x @%x ", x->start, x->rreg, x->ofst);

                if (x->inc) DBGPRT(0," INC");     //implies list already
                if (x->imd) DBGPRT(0," IMD");     // and look fro another....
                if (x->imd) DBGPRT(0," INX");     // list or struct

               #endif


// if this is an IMD, see if there is another entry.


                xofst = x->ofst;
//could be in loop here.....


               kx = jx-1;
               while (kx < chdtkd.num) //while ??  could be multiple data entries
                 {
                   z = (DTK*) chdtkd.ptrs[kx];
                   if (x->start - z->start > 256) break;
                   if (z->imd)
                     {
                       #ifdef XDBGX
                       DBGPRT(0," Prev IMD %x R%x @%x", z->start, z->rreg, z->ofst);
                if (z->inc) DBGPRT(0," INC");     //implies list already
                if (z->imd) DBGPRT(0," IMD");     // and look fro another....
                if (z->imd) DBGPRT(0," INX");     // list or struct
                       #endif
           //            if (x->ofst - z->ofst < 256)
           //            xofst = z->ofst; else xofst = 0;
                       break;
                     }


                   kx--;
                 }


// need a NEXT imd as well

               kx = jx;
               while (kx < chdtkd.num)
                 {
                   z = (DTK*) chdtkd.ptrs[kx];
                 if (z->start - x->start > 256) break;
                     if (z->imd)
                        {
#ifdef XDBGX
                         DBGPRT(0," Next IMD %x R%x @%x", z->start, z->rreg, z->ofst);
#endif
                         break;
                        }

                     kx++;
                  }

             }


           j = (JMP*) chmem(&chjpf,0);
           j->fromaddr = x->ofst;

           jx = bfindix(&chjpf, j); // find nearest by Doffset

           //   jx--;

           if (jx < chjpf.num)
             {
               j = (JMP*) chjpf.ptrs[jx];
               if (j->back && j->toaddr <= x->ofst && j->fromaddr > x->ofst)
                  {
#ifdef XDBGX
                   DBGPRT(1, " found loop %x-%x", j->fromaddr, j->toaddr);
#endif
                  }
             }



// now swop to OFST chain....................if loop ??

//adjacent.

           if (xofst)
             {
               jx = bfindix(&chdtk, x); // find nearest by ofst
               z = (DTK*) chdtk.ptrs[jx];
               jx--;             //previous ofst

               while (jx < chdtk.num)
                 {
                   x = (DTK*) chdtk.ptrs[jx];
                     if (x->imd || x->ofst <= xofst)
                        {
#ifdef XDBGX
                         DBGPRT(0," Prev IMD %x R%x @%x (%d)", x->start, x->rreg, x->ofst, jx);
#endif
                         break;
                        }

                     if (x->ofst == z->ofst && x->start != z->start)
#ifdef XDBGX
                           DBGPRT(0," ofst %x, data %x %x", x->ofst, x->start, z->start);
#endif
z = x;
jx--;

// adjacent and loop too, to verify increment size and types, and whether to swap to index instead.

                    }
            //     }
}


// only if no xcode ...probably this is not reqd...............

               t = (SBK*) chmem(&chscan,1);              // dummy block for search
               t->start = addr;                  // symbol addr

               jx = bfindix(&chscan, t);


               if (jx && jx < chscan.num)
                 {
                  t = (SBK*) chscan.ptrs[jx-1];

                  if (t) {
#ifdef XDBGX
                  if ((addr - t->start) < 256) DBGPRT(0," Prev code for %x = %x-%x",addr, t->start,t->nextaddr);   // not if within xcode area.............
#endif
                         }
                  }
#ifdef XDBGX
          DBGPRT(1,0);
#endif
            } // if data cmd gap


     //    }  //if opdata gap

      }
    ix++;
   }


do pass for functions and lists - inside the data blocks....

rules for function - at least 4 rows (?) must start and end with defined values.



 how to decide to go index, and then what about pointers to tables and funcs ??





test

 for (i = 0; i < BMAX; i++)
    {
     b = bkmap+i;
      if (b->bok)
       {

        for (ofst = b->minpc; ofst < b->maxbk; ofst++)
          {
       //      if (!get_opdata(ofst)) {DBGPRT(1, "No flag %x", ofst);}
            if (state  && !get_opdata(ofst)) { state = 0; DBGPRT(0, "No flag %x", ofst);}
            if (!state &&  get_opdata(ofst)) { state = 1; DBGPRT(1, "-%x",ofst-1) ;}
          }
        DBGPRT(1,0);
       }

     }
} */