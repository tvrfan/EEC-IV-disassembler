
Daily version information.

The daily was supposed to be an early test version, but it's grown a bit bigger.....


Notes on regression errors.

Whilst I am sure there ARE some regression bugs, please be aware that the 'final spec' is not fully defined, and some things have been swopped around deliberately.

Symbol names have appeared and disappeared in some places for example, because of the questions around "when should symbols be automatically printed ?" No, it's NOT always obvious.


Subroutine Arguments (and related to symbol names)

Until version 4, SAD could not handle variable arguments properly (that is where a subroutine gets variable numbers of bytes).
I finally decided that the only way to handle this was to do a limited emulation of the code after seeing what CARD bin does in one of its subroutines.
This was required a whole new 'layer' of code and caused a LOT of code changes, which did break quite a few things.

Even now, whilst SAD gets the right number of bytes, it still cannot accurately determine the true size of each argument in all cases.
This leads to the question about symbol names, and why things change deliberately.
Current rule here is that arguments determined to be WORDS ARE checked for symbol names, bytes are NOT.   Is this always right ??  I don't know.

Until version 'M' I also discovered that SUB commands which specified arguments were NOT WORKING, which meant more confusion with symbol names and arguments.
 
In conversations with jsa (from eectuning.org blog) various things have changed or been added between the daily versions, so I will add a versions.txt to explain what has changed.
There have been a whole raft of changes to command checking and errors (things used to fail silently - no good), and the command structure has been altered to try to improve its consistency.  


There are still things outstanding.

1. Correctly size subroutine arguments

2. Change bank addresses - SAD attaches bank number to addresses 0x2000-0xffff, but I realised this is WRONG.
   Technically in an 8065 multibank, the common RAM addresses (0x400-0x1fff) actually exist separately in EACH bank.  Only 0-3ff exists 'outside' the banks (they are inside the CPU)
   This issue arises with some special function chips which can be mapped into those places.

3. Data structures not being found.   Where an address is referenced directly, then this is fine.  (same for an RBASE+offset)
   Where a 'set' of structures are referenced by base_addr+Reg, SAD wil only see the first one at base-addr.  This is because there is no easy way to know the possible value ranges of the register.
   Loops which access lists of items are similar, only the first few will be identified.


4. other bugs.
  a. There is still a strange bug to do with one subr call not finding arguments.  This does not happen very often, but is not solved as yet.
  b. Conditional jumps and psuedo code - I think I have those sorted now, but there is still a question mark on some JC/JNC jumps.
  c. More work to be done to try to sort out an ELSE and perhaps a WHILE (for loops)
  d. The pseudo code can also show "if ( 0 > 0)" sometimes, when SAD cannot find which instruction last set the condition code (typically over a subroutine call).
  e. Print Layout - Layout options have been added (see below) which probably have some quirks to be ironed out...


New command items -

To allow for new options and functions, the command structure has changed as follows


OLD 
[command text] [Values] [“name”]  [(: or |) data item]  [(: or |) data item] ...

NEW
[command text] [Values] [“name”] [$ global]  [ (: or |) data item] [ (: or |) data item] ...

There are now some 'global' options for the command, which are layout and special purpose options.

in the global options there are -

A - use argument layout (one line per item)       (default for subroutines,arguments. Can be used for structs)

C - use compact layout  (all items in a 'row' on same line)  (default for structs, can be specified for arguments,subroutines)

Q - (moved from data items)  item has a terminator , can be Q <n>  where n is 1-3 bytes. default 1 byte   Struct only

F - (moved from data items) Subroutine has special function.


Other changes -

the F option - moved to global options and now has text strings to define the lookup type, and conforms to the following AMENDED syntax.
This change was made as it became evident that the spec for the sizes is the same as the arguments, and so can easily be confused....


F string par1 par2

where string is

"uuyflu"          unsigned in, unsigned out, byte function (1D) lookup
"usyflu"          unsigned in,   signed out, byte
"suyflu"            signed in, unsigned out, byte
"ssyflu"
"uuwflu"          unsigned in, unsigned out, word function (1D)
"uswflu"
"suwflu"
"sswflu"

"uytlu"          unsigned out, byte table (2D)
"sytlu"            signed out, byte table (2D)

(no word tables found anywhere yet)

par1  is the address of the data structure (function or table)
par2  is the number of columns - table options only






OPT is deprecated in favour of  SETOPTS and CLROPTS   (set and clear options)

Format now uses text strings.

SETOPT : string: string : string ...

where

default		Set/Clear the default options
sceprt		print psuedo source code
tabnames	automatically name new tables
funcnames	automatically name new functions 
ssubnames	automatically name new special function subroutines 
8065		set SAD for 8065 cpu instead of 8061
labelnames	automatically add label names for all jumps
manual		inhibit ALL automatic analysis, just use user commands
subnames	automatically name subroutines when called
signatures	look for signatures (special code sequences, eg, table lookup)
acomments	Auto comments
sympresets      Add preset symbol names for special registers 

(default = sceprt:tabnames:funcnames:ssubnames:subnames:signatures:acomments:sympresets)

probably more options to add, and last two may not work yet.


Comments File.

The old layout had special characters to enhance comments, these have been replaced with "\x" sequences (as I was running out of chars)

OLD     NEW       Purpose
|       \n        Print newline (for block comments)
$       \s        Symbol  Full format \s 81234:6 4321) where 81234 is symbol address, :n is bit number, 4321 is range )
@       \p        Symbol with padding 
^       \w        Wrap  (wrap at comment column for multi lines NOT at start) 
\1      \1        Embed current operand (1-3) in comment (same a psuedo source print)
\2      \2        
\3      \3

















  