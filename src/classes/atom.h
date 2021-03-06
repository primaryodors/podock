
#include <iostream>
#include <fstream>
#include <iomanip>
#include "point.h"

#ifndef _ATOM
#define _ATOM

class Atom;
class Bond
{
	public:
    Atom* atom = 0;
    Atom* btom = 0;
    float cardinality=0;			// aromatic bonds = 1.5.
    bool can_rotate=false;
    bool can_flip=false;
    float flip_angle=0;				// signed.
    float angular_momentum=0;
    float total_rotations=0;

    Bond();
    Bond(Atom* a, Atom* b, int card);
    ~Bond();

    bool rotate(float angle_radians, bool allow_backbone = false);
    void clear_moves_with_cache()	{	moves_with_btom = 0;	}
    Atom** get_moves_with_btom();
    int count_moves_with_btom();
    void swing(SCoord newdir);		// Rotate btom, and all its moves_with atoms, about atom so that the bond points to newdir.

	protected:
    void fill_moves_with_cache();
    void enforce_moves_with_uniqueness();
    Atom** moves_with_btom = 0;
};

enum RING_TYPE
{
	AROMATIC,
	ANTIAROMATIC,
	COPLANAR,
	OTHER,
	UNKNOWN
};

class Ring
{
	public:
	Ring() { ; }
	Ring(Atom** from_atoms);
	Ring(Atom** from_atoms, RING_TYPE type);
	
	int get_atom_count() { return atcount; }
	Atom* get_atom(int index);
	Atom** get_atoms() const;
	RING_TYPE get_type();
	Point get_center();
	SCoord get_normal();
	LocatedVector get_center_and_normal();
	bool is_coplanar();
	bool is_conjugated();
    bool Huckel();						// Compiler doesn't allow ü in an identifier - boo hiss!
	
	protected:
	Atom** atoms = nullptr;
	int atcount = 0;
	RING_TYPE type = UNKNOWN;
	
	void fill_with_atoms(Atom** from_atoms);
	void determine_type();
	void make_coplanar();
};

class Atom
{
	friend class Bond;
	friend class Ring;
	
	public:
    // Constructors and destructors.
    Atom(const char* elem_sym);
    Atom(const char* elem_sym, const Point* location);
    Atom(const char* elem_sym, const Point* location, const float charge);
    Atom(FILE* instream);
    ~Atom();

    // Basic getters.
    const char* get_elem_sym();
    int get_Z()    {        return Z;    }
    int get_family()    {        return family;    }
    int get_valence()    {        return valence;    }
    int get_geometry()    {        return geometry;    }
    Point get_location();
    float get_vdW_radius()    {        return vdW_rad;    }
    float get_atomic_weight()    {        return at_wt;    }
    float get_acidbase();
    float get_charge();
    float is_polar();						// -1 if atom is H-bond acceptor; +1 if donor.
    bool is_metal();
    int is_thio();							// -1 if atom is S; +1 if atom is H of a sulfhydryl.
    bool is_pi();

    // Setters.
    void set_aa_properties();
    void set_acidbase(float ab)    {        acidbase = ab;    }
    void clear_all_moves_cache();
    void increment_charge(float lcharge)    {        charge += lcharge;    }

    // Bond functions.
    Bond** get_bonds();
    int get_bonded_atoms_count();
    int get_count_pi_bonds();

    bool bond_to(Atom* btom, float cardinality);
    void unbond(Atom* btom);
    void unbond_all();
    void consolidate_bonds();

    float is_bonded_to(Atom* btom);			// If yes, return the cardinality.
    Atom* is_bonded_to(const char* element);
    Atom* is_bonded_to(const char* element, const int cardinality);
    Atom* is_bonded_to(const int family);
    Atom* is_bonded_to(const int family, const int cardinality);
    
    int num_bonded_to(const char* element);

    bool shares_bonded_with(Atom* btom);

    Bond* get_bond_between(Atom* btom);
    Bond* get_bond_between(const char* bname);
    Bond* get_bond_by_idx(int bidx);
    int get_idx_bond_between(Atom* btom);
    
    float hydrophilicity_rule();
    
    // Ring membership.
    int num_rings();
    int num_conj_rings();
	Ring** get_rings();
	bool is_in_ring(Ring* ring);
	Ring* closest_arom_ring_to(Point target);
	bool in_same_ring_as(Atom* b);
    void aromatize()
    {
        geometry=3;
        // if (valence>3) valence--;
        if (bonded_to)
        {
		    int i;
		    for (i=0; i<geometry; i++)
		    {
		    	if (bonded_to[i].cardinality > 1
		    		||
		    		(	bonded_to[i].cardinality == 1
		    			&& bonded_to[i].btom->get_Z() > 1
		    		)
		    		)
		    	{
		    		bonded_to[i].cardinality = 1.5;
		    	} 
	    	}
        }
        geov=0;
    }

    // Serialization
    void save_pdb_line(FILE* pf, unsigned int atomno);
    void stream_pdb_line(ostream& os, unsigned int atomno);

    // Spatial functions.
    bool move(Point* pt);
    bool move(Point pt) { return move(&pt); }
    bool move_rel(SCoord* v);
    int move_assembly(Point* pt, Atom* excluding);			// Return number of atoms moved. Note excluding must be a bonded atom.
    SCoord* get_basic_geometry();
    SCoord* get_geometry_aligned_to_bonds();
    float get_geometric_bond_angle();
    float get_bond_angle_anomaly(SCoord v, Atom* ignore);	// Assume v is centered on current atom.
    float distance_to(Atom* btom)
    {
        if (!btom) return -1;
        else return location.get_3d_distance(&btom->location);
    };
    SCoord get_next_free_geometry(float lcard);
    int get_idx_next_free_geometry();
    void rotate_geometry(Rotation rot);			// Necessary for bond rotation.
    void clear_geometry_cache() { geov=0; }
    void swing_all(int startat=0);
    void swap_chirality() { swapped_chirality = !swapped_chirality; chirality_unspecified = false; }

    // Static fuctions.
    static int Z_from_esym(const char* elem_sym);
    static char* esym_from_Z(const int lZ)
    {
        if (!lZ || lZ >= _ATOM_Z_LIMIT) return 0;
        else return elem_syms[lZ];
    }
    static void dump_array(Atom** aarr);

    // Public member vars.
    int residue=0;					// To be managed and used by the AminoAcid class.
    char aaletter;					// "
    char aa3let[4];					// "
    char* region;					// "
    bool is_backbone=false;			// "
    char* name;						// "
    bool used;						// Required for certain algorithms such as Molecule::identify_rings().
    int mirror_geo=-1;				// If >= 0, mirror the geometry of the btom of bonded_to[mirror_geo].
    bool flip_mirror=false;			// If true, do trans rather than cis bond conformation.
    bool dnh=false;					// Do Not Hydrogenate. Used for bracketed atoms in SMILES conversion.
    bool EZ_flip = false;
    float last_bind_energy=0;
    
    #if debug_break_on_move
    bool break_on_move = false;		// debugging feature.
    #endif

	protected:
    int Z=0;
    Point location;
    int valence=0;
    int geometry=0;						// number of vertices, so 4 = tetrahedral; 6 = octahedral; etc.
    int origgeo=0;
    SCoord* geov=0;
    float at_wt = 0;
    float vdW_rad = 0;
    float elecn = 0;
    float Eion = 0;
    float Eaffin = 0;
    float charge = 0;					// can be partial.
    float acidbase = 0;					// charge potential; negative = acid / positive = basic.
    float polarity = 0;					// maximum potential relative to -OH...H-.
    int thiol = 0;
    Bond* bonded_to = 0;
    bool reciprocity = false;
    int family=0;
    // InteratomicForce** Zforces;			// Non-covalent bond types where the atom's Z = either Za or Zb.
    Rotation geo_rot_1, geo_rot_2;
    bool swapped_chirality = false;
    bool chirality_unspecified = true;
	Ring** member_of = nullptr;

    static void read_elements();
    void figure_out_valence();

    static char* elem_syms[_ATOM_Z_LIMIT];
    static float vdW_radii[_ATOM_Z_LIMIT];
    static float electronegativities[_ATOM_Z_LIMIT];
    static float ioniz_energies[_ATOM_Z_LIMIT];
    static float elec_affinities[_ATOM_Z_LIMIT];
    static float atomic_weights[_ATOM_Z_LIMIT];
    static int valences[_ATOM_Z_LIMIT];
    static int geometries[_ATOM_Z_LIMIT];
};

bool atoms_are_conjugated(Atom** atoms);

static bool read_elem_syms = false;

std::ostream& operator<<(std::ostream& os, const Bond& b);
std::ostream& operator<<(std::ostream& os, const Ring& r);




#endif

