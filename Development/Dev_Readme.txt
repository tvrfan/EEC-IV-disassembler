
Development versions information.

The development subdir contains the latest test versions.  These should be stable to use, but may have odd bugs.

.exe are Windows build (32 bit, built with CodeLite/gnu) and linux (64 bit) are in 'Linux_64' subdirectory (built with Codelite/gcc)


Dev_Versions.txt lists changes between these dev versions here only.   Stable release info in /Docs/Versions.txt 

Notes on regression errors.

Whilst I am sure there are regression bugs, please be aware that SAD 'final spec' is not fully defined, and some things have been swopped around deliberately as new info arrives.

Symbol names have appeared and disappeared in some places for example, because of questions around "when should symbols be automatically printed ?"  No, it's NOT always obvious.

Formats (and names) may change slightly as new things get discovered about the bins and CPU details.


Ongoing Issues 

1. Subroutine Arguments (and related symbol names)

Until version 4, SAD could not handle variable arguments properly (that is where a subroutine gets variable numbers of bytes according to parameters).

From Version 4, SAD now gets the right number of bytes, but it still cannot always determine the true size of each argument (ie bytes versus words).
This leads to the question about symbol names.  Current rule is that arguments determined to be WORDS ARE checked for symbol names, bytes are NOT.
Is this always right ??  I honestly don't know.   You can specify argument layout with the ARGS command, and what gets a symbol name. 


2. Conditional jumps 
Because SAD cannot always resolve which opcode set the PSW for each conditional jump, it will sometimes show "if (0 = 0)" style.
The command 'pswset' has been added to specify where the 'source' for the conditional jump is.  Probably more work to do here.


3. Data structures not being found.

Where an address is referenced directly, then data is always found.   Same for an [RBASE+offset] style, used extensively in later bins.  
Where a 'list' of structures are referenced by [Register+base_addr], SAD will typically only get the first one at base-addr.
This is because there is no easy way to know the possible value ranges of the register.
Loops which access lists of items are similar, only the first few will be identified.
Vector (pointer) lists do work as they are verified in a different way. Some are missed entirely, an example being where 
a register is assigned an address which is pushed onto the stack and used later on.

4. Conditional jumps and 'psuedo code' - there is still some debate about the best way to represent some jumps in sensible way - JC/JNC, JV/JNV.  So these are likely to change.

5. Print Layout options may still have some quirks to be ironed out...and more features requested have not been done yet

6. Address representation.  (After reworking bank handling in 4.0.7)
In single banks, no bank numbers are shown anywhere.
In multibanks,   addresses below 0x400 are shown without a bank number - they are registers in CPU. All other addresses as having a bank (i.e. 5 digits).
This might look strange, as some bins have RAM at 10400, which means that the 0x3ff - 0x10400 step can be treated as a contiguous block in a loop, which doesn't look right.
This does represent what would happen in real 8065 hardware however, with data bank set as 1 (the typical setup). (NB. development versions 4.08 on have tighter checks)


Running SAD

Please be aware that your commands (in .dir file) can actually break the processing, because user commands are always taken as the 'master'.
So if something doesn't look quite right, please try running SAD without a directive file, or use a version only with SYMbol commands in it.
This extra check may help show a command error, and/or new information.  

