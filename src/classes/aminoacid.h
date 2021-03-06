
#include <cstring>
#include <stdio.h>
#include "molecule.h"

#ifndef _AMINACID
#define _AMINACID

enum bb_rot_dir
{
 	N_asc,
 	CA_asc,
 	CA_desc,
 	C_desc
};

struct AABondDef
{
 	char aname[7];
 	char bname[7];
 	float cardinality=0;
 	float acharge=0;
 	bool can_rotate=false;
};

struct AADef
{
 	char _1let = '\0';
 	char _3let[4];
 	char name[40];
 	float reach = 2.5;
 	AABondDef** aabonds = nullptr;
 	Ring** aarings = nullptr;
 	bool proline_like = false;
 	std::string SMILES = "";
 	float hydrophilicity = 0;
 	bool aromatic = false;
 	bool can_coord_metal = false;
 	int charged = 0;
 	bool loaded = false;
 	bool isoleucine_fix = false;
};

struct MetalCoord
{
 	Atom* metal;
 	Atom** coord_atoms;
 	AminoAcid** coord_res;
 	bool locked = false;

 	Point coord_atom_avg_loc();
};

class AminoAcid : public Molecule
{
	public:
 	// Constructors.
 	AminoAcid(FILE* instream, AminoAcid* prev_res=0);
 	AminoAcid(const char letter, AminoAcid* prev_res=0);

 	// Getters and setters.
 	AminoAcid* get_prev() const { return prev_aa; }
 	AminoAcid* get_next() const { return next_aa; }
 	int get_residue_no() const { return residue_no; }
 	char get_letter() const { return aadef ? aadef->_1let : 0; }
 	char* get_3letter() const { return aadef ? aadef->_3let : 0; }
 	AADef* get_aa_definition() const { return aadef; }
 	bool is_tyrosine_like();		// An amino acid is tyrosine-like if it has an aromatic ring and a non-backbone H-bond acceptor not part of the ring.
 	float get_reach() const { return aadef ? aadef->reach : 2.5; }
 	bool can_reach(AminoAcid* other) const;
 	void set_region(const char* regname) { strcpy(region, regname); }
 	Atom* previous_residue_C();
 	Atom* next_residue_N();
 	Atom* HN_or_substitute();
 	Point get_CA_location();
 	Point HN_or_substitute_location();

 	// Serialization.
 	int from_pdb(FILE* instream);							// returns number of atoms loaded.
 	void save_pdb(FILE* outstream, int atomno_offset=0);

 	// Spatial functions.
 	void aamove(SCoord move_amt);
 	void recenter(Point new_location) { return; }
 	void rotate(LocatedVector SCoord, float theta);
 	LocatedVector rotate_backbone(bb_rot_dir direction, float angle);	// Return the origin and direction of the rotation axis.
 	LocRotation rotate_backbone_abs(bb_rot_dir direction, float angle);
 	LocRotation* flatten();		// Ensure the peptide bond is coplanar and the CA lies in the same plane. Return LocRotation[5].
 	
	Point* predict_previous_COCA();
	Point* predict_next_NHCA();
	void glom(Point* predicted, bool CO = false);		// Glom the AA by moving its NHCA or COCA to the result of a predict().
	

 	// Bond functions.
 	bool disulfide_bond(const AminoAcid* bond_to);
 	Bond** get_rotatable_bonds();
 	bool capable_of_inter(intera_type inter);
 	LocRotation enforce_peptide_bond(bool cis = false);				// Peptide bonds should almost always be in the trans (E) configuration.
 	

 	// Intermol functions.
 	float get_intermol_binding(AminoAcid** neighbors, bool backbone_atoms_only = false);
    float hydrophilicity();

 	// Misc.
 	void delete_sidechain();
 	static Molecule** aas_to_mols(AminoAcid** aas);
 	int similarity_to(const char letter);
    Ring* get_most_distal_arom_ring();
    std::string printable();
    
 	// Public properties.
 	int strand;
 	int atno_offset=0;
 	MetalCoord* m_mcoord=0;

	protected:
 	void load_aa_defs();
 	void copy_loaded_to_object(char letter, int tbdctr, AABondDef** tmpbdefs, bool proline_like);

 	int residue_no=0;
 	char region[25];
 	AADef* aadef=0;
 	AminoAcid *prev_aa=0, *next_aa=0;
 	float current_phi = M_PI, current_psi = M_PI, current_omega = M_PI;
};

extern AADef aa_defs[256];		// Indexed by ASCII value of one-letter code.
extern char* override_aminos_dat;

std::ostream& operator<<(std::ostream& os, const AminoAcid& aa);
std::ostream& operator<<(std::ostream& os, const AABondDef& b);


#endif

