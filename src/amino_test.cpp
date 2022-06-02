#include <cstring>
#include <iostream>
#include <stdio.h>
#include <string>
#include <math.h>
#include "classes/aminoacid.h"

using namespace std;

int main(int argc, char** argv)
{
    char letter = 'A';
    if (argc>1)
    {
        int i, j, seqarg=1;

        if (!strcmp("--dat", argv[seqarg]))
        {
            override_aminos_dat = argv[seqarg+1];
            seqarg += 2;
        }

        AminoAcid* firstaa = 0;
        AminoAcid* prevaa = 0;
        for (i=0; argv[seqarg][i]; i++)
        {
            letter = argv[seqarg][i];
            AminoAcid* aa = new AminoAcid(letter, prevaa);
            if (prevaa == aa) throw 0xbad;
            prevaa = aa;
            if (!i) firstaa = aa;

            Bond** b = aa->get_rotatable_bonds();
            AADef* def = aa->get_aa_definition();

            cout << def->name << " bonds can rotate: " << endl;
            if (!b || !b[0]) cout << "None." << endl;
            else
            {
                for (j=0; b[j]; j++)
                {
                    cout << b[j]->atom->name << " - " << b[j]->btom->name << endl;
                }
            }
            cout << endl;
        }

        const char* outfn = "test.pdb";
        FILE* pf = fopen(outfn, "wb");
        AminoAcid* aa = firstaa;
        do
        {
            aa->save_pdb(pf);
        }
        while (aa = aa->get_next());
        fclose(pf);
        cout << "Wrote " << outfn << endl;

		const char* outfn2 = "test.sdf";
		pf = fopen(outfn2, "wb");
		firstaa->save_sdf(pf);
		fclose(pf);
		cout << "Wrote " << outfn2 << endl;
    }
    else
    {
        AminoAcid aa(letter);

        const char* outfn = "test.pdb";
        FILE* pf = fopen(outfn, "wb");
        aa.save_pdb(pf);
        fclose(pf);
        cout << "Wrote " << outfn << endl;

		const char* outfn2 = "test.sdf";
		pf = fopen(outfn2, "wb");
		aa.save_sdf(pf);
		fclose(pf);
		cout << "Wrote " << outfn2 << endl;
    }

    return 0;
}


