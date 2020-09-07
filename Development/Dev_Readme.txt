
Development versions information.

The development subdir contains the latest test versions.  These should be stable to use, but may have odd bugs.

Dev_Versions.txt lists changes/Fixes between the test versions here only.  Full release info in /Docs/Versions.txt) 

Notes on regression errors.

Whilst I am sure there ARE some regression bugs, please be aware that SAD 'final spec' is not fully defined, and some things have been swopped around deliberately.

Symbol names have appeared and disappeared in some places for example, because of the questions around "when should symbols be automatically printed ?" No, it's NOT always obvious.

Formats may change slightly as new things get discovered about the bins.


Ongoing Issues 

1. Subroutine Arguments (and related symbol names)

Until version 4, SAD could not handle variable arguments properly (that is where a subroutine gets variable numbers of bytes according to parameters).

From Version 4, SAD gets the right number of bytes, but it still cannot always determine the true size of each argument in all cases (ie bytes versus words).
This leads to the question about symbol names. Current rule is that arguments determined to be WORDS ARE checked for symbol names, bytes are NOT.   Is this always right ??  I don't know.


2. Conditional jumps 
Because SAD cannot always resolve which opcode set the PSW for each conditional jump, it will sometimes show "if (0 = 0)" style.
The command 'pswset' has been added to specify where the 'source' for the conditional jump is. Probably more work to do here.



3. Data structures not being found.   Where an address is referenced directly, then this is always found.  Same for an [RBASE+offset].  
   Where a 'set' of structures are referenced by [Register+base_addr], SAD will typically only see the first one at base-addr.  This is because there is no easy way to know the possible value ranges of the register.
   Loops which access lists of items are similar, only the first few will be identified.  Vector (pointer) lists do work as they are verified in a different way. Some are missed entirely, an example being where 
   a register is assigned an address which is pushed onto the stack and used later.


4. Conditional jumps psuedo code - there is still some debate about the best way to represent some jumps in sensible way - JC/JNC, JV/JNV.  These are likely to change.

5. Print Layout options may still have some quirks to be ironed out...



A note about address representation.

After reworking bank handling in 4.0.7,  SAD now shows all addresses below 0x400 without a bank number, as they are registers in CPU, and all other addresses as having a bank.
For single banks, no bank numbers are shown anywhere.
This might look strange, as some bins have RAM at 10400, which means that 0x3ff - 0x10400 is actually contiguous, but doesn't look like it from the printout.


Running SAD

Your directives/commands can actually break the processing, and user commands are always taken as 'master'.  So if something doesn't look quite right, please try running SAD without a directive file, or use a file only with SYMbol commands in it. This extra check may help show a command error, or new information.  