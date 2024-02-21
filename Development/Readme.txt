
Development versions information.


The development subdir contains the latest test versions.  These should be stable to use, but probably have bugs.

Windows builds (32 bit, built with CodeLite/gnu) in Win32,  Linux builds (64 bit, built with Codelite/gcc) in Linux64.


Dev_Versions.txt lists changes/fixes between dev versions only.   Stable release info in /Docs/Versions.txt 

Generic address rules from version 4.0.7

1. Output 
 In single banks, no bank numbers are shown anywhere. all addresses are 16 bits (4 hex digits)

 In multibanks, addresses below 0x400 are shown with no bank number - these are registers internal to CPU. 
   All other addresses have a bank (i.e. 5 digits). This might look strange in some cases, but is technically correct,
   as any address 0x400 and above must be external to CPU and therefore MUST have a bank.

 where SAD can verify a value must be an address it will add the relevant bank automatically (typically bank 1 for data)

2. Input (in commands).

 In single banks, no bank number is required.  

 In multibanks, any address 0x400 and over MUST HAVE A BANK (i.e. 5 digits).  Valid banks are 0,1,8,9
   To maintian compatibility with other tools, addresses below 0x2000 can be input in SAD commands without a bank, 
   and they will automatically be amended to Bank 1 (0x1400 - 0x1ffff)

3. Running SAD

Please be aware that your commands (in .dir file) can actually break the processing, because user commands are always taken as the 'master'.
So if something doesn't look quite right, please try running SAD without a directive file, or use a version only with SYMbol commands in it.
This extra check may help show a user command error, and/or new information in case of a bug.  

---------------------------------------------------------------------------------------------

SAD version 5.     Alpha version release

Main changes 

Split into modules by their purpose. 
Tidy code. made inot more C++ like, data + handler subroutines together).
True 'subfields'. In data structures, subfields are used in many binaries. Timers are a good example.
Add 'calc' to do math calculations on any field.

All 'data items' are now  start bit to end bit anywhere up to  0 to 32 bits long.  Items can be signed (top bit of field is sign bit). 

NEW COMMAND SYNTAX to support the additional data definitions.  This uses square brackets to describe a single data item. Colon delimiter ':' moves to '[]'
Data items can be NESTED to define fields within fields and reuse for printing. 

The colon ':' form of command is still useable to preserve backwards compatibility, but is converted so outputs are always in NEW format

V5 internal analysis model has been rewritten, using priciple that 'every code block is scanned only once', to make things simpler and faster.

Work is still ongoing....
