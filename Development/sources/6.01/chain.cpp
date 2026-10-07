
#include  "chain.h"               // this calls shared.h

void *shuffle[16];                // for chain reorganises (and deletes), a block of void pointers

char tempname[96];


/**********************************************************************************
* CHAIN declarations, used ordered storage with for binary chop searches
* changed to an array so simple index can be used.
*
* CHAIN structures use common find and insert mechanisms with defined subroutines for
* each chain type, Each type has its own structure associated with it.
* searched with 'binary chop' type algorithm.
* CHAIN params =  allocsize, entrysize, num entries, num ptrs, maxptrs, num mallocs, ptr array,
* subroutines - free, compare, equals, (debug)
***********************************************************************************/

// Free block

void fsym(void *); void fsub(void *);
void fcmd(void *);
void fmnam(void *);

// compare, for binary chop searches

int cpjmpf  (CHAIN *,uint, void *); int cpjmpt  (CHAIN *,uint, void *);

int cpbase (CHAIN *,uint, void *);
int cpcmd  (CHAIN *,uint, void *);   int cpscan (CHAIN *,uint, void *);
int cpsub  (CHAIN *,uint, void *);   int cpsym  (CHAIN *,uint, void *);
int cpsig  (CHAIN *,uint, void *);   int cpsscn (CHAIN *,uint, void *);
int cpadt  (CHAIN *,uint, void *);   int cpdtdo (CHAIN *,uint, void *);
int cpspf  (CHAIN *,uint, void *);   int cppsw  (CHAIN *,uint, void *);
int cpbnk  (CHAIN *,uint, void *);   int cpopcd (CHAIN *,uint, void *);
int cprgst (CHAIN *,uint, void *);   int cprgsta (CHAIN *,uint, void *);
int cpdtdd (CHAIN *,uint, void *);

int cpname (CHAIN *,uint, void *);
int cpmath (CHAIN *,uint, void *);
int cplink (CHAIN *,uint, void *);


// data storage - as ordered arrays, searched by binary chop
// chain index defines


//one search pointer (malloced on first use) for search chsmem
//one 'next' pointer for chimem, reset to num when insert (fail or success) for linked items BEFORE insert



//scech, mchain
CHAIN datach [22] = {

{ 2000, sizeof(JMP),   0,0,0,  0,0,0,    0,0, 0      ,cpjmpf  },     // CHJPF  [0]  jumps, indexed by 'from'
{ 2000, 0          ,   0,0,0,  0,0,0,    0,0, 0      ,cpjmpt  },     // CHJPT   jumps, indexed by 'to' [REMOTE]

{ 1000, sizeof(SYM),   0,0,0,  0,0,0,    0,0, fsym   ,cpsym   },     // CHSYM symbols
{ 200,  sizeof(RBT),   0,0,0,  0,0,0,    0,0, 0      ,cpbase  },     // CHBASE rbases
{  20,  sizeof(SIG),   0,0,0,  0,0,0,    0,0, 0      ,cpsig   },     // CHSIG signatures

{ 200,  sizeof(LBK),   0,0,0,  0,0,0,    0,0, fcmd   ,cpcmd  },      // CHCMD [5] main code and data commands
{  50,  sizeof(LBK),   0,0,0,  0,0,0,    0,0, fcmd   ,cpcmd  },      // CHAUX  auxilliary commands (arg, xcode, ...)
{ 2000, sizeof(SBK),   0,0,0,  0,0,0,    0,0, 0      ,cpscan },      // CHSCAN code scans
{  50,  sizeof(SBK),   0,0,0,  0,0,0,    0,0, 0      ,cpscan },      // CHEMUL code emulations cannot merge with alt if do in-line vect lists....

{  500, sizeof(SUB),   0,0,0,  0,0,0,    0,0, fsub   ,cpsub  },      // CHSUB  subroutines
{  50,  sizeof(ADT),   0,0,0,  0,0,0,    0,0, 0      ,cpadt  },      // CHADNL [10] additional data, linked by FK
{  10,  sizeof(SPF),   0,0,0,  0,0,0,    0,0, 0      ,cpspf  },      // CHSPF  special funcs (subroutines, tablu, funclu etc.)
{  20,  sizeof(PSW),   0,0,0,  0,0,0,    0,0, 0      ,cppsw  },      // CHPSW  psw setters


{ 2000, sizeof(DTD),   0,0,0,  0,0,0,    0,0, 0      ,cpdtdo  },     // CHDTDO data address tracking.  by ofst not reqd if scan the opcodes ? but is if more than one address (loops)
{ 2000, 0          ,   0,0,0,  0,0,0,    0,0, 0      ,cpdtdd  },     // CHDTDD data address tracking.  by data ptr [REMOTE]

{  10,  sizeof(BNKF),  0,0,0,  0,0,0,    0,0, 0      ,cpbnk   },     // CHBKF  [15] bank finds (not final banks.)
{10000, sizeof(INST),  0,0,0,  0,0,0,    0,0, 0      ,cpopcd  },     // CHOPC  main opcodes index by ofst.

{ 100,  sizeof(RST),   0,0,0,  0,0,0,    0,0, 0      ,cprgst  },     // CHRGST  register status for args (by register)
{ 100,  0          ,   0,0,0,  0,0,0,    0,0, 0      ,cprgsta },     // CHRGSTA register status (by address)  REMOTE [18]

{50  ,  sizeof(MATHN) ,0,0,0,  0,0,0,    0,0, fmnam  ,cpname  },     // CHMATHN          math func names.  index by name
{50  ,  sizeof(MATHX), 0,0,0,  0,0,0,    0,0, 0      ,cpmath  },     // CHMATHX   [20]   math calc definitions - first term links to name

{500,   sizeof(FKL),   0,0,0,  0,0,0,   0,0,  0      ,cplink  },     // CHLINK    general linker many->many,   keyin ->  keyout

// alternate items for 'test' mode (vects, emulates etc)
// instead of parallel queus, have flags 'inv' and 'test' in every relevant chain.



//only need scans and jumps ?? opcodes ??

/*alts not used right now, but leave in for now...

{ 200, sizeof(JMP),   0,0,0,  0,0,0,    0,0, 0    ,cpjmpf  },     // [22]   ALTJFP jump, indexed by 'from'
{ 200, 0          ,   0,0,0,  0,0,0,    0,0, 0    ,cpjmpt  },      // ALTJPT [20] jump, indexed by 'to' [REMOTE]

{ 200, sizeof(SYM),   0,0,0,  0,0,0,    0,0, fsym ,cpsym  },     // ALTSYM symbols

{ 200, sizeof(RBT),   0,0,0,  0,0,0,    0,0, 0    ,cpbase },      // [25]  ALTBASE rbases
{  50, sizeof(LBK),   0,0,0,  0,0,0,    0,0, fcmd ,cpcmd  },      // ALTAUX auxilliary commands (arg, xcode, ...)
{  20, sizeof(SBK),   0,0,0,  0,0,0,    0,0, 0    ,cpscan },      // ALTSCAN alternate scans  for gap and vect
{  50, sizeof(SUB),   0,0,0,  0,0,0,    0,0, fsub ,cpsub  },      // ALTSUBR subroutines
{ 200, sizeof(DTD),   0,0,0,  0,0,0,   0,0, 0    ,cpdtdo  },     // ALTTDO data address tracking.  by ofst not reqd if scan the opcodes ? but is if more than one address (loops)
{ 500, 0          ,   0,0,0,  0,0,0,   0,0, 0    ,cpdtdd  },     // [30] ALTTDD data address tracking.  by data ptr [REMOTE]
{ 500, sizeof(INST),  0,0,0,  0,0,0,   0,0, 0    ,cpopcd  },     // ALTOPC alt main opcodes index by ofst.

*/

};






#ifdef XDBGX


 // memory allocation debugs


 uint DBGmcalls   = 0;            // malloc calls
 uint DBGmrels    = 0;            // malloc releases
 uint DBGmaxalloc = 0;           //  TEMP for malloc tracing
 uint DBGcuralloc = 0;


#endif



   uint adds [10];
   uint szes [32];

   uchar rbflags[128];     //  128*8 = 0x400 flags for rbt registers




CHAIN *get_chain(uint chn)
{          //real chain
    if (chn >= NC(datach) ) return NULL;
    return datach+chn;
}





uint get_lastix(uint ch)       //couple of uses from core
{
  CHAIN *x;
  if (ch >= NC(datach) ) return 0;
  x = datach + ch;
  return x->lastix;
}


void* get_next_item(uint ch)
{
 CHAIN *x;

 if (ch >= NC(datach) ) return 0;
 x = datach + ch;

 x->lastix++;
 if (x->lastix >= x->num) return 0;

return x->ptrs[x->lastix];

}



uint get_lasterr(uint ch)
{
  CHAIN *x;
    if (ch >= NC(datach) ) return 0;
  x = datach + ch;
 return x->lasterr;
}


uint valid_ix(uint ch, uint ix)
{
  CHAIN *x;
  if (ch >= NC(datach) ) return 0;
  x = datach + ch;
 if (ix >= x->num) return 0;
 return ix;
}


void* get_chitem(uint ch, uint ix)
{
  CHAIN *x;
  if (ch >= NC(datach) ) return 0;
  x = datach + ch;
 if (ix >= x->num) return 0;
  return x->ptrs[ix];
}

// get free block ??





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
  if (nsize < 1) xprt(MSGFILE,1,"Invalid malloc size");

  #ifdef XDBGX
  if (ptr)  DBGcuralloc -= osize;
  DBGmcalls++;
  DBGcuralloc += nsize;
  if (DBGcuralloc > DBGmaxalloc) DBGmaxalloc = DBGcuralloc;
  #endif

  ans = realloc(ptr, nsize);

  if (!ans) xprt(MSGFILE,1,"Run out of malloc space");
  return ans;
}


void* cmem(void *ptr, size_t osize, size_t nsize)
{  // malloc with clearmem
  void *ans;
  ans = mem(ptr,osize,nsize);
  memset(ans,0,nsize);
  return ans;
}







 //----------------------------------------------------------------
 // add pointers for chains. malloced entries
 // pointer block is realloced, so is continuous...
 // but data blocks are NOT....
 //----------------------------------------------------------------

void adpch(uint ch)
 {   // add pointers for more chain pointers in main chain (malloced entries)
     // pointer block is realloced, so is always contiguous.
     // data blocks are not.
     // called from chimem and chinsert

    CHAIN *x;
    int old, nw, i;

    x = get_chain(ch);

    // grow pointer array - always as single block for arrays so must realloc
    old = x->pnum;
    x->pnum += x->asize;

    nw = x->pnum;            // new array size (contiguous)

    x->ptrs = (void**) mem(x->ptrs, old*sizeof(void*), nw*sizeof(void*));

    // malloc each new item if not a remote chain

    if (x->esize)
     {
       for (i = old;  i < nw; i++) x->ptrs[i] = mem(0,0,x->esize);
     }
 }





void *chimem(uint ch)
 {
   // get a free entry of the chain for insert.
   // this is from x->num onwards (next free entry) + ix for nested use

   //relies on x->next being reset for insrt,delete,clear chain etc.

  CHAIN *x;
  void *ptr;

  x = get_chain(ch);

  if (!x->esize) return 0;

  if (x->pnum < (x->next+2)) adpch(ch);        // need more pointers (unsigned check) NOT IF REMOTE

//temp

//if (x->next > x->num )
//{
 //   DBGPRT(0,0);       // OK- never hits this now ....
//}


  ptr = x->ptrs[x->next];         //num];                  // next free block, rest by insert or delete
  x->next++;

  memset(ptr,0, x->esize);                    // and clear it
  return ptr;
 }



void *chsmem(uint ch)
 {
   // get the 'search' ptr/block for defined chain, separate from the real chain

  CHAIN *x;
  void *ptr;

  // note that remote chains cannot have own search block (no esize)

  x = get_chain(ch);

  if (!x->esize) return 0;

  if (!x->schptr) x->schptr = mem(0,0,x->esize);

  ptr = x->schptr;

  memset(ptr,0, x->esize);                   // and clear it
  return ptr;
 }






uint bfindix(CHAIN *x, void *blk)
{
  // generic binary chop.
  // send back an INDEX value to allow access to adjacent blocks.
  // use a compare subr as supplied or default for chain struct.
  // Candidate block is void* and cast to reqd type inside cmp (compare subroutine).
  // answer (min) is always min <= blk, or blk is always >= min (= answer)
  // and min+1 is > blk.   Answer is therefore INSERT point for new block, or FOUND.

  uint tst, min, max;

  min = 0;
  max = x->num;

  while (min < max)
    {
      tst = min + ((max - min) / 2);                // safest way to do midpoint of (min, max);
      if (x->comp(x,tst,blk) < 0) min = tst + 1;    // blk > tst, so new minimum is (tst+1)
      else  max = tst;                              // blk <= tst, so new maximum is tst .. tst-1 when =0 tested ??
     }

  x->lastix = min;                                  // keep last index (including x->num)
  return min;
}




void chinsert(uint ch, int ix, void *blk)
{
  // insert new pointer (= blk) at index ix
  // relies on main pointer block being contiguous.

  // Note, blk = x->ptrs[x->num], UNLESS x is a remote chain
  // which does not have its own blocks/items
  // this overwrites ptr at x->num in shuffle, so just increment x->num


 CHAIN *x;

 if (!blk) return ;

 x = get_chain(ch);

 if (x->pnum < (x->num+3)) adpch(ch);     // need more pointers in MAIN chain

// if (blk != x->ptrs[x->num] && x->ptrs[x->num])        //TEST !! remote chains should be zero
// {
//     DBGPRT(0,0);
// }

 // move entries up one from ix to make a space.
 memmove(x->ptrs+ix+1, x->ptrs+ix, sizeof(void*) * (x->num-ix));   // fastest way for contiguous array

 x->ptrs[ix] = blk;                              // insert new block at ix
 x->num++;

 x->next = x->num;                               // reset after insert
 x->lastix = x->num;                             // invalidate any get_next

// #ifdef  XDBGX
//  if (x->num > DBGmax[ch]) DBGmax[ch] = x->num;     // for debug
// #endif


}




void chdelete(CHAIN *x, uint ix, int num)
{
 // drop num block(s) from list from ix, shuffle down rest of
 // list and restore ptr(s) after x->num.
 // relies on main pointer block being contiguous
 // shuffle array to hold pointers (max 16)
 // extra +1 in shuffle is for x->num block (used by chmem)

// CHAIN *x;
 uint i;

// x = get_chain(ch);
 if (ix >= x->num) return;

 if (x->cfree)
   {                 //free any additional items in entries
    for (i = ix;  i < ix+num; i++) x->cfree(x->ptrs[i]);
   }

 // save pointers about to be 'deleted' (released) in temp array
 memcpy(shuffle, x->ptrs+ix, sizeof(void*) * num);

 x->num -= num;                   //drop no of defined entries
 x->next = x->num;                // reset x->next, but maybe should be later ??

 // move entries down by 'num' from ix+num to overwrite released entries, with extra chimem pointer.
 // at x->num. as safety  // but does this need to change to x->next for multiple inserts ??

 memmove(x->ptrs+ix, x->ptrs+ix+num, sizeof(void*) * (x->num-ix+1));

 // put released pointers back above new x->num position so they become unused, or should they go at end?

 memcpy(x->ptrs + x->num+1, shuffle, sizeof(void*) * num);

 x->lastix = x->num;                              // invalidate any get_next
}









void chupdate(uint ch, uint ix)
{
  // block at index 'ix' has been updated - rechain in correct place
  // may need extra check here if ix = x->num

  CHAIN *x;
  void *blk;
  uint newix;

  x = get_chain(ch);
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




void emptychain(uint ch)
{
  // zero pointer list contents, but not pointers themselves. Free any attached structures.
  //  NB.  count DOWN so that for mblks the first entry (which points to itself) is released last.
  // does NOT release chain itself (freechain does that)

 CHAIN *x;

 int i;

 x = get_chain(ch);

 if (x->esize && x->cfree)
  {           // if extra add ons and not remote chain
   for (i = x->num-1; i >=0; i--)
    {
      x->cfree(x->ptrs[i]);
    }


  }
 x->num = 0;
 x->next = 0;
}


void freeptrs(uint ch)
{
    // free the master pointer block of chain x (after emptychain does entries)
    // pointers always one contiguous block
    CHAIN *x;
    int i;

    x = get_chain(ch);

    //free data blocks via pointers
     if (x->esize)
       {
         for (i = x->pnum-1; i >=0; i--)
           {
             mfree(x->ptrs[i], x->esize);
             x->ptrs[i] = 0;
           }
         mfree(x->schptr, x->esize);      // and search block
       }


 if (x->pnum)
   {
    mfree(x->ptrs, x->pnum * sizeof(void*));
    x->ptrs = NULL;
    x->pnum = 0;
   }
}

void freechain(uint ch)
{

    #ifdef XDBGX
    DBG_chn(ch);
    #endif

  emptychain(ch);
  freeptrs(ch);

}

void free_structs ()
{                        // complete tidyup for closing or restart
 uint i;
 FDATA *fld;

 for (i = 0; i < NC(datach); i++) freechain(i);           // this includes mblock (last)

   #ifdef XDBGX
    DBGPRT(1,"max alloc = %d (%dK)", DBGmaxalloc, DBGmaxalloc/1024);
    DBGPRT(1,"cur alloc = %d", DBGcuralloc);
    DBGPRT(1,"memcalls = %d", DBGmcalls);
    DBGPRT(1,"memrels  = %d", DBGmrels);

    DBGPRT(1,"Max recurse = %d", DBGrecurse);
    DBGPRT(1,"Num Opcodes = %d", DBGnumops);
   #endif

   fld = get_fldata(0);
   fflush (fld[1].fh);            // flush output files (safety)
   fflush (fld[2].fh);


}


void *vconvi(uint addr)
{
   // Convert a uint of 32 bits (e.g. address) into void* (64 bits) and avoid compiler warnings.
   // compiler alignment irrelevant as long as everything uses this subr, they will match.

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

 INST *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (INST*) x->ptrs[ix];
 nw = (INST*) newb;

 ans = ch->ofst - nw->ofst;                 // start address

 return ans;
 }





int cpjmpf (CHAIN *x, uint ix, void *d)
{
  //int ans;
  JMP *j, *t;

  // jump chains.  fromaddr is unique, to chain is not.

  if (ix >= x->num) return -1;

  j = (JMP*) x->ptrs[ix];
  t = (JMP*) d;

  return j->fromaddr - t->fromaddr;

//  return ans;
}


int cpjmpt (CHAIN *x, uint ix, void *d)
{
  int ans;
  JMP *j, *t;

   // jump chains.  fromaddr is unique, to chain is not.


  if (ix >= x->num) return -1;

  j = (JMP*) x->ptrs[ix];
  t = (JMP*) d;

  // includes fromaddr (0 gives first from jump)
  ans = j->toaddr - t->toaddr;
  if (ans) return ans;

  ans = j->fromaddr - t->fromaddr;
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

// check range, for searches but not inserts....

 return nw->rstart  -  ch->rstart;    // by range start

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

  ans = t->rstart - b->rstart;  // by start, in reverse order

  return ans;
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



int cpspf (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   //only ever one spf per subr, so unique

 SPF  *s, *t;
 long ans;     // for fid compare

 if (ix >= x->num) return -1;

 s = (SPF*) x->ptrs[ix];
 t = (SPF*) d;

 ans = (char *) s->fid - (char *) t->fid;

 if (ans < 0) return -1;
 if (ans > 0) return 1;
 return 0;

 //return (long) s->pkey - (long) t->pkey;

}

int cpadt (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   // must use long and change to char for ptr maths
   //this compares solely by fid and first/second marker

 ADT  *s, *t;
 long ans;

 if (ix >= x->num) return -1;

 s = (ADT*) x->ptrs[ix];
 t = (ADT*) d;

 ans = (char*) s->fid - (char*) t->fid;

 if (!ans)
   {
    ans = s->sbix - t->sbix;         // and sub index
   }

 return ans;

 if (ans < 0) return -1;
 if (ans > 0) return 1;
 return 0;



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

// need to tell where block called/jumped from,
// for args.  Not got better solution yet.

if (t->substart) ans = s->substart - t->substart;

return ans;
}

/*
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
*/


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

/*int cpdtk (CHAIN *x, uint ix, void *d)
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

*
if ( x == &chdtkr)
  {
    ans = s->?
*

// return ans;
}
*/








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


int cpdtdo (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   // offset find.

 DTD  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (DTD*) x->ptrs[ix];
 t = (DTD*) d;

 ans = s->ofst - t->ofst;
 if (ans) return ans;

 ans = s->dataptr - t->dataptr;
 return ans;
 }


int cpdtdd (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   // data find.

 DTD  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (DTD*) x->ptrs[ix];
 t = (DTD*) d;

  ans = s->dataptr - t->dataptr;
  if (ans) return ans;

  ans = s->ofst - t->ofst;
 return ans;

}



int cprgst (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   // register find.

 RST  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (RST*) x->ptrs[ix];
 t = (RST*) d;

  ans = s->reg - t->reg;
  if (ans) return ans;

  ans = s->argofst - t->argofst;         // just for order
  return ans;

}

int cprgsta (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk
   // data find.

 RST  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (RST*) x->ptrs[ix];
 t = (RST*) d;

 ans = s->argofst - t->argofst;      //ofst should be unique
 return ans;

}


int cpname  (CHAIN *x, uint ix, void *newb)
{
// math terms, name.  use strcmp for name ordering

 MATHN  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (MATHN*) x->ptrs[ix];
 nw = (MATHN*) newb;

 // not safe without size check, as lookups (nw) are not
 // terminated, but chain item (ch) has correct lenght

 ans = ch->nsize - nw->nsize;
 if (ans) return ans;

 ans = strncmp(ch->mathname, nw->mathname,ch->nsize);

 return ans;

}

/*
 if (nw->fid)
   {
     ans = (char *) ch->fid - (char *) nw->fid;
     if (ans) return ans;
   }

//as passed in by CSTR, strings may not be terminated, so use nsize...

if (nw->nsize && ch->nsize)
  {      //check by name

    ans = ch->nsize - nw->nsize;
    if (ans) return ans;

    ans = strncmp(ch->mathname, nw->mathname, nw->nsize);
    if (ans) return ans;
  }
  else ans = -1;         //can't match null strings

  return ans;
}
*/


int cpmath  (CHAIN *x, uint ix, void *newb)
{
// math terms chained to each other or to name


 MATHX  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (MATHX*) x->ptrs[ix];
 nw = (MATHX*) newb;

 ans = (char *) ch->fid - (char *) nw->fid;

 if (ans) return ans;

 ans = ch->pix - nw->pix;      //parameter index

 return ans;

}

int cplink  (CHAIN *x, uint ix, void *newb)
{
   // key to key generic multiple linker
   // many to many, with extra type
   // no key is unique, and only unique with all 3 values set

   // order by scechain, scekey, destkey, ans store both ways around

 FKL *ch, *nw;
 long ans;

 if (ix >= x->num) return -1;

 ch = (FKL*) x->ptrs[ix];
 nw = (FKL*) newb;


//what if keysce = 0 ???

  ans =  ch->chsce - nw->chsce;

  if (!ans)  ans = (char *) ch->keysce - (char *) nw->keysce;
  if (!ans)  ans =  ch->chdst - nw->chdst;
  if (!ans && nw->keydst)  ans =  (char *) ch->keydst - (char *) nw->keydst;
  if (!ans && nw->type)    ans = ch->type - nw->type;

  if (ans < 0) return -1;
  if (ans > 0) return 1;

  return 0;

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

int eqadt(ADT *a, ADT *b)
{
 /*   will compare the whole chain.....

 need flag for int versus float....

maybe memcmp after the fid ??
offsetof     (byte position of any member)


 typedef struct xadt {

    // data (+others) changed or removed with calc..............
void *fid;                       //  foreign key,  (ofst < 8) | seq 1-255


union
{
float fldat;           // full float for divisor (float = int32 for size)
int   data;            // addr, reg, offset, etc. int to allow for -ve
};


//31
uint dreg    : 10 ;    // data destination register for data - for arg processing (0-0x3ff) (move out to an spf?)
uint fstart  : 5 ;     // 0-31 start of sub field (bit number).
uint fend    : 7 ;     // 0-31 end   of sub field (bit number), sign 0x20, write 0x40    - what about  WHOLE (0x80) ??
uint cnt     : 5 ;     // (O) repeat count, 31 max
uint pfw     : 5 ;     // (P) print min fieldwidth (0-31)



//19
uint bank    : 4 ;     // (K) holds a bank override, zero if none
uint places  : 4 ;     // 0-15 print decimal places (is 7 enough ?) [forces radix 10]
uint enc     : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)
uint prdx    : 2 ;     // (X) print radix 0 = not set, 1 = hex, 2 = dec, 3 bin
uint fnam    : 1 ;     // (N) look for symbol name
uint newl    : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr   : 1 ;     // (R) value is an addr/pointer to symbol (R) (full address)
uint foff    : 1 ;     // (D) data is an offset address
// uint ans     : 1 ;     // (=) this is ANSWER definition (separate item, for verification) move to spf
uint div     : 1 ;     // (V or /) data is FLOAT, use fldat (union with data)
uint mult    : 1 ;     // (*) data is FLOAT, use fldat (union with data)
uint sbix : 1;         // subchain ix (1 if subchain)

// uint write  : 1 ??
} ADT;*/


return 0;
}





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

      DIRS *d;

      d = get_dirs(0);

     // can always merge code and xcode
     if (newb->fcom == C_CODE  && t->fcom == C_CODE)  return 1;
     if (newb->fcom == C_XCODE && t->fcom == C_XCODE) return 1;

     if (newb->usrcmd)  return 0;
     if (t->usrcmd)     return 0;
     if (newb->nomerge) return 0;
     if (t->nomerge)    return 0;

     if (d[t->fcom].merge && t->fcom == newb->fcom)   return 1;

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

//what about additional data - extra param may specifiy what combinations allowed (match, adt <=> none (carry over) etc.
//nedd adt matcher....

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
  if (ans) DBGPRTFLGS(ans,newb,t);
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

 // CHAIN *x;
  LBK *t;
  uint chkf, min, max;
  int ans;

  // OK, find min and max overlap

//  x = get_chain(ch);

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
            DBGPRT(0,"delete1 (%d)", min);
            DBGLBK(0,t);
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
                   DBGPRT(0,"(%d) delete2 ", min);
                   DBGLBK(0,t);
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
              DBGPRT(0,"(%d) DFLT Reject",min);
              DBGLBK(0,newb);
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




uint inscmd(uint ch, LBK *blk)
 {
   /* need extra checks the for address ranges (start->end)
   * returns error or zero.

    * at this call 'blk' is always x->ptrs[x->num] ....
    * do a simple check,   THEN do more complex merging as necessary

   this works but does not merge anything yet.  THis assumes you never get an END overlap ?
   probably true for current scan setup, but not safe.....

   eqcmd only checks that start falls between start&end of chained block....

   NB.   Probably don't care about overlaps right now, can do later, but must capture
   any extended END markers.

   so have to check for exact match, and non_allowed overlaps/overrides ?

    */
  CHAIN *x;
  int ans;
  uint ix;
  LBK *k;

  x = get_chain(ch);

  ix = bfindix(x, blk);                 // where to insert

  k = (LBK*) x->ptrs[ix];

  // check for exact match
  if (ix < x->num && k->start == blk->start && k->end == blk->end && k->fcom == blk->fcom)
    {
     x->lasterr = E_DUPL;
     blk = 0;
     x->next = x->num;               //reset temp pointer
     return E_DUPL;       //no insert
    }

  ans = inschk(x,ix,blk);              // is insert allowed ? (can this do delete ?)

  if (ans)
      {
       ix = bfindix(x, blk);          // find again in case stuff deleted/merged
       chinsert(ch, ix, blk);         // do insert at ix
       x->lasterr = 0;                // lasterr OK
      }
  else
     {
      blk = (LBK*) x->ptrs[ix];       // overlap. (duplicate above)
      x->lasterr =  E_OVLP;
      x->next = x->num;               //reset temp pointer
      return E_OVLP;
     }
 return 0;

}



// match for signature blocks OK

SIG* add_sig(SIG *sig)
 {
  SIG *s;
  CHAIN *chsig;
  int ans;
  uint ix, jx, cnt;

  chsig = datach + CHSIG;
  //no chemem as done externally for data to be copied in.


//  if (sig->start == 0x935cd)
//  {
//      DBGPRT(0,0);
//  }



  jx = bfindix(chsig, sig);

  ans = -1;

  ix = jx;

  cnt = 0;
  // check for overlaps of same sig, due to repeat ops
  // go back fixed number of entries as setbank (encode?)
  // break standard start->end checks

  while (cnt < 5)
    {
      if (!ans || ix >= chsig->num) break;

      s = (SIG*) chsig->ptrs[ix];             // previous entry

  //    if (s->start == sig->start) ans = 0;       // always duplicate      //temp disable for tests

      if (s->patno == sig->patno)
        {                   // check for overlaps for same sig only
          if (s->start <= sig->start && s->end >= sig->start) ans = 0;
        }
       ix--;
       cnt++;
     }

   #ifdef XDBGX
        DBGPRT(0, "add sig %s (%d) %x %x", get_signame(sig->patno),sig->patno, sig->start,sig->end);
      #endif



    if (!ans)
     {
      chsig->lasterr = E_DUPL;
      chsig->next    = chsig->num;               //reset temp pointer
    #ifdef XDBGX
      DBGPRT(0, " FAIL");
      DBGPRT(1, " with sig %s (%d) %x %x", get_signame(s->patno),s->patno, s->start,s->end);
            #endif
      return 0;
     }

//  DBGPRT(1,0);
  chinsert(CHSIG, jx, sig);      // do insert

  return sig;

}


LBK* add_cmd(uint start, uint end, uint com)
 {

   LBK *b;
   CHAIN *x;
   uint tcom;

   x = datach + CHCMD;
   x->lasterr = E_INVA;           // set "invalid address"
   tcom = com & 0x1f;                // drop any extra flags

   if (end < start) end = start;             // safety

   //only code at 0x2000 in each bank
   if (nobank(start) == 0x2000 && tcom != C_CODE) return NULL;


   if  (tcom == C_DFLT || tcom == C_TEXT)
    {  // special for fill and text cmd - use whole bank
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
        x->lasterr = E_BKNM;           // set "banks don't match"
        return NULL;
       }
     }


 //  chcmd->lasterr = 0;      //done in insert
 // no need for C_ALT

  // b = (LBK *) chmem(CHCMD);
 //  ix = chmem(CHCMD);

   b = (LBK*) chimem(CHCMD);

   b->start = start;
   b->end = end;
 //  b->from = from;
   b->fcom  = tcom;             // max 31 as set
   if (com & C_USER)          b->usrcmd = 1;
   if (com & C_NOMERGE)      b->nomerge = 1;
 //  if (get_cmdopt(OPTCMPA))  b->cptl = 1;         // compact args layout

// gribble NOMERGE


   x->lasterr = inscmd(CHCMD,b);

 //  if (!chdata.lasterr) set_opdatar(start,end);       // mark data

// note that b may be WRONG here after merges etc.

   #ifdef XDBGX

   if (x->lasterr) DBGPRT(0, "FAIL ");
   DBGPRT (0,"add cmd ");
   DBGLBK(1,b);

   #endif

   return b;
}





INST *add_opcode(INST *c)
{
  CHAIN *x;
  INST *z;
  uint ix;
  int ans;

  x = datach + CHOPC;
  x->lasterr = E_INVA;
  if (!val_rom_addr(c->ofst))  return 0;

  z = (INST *) chimem(CHOPC);

  *z = *c;                      //  complete copy of struct

  ix = bfindix(x, z);
  ans = x->comp(x,ix, z);

  if (ans)
   {
    chinsert(CHOPC, ix, z);
    x->lasterr = 0;

    return z;
   }

 //  #ifdef XDBGX
 //     if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail");
 //     DBGPRT(0," opcode %x", start);
 //     DBGPRT(1,0);
 //    #endif


  x->lasterr = E_DUPL;       // duplicate
  x->next = x->num;         // reset temp pointer
return 0;
}


uint get_opcode_ix(uint addr)
{
  INST *r;
  uint ix;
  CHAIN *chopcd;

 //   if (c->scanblk->proctype & 1) chopcd = datach + ALTOPC;
 // else

 chopcd = datach + CHOPC;


  r = (INST*) chsmem(CHOPC);
  r->ofst = addr;

  ix = bfindix(chopcd, r);
  if (chopcd->comp(chopcd,ix,r)) return chopcd->num;    // no match
  return ix;
}

CINST *get_copcode(uint addr)  //, uint *iy)
{
  // returns a CONST INST, not modifiable.
  // this for lookups which do NOT change inst.
  // direct chain reference.
  INST *r;
  uint ix;
  CHAIN *chopcd;

 //   if (c->scanblk->proctype & 1) chopcd = datach + ALTOPC;
 // else

     chopcd = datach + CHOPC;

  r = (INST*) chsmem(CHOPC);
  r->ofst = addr;

  ix = bfindix(chopcd, r);
  if (chopcd->comp(chopcd,ix,r)) return NULL;    // no match

  return (CINST*) chopcd->ptrs[ix];
}


CINST *get_nearcopcode(uint addr)
{
  // returns a CONST INST, not modifiable.
  // this for lookups which do NOT change inst.
  // direct chain reference.
  INST *r;
  CINST *c;
  uint ix;
  CHAIN *chopcd;

 //   if (c->scanblk->proctype & 1) chopcd = datach + ALTOPC;
 // else

  chopcd = datach + CHOPC;

  r = (INST*) chsmem(CHOPC);
  r->ofst = addr;

  ix = bfindix(chopcd, r);
  if (ix >= chopcd->num) return 0;

  c = (CINST*) chopcd->ptrs[ix];

  if (addr > c->ofst && addr <= uint (c->ofst + c->opsize))
   { //inside an operand (vect check etc)
    return c;
   }

  return NULL;    // no match

}





uint get_mopcode(INST *dest)
{
  // this is used for a modifiable INST (e.g. printing),
  // so COPY chain entry to dest.
  //ofst set in dest to start with

 CHAIN *chopcd;

 //   if (c->scanblk->proctype & 1) chopcd = datach + ALTOPC;
 // else

     chopcd = datach + CHOPC;

  uint ix;

  ix = bfindix(chopcd, dest);
  if (chopcd->comp(chopcd,ix,dest)) return 0;    // no match

  // this is used for a modifiable INST (e.g. printing),
  // so COPY chain entry to dest.

  memcpy(dest, chopcd->ptrs[ix], sizeof(INST) );
  return 1;
}




CINST *find_opcode (uint ofst, uint up)
{
  // get previous or next opcode start
  // up = 0 for previous, !0 for next

  //this doesn't work if opcode not scanned !!


  CHAIN *x;
  uint ix;
  CINST *c;

  x = get_chain(CHOPC);

  get_copcode(ofst);             // get this opcode

  ix = x->lastix;                //  get_lastix(CHOPC); chain index

 if (up) ix++; else ix--;        // move up or down

  if (ix >= x->num) return 0;    // check valid

  c = (CINST*) x->ptrs[ix];

  if (!bankeq(c->ofst,ofst)) return 0;  // check in same bank

  return c;

}


/*        maybe need this later ?

uint mfind_opcode (INST *dest, uint ofst, uint up)
{
  // get previous or next opcode start
  // up = 0 for previous, !0 for next

  //this doesn't work if opcode not scanned !!


  CINST *c;

  c = find_opcode(ofst,up);

  if (!c) return 0;

  memcpy(dest, c, sizeof(INST) );

  return 1;

}

*/






// general chain finder for all chains



PSW *add_psw(uint jstart, uint pswaddr)
{

  //user cmd only ??
  PSW *x;
  CHAIN *chpsw;
  uint ix;
  int ans;

 //   if (c->scanblk->proctype & 1) chopcd = datach + ALTOPC;
 // else

  chpsw = datach + CHPSW;

  chpsw->lasterr = E_INVA;
  if (!val_rom_addr(jstart))  return 0;
  if (!val_rom_addr(pswaddr)) return 0;


  x = (PSW *) chimem(CHPSW);
  x->jstart = jstart;
  x->pswop  = pswaddr;

  ix = bfindix(chpsw, x);
  ans = chpsw->comp(chpsw,ix, x);

  if (ans)
   {
    chinsert(CHPSW, ix, x);
    chpsw->lasterr = 0;
   }

   #ifdef XDBGX
      if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail");
      DBGPRT(0," pswset %x = %x", jstart, pswaddr);
      DBGPRT(1,0);
     #endif


if (ans) return x;

chpsw->lasterr = E_DUPL;       //duplicate
    chpsw->next = chpsw->num;               //reset temp pointer
return 0;
}

PSW *get_psw(uint addr)
{
  PSW *r;
  uint ix;
  CHAIN *chpsw;

 //   if (c->scanblk->proctype & 1) chopcd = datach + ALTOPC;
 // else

  chpsw = datach + CHPSW;

  r = (PSW*) chsmem(CHPSW);
  r->jstart = addr;

  ix = bfindix(chpsw, r);
  if (chpsw->comp(chpsw,ix,r)) return NULL;    // no match
  return (PSW*) chpsw->ptrs[ix];
}


RBT *add_rbase(uint reg, uint addr, uint rstart, uint rend)
{
  RBT *x, *z;     // val includes bank
  CHAIN *chbase;
  uint ix;
  int ans;

  chbase = datach + CHBASE;

  addr &= 0xfffff;          //strip flags;

  chbase->lasterr = E_INVA;                  // set "invalid addr"

  if (reg  & 1)              return 0;          // can't have odd registers (word addressed)
  if (valid_reg(reg) != 1)   return 0;          // not any special regs
  if (addr > maxadd(addr))   return 0;          // anywhere in address range

  if (rstart && !val_rom_addr(rstart)) return 0;  // range must be within ROM

//but swapping chains may casue phantom rbases ???




  x = (RBT *) chimem(CHBASE);
  x->reg    = reg;
  x->val    = addr;
  x->rstart = rstart;
  x->rend   = rend;


  ix = bfindix(chbase, x);
  ans = chbase->comp(chbase,ix, x);

// if ans=0 duplicate


  // check for overlap here

  z = (RBT *) chbase->ptrs[ix];         // chain block found, nearest below

  // ranges - can be within existing range, or outside as a new one
  // check for overlaps CHANGE <= and >= to < and >  FROM OLCHK COMMANDS

  if (ix < chbase->num)
    {
     int err;

     err = 0;

     if (x->rstart < z->rstart && x->rend   >= z->rstart && x->rend   <= z->rend) err = 1;   //overlaps rstart
     if (x->rend   > z->rend   && x->rstart >= z->rstart && x->rstart <= z->rend) err = 1;   // overlap rear

     if (err)
      {
       chbase->lasterr = E_OVRG;          // overlap ranges

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
    chinsert(CHBASE, ix, x);
    chbase->lasterr = 0;
//    set_rbflg(reg);
   }
  else
   {
     chbase->lasterr = E_DUPL;       //duplicate
         chbase->next = chbase->num;               //reset temp pointer
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
  CHAIN *chbase;
  uint ix;

  chbase = datach + CHBASE;

  r = (RBT*) chsmem(CHBASE);
  r->reg    = reg;
  r->rstart = pc;
  r->rend   = pc;

  ix = bfindix(chbase, r);

  if (ix < chbase->num)
    {
     x = (RBT*) chbase->ptrs[ix];
     if (x->oinv) return NULL;
     if (x->reg == r->reg && r->rstart >= x->rstart && r->rstart <= x->rend) return x;          //   match
    }

return NULL;

}


RST* get_rgstat(uint reg)
{
  uint ix;
  RST *b, *t;
  CHAIN *chrgst;

  chrgst = datach + CHRGST;

 if (valid_reg(reg) != 1) return NULL;        // standard register only

 t = (RST *) chsmem(CHRGST);
 t->reg  = reg;

 ix = bfindix(chrgst, t);

  if (ix < chrgst->num)
   {       // exact match reqd
    b = (RST*) chrgst->ptrs[ix];
    if (b->reg != t->reg) return NULL;
    return b;    // must match to get here
   }

 return NULL;
 }



RST* get_rgstata(uint ofst)
{
  uint ix;
  RST *b, *t;
 CHAIN *chrgsta;

  chrgsta = datach + CHRGSTA;
 //if (valid_reg(reg) != 1) return NULL;        // standard register only

 t = (RST *) chsmem(CHRGST);
 t->argofst  = ofst;

 ix = bfindix(chrgsta, t);
 b = (RST*) chrgsta->ptrs[ix];

 if (ix < chrgsta->num)  return b;


 return NULL;
 }









RST* get_next_rgstat(uint reg)
{

  RST  *t;
  CHAIN *chrgst;

  chrgst = datach + CHRGST;
 if (valid_reg(reg) != 1) return NULL;        // standard register only

 if (chrgst->lastix > chrgst->num) return NULL;

 chrgst->lastix ++;
 t = (RST *) chrgst->ptrs[chrgst->lastix];
 if (t->reg  != reg) return NULL;

 return t;

}






RST* add_rgstat(uint reg, uint argofst, uint cofst)
{

  RST *x;
  CHAIN *chrgst;
  int ix, ans;

  if (valid_reg(reg) != 1)         // standard register only
   {
    #ifdef XDBGX
    DBGPRT (0,"REJECT Reg Inv");
    DBGPRT  (0," rgstat %x from %x", reg, argofst);
    DBGPRT(1,0);
   #endif
    return NULL;
  }

  chrgst = datach + CHRGST;

  x = (RST *) chimem(CHRGST);
  x->reg     = nobank(reg);
  x->argofst = argofst;
  x->cofst   = cofst;

  ix  = bfindix(chrgst, x);
  ans = chrgst->comp(chrgst,ix, x);

 if (ans)
   {
    chinsert(CHRGST, ix, x);

    chrgst = datach + CHRGSTA;
    ix = bfindix(chrgst, x);                 // and in addr chain
    chinsert(CHRGSTA, ix, x);

   }
 else
   {
    x = (RST*) chrgst->ptrs[ix];
    chrgst->next = chrgst->num;               //reset temp pointer
   }


 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT  (0," rgstat %x argofst %x", x->reg, x->argofst);
    DBGPRT(1,0);
   #endif

 return x;

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
  CHAIN *chspf;


  uint ix;
  int ans;

  if (!spf) return NULL;            //safety
//  if (!lev) return NULL;            //safety

  chspf = datach + CHSPF;
//look for last item in chain, and at same time check for duplicates

  s = get_spf(fid);

  while (s)
  {
 //   if (s->spf == spf && s->level == lev && s->fromadd == from)
    if (s->spf == spf && s->fromadd == from)
      {
       chspf->lasterr = E_DUPL;
       #ifdef XDBGX
          DBGPRT(1, "DUPL spf %d to %lx from %x",  spf,  fid, from);
       #endif
       return 0;
       }

    a = get_spf(s);         // get next item
    if (!a) break;          // a is end of chain
    s = a;                  // last valid item
  }

  a = (SPF *) chimem(CHSPF);

//last valid id of chain

  if (!s) a->fid = fid;
  else
    {
        a->fid = s;
    }

  a->spf = spf;
 // a->level = lev;
  a->fromadd = from;

  ix = bfindix(chspf,a);
  ans = chspf->comp(chspf,ix, a);     //chspf->cheq(

  if (!ans)
   {
            #ifdef XDBGX
     DBGPRT(0, "add spf PANIC!!");
         chspf->next = chspf->num;               //reset temp pointer
     #endif
   }
  else
  {
    chinsert(CHSPF, ix, a);

    #ifdef XDBGX
      DBGPRT(1, "Add spf %d to %lx from %x",  spf,  fid, from);
    #endif

  }

  return a;
}
























SPF *get_spf(void* fid)
{
 uint ix;
 SPF *s;
 CHAIN *chspf;

 chspf = datach + CHSPF;

 s = (SPF*) chsmem(CHSPF);
 s->fid = fid;


 ix = bfindix(chspf, s);

 if (!chspf->comp(chspf,ix,s))
 {
   return (SPF*) chspf->ptrs[ix];
 }

chspf->lastix = chspf->num;   // invalid lastix
return NULL;
}


/*
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
       chspf->lasterr = E_DUPL;
       #ifdef XDBGX
          DBGPRT(1, "DUPL spf %d to %lx from %x",  spf,  fid, from);
       #endif
       return 0;
       }

    a = get_spf(s);         // get next item
    if (!a) break;          // a is end of chain
    s = a;                  // last valid item
  }

  a = (SPF *) chmem(CHSPF);

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
  ans = cpspf(&chspf,ix, a);     //chspf->cheq(

  if (!ans)
   {
            #ifdef XDBGX
     DBGPRT(0, "add spf PANIC!!");
     #endif
   }
  else
  {
    chinsert(CHSPF, ix, a);

    #ifdef XDBGX
      DBGPRT(1, "Add spf %d to %lx from %x",  spf,  fid, from);
    #endif

  }

  return a;
}
*/


ADT *get_adt(void *fid, uint bix)
{
    // bix is for subchains on same fid - change to bottom bit of the void ??

    // CANNOT USE CHMEM when blocks built outside search !!

 uint ix;
 ADT *s;
 CHAIN *x;
 x = datach + CHADNL;      //&chadnl;

 // s = (ADT*) chmem(CHADNL);   mot anymore

 s = (ADT*) chsmem(CHADNL); //adtpool;               //entry zero
 // memset(s,0, sizeof(ADT));

 s->fid = fid;
 s->sbix = bix;

 ix = bfindix(x, s);

 if (!cpadt(x, ix,s)) return (ADT*) x->ptrs[ix];

// x->lastix = x->num;   //invalidate if not found not reqd ??
 return NULL;
}


void free_adt(void *fid)
 {

  ADT *a;
  uint ix;
  CHAIN *x;

  x = get_chain(CHADNL);
 while ((a = get_adt(fid,0)))
  {
    //but what about subfields ??? copy from print ??
 //   ix = get_lastix(CHADNL);      // chadnl.lastix;            // ix of a ?
    ix = x->lastix;

    chdelete(datach+CHADNL, ix,1);       //&chadnl,ix,1);         // a->cnt = 0;
    fid  = a;
  }


 }






int cmp_adt (ADT *a, ADT *b)
  {
   // compare adt, but not master key not good...

   // is a checksum of entire string better ??

//cheap way, maybe not good
//   ans = memcmp(x+8,y+8, sizeof(ADT)-8);  /but this is crude.............

// compare each element

// if ( a->fid != b->fid ) return 1;           // not fid

if ( a->fstart != b->fstart ) return 1;
if ( a->fend != b->fend )     return 1;
if ( a->dptype != b->dptype ) return 1;

if ( a->pfw != b->pfw )   return 1;
if ( a->cnt != b->cnt )   return 1;
if ( a->pfd != b->pfd )   return 1;
if ( a->newl != b->newl ) return 1;
if ( a->fnam != b->fnam ) return 1;
if ( a->sbix != b->sbix ) return 1;
if ( a->sub  != b->sub )  return 1;
if ( a->bank != b->bank ) return 1;



 // if ( a->uint prt ) return 1;    not prt flag

   return 0;                // match
 }

ADT* get_last_adt (void *fid, uint bix)
  {
   //look for last item in chain, no duplicate check

   ADT *a, *s;

   s = get_adt(fid, bix);

  if (!s) return NULL;     //empty chain

  while (s)
    {
    a = get_adt(s,0);       // get next item (bix always zero)
    if (!a) break;          // a is end of chain
    s = a;                  // last valid item
    }

  return s;                // last valid fid of chain
 }

//want to be able to prepoulate ADT block and then add it....


 ADT* append_adt (ADT *a)     //, uint sub)

//ADT* append_adt (void *fid, uint sub)
  {
   // add special func block
   // submit preset chain so that duplicate check can be done.

//may do a checksum type rather than each element.

  ADT *s;
  CHAIN *chadnl;
  uint ix;
  int ans;

  if (!a->fid) return NULL;            //safety


//if ((long) ins->fid == 0x924b8)
//{
//    DBGPRT(0,0);

//}


  chadnl = datach + CHADNL;

 // a = (ADT *) chmem(CHADNL);
 // a->cnt    = 1;            //      aargh !!
 // a->fend   = 7;               // single byte default

  s = get_last_adt(a->fid,a->sbix); // last valid item in chain, can return 0

  if (s)  a->fid = s;            //to attach to correct place
//  else    a->fid = fid;

  ix = bfindix(chadnl,a);
  ans = cpadt(chadnl,ix, a);

  if (!ans)
   {
            #ifdef XDBGX
     DBGPRT(0, "PANIC!!");
         chadnl->next = chadnl->num;               //reset temp pointer
     #endif
   }
  else
  {
      //while.....

    chinsert(CHADNL, ix, a);
  }

  return a;
}


 ADT* append_adt_mult (ADT *x)
 {
// multiple insert test.
// may do a checksum type rather than each element.

  ADT *a, *s;
  CHAIN *chadnl;
  uint ix, iy, end, save;
  int ans;



  if (!x->fid) return NULL;            //safety


//if ((long) ins->fid == 0x924b8)
//{
//    DBGPRT(0,0);

//}


  chadnl = datach + CHADNL;

  end = chadnl->next;           //where to stop

  save = chadnl->num;          //in case of error
  iy =  chadnl->num;

  a = (ADT*) chadnl->ptrs[iy];         //first chimem block
  s = get_last_adt(a->fid,a->sbix); // last valid item in chain, can return 0

  if (s)  a->fid = s;            // to attach to correct place

  while (iy < end)
  {

    a = (ADT*) chadnl->ptrs[iy];             // same as *x first time
  ix = bfindix(chadnl,a);
  ans = cpadt(chadnl,ix, a);

  if (!ans)
   {
                chadnl->num = save;
            #ifdef XDBGX
     DBGPRT(0, "PANIC!!");

         chadnl->next = chadnl->num;               //reset temp pointer
         break;                                //?? reset x->num and x->next?
     #endif
   }
  else
  {
      //while.....

    chinsert(CHADNL, ix, a);
  }

  iy++;
  }
  return a;
}



LBK *get_cmd (uint start, uint fcom)
{
  // returns match if ofst within range of a data block (start-end)
  LBK *blk;
  CHAIN *chcmd;
  int ix;

  chcmd = datach + CHCMD;

  blk = (LBK*) chsmem(CHCMD);
  blk->start = start;
  blk->fcom =  fcom;

  ix = bfindix(chcmd, blk);

  // if command SPANs this address, will not always be selected.

//  while (ix < chcmd->num)
//    {
   //  blk = (LBK*) chcmd->ptrs[ix];
//     if (p->end < start) return NULL;



  //    if (p->start <= start && p->end >= start) return start = 0;
    //            }
      //        if (start) DBGPRT(1,"push %x from %x",start, d->ofst);
        //     }





  if (eqcmd(chcmd,ix,blk)) return NULL;    // no match

  return (LBK *) chcmd->ptrs[ix];
}

LBK *get_aux_cmd (uint start, uint fcom)
{
  // returns match if ofst within range of a data block (start-end)
  LBK *blk;
  CHAIN *chaux;
  int ix;

  chaux = datach + CHAUX;

  blk = (LBK*) chsmem(CHAUX);
  blk->start = start;
  blk->fcom =  fcom;

  ix = bfindix(chaux, blk);

  if (eqcmd(chaux,ix,blk)) return NULL;    // no match

  return (LBK *) chaux->ptrs[ix];
}






/*
RST *get_rgarg(uint reg, uint addr)
{
  uint i;
  RST *r, *ans;

  ans = 0;
  for (i = 0; i < chrgst->num; i++)
  {
    r = (RST*) chrgst->ptrs[i];
//    if (r->arg && r->sreg == reg)
      if (r->ofst == addr)
    {
        ans = r;
        break;
    }
  }
        return ans;
}
*/

/*
SBK *get_scan(uint ofst)
{
 SBK *x, *ans;
 int ix;

  x = (SBK*) chmem(&chscan);      // dummy block for search
  x->start = ofst;                // symbol addr
                                  // would check substart if set
  ix = bfindix(&chscan, x);
  if (chscan.comp(&chscan,ix,x)) return NULL;

  return (SBK *) chscan.ptrs[ix];

}*/


SBK *get_scan(uint addr, uint subaddr)
{

 SBK *s;
 CHAIN *chscan;
 int ix;

 chscan = datach + CHSCAN;

 s = (SBK*) chsmem(CHSCAN);
 s->start = addr;
 s->substart = subaddr;       //can be zero


 ix = bfindix(chscan, s) ;

 if (!chscan->comp(chscan,ix,s)) return (SBK*) chscan->ptrs[ix];

 chscan = datach + CHSCAN;

 ix = bfindix(chscan, s) ;

 if (!chscan->comp(chscan,ix,s)) return (SBK*) chscan->ptrs[ix];



 return NULL;



}



SUB *get_subr(uint addr)
{
  SUB *s;
  CHAIN *chsubr;
  uint ix;

  chsubr = datach + CHSUBR;

  if (!addr) return 0;         // shortcut..
  s = (SUB*) chsmem(CHSUBR);
  s->start = addr;

 ix = bfindix(chsubr, s);
 if (chsubr->comp(chsubr,ix,s)) return NULL;    // no match
 return (SUB*) chsubr->ptrs[ix];
}



//used for gap scans

uint get_dtdo_ix (uint ofst)
{
 uint ix;
 DTD *s;
 CHAIN *chdtdo;

//may be more than one data address for an opcode.
//this should return first one

chdtdo = datach + CHDTDO;

 s = (DTD*) chsmem(CHDTDO);     //always master chain
 s->ofst = ofst;

 ix = bfindix(chdtdo, s);      //ofst chain

if (ix < chdtdo->num)
 {
  s = (DTD*) chdtdo->ptrs[ix];
  if (ofst == s->ofst) return ix;
 }

chdtdo->lastix = chdtdo->num;   //invalidate if not found
return chdtdo->num;

}


/*
uint get_dtdd_ix (uint ofst)
{
 uint ix;
 DTD *s;

//may be more than one opcode for data address.
//this should return first one

 s = (DTD*) chmem(CHDTKD);     //always master chain
 s->dataptr = ofst;

 ix = bfindix(&chdtdd, s);      //ofst chain

if (ix < chdtdd.num)
 {
  s = (DTD*) chdtdd.ptrs[ix];
  if (ofst == s->dataptr) return ix;
 }

chdtdd.lastix = chdtdd.num;   //invalidate if not found
return chdtdd.num;

}


DTD *get_dtkd(uint ch, uint ofst, uint start)
{
 uint ix;
 DTD *s;

CHAIN *x;

// chdtko   index by ofst
// chdtkd   index by start [REMOTE]

x = get_chain (ch);

 s = (DTD*) chmem(CHDTKO);     //always master chain
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

*
DTD *start_dtdk_loop(uint ofst)
{
 DTD *a;
  a = (DTD*) chmem(&chdtko);
  if (a) a->ofst = ofst;
 return a;
}
/


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

 a = (DTD*) chmem(CHDTKO);
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

*/

/*
DTD* add_dtkd (TRK *k, INST *c, int start)
  {

  DTD *a, *x;
  uint ix;
  int ans;
  OPC *ptab;

  ptab = get_opc_entry(c->opcix);
  a = (DTD *) chmem(CHDTKO);

  a->ofst    = k->ofst;
  a->stdat = start;

  a->psh   = k->psh;
  a->inc   = k->ainc;
  a->bsze = bytes(ptab->fend[1]);                             //[c->opcix].fend[1]);

 //if (c->opcsub == 1)  a->reg = c->opr[c->wop].addr;

 //if (c->opcsub == 2)  a->reg = c->opr[4].addr;

  if (c->opcsub == 3) a->off = c->opnd[0].addr;


  if (get_cmd(start,C_CODE)) a->olp = 1;

  a->opcsub = k->opcsub;

  //more to go here
  if (c->opcsub == 3)
    {
     if (c->opnd[1].reg == 0) a->opcsub = 1;    // R0, make an imd
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
    chinsert(CHDTKO, ix, a);
    ix = bfindix(&chdtkd,a);
    chinsert(CHDTKD, ix, a);
   }
  else
  { // exists already.  Clear it and reset for use. may not be necessary ?
   return NULL;
  }

  chdtko.lastix = chdtko.num;        //must reset any queries
  return a;
}






*/

uint valid_dtd(uint addr)
{
  if (valid_reg(addr)) return 0;
  if (addr > maxadd (addr)) return 0;
  return 1;
}


void add_dtr(SBK *s, INST *c)
 {

  OPER *o;
  uint i,addr;


if (!s->proctype) return;

if (s->nodata) return;

//called by upd_watch
  // no direct or imd. imd would be in an [Rx] later on if an address from imd. BUT MAYBE NOT !!

  // NB. stx is swopped by the time gets here

  if (c->opcsub < 2) return ;       // imd or reg-reg BUT this could be an address or limit as imd !!
  //better tables and funcs without the imds...

  // multimode only, others can't read or write outside registers

  if (c->opcode < 0x40 or c->opcode > 0xcf) return;

  //ignore RAM at the moment, ROM only


 for (i = 1; i < c->numops; i++)
   {
    o = c->opnd + i;
    addr = o->addr;

    if (!valid_dtd(addr) && o->optype == OPINX) addr = c->opnd[0].addr;   // use index base

    if (!valid_dtd(addr)) addr = 0;

    if (o->optype == OPIMD && nobank(addr) < PCORG) addr = 0;      // only allow imd with ROM addrs

    if (addr)
      {
        DTD *a ;
        uint ix;
        int ans;
        CHAIN *x, *y;

        x = get_chain(CHDTDO);
        y = get_chain(CHDTDD);

        a = (DTD *) chimem(CHDTDO);

        a->ofst    = c->ofst;
        a->dataptr = addr;
        a->opcix   = i;
        a->optype  = o->optype;
        a->fend    = o->fend;

        if (s->tstvect) a->test = 1;

        ix = bfindix(x,a);           //ofst is real chain
        ans = x->comp(x,ix, a);

        if (ans)
          {
           chinsert(CHDTDO, ix, a);
           ix = bfindix(y,a);
           chinsert(CHDTDD, ix, a);        // dataptr is remote

           #ifdef XDBGX
             DBGPRT(0,"Add dtrack %x, %x", a->ofst, a->dataptr);
             if (a->test) DBGPRT(0, " TEST");
             DBGPRT(1,0);
           #endif
          }
        else
         { // exists already.  Clear it and reset for use. may not be necessary ?
           x->next = x->num;               //reset temp pointer
         }

        x->lastix = x->num;        //must reset any queries
        y->lastix = y->num;
      }         //end insert


   }     //end for


}



BNKF *add_bnkf(uint add, uint pc)
{
  CHAIN *x;
  BNKF *b;
  uint ix;
  int ans;

  x = datach + CHBKF;        //&chbkf;         //chindex[CHBNK];

  b = (BNKF *) chimem(CHBKF);
  b->filestart = add;
  b->pcstart   = pc;

  ix = bfindix(x,b);

  ans = x->comp(x, ix,b);

  if (!ans)
    { // exists already.
        x->next = x->num;               //reset temp pointer
     return NULL;
    }
  else
   {
    chinsert(CHBKF,ix,b);
   }

return b;
  }



void fsym(void *x)
{            // free symbol
 SYM *s;

 s = (SYM*) x;
 if (s->symsize)  mfree (s->name, s->symsize+1);              // free symbol name
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


void fmnam(void *x)
{           // malloced blocks tracker (free'd last)
  MATHN *n;
  n = (MATHN*) x;
  if (n->nsize) mfree(n->mathname, n->nsize+1);
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
  CHAIN *chjpt;       //, *chjpf;
  uint ix;
  JMP *t;

  if (x->jtype != J_STAT) return;

 // chjpf = datach + CHJPF;
  chjpt = datach + CHJPT;

  t = (JMP*) chsmem(CHJPF);     //use forward chain, can't use backward
  t->toaddr = x->fromaddr;
  ix = bfindix(chjpt, t);     // TO chain, finds first match

  while (ix < chjpt->num)
   {
    t = (JMP *) chjpt->ptrs[ix];     // match
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

CHAIN *chjpf;
  uint ix, zx;
 // uint initofst;             // where initialise is.
//  uint tstart, tend, fstart, fend;    // have to use indexes here as interleaved scan

//  uint excnt;
  uint tend;
  JMP *f, *t;

  // first, check for jumps to return opcodes
  // true ret has jtype 0

  chjpf = datach + CHJPF;

  for (ix = 0; ix < chjpf->num; ix++)
    {         // down list of forward jumps
     f = (JMP*) chjpf->ptrs[ix];

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

  for (ix = 0; ix < chjpf->num; ix++)          //unsigned !!
    {
     f = (JMP*) chjpf->ptrs[ix];                // down full list of forward jumps


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

           while (zx < chjpf->num)
             {      // inside -> outside (same chain)

              t = (JMP*) chjpf->ptrs[zx];
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






LBK* add_aux_cmd (uint start, uint end, uint com)
 {
  LBK *n;
  CHAIN *x;

  // currently XCODE and ARGS go in here.

  if (get_anlpass() >= ANLPRT)    return NULL;

  //  verify start and end addresses

  if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

  // can get requests for 0xffff with bank, which will break boundary....

  x = get_chain(CHAUX);

  if (!bankeq(end, start))
    {
      #ifdef XDBGX
     // if (nobank(start) < 0xffff) to drop warning
      x->lasterr = E_BKNM;
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

// x = get_chain(ch);

  n = (LBK *) chimem(CHAUX);

  n->start = start;
  n->end   = end;
  n->fcom  = com & 0x1f;    // max 31 as set
  if (com & C_USER) n->usrcmd = 1;
 // if (com & C_SYS) n->sys = 1;

  x->lasterr = inscmd(CHAUX,n);            //is this reqd with only aux cmds ? (xcode and args)


  #ifdef XDBGX
   if (x->lasterr) DBGPRT(0, "FAIL ");
   DBGPRT (0,"add aux ");
   DBGLBK(1,n);
  #endif

  if (x->lasterr) return 0;
  return n;   // successful
 }


SUB *add_subr (SBK *s,uint addr)
{
  // type is 1 add name, 0 no name
  // returns valid pointer even for duplicate

  int ans, ix;
  CHAIN *x;
  SUB *b;

  if (get_anlpass() >= ANLPRT) return NULL;

  x = get_chain(CHSUBR);

  if (!val_rom_addr(addr))
     {
       x->lasterr = E_INVA;
       return NULL;
     }

  if (get_aux_cmd(addr, C_XCODE))
     {
       #ifdef XDBGX
       DBGPRT(1,"XCODE bans add_subr %x", addr);
       #endif
      x->lasterr = E_XCOD;
      return NULL;
     }


// insert into subroutine chain



  b = (SUB *) chimem(CHSUBR);
  b->start = addr;
  if (s && s->tstvect) b->test = 1;

  ix = bfindix(x, b);
  ans = cpsub (x,ix,b);

  if (ans)
   {
    chinsert(CHSUBR, ix, b);       // do insert
    x->lasterr = 0;
 //   if (get_cmdopt(OPTCMPA)) b->cptl = 1;         // set compact layout
   }
  else
  {
   // match - do not insert. Duplicate.
   b = (SUB*) x->ptrs[ix];
   x->lasterr = E_DUPL;
       x->next = x->num;               //reset temp pointer
  }

  #ifdef XDBGX
    if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Dup");
        DBGPRT(0," sub %x",  addr);
        if (b->test) DBGPRT(0, " TEST");
        DBGPRT(1,0);
  #endif
  return b;
  }


// test proc....


SBK *copy_scanblk (SBK *caller, uint ch)
{

 /* copy scan blk from std to escan or test chain.
  * no subr to add here
  */

   SBK *s;
   CHAIN *chn;

   chn = datach + ch;

   int ans, ix;

   if (get_anlpass() >= ANLPRT)   return 0;

   s = (SBK *) chimem(ch);            // new block (zeroed)
   s->start    = caller->start;       // to chain into right place

   ix = bfindix(chn, s);
   ans = cpscan(chn,ix,s);             // checks start address ONLY

   if (!ans)
    {                                  // match - duplicate
     s = (SBK*) chn->ptrs[ix];         // block which matches
     chn->next = chn->num;             // reset temp pointer
  //   return s;                         // ?? if change from overwite...
    }
   else
    {        // no match - do insert (s is valid pointer)
     chinsert(ch, ix, s);
    }

   // copy the caller block, update below. This doesn't update other details.
   // but will pass 'test' flag
   *s = *caller;
  return s;
}


//this seems to work foe emulate..... stage 1

SBK* copy_scanchain(SBK* base, uint addr, uint ch)

{
     SBK *newb, *x, *y;
   // build parallel chain for emulate or test.

   // Blk copies (copy-scanblk) have incorrect caller which is fixed in this loop.
   // but susequent ones called from add_scan (with address) will have correct callers.

   // base is scan blk where emulate originates, or caller of vect/push to test


   if (ch == CHEMUL)         // for emulate - restart scan
    {
      newb = copy_scanblk(base,ch);      // copy base block for emu (this is actual emu start)
      if (!newb)  return 0;
      newb->curaddr  = base->start;
      newb->nextaddr = base->start;      // first emu block must start over.
      newb->proctype = 4;                // emulating
      newb->type     = 6;                // subr emulate call
      newb->regstat  = 1;                // keep register sizes
      newb->stop     = 0;
      newb->inv      = 0;                // and always reset for emu rescan
    }


 /* if (ch == ALTSCAN)
    {                        // which could come from an emulate....
      //add a new test scan, with base as caller.

     newb = add_scan (addr, J_SUB|C_ALT, base);  // C_ALT TEST mode
     if (!newb)  return 0;

     if (base->proctype & 4) newb->proctype = 4;         // keep emulate mode even if testing
     newb->proctype |= 1;                                // add test mode
    }
*/
   //now go 'up' through caller chain

   base = newb;                      // for answer, first block

   x = base->caller;                    // next scan block (in)
   y = newb;                          // start of new copy chain

   while (x)
    {
      newb = copy_scanblk(x, ch);   // copy of scanblock to emuchain with correct caller
      y->caller = newb;             // update caller
      newb->proctype = y->proctype ;
      newb->regstat  = y->regstat  ;                         // keep register sizes
      y = y->caller;                 // move to new emu chain end
      x = x->caller;                 // and loop for next caller
    }
return base;
}




 SBK *add_scan (uint add, int type, SBK *caller)
{

 /* add code scan.  Will autocreate a subr for type "SUBR"
  * updates type etc even for duplicates to fit 'rescan' type model
  * C_CMD (+32) = by user command (can't be changed)
  * caller is only set in calls from do-sjsubr and taks lists (vect)
  * uses an 'alt' flag to swich chains either from call itself, or fom caller
  */

   SBK *s;
   CHAIN *x;

   int ans, ch;
   uint ix;

   if (get_anlpass() >= ANLPRT)   return 0;

   if (!val_rom_addr(add))
    {
  //   x->lasterr = E_INVA;
     #ifdef XDBGX
       ix  = (nobank(add));   // reserved addresses
       if (ix >= 0xd000 && ix <= 0xd010) return NULL;
       if (ix == 0x1f1c || ix == 0x1000 || ix == 0x1800) return NULL;
       DBGPRT (1,"Invalid scan %x", add);
     #endif

     return NULL;
    }


   if (get_aux_cmd (add, C_XCODE))
     {
      #ifdef XDBGX
      DBGPRT(0,"XCODE bans scan %x", add);
      DBGPRT(1,0);
      #endif
  //    x->lasterr = E_XCOD;
      return NULL;
     }

   if (get_cmdopt(OPTMAN))
      {
       #ifdef XDBGX
       DBGPRT(1,"no scan %x, manual mode", add);
       #endif
       return NULL;
      }



   ch = CHSCAN;

   x = get_chain(ch);

   // TEMP add a safety barrier ...
   if (x->num > 20000) return 0;

   s = (SBK *) chimem(ch);          // new block (zeroed)

if (s == caller)
{
   // DBGPRT(1,"PANIC - caller = start %x", add);
    x->next = x->num;      //must reset pointer
    return 0;
}


   s->start    = add;
   s->curaddr  = add;                    // important !!
   s->nextaddr = add;
   s->proctype = 2;                     // scanning


   if (type & C_USER)  s->usercmd = 1;         // added by user command
   if (type & C_TEST)  s->tstvect = 1;         // test mode

   s->type     =  type & 0x1f;

   if (caller)
      {
       s->psw       = caller->psw;
       s->substart  = caller->substart;       // preserve substart
       s->caller    = caller->caller;         // default to same level (= subroutine)
       if (caller->tstvect)  s->tstvect   = caller->tstvect;        // keep test flag

       if (s->start == caller->start)
  {
       #ifdef XDBGX
          DBGPRT (1,"REJECT - caller->start %x == s->start", s->start);
       #endif

 x->next = x->num;      //must reset pointer
 return 0;  }          // safety check for loops
      }

   if (s->type == J_SUB)                   // maybe not if alt scan ??
    {
      if (caller) s->caller = caller;     // new level for new subr
      add_subr(s,s->start);                 // **********************
      s->substart =  s->start;            // new subr, new substart

      ans = 1;
      if (s->tstvect) ans |= C_TEST;
      add_autosym(ans,s->start);            // add name (autonumbered subr)
    }


//could change chain here on basis of flag (for gaps and vects) ???


//if alt check MAIN chain as well, for duplicates.... or maybe just opcodes....


   ix = bfindix(x, s);
   ans = cpscan(x,ix,s);                     // checks start address and substart




   if (!ans)
    {                                     // match - duplicate

     SBK *t;
     t = (SBK*) x->ptrs[ix];          // block which matchesadd_scan

     if (s->caller && t->caller == 0)
       {   // was a user or system added scan,and now has caller
        *t = *s;       // replace
       #ifdef XDBGX
       DBGPRT (0,"Replace");
           if (s->tstvect) DBGPRT(0, " TEST");
       DBGPRT  (0," scan %05x %s", s->start, jtxt[s->type]);
       if (s->substart)  DBGPRT (0," sub %x", s->substart);
       if (s->caller) DBGPRT(0, " caller %x", s->caller->start);
       else DBGPRT(0," No caller");
       #endif
       if (ix < x->lowins)  x->lowins = ix;       // reset lowest scan index
          x->next = x->num;               //reset temp pointer - must do
       return t;
       }
     s = (SBK*) x->ptrs[ix];          // block which matches
     x->lasterr = E_DUPL;
     x->next = x->num;               //reset temp pointer - must do
    }
   else
    {        // no match - do insert

     chinsert(ch, ix, s);
       if (ix < x->lowins)  x->lowins = ix;       // reset lowest scan index

   //  if (s->substart)                       // scan all blks within subr
   //    {
   //     ix = bfindix(&chsbcn, s);           // and insert in subscan chain
   //     chinsert(CHSBCN, ix, s);
  //     }

   //  if (s->gapscan)
    //   {
    //    ix = bfindix(&chsgap, s);           // and insert in gap chain
    //    chinsert(CHSGAP, ix, s);
    //   }
     x->lasterr = 0;
    }


 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
              if (s->tstvect) DBGPRT(0, " TEST");
    DBGPRT  (0," scan %05x %s", s->start, jtxt[s->type]);
    if (s->substart)  DBGPRT (0," sub %x", s->substart);
    if (s->caller) DBGPRT(0, " caller %x", s->caller->start);     else DBGPRT(1," No caller");

    if (!ans)
    {      //dupl
//    if (!s->stop) DBGPRT(0," Not");
    if (s->stop) DBGPRT(0, " Scanned");
    if (s->argsget) DBGPRT(0," ARGS!");
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
  //no subr to add here
  */

   SBK *s;
   CHAIN *chemul;

   chemul = datach + CHEMUL;

   int ans, ix;

   if (get_anlpass() >= ANLPRT)   return 0;

   s = (SBK *) chimem(CHEMUL);            // new block (zeroed)
   if (add) s->start = add;                // standard add emu
   else s->start    = caller->start;       // copy of SBK to new emu for chain in right place

   ix = bfindix(chemul, s);
   ans = cpscan(chemul,ix,s);             // checks start address ONLY

   if (!ans)
    {                                     // match - duplicate
     s = (SBK*) chemul->ptrs[ix];          // block which matches
         chemul->next = chemul->num;               //reset temp pointer
    }
   else
    {        // no match - do insert (s is valid pointer)
     chinsert(CHEMUL, ix, s);
    }



/*
   if (!add)         never true anow have copy proc instead
    {
      // just copy the caller block, update below.  !!! BUT this doesn't update 'caller' field for chain....
      *s = *caller;


       s->proctype = 4;                              // emulating;
    }
   else  */
    {
     // this add is ALWAYS a subr call in EMULATE mode, so caller will always be set
      s->caller   = caller;
      s->substart = s->start;               // new subr
      s->curaddr  = s->start;
      s->nextaddr = s->start;
      s->proctype = caller->proctype;     //continue proctype down the chain

    }

//if C_ALT)  if (type & C_ALT)  pt = 5;             //  test+emu

   s->type     = 6;                         // subr emulate call
   s->regstat  = 1;                         // keep register sizes
   s->stop     = 0;
   s->inv      = 0;                             // and always set for emu rescan

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

// move fstart to correct byte addr, and fend to match
// keep sign,write,nobit flags (0xe0)


//change rules FOR SFRS if valid_reg() == 3 ?  no address shuffle

  flags = (*fend) & 0xe0;   // sign,write, nobit flags

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




uint do_olap(SYM *t, SYM* xnew)
 {
   //  ans 0 no dupl, error if dupl/overlap
   if (t->addr != xnew->addr)      return 0;                     // address, OK
   if ((t->fend & 0xe0) != (xnew->fend & 0xe0)) return 0;        // write, sign, nobit

   // fields can overlap but not be identical
   if (xnew->fstart != t->fstart) return 0;
   if (xnew->fend   != t->fend)   return 0;

   // so this is a duplicate sym if it gets here, now do range check.
   // 'no range' (0 - 0xfffff) for both gets caught in next statement

   if (xnew->rstart == t->rstart && xnew->rend == t->rend) return E_DUPL;


   if (xnew->rstart < t->rstart && xnew->rend > t->rend) return 0;         // spans OK.
   if (xnew->rstart > t->rstart && xnew->rend < t->rend) return 0;         // contained, OK
   if (xnew->rend   < t->rstart) return 0;                            // before
   if (xnew->rstart > t->rend)   return 0;                            // after

   return E_OVRG;

 }

uint check_sym_overlaps(CHAIN *x,  uint ix, SYM* xnew)
{
   // t is chained, xnew is new insert
   // bit fields can overlap so this is a duplicate check
   // followed by a range check

   //have to check against previous entry as well for adding at end....
   SYM *t;
   uint ans, iy ;

   ans = 0;
                                //backward checker
   iy = ix;
   while (iy < x->num)           // will work for -1
    {
      t = (SYM *) x->ptrs[iy];
      if (t->addr != xnew->addr) break;
      ans = do_olap(t, xnew);
      if (ans) break;              // fails
      iy--;
    }
   if (ans) return ans;

   while (ix < x->num)           //forward checker
    {
      t = (SYM *) x->ptrs[ix];
      if (t->addr != xnew->addr) break;
      ans = do_olap(t, xnew);
      if (ans) break;              // fails
      ix++;
    }

  return ans;

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




void fixbitsym(SYM *s, uint add, uint fstart, uint fend)
{
  if (valid_reg(s->addr) == 3) return;      //not if SFR

  fixbitfield(&add,&fstart, &fend);         //just keeps any flags....

  s->addr   = add;
  s->fstart = fstart;
  s->fend   = fend;
  s->fmask  = fmask(fstart, fend);         // set up field mask for later use

}




SYM *add_sym  (CSTR *fnam, uint add, uint fstart, uint fend, uint rstart, uint rend)

 {
   // chain for symbols bitno = -1 for no bit
   // fend as OPS as field end plus sign (+32) plus write (+64)

   SYM *s, *t;
   int ans;
   uint ix,ch;
   CHAIN *x;

   /* fend has several flags

    #define C_WRITE    0x40     // write flag
    #define C_WHOLE    0x80     // 'whole' flag for symbols

    #define C_USER     0x200     // by user command (can't change or merge)
    #define C_SYS      0x400     // for system 'base generated' cmds
    #define C_RENAME   0x800     // rename of symbol is allowed, even if duplicate
    #define C_TEST     0x200000    // for verify scan from VECT, GAP, etc) add to addresses

*/

   ch = CHSYM;   //&altsym; else ch = &chsym;


   x = get_chain(ch);

   add &= 0xfffff;

   if (!fnam) return 0;           // zero string
   if (!fnam->len) return 0;      // zero string length.

 s = (SYM *) chimem(ch);

   ///map into byte addressed unless it's an overlapped SFR
  // NB check is in fixbitsym

  // in fixbit sym, move here *

  if (fend & (C_SIGN|C_WRITE))   s->pbkt   = 1;       // sign or write set
  if (fend & C_USER)             s->usrcmd = 1;       // by user command
  if (fend & C_SYS)              s->sys    = 1;       // system generated
  if (fend & C_TEST)             s->test    = 1;      // from test scan
  if (!(fend & C_NOBIT))         s->pbkt = 1;         // has bitfield

  fixbitsym (s, add, fstart, fend);

   s->rstart  = rstart;                    //range start and end
   s->rend    = rend;

   ix = bfindix(x, s);

   ans = x->comp(x, ix, s);              // zero if matches

// Note - !ans is an exact duplicate,
// by address ,fstart ,fend, range start addr,
// but not end range.   addresses are byte
// for inserts, check range even if answer not zero.

   t = (SYM *) x->ptrs[ix];         // map chain block if found

   x->lasterr = check_sym_overlaps(x, ix, s);

   if (x->lasterr == E_OVRG)
          {
            #ifdef XDBGX
             DBGPRT(0,"add sym %x",add);
             DBGPRT(0," %c%s%c " ,'"',fnam->string, '"');
             DBGPRT(0,"range overlap");
             DBGPRT(0," (%d) with ", x->lasterr);
             DBGPRT(0," %c%s%c " ,'"',t->name, '"');
             DBGPRT(1,0);
            #endif
                x->next = x->num;               //reset temp pointer
             return 0;
          }



   if (!ans)
      {       // duplicate
              // check for range overlaps first (DUPL or OVRG)
              // allow rename if flag set


    x->next = x->num;               //reset temp pointer

       //OK, can rename a 'sys' autoname, but NEVER a user one (in new_symname).
       if (x->lasterr == E_DUPL)
         {




           if ((fend & C_RENAME) && new_symname (t, fnam))     // allow name replace, will reset error
         {
              x->lasterr = 0;
              if (fend & C_USER)
                {
                 s->usrcmd = 1;       // by user command
                 s->sys = 0;          // if renaming auto sysname
                }
           #ifdef XDBGX
            DBGPRT(0,"add sym %x",add);
            DBGPRT(0," Dup rename %x",add);
            DBGPRT(1," from  %s to %s" ,t->name,fnam->string);
           #endif
         }
       else
       {
         x->lasterr = E_DUPL;

       }
         }
       return t;
     }          //end duplicate



     // do insert, attach name first
     new_symname (s, fnam);

     chinsert(ch, ix, s);

     #ifdef XDBGX
      DBGPRT(0,"add sym %x",add);
      if (fnam) DBGPRT(0," %c%s%c " ,'"',s->name, '"');
      if (s->test) DBGPRT(0," TEST");
      if (s->rstart)      DBGPRT (0," %05x - %05x" , s->rstart, s->rend);
      DBGPRT (0," B%d %2x", s->fstart, s->fend);
      if (s->fend & C_WRITE)  DBGPRT(0," write");        //s->writ
      if (s->fend & C_NOBIT)  DBGPRT(0," nobit");
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
  CHAIN *chsym;
  SYM *s, *t;
  uint ix, end;

  chsym = datach + CHSYM;

  list = (SYMLIST*) mem (0, 0, sizeof(SYMLIST));
  list->defwr = chsym->num+1;
  list->defrd = chsym->num+1;
  list->startix = chsym->num+1;
  list->endix   = chsym->num+1;

  list->fend = fend;

  list->bcnt = 0;                // count of bitfield syms found
  list->wcnt = 0;                // count of 'whole' syms found

  end = add + bytes(fend);          //  end address, widest possible

  s = (SYM*) chsmem(CHSYM);         // block for search

  s->addr   = add;
  s->fend   = 0xff;                // largest possible fend
  s->rstart = 0;
  s->rend   = 0xfffff;             // range required for binary search..........

  ix = bfindix(chsym, s);

  list->startix = ix;              //first matching entry

  while (ix < chsym->num)
    {

     t = (SYM*) chsym->ptrs[ix];        // first entry found

     if (t->addr >= end)
        {
         break;          //return list;             // address not match, or end of chain
        }

     if ((fend & C_NOBIT) && !(t->fend && C_NOBIT)) break;    // no bit fields allowed if nobit reqd.

     if (pc >= t->rstart && pc <= t->rend && !t->inv)
       {  // sym range covers pc, deflt read in [0] , deflt write in [1]

           // in order in list -
           //'whole' write,  whole read,
           // largest field write (from zero) , then read
           // start bit write, read

           if ((t->fend & C_NOBIT) && t->addr == add)           //must have addr check for default
             {  // whole symbol defaults
                list->wcnt++;           //whole sym found
                if (t->fend & C_WRITE)
                   {     // write
                     if (list->defwr > chsym->num) list->defwr = ix;
                   }
                else
                  {     // read
                     if (list->defrd > chsym->num) list->defrd = ix;
                  }
             }
          if (!(t->fend & C_NOBIT))  list->bcnt++;     // bitfield
       }

     list->endix = ix;
     ix++;                  //next sym

    }

if (list->endix > chsym->num)       //or use bcnt + wcnt ??
     {       //no matches
         mfree(list,sizeof(SYMLIST));
         return NULL;
     }
    // read exists but no write ?? assign write to read as default
    if (list->defwr > chsym->num && list->defrd < chsym->num) list->defwr = list->defrd;

  return list;
}




SYM* get_sym(uint add, uint fstart, uint fend, uint pc)
{

    SYM *s;
    SYMLIST *list;
    uint i, ans;

   CHAIN* chsym;
   chsym = datach + CHSYM;

   ans = chsym->num;

//get_list must operate before fixbit if overlap - how to do this ??
//or is this OK, and it's only bitwise....
//but CANNOT move base addr for search if it's a high bit num of a word op !!!!


 fixbitfield(&add, &fstart, &fend);     // adjust pars to byte based

   list = get_symlist(add, fend, pc);

   if (!list) return NULL;

   // defaults in defwr, defrd, not set is >chsym->num

    i = list->startix;

    while (i <= list->endix)
      {
        s = (SYM*) chsym->ptrs[i];

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

            if (fend & C_WRITE)
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

     if (ans >= chsym->num)
       {
          // no match if it got here, so use default
        if ((fend & 0x40) && list->defwr < chsym->num) ans = list->defwr;   // default write sym
        else if (list->defrd < chsym->num) ans = list->defrd;               // default sym (read or write)
       }


   if (ans < chsym->num) s = (SYM*) chsym->ptrs[ans];   else s = NULL;

   mfree(list,sizeof(SYMLIST));
   return s;
 }



MATHX *get_mterm(void *fid, int pix)
{
 uint ix;
 MATHX *m;
 CHAIN *x;

  x = datach + CHMATHX;

 m = (MATHX*) chsmem(CHMATHX);
 m->fid = fid;
 m->pix = pix;    // and param index

 ix = bfindix(x, m);

 if (!x->comp(x, ix,m))
 {
   return (MATHX*) x->ptrs[ix];
 }

x->lastix = x->num;   //invalidate if not found
return NULL;
}


void chk_mx(void *fid, uint ix, uint *chks)
{
  uint i;
  MATHX *a;
  MNUM *n;
  // just add everything as first try
  // multiply by term number as extra piece

  while ((a = get_mterm(fid,ix)))
   {
       // (*chks) *= 31;     // multiply each successive math term by 31...
    fid = a;
    for (i = 0; i < a->npars; i++)
      {    //pars first
         n = a->fpar + i;
        if (n->dptype == DP_SUB) chk_mx(fid,i+1,chks);   // sub instead of par
        else
         {
           *chks += n->ival;
           *chks += n->dptype;
           (*chks) *= (i+1);
         }
      }

    *chks += a->func;
    *chks += a->calctype;
    *chks += a->npars;
    *chks += a->pix;
    *chks += a->lfirst;
    *chks += a->mfirst;

  }
}









//make same as adt version....

MATHX* add_mterm (MATHX *a)                  //void *fid, int pix)
  {
   // add additional data block with fid as key

   // sys embedded ones have name of function........for now.
   // allow fid = zero, so a->fid = a + 5 as dummy key
   // then link name IN to a ?

 // MATHX *a;
  uint ix;
  long ans;
  CHAIN *x;

  x = datach + CHMATHX;

 // a = (MATHX *) chimem(CHMATHX);


//could use x->num as fid, but then need some other way to compare math funcs...
//or use the adt where it attaches, saving a link ? but then can't have many-many, so still need link...


//temp fixup for testing

if (!a->fid)  a->fid = vconvi(x->num+10);




 // a->fid = fid;    //parent
 // a->pix = pix;    // and param index

  ix = bfindix(x,a);

  ans = x->comp(x, ix, a);

  if (!ans)

  { // exists already.  Clear it and reset for use. may not be necessary ?
   x->lasterr = E_DUPL;
       x->next = x->num;               //reset temp pointer
   // memset ?
   return (MATHX *) x->ptrs[ix];      //   return NULL;
  }

    chinsert(CHMATHX, ix, a);

  x->lastix = x->num;        //must reset any queries
  return a;
}


MATHX* add_mterms (MATHX *a)
 {
// multiple insert test.
// may do a checksum type rather than each element.

  CHAIN *x;
  uint ix, iy, end, save;
  int ans;


 x = datach + CHMATHX;
if (!a->fid)  a->fid = vconvi(x->num+10);


  end = x->next;           //where to stop

  save = x->num;          //in case of error
  iy =  x->num;

  a = (MATHX*) x->ptrs[iy];         //first chimem block

  while (iy < end)
  {

  a = (MATHX*) x->ptrs[iy];             // same as *x first time
  ix = bfindix(x,a);
  ans = x->comp(x,ix, a);

  if (!ans)
   {
               x->num = save;
            #ifdef XDBGX
     DBGPRT(0, "PANIC!!");

         x->next = x->num;               //reset temp pointer
         break;                                //?? reset x->num and x->next?
     #endif
   }
  else
  {
      //while.....

    chinsert(CHMATHX, ix, a);
  }

  iy++;
  }
  return a;
}


/*
MATHN *get_mname(void *fid, char *name)
{
 uint ix;
 MATHN *m;
 CHAIN *x;

  x = datach + CHMATHN;

//allow empty name ???

 m = (MATHN*) chmem(CHMATHN);
 m->fid = fid;
 m->mathname = name;               //just copy char pointer for search
 if (name) m->nsize = strlen(name) + 1 ;

 ix = bfindix(x, m);


 if (!x->comp(x, ix,m))
 {
   return (MATHN*) x->ptrs[ix];
 }

x->lastix = x->num;   // invalidate if not found
return NULL;
}
*/




MATHN *get_mname(void *fid, CSTR *name)
{
 uint ix;
 MATHN *m;
 CHAIN *x;

  x = datach + CHMATHN;

//allow empty name ???

 m = (MATHN*) chsmem(CHMATHN);
// m->fid = fid;
 m->mathname = (char*) name->string;               //just copy char pointer for search
 m->nsize = name->len;

 ix = bfindix(x, m);


 if (!x->comp(x, ix,m))
 {
   return (MATHN*) x->ptrs[ix];
 }

x->lastix = x->num;   // invalidate if not found
return NULL;
}



MATHN *add_mname(void *fid, CSTR *cname)           //STRG s)
{

  //  create a math function name, for print and
  //  linkage only.

  //ordered solely by name string

  MATHN *n;
  int ans;
  uint ix;
  CHAIN *x;

  x = datach + CHMATHN;

  n = (MATHN *) chimem(CHMATHN);
//  n->fid = fid;

  if (!cname) return NULL;

  if (!cname->len) return NULL;

// must have real name

  n->mathname = (char *) mem (0,0,cname->len+1);       // get new sym size, allow for null
  strncpy (n->mathname, cname->string, cname->len);    // and copy it
  n->mathname[cname->len] = '\0';
  n->nsize = cname->len;                               // size without null

  ix = bfindix(x,n);

  ans = x->comp(x, ix, n);

  if (!ans)
  {  // exists already.  Clear it and reset for use. may not be necessary ?
  //  allow a duplicate.............
    x->lasterr = E_DUPL;
    x->next = x->num;               //reset temp pointer
    return (MATHN*) x->ptrs[ix];                //NULL;
  }

  chinsert(CHMATHN, ix, n);

  x->lastix = x->num;        //must reset any queries
  return n;
}








int eqlink  (CHAIN *x, uint ix, void *newb)
{
   // key to key generic multiple linker
   // scekey <-> destkey both ways.
   // many to many, so need both keys

 FKL *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (FKL*) x->ptrs[ix];
 nw = (FKL*) newb;

 // no key is unique, only the combination of the five....

  if (nw->keysce) ans = (char *) ch->keysce - (char *) nw->keysce;
  if (ans) return ans;

  if (nw->chsce) ans =  ch->chsce - nw->chsce;
  return ans;


  if (nw->keydst) ans =  (char *) ch->keydst - (char *) nw->keydst;
  return ans;

  if (nw->chdst) ans =  ch->chdst - nw->chdst;
  return ans;

  if (nw->type)  ans =  ch->type - nw->type;
  return ans;

 }



FKL *add_slink(uchar chsce, void *sce, uchar chdst, void *dst, uchar type)
{
   // add a single link between chain elements.
   // for a 'many to many' type links
   // need source chain and item, dest chain and item.

  CHAIN *x;
  FKL *a;
  uint ix;
  int ans;

  if (!sce)   return NULL;
  if (!dst)   return NULL;
  if (!chsce) return NULL;
  if (!chdst) return NULL;


  x = datach + CHLINK;

  a = (FKL*) chimem(CHLINK);
  a->keysce = sce;
  a->keydst = dst;
  a->chsce  = chsce;
  a->chdst  = chdst;
  a->type   = type;

  ix = bfindix(x,a);

  ans = eqlink(x, ix, a);   //this is separate subr

  if (!ans)

    {   // exists already.
        x->next = x->num;               //reset temp pointer
     return NULL;
    }

  chinsert(CHLINK,ix,a);

  return a;
  }


FKL *add_link(uchar chsce, void *sce, uchar chdst, void *dst, uchar type)
{
    // add two way link with a type.
    // could do 'back' chain like jumps, but by the time a CHAIN is used
    // savings are only 1 pointer per entry ...
    // all types set to 1 for now


  FKL *a;

  a = add_slink(chsce,sce,chdst,dst,type);

  if (!a) return NULL;

//  add_slink(chdst,dst,chsce,sce,type);                  // add reverse link   - temp disable

  return a;
  }



FKL *get_link (uchar chsce, void *sce, uchar chdst, void *dst, uchar type)     //void *sce, void *dst, uint type)
{
 FKL *a;
 uint ix;
 CHAIN *x;

  // set up loop for generic linker, and find first item
  // don't need chain for sce, but do need for dest to get right cast
  // all types are 1 at present

 // x = chindex[CHLINK]; CHAIN *x;

  x = datach + CHLINK;

  a = (FKL*) chsmem(CHLINK);              //&chlkdop,0);
  if (!a) return  NULL;

  a->keysce = sce;
  a->keydst = dst;
  a->chsce  = chsce;
  a->chdst  = chdst;
  a->type   = type;


  ix = bfindix(x,a);

  if (eqlink(x, ix ,a)) return NULL;    // no match

  return (FKL*) x->ptrs[ix];

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
               do_one_xopcode(d->from);               // get details of opcode
               j = bytes(sinst.oper[1].rfend);        // size in bytes
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

  x = add_scan(addr, J_STAT | not C_ALT,0);          // but this also adds into MAIN SCAN chain
  if (!x) return;
//  if (chscan.lasterr) return;

 // x->gapscan = 1;              //set here for

  scan_blk(x, &linst);



  if (!x->inv)
  {
  // this may have added new scans, will be in alt chain -
  // not if scan_blk done upon add

//  wnprt(1,"# scan gap at %x as code",cmdaddr(addr));  ix = 0;

  num = chscalt.num;

  while (ix < chscalt.num)
   {
    s = (SBK*) chscalt.ptrs[ix];
    if (!s->stop && !s->inv && s->scnt < 10)
      {
        #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(0,"Sgap Scan (%d) ", ix);
        #endif
//          scan_blk(s, &linst);
          if (chscalt.num != num)
            {
             #ifdef XDBGX
             DBGPRT(1,"NEW SCANS %d -> %d", num, chscalt.num);
             #endif
             ix = -1;   // rescan if changed
             num = chscalt.num;
            }
         }
    ix++;
   }
  }

 }
*/

uint get_jump_ix(CHAIN *x, uint addr)
{
  JMP *j;
  j = (JMP*) chsmem(CHJPF);         //&chjpf);

  if (x == datach + CHJPF)  j->fromaddr = addr;
  else j->toaddr = addr;

  return bfindix(x, j);
}


JMP * get_fjump(uint addr)
{
  JMP *j;
  uint ix;
  CHAIN *x;

  x = datach + CHJPF;

  j = (JMP*) chsmem(CHJPF);
  j->fromaddr = addr;



  ix = bfindix(x, j);


  if (x->comp(x,ix,j)) return NULL;    // no match
  return (JMP*) x->ptrs[ix];
}



void check_adjacent(CHAIN *x, JMP* j)
{
  JMP *t;
  uint ix;
  ix = get_jump_ix(x, j->fromaddr+j->size);

   if (ix >= x->num) return;
   t = (JMP*) x->ptrs[ix];

//staic and returns

   if (j->jtype == J_STAT && j->fromaddr+j->size == t->toaddr) j->jelse = 1;
   if (j->jtype == J_RET && j->fromaddr+j->size == t->toaddr) j->jelse = 1;

}





JMP * add_jump (SBK *caller, int to, int type)
{
  JMP *j;
  CINST *c;           // OPCDH *x;
  CHAIN *jf, *jt;
  uint ix;
  int ans;

  if (get_anlpass() >= ANLPRT) return NULL;

  jf = get_chain(CHJPF);
  jt = get_chain(CHJPT);

  jf->lasterr = 0;
  jt->lasterr = 0;
  if (!val_rom_addr(to))
    {
     #ifdef XDBGX
       ix  = (nobank(to));   // don't want calcons
       if (ix >= 0xd000 && ix <= 0xd010) return NULL;
       if (ix == 0x1f1c || ix == 0x1000 || ix == 0x1800) return NULL;
       DBGPRT(1,"Invalid jump to %x from %x", to, caller->curaddr);
     #endif
     return NULL;
    }








 // x = get_chain(ch);

  j = (JMP *) chimem(CHJPF);
  j->toaddr     = to;
  j->fromaddr   = caller->curaddr;
  j->jtype  = type;
  j->size   = caller->nextaddr - caller->curaddr;   // excludes compare

  if (caller->tstvect) j->test = 1;


//if (j->fromaddr == 0x960d8)
//{
//    DBGPRT(0,0);
//}


  // is this preceeded by a compare ?
  // get size of cmp as well
 // ix = s->curaddr;

  c = find_opcode(caller->curaddr,0);      // previous opcode

  if (c && c->sigix == 10)  j->cmpcnt = caller->curaddr - c->ofst;   // count back for cmp


  if (!bankeq(j->toaddr, j->fromaddr))  j->bswp = 1;        // bank change
  else
  if (j->jtype && j->fromaddr >= j->toaddr)
     {
      j->back = 1;
     }

  // insert into the two jump chains




  ix = bfindix(jf, j);      // FROM chain
  ans = jf->comp(jf, ix, j);

  if (ans)
    {
     chinsert(CHJPF, ix, j);                    // do insert in FROM chain
     ix = bfindix(jt, j);                 // and in TO chain
     chinsert(CHJPT, ix, j);
    }
  else
    {
      j =  (JMP *) jf->ptrs[ix];             // match - duplicate
      jf->lasterr = E_DUPL;
      jf->next = jf->num;                      // reset temp pointer
}

//  check_jump_span(j);

  check_adjacent(jf,j);        //NB. can't do searches between chmem and bfind....  MUST DO THIS ON COPIES !!

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");

    DBGPRT (0," jump %x-%x %s", j->fromaddr, j->toaddr, jtxt[j->jtype]);

    if (j->retn)  DBGPRT(0," ret");
    DBGPRT(1,0);
   #endif

//  if (!ans) return 0;             // for detection of duplicate (sjsubr)
  return j;
}










uint get_tjmp_ix(uint ofst)
{
  // find to-jump - as this is multiple,
  // bfindix (via cpjump) will find first one

  JMP * j;
  uint ix;
  CHAIN *chjpt;

  chjpt = datach + CHJPT;

  if (!chjpt->num) return 1;

  j = (JMP*) chsmem(CHJPF);
  j->toaddr = ofst;
  ix = bfindix(chjpt, j);

  // bfindix WILL find first to jump (code in cpjmp)

  if (ix >= chjpt->num) return ix;

  j = (JMP*) chjpt->ptrs[ix];

  if (j->toaddr != ofst) return chjpt->num;

  return ix;
}




uint get_fjmp_ix(uint ch, uint ofst)
{
  // find from jump - as this is multiple,
  // bfindix (via cpjump) will find first one

  JMP * j;
  CHAIN *x;
  uint ix;

 x = get_chain(ch);
  if (!x->num) return 1;

  j = (JMP*) chsmem(ch);
  j->fromaddr = ofst;


  ix = bfindix(x, j);

  // bfindix WILL find first to jump (code in cpjmp)

 // if (ix >= ch->num) return ix;

 // j = (JMP*) chjpf.ptrs[ix];

//  if (j->toaddr != ofst) return chjpt.num;

  return ix;
}








SIG* get_sig (uint addr)
{
  SIG *t, *s;
  uint ix;
  CHAIN *chsig;

  chsig = datach + CHSIG;

  t = (SIG*) chsmem(CHSIG);
  t->start = addr;

  ix = bfindix(chsig, t);

 if (ix >= chsig->num) return  NULL;    // no match

  //check for correct address ....

  s = (SIG*) chsig->ptrs[ix];
  if (s->start != addr) return NULL;

  return s;


//  if (eqsig(&chsig,ix,t)) return  NULL;    // no match
//  return (SIG*) chsig->ptrs[ix];

}

LBK *get_prt_cmd(LBK *dflt, BANK *b, uint ofst)
  {
    // find next command for printout from cmd either code or data chain
    // if none found use a default (fill) block
    // code always overrides data

    LBK *c  ;      //, *d;
    int ixc ;      //, ixd;
    CHAIN *chcmd;

    chcmd = datach + CHCMD;

    dflt->start = ofst;
    dflt->end = 0;
    dflt->fcom = 0;

    ixc = bfindix(chcmd,dflt);             // search for code entry first
    c = (LBK*) chcmd->ptrs[ixc];

    if (!eqcmd(chcmd,ixc,dflt)) return c;    // match

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
        DBGPRT(0,"PANIC for %x  %x  CD !!! ", dflt->start, dflt->end);
        DBGLBK(1,c);
     //   DBGPBK(1,d,"DAT");
       #endif
       dflt->end = dflt->start+1;
     }
  dflt->size = 1;
return dflt;

}

int get_tjump_bkts (uint ofst)
{
  // find trailing jump, for brackets, matches fjump
  // prints close bracket for each jump found

 JMP * j;
 uint ix, ans;
 CHAIN *chjpt;

 chjpt = datach + CHJPT;


 if (get_anlpass() < ANLPRT) return 0;
 if (!get_cmdopt(OPTSRC)) return 0;
 if (!get_cmdopt(OPTBKT)) return 0;

 ans = 0;
 ix = get_tjmp_ix(ofst);   // find first 'to' jump

 while (ix < chjpt->num)
  {
   j = (JMP*) chjpt->ptrs[ix];
   if (j->toaddr == ofst)
    {
     if (j->cbkt) ans++;
     ix++;
    }
   else break;    //stop as soon as different address
  }
  return ans;
}




JMP* get_tjump(uint ofst, uint *rix)
{
  // find to-jump - as this is multiple,
  // bfindix (via cpjump) will find first one

 uint ix;
 CHAIN *chjpt;

 chjpt = datach + CHJPT;


 ix = get_tjmp_ix(ofst);   // find first 'to' jump

 if (rix) *rix = ix;

 if (ix < chjpt->num) return (JMP*) chjpt->ptrs[ix];

 return NULL;
}


JMP * find_fjump (uint ofst, int *x)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 8 for 'else', 4 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JMP *j;

  if (get_anlpass() < ANLPRT) return 0;

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

    score = 0;

    addr = *start;            //safety

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


/*

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

   imd is NOT a guaranteed start for a struct.......need to know if
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



/*
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

   get_dtkd(CHDTKD,0, start);
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

  get_dtkd(CHDTKD,0, start); //may not match, use lastix for nearest after start
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


*

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




/


}


*/








void turn_testscans_into_code (void)
 {

  // turn gapscan list into code commands.
  // if any are invalid, ignore the whole lot

  uint ix;
  SBK *s;
  CHAIN *x;

#ifdef XDBGX
  DBGPRT(1,"Testcans_into_code");
  #endif

x = get_chain(CHSCAN);


  for (ix = 0; ix < x->num; ix++)
   {
    s = (SBK*) x->ptrs[ix];
    if (s->tstvect)
     {
      add_cmd (s->start,s->nextaddr,C_CODE);
      }
   }            // for loop


 }


/*
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


*/



uint verify_test_scans(void)
{
uint ix, state;
CHAIN *x;
SBK *s;

x = get_chain(CHSCAN);

state = 0;

for (ix = 0; ix < x->num; ix++)
  {
    s = (SBK*) x->ptrs[ix];

    if (s->tstvect && s->inv)
      {
       state = 1;
       break;
      }
  }
return state;
}













uint do_cgap(INST *c, uint start)
{

    //cross check for vect lists ??

//test scan and then clean up accordingly.....

SBK* blk;
uint state, x;
   #ifdef XDBGX
      DBGPRT(1,"Gap Code scan %x", start);
   #endif

   blk = add_scan(start, J_STAT|C_TEST, 0);                     // vect subr (checked inside cadd_scan)
   x = scan_blk(blk,c);

if (!x) return 0;
    state = verify_test_scans();

    if (!state)  turn_testscans_into_code ();

    cleantestdata(blk);

if (!state) return 1;           //success

   return 0;
}











void scan_cmd_gaps(INST *x, uint (*pgap) (INST *c, LBK *s, LBK *n))
{
   // COMMAND CHAIN gaps.

  LBK *s, *n;
  uint ix;
  CHAIN *chcmd;

  chcmd = datach + CHCMD;

 #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(2, "Gap analysis");
  #endif


 // while (ix < chcmd->num)

  for (ix = 0; ix < (chcmd->num-1); ix++)
   {
     s = (LBK*) chcmd->ptrs[ix];
     if (nobank(s->end) <= 0x201e) continue;

     n = (LBK*) chcmd->ptrs[ix+1]; // next scan block

     if ((n->start-1) < (s->end+1)) continue;

     if (g_bank(s->start) != g_bank(n->start) ) continue;       //no bank change

     #ifdef XDBGX
       {
         CSTR *c;        //, *d;      //don't need both....
         c = get_cstr(CMDSTR, 0);     //n->fcom);

         DBGPRT(0,"\n\nGAP %x-%x", s->end-1, n->start-1);
         DBGPRT(1, "  %s <-> %s", c[s->fcom].string, c[n->fcom].string);
       }
     #endif

    if (pgap (x,s,n)) ix--;
   }

}


uint code_stop(uint addr)
{
  // look for 'scan end' opcodes
  // f0 f1, static jumps 0x20 - 0x27,  0xe7

  uint val;

  val = g_byte(addr);
  if (val == 0xf0) return 1;
  if (val == 0xf1) return 1;
  if (val == 0xe7) return 1;

  if (val >= 0x20 && val <=0x27) return 1;

  return 0;
}




uint scan_cgap(INST *c,LBK *s, LBK *n)
{
    //gap between s and n
   LBK *cb;
   DTD *d;
   CHAIN *chdtdd;

   chdtdd = datach + CHDTDD;

   uint jx, start, end;   //, val;  //, cnt;

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
       return 0; //to where, maybe move start up ??

       //if end < gap end ??
     }

   jx = get_dtdo_ix (start);
   d = 0;
   if (jx < chdtdd->num)
      {
       d = (DTD *) chdtdd->ptrs[jx];
       if (d->dataptr <= end)
         {
          #ifdef XDBGX
            DBGPRT(1,"Data found at %x, STOP", d->dataptr);
          #endif

          end = d->dataptr-1;

          if (code_stop(end))
            {
             #ifdef XDBGX
            DBGPRT(1,"continue to %x", end);
          #endif
            }
        }
      }
    else

   {
//check at least one side is code....
   if (s->fcom != C_CODE && n->fcom != C_CODE)
   {
         #ifdef XDBGX
            DBGPRT(1,"Not Code on both sides - STOP");
          #endif

          // if 0xf0 at previous to stdat then can still scan....
          return 0;
        }
   }

   //change this to scan between f0/f1 backwards ? for gap which have data at end.
   //also if start-1 is an f0 or not.


   for (jx = start; jx <= end; jx++)
     {
         if (code_stop(jx)) break;
     }

   if (jx > end) return 0;

   //ok to try a code scan

   return do_cgap(c,start);

}





uint scan_fillgap(INST *c,LBK *s, LBK *n)
{
    // not code, so look for data structs
    // check for fill block first.  Must not overlap anything data.

   DTD *d;
     CHAIN *chdtdd, *chcmd;
  uint i,jx, start, end, flag;

   chdtdd = datach + CHDTDD;
   chcmd = datach + CHCMD;

   if (n->fcom <= C_BYTE) return 0; // n bigger than byte

   start = s->end+1;
   end  = n->start-1;        i = s->end+1;

   jx = get_dtdo_ix(start);       //d get_dtkd(CHDTKD,0, start);
 //  jx = chdtkd.lastix;
   d = 0;
   if (jx < chdtdd->num)
      {
       d = (DTD *) chdtdd->ptrs[jx];
         if (d && d->dataptr < end) end = d->dataptr-1;
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
            add_cmd(start, end,0);      // all fill
            if (!chcmd->lasterr) return 1;   // new command OK
           }
        }



return 0;
}


   void set_rbflag(uint index)
      {
        rbflags[index / 8] |= 1 << (index % 8);
      }


    char get_rbflag(uint index)
     {
        return 1 & (rbflags[index / 8] >> (index % 8));
     }


void scan_opcodes(void)
{
   // scan opcodes for rbases (and others later)

  INST *c;
  OPER *o;
  RBT *r;
  CHAIN *chbase, *chopcd;
  uint ix;
  int i;             //Allows neg numbers...

 #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(2, "Rbase/opcode scan");
  #endif

chbase = datach + CHBASE;
chopcd = datach + CHOPC;

// build a RBT bit array for speed to start with............

 for (ix = 0; ix < chbase->num; ix++)
   {
     r = (RBT*) chbase->ptrs[ix];
     if (!r->oinv)   set_rbflag(r->reg);  //at least one valid rbase...
   }


 // while (ix < chcmd->num)

  for (ix = 0; ix < chopcd->num; ix++)
   {
     c = (INST*) chopcd->ptrs[ix];


     for (i = 1; i < 4;  i++)
       {
         //check destination too, for stx
         o = c->opnd + i;
         if (o->fend)
           {
             if (get_rbflag(o->reg))

          {
            // possible rbase............
            r = get_rbt(o->reg, c->ofst);
         //   DBGPRT(1, "scan-rbt [%d] R%x is rbs = %x at %x", i, r->reg, r->val, c->ofst);

            //o->rbs = 1;
            //and recalc and reset optype ?? do_code ?
          }

        else
          {  o->rbs = 0;
          //may have to rest OPTYPES and registers....
          }
           }



     // now recalc/fix opcode addresses if neccesary ?
     // maybe have to lookup back, or do a do_code again ??


       }


  }

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


void cleanjmp(uint state)
{

uint ix;
CHAIN *x;
JMP *s;             //NB. *can* modify chain entry, just use const as safety.

x = get_chain(CHJPF);

//only one block, 2 pointers, mark block

for (ix = 0; ix < x->num; ix++)
  {
    s = (JMP*) x->ptrs[ix];

    if (s->test)
      {
       s->test = 0;
       s->inv = state;            // feed in 1 or 0 for set/clear
      }
  }

}

void cleandtd(uint state)
{

uint ix;
CHAIN *x;
DTD *s;             //NB. *can* modify chain entry, just use const as safety.

x = get_chain(CHDTDO);

//only one block, 2 pointers, mark block

for (ix = 0; ix < x->num; ix++)
  {
    s = (DTD*) x->ptrs[ix];

    if (s->test)
      {
       s->test = 0;
       s->inv = state;            // feed in 1 or 0 for set/clear
      }
  }

}



void cleanbase(uint state)
{

uint ix;
CHAIN *x;
RBT *s;             //NB. *can* modify chain entry, just use const as safety.

x = get_chain(CHSYM);


for (ix = 0; ix < x->num; ix++)
  {
    s = (RBT*) x->ptrs[ix];

    if (s->test)
      {
       s->test = 0;
       s->tinv = state;            // feed in 1 or 0 for set/clear
      }
  }

}




void cleansub(uint state)
{

uint ix;
CHAIN *x;
SUB *s;             //NB. *can* modify chain entry, just use const as safety.

x = get_chain(CHSUBR);


for (ix = 0; ix < x->num; ix++)
  {
    s = (SUB*) x->ptrs[ix];

    if (s->test)
      {
       s->test = 0;
       s->inv = state;            //feed in 1 or 0 for set/clear
      }
  }

}








void cleansym(uint state)
{

uint ix;
CHAIN *x;
SYM *s;             //NB. *can* modify chain entry, just use const as safety.

x = get_chain(CHSYM);


for (ix = 0; ix < x->num; ix++)
  {
    s = (SYM*) x->ptrs[ix];

    if (s->test)
      {
       s->test = 0;
       s->inv = state;            //feed in 1 or 0 for set/clear
      }
  }

}



void cleanopc(uint state)
{

uint ix;
CHAIN *x;
INST *c;             //NB. *can* modify chain entry, just use const as safety.

x = get_chain(CHOPC);


for (ix = 0; ix < x->num; ix++)
  {
    c = (INST*) x->ptrs[ix];

    if (c->test)
      {
       c->test = 0;
       c->inv = state;            //feed in 1 or 0 for set/clear
      }
  }

}

void cleanscan(uint state)
{

uint ix;
CHAIN *x;
SBK *s;

x = get_chain(CHSCAN);


for (ix = 0; ix < x->num; ix++)
  {
    s = (SBK*) x->ptrs[ix];

    if (s->tstvect)
      {
       s->tstvect = 0;
       s->inv = state;            //feed in 1 or 0 for set/clear
      }
  }

}


void cleantestdata(SBK *z)
{

  //  clean all test entries by flagging with INV

  //chainlist

//   spf?

// CHDTDO, CHDTDD
// CHJPF, CHJPT

// CHSYM
// CHBASE
// CHSUB
// CHOPC



// uint ix,
uint state;
//CHAIN *x;
//SBK *s;

/*
x = get_chain(CHSCAN);

state = 0;

for (ix = 0; ix < x->num; ix++)
  {
    s = (SBK*) x->ptrs[ix];

    if (s->tstvect && s->inv)
      {
       state = 1;
       break;
      }
  }
*/
state = verify_test_scans();


#ifdef XDBGX
  DBGPRT(1,0);

  if (state)
    {
             //flag all test items as invalid
              DBGPRT(1,"DGDGDG tree invalid %x", z->start);
    }
  else
    {
            //OK, remove test flags.

 DBGPRT(1,"DGDGDG tree OK %x", z->start);
            }
//if (s->inv) mark all tests as invalid, otherwise clear the test flags.
#endif


cleanopc(state);
cleansym(state);
cleanbase(state);
cleansub(state);
cleanjmp(state);
cleandtd(state);
cleanscan(state);


}