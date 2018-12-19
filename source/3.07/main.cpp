
#include <stdio.h>
#include <string.h>                // linux
#include "shared.h"

int    get_config(char **);
void   shutdwn   (void);
void   startup   (void);
short  dissas    (char *);
char   *get_filemap (void);


char fstr[256];

short incpg = 0;
uint lastsym = 0;
int lastpass = -1;

const char *passd [] = {"Checking Binary", "Processing Commands", "Analysing Bin 1" ,
                 "Analysing Bin 2","Analysing Bin 3", " Producing Listing" };



char prg[] = { '|', '/', '-', '\\' };        /* for rotating symbol display */


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

  if (incpg > 200 )
    {
      incpg = 0;
      if (lastsym >= sizeof (prg)) lastsym = 0;
      printf ("  %c\r", prg[lastsym]);
      fflush (stdout);
      lastsym++;
    }
}





void prt_stars(void)
{
 short i;
  printf ("\n");
  for (i = 0; i < 50; i++) printf ("*");
  printf ("\n");
}


/* change to allow cmd line options
-c  config file location
-h  help info
-o  options ?
then <bin> file

*/


int main (int argc, const char **argv)
{
  short go;
  char *x, *fn;
  char* pth[3];
  prt_stars();
  printf ("EEC-IV disassembler Version %s (%s)", SADVERSION, SADDATE);

 /* #ifdef __unix__
  printf ("UNIX");
  #endif

  #ifdef __DOS__
  printf ("DOS");
  #endif   */

  printf ("\nAbsolutely no warranty or liability");

  prt_stars();

 
  fn = 0;

  pth[0] = (char *) argv[0];
  pth[1] = 0;
  pth[2] = 0; 
  for (go = 1; go < argc; go++)
    {
     if (*argv[go] == '-')
     {
       x = (char *)(argv[go]+1);

       switch (*x)
        {
         default:
           printf("unknown option %s\n", argv[go]);
           return 0;

         case 'c' :
         case 'C' :
          pth[1] = (char *) argv[go+1];              // path of config file
  //        printf ("config R dir = %s\n", pth[1]);
          go++;
          break;
          
 /*       case 'w' :
         case 'W' :
          pth[2] = (char *) argv[go+1];              // path of config file
   //       printf ("config W dir = %s\n", pth[2]);
          go++;
          break; 
   */       
          
          
         case 'h':
         case 'H':
           printf ("\nsad -h   this help info");
           printf ("\nsad  <-c configdir> <-w configdir> <bin_filename>");
           printf ("\n   options");
           printf ("\n      -c for directory of sad.ini config file to read");
     //      printf ("\n      -w for directory to write a sad.ini config file to");
      //     printf ("\n      'filename' to decode. Name is appended with '.bin'\n");
           printf ("\n      if no -c then looks for a sad.ini in same dir as sad exe");
      //     printf ("\n      if -w specified will write a 'sad.ini' config file");
      //     printf ("\n      if no directory will write it to same dir as sad exe");
           printf ("\n      asks for bin_filename if none specified\n");
          return 0;
          
          
          //add a -w for create config file....
        }






     }
     else fn = (char*) argv[go];


  //   printf(argv[go]);
   //  printf (" | ");
    }

 // printf("FN=%s\n",fn);
  // get_args here

  // save argv num of one after -c and then use in get_config

  if (get_config(pth))   return 1;           // this seems to work fine !!

/*******************************/

 // prt_stars();







    if (fn)
       {
        printf ("\nFor binary file '%s'\n", fn);
        go = dissas(fn);
        
        return go;
       }


     go = 1;
     while (go)
      {                                     //   fstat / stat    to check file
       printf ("\nEnter binary file name (full path allowed)\n");
       fgets (fstr, 64, stdin);
       if (!strcmp (fstr, "\n")) return 10;
       go = dissas(fstr);                     // = 0 if successful
       fflush(stdout);
      }

 return go;
}
