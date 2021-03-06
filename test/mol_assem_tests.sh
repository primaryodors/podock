
# Note since {AtomName} is a nonstandard feature, the code will always attempt to natively decode a string
# containing { rather than integrate with a third party app.
# The ! char suppresses the warning about native functionality often getting rings wrong.

test/mol_assem_test 'c{C1}1ccccc1!' test/benzene.sdf
test/mol_assem_test 'C{C1}c1ccccc1!' test/toluene.sdf
test/mol_assem_test 'c{C1}1ccccc1!C' test/toluene1.sdf
test/mol_assem_test 'C{C1}1CCCCC1!' test/cyclohexane.sdf
test/mol_assem_test 'C{C1}1CCCC1!' test/cyclopentane.sdf
test/mol_assem_test 'C{C1}1CCC1!' test/cyclobutane.sdf
test/mol_assem_test 'C{C1}1CC1!' test/cyclopropane.sdf
test/mol_assem_test 'C{C1}1=CCC1!' test/cyclobutene.sdf
test/mol_assem_test 'C{C1}1=CC=C1!' test/cyclobutadiene.sdf
test/mol_assem_test 'C{C1}1=CC1!' test/cyclopropene.sdf
test/mol_assem_test 'C{C1}1(C=CC=C1!)=C2C=C2' test/calicene.sdf
test/mol_assem_test 'O{O1}c1c(C)cccc1!C' test/2_6-xylenol.sdf
test/mol_assem_test 'N{N1}1[C@H](C(=O)O)CCC1!' test/proline.sdf
test/mol_assem_test 'C{C1}c1c([Cl])c(CSC)c(O)c(C(=O)[O-])c1![NH3+]' test/total_substitution.sdf
test/mol_assem_test 'c{C1}1cc[o+](C)c1!' test/o-methylfuranium_ion.sdf
test/mol_assem_test 'c{C1}1c2ccccc2ccc1!' test/naphthalene.sdf
test/mol_assem_test 'c{C1}1ccccc1!Oc2ccccc2' test/diphenyl_ether.sdf
test/mol_assem_test 'c{C1}1ccccc1!N=Nc2ccccc2' test/azobenzene.sdf
test/mol_assem_test 'n{N1}1c[nH]cc1!' test/imidazole.sdf
test/mol_assem_test 'C{C1}C/C=C\CCO' test/leaf_alcohol.sdf
test/mol_assem_test 'C{C1}([C@@H]1[C@H]([C@@H]([C@H](C(O1!)O)O)O)O)O' test/glucose.sdf
test/mol_assem_test 'C{C1}([C@@H]1[C@H]([C@@H]([C@H](C(S1!)S)O)S)S)O' test/tetrathioglucose.sdf
test/mol_assem_test 'C{C1}([C@@H]1[C@H]([C@@H]([C@H](C(S1!)S)S)S)S)S' test/hexathioglucose.sdf

