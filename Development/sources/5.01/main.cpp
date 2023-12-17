
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
    return fldata.fh[ix]; //filedt[ix].fh;
}







void show_prog (int s)
{
  if (s < 0 || s > 5) s = 4;               // generic safety

  if (s && s != lastpass)
    {
      printf ("\n  %c  %s\r", prg[0], passd[s]);
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
  printf ("\n");
  for (i = 0; i < 50; i++) printf ("*");
  printf ("\n");
}


/*  opening directory for case insensitive names.

  #include <stdio.h>
    #include <dirent.h>

    int main (int argc, char *argv[]) {
        struct dirent *pDirent;
        DIR *pDir;

        // Ensure correct argument count.

        if (argc != 2) {
            printf ("Usage: testprog <dirname>\n");
            return 1;
        }

        // Ensure we can open directory.

        pDir = opendir (argv[1]);
        if (pDir == NULL) {
            printf ("Cannot open directory '%s'\n", argv[1]);
            return 1;
        }

        // Process each entry.

        while ((pDirent = readdir(pDir)) != NULL) {
            printf ("[%s]\n", pDirent->d_name);
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
       fldata.error = ix+1;
       return fldata.error;
      }
return 0;
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
   uint i,rel;
   cchar *x;

   i = strlen(path);   // length of new path (no terminator)

   rel = 0;                //absolute path by default

   if (PATHCHAR == '\\')
     {                    // this is a windows OS
      x = path+1;
      if (*x != ':') rel = 1;    // a relative path (c:\ ...is absolute )
     }

//strrchr to remove a leading slash ???
   if (fldata.pathsize[ix]) mfree(fldata.fpath[ix],fldata.pathsize[ix]);


   if (PATHCHAR == '/')
     {          // linux or unix type OS
       if (*path != PATHCHAR) rel = 1;     // not begining with a slash
     }

   if (rel)
     {      //relative - default path prefixed
       fldata.pathsize[ix] = i + fldata.dpsize + 3;      // + term + slash + 1
       fldata.fpath[ix] = (char*) mem(0,0,fldata.pathsize[ix]);
       sprintf(fldata.fpath[ix], "%s%c%s", fldata.defpath, PATHCHAR, path);
     }
   else
     {   // an absolute path
       fldata.pathsize[ix] = i+2;
       fldata.fpath[ix] = (char*) mem(0,0,fldata.pathsize[ix]);   //+ term
       sprintf(fldata.fpath[ix], "%s",path);
     }
 }


void make_name  (uint ix)

{
  //  make name from path, default bare name, suffix
  fldata.namesize[ix] = fldata.pathsize[ix] + fldata.bnsize + 8;       // 8 is longest suffix '_lst.txt'  etc.
  fldata.fname[ix] = (char*) mem(0,0,fldata.namesize[ix]);
  if (fldata.bnsize)             //maybe ix == 6 ??
     sprintf(fldata.fname[ix], "%s%c%s%s", fldata.fpath[ix],PATHCHAR, fldata.barename, fldata.suffix[ix]);
  else
     sprintf(fldata.fname[ix], "%s%c%s", fldata.fpath[ix], PATHCHAR, fldata.suffix[ix]);            //sad.ini
}






uint read_sad_config()
{
  //  names should be set up correctly by now.

  uint i;
  char *t;


  fldata.fh[6] = fopen(fldata.fname[6], fldata.fpars[6]);

  if (!fldata.fh[6])
    {
     printf("\ncan't find config file %s. Using local dir\n", fldata.fname[6]);

     for (i = 0; i < 6; i++)
      {
        make_path(i,fldata.defpath);
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
   // for both argv and user input

 char *x;
 cchar *t;

 if (!name || !*name) return -1;       //safety check....

 t = name;
 while (*t == ' ') t++;               // skip any spaces

 if (*name == '\r')  return -1;
 if (*name == '\n')  return -1;


 fldata.bnsize = strlen(t) + 1;   // length of name
 fldata.barename = (char*) mem(0,0,fldata.bnsize);
 strcpy(fldata.barename, t);

 x = strrchr(fldata.barename, '.');                         // ditch any extension
 if (x) *x = '\0';

 x = strrchr(fldata.barename, '\n');             // and terminator for user input
 if (x) *x = '\0';


  //  fldata.bnsize = strlen(name) + 1;   // new length
  return fldata.bnsize;
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

  fldata.dpsize = strlen(t) + 1;
  fldata.defpath = (char*) mem(0,0,fldata.dpsize);
  strcpy(fldata.defpath, t);

  x = strrchr(fldata.defpath, PATHCHAR);           // stop at last backslash to get path
  if (x) *x = '\0';

  // copy (absolute) default path to SAD path in case no -c option



  // now go through args

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
                printf("unknown option %s\n", argv[ix]);
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
                make_name(6);            // will be safe if no bare name yet......, suffix[6] = sad.ini
                ix++;                                //next argument
                break;

              case 'h':
              case 'H':
                printf ("\nsad -h   this help info");
                printf ("\nsad  -c <configdir>  <bin_filename>");
                printf ("\n   options");
                printf ("\n      -c for directory of sad.ini config file to read");
                printf ("\n      if no -c then looks for a sad.ini in same directory as sad exe image");
                printf ("\n      asks for bin_filename if none specified\n");
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


 // int rename(const char *old_filename, const char *new_filename)
 //* int remove(const char *filename)



#ifdef __linux__
void segfault(int signal, siginfo_t *si, void *arg)
{
    printf("** Caught CRASH signal at address %p code %x\n", si->si_addr, si->si_code);
    printf("** Abandon disassembly\n\n");

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


  prt_stars();
  printf ("EEC-IV disassembler Version %s (%s)", SADVERSION, SADDATE);
  printf ("\nAbsolutely no warranty or liability");
  prt_stars();


  fstr = (char *) mem(0,0, 4096);

  if (get_options(argc,argv))   return 1;

  if (!fldata.pathsize[6])
    {       //no -c option, assemble sad.ini as default
      make_path(6,fldata.defpath);
      make_name(6);
    }

  read_sad_config();                   //so there should be a sad.ini by now.

  if (fldata.bnsize)
   {           //binfile specified
    for (i = 0; i < 6; i++)   make_name(i);
    printf ("\nFor binary file '%s'\n", fldata.fname[0]);
    for (i = 0; i < numfiles; i++)   openfile(i);                        //temp
    i = disassemble(fldata.barename);
    return i;
   }

  while (1)
   {
     printf ("\nEnter binary file name\n");
     fgets (fstr, 256, stdin);


     if (make_barename(fstr) < 1) return 0;        // no name
     for (i = 0; i < 6; i++)   make_name(i);
     if (!openfile(0)) break;                      // file opened OK
   }

  for (i = 0; i < numfiles; i++)   openfile(i);
  disassemble(fldata.barename);                     // = 0 if successful

  closefiles();
  mfree(fstr, 4096);
  return 0;
}


//original main............