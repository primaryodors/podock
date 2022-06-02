
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include "classes/molecule.h"

using namespace std;

int main(int argc, char** argv)
{
    Molecule m("Test");
    cout << "Created empty molecule named " << m.get_name() << ".\n";

    if (argc > 1)
    {
        m.from_smiles(argv[1]);
    }
    else
    {
        Atom* C1 = m.add_atom("C", "C1", 0, 0);
        char* buffer;
        cout << "Added a carbon atom. Its location is " << (buffer = C1->get_location().printable()) << "." << endl;
        delete buffer;

        Atom* C2 = m.add_atom("C", "C2", C1, 1);
        cout << "Added another carbon atom. Its location is " << (buffer = C2->get_location().printable()) << "." << endl;
        delete buffer;

        Atom* O3 = m.add_atom("O", "O3", C2, 1);
        cout << "Added an oxygen atom. Its location is " << (buffer = O3->get_location().printable()) << "." << endl;
        delete buffer;
    }

    m.hydrogenate();

    cout << "Internal clashes: " << m.get_internal_clashes() << " cu.A." << endl;

    cout << "Minimizing internal clashes..." << endl;
    m.minimize_internal_clashes();

    cout << "Internal clashes: " << m.get_internal_clashes() << " cu.A." << endl;

    const char* tstoutf = "test.sdf";
    FILE* pf = fopen(tstoutf, "wb");
    if (m.save_sdf(pf)) cout << "Saved " << tstoutf << endl;
    else cout << "Failed to save " << tstoutf << endl;
    fclose(pf);
    
    const char* tstpdbf = "test.pdb";
    pf = fopen(tstpdbf, "wb");
    m.save_pdb(pf);
    cout << "Saved " << tstpdbf << endl;
    fclose(pf);

    return 0;
}