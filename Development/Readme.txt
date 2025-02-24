
Development versions information.


The development subdir contains the latest test versions.  These should be stable to use, but probably have bugs, and may crash.

Windows builds (32 bit, built with CodeLite/gnu) in Win32,  Linux builds (64 bit, built with Codelite/gcc) in Linux64.


"Dev_x_Versions.txt" lists changes/fixes between dev versions.  Stable releases info in /Docs/Versions.txt 

--- Running SAD ---

Please be aware that your commands (in _dir file) are treated as 'master', and can actually break the processing.
Therefore if something doesn't look quite right, please try running SAD without a directive file, or use a _dir version only with SYMbol commands in it.
This extra check may help show a user command error, and/or new information in case of a SAD bug.  



---- Generic address rules applied from version 4.0.7 onwards  -----

1. Output 
 In single banks, no bank numbers are shown anywhere. all addresses are 16 bits (4 hex digits)

 In multibanks, addresses below 0x400 are shown with no bank number - these are registers internal to CPU. 
   All other addresses must have a bank number (i.e. 5 digits). This might look strange in some cases, but is technically correct,
   as any address over 0x3ff is external to the CPU and therefore MUST have a bank.

 where SAD can automatically verify a value is an address it will add a bank automatically (typically bank 1 for data bank)


2. Input (in commands).

 In single banks, no bank number is required.  ( Internally, single banks are treated as bank 8 to make code logic simpler) .   

 In multibanks, any address above 0x3ff MUST have a bank number (i.e. 5 digits). Valid banks are 0,1,8,9
   To maintain compatibility with other tools, addresses below 0x2000 can be input in SAD commands without a bank, 
   and they will automatically be assigned as Bank 1. (i.e. 0x400 - 0x1fff becomes 0x1400 - 0x11fff)


---------------------------------------------------------------------------------------------

SAD version 5.     Alpha version release  

V5 internal analysis model has been rewritten, but this did not work as well as intended. Work is still ongoing....
version 4 still maintained.

Main changes 

Internal - Split code into modules by purpose. 
Internal - Tidy code, make more C++ like, with internal data and handler subroutines put together.

Bin files and prinouts

True 'subfields' throughout, of any size up to 32 bits.

  e.g. In data structures, subfields down to separate bit flags are used in many binaries. 
       Timer structures typically have bit masks and pointers to registers. Injection structure again typically has mix of bitmasks, pointers to subroutines, 'offset' pointers
       to event queues, and sometimes fields are used more than once.
 
To handle this better, all 'data items/fields' are now defined as 'start bit to end bit' anywhere within a 32 bit 'long'. Items can be signed (top bit of data item/field is sign bit). Size can be anywhere from 1 to 31 bits. Effectively all operands from opcodes therefore start at bit 0.

Add a 'calc' command to do multi-term math type calculations on any field/item (except operands themselves).

NEW COMMAND SYNTAX to support the new data definitions.  This syntax uses matched square brackets to describe a single data item. Colon delimiter ':' is replaced by '[]'
this is so that Data items can be NESTED to define fields within fields, and repeated (maybe with different print options) for printing purposes. 

The colon ':' form of command is still useable to preserve backwards compatibility, but is converted internally so outputs are always in NEW format




