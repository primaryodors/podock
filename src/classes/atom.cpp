
#include <cmath>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <unistd.h>
#include <climits>
#include "atom.h"

#define _DBGMOVES 0
#define _DBGGEO 0

using namespace std;

/*const char* elements[] = {"n","H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn","Fr","Ra","Ac","Th","Pa","U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr",
						  "Rf","Db","Sg","Bh","Hs","Mt","Ds","Rg","Cn","Nh","Fl","Mc","Lv","Ts","Og",NULL
						 };*/
char* Atom::elem_syms[_ATOM_Z_LIMIT];	// Allow for future exapnsion of the periodic table.
float Atom::vdW_radii[_ATOM_Z_LIMIT];
float Atom::electronegativities[_ATOM_Z_LIMIT];
float Atom::ioniz_energies[_ATOM_Z_LIMIT];
float Atom::elec_affinities[_ATOM_Z_LIMIT];
float Atom::atomic_weights[_ATOM_Z_LIMIT];
int Atom::valences[_ATOM_Z_LIMIT];
int Atom::geometries[_ATOM_Z_LIMIT];

void Atom::read_elements()
{
    if (read_elem_syms) return;
    FILE* pf = fopen("elements.dat", "rb");
    if (pf)
    {
        char buffer[1024];
        while (!feof(pf))
        {
            fgets(buffer, 1003, pf);
            if (buffer[0] != '#')
            {
                char** fields = chop_spaced_fields(buffer);
                if (!fields) continue;

                if (fields[0])
                {
                    int lZ = atoi(fields[0]);
                    if (lZ && fields[1])
                    {
                        elem_syms[lZ] = new char[3];
                        strcpy(elem_syms[lZ], fields[1]);
                        // cout << fields[6] << endl;

                        if (fields[2]) vdW_radii[lZ] = atof(fields[2]);
                        if (fields[3]) electronegativities[lZ] = atof(fields[3]);
                        if (fields[4]) atomic_weights[lZ] = atof(fields[4]);
                        if (fields[5]) valences[lZ] = atoi(fields[5]);
                        if (fields[6])
                        {
                            geometries[lZ] = atoi(fields[6]);
                            if (fields[6][1] == 'p') geometries[lZ] = -geometries[lZ];

                            if (fields[7])
                            {
                                ioniz_energies[lZ] = atof(fields[7]);
                                if (fields[8])
                                {
                                    elec_affinities[lZ] = atof(fields[8]);
                                }
                            }
                        }
                    }
                }

                delete[] fields;
            }
            buffer[0] = 0;
        }
        fclose(pf);
        read_elem_syms = true;
    }
    else
    {
        char buffer[PATH_MAX];
        getcwd(buffer, PATH_MAX);
        cout << "ERROR elements.dat not found in " << buffer << ".\n";
    }
}

void Atom::figure_out_valence()
{
    // Subtract successive 2, 6, 6, 10, 10, 14, 14 from Z until you can't.
    int sub, lsub, subt, sh, z;
    lsub = sub = sh = 2;
    z = Z;
    while (z >= sub)
    {
        z -= sub;
        subt = sub;
        if (sub == lsub)
        {
            sh += 4;
            sub += sh;
        }
        lsub = subt;
    }

    if (sub > 8 && z > 4) z -= (sub-8);

    if (z > 0) family = z;
    if (z == 0) family = 8;

    if (elem_syms[Z] && geometries[Z])
    {
        valence = valences[Z];
        geometry = geometries[Z];
        if (valence > abs(geometry)) cout << "ERROR Valence for " << elem_syms[Z] << " exceeds geometry! Check values in elements.dat are correct.\n";
    }
    else
    {

        valence = 0;

        // If the remainder < 8, that's your geometry. Valence = geometry up until 4.
        if (z < 8)
        {
            geometry = valence = z;
            // If the next subtractor minus the remainder < 4, that's your valence.
            valence = (geometry <= 4) ? geometry : (8 - geometry);
            if (geometry == 3) geometry = 4;
        }

        if (!geometry) geometry = (Z <= 2) ? Z : 8;

        // Except: elements 7 and 8 are tetrahedral, and 9 has a geometry of 1.
        if (Z == 7 || Z == 8) geometry = 4;
        if (Z == 9 || Z == 17 || Z == 35 || Z == 53) geometry = 1;

        // Elements 29, 30, 48, 80, 82 have a valence of 2 and geometry of 4; 47, 79, 81 have a valence of 1 and geometry of 4.
        if (Z == 29 || Z == 30 || Z == 48 || Z == 80 || Z == 82)
        {
            valence = 2;
            geometry = 4;
        }
        if (Z == 47 || Z == 79 || Z == 81 || Z == 111)
        {
            valence = 1;
            geometry = 4;
        }
        if (Z == 72 || Z == 104)
        {
            valence = geometry = 4;
        }

        // Otherwise, assume a valence of 3 and a geometry of 4.
        if ((Z >= 23 && Z <= 28)
                ||
                (Z >= 41 && Z <= 46)
                ||
                (Z >= 57 && Z <= 71)
                ||
                (Z >= 73 && Z <= 78)
                ||
                (Z >= 91 && Z <= 103)
                ||
                (Z >= 105 && Z <= 110)
           )
        {
            valence = 3;
            geometry = 4;
        }

        if (valence < 0) valence = 3;
        if (valence > 8) valence = 8;
    }
    origgeo = geometry;
    
    if (valence > abs(geometry)) throw VALENCE_EXCEEDS_GEOMETRY;

    if (Z >=  21 && Z <=  30) family = HEAVYMETAL;
    if (Z >=  39 && Z <=  48) family = HEAVYMETAL;
    if (Z >=  57 && Z <=  71) family = LANTHANIDE;
    if (Z >=  72 && Z <=  80) family = HEAVYMETAL;
    if (Z >=  89 && Z <= 103) family = ACTINIDE;
    if (Z >= 104 && Z <= 112) family = HEAVYMETAL;

    elecn = electronegativities[Z];
    Eaffin = elec_affinities[Z];
    at_wt = atomic_weights[Z];
    Eion = ioniz_energies[Z];
    location.weight = at_wt;
    vdW_rad = vdW_radii[Z];
}

int Atom::Z_from_esym(const char* elem_sym)
{
    read_elements();

    int i;
    for (i=1; elem_syms[i]; i++)
    {
        if (!strcmp(elem_sym, elem_syms[i]))
        {
            return i;
            break;
        }
    }

    return 0;
}

Atom::Atom(const char* elem_sym)
{
    Z = Z_from_esym(elem_sym);

    reciprocity = used = false;

    figure_out_valence();

    bonded_to = new Bond[abs(geometry)];
    int i;
    for (i=0; i<geometry; i++) bonded_to[i].btom = nullptr;
    strcpy(aa3let, "LIG");
    residue = 999;
}

Atom::Atom(const char* elem_sym, const Point* l_location)
{
    location.x = l_location->x;
    location.y = l_location->y;
    location.z = l_location->z;
    location.weight = at_wt;

    Z = Z_from_esym(elem_sym);

    reciprocity = used = false;

    figure_out_valence();

    bonded_to = new Bond[abs(geometry)];
    int i;
    for (i=0; i<geometry; i++) bonded_to[i].btom = nullptr;
    strcpy(aa3let, "LIG");
    residue = 999;
}

Atom::Atom(const char* elem_sym, const Point* l_location, const float lcharge)
{
    location.x = l_location->x;
    location.y = l_location->y;
    location.z = l_location->z;
    location.weight = at_wt;
    charge = lcharge;

    Z = Z_from_esym(elem_sym);

    reciprocity = used = false;

    figure_out_valence();

    bonded_to = new Bond[abs(geometry)];
    int i;
    for (i=0; i<geometry; i++) bonded_to[i].btom = nullptr;
    strcpy(aa3let, "LIG");
    residue = 999;
}

Atom::Atom(FILE* is)
{
    char buffer[1024], res3let[5];
    int resno=0;
    init_nulls(buffer, 1024);

    while (1)
    {
        fgets(buffer, 1022, is);
        char** fields = chop_spaced_fields(buffer);

        if (fields)
        {
            if (!strcmp(fields[0], "ATOM")
                    ||
                    !strcmp(fields[0], "HETATM")
               )
            {
                try
                {
                    resno = atoi(fields[4]);
                    strcpy(res3let, fields[3]);

                    char esym[7] = {0,0,0,0,0,0,0};
                    if (fields[2][0] >= '0' && fields[2][0] <= '9')
                        strcpy(esym, &fields[2][1]);
                    else
                        strcpy(esym, fields[2]);

                    int i;
                    if (fields[10])
                    {
                        esym[0] = fields[10][0];
                        esym[1] = fields[10][1] | 0x20;
                        if (esym[1] < 'a') esym[1] = 0;
                        else esym[2] = 0;
                    }
                    else if (fields[9] && fields[9][0] >= 'A')
                    {
                        esym[0] = fields[9][0];
                        esym[1] = fields[9][1] | 0x20;
                        if (esym[1] < 'a') esym[1] = 0;
                        else esym[2] = 0;
                    }
                    else
                    {
                        for (i=1; i<6; i++)
                        {
                            if (!esym[i+1]) esym[i] = 0;
                            if (esym[i+1] >= '0' && esym[i+1] <= '9') esym[i]=0;
                            if (i>1) esym[i] = 0;
                            if (!esym[i]) break;
                        }
                        esym[1] |= 0x20;
                    }

                    name = new char[5];
                    strcpy(name, fields[2]);

                    Z = Z_from_esym(esym);

                    reciprocity = used = false;

                    figure_out_valence();
                    bonded_to = new Bond[abs(geometry)];
				    for (i=0; i<geometry; i++) bonded_to[i].btom = nullptr;

                    Point aloc(atof(fields[5]), atof(fields[6]),atof(fields[7]));
                    location = aloc;

                    char* strcharge = 0;
                    int chgoff = strlen(esym);
                    if (fields[10])
                    {
                        if (fields[10][chgoff-1] && fields[10][chgoff]) strcharge = &fields[10][chgoff];
                    }
                    else if (fields[9])
                    {
                        if (fields[9][0] == esym[0]
                                &&
                                fields[9][chgoff-1]
                                &&
                                fields[9][chgoff]
                           ) strcharge = &fields[9][chgoff];
                    }
                    if (strcharge)
                    {
                        if 		(!strcmp(strcharge, "+")) charge = 1;
                        else if	(!strcmp(strcharge, "++")) charge = 2;
                        else if	(!strcmp(strcharge, "-")) charge = -1;
                        else if	(!strcmp(strcharge, "--")) charge = -2;
                        else if (atoi(strcharge)) charge = atoi(strcharge);
                    }

                    residue = atoi(fields[4]);
                    strcpy(aa3let, fields[3]);

                    return;
                }
                catch (int ex)
                {
                    cout << "Failed to read PDB data. Please inspect the following line for errors:" << endl;
                    cout << buffer << endl;
                    throw 0xbadda7a;
                }
            }
        }
        buffer[0] = 0;
    }
}

Atom::~Atom()
{
    /*if (region) delete[] region;
    if (name) delete[] name;
    if (bonded_to) delete[] bonded_to;*/

    if (bonded_to)
    {
        int i;
        for (i=0; i<geometry; i++)
            if (bonded_to[i].btom)
                bonded_to[i].btom->unbond(this);
    }
}

void Atom::unbond(Atom* btom)
{
	// if (btom) cout << "Unbonding " << btom->name << " from " << name << endl;
    if (bonded_to)
    {
        int i;
        for (i=0; i<geometry; i++)
        {
            if (bonded_to[i].btom == btom)
            {
            	if (!reciprocity)
            	{
            		bonded_to[i].btom->reciprocity = true;
            		bonded_to[i].btom->unbond(this);		// RECURSION!
            		bonded_to[i].btom->reciprocity = false;
        		}
                bonded_to[i].btom = NULL;
                bonded_to[i].cardinality=0;
                bonded_to[i].can_rotate=0;
            }
        }
    }
}

void Atom::unbond_all()
{
	int i;
	for (i=0; i<geometry; i++)
	{
		if (bonded_to[i].btom)
		{
			bonded_to[i].btom->reciprocity = true;
    		bonded_to[i].btom->unbond(this);		// Potential for recursion!
    		bonded_to[i].btom->reciprocity = false;
    		
            bonded_to[i].btom = NULL;
            bonded_to[i].cardinality=0;
            bonded_to[i].can_rotate=0;
		}
	}
}

const char* Atom::get_elem_sym()
{
    return elem_syms[Z];
}

Point Atom::get_location()
{
    Point pt = location;
    return pt;
}

Atom** Bond::get_moves_with_btom()
{
	enforce_moves_with_uniqueness();
	
    int i, cachesz=0;
    if (!moves_with_btom) fill_moves_with_cache();
    if (!moves_with_btom) return 0;
    for (i=0; moves_with_btom[i]; i++)
        cachesz = i+1;
    if (!cachesz) return 0;

    Atom** retval = new Atom*[cachesz+1];
    // Not calling init_nulls() for performance reasons.
    for (i=0; i<cachesz; i++)
        retval[i] = moves_with_btom[i];
    retval[cachesz] = 0;

    return retval;
}

bool Atom::move(Point* pt)
{
	#if debug_break_on_move
	if (break_on_move) throw 0xb16fa7012a96eca7;
	#endif

	/*if (name && !strcmp(name, "CB"))
	{
		Bond* b = get_bond_between("CA");
		if (b && b->btom)
		{
			float r = b->btom->get_location().get_3d_distance(pt);
			if (r > 1.55) throw 0x7e57196;
		}
	}*/
	
    location = *pt;
    location.weight = at_wt;
    geov = NULL;
    return true;
}

bool Atom::move_rel(SCoord* v)
{
	#if debug_break_on_move
	if (break_on_move) throw 0xb16fa7012a96eca7;
	#endif

	/*if (name && !strcmp(name, "CB"))
	{
		Bond* b = get_bond_between("CA");
		if (b && b->btom)
		{
			float r = b->btom->get_location().get_3d_distance(location.add(v));
			if (r > 1.55) throw 0x7e57196;
		}
	}*/
    location = location.add(v);
    geov = NULL;
    return true;
}

int Atom::move_assembly(Point* pt, Atom* excluding)
{
    if (!excluding) return 0;
    int bi = get_idx_bond_between(excluding);
    Bond* palin = excluding->get_bond_between(this);
    Atom** atoms = palin->get_moves_with_btom();
    if (!atoms) return 0;

    //cout << "Moving assembly starting with " << name << " excluding " << excluding->name << endl;

    int i, atomct=0;
    SCoord mov, alignv;

    mov = v_from_pt_sub(*pt, location);
    alignv = v_from_pt_sub(excluding->location, *pt);

    get_geometry_aligned_to_bonds();
    Point center, alignp(&alignv), changeme(&geov[bi]);

    Rotation rot = align_points_3d(&changeme, &alignp, &center);

    Point a0loc = location;
    a0loc = a0loc.add(&mov);
    a0loc = rotate3D(&a0loc, pt, &rot);
    
	#if debug_break_on_move
	if (break_on_move) throw 0xb16fa7012a96eca7;
	#endif
	
    location = a0loc;
    atomct++;
    //cout << "Motion includes " << name << endl;

    for (i=0; atoms[i]; i++)
    {
		#if debug_break_on_move
		if (atoms[i]->break_on_move) throw 0xb16fa7012a96eca7;
		#endif

        // atoms[i]->move_rel(mov);
        Point aloc = atoms[i]->location;
        aloc = aloc.add(&mov);
        aloc = rotate3D(&aloc, pt, &rot);
        atoms[i]->location = aloc;
        atoms[i]->geov = NULL;
        atomct++;
        //cout << "Motion includes " << atoms[i]->name << endl;
    }

    return atomct;
}

float Atom::get_charge()
{
    if (Z == 1 && !charge)
    {
        if (bonded_to && bonded_to[0].btom)
        {
            float bchg = bonded_to[0].btom->charge;
            if (bchg > 0) return bchg;
        }
    }
    return charge;
}

float Atom::hydrophilicity_rule()
{
	float total = 0;
	
	if (is_polar()) total += fabs(is_polar());
	else if (Z == 7 || Z == 8) total += 1;
	else if (Z == 15 || Z == 16) total += 0.5;
	else if (family == PNICTOGEN) total += 0.25;
	else if (family == CHALCOGEN) total += 0.25;
	else if (family == HALOGEN)
	{
		if (is_bonded_to("H") || is_bonded_to(CHALCOGEN)) total += 1;
	}
	
	total += fabs(get_charge());
	
	return total;
}

Bond::Bond()
{
    atom = btom = 0;
    moves_with_btom = 0;
    cardinality = 0;
    can_rotate = false;
}

Bond::Bond(Atom* a, Atom* b, int card)
{
    atom = a;
    btom = b;
    cardinality = card;
    can_rotate = (card == 1);
}

Bond::~Bond()
{
    if (moves_with_btom) delete[] moves_with_btom;
}

Bond** Atom::get_bonds()
{
    if (!bonded_to) return NULL;
    int i;
    if (!geometry || geometry<0 || isnan(geometry)) geometry=4;
    try
    {
        Bond** retval = new Bond*[geometry+1];
        // No init_nulls() because performance.
        for (i=0; i<geometry; i++) retval[i] = &bonded_to[i];
        retval[geometry] = 0;
        return retval;
    }
    catch (int e)
    {
        return NULL;
    }
}

void Atom::clear_all_moves_cache()
{
    if (!bonded_to) return;
    int i;
    for (i=0; i<geometry; i++) bonded_to[i].clear_moves_with_cache();
    geov = NULL;
}

int Atom::get_bonded_atoms_count()
{
    if (!bonded_to) return 0;
    int i, retval=0;
    for (i=0; i<geometry; i++) if (bonded_to[i].btom) retval++;
    return retval;
}
float Atom::is_bonded_to(Atom* lbtom)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom == lbtom)
                return bonded_to[i].cardinality;
    return 0;
}

bool Atom::shares_bonded_with(Atom* btom)
{
    if (!bonded_to) return false;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom->is_bonded_to(btom)) return true;
    return false;
}

Atom* Atom::is_bonded_to(const char* element)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (!strcmp(bonded_to[i].btom->get_elem_sym(), element)
               )
                return bonded_to[i].btom;
    return 0;
}

int Atom::num_bonded_to(const char* element)
{
	if (!bonded_to) return 0;
    int i, j=0;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (!strcmp(bonded_to[i].btom->get_elem_sym(), element)
               )
                j++;
    return j;
}

Bond* Atom::get_bond_between(Atom* btom)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom == btom)
                return &bonded_to[i];
    return 0;
}

Bond* Atom::get_bond_between(const char* bname)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom && !strcmp( bonded_to[i].btom->name, bname ))
                return &bonded_to[i];
    return 0;
}

int Atom::get_idx_bond_between(Atom* btom)
{
    if (!bonded_to) return -1;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom == btom)
                return i;
    return -1;
}

Atom* Atom::is_bonded_to(const char* element, const int lcardinality)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (!strcmp(bonded_to[i].btom->get_elem_sym(), element)
				&&
				bonded_to[i].cardinality == lcardinality
               )
                return bonded_to[i].btom;
    return 0;
}

Atom* Atom::is_bonded_to(const int family)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom->get_family() == family
               )
                return bonded_to[i].btom;
    return 0;
}

Atom* Atom::is_bonded_to(const int family, const int lcardinality)
{
    if (!bonded_to) return 0;
    int i;
    for (i=0; i<geometry; i++)
        if (bonded_to[i].btom)
            if (bonded_to[i].btom->get_family() == family
				&&
				bonded_to[i].cardinality == lcardinality
               )
                return bonded_to[i].btom;
    return 0;
}

bool Atom::bond_to(Atom* lbtom, float lcard)
{
	if (!lbtom) return false;
    int i;
    
    // if (this < lbtom && Z > 1 && lbtom->Z > 1) cout << "Bond " << name << cardinality_printable(lcard) << lbtom->name << endl;

    geov = NULL;

    // TODO: This will fail if creating a nitrate or a sulfite.
    if (lcard>=2 && !reciprocity)
    {
        geometry -= (lcard-1);
        lbtom->geometry -= (lcard-1);
        geov=0;
        lbtom->geov=0;
    }

    if (!reciprocity && !bonded_to[0].btom)
    {
        mirror_geo = (geometry==3 && lcard >= 1.5 && lcard<=2) ? 0 : -1;
        //if (mirror_geo >= 0) cout << name << " mirrors the geometry of " << lbtom->name << "(" << lbtom->geometry << ")" << endl;
    }

    for (i=0; i<geometry; i++)
    {
        if (!bonded_to[i].btom)
        {
            bonded_to[i].atom = this;
            bonded_to[i].btom = lbtom;
            bonded_to[i].cardinality = lcard;
            bonded_to[i].can_rotate = (lcard == 1
                                       && Z != 1
                                       && lbtom->Z != 1
                                      );		// Later when we look for rings we update this.

            // Hydrogen magic.
            if (lbtom->Z == 1)
            {
                switch (Z)
                {
                	case 7:
                    polarity = -0.9;
                    lbtom->polarity = 0.9;
                    lbtom->acidbase = 1;
                    acidbase = 1;
                    break;

                	case 15:
                    polarity = -0.5;
                    lbtom->polarity = 0.5;
                    lbtom->acidbase = 0.75;
                    acidbase = 0.75;
                    break;

		            case 8:
		            case 9:
		            case 17:
		            case 35:
		            case 53:
		            case 85:
		            case 117:
                    polarity = -1;
                    lbtom->polarity = 1;
                    break;

		            case 16:
		            case 34:
                    polarity = -0.25;
                    lbtom->polarity = 0.25;
                    thiol = -1;
                    lbtom->thiol = 1;
                    break;

                	default:
                    break;
                }
            }

            if (!reciprocity)
            {
                lbtom->reciprocity = true;
                lbtom->bond_to(this, lcard);
                lbtom->reciprocity = false;

                geo_rot_1.v.r = geo_rot_2.v.r = 0;
            }

            return true;
        }
    }

    return false;
}

float Atom::is_polar()
{
	if (charge) polarity = sgn(charge);
    return polarity;
}

float Atom::get_acidbase()
{
    if (fabs(acidbase) < 0.000001) acidbase = 0;
    return acidbase;
}

int Atom::is_thio()
{
    return thiol;
}

bool Atom::is_metal()
{
    switch (Z)
    {
    case 1:
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 34:
    case 35:
    case 36:
    case 53:
    case 54:
    case 85:
    case 86:
        return false;

    default:
        return true;
    }
}

bool Atom::is_pi()
{
    if (!bonded_to) return false;
    int i;
    for (i=0; i<valence; i++)
    {
        if (bonded_to[i].cardinality > 1 && bonded_to[i].cardinality <= 2) return true;
    }
    return false;
}

void Atom::set_aa_properties()
{
    if (!aaletter) return;
    if (!name) return;

    // https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5441424/
    if (false!=strcmp(name, "O"))
    {
        charge = -0.53;
        polarity = -1;
        return;
    }

    if (false!=strcmp(name, "N"))
    {
        charge = -0.40;
        acidbase = 0.3;
        polarity = -1;
        return;
    }

    if (false!=strcmp(name, "H")
            ||
            false!=strcmp(name, "NH")
       )
    {
        charge =  0.29;
        acidbase = 0.3;
        polarity =  1;
        return;
    }

    switch (aaletter)
    {
    case 'C':
        if (false!=strcmp(name, "SG"))
        {
            polarity = -0.25;
            thiol = -1;
            return;
        }
        if (false!=strcmp(name, "HG"))
        {
            polarity =  0.25;
            thiol =  1;
            return;
        }
        break;

    case 'D':
        break;

    default:
        ;
    }
}

void Atom::dump_array(Atom** aarr)
{
    int i;

    if (!aarr) return;
    for (i=0; aarr[i]; i++)
        cout << (aarr[i]->name ? aarr[i]->name : aarr[i]->get_elem_sym()) << " ";

    cout << endl;
}

Bond* Atom::get_bond_by_idx(int bidx)
{
    return &bonded_to[bidx];
}

void Atom::consolidate_bonds()
{
	
}

void Bond::fill_moves_with_cache()
{
    Atom* attmp[65536];
    int tmplen = 0;
    int i, j, k;

    if (_DBGMOVES) cout << "What moves with " << btom->name << " when rotating about " << atom->name << "?" << endl;

    btom->used = true;
    Bond** b = btom->get_bonds();
    if (!b) return;
    for (i=0; b[i]; i++)
    {
        if (b[i]->btom && b[i]->btom != atom)
        {
            attmp[tmplen++] = b[i]->btom;
            b[i]->btom->used = true;
            if (_DBGMOVES) cout << b[i]->btom->name << " ";
        }
    }
    delete[] b;

    do
    {
        k=0;
        for (j=0; j<tmplen; j++)
        {
            b = attmp[j]->get_bonds();
            if (b)
            {
                for (i=0; b[i]; i++)
                {
                    //if (b[i]->btom) cout << "(" << b[i]->btom->name << "?) ";
                    if (b[i]->btom && !b[i]->btom->used && b[i]->btom != atom)
                    {
                        attmp[tmplen++] = b[i]->btom;
                        b[i]->btom->used = true;
                        if (_DBGMOVES) cout << b[i]->btom->name << " ";
                        k++;
                    }
                }
                delete[] b;
            }
        }
    }
    while (k);

    moves_with_btom = new Atom*[tmplen+4];
    init_nulls(moves_with_btom, tmplen+4);

    for (i=0; i<tmplen; i++)
    {
        moves_with_btom[i] = attmp[i];
        attmp[i]->used = false;
    }
    moves_with_btom[i] = 0;
    btom->used = false;

    if (_DBGMOVES) cout << endl << endl;
    
    enforce_moves_with_uniqueness();
}

void Bond::enforce_moves_with_uniqueness()
{
	/*if (!strcmp(atom->name, "CD1") && !strcmp(btom->name, "NE1"))
	{
		Ring** rr = atom->get_rings();
		
		cout << atom->name << " is a member of:" << endl;
		if (rr)
		{
			int l;
			
			for (l=0; rr[l]; l++)
			{
				cout << *rr[l] << endl;
			}
			
			delete[] rr;
		}
		else cout << "(no rings.)" << endl;
		
		rr = btom->get_rings();
		
		cout << btom->name << " is a member of:" << endl;
		if (rr)
		{
			int l;
			
			for (l=0; rr[l]; l++)
			{
				cout << *rr[l] << endl;
			}
			
			delete[] rr;
		}
		else cout << "(no rings.)" << endl;
		
		cout << endl;
	}*/
	
	// Ring bond rotation is not supported currently.
	if (atom && btom && atom->in_same_ring_as(btom))
	{
		can_rotate = false;
		if (moves_with_btom) delete[] moves_with_btom;
		moves_with_btom = nullptr;
		// cout << atom->name << " is in the same ring as " << btom->name << "; no rotations allowed." << endl;
		return;
	}
	
	if (!moves_with_btom) return;
	
	int i, j, k;
	
	for (i=0; moves_with_btom[i]; i++)
	{
		for (j=i+1; moves_with_btom[j]; j++)
		{
			if (moves_with_btom[j] == moves_with_btom[i]
				||
				moves_with_btom[j]->is_backbone
				||
				moves_with_btom[j]->residue != btom->residue
			   )
			{
				moves_with_btom[j] = 0;
				break;
				
				for (k=j+1; moves_with_btom[k]; k++)
				{
					moves_with_btom[k-1] = moves_with_btom[k];
				}
				moves_with_btom[k-1] = 0;
			}
		}
	}
}

bool Bond::rotate(float theta, bool allow_backbone)
{
    if (!moves_with_btom) fill_moves_with_cache();
    enforce_moves_with_uniqueness();
    if (!moves_with_btom) return false;
    if (!can_rotate)
    {
        if (can_flip) theta = flip_angle;
        else return false;
    }
    // cout << "Rotate " << atom->name << cardinality_printable(cardinality) << btom->name << endl;
    
    if (atom->residue && greek_from_aname(atom->name) > greek_from_aname(btom->name))
    {
    	can_rotate = false;
    	return false;
    }
    
    int i;
    Point cen = btom->get_location();
    Point bas = atom->get_location();
    Point dir = cen.subtract(&bas);
    SCoord v(&dir);

    Rotation rot;
    rot.v = v;
    rot.a = theta;
    btom->rotate_geometry(rot);
    
    #if allow_tethered_rotations
    // Whichever side has the lower sum of Atom::last_bind_energy values, rotate that side.
    float mwb_total_binding=0, mwbi_total_binding=0;
    Bond* inverse = btom->get_bond_between(atom);
    
    if (!atom->residue && !btom->residue && inverse && inverse->can_rotate && !(inverse->moves_with_btom)) inverse->fill_moves_with_cache();
    if (!atom->residue && !btom->residue && inverse && inverse->can_rotate && inverse->moves_with_btom)
    {
    	for (i=0; moves_with_btom[i]; i++)
    	{
    		if (moves_with_btom[i]->is_backbone) mwbi_total_binding += 1e9; // goto _cannot_reverse_bondrot;
    		mwb_total_binding += moves_with_btom[i]->last_bind_energy;
    	}
    	for (i=0; inverse->moves_with_btom[i]; i++)
    	{
    		if (inverse->moves_with_btom[i]->is_backbone) goto _cannot_reverse_bondrot;
    		mwbi_total_binding += inverse->moves_with_btom[i]->last_bind_energy;
    	}
    	
    	if (mwb_total_binding < mwbi_total_binding)
    		return inverse->rotate(-theta, allow_backbone);				// DANGER! RECURSION.
    }
    _cannot_reverse_bondrot:
    ;
	#endif

    // cout << "Rotating " << atom->name << "-" << btom->name << "... ";
    for (i=0; moves_with_btom[i]; i++)
    {
        if (!allow_backbone)
        {
            if (moves_with_btom[i]->is_backbone)
            {
                /*cout << "DANGER: Rotation of " << atom->residue << ":" << atom->name << " - " << btom->name << endl;
                if (can_flip) flip_angle = -flip_angle;
                throw 0xbadb09d;*/
                continue;
            }
        }
        if (moves_with_btom[i]->residue != btom->residue) continue;
        // cout << moves_with_btom[i]->name << " ";
        
        /*cout << moves_with_btom[i]->residue << ":" << moves_with_btom[i]->name
        	 << " from "
        	 << atom->residue << ":" << atom->name
        	 << "-"
        	 << btom->residue << ":" << btom->name
        	 << endl << flush;*/

        Point loc = moves_with_btom[i]->get_location();
        Point nl  = rotate3D(&loc, &cen, &v, theta);
        
        /*if (moves_with_btom[i]->residue == 203 && !strcmp(moves_with_btom[i]->name, "HB1"))
        	cout << "Moving " << moves_with_btom[i]->residue << ":" << moves_with_btom[i]->name << " about axis "
        		 << atom->residue << ":" << atom->name << " - " << btom->residue << ":" << btom->name
        		 << endl;*/
        
        // cout << moves_with_btom[i]->name << loc << " " << (theta*fiftyseven) << " " << nl << endl;
        moves_with_btom[i]->move(&nl);
        moves_with_btom[i]->rotate_geometry(rot);
    }
    // cout << endl;

    if (can_flip) flip_angle = -flip_angle;
    total_rotations += theta;
    
    // cout << theta << endl;

    return true;
}

void Atom::rotate_geometry(Rotation rot)
{
    if (!geov) return;				// If no cached geometry, it will be calculated the next time it's required.

    Point center;

    int i;
    for (i=0; i<geometry; i++)
    {
        Point pt(&geov[i]);
        pt = rotate3D(&pt, &center, &rot);
        SCoord v(&pt);
        geov[i] = v;
    }
}

void Bond::swing(SCoord newdir)
{
    if (!moves_with_btom) fill_moves_with_cache();
    if (!moves_with_btom) return;

    int i;
    Point cen = atom->get_location();
    Point tgt(&newdir);
    tgt = tgt.add(cen);
    Point loc = btom->get_location();

    Rotation rot = align_points_3d(&loc, &tgt, &cen);
    Point nl = rotate3D(&loc, &cen, &rot);
    btom->move(&nl);

    for (i=0; moves_with_btom[i]; i++)
    {
        Point ol = moves_with_btom[i]->get_location();
        nl = rotate3D(&ol, &cen, &rot);
        moves_with_btom[i]->move(&nl);
    }
}


SCoord* Atom::get_basic_geometry()
{
    SCoord* retval = new SCoord[geometry+2];

    int i, j;
    float x, y, z;

    if (geometry == 1)
    {
        SCoord v1(1, M_PI/2, 0);
        retval[0] = v1;
    }

    if (geometry == 2)
    {
        SCoord v1(1, M_PI*0.5, 0);
        SCoord v2(1, M_PI*1.5, 0);
        retval[0] = v1;
        retval[1] = v2;
    }

    if (geometry == 3)
    {
        for (i=0; i<geometry; i++)
        {
            SCoord v(1, 0, M_PI/1.5*i);
            retval[i] = v;
        }
    }

    if (geometry == 4)
    {
        SCoord v1(1, M_PI/2, 0);
        retval[0] = v1;

        for (i=1; i<geometry; i++)
        {
            SCoord v(1, M_PI/2-tetrahedral, M_PI/1.5*(i-1));
            retval[i] = v;
        }
    }

    if (geometry == 5)
    {
        SCoord v1(1, M_PI*0.5, 0);
        SCoord v2(1, M_PI*1.5, 0);
        retval[0] = v1;

        for (i=1; i<geometry-1; i++)
        {
            SCoord v(1, 0, M_PI/1.5*(i-1));
            retval[i] = v;
        }

        retval[geometry-1] = v2;
    }

    if (geometry == 6)
    {
        SCoord v1(1, M_PI*0.5, 0);
        SCoord v2(1, M_PI*1.5, 0);
        retval[0] = v1;

        for (i=1; i<geometry-1; i++)
        {
            SCoord v(1, 0, M_PI/2*(i-1));
            retval[i] = v;
        }

        retval[geometry-1] = v2;
    }

    if (geometry == 7)
    {
        for (i=0; i<3; i++)
        {
            SCoord v(1, M_PI/4, M_PI/1.5*i);
            retval[i] = v;
        }
        for (i=3; i<geometry; i++)
        {
            SCoord v(1, M_PI/2+M_PI/4, M_PI/2*(i-3));
            retval[i] = v;
        }
    }

    if (geometry == 8)
    {
        for (i=0; i<4; i++)
        {
            SCoord v(1, M_PI/4, M_PI/2*i);
            retval[i] = v;
        }
        for (i=4; i<geometry; i++)
        {
            SCoord v(1, M_PI/2+M_PI/4, M_PI/2*(i-4));
            retval[i] = v;
        }
    }

    if (geometry == 10)
    {
        for (i=0; i<5; i++)
        {
            SCoord v(1, M_PI/4, M_PI/2.5*i);
            retval[i] = v;
        }
        for (i=5; i<geometry; i++)
        {
            SCoord v(1, M_PI/2+M_PI/4, M_PI/2.5*(i-5));
            retval[i] = v;
        }
    }


    return retval;
}

void Atom::swing_all(int startat)
{
    SCoord* v = get_geometry_aligned_to_bonds();
    int i;

    for (i=startat; i<geometry; i++)
    {
        if (bonded_to[i].btom) bonded_to[i].swing(v[i]);
    }
}

float Atom::get_bond_angle_anomaly(SCoord v, Atom* ignore)
{
	if (!bonded_to) return 0;
	int i;
	float anomaly = 0;
	float lga = get_geometric_bond_angle();
	
	//cout << " -=- " << lga*fiftyseven << " | ";
	for (i=0; i<geometry; i++)
	{
		if (bonded_to[i].btom && bonded_to[i].btom != ignore)
		{
			//cout << bonded_to[i].btom->location << " - " << location;
			SCoord vb = bonded_to[i].btom->location.subtract(location);
			//cout << " = " << (Point)vb << endl;
			float theta = find_3d_angle(v, vb, Point(0,0,0));
			anomaly += fabs(theta-lga);
			/*cout << "Geometric anomaly for " << name << ": "
				 << lga*fiftyseven << "deg - " << theta*fiftyseven << "deg difference of " << fabs(theta-lga)*fiftyseven << "deg."
				 << endl;*/
			//cout << " " << theta*fiftyseven;
		}
	}
	//cout  << " -=- ";
	
	//cout << "Geometric bond angle for " << name << " is " << lga*fiftyseven << "deg." << endl;
	return anomaly;
}

float Atom::get_geometric_bond_angle()
{
	int lgeo = origgeo;
	
	//cout << name << " " << origgeo << " ";
	
	int i, bonded_atoms = 0;
	for (i=0; i<lgeo; i++) if (bonded_to[i].btom) bonded_atoms++;
	
	for (i=0; i<lgeo; i++)
	{
		float bcard = bonded_to[i].cardinality;
		if (bonded_to[i].btom && bcard > 1)
		{
			lgeo -= ((i&1) ? floor(bcard-1) : ceil(bcard-1));		// lgeo is an integer so treat two 1.5 bonds the same as a 1 and a 2.
			//cout << bonded_to[i].btom->name << "-" << bcard << " ";
		}
	}
	if (lgeo < bonded_atoms) lgeo = bonded_atoms;
	
	//cout << lgeo << " ";
	
	switch (lgeo)
	{
		case 1:
		case 2:
		return M_PI;
		
		case 3:		return triangular;
		case 4:		return tetrahedral;
		case 5:		return (triangular*3 + square*2)/5;		// Irregular so just return an average.
		case 6:		return square;
		
		case 7:
		case 8:
		return tetrahedral/2;
		
		default:
		return tetrahedral;
	}
}

SCoord* Atom::get_geometry_aligned_to_bonds()
{
    int bc = get_bonded_atoms_count();
    if (origgeo>4 && bc && bc <= 4)
    {
        geometry=4;
        if (is_pi()) geometry--;
        else
        {
            int i;
            for (i=0; i<bc; i++) geometry -= fmax(0,bonded_to[i].cardinality-1);
            geov=NULL;
        }
    }

    if (geov)
    {
        if (_DBGGEO) cout << name << " returns cached geometry." << endl;
        return geov;
    }
    geov = get_basic_geometry();

    Point center;
    int i, j;

    if (num_conj_rings())
    {
    	Point arom_center;
    	
    	if (get_count_pi_bonds())
    	{
		    geometry = 3;
        }
        
        if (bonded_to[1].btom)
        {
        	for (j=0; member_of[j]; j++)
			{
				if (bonded_to[0].btom->is_in_ring(member_of[j]) && bonded_to[1].btom->is_in_ring(member_of[j]))
					arom_center = member_of[j]->get_center();
			}
        	
            geov[0] = v_from_pt_sub(bonded_to[0].btom->get_location(), location);
            geov[1] = v_from_pt_sub(bonded_to[1].btom->get_location(), location);
            geov[2] = v_from_pt_sub(location, arom_center);
        }
        else
        {
        	for (j=0; member_of[j]; j++)
			{
				if (bonded_to[0].btom->is_in_ring(member_of[j]))
					arom_center = member_of[j]->get_center();
			}
        	
            geov = get_basic_geometry();
            Point bond0v = bonded_to[0].btom->get_location().subtract(&location);
            Point vanticen = location.subtract(arom_center);
            bond0v.scale(1);
            vanticen.scale(1);
            Point g0(&geov[0]);
            g0.scale(1);
            Point g1(&geov[2]);
            g1.scale(1);

            Rotation* rots = align_2points_3d(&g0, &bond0v, &g1, &vanticen, &center);

            geo_rot_1 = rots[0];
            geo_rot_2 = rots[1];

            int k;
            for (k=0; k<2; k++)
            {
                for (i=0; i<geometry; i++)
                {
                    Point pt1(&geov[i]);
                    Point pt2 = rotate3D(&pt1, &center, &rots[k]);
                    SCoord v2(&pt2);
                    v2.r = 1;
                    geov[i] = v2;
                }
            }
        }
        if (_DBGGEO) cout << name << " returns aromatic geometry." << endl;
        return geov;
    }

    if (!bonded_to[0].btom)
    {
        if (_DBGGEO) cout << name << " returns default geometry." << endl;
        return geov;
    }

    if (mirror_geo >= 0)
    {
        Bond* b = &bonded_to[mirror_geo];
        if (_DBGGEO) cout << name << " location: " << location.printable() << endl;
        if (_DBGGEO) cout << b->btom->name << " location: " << b->btom->location.printable() << endl;
        geov[0] = v_from_pt_sub(b->btom->location, location);
        geov[0].r = 1;
        Point _4avg[2];
        _4avg[0] = b->btom->location;
        _4avg[1] = location;
        Point avg = average_of_points(_4avg, 2);
        if (_DBGGEO) cout << "avg: " << avg.printable() << endl;

        j=1;
        b->btom->mirror_geo = -1;		// Prevent infinite loop.
        SCoord* bgeov = b->btom->get_geometry_aligned_to_bonds();		// RECURSION!

        for (i=0; i<b->btom->geometry; i++)
        {
            if (!b->btom->bonded_to[i].btom || b->btom->bonded_to[i].btom != this)
            {
                Point bgp(&bgeov[i]);
                bgp = bgp.add(b->btom->location);
                if (_DBGGEO) cout << "bgp: " << bgp.printable() << " from SCoord ??=" << bgeov[i].phi << " ??=" << bgeov[i].theta << " r=" << bgeov[i].r << endl;
                Point mirr = avg.subtract(bgp.subtract(&avg));
                if (_DBGGEO) cout << "mirr: " << mirr.printable() << endl;
                if (flip_mirror) mirr = rotate3D(&mirr, &avg, &geov[mirror_geo], M_PI);
                geov[j++] = v_from_pt_sub(mirr, location);
            }
        }

        if (_DBGGEO) cout << name << " returns mirrored geometry." << endl;

        return geov;
    }

    bool doswings = false;

    if (bonded_to[0].btom)
    {
        if (geometry > 2)
        {
        	j = 0;
        	if (bonded_to[1].btom) j = 1;
        	else if (bonded_to[2].btom) j = 2;
        	int l;
        	
            if (j)
            {
                Point g0(&geov[0]);
                g0.scale(1);
                Point g1(&geov[j]);
                g1.scale(1);
                Point a0 = bonded_to[0].btom->location.subtract(location);
                a0.scale(1);
                Point a1 = bonded_to[j].btom->location.subtract(location);
                a1.scale(1);
                Rotation* rots = align_2points_3d(&g0, &a0, &g1, &a1, &center);

                geo_rot_1 = rots[0];
                geo_rot_2 = rots[1];

                int k;
                for (k=0; k<2; k++)
                {
                    for (i=0; i<geometry; i++)
                    {
                        Point pt1(&geov[i]);
                        Point pt2 = rotate3D(&pt1, &center, &rots[k]);
                        SCoord v2(&pt2);
                        v2.r = 1;
                        geov[i] = v2;
                    }
                }
                
                g0 = geov[0];
                g1 = geov[1];
                g0.scale(1);
                g1.scale(1);
                rots = align_2points_3d(&g1, &a1, &g0, &a0, &center);
                rots[0].a /= 2;
                rots[1].a /= 2;
                
                for (k=0; k<2; k++)
                {
                    for (i=0; i<geometry; i++)
                    {
                        Point pt1(&geov[i]);
                        Point pt2 = rotate3D(&pt1, &center, &rots[k]);
                        SCoord v2(&pt2);
                        v2.r = 1;
                        geov[i] = v2;
                    }
                }
                

                if (_DBGGEO) cout << name << " returns " << (j==1?"trans":"cis") << " double-aligned geometry (" << geometry << "):"
                                      << bonded_to[0].btom->name << ", " << bonded_to[1].btom->name << "."
                                      << endl;
                return geov;
            }

        }


        // Get alignment for the first bond.
        Point pt(&geov[0]);
        Point aln = bonded_to[0].btom->get_location().subtract(&location);
        aln.scale(1);
        Rotation rot = align_points_3d(&pt, &aln, &center);
        SCoord av(&aln);
        geo_rot_1 = rot;
        if (_DBGGEO) cout << name << " bond 0 geometry " << pt.printable() << " align to " << aln.printable() << " for rotation: " << (rot.a * 180/M_PI) << " degrees." << endl;

        float angle;

        // Rotate all geometry according to the first bond alignment.
        for (i=0; i<geometry; i++)
        {
            Point pt1(&geov[i]);
            Point pt2 = rotate3D(&pt1, &center, &rot);
            SCoord v2(&pt2);
            geov[i] = v2;
        }

        if (_DBGGEO) cout << name << " returns single-aligned geometry (" << geometry << ")." << endl;
    }

    // Life is a cruel and dismal place, and sometimes you're hit with such an awful turn of events that you really don't know how you can go on.
    return geov;
}

SCoord Atom::get_next_free_geometry(float lcard)
{
    int lgeo = geometry;
    if (lcard > 1)
    {
        geometry -= ceil(lcard-1);
        geov = 0;
    }
    SCoord* v = get_geometry_aligned_to_bonds();
    SCoord retval;
    if (!bonded_to) retval = v[0];
    else
    {
        int i;
        for (i=0; bonded_to[i].btom; i++);	// Get count.

        if (i >= geometry) i=0;

		int j=i;
        if (geometry == 4 && swapped_chirality && i >= 2) i ^= 1;
        if (geometry == 3 && EZ_flip && i >= 1) i = 3-i;
        // if (bonded_to[i].btom) i=j;			// For some reason, this line makes everything go very wrong.
        if (geometry == 4 && chirality_unspecified)
        {
        	if (!strcmp(name, "Tumbolia")) cout << endl;
    		for (i=0; i<geometry; i++)
    		{
		    	Point pt = v[i];
				pt.scale(1);
				
    			float closest = 81;
		    	if (!strcmp(name, "Tumbolia")) cout << i << "*: " << (Point)v[i] << endl;
		    	for (j=0; j<geometry; j++)
		    	{
		    		if (!bonded_to[j].btom) continue;
		    		Point pt1 = bonded_to[j].btom->location.subtract(location);
		    		pt1.scale(1);
		    		float r = pt1.get_3d_distance(pt);
		    		if (!strcmp(name, "Tumbolia")) cout << j << ":: " << pt1 << " " << r << endl;
		    		if (r < closest) closest = r;
		    	}
		    	if (!strcmp(name, "Tumbolia")) cout << endl;
		    	if (closest > 0.7) goto _successful;				// Slightly less than 1/sqrt(2).
        	}
        	return retval;
    	}

		_successful:
        retval = v[i];
        
    }
    geometry = lgeo;
    return retval;
}

int Atom::get_idx_next_free_geometry()
{
    if (!bonded_to) return 0;
    else
    {
        int i;
        for (i=0; i < geometry && bonded_to[i].btom; i++);	// Get count.
        if (i >= geometry) i=0;
        if (geometry == 4 && swapped_chirality && i >= 2) i ^= 1;
        if (geometry == 3 && EZ_flip && i >= 1) i = 3-i;
        return i;
    }
}

int Atom::get_count_pi_bonds()
{
	if (!bonded_to) return 0;
	int i, retval=0;
	for (i=0; i<geometry; i++)
	{
		if (bonded_to[i].btom && bonded_to[i].cardinality > 1 && bonded_to[i].cardinality <= 2.1) retval++;
	}
	return retval;
}

void Atom::save_pdb_line(FILE* pf, unsigned int atomno)
{
    /*
    ATOM   2039  CA  ALA   128      -6.065 -24.834  -5.744  1.00001.00           C
    */
    fprintf(pf, residue ? "ATOM   " : "HETATM ");
    if (atomno<1000) fprintf(pf," ");
    if (atomno< 100) fprintf(pf," ");
    if (atomno<  10) fprintf(pf," ");
    fprintf(pf, "%d ", atomno);

    if (strlen(name) < 4)
        fprintf(pf, " ");

    fprintf(pf, "%s ", name);

    if (strlen(name) < 3) fprintf(pf, " ");
    if (strlen(name) < 2) fprintf(pf, " ");

    fprintf(pf, "%s   ", aa3let);

    if (residue < 100) fprintf(pf, " ");
    if (residue <  10) fprintf(pf, " ");
    fprintf(pf, "%d    ", residue);

    if (!location.x) location.x = 0;
    if (location.x>=0) fprintf(pf," ");
    if (fabs(location.x) < 100) fprintf(pf," ");
    if (fabs(location.x) <  10) fprintf(pf," ");
    fprintf(pf, "%4.3f", location.x);

    if (!location.y) location.y = 0;
    if (location.y>=0) fprintf(pf," ");
    if (fabs(location.y) < 100) fprintf(pf," ");
    if (fabs(location.y) <  10) fprintf(pf," ");
    fprintf(pf, "%4.3f", location.y);

    if (!location.z) location.z = 0;
    if (location.z>=0) fprintf(pf," ");
    if (fabs(location.z) < 100) fprintf(pf," ");
    if (fabs(location.z) <  10) fprintf(pf," ");
    fprintf(pf, "%4.3f", location.z);

    fprintf(pf, "  1.00001.00           %s\n", get_elem_sym());
}

void Atom::stream_pdb_line(ostream& os, unsigned int atomno)
{
    os << "ATOM   ";
    os << setw(4) << atomno << " ";

    if (strlen(name) < 4)
        os << " ";

    os << name << " ";

    if (strlen(name) < 3) os << " ";
    if (strlen(name) < 2) os << " ";

    os << aa3let << "  ";

    os << setw(4) << residue << "    ";

    if (!location.x) location.x = 0;
    os << fixed << setprecision(3) << setw(8) << setfill(' ') << location.x;

    if (!location.y) location.y = 0;
    os << fixed << setprecision(3) << setw(8) << setfill(' ') << location.y;

    if (!location.z) location.z = 0;
    os << fixed << setprecision(3) << setw(8) << setfill(' ') << location.z;

    os << "  1.00001.00           " << get_elem_sym() << endl;
}

int Bond::count_moves_with_btom()
{
    if (!moves_with_btom) return 0;
    int i;
    for (i=0; moves_with_btom[i]; i++);	// Get count.
    return i;
}

int Atom::num_rings()
{
	if (!member_of) return 0;
	int i;
	for (i=0; member_of[i]; i++);	// Get count.
	return i;
}

int Atom::num_conj_rings()
{
	if (!member_of) return 0;
	int i, j=0;
	for (i=0; member_of[i]; i++)
		if (member_of[i]->is_conjugated()) j++;
	return i;
}

Ring** Atom::get_rings()
{
	if (!member_of) return nullptr;
	int i;
	for (i=0; member_of[i]; i++);	// Get count.
	Ring** retval = new Ring*[i+4];
	
	for (i=0; member_of[i]; i++) retval[i] = member_of[i];
	retval[i] = nullptr;
	
	return retval;
}

bool Atom::in_same_ring_as(Atom* b)
{
	if (!member_of) return false;
	
	int i;
	for (i=0; member_of[i]; i++)
		if (b->is_in_ring(member_of[i])) return true;
	
	return false;
}

bool Atom::is_in_ring(Ring* ring)
{
	if (!ring) return false;
	
	// if (!strcmp(name, "NE1")) cout << "Checking if " << aaletter << residue << ":" << name << " is part of ring " << *ring << endl;
	
	// Since the objects can't be trusted to keep the damn data that have been set...
	Atom** ra = ring->get_atoms();
	int i;
	for (i=0; ra[i]; i++)
	{
		if (ra[i] == this)
		{
			// if (!strcmp(name, "NE1")) cout << "Yep!" << endl;
			int j, n=0;
			bool already = false;
			if (member_of) for (n=0; member_of[n]; n++) if (member_of[n] == ring) already = true;		// Determine length.
			if (!already)
			{
				Ring** array = new Ring*[4+n];
				for (j=0; j<n+4; j++) array[j] = nullptr;
				if (member_of)
					for (j=0; j<n; j++)
						array[j] = member_of[j];
				array[n++] = ring;
				array[n] = nullptr;
				if (member_of) delete[] member_of;
				member_of = array;
			}
		}
	}
	
	/*if (!strcmp(name, "NE1"))
	{
		cout << "----" << endl;
		for (i=0; member_of[i]; i++) cout << *member_of[i] << endl;
		cout << "----" << endl << endl;
	}*/
	
	if (!member_of) return false;
	for (i=0; member_of[i]; i++)
		if (member_of[i] == ring) return true;
	
	return false;
}

Ring* Atom::closest_arom_ring_to(Point target)
{
	int j, k;
	if (member_of)
	{
		k = -1;
		float brr = 99999;
		for (j=0; member_of[j]; j++)
		{
			Point rloc = member_of[j]->get_center();
			float rr = rloc.get_3d_distance(target);
			if (rr < brr)
			{
				k = j;
				brr = rr;
			}
		}
		
		if (k >= 0)
		{
			return member_of[k];
		}
	}
	
	return nullptr;
}

void Ring::fill_with_atoms(Atom** from_atoms)
{
	if (!from_atoms) return;
	
	int i;
	for (i=0; from_atoms[i]; i++);	// Get count.
	atcount = i;
	
	atoms = new Atom*[atcount+2];
	for (i=0; i < atcount; i++)
	{
		atoms[i] = from_atoms[i];
		if (!atoms[i]->member_of)
		{
			atoms[i]->member_of = new Ring*[4];
			atoms[i]->member_of[0] = this;
			atoms[i]->member_of[1] = atoms[i]->member_of[2] = atoms[i]->member_of[3] = nullptr;
			/*if (atoms[i]->aaletter == 'W') cout << "Ring::Ring(Atom**): "
				<< atoms[i]->aa3let << atoms[i]->residue << ":" << atoms[i]->name
				<< " is a member of a ring. " << hex << this << dec << endl;*/
		}
		else
		{
			int j, n;
			for (n=0; atoms[i]->member_of[n]; n++);		// Determine length.
			Ring** array = new Ring*[4+n];
			for (j=0; j<n+4; j++) array[j] = nullptr;
			for (j=0; j<n; j++)
				array[n] = atoms[i]->member_of[n];
			array[n++] = this;
			array[n] = nullptr;
			delete[] atoms[i]->member_of;
			atoms[i]->member_of = array;
			/*if (atoms[i]->aaletter == 'W') cout << "Ring::Ring(Atom**): "
				<< atoms[i]->aaletter << atoms[i]->residue << ":" << atoms[i]->name
				<< " is a member of a ring. " << hex << this << dec << endl;*/
		}
	}
	
	atoms[atcount] = nullptr;
}

Ring::Ring(Atom** from_atoms)
{
	fill_with_atoms(from_atoms);
	determine_type();
}

Ring::Ring(Atom** from_atoms, RING_TYPE ltype)
{
	fill_with_atoms(from_atoms);
	type = ltype;
}

Atom* Ring::get_atom(int index)
{
	if (index < 0 || index >= atcount) return nullptr;
	return atoms[index];
}

Atom** Ring::get_atoms() const
{
	if (!atcount) return nullptr;
	Atom** retval = new Atom*[atcount+2];
	int i;
	
	for (i=0; i<atcount; i++)
	{
		retval[i] = atoms[i];
	}
	retval[atcount] = nullptr;
	
	return retval;
}

RING_TYPE Ring::get_type()
{
	if (type == UNKNOWN) determine_type();
	return type;
}

Point Ring::get_center()
{
	Point _4avg[atcount+2];
	int i;
	for (i=0; i < atcount; i++)
	{
		_4avg[i] = atoms[i]->location;
	}
	
	return average_of_points(_4avg, atcount);
}

SCoord Ring::get_normal()
{
	int i, j, k;
	float w=0, x=0, y=0, z=0;
	
	for (i=0; i < atcount; i++)
	{
		for (j=i+1; j < atcount; j++)
		{
			for (k=j+1; k < atcount; k++)
			{
				// Note if the atoms are not in sequence, this approach will fail.
				Point pt = compute_normal(atoms[i]->location, atoms[j]->location, atoms[k]->location);
				w += pt.weight ? pt.weight : 1;
				x += pt.x;
				y += pt.y;
				z += pt.z;
			}
		}
	}
	
	Point pt1(x/w, y/w, z/w);
	return (SCoord)pt1;
}

bool Ring::is_conjugated()
{
	if (!atoms) return false;
	return atoms_are_conjugated(atoms);
}

bool Ring::is_coplanar()
{
	if (type != UNKNOWN)
	{
		return (type == AROMATIC || type == ANTIAROMATIC || type == COPLANAR);
	}
	else
	{
		if (!atoms) return false;
		if (!atcount) return false;
		if (atcount < 4) return true;

		int i;
		float anomaly;
		for (i=3; i<atcount; i++)
		{
		    anomaly = are_points_planar(atoms[0]->get_location(),
		                                atoms[1]->get_location(),
		                                atoms[2]->get_location(),
		                                atoms[i]->get_location()
		                               );
		}

		return (anomaly < 0.1);
    }
}

LocatedVector Ring::get_center_and_normal()
{
	LocatedVector retval;
	retval.copy(get_normal());
	retval.origin = get_center();
	return retval;
}

bool Ring::Huckel()
{
	if (!atcount) return false;
	int i;
	int pi_electrons = 0;
	int wiggle_room = 0;
	
	for (i=0; atoms[i]; i++)
	{
		if (atoms[i]->is_pi()) pi_electrons++;
		
		switch (atoms[i]->get_family())
		{
			case PNICTOGEN:
			case CHALCOGEN:
			wiggle_room += 2;
			break;
			
			default:
			;
		}
	}
	
	for (i=0; i<=wiggle_room; i+=2)
	{
		int n = pi_electrons + i;
		n -= 2;
		if (n && !(n % 4)) return true;
	}
	return false;
}

bool atoms_are_conjugated(Atom** atoms)
{
	int i;
	
	for (i=0; atoms[i]; i++)
	{
		if (i && !atoms[i]->is_bonded_to(atoms[i-1])) return false;
		switch (atoms[i]->get_family())
		{
			case TETREL:
			// cout << atoms[i]->name << "|" << atoms[i]->get_count_pi_bonds() << "|" << atoms[i]->get_charge() << endl;
			if (atoms[i]->get_count_pi_bonds() != 1
				&&
				!atoms[i]->get_charge()
				)
				return false;
			break;
			
			case TRIEL:
			case PNICTOGEN:
			case CHALCOGEN:
			break;
			
			default:
			return false;
		}
	}
	
	return true;
}

void Ring::determine_type()
{
	if (!atoms_are_conjugated(atoms))
	{
		type = OTHER;
		return;
	}
	
	if (!is_coplanar())
	{
		type = OTHER;
		return;
	}
	
	if (Huckel()) type = AROMATIC;
	else if (0) type = ANTIAROMATIC;		// TODO
	else type = COPLANAR;
}

std::ostream& operator<<(std::ostream& os, const Bond& b)
{
	os << b.atom->name;
	os << cardinality_printable(b.cardinality);
	os << b.btom->name;
	
	return os;
}

std::ostream& operator<<(std::ostream& os, const Ring& r)
{
	Atom** a = r.get_atoms();
	if (!a) os << "[empty]";
	else
	{
		os << "[";
		int i;
		for (i=0; a[i]; i++) os << a[i]->name << " ";
		os << "]";
	}
	
	return os;
}






















