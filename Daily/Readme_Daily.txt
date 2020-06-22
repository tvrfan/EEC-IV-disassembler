
Daily version information.

The daily subdir contains test version(s), should be stable, but may have odd bugs.

Versions.txt tracks changes between the test versions here in /Daily only, not the full releases (which is in /Docs/versions.txt) 

Notes on regression errors.

Whilst I am sure there ARE some regression bugs, please be aware that the 'final spec' is not fully defined, and some things have been swopped around deliberately.

Symbol names have appeared and disappeared in some places for example, because of the questions around "when should symbols be automatically printed ?" No, it's NOT always obvious.


Subroutine Arguments (and related to symbol names)

Until version 4, SAD could not handle variable arguments properly (that is where a subroutine gets variable numbers of bytes).

Even now, whilst SAD gets the right number of bytes, it still cannot always determine the true size of each argument in all cases.
This leads to the question about symbol names. Current rule is that arguments determined to be WORDS ARE checked for symbol names, bytes are NOT.   Is this always right ??  I don't know.


Conditional jumps 
Becuase SAD cannot always resolve which opcode set the PSW for each conditional jump, it will sometimes show "if ( 0 = 0)" style.
The command 'pswset' has been added to specify where the 'source' for the conditional jump is. Possibly more work to do here.





There are still things/bugs outstanding.

1. Correctly size subroutine arguments

2. Data structures not being found.   Where an address is referenced directly, then this is fine.  (same for an RBASE+offset)
   Where a 'set' of structures are referenced by base_addr+Reg, SAD wil only see the first one at base-addr.  This is because there is no easy way to know the possible value ranges of the register.
   Loops which access lists of items are similar, only the first few will be identified.

3. There is still a strange bug to do with one subr call not finding arguments.  This does not happen very often, but is not solved as yet.

4. Conditional jumps psuedo code - there is still some debate about the best way to represent some jumps in sensible way - JC/JNC, JV/JNV.  These are likely to change.

5. Print Layout options may still have some quirks to be ironed out...