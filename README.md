# SymbolicTM
Symbolic Turing Machine for Busy Beaver problems

This program can be used to prove that a Turing Machine terminates or not by
specifying a number of patterns with the use of regular expressions.

The Turing Machine is represented with the format use on
[bbchallenge.org](https://bbchallenge.org/) followed by a empty line.
An example is:
```
 	0 	1 
A 	1RB	0RD
B 	1LC	1LB
C 	1RA	0LB
D 	0RE	1RD
E 	---	1RA

```

A rule consists of a state (named with a single capital starting with 'A')
followed by a colong, a regular expression for the left side of the tape,
the symbol (named with the digits starting with '0'), and a regular
expression for the right side of the tape. In the regular expressions
the '.' stand for any symbol, '+' for a repetition of one or more times
of the sub expression to the left, '*' for a repetition zero or more
times of the sub expression to the left, and '@' for an infinite repetition
of the sub expression to the left. '(' and ')' can be used for grouping.
Furthermore, '|' can be used to specify alternatives, where only one
of the alternatives need to be matched.

The program takes every pattern, applies one step of the Turing Machine
to it, and verifies if the resulting pattern matches with one of the
listed patterns. Depending on the direction a symbol is written to one
tape and a symbol taken from the other tape. If the tape 'starts' (we
assume that the left tape is reverted for this purpose of this description)
of the form A+B, it is first rewritten as AA*B before the first symbol is
taken from A. If it is of the form A*B, then it is also first rewritten to
AA*B but also pattern where the first symbol is taken from B is verified.
If the tape is of the form s@ it is rewritten as ss@.

Let L(E) stand for the language produced the regular expression E, than
we say that A matches B if every string produced by L(A) is included in
L(B). It is known that L(AA+) is equal to L(A+A) and that L(AA*) is equal to L(A+).
The following rules apply:
* A matches .@
* A matches A
* A+ matches A*
* AA+ matches A+
* AB matches CD if A matches C and B matches D
* A* matched B* if A matches B
* AB* matches C+ if A matches C and B matches C
* AB* matches C* if A matches C and B matches C
* AB+ matches C* if A matches C and B matches C
* A matched (B|C) if A matches B or A matches C
* (A|B) matches C if A matches C and B matches C

The program has the following command line options
* `-v`: Verbose output
* `-vv`: Extra verbose output
* `-csa`: Will clean-up (right) rules like `(01)*.@` into `.@`
* `-a` followed by a filename: Read from the file and rewrites it, commenting out repeated and/or matched rules and add missing rules at the end.
* `-ru`: When used in combination with `-a`: Also comment out unused rules.
