
SAD version 5.     Alpha version release

Intro and background. 

SAD wasn't ever a typical software project with a fixed end.  Early versions got new features as things were discovered.
I started it to understand my old EEC-IV box, and I ended up not even messing with it.

Until version 4, SAD didn't even handle subroutine arguments correctly. Everything slowly changed and 'morphed' over time.
More was discovered about multibanks and 8065 CPU and included, but there's still some unknowns/uncertanties remaining today.

The V4 software became quite a rat's nest, with bits nailed on all over the place, bits removed, and it got hard to keep track.
Bug fixes would often cause new bugs to appear somewhere else.  
Inside the code, some things were done many times over, e.g. scanning of the binary file, to get the right answers.  That didn't help with debugging either.

User requests also came in for several things that would be really hard to do under V4 internal structures (e.g. math calculations). 


What has changed in V5, and why ............

I decided to do a tidy up and split the code into more manageable modules, and make it a cleaner design with a more C++ like view (ie. data items together with their handlers).
User requests had come in for more flexible print options, the ability to have proper 'subfields' (single bit masks were handled, but not very neatly) and pointers/offsets are used
more than once in some binaries, with no way to do this outside the comments. There was no way to do proper calculations on data fields either.

Lots of SAD code has been amended and simplified to support more flexible/generic use, and remove 'duplicates'.

The concept of a 'data item' is no longer byte (or multiple byte) sized.
A data item is now start bit to end bit anywhere up to a 32 bit long.  The item can be signed (assumes top bit of field is sign bit). 
This has been built in right down to the base layers of SAD, i.e. memory lookups and so on. This caused many changes but was made universal throughout. 
Print options (mainly set via the additional items) have been reworked to prevent inconsistencies, and again made more generic to match the new calc commands.    

I rewrote the 'signature/fingerprint' matcher, making it simpler to add new code fingerprints, and in future these may become a separate data file, allowing easier amendments
(these are bin code fingerprints used for finding function and table lookups, rbase registers, background task lists, encoded addresses, etc.).
I replaced several 'duplicate' code sections, merging them into single sections to serve the multiple purposes.   

Version 5 has a NEW COMMAND SYNTAX to support the additional data definitions.  This no longer uses a colon delimiter ':' to split fields, moving to square brackets '[]' to delineate each item.
This change was done so that data items can now be NESTED to define fields within fields and reused for printing. 
This allows sybol names and their values to be calculated when they are only part of a byte or word, and can be printed more than once with different settings (i.e. a different calculation)
where it is used more than once, and so on.
The colon ':' form of command is still useable to preserve backwards compatibility, and you can mix old and new syntax (but not within a single command).
note SAD output is always in NEW format, for errors and for _msg file

There is now a 'calc' command, allowing a definition of a maths calculation (in the formula/algebra style of  " = CALCTYPE (x/128)" which can be applied to any data item.
This calculation can have many terms and a type (float, integer, address) and a name , e.g. calc    "nmtest" =  float ((1/x) * (3/x) + (4 - (3.14/(5-x))).
(No that doesn't make any maths sense, it's one of my test sequences)
I will be adding some preset functions like PWR, ENC (encoded addresses) and so on. see below on how to do this via '=' option.
This may be extended some more for math types (Do any bins use Pi or radians ??) and possibly timing like items (e.g. number of IO ticks per millisecond )

V5 analysis model has been rewritten, using the aim that 'every code block is scanned only once', to make things simpler (and faster I hope).
This also required some BIG changes, and a complete redesign of how subroutine arguments are handled.

Alpha release.

There are things still broken in V5, but it's already been over a year in the making, so I wanted to release an early 'alpha' version for users to play with (and probably break...).
Also there are some things I'm not sure what would be best, like how to print subfields in structures/args, so that's done in a 'temporary' way at the moment.
I have done the 'basic' testing to make sure it doesn't crash, works for (almost) all bins, gets arguments right, and so on.

I am still working through bugs, both new and regression, so I intend to continue with V4 in parallel for a while.

If you read the source code, it's still got lots of commented out sections... I need to tidy that up too.

Outstanding items -  (planned, but not done yet).

Change comments syntax so it matches the new command sytax, i.e. with square brackets (and even calculations) allowed in it, matching command systax as much as possible.
Add a 'group' command for better layout.  This allows data structures within the group to share the same column layout and be grouped together.
This is for structures which have different sized rows, to give a generic way to print them. Timer structures and some injection structures have different sized rows.

Dropped the 'timer' command. To be replaced by a 'group' command.  Uses 'struct' in the meanwhile.
Some data options now deprecated but still work, and are replaced by equivalent 'calc' (D offset, E encoded, V divide).

Symbols - 

Probably need some more work/user options.  Why - some things are inconsistent in V4, and only single bit flags are supported.
It was kind of 'kludged' in various ways and didn't work everywhere. Possible options are to have a 'print mask' option to define
what gets printed and which flags are used. (in LDB, LDW etc).  Not finalised yet.

Note V5 prints only fields which have names defined in this release.  This may need to change. e.g. in A9L

26a1: 32,0a,02            jnb   B2,Ra,26a6       if (HSI_Ready = 1) {
26a4: 2e,8d               scall 2533             Read_hsi (); }
26a6: 11,4c               clrb  R4c              Bypass_time = 0;
                                                 Bypass_limit = 0;
                                                 Immediate = 0;
                                                 No_service = 0;
                                                 Late_output = 0;
                                                 Queued = 0;
                                                 No_queue = 0;

But not just single bits any more - xdt2 with Register 11 (bank select) prints this with default symbols, 2 x 4 bit fields))

820bb: b1,11,11           ldb   R11,11           Data_Bank = 1;
                                                 Stack_Bank = 1;

NOTE - there's nothing in the software design to prevent users having multiple copies of SADwin and SAD in separate directories, each with their own config.
So you can have a V4 and a V5 active at the same time.


More on new command syntax.

REPEAT - The original 'colon syntax' ':' is still useable to preserve backwards compatibility, and you can mix old and new syntax (not within a command).

The overall syntax has not massively changed, but additional data items are now in square brackets.  Note this includes the 'global' options  e.g.
OLD syntax - still supported  ($C is compact layout)

struct  22a6 2355  "InjStct" $C  :R N: O 2 UY :UY P 2 : UY N  D 2c5 : O 2 UY  : UW  : UW  | R N  : O 2 UY : UY P 2 : D 2c5 UY N : O 2 UY  : UW       

NEW syntax, with short inline calc replacing the 'D' with calc ('='). Note the '|' for newline still supported, but as an option, not a delimiter. 

struct  22a6 2355  "InjStct"   [$ C] [ UW N R ][ O 3 UY ][ UY N = address ( x + 2c5) ][ O 2 UY ][ UW ][| UW ][ UW N R ][ O 3 UY ][ UY N = address ( x + 2c5) ][ O 2 UY ][ UW ]

in this new syntax, you can specify subfields, as in the 'AA' timer structure....

struct 3e05 3e29  [ UY [B 0 ] [B 1] [B 2] [B 3 5] [B6] [B7] ] [UY]      

This shows single bit fields and a 2 bit field to print inside a byte.   I haven't fully sorted out exact printing format yet, it currently works by printing a '|' between these fields.


Calc commands.

For small calcs, like say " = float (x/128)" can be done inline.  The '=' requires a 'calctype' first.
For something larger or used often, you can specify the calculation separately, with a name, and then just put ' = name'  in the additional data.

Calctypes are  "address", "integer", "float", and the radix ('X') can be binary, decimal (+float), hex (default is hex). 3 letters min, same as commands.  Float in calctype forces float throughout.
'Address' is same as integer hex, but wraps around 16 bits, preserving the bank as the CPU would do.   Standard integers are not limited to 16 bits for calculations 
  
  Examples for A9L

calc "Encode1" =  address (  enc (x, 1, e0) )          # replacement of 'E 1' option   type 1 (at 3695)
calc "Encode2" =  address (  enc (x, 3, f0) )          # replacement of 'E 3' option   type 3 (at 7998) 

and then -

args    3a12 3a13     [$ C] [ UW N = Encode1 ]
args    3c4e 3c4f     [$ C] [ UW N = Encode1 ]
args    3d46 3d47     [$ C] [ UW N = Encode1 ]
args    3d53 3d54     [$ C] [ UW N = Encode1 ]


args    7ca5 7cab     [$ C] [ UW N ][ UW N = Encode2 ][ UW N = Encode2 ][ UY ]     # only middle 2 words encoded

or inline, repeat of above.

struct  22a6 2355  "InjStct"    [ UW N R ][ O 3 UY ][ UY N = address ( x + 2c5) ][ O 2 UY ][ UW ][ | UW ][ UW N R ][ O 3 UY ][ UY N = address ( x + 2c5) ][ O 2 UY ][ UW ] # injection struct



















