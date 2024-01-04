
/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version number and notes in shared.h
 ******************************************************/


#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>                // linux
#include "shared.h"

void   *mem          (void*, size_t, size_t);
void   mfree         (void*, size_t);
int    disassemble   (char *);
void   closefiles    (void);
void   DBG_data      (void);
void   prt_dirs      (void);



// -------------- local declarations


#ifdef __linux__
struct sigaction sa;                // for signal capture
#endif

char *fstr;                 //malloced

int  incpg    = 0;
uint lastsym  = 0;
int  lastpass = -1;

 FILE *tty;             //= stdout unless testing


//********** file data holder, alternates for linux and Win32 ********************

// HDATA declared in shared.h

// number of files (+1) to open.  Excludes sad.ini (= entry [6]), for whether debug file opened.


#ifdef XDBGX
  uint numfiles = 6;                     //open xx_dbg.txt
#else
  uint numfiles = 5;
#endif

#ifdef __linux__

 HDATA fldata = {

  { ".bin" , "_lst.txt", "_msg.txt", "_dir.txt", "_cmt.txt", "_dbg.txt", "sad.ini" },
  { "r"   , "w"       , "w"       , "r"       , "r"       , "w"       , "r"        },
  {  0,      0,          0,          0,          0,          0,          0         }              //file entries
 } ;


 #define PATHCHAR  '/'

#endif

// Windows (via CodeBlocks, mingw) has file type (bin/text) in addition to read/write
// backwards slash in directory tree

#if defined __MINGW32__ || defined __MINGW64__

 HDATA fldata = {

  { ".bin", "_lst.txt", "_msg.txt", "_dir.txt", "_cmt.txt", "_dbg.txt", "sad.ini" },
  { "rb"  , "wt"      , "wt"      , "rt"      , "rt"      , "wt"      , "rt"      },
  {  0,      0,          0,          0,          0,          0,          0        },              //file entries
 } ;
 #define PATHCHAR  '\\'

#endif




cchar *passd [] = {"Check Binary", "Process Commands", "Scan Code" ,
                 "Check Code","Analyse Data Structs", "Output Listing" };


char prg[] = { '|', '/', '-', '\\' };       //  for rotating progress display


HDATA *get_fldata(void)
{
    return &fldata;
}


FILE *get_file_handle(uint ix)
{
    return fldata.fh[ix];
}




void show_prog (int s)
{
  if (s < 0 || s > 5) s = 4;               // generic safety

  if (s && s != lastpass)
    {
      fprintf (tty, "\n  %c  %s\r", prg[0], passd[s]);
      lastpass = s;
      incpg = 0;
      lastsym = 0;
    }
  incpg++;


if (isatty(STDOUT_FILENO))
  {  // connected to terminal, show progress 'tickers'

   if (incpg > 50 )           // not if redirected to a file.....how to check ??
    {
      incpg = 0;
      if (lastsym >= sizeof (prg)) lastsym = 0;
      printf ("  %c\r", prg[lastsym]);
      fflush (stdout);
      lastsym++;
    }
  }

}



void prt_stars(void)
{
 short i;
  fprintf (tty, "\n");
  for (i = 0; i < 50; i++) fprintf (tty, "*");
  fprintf (tty, "\n");
}


/*  opening directory for case insensitive names.

  #include <stdio.h>
    #include <dirent.h>

    int main (int argc, char *argv[]) {
        struct dirent *pDirent;
        DIR *pDir;

        // Ensure correct argument count.

        if (argc != 2) {
            fprintf (tty, ("Usage: testprog <dirname>\n");
            return 1;
        }

        // Ensure we can open directory.

        pDir = opendir (argv[1]);
        if (pDir == NULL) {
            fprintf (tty, ("Cannot open directory '%s'\n", argv[1]);
            return 1;
        }

        // Process each entry.

        while ((pDirent = readdir(pDir)) != NULL) {
            fprintf (tty, ("[%s]\n", pDirent->d_name);
        }

        // Close directory and exit.

        closedir (pDir);
        return 0;
    }
*/






uint openfile(uint ix)
{

  fldata.error = 0;

 if (fldata.fh[ix]) return 0;          //open already

  fldata.fh[ix] = fopen(fldata.fname[ix], fldata.fpars[ix]);           // open precalc'ed names

  // only first 3 files are mandatory (bin, lst, msg)
  if(ix < 3 && fldata.fh[ix] == NULL)
      {
       fprintf (tty,"fail to open %s\n", fldata.fname[ix]);
       fldata.error = ix+1;
       return fldata.error;
      }
return fldata.error;
}


void closefiles(void)
{
    // flush and close all files
  uint i;

  for (i=0; i < numfiles; i++)         // include sad.ini
   {
    if (fldata.fh[i])         // close file
       {
         fflush (fldata.fh[i]);
         fclose (fldata.fh[i]);
         fldata.fh[i] = NULL;
       }
   }
  fldata.flength = 0;             //flag marker.

}


void make_path(uint ix, cchar *path)
 {
     // make full path in windows or linux, can be absolute or relative
   uint i;

   i = strlen(path);   // length of new path (no terminator)

   // an absolute path
   fldata.pathsize[ix] = i+2;
   fldata.fpath[ix] = (char*) mem(fldata.fpath[ix],0,fldata.pathsize[ix]);   //+ term
   sprintf (fldata.fpath[ix], "%s",path);

 }


void make_name (uint ix)

{
  //  make name from path, default bare name, suffix
  fldata.namesize[ix] = fldata.pathsize[ix] + fldata.baresize + 8;       // 8 is longest suffix '_lst.txt'  etc.
  fldata.fname[ix] = (char*) mem(fldata.fname[ix],0,fldata.namesize[ix]);

  if (ix == 6)
     sprintf(fldata.fname[ix], "%s%c%s", fldata.fpath[ix], PATHCHAR, fldata.suffix[ix]);            //sad.ini
  else
     sprintf(fldata.fname[ix], "%s%c%s%s", fldata.fpath[ix],PATHCHAR, fldata.barename, fldata.suffix[ix]);  // file names

}





uint read_sad_config()
{
  //  names should be set up correctly by now.

  uint i;
  char *t;

//must allow for bin path to be different (drag and drop)
// sad.exe path first, then bin if there is one.

  fldata.fh[6] = fopen(fldata.fname[6], fldata.fpars[6]);

  if (!fldata.fh[6] && fldata.filpsize)
    {
     fprintf (tty,"\ncan't find config file %s. Try '%s'\n", fldata.fname[6], fldata.filepath);

     make_path(6, fldata.filepath);
     fldata.fh[6] = fopen(fldata.fname[6], fldata.fpars[6]);
    }

  if (!fldata.fh[6])
    {
      //still no sad.ini, default to filepath
     fprintf (tty,"\ncan't find config file %s. Default to local dir\n", fldata.fname[6]);

     if (fldata.filpsize) t = fldata.filepath; else t = fldata.sadpath;

     for (i = 0; i < 6; i++)
      {
        make_path(i,t);
      }

      if (fldata.filpsize) {make_path(0,fldata.filepath);
      make_name(0);







      }
     return 0;
    }

  for (i = 0; i < 5; i++)
      {
       if (fgets (fstr, 4094, fldata.fh[6]))
        {
         t = strchr(fstr, ' ');                 // first space
         if (!t) t = strchr(fstr,'#');          // or comment
         if (!t) t = strchr(fstr, '\n');        // or newline
         if (t)  *t = '\0';
         fstr[4094] = '\0';
         make_path(i,fstr);
        }
      }

  make_path(5,fldata.fpath[2]);         //debug file in same place as _msg

  return 0;
 }



int make_barename(cchar *name)
{
   // for both argv and user input, bin filename only !!

 char *x;

 if (!name || !*name) return -1;       //safety check....

 while (*name == ' ') name++;               // skip any spaces

 if (*name == '\r')  return -1;
 if (*name == '\n')  return -1;

 strcpy(fstr, name);                //sp it can be modified

// but this filename may have a path !
// if it does, make it default.....

  x = strrchr(fstr, PATHCHAR);           // is there a path in this filename (drag and drop etc)
  if (x)
    {
     *x = '\0';
     fldata.filpsize = strlen(fstr) + 1;   // length of name
     fldata.filepath = (char*) mem(fldata.filepath,0,fldata.filpsize);
     strcpy(fldata.filepath, fstr);           //copy new default path
     x++;
 //    fprintf (tty,"filepath1 = %s\n", fldata.filepath);
    }
  else x = fstr;                             // original name, no path


 fldata.baresize = strlen(x) + 1;   // length of name
 fldata.barename = (char*) mem(0,0,fldata.baresize);
 strcpy(fldata.barename, x);

 x = strrchr(fldata.barename, '.');                         // ditch any extension
 if (x) *x = '\0';

 x = strrchr(fldata.barename, '\n');             // and terminator for user input
 if (x) *x = '\0';


  return fldata.baresize;
}



int get_options(int argc, cchar** argv)
 {
   //argc and argv from command line

  short ix;
  cchar *t;       //for read of argv
  char *x;

  // PATHCHAR defined at beginning with file option flags
  t = argv[0];
  if (*t == '\"' || *t == '\'') t++;      // skip any opening quotes (Windows)

  fldata.sadpsize = strlen(t) + 1;
  fldata.sadpath = (char*) mem(0,0,fldata.sadpsize);
  strcpy(fldata.sadpath, t);

  x = strrchr(fldata.sadpath, PATHCHAR);           // stop at last backslash to get path
  if (x) *x = '\0';

  // now go through args


//  fprintf (tty,"filepath=%s\n",fldata.filepath);    //wrong....
//  fprintf (tty,"sadpath=%s\n",fldata.sadpath);
//  ix = 0;
//  while (ix < argc) { fprintf (tty,"arg [%d]=%s\n",ix, argv[ix]); ix++; }
//  fflush(tty);


  ix = 1;

  while (ix < argc)
    {
      t = argv[ix];                 //first char must be a '-'

      switch (*t)
       {
         case '-' :
           t++;

           switch (*t)
            {              // - options.
              default:
                fprintf (tty,"unknown option %s\n", argv[ix]);
                return 0;

              case 'c' :
              case 'C' :
                t++;
                if (*t == 0)
                    {      // path is in NEXT argument.
                      ix++;
                      t = argv[ix];
                    }
                make_path(6,t);
                make_name(6);            // safe if no bare name yet......, suffix[6] = sad.ini
                ix++;                    // next argument
                break;

              case 'h':
              case 'H':
                fprintf (tty, "\nsad -h   this help info");
                fprintf (tty, "\nsad  -c <configdir>  <bin_filename>");
                fprintf (tty, "\n   options");
                fprintf (tty, "\n      -c for directory of sad.ini config file to read");
                fprintf (tty, "\n      if no -c then looks for a sad.ini in same directory as sad exe image");
                fprintf (tty, "\n      asks for bin_filename if none specified\n");
                return 0;
            }            //inner switch (from a '-')


//case K  ?? keep old listing and wrn files ??



         default:         // of outer switch, so this is assumed to be a filename
           make_barename(argv[ix]);
           ix++;         //next param
           break;
       }       // outer switch ('-' and bin name)

    }      //end for args loop

return 0;

 }



#ifdef __linux__
void segfault(int signal, siginfo_t *si, void *arg)
{
    fprintf (tty,"** Caught CRASH signal at address %p code %x\n", si->si_addr, si->si_code);
    fprintf (tty,"** Abandon disassembly\n\n");

    fprintf (fldata.fh[2], "\n\n ** Caught signal %d ** \n", signal); // same as wrnprt
    fprintf (fldata.fh[2],"** Abandon disassembly\n\n");


 #ifdef XDBGX
    fprintf (fldata.fh[5], "\n\n ** Caught signal %d ** \n", signal); // same as dbgprt
    fprintf (fldata.fh[5],"** Abandon disassembly\n\n");

    if (signal == SIGINT) DBG_data();            // this seems to work, even if not technically safe...
 #endif

    prt_dirs();
    closefiles();

    exit(0);
}
#endif



int main (int argc, cchar **argv)
{

  uint i;

 #ifdef __linux__
  // set up signal catcher for disasters

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
 #endif

tty = stdout;       //change for path debug
  //if (isatty(STDOUT_FILENO)) tty = stdout;

  //else

  // tty = fopen ("/home/me/sad_test.txt","w");

// /dev/null ....

       //getcwd




  prt_stars();
  fprintf (tty, "EEC-IV disassembler Version %s (%s)", SADVERSION, SADDATE);
  fprintf (tty, "\nAbsolutely no warranty or liability");
  prt_stars();


  fstr = (char *) mem(0,0, 4096);

  fldata.filpsize = 0;
  fldata.sadpsize = 0;
  for (i = 0; i < 7; i++)
    {       // safety as path my be fed in
      fldata.pathsize[i] = 0;
      fldata.namesize[i] = 0;
    }


  if (get_options(argc,argv))   return 1;

  if (!fldata.pathsize[6])
    {       //no -c option, assemble sad.ini as default
      make_path(6,fldata.sadpath);           //use sadpath first
      make_name(6);
    }

  read_sad_config();                   //so there should be a sad.ini by now.

  if (fldata.baresize)
   {           //binfile specified
    for (i = 0; i < 6; i++)   make_name(i);
    fprintf (tty, "\nFor binary file '%s'\n", fldata.fname[0]);
    for (i = 0; i < numfiles; i++)
       if (openfile(i)) break;

   if (fldata.error)
   {
       fprintf (tty,"cannot find file '%s'\n",fldata.fname[0]);
       fgets (fstr, 256, stdin);
       return 0;
   }
                  //temp
    i = disassemble(fldata.barename);
      fprintf (tty, "\n END of run\n");
  fflush(tty);
    return i;
   }

  while (1)
   {
     fprintf (tty, "\nEnter binary file name\n");
     fgets (fstr, 256, stdin);


     if (make_barename(fstr) < 1) return 0;        // no name
     for (i = 0; i < 6; i++)   make_name(i);
     if (!openfile(0)) break;                      // file opened OK
   }




 // i = 0;

 // fprintf (tty,"sadpath = %s\n",fldata.sadpath);
 // fprintf (tty,"filepath = %s\n",fldata.filepath);
 // while (i < argc) { fprintf (tty,"arg [%d] = %s\n",i, argv[i]); i++; }
 // fflush(tty);


  for (i = 0; i < numfiles; i++)   openfile(i);
  disassemble(fldata.barename);                     // = 0 if successful



  closefiles();
  mfree(fstr, 4096);

  fprintf (tty, "\n END of run\n");
  fflush(tty);


  return 0;
}


//original main............