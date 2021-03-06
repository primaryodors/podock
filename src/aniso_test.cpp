#include <cstring>
#include <iostream>
#include <stdio.h>
#include <string>
#include <math.h>
#include "classes/molecule.h"

using namespace std;

void set_color(float r, float g, float b)
{
    int lr = fmax(0,fmin(5,r*6));
    int lg = fmax(0,fmin(5,g*6));
    int lb = fmax(0,fmin(5,b*6));

    int ccode = 16 + lb + 6*lg + 36*lr;

    cout << "\x1b[48;5;" << ccode << "m";
}

void clear_color()
{
    cout << "\x1b[49m";
}

int main(int argc, char** argv)
{
    Molecule m("Test");
    cout << "Created empty molecule named " << m.get_name() << ".\n";

    Atom* anisoa;
    bool colors = false;

    if (argc > 1)
    {
        m.from_smiles(argv[1]);
    }
    else
    {
        Atom* C1 = m.add_atom("C", "C1", 0, 0);
        cout << "Added a carbon atom. Its location is " << C1->get_location().printable() << "." << endl;

        Atom* C2 = m.add_atom("C", "C2", C1, 1);
        cout << "Added another carbon atom. Its location is " << C2->get_location().printable() << "." << endl;

        Atom* O3 = m.add_atom("O", "O3", C2, 2);
        cout << "Added an oxygen atom. Its location is " << O3->get_location().printable() << "." << endl;

        Atom* C4 = m.add_atom("C", "C4", C2, 1);
        cout << "Added another carbon atom. Its location is " << C4->get_location().printable() << "." << endl;

        anisoa = O3;
    }

    m.hydrogenate();
    cout << "Hydrogenated." << endl;

    if (argc > 2)
    {
        anisoa = m.get_atom(argv[2]);
        if (!anisoa)
        {
            cout << argv[2] << " not found in molecule. Exiting." << endl;
            return -1;
        }
    }

    if (argc > 4 && !strcmp(argv[4], "colors")) colors = true;

    const int size=22;
    const float ar = 2.1;		// Aspect ratio.
    int x, y, i;
    const char* asciiart = " .':*+=nm@";
    int asciilen = 10;

    Atom probe((argc > 3) ? argv[3] : "H");
    int pz = probe.get_Z();
    Atom oxy(pz == 1 ? "O" : "C");
    probe.bond_to(&oxy, 1);

    Atom* aarr[4];;
    aarr[0] = &probe;
    aarr[1] = &oxy;
    aarr[2] = NULL;

    Molecule mp("probe", aarr);

    InteratomicForce** ifs = InteratomicForce::get_applicable(&probe, anisoa);
    if (!ifs)
    {
        cout << "No forces to measure; check bindings.dat." << endl;
        return -1;
    }

    InteratomicForce* hb=0;
    for (x=0; ifs[x]; x++)
    {
        cout << ifs[x]->get_type() << endl;
        if (ifs[x]->get_type() == hbond)
        {
            hb = ifs[x];
            break;
        }
    }

    if (!hb)
    {
        cout << "No hbond force to measure; check bindings.dat." << endl;
        return -1;
    }

    Point paim(0,10000000,0);

    Bond** bb = anisoa->get_bonds();
    Point aloc = anisoa->get_location();
    Point bloc;
    Point bblocs[16];
    if (bb)
    {
        for (i=0; bb[i]; i++)
        {
            if (!bb[i]->btom) break;
            Point pt = bb[i]->btom->get_location();
            pt = pt.subtract(aloc);
            pt.scale(1);
            pt = pt.add(aloc);
            pt.weight = 1;
            bblocs[i] = pt;
            // cout << bb[i]->btom->name << endl;
        }

        // cout << i << " points for average." << endl;
        if (i) bloc = average_of_points(bblocs, i);
        // mp.add_atom("He", "He1", &bloc, NULL, 0);

        delete bb;
    }


    paim = paim.add(bloc);

    // cout << "Align " << aloc << " to " << paim << " centered at " << bloc << endl;
    Rotation rot = align_points_3d(&aloc, &paim, &bloc);

    // cout << rot << endl;

    m.rotate(&rot.v, rot.a);

    aloc = anisoa->get_location();

    for (y=-size; y<=size; y++)
    {
        for (x=-size*ar; x<=size*ar; x++)
        {
            float lx = (float)x/ar;
            float r = sqrt(lx*lx + y*y);
            if (r > size) cout << " ";
            else
            {
                float phi = find_angle(lx, y);
                float theta = (1.0-r/(size))*(M_PI/2);

                SCoord v(hb->get_distance(), theta, phi);
                Point loc = anisoa->get_location().add(&v);

                probe.move(&loc);
                probe.clear_geometry_cache();

                v.r = 1;
                loc = loc.add(&v);

                oxy.move(&loc);
                oxy.clear_geometry_cache();

                float tb = InteratomicForce::total_binding(anisoa, &probe);
                tb /= hb->get_kJmol();	// This is not working.
                if (tb<0) tb=0;

                if (!colors)
                {
                    tb *= asciilen;
                    if (tb > asciilen-1) tb = asciilen-1;

                    cout << asciiart[(int)tb];
                }
                else
                {
                    set_color(tb*tb,tb,sqrt(tb));
                    cout << " ";
                    clear_color();
                }

                if (!x && !y)
                {
                    int anisg = anisoa->get_geometry();
                    SCoord* anisgeo = anisoa->get_geometry_aligned_to_bonds();
                    if (anisgeo)
                        for (i=0; i<anisg; i++)
                        {
                            Point pt(&anisgeo[i]);
                            pt.scale(0.4);
                            pt = pt.add(aloc);
                            char buffer[10];
                            sprintf(buffer, "He%d", i);
                            mp.add_atom("He", buffer, &pt, NULL, 0);
                        }

                    FILE* pf = fopen("aniso.sdf", "wb");
                    Molecule* ligands[3];
                    ligands[0] = &mp;
                    ligands[1] = NULL;
                    m.save_sdf(pf, ligands);
                    fclose(pf);
                }
            }
        }
        cout << endl;
    }

}









