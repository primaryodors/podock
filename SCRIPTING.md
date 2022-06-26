
# POdock Scripting Language Documentation

To run a protein editing script, after building POdock, please use the following command:

```
bin/interpreter path/to/script.po
```

# Variables

With few exceptions, any variable can stand in for any parameter of any command.

All integers begin with `%`, e.g. `%start_resno`.
All floats begin with `&`, e.g. `&transpose_amt`.
All Cartesian 3D locations begin with `@`, e.g. `@pocket_center`.
All strings begin with `$`, e.g. `$name`.

Cartesians have members .x, .y, and .z that behave as floats.

Casting a float to an integer rounds the value (e.g. 0.4 rounds to zero but 0.5 rounds to 1).
Casting a float to a Cartesian normally sets the .x member to the float value. But a command like `LET @foo = @bar.y` will set the value of `@foo.y`.
Casting an int to a Cartesian obtains the location of the CA atom for that residue number, if it exists.

The following "magic variables" are supplied upon loading a protein:
- `$PDB` the path and name of the source PDB file.
- `$PROTEIN` the name of the protein, derived from a `REMARK 6` record if present.
- `%SEQLEN` the length of the protein sequence.
- `$SEQUENCE` the sequence of the protein in standard one-letter amino acid code.
- Region start and end residue numbers from any `REMARK 650 HELIX` records, e.g. `%TMR3.s` and `%TMR3.e` for the start and end resnos of TMR3.

The following commands are supported:

# ALIGN
Example:
```
ALIGN 174 182 4 @location1 @location2
```

Repositions a piece of the protein so that the residues near the start resno occur at one location, and the residues near the end resno occur in the
direction of the other location.

The first two numbers are the starting and ending residue numbers. They define the range of residues that will be moved.

The next number is the averaging length from each end of the region. In the example this number is 4, so the locations of the CA atoms of four residues
at each end will be averaged together, in this case 174-177 and 179-182, to determine the alignment. The piece will be moved so that the average of CA
atom locations of residues 174-177 will now equal @location1, and the average of CA locations of residues 179-182 will fall along a straight line pointing
from @location1 to @location2.

# CONNECT
Examples:
```
CONNECT 160 174
CONNECT 160 174 250
```

Flexes a section of protein backbone to try to reconnect a broken strand, for example after a HELIX and/or ALIGN command.

The first parameter is the starting residue number of the section to be flexed. The second parameter is the target residue to reconnect to. In these
examples, the program will flex the range of 160-173 and attempt to reunite 173 with 174. The last optional parameter is the number of iterations, the
default being 50.

# DUMP
Example:
```
DUMP
```

Useful for debugging, this outputs a list of all variables and their values to stdout. It does not take any parameters.

# ECHO
Examples:
```
ECHO $PROTEIN
ECHO $SEQUENCE " is " %SEQLEN " residues long."
ECHO @location ~
```

Writes information to stdout.

Unlike the `LET` command, `ECHO` does not require a plus sign for concatenation.

If a tilde `~` occurs at the end of the statement, no newline will be output. Otherwise, a newline will be added to whatever was echoed.

# END / EXIT / QUIT
Examples:
```
END
EXIT
QUIT
```

Ceases script execution.

# GOTO
Example:
```
GOTO label
...
label:
```

Diverts script execution to the specified label. A line must exist in the script that consists of only the label and a colon `:` character, with
no spaces and no other characters.

# LET
Examples:
```
LET %i = 1
LET $name = "POdock"
LET $range = $SEQUENCE FROM 174 FOR 20
LET $sub = $name FROM 3 FOR 4
LET &j = %i
LET @res174loc = 174
LET &j += @res174loc.y
LET @loc3 = @loc1 + @loc2
LET $message = "TMR4 ends on residue " + %TMR4.e + " and TMR5 starts on residue " + %TMR5.s + "."
```

Assigns a value to a new or existing variable.

For assigning integer and float variables, the following operators are allowed: `=` `+=` `-=` `*=` `/=`.

Integer variables are also allowed `++` `--`, which do not take an r-value (a variable or literal value to apply to the variable assignment).

For algebraic assignments, e.g. `LET &k = %i + &j`, the following operators are supported: `+` `-` `*` `/` `^`, the last being an exponent operator.
Note parentheses are not currently supported. Also, the data type of the variable being assigned determines how all r-value variables are interpreted,
so even though `%i` is an integer in the above statement, its value is cast to a float before `&j` is added to it.

For strings, the operators `=` `+=` and `+` are allowed, with the plus sign indicating concatenation.
Substrings are indicated with the word FROM and (optinally) FOR, e.g. `LET $end = $string FROM 10` or `LET $range = $SEQUENCE FROM %start FOR %length`.

# HELIX
Examples:
```
HELIX ALPHA 174 182
HELIX -57.8 -47.0 174 182
```

Create a helix of the specified residue range. If the helix type is specified, it cannot be a variable and must be one of: `ALPHA`, `PI`, `3.10`,
`PPRO1` (polyproline I), `PPRO2` (polyproline II), `BETA`, or `STRAIGHT`. Note that while the last two aren't helices, they use the same syntax
because like with helices, the residues' phi and psi bonds are rotated to preset angles.

Phi and psi angles can also be manually specified as in the second example.

# LOAD
Example:
```
LOAD "path/to/protein.pdb"
```

Loads the specified protein into working memory. Note only one protein is allowed in memory at a given time.

# SAVE
Example:
```
SAVE "path/to/output.pdb"
SAVE "path/to/output.pdb" QUIT
```

Saves the protein in working memory to the specified file path (can be a string variable).

If `QUIT`, `EXIT`, or `END` follows the path parameter, script execution will terminate immediately after saving the protein.

# SEARCH
Example:
```
SEARCH 1 %SEQLEN "MAYDRYVAIC" %olfr_motif
```

Performs a fuzzy search on the specified residue range looking for the closest match to the indicated search term, and stores the starting resno 
of the result in the variable given as the 4th parameter. The fuzzy search matches amino acids based on their similarity using the parameters of
hydrophilicity, aromaticity, metal binding capability, and charge. This search finds the closest match *no matter what*, so it cannot be used to
determine whether a motif is present, only where the search string most closely matches the sequence.










