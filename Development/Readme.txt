
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

SAD version 5.     Withdrawn 





