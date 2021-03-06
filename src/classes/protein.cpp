
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <string>
#include <math.h>
#include "protein.h"

#define _DBG_REACHLIG true

using namespace std;

Protein::Protein(const char* lname)
{
    name = lname;
    aaptrmin.n = aaptrmax.n = 0;
}

bool Protein::add_residue(const int resno, const char aaletter)
{
    int i;

    if (!residues)
    {
    	int arrlimit = resno+256;
        residues = new AminoAcid*[arrlimit];
        sequence = new char[arrlimit];
        ca = new Atom*[arrlimit];
        res_reach = new float[arrlimit];
        
        for (i=0; i<arrlimit; i++)
        {
        	residues[i] = NULL;
        	sequence[i] = 0;
        	ca[i] = NULL;
        	res_reach[i] = 0;
        }
    }
    else if (resno % 256)
    {
        AminoAcid** oldres = residues;
        char* oldseq = sequence;
        Atom** oldca = ca;
        float* oldreach = res_reach;
        int arrlimit = resno+261;
        residues = new AminoAcid*[arrlimit];
        sequence = new char[arrlimit];
        ca = new Atom*[arrlimit];
        res_reach = new float[arrlimit];
        
        for (i=0; i<arrlimit; i++)
        {
        	residues[i] = NULL;
        	sequence[i] = 0;
        	ca[i] = NULL;
        	res_reach[i] = 0;
        }
        
        for (i=0; oldres[i]; i++)
        {
            residues[i] = oldres[i];
            sequence[i] = oldseq[i];
            ca[i] = oldca[i];
            res_reach[i] = oldreach[i];
        }
        residues[i] = 0;
        sequence[i] = 0;
    }

    i = resno-1;
    
    if (i)
    {
    	Point* pts = residues[i-1]->predict_next_NHCA();
    	residues[i] = new AminoAcid(aaletter, residues[i-1]);
    	
    	Atom* a = residues[i]->get_atom("N");
    	if (a)
    	{
    		float r = pts[0].get_3d_distance(a->get_location());
    		if (fabs(r) >= 0.01) cout << "Warning: Residue " << resno << " N location is off by " << r << "A." << endl;
    		
    		Atom* b = residues[i]->get_atom("HN");
    		if (!b) b = residues[i]->get_atom("H");
    		if (b)
    		{
    			Point HN = b->get_location().subtract(a->get_location());
    			HN.scale(1);
    			Point pt = pts[1].subtract(pts[0]);
    			pt.scale(1);
    			r = pt.get_3d_distance(HN);
    			if (fabs(r) >= 0.01) cout << "Warning: Residue " << resno << " HN location is off by " << r << "A." << endl;
			}
    	}
    	else cout << "Warning: Residue " << resno << " has no N atom." << endl << flush;
    	
    	delete[] pts;
    }
    else
    	residues[i] = new AminoAcid(aaletter, 0);
    
    sequence[i] = aaletter;
    ca[i] = residues[i]->get_atom("CA");
    res_reach[i] = residues[i]->get_aa_definition()->reach;
    residues[i+1] = 0;
    sequence[i+1] = 0;

    if (!aaptrmin.n || residues[i] < aaptrmin.paa) aaptrmin.paa = residues[i];
    if (!aaptrmax.n || residues[i] > aaptrmax.paa) aaptrmax.paa = residues[i];

    return true;
}

AminoAcid* Protein::get_residue(int resno)
{
    if (!resno) return 0;
    if (!residues) return 0;

    int i;
    for (i=0; residues[i]; i++)
    {
        if (residues[i]->get_residue_no() == resno) return residues[i];
    }

    return NULL;
}

Atom* Protein::get_atom(int resno, const char* aname)
{	AminoAcid* aa = get_residue(resno);
	
	if (!aa) return NULL;
	return aa->get_atom(aname);
}

Point Protein::get_atom_location(int resno, const char* aname)
{	AminoAcid* aa = get_residue(resno);
	
	if (!aa)
	{
		Point pt;
		return pt;
	}
	return aa->get_atom_location(aname);
}

bool Protein::add_sequence(const char* lsequence)
{
    if (!lsequence) return false;

    int i;
    for (i=0; lsequence[i]; i++)
    {
        add_residue(i+1, lsequence[i]);
    }
    
    for (i=0; lsequence[i]; i++)
    {
    	if (get_atom(i+1, "CB"))		// Don't check glycine.
    	{
		    float r = get_atom_location(i+1, "CA").get_3d_distance(get_atom_location(i+1, "CB"));
		    if (fabs(r-1.54) > 0.5) cout << "Error: " << i+1 << lsequence[i] << ":CA-CB = " << r << endl;
        }
    }
    
    int seql = get_seq_length();
    Molecule* aas[seql+4];
    for (i=1; i<=seql; i++)
    {
    	aas[i-1] = get_residue(i);
    	aas[i] = 0;
    }
    Molecule::multimol_conform(aas, 25);
    
    set_clashables();

    return true;
}

void Protein::save_pdb(FILE* os)
{
    int i, offset=0;

	if (remarks.size())
	{
		for (i=0; i<remarks.size(); i++)
		{
			fprintf(os, "%s", remarks[i].c_str());
		}
	}
	
    if (!residues) return;
    for (i=0; residues[i]; i++)
    {
        residues[i]->save_pdb(os, offset);
        offset += residues[i]->get_atom_count();
    }
    if (m_mcoord)
    {
        for (i=0; m_mcoord[i]; i++)
        {
            cout << "Saving " << m_mcoord[i]->metal->name << endl;
            m_mcoord[i]->metal->save_pdb_line(os, ++offset);
        }
    }

    fprintf(os, "\nTER\n");
}

void Protein::end_pdb(FILE* os)
{
    fprintf(os, "END\n");
}

int Protein::load_pdb(FILE* is)
{
    AminoAcid* restmp[65536];
    char buffer[1024];
    Atom* a;
    
    AminoAcid useless('#');		// Feed it nonsense just so it has to load the data file.
    AminoAcid* prevaa = nullptr;
    
    int i, rescount=0;

    while (!feof(is))
    {
        try
        {
        	int told = ftell(is);
        	fgets(buffer, 1003, is);
        	
        	if (buffer[0] == 'A' &&
        		buffer[1] == 'T' &&
        		buffer[2] == 'O' &&
        		buffer[3] == 'M'
        		)
    		{
    			fseek(is, told, SEEK_SET);
    			
				char tmp3let[5];
    			for (i=0; i<3; i++)
    				tmp3let[i] = buffer[17+i];
    			tmp3let[4] = 0;
    			
    			for (i=0; i<256; i++)
    			{
    				// if (aa_defs[i].name[0] && !strcmp(aa_defs[i]._3let, tmp3let))
    				if (!aa_defs[i].loaded)
    				{
    					AminoAcid* aa = new AminoAcid(is, prevaa);
						// cout << rescount << tmp3let << " " << flush;
						restmp[rescount++] = aa;
						restmp[rescount] = NULL;
						prevaa = aa;
						goto _found_AA;
    				}
    			}
    			
    			// If no AA, load a metal.
                if (!metcount % 16)
                {
                    Atom** mtmp = new Atom*[metcount+20];
                    if (metals)
                    {
                        for (i=0; metals[i]; i++)
                            mtmp[i] = metals[i];
                        mtmp[i] = NULL;
                        delete metals;
                    }
                    metals = mtmp;
                }
    			
    			a = new Atom(is);
                metals[metcount++] = a;
                metals[metcount] = NULL;
                // cout << "M" << metcount << " ";

                for (i=0; i<26; i++)
                {
                    if (aa_defs[i]._1let && !strcmp(aa_defs[i]._3let, a->aa3let ))
                    {
                        a->aaletter = aa_defs[i]._1let;
                        break;
                    }
                }
    		}
    		else
    		{
    			if (buffer[0] == 'R' &&
            		buffer[1] == 'E' &&
            		buffer[2] == 'M' &&
            		buffer[3] == 'A' &&
            		buffer[4] == 'R' &&
            		buffer[5] == 'K'
            		)
            	{
	            	remarks.push_back(buffer);
	            	// cout << "Found remark " << buffer;
	            	
	            	if (buffer[7] == '6' && buffer[8] < '!')
	            	{
	            		char** fields = chop_spaced_fields(buffer);
	            		if (fields[2] && !fields[3])
	            			name = fields[2];
	            	}
            	}
    		}
        	
            _found_AA:
            ;
        }
        catch (int ex)
        {
            // cout << "Exception " << ex << endl;
            switch (ex)
            {
            	case ATOM_NOT_OF_AMINO_ACID:
                if (!metcount % 16)
                {
                    Atom** mtmp = new Atom*[metcount+20];
                    if (metals)
                    {
                        for (i=0; metals[i]; i++)
                            mtmp[i] = metals[i];
                        mtmp[i] = NULL;
                        delete metals;
                    }
                    metals = mtmp;
                }

                a = new Atom(is);
                metals[metcount++] = a;
                metals[metcount] = NULL;

                for (i=0; i<26; i++)
                {
                    if (aa_defs[i]._1let && !strcmp(aa_defs[i]._3let, a->aa3let ))
                    {
                        a->aaletter = aa_defs[i]._1let;
                        break;
                    }
                }
                break;

            	case NOT_ATOM_RECORD:
            	fgets(buffer, 1003, is);
            	cout << buffer << endl;
            	if (buffer[0] == 'R' &&
            		buffer[1] == 'E' &&
            		buffer[2] == 'M' &&
            		buffer[3] == 'A' &&
            		buffer[4] == 'R' &&
            		buffer[5] == 'K'
            		)
	            	remarks.push_back(buffer);
	            break;
	            
	            default:
	            throw 0xbadca22;
            }
        }
    }

	int arrlimit = rescount+1;
    residues 	= new AminoAcid*[arrlimit];
    sequence 	= new char[arrlimit];
    ca       	= new Atom*[arrlimit];
    res_reach	= new float[arrlimit];
    
    for (i=0; i<arrlimit; i++)
    {
    	residues[i] = NULL;
    	sequence[i] = 0;
    	ca[i] = NULL;
    	res_reach[i] = 0;
    }

    for (i=0; i<rescount; i++)
    {
        try
        {
            residues[i] = restmp[i];

            Atom *atom = residues[i]->get_atom("N"), *btom;
            AminoAcid* prev = get_residue(residues[i]->get_residue_no()-1);
            if (prev)
            {
                btom = prev->get_atom("C");
                if (atom && btom) atom->bond_to(btom, 1.5);
            }

            if (!aaptrmin.n || residues[i] < aaptrmin.paa) aaptrmin.paa = residues[i];
            if (!aaptrmax.n || residues[i] > aaptrmax.paa) aaptrmax.paa = residues[i];

            AADef* raa   = restmp[i]->get_aa_definition();
            if (!raa) cout << "Warning: Residue " << (i+1) << " has no AADef." << endl;
            sequence[i]  = raa ? raa->_1let : '?';
            ca[i]		 = restmp[i]->get_atom("CA");
            res_reach[i] = raa ? raa->reach : 2.5;
        }
        catch (int e)
        {
            cout << "Residue " << (i+1) << " threw an error." << endl;
            throw e;
        }
    }
    // cout << "Read residue " << *residues[rescount] << endl;
    residues[rescount] = 0;

    set_clashables();

    return rescount;
}

int  Protein::get_seq_length()
{
    if (!sequence) return 0;
    int i;
    for (i=0; sequence[i]; i++);	// Obtain the count.
    return i;
}

std::string Protein::get_sequence()
{
	if (!sequence) return 0;
	return std::string(sequence);
}

int  Protein::get_start_resno()
{
    if (!residues) return 0;
    else return residues[0]->get_residue_no();
}

std::vector<std::string> Protein::get_remarks(std::string search_for)
{
	std::vector<string> retval;
	int i;
	for (i=0; i<remarks.size(); i++)
	{
		if (!search_for.length() || strstr(remarks[i].c_str(), search_for.c_str()))
			retval.push_back(remarks[i]);
	}
	
	return retval;
}

void Protein::set_clashables()
{
    int i, j, k;

    // cout << "Setting clashables." << endl;

    if (res_can_clash)
    {
        delete[] res_can_clash;
    }

    int seqlen = get_seq_length();
    res_can_clash = new AminoAcid**[seqlen+1];

    // cout << "seqlen is " << seqlen << endl;

    for (i=0; i<seqlen; i++)
    {
        if (debug) *debug << endl << "Testing residue " << residues[i]->get_residue_no() << endl;
        AminoAcid* temp[seqlen+1];
        k=0;
        for (j=0; j<seqlen; j++)
        {
            if (j == i) continue;
            if (residues[i]->can_reach(residues[j]))
            {
                temp[k++] = residues[j];
                if (debug) *debug << *residues[j] << " can reach " << *residues[i] << endl;
            }
        }
        res_can_clash[i] = new AminoAcid*[k+1];
        for (j=0; j<k; j++)
        {
            res_can_clash[i][j] = temp[j];
            /*cout << residues[i]->get_aa_definition()->_3let << residues[i]->get_residue_no()
            	 << " can clash with "
            	 << res_can_clash[i][j]->get_aa_definition()->_3let << res_can_clash[i][j]->get_residue_no()
            	 << endl;*/
        }
        res_can_clash[i][k] = 0;
    }

    res_can_clash[seqlen] = 0;
    
    /*for (i=0; i<seqlen; i++)
    {
    	cout << i << ": ";
    	for (j=0; res_can_clash[i][j]; j++)
    		cout << *res_can_clash[i][j] << " ";
    	cout << endl;
    }*/
}

std::vector<AminoAcid*> Protein::get_residues_near(Point pt, float maxr, bool facing)
{
	std::vector<AminoAcid*> retval;
	
	float cb_tolerance_angle = 44 * fiftyseventh;
	float tolerance_sine = sin(cb_tolerance_angle);
	
	if (!residues) return retval;
	int i, j;
	
	#if _DBG_TUMBLE_SPHERES
	cout << "Protein::get_residues_near(" << pt << ", " << maxr << ")" << endl;
	#endif
	
    for (i=0; residues[i]; i++)
    {
    	float r = pt.get_3d_distance(residues[i]->get_atom_location("CA"));
    	
    	if (facing && residues[i]->get_atom("CB"))
    	{
    		float r1 = pt.get_3d_distance(residues[i]->get_atom_location("CB"));
    		float r2 = residues[i]->get_atom_location("CA").get_3d_distance(residues[i]->get_atom_location("CB"));
    		float tolerance = r2 * tolerance_sine;
    		if (r1 > r+tolerance) continue;
    	}
    	
    	if (r <= maxr)
    	{
    		retval.push_back(residues[i]);
    		#if _DBG_TUMBLE_SPHERES
    		cout << residues[i]->get_3letter() << residues[i]->get_residue_no() << " ";
    		#endif
		}
    }
    #if _DBG_TUMBLE_SPHERES
    cout << endl << endl;
    #endif
    
    return retval;
}

AminoAcid** Protein::get_residues_can_clash(int resno)
{
    if (!residues) return 0;
	if (!res_can_clash) set_clashables();

    int i, j;
    for (i=0; residues[i]; i++)
    {
        if (residues[i]->get_residue_no() == resno)
        {
        	if (!res_can_clash[i] || !res_can_clash[i][0]) set_clashables();
        	/*cout << i << ": " << flush;
        	if (res_can_clash[i] && res_can_clash[i][0])
        	{
        		cout << *res_can_clash[i][0] << endl;
				for (j=0; res_can_clash[i][j]; j++)
					cout << j << " " << flush;
				cout << endl;
			}*/
        	return res_can_clash[i];
    	}
    }

    return 0;
}

int Protein::get_residues_can_clash_ligand(AminoAcid** reaches_spheroid,
        Molecule* ligand,
        const Point nodecen,
        const Point size,
        const int* mcoord_resno
		)
{
    int i, j, sphres = 0;
    int seql = get_seq_length();
    bool resno_already[8192];
    for (i=0; i<8192; i++) resno_already[i] = false;

    for (i=0; i<SPHREACH_MAX; i++) reaches_spheroid[i] = NULL;

    for (i=1; i<=seql; i++)
    {
        AminoAcid* aa = residues[i-1];
        if (!aa) continue;

        int resno = aa->get_residue_no();

        if (mcoord_resno)
        {
            for (j=0; mcoord_resno[j]; j++)
            {
                if (mcoord_resno[j] == resno)
                {
                    if (!resno_already[resno])
                    {
                        reaches_spheroid[sphres++] = aa;
                        resno_already[resno] = true;
#if _DBG_REACHLIG
                        if (debug)
                        {
                            Star s;
                            s.paa = aa;
                            *debug << std::hex << s.n << std::dec << " " << flush;
                        }
#endif
                    }
                    continue;
                }
            }
        }

        Atom* ca = aa->get_atom("CA");
        if (!ca) continue;
        Atom* cb = aa->get_atom("CB");

        Point pt = ca->get_location();
        Atom* a = ligand->get_nearest_atom(pt);
        Point pt2;
        if (a) pt2 = a->get_location();
        else   pt2 = ligand->get_barycenter();

        if (cb)
        {
            Point pt1 = pt.subtract(&pt2);
            float angle = find_3d_angle(cb->get_location(), pt2, ca->get_location());
            if (pt1.magnitude() < 3 || angle < _can_clash_angle)
            {
                if (pt1.magnitude() < (aa->get_reach()+2))
                {
                    if (!resno_already[resno])
                    {
                        reaches_spheroid[sphres++] = aa;
                        resno_already[resno] = true;
#if _DBG_REACHLIG
                        if (debug)
                        {
                            Star s;
                            s.paa = aa;
                            *debug << std::hex << s.n << std::dec << " " << flush;
                        }
#endif
                    }
                    continue;
                }
            }

            angle = find_3d_angle(cb->get_location(), nodecen, ca->get_location());
            if (angle > _can_clash_angle) continue;
        }

        pt = pt.subtract(&nodecen);
        Point pt1 = pt;
        float dist = pt.magnitude();
        pt1.scale(fmax(dist - aa->get_reach(), 0));

        pt1.x /= size.x;
        pt1.y /= size.y;
        pt1.z /= size.z;

        SCoord dir(&pt1);

        if (dir.r <= 1)
        {
            if (!resno_already[resno])
            {
                reaches_spheroid[sphres++] = aa;
                resno_already[resno] = true;
#if _DBG_REACHLIG
                if (debug)
                {
                    Star s;
                    s.paa = aa;
                    *debug << std::hex << s.n << std::dec << " " << flush;
                }
#endif
            }
        }

        aa->reset_conformer_momenta();
    }

    reaches_spheroid[sphres] = NULL;
#if _DBG_REACHLIG
    if (debug) *debug << endl << flush;
#endif

    return sphres;
}

bool Protein::aa_ptr_in_range(AminoAcid* aaptr)
{
    if (!aaptr) return false;
    if (aaptr < aaptrmin.paa || aaptr > aaptrmax.paa) return false;
    else return true;
}

Molecule* Protein::metals_as_molecule()
{
    Molecule* met=NULL;
    if (metals) met = new Molecule("(metals)", metals);

    InteratomicForce f;

    // Associate coordinating residue atoms with the metal ions so that the ions can have geometry.
    // Otherwise we end up with an exception later on in the InteratomicForce::total_binding() function.
    if (mcoord_resnos && mcoord_resnos[0])
    {
        int i, j, k, l, m, n;
        for (i=0; metals[i]; i++)
        {
            Point mloc = metals[i]->get_location();
            k = 0;
            for (j=0; mcoord_resnos[j]; j++)
            {
                AminoAcid* caa = get_residue(mcoord_resnos[j]);		// caa = coordinating amino acid.
                if (!caa) continue;
                caa->movability = MOV_NONE;
                Atom* mca = caa->get_nearest_atom(mloc, mcoord);
                if (!mca) continue;

                float r = mloc.get_3d_distance(mca->get_location());

                if (r < 2 * InteratomicForce::coordinate_bond_radius(metals[i], mca, mcoord))
                {
                    Bond* b = metals[i]->get_bond_by_idx(k++);
                    if (!b) break;
                    b->btom = mca;
                    b->cardinality = 0.5;
                    // cout << metals[i]->name << " coordinates to " << *caa << ":" << mca->name << endl;
                }
            }
        }
    }

    return met;
}

void Protein::rotate_backbone(int resno, bb_rot_dir dir, float angle)
{
    AminoAcid* bendy = get_residue(resno);
    if (!bendy) return;
    LocatedVector lv = bendy->rotate_backbone(dir, angle);

    if (lv.r)
    {
        int i, inc;
        AminoAcid* movable;
        inc = (dir == CA_desc || dir == C_desc) ? -1 : 1;

        for (i=resno+inc; movable = get_residue(i); i+=inc)
        {
            // cout << "Rotating " << i << endl;
            movable->rotate(lv, angle);
        }
    }
}

void Protein::rotate_backbone_partial(int startres, int endres, bb_rot_dir dir, float angle)
{
	if (startres == endres) return;
    int inc = (dir == CA_desc || dir == C_desc) ? -1 : 1;
    if (sgn(endres - startres) != sgn(inc))
    {
        cout << "ERROR: direction mismatch " << startres << "->" << endres
             << " but direction is " << inc << endl;
        return;
    }

    AminoAcid* bendy = get_residue(startres);
    if (!bendy) return;
    LocatedVector lv = bendy->rotate_backbone(dir, angle);

    if (lv.r)
    {
        int i;
        AminoAcid* movable;

        for (i=startres+inc; movable = get_residue(i); i+=inc)
        {
            movable->rotate(lv, angle);
            if (i == endres) break;
        }
    }
    
    set_clashables();
}

void Protein::conform_backbone(int startres, int endres, int iters, bool backbone_atoms_only)
{
    Point pt;
    conform_backbone(startres, endres, NULL, pt, NULL, pt, iters, backbone_atoms_only);
}

void Protein::conform_backbone(int startres, int endres, Atom* a, Point target, int iters)
{
    Point pt;
    conform_backbone(startres, endres, a, target, NULL, pt, iters, false);
}

void Protein::conform_backbone(int startres, int endres, Atom* a1, Point target1, Atom* a2, Point target2, int iters, bool backbone_atoms_only)
{
    conform_backbone(startres, endres, a1, target1, a2, target2, nullptr, Point(), iters, backbone_atoms_only);
}

#define DBGCONF 0

void Protein::conform_backbone(int startres, int endres,
                               Atom* a1, Point target1,
                               Atom* a2, Point target2,
                               Atom* a3, Point target3,
                               int iters, bool backbone_atoms_only
                              )
{
    int inc = sgn(endres-startres);
    int res, i, j, iter;
    bb_rot_dir dir1 = (inc>0) ? N_asc : CA_desc,
               dir2 = (inc>0) ? CA_asc : C_desc;
    
    int am = abs(endres-startres), minres = (inc>0) ? startres : endres;
    float momenta1o[am+4], momenta2o[am+4], momenta1e[am+4], momenta2e[am+4];
    int eando_res[am+4];
    float eando_mult[am+4];
    float r = 0, lastr = 99999;
    int iters_since_improvement = 0;
    
    for (res = startres; res; res += inc)
    {
    	int residx = res-minres;
    	momenta1o[residx] = randsgn()*_fullrot_steprad;
    	momenta2o[residx] = randsgn()*_fullrot_steprad;
    	momenta1e[residx] = randsgn()*_fullrot_steprad;
    	momenta2e[residx] = randsgn()*_fullrot_steprad;
    	eando_res[residx] = min(res + (rand() % 5) + 1, endres);
    	if (eando_res[residx] == res) eando_res[residx] = 0;
    	eando_mult[residx] = 1;
    	if (res == endres) break;
    }

	set_clashables();
    float tolerance = 1.2, alignfactor = 100, reversal = -0.81, enhance = 1.5;
    int ignore_clashes_until = iters*0.666;
    for (iter=0; iter<iters; iter++)
    {
    	#if DBGCONF
        // cout << "Iteration " << iter << endl;
        cout << " " << iter << flush;
        #endif
        
        for (res = startres; res != endres; res += inc)
        {
        	int residx = res-minres;
        	
            // Get the preexisting nearby residues and inter-residue binding/clash value.
            // These will likely have changed since last iteration.
            float bind=0, bind1=0, angle;

            for (i=res; i != endres; i += inc)
            {
                AminoAcid* aa = get_residue(i);
                if (!aa) continue;
                AminoAcid** rcc = get_residues_can_clash(i);
                if (!rcc) cout << "No clashables." << endl;
                if (a1 && (iter >= ignore_clashes_until)) bind -= aa->get_intermol_clashes(AminoAcid::aas_to_mols(rcc));
                else bind += aa->get_intermol_binding(rcc, backbone_atoms_only);
            }
            if (a1 && iter>10)
            {
                Point pt = a1->get_location();
                bind += alignfactor/(pt.get_3d_distance(target1)+0.001);
            }
            if (a2 && iter>10)
            {
                Point pt = a2->get_location();
                bind += alignfactor/(pt.get_3d_distance(target2)+0.001);
            }

			if (reinterpret_cast<long>(get_residue(res)) < 0x1000) cout << "Warning missing residue " << res << endl << flush;
			else if (strcmp(get_residue(res)->get_3letter(), "PRO"))		// TODO: Don't hard code this to proline, but check bond flexibility.
            {
                // Rotate the first bond a random amount. TODO: use angular momenta.
                angle = (iter & 1) ? momenta1o[residx] : momenta1e[residx]; // frand(-_fullrot_steprad, _fullrot_steprad);
                rotate_backbone_partial(res, endres, dir1, angle);
                if ((iter & 1) && eando_res[residx]) rotate_backbone_partial(eando_res[residx], endres, dir2, -angle*eando_mult[residx]);

                // Have bindings/clashes improved?
                for (i=res; i != endres; i += inc)
                {
                    AminoAcid* aa = get_residue(i);
                    AminoAcid** rcc = get_residues_can_clash(i);
                    if (a1 && (iter >= ignore_clashes_until)) bind1 -= aa->get_intermol_clashes(AminoAcid::aas_to_mols(rcc));
                	else bind1 += aa->get_intermol_binding(rcc, backbone_atoms_only);
                }
                if (a1)
                {
                    Point pt = a1->get_location();
                    bind1 += alignfactor/(pt.get_3d_distance(target1)+0.001);
                }
                if (a2)
                {
                    Point pt = a2->get_location();
                    bind1 += alignfactor/(pt.get_3d_distance(target2)+0.001);
                }
                if (a3)
                {
                    Point pt = a3->get_location();
                    bind1 += alignfactor/(pt.get_3d_distance(target3)+0.001);
                }

                // If no, put it back.
                // if (res == startres) cout << bind << " v. " << bind1 << endl;
                if (bind1 < tolerance*bind || (a1 && iters_since_improvement > 10 && frand(0,1)<0.25))
                {
                    rotate_backbone_partial(res, endres, dir1, -angle);
                    if (eando_res[residx]) rotate_backbone_partial(eando_res[residx], endres, dir2, angle*eando_mult[residx]);
                    if (iter & 1) momenta1o[residx] *= reversal;
                    else momenta1e[residx] *= reversal;
                }
                else
                {
                    if (bind1 < bind) bind = bind1;
                    if (iter & 1) momenta1o[residx] *= enhance;
                    else momenta1e[residx] *= enhance;
                }
            }

            // Rotate the second bond.
            angle = (iter & 1) ? momenta2o[residx] : momenta2e[residx]; // frand(-_fullrot_steprad, _fullrot_steprad);
            rotate_backbone_partial(res, endres, dir2, angle);
            if ((iter & 1) && eando_res[residx]) rotate_backbone_partial(eando_res[residx], endres, dir1, -angle*eando_mult[residx]);

            // Improvement?
            bind1 = 0;
            for (i=res; i != endres; i += inc)
            {
                AminoAcid* aa = get_residue(i);
                if (!aa) continue;
                AminoAcid** rcc = get_residues_can_clash(i);
                if (a1 && (iter >= ignore_clashes_until)) bind1 -= aa->get_intermol_clashes(AminoAcid::aas_to_mols(rcc));
            	else bind1 += aa->get_intermol_binding(rcc, backbone_atoms_only);
            }
            if (a1)
            {
                Point pt = a1->get_location();
                bind1 += alignfactor/(pt.get_3d_distance(target1)+0.001);
            }
            if (a2)
            {
                Point pt = a2->get_location();
                bind1 += alignfactor/(pt.get_3d_distance(target2)+0.001);
            }
            if (a3)
            {
                Point pt = a3->get_location();
                bind1 += alignfactor/(pt.get_3d_distance(target3)+0.001);
            }

            // If no, put it back.
            // if (res == startres) cout << bind << " vs. " << bind1 << endl;
            if (bind1 < tolerance*bind || (a1 && iters_since_improvement > 10 && frand(0,1)<0.25))
            {
                rotate_backbone_partial(res, endres, dir2, -angle);
                if ((iter & 1) && eando_res[residx]) rotate_backbone_partial(eando_res[residx], endres, dir1, angle*eando_mult[residx]);
                if (iter & 1) momenta2o[residx] *= reversal;
                else momenta2e[residx] *= reversal;
            }
            else
            {
                if (iter & 1) momenta2o[residx] *= enhance;
                else momenta2e[residx] *= enhance;
            }

            alignfactor *= 1.003;
            tolerance = ((tolerance-1)*0.97)+1;
        }
        
        r = 0;
        if (a1)
        {
        	Point pt = a1->get_location();
            r += pt.get_3d_distance(target1);
        }
        if (a2)
        {
        	Point pt = a2->get_location();
            r += pt.get_3d_distance(target2);
        }
        if (a3)
        {
        	Point pt = a3->get_location();
            r += pt.get_3d_distance(target3);
        }
        
        if (r < 0.999*lastr) iters_since_improvement = 0;
        else iters_since_improvement++;
        lastr = r;

        #if DBGCONF
        if (r) cout << "." << r << flush;
        #endif
    }
    #if DBGCONF
    cout << endl;
    #endif
    if (r > 2.5) cout << "Warning - protein strand alignment anomaly outside of tolerance." << endl << "# Anomaly is " << r << " Angstroms." << endl;
}

#define DBG_BCKCONN 0
#define _INCREMENTAL_BKCONN 1
void Protein::backconnect(int startres, int endres)
{
	int i;
    int inc = sgn(endres-startres);
    
    #if _INCREMENTAL_BKCONN
    int iter;
    for (iter=24; iter>=0; iter--)
    {
    #endif
    
		// Glom the last residue onto the target.
		// Then adjust its inner bonds so the other end points as closely to the previous residue as possible.
		// Then do the same for the previous residue, and the one before, etc,
		// all the way back to the starting residue.
		// Give a warning if the starting residue has an anomaly > 0.1A.
		AminoAcid *next, *curr, *prev;
		int pointer = endres;
		float movfactor = 1, decrement, anomaly = 0;
		
		decrement = 1.0 / fabs(endres - startres);		// if exterior is the opposite of interior, what's the opposite of increment?
		
		next = get_residue(endres+inc);
		curr = get_residue(pointer);
		prev = get_residue(pointer-inc);
		
		#if DBG_BCKCONN
		cout << "backconnect( " << startres << ", " << endres << ")" << endl;
		#endif
		while (next && curr)
		{
			#if DBG_BCKCONN
			cout << pointer << ":";
			#endif
			
			Point* pts = (inc > 0) ? next->predict_previous_COCA() : next->predict_next_NHCA();
			Point ptsc[4];
			
			#if _INCREMENTAL_BKCONN
			ptsc[0] = (inc > 0) ? curr->get_atom_location("C") : curr->get_atom_location("N");
			ptsc[1] = (inc > 0) ? curr->get_atom_location("O") : curr->HN_or_substitute_location();
			ptsc[2] = curr->get_atom_location("CA");
			
			Point _4avg[3];
			for (i=0; i<3; i++)
			{
				_4avg[0] = pts[i];
				_4avg[1] = ptsc[i];
				_4avg[0].weight = movfactor;
				_4avg[1].weight = 1.0 - movfactor;
				pts[i] = average_of_points(_4avg, 2);
			}
			#endif
			
			curr->glom(pts, inc > 0);
			delete[] pts;
			// break;
			
			#if DBG_BCKCONN
			cout << "g";
			#endif
			
			if (prev)
			{
				MovabilityType fmov = curr->movability;
				curr->movability = MOV_ALL;
				
				#if DBG_BCKCONN
				cout << "a";
				#endif
			
				pts = (inc < 0) ? prev->predict_previous_COCA() : prev->predict_next_NHCA();
				Point target_heavy = pts[0];
				Point target_pole = pts[1];
				delete[] pts;
				
				#if DBG_BCKCONN
				cout << "b";
				#endif
				
				float theta, step = fiftyseventh*1.0, r, btheta = 0, bestr;
				for (theta=0; theta < M_PI*2; theta += step)
				{
					r = target_heavy.get_3d_distance( (inc > 0) ? curr->get_atom_location("N") : curr->get_atom_location("C") );
					// r -= target_pole.get_3d_distance( (inc > 0) ? curr->HN_or_substitute_location() : curr->get_atom_location("O") );
					if (inc > 0) r -= target_pole.get_3d_distance(curr->HN_or_substitute_location());
					
					if (!theta || (r < bestr))
					{
						bestr = r;
						btheta = theta;
					}
					
					curr->rotate_backbone( (inc > 0) ? CA_desc : N_asc , step );
				}
				curr->rotate_backbone( (inc > 0) ? CA_desc : N_asc , btheta );
				
				#if DBG_BCKCONN
				cout << "c";
				#endif
				
				btheta=0;
				for (theta=0; theta < M_PI*2; theta += step)
				{
					r = target_heavy.get_3d_distance( (inc > 0) ? curr->get_atom_location("N") : curr->get_atom_location("C") );
					// r -= target_pole.get_3d_distance( (inc > 0) ? curr->HN_or_substitute_location() : curr->get_atom_location("O") );
					if (inc < 0) r -= target_pole.get_3d_distance(curr->get_atom_location("O"));
					
					if (!theta || (r < bestr))
					{
						bestr = r;
						btheta = theta;
					}
					
					curr->rotate_backbone( (inc > 0) ? C_desc : CA_asc , step );
				}
				curr->rotate_backbone( (inc > 0) ? C_desc : CA_asc , btheta );
				anomaly = bestr;
				
				#if DBG_BCKCONN
				cout << "d";
				#endif
			
				curr->movability = fmov;
			}
			
			if (pointer == startres)
			{
				#if DBG_BCKCONN
				cout << ". ";
				#endif
				break;
			}
			
			pointer -= inc;
			next = curr;
			curr = prev;
			prev = get_residue(pointer-inc);
			movfactor -= decrement;
			
			#if DBG_BCKCONN
			cout << ", ";
			#endif
		};
			
		#if DBG_BCKCONN
		cout << endl;
		#endif
		
		/*if (anomaly > 0.1) cout << "Warning! conform_backbone( " << startres << ", " << endres << " ) anomaly out of range." << endl
								<< "# " << (startres-inc) << " anomaly: " << anomaly << endl;*/
	#if _INCREMENTAL_BKCONN
	}
	#endif
}

void Protein::make_helix(int startres, int endres, float phi, float psi)
{
    make_helix(startres, endres, endres, phi, psi);
}

#define DBG_ASUNDER_HELICES 0
void Protein::make_helix(int startres, int endres, int stopat, float phi, float psi)
{
    int inc = sgn(endres-startres);
    if (inc != sgn(stopat-startres)) return;
    if (stopat < endres) endres = stopat;
    int res, i, j, iter;
    int phis[365], psis[365];
    bb_rot_dir dir1 = (inc>0) ? N_asc : CA_desc,
               dir2 = (inc>0) ? CA_asc : C_desc;

	#if DBG_ASUNDER_HELICES
	cout << "make_helix( " << startres << ", " << endres << ", " << stopat << ", " << phi << ", " << psi << " )" << endl;
	#endif
    for (res = startres; inc; res += inc)
    {
        AminoAcid* aa = get_residue(res);
        
        LocRotation lr = aa->enforce_peptide_bond();
        
        if (lr.v.r)
        {
            AminoAcid* movable;

            if (res != endres)
            for (i=res+inc; movable = get_residue(i); i+=inc)
            {
                LocatedVector lv = lr.get_lv();
                movable->rotate(lv, lr.a);
				#if DBG_ASUNDER_HELICES
				cout << i << " ";
				#endif
                if (i == stopat) break;
            }
			#if DBG_ASUNDER_HELICES
			cout << endl;
			#endif
        }

        LocRotation* lr2 = aa->flatten();

        for (j=0; j<5; j++)
        {
        	// cout << "Rotating " << *aa << " " << lr2[j].a*fiftyseven << " degrees." << endl;
            if (lr2[j].v.r && lr2[j].a)
            {
                AminoAcid* movable;

                if (res != endres) for (i=res+1; movable = get_residue(i); i+=inc)
                {
                    LocatedVector lv = lr2[j].get_lv();
                    movable->rotate(lv, lr2[j].a);
					#if DBG_ASUNDER_HELICES
					cout << i << " ";
					#endif
                    if (i == stopat) break;
                }
				#if DBG_ASUNDER_HELICES
				cout << endl;
				#endif

                int round_theta = (int)(lr2[j].a*fiftyseven+0.5);
                while (round_theta < 0) round_theta += 360;
                if (round_theta <= 360)
                {
                    if (j == 2) phis[round_theta]++;
                    if (j == 3) psis[round_theta]++;
                }
            }
        }
        delete[] lr2;
        
		// cout << "Rotating " << *aa << " phi " << (phi*fiftyseven) << " degrees." << endl;
        lr = aa->rotate_backbone_abs(dir1, phi);

        if (lr.v.r)
        {
            AminoAcid* movable;

            if (res != endres) for (i=res+inc; movable = get_residue(i); i+=inc)
            {
            	// cout << i << " ";
                LocatedVector lv = lr.get_lv();
                movable->rotate(lv, lr.a);
				#if DBG_ASUNDER_HELICES
				cout << i << " ";
				#endif
                if (i == stopat) break;
            }
			#if DBG_ASUNDER_HELICES
			cout << endl;
			#endif
        }
        // cout << endl;

        // cout << "Rotating " << *aa << " psi " << (psi*fiftyseven) << " degrees." << endl;
        lr = aa->rotate_backbone_abs(dir2, psi);

        if (lr.v.r)
        {
            AminoAcid* movable;

            if (res != endres) for (i=res+inc; movable = get_residue(i); i+=inc)
            {
                // cout << i << " ";
                LocatedVector lv = lr.get_lv();
                movable->rotate(lv, lr.a);
				#if DBG_ASUNDER_HELICES
				cout << i << " ";
				#endif
                if (i == stopat) break;
            }
			#if DBG_ASUNDER_HELICES
			cout << endl;
			#endif
        }
        // cout << endl;

        if (res == endres) break;
    }

    // Hang onto this line, might want it later.
    // if (phi > 0.1 || psi > 0.1) conform_backbone(startres, endres, 20, true);

#if 0
    // This is for finding values of phi and psi for helices.
    for (j=0; j<=360; j++)
    {
        if (phis[j])
        {
            cout << "??=" << j << ": ";
            for (i=0; i<phis[j]; i++) cout << "*";
            cout << endl;
        }
    }
    for (j=0; j<=360; j++)
    {
        if (psis[j])
        {
            cout << "??=" << j << ": ";
            for (i=0; i<psis[j]; i++) cout << "*";
            cout << endl;
        }
    }
#endif
	
	set_clashables();
    
    int seql = get_seq_length();
    Molecule* aas[seql+4];
    for (i=startres; i<=endres; i++)
    {
    	aas[i-startres] = get_residue(i);
    	aas[i-startres]->movability = MOV_FLEXONLY;
    	aas[i-startres+1] = 0;
    }
    aas[endres-startres+1] = 0;
    Molecule::multimol_conform(aas, 25);
}

void Protein::delete_residue(int resno)
{
    if (!resno) return;
    if (resno > get_seq_length()) return;
    if (!residues) return;

    int i, j;
    for (i=0; residues[i]; i++)
    {
        if (residues[i]->get_residue_no() == resno)
        {
            for (j=i+1; residues[j-1]; j++)
            {
                residues[j-1] = residues[j];
            }
            // TODO: sequence.
            ca = 0;
            res_reach = 0;

            return;
        }
    }
}

void Protein::delete_residues(int startres, int endres)
{
    int i;
    for (i=startres; i<=endres; i++) delete_residue(i);
}

void Protein::delete_sidechain(int resno)
{
    if (!resno) return;
    if (resno > get_seq_length()) return;
    if (!residues) return;

    AminoAcid* aa = get_residue(resno);
    aa->delete_sidechain();
}

void Protein::delete_sidechains(int startres, int endres)
{
    if (!residues) return;
    int i;
    for (i=0; residues[i]; i++)
    {
        int res = residues[i]->get_residue_no();
        if (res >= startres && res <= endres) residues[i]->delete_sidechain();
    }
}

Protein* gmprot;
Point gmtgt;

void ext_mtl_coord_cnf_cb(int iter)
{
    gmprot->mtl_coord_cnf_cb(iter);
}

MetalCoord* Protein::coordinate_metal(Atom* metal, int residues, int* resnos, std::vector<string> res_anames)
{
    int i, j=0, k=0, l, n;
    if (!m_mcoord)
    {
        m_mcoord = new MetalCoord*[2];
        m_mcoord[0] = new MetalCoord();
        m_mcoord[1] = NULL;
    }
    else
    {
        for (j=0; m_mcoord[j]; j++);	// Get count.
        MetalCoord** nmc = new MetalCoord*[j+2];
        for (i=0; i<j; i++) nmc[i] = m_mcoord[i];
        nmc[j] = new MetalCoord();
        nmc[j+1] = NULL;
        delete[] m_mcoord;
        m_mcoord = nmc;
    }

    if (!metals)
    {
        metals = new Atom*[2];
        metals[0] = metal;
        metals[1] = NULL;
    }
    else
    {
        for (k=0; metals[k]; k++);	// Get count.
        Atom** nma = new Atom*[k+2];
        for (i=0; i<k; i++) nma[i] = metals[i];
        nma[k] = metal;
        nma[k+1] = NULL;
        delete[] metals;
        metals = nma;
    }

    m_mcoord[j]->metal = metal;
    m_mcoord[j]->coord_res = new AminoAcid*[residues+2];
    m_mcoord[j]->coord_atoms = new Atom*[residues+2];

    int maxres = 0, minres = 0;
    for (i=0; i<residues; i++)
    {
        if (resnos[i] > maxres) maxres = resnos[i];
        if (!minres || resnos[i] < minres) minres = resnos[i];
        m_mcoord[j]->coord_res[i] = get_residue(resnos[i]);
        if (!m_mcoord[j]->coord_res[i])
        {
            cout << "Attempt to bind metal to residue " << resnos[i] << " not found in protein!" << endl;
            throw 0xbad12e5d;
        }
        m_mcoord[j]->coord_res[i]->m_mcoord = m_mcoord[j];
        m_mcoord[j]->coord_atoms[i] = m_mcoord[j]->coord_res[i]->get_atom(res_anames[i].c_str());
        if (!m_mcoord[j]->coord_atoms[i])
        {
            cout << "Attempt to bind metal to " << resnos[i] << ":" << res_anames[i] << " not found in protein!" << endl;
            throw 0xbada70b;
        }
    }
    m_mcoord[j]->coord_res[residues] = NULL;
    m_mcoord[j]->coord_atoms[residues] = NULL;

    // Get the plane of the coordinating atoms, then get the normal.
    Point ptarr[3] =
    {
        m_mcoord[j]->coord_atoms[0]->get_location(),
        m_mcoord[j]->coord_atoms[1]->get_location(),
        m_mcoord[j]->coord_atoms[2]->get_location()
    };
    Point coordcen = average_of_points(ptarr, 3);
    SCoord normal = compute_normal(ptarr[0], ptarr[1], ptarr[2]);
    normal.r = 3;
    Point pnormal = coordcen.add(normal), pantinormal = coordcen.subtract(normal);
    int nc = 0, anc = 0;

    // Iterate from minres to maxres, counting how many CA atoms are on each side of the plane and are within a threshold distance.
    for (i = minres; i <= maxres; i++)
    {
        Atom* la = ca[i];
        if (!la) continue;
        Point lpt = la->get_location();
        float nr = lpt.get_3d_distance(pnormal);
        float anr = lpt.get_3d_distance(pantinormal);

        // Don't sweat residues on the opposite side of the binding pocket if the coord atoms are on different helices.
        if (nr > 7 || anr > 7) continue;

        // If equidistant, don't count it.
        if (nr < anr) nc++;
        if (nr > anr) anc++;
    }

    // Choose the side of the plane with the fewest CA atoms and set gmtgt about 1A in that direction of plane center.
    // If the two sides are the same, set gmtgt to coordcen.
    // cout << "nc " << nc << " | anc " << anc << endl;
    if (nc < anc)
    {
        normal.r = 7;
        gmtgt = pnormal.add(normal);
        metal->move(gmtgt);
        gmtgt = pnormal;

    }
    else if (nc > anc)
    {
        normal.r = 7;
        gmtgt = pantinormal.subtract(normal);
        metal->move(gmtgt);
        gmtgt = pantinormal;
    }
    else gmtgt = coordcen;

    // Create an array of Molecules containing the coordinating residues and surrounding residues, with the metal as its own Molecule.
    Atom* ma[2];
    ma[0] = metal;
    ma[1] = NULL;
    Molecule m("Metal", ma);
    m.movability = MOV_ALL;
    Molecule* lmols[maxres-minres+8];
    int lmolc=0;
    lmols[lmolc++] = &m;
    for (i=minres; i<=maxres; i++)
    {
        AminoAcid* aa = get_residue(i);
        if (aa)
        {
            lmols[lmolc++] = aa;
            if (i >= minres && i <= maxres)
                aa->m_mcoord = m_mcoord[j];		// Sets the coordinating residues, and all in between residues, to backbone immovable.
        }
    }
    lmols[lmolc] = NULL;

    // Make sure to set the coordinating residues so that their side chains are flexible.
    for (i=0; m_mcoord[j]->coord_res[i]; i++)
        m_mcoord[j]->coord_res[i]->movability = MOV_FLEXONLY;

	// Flex the side chains to all be close to one another.
	int iter;
	for (iter=0; iter<10; iter++)
	{
		for (i=0; i<residues; i++)
		{
		    AminoAcid* aa = get_residue(resnos[i]);
		    if (aa)
		    {
		    	Bond** bb = aa->get_rotatable_bonds();
		    	if (bb)
		    	{
		    		float rad = 0, step = 10*fiftyseventh;
		    		float bestrad, bestr, r;
		    		
		    		for (l=0; bb[l]; l++)
		    		{
		    			bestrad = 0;
		    			bestr = 999999;
		    			for (; rad < M_PI*2; rad += step)
		    			{
		    				bb[l]->rotate(step);
		    				r = 0;
		    				for (n=0; n<residues; n++)
		    				{
		    					if (resnos[n] == resnos[i]) continue;
		    					r += get_atom_location(resnos[n], res_anames[n].c_str()).get_3d_distance(get_atom_location(resnos[i], res_anames[i].c_str()));
		    				}
		    				
		    				/*cout << iter << " " << *aa << ":"
		    					 << bb[l]->atom->name << "-" << bb[l]->btom->name
		    					 << " " << rad*fiftyseven << "deg "
		    					 << r << endl;*/
		    				
		    				if (r < bestr)
		    				{
		    					bestrad = rad;
		    					bestr = r;
		    				}
		    			}
		    		
		    			if (bestrad) bb[l]->rotate(bestrad);
		    			
		    		}		// for (l=0; bb[l]; l++)
		    	}		// if (bb)
		    }		// if (aa)
		}		// for (i=0; i<residues; i++)
	}		// for iter
	
	// Move the metal to the new center of all coordinating atoms.
	Point pt4avg[residues+2];
	l=0;
	for (n=0; n<residues; n++)
	{
		Point respt = get_atom_location(resnos[n], res_anames[n].c_str());
		
		if (n>0 && resnos[n] == resnos[n-1])
		{
			pt4avg[l-1].x = (pt4avg[l-1].x + respt.x)/2;
			pt4avg[l-1].y = (pt4avg[l-1].y + respt.y)/2;
			pt4avg[l-1].z = (pt4avg[l-1].z + respt.z)/2;
		}
		else
			pt4avg[l++] = respt;
	}
	Point ptmtl = average_of_points(pt4avg, l);
	metal->move(ptmtl);

    // Multimol conform the array.
    gmprot = this;
    // Molecule::multimol_conform(lmols, 250, &ext_mtl_coord_cnf_cb);

    // Set the coordinating residues' sidechains to immovable.
    for (i=0; m_mcoord[j]->coord_res[i]; i++)
        m_mcoord[j]->coord_res[i]->movability = MOV_NONE;

    m_mcoord[j]->locked = true;
    return m_mcoord[j];
}

void Protein::mtl_coord_cnf_cb(int iter)
{
    int i;
    for (i=0; m_mcoord[i]; i++)
    {
        // SCoord delta(m_mcoord[i]->coord_atom_avg_loc().subtract(m_mcoord[i]->metal->get_location()));
        SCoord delta(gmtgt.subtract(m_mcoord[i]->metal->get_location()));
        delta.r *= 0.1;
        m_mcoord[i]->metal->move_rel(&delta);
    }
}

float Protein::get_helix_orientation(int startres, int endres)
{
    int i, j;

    AminoAcid* aa;
    Atom* a;
    int rescount = (endres - startres)+1;
    int acount = 3*rescount;
    if (acount < 11)
    {
        cout << "Helix too short for determining orientation. " << startres << "-" << endres << endl;
        return 0;
    }
    Point pt, ptarr[acount+8];
    for (i=0; i<=rescount; i++)
    {
        /*Star s;
        s.pprot = this;
        cout << i << ":" << hex << s.n << dec << " " << flush;*/
        int resno = i+startres;
        // cout << resno << " ";
        aa = get_residue(resno);
        if (aa) a = aa->get_atom("N");
        if (a) pt = a->get_location();
        ptarr[i*3] = pt;

        if (aa) a = aa->get_atom("CA");
        if (a) pt = a->get_location();
        ptarr[i*3+1] = pt;

        if (aa) a = aa->get_atom("C");
        if (a) pt = a->get_location();
        ptarr[i*3+2] = pt;
    }

    // Take a running average of 10-atom blocks, then measure the average radians from vertical for imaginary lines between consecutive averages.
    int bcount = acount-10;
    Point blkavg[bcount+8];
    float retval = 0;
    j=0;

    for (i=0; i<bcount; i++)
    {
        blkavg[i] = average_of_points(&ptarr[i], 10);
        if (i>0)
        {
            SCoord v(blkavg[i].subtract(blkavg[i-1]));
            retval += v.theta;
            j++;
        }
    }

    return retval/j;
}

float Protein::orient_helix(int startres, int endres, int stopat, float angle, int iters)
{
    AminoAcid* aa = get_residue(startres-1);
    float n_am = 0.1, ca_am = 0.1;
    int iter;
    float ha;

    ha = get_helix_orientation(startres, endres);
    for (iter = 0; iter < iters; iter++)
    {
        rotate_backbone_partial(startres, stopat, N_asc, n_am);
        float nha = get_helix_orientation(startres, endres);

        if (fabs(nha-angle) <= fabs(ha-angle))
        {
            ha = nha;
            n_am *= 1.1;
            cout << "+";
        }
        else
        {
            rotate_backbone_partial(startres, stopat, N_asc, -n_am);
            n_am *= -0.75;
            cout << "x";
        }

        rotate_backbone_partial(startres, stopat, CA_asc, ca_am);
        nha = get_helix_orientation(startres, endres);

        if (fabs(nha-angle) <= fabs(ha-angle))
        {
            ha = nha;
            ca_am *= 1.1;
            cout << "+";
        }
        else
        {
            rotate_backbone_partial(startres, stopat, CA_asc, -ca_am);
            ca_am *= -0.75;
            cout << "x";
        }
    }
    cout << " ";

    return ha;
}

void Protein::set_region(std::string rgname, int start, int end)
{
    int i;
    for (i=0; i<PROT_MAX_RGN; i++) if (!regions[i].start) break;
    if (i >= PROT_MAX_RGN) return;		// Nope.

    regions[i].name = rgname;
    regions[i].start = start;
    regions[i].end = end;
}

Region Protein::get_region(const std::string rgname)
{
    int i;
    for (i=0; i<PROT_MAX_RGN; i++) if (regions[i].name == rgname) return regions[i];
    return Region();
}


int Protein::get_region_start(const std::string name)
{
	Region rgn = get_region(name);
	return rgn.start;
}

int Protein::get_region_end(const std::string name)
{
	Region rgn = get_region(name);
	return rgn.end;
}

Point Protein::get_region_center(int startres, int endres)
{
	int rglen = endres-startres;
	Point range[rglen+4];
	
	int i;
	for (i=0; i<rglen; i++)
	{
		// This is slow but that's okay.
		range[i] = get_residue(startres+i)->get_barycenter();
	}
	
	return average_of_points(range, rglen);
}

void Protein::move_piece(int start_res, int end_res, Point new_center)
{
	Point old_center = get_region_center(start_res, end_res);
	SCoord move_amt = new_center.subtract(old_center);
	
	int i;
	for (i=start_res; i<=end_res; i++)
	{
		AminoAcid* aa = get_residue(i);
		aa->aamove(move_amt);
	}
}

void Protein::rotate_piece(int start_res, int end_res, int align_res, Point align_target, int pivot_res)
{
	Point pivot = pivot_res ? get_residue(pivot_res)->get_barycenter() : get_region_center(start_res, end_res);
	Point align = get_residue(align_res)->get_barycenter();
	Rotation rot = align_points_3d(&align, &align_target, &pivot);
	
	LocatedVector lv(rot.v);
	lv.origin = pivot;
	int i;
	for (i=start_res; i<=end_res; i++)
	{
		AminoAcid* aa = get_residue(i);
		aa->rotate(lv, rot.a);
	}
}



Point Protein::find_loneliest_point(Point cen, Point sz)
{
	if (!residues) return cen;
	
	float x, y, z, xp, yp, zp, xa, ya, za, r, bestr = 0, step = 0.25;
	int i;
	Point retval = cen;
	
	sz.x /= 2; sz.y /= 2; sz.z /= 2;
	
	/*if (fabs(sz.x) > 4) sz.x = 4;
	if (fabs(sz.y) > 4) sz.y = 4;
	if (fabs(sz.z) > 4) sz.z = 4;*/
	
	for (x = -sz.x; x <= sz.x; x += step)
	{
		xp = x / sz.x; xp *= xp;
		xa = cen.x + x;
		for (y = -sz.y; y <= sz.y; y += step)
		{
			yp = y / sz.y; yp *= yp;
			ya = cen.y + y;
			for (z = -sz.z; z <= sz.z; z += step)
			{
				zp = z / sz.z; zp *= zp;
				za = cen.z + z;
				r = sqrt(xp+yp+zp);
				if (r > 1) continue;
				
				Point maybe(xa, ya, za);
				float minr = Avogadro;
				
				for (i=0; residues[i]; i++)
				{
					Atom* a = residues[i]->get_nearest_atom(maybe);
					if (a)
					{
						r = a->get_location().get_3d_distance(maybe);
						if (r < minr) minr = r;
					}
				}
				
				if (minr < 1e9 && minr > bestr)
				{
					retval = maybe;
					bestr = minr;
				}
			}
		}
	}
	
	return retval;
}

Point Protein::estimate_pocket_size(std::vector<AminoAcid*> ba)
{
	int i, n = ba.size();
	if (!n) return Point();
	float cx, cy, cz;
	
	cx = cy = cz = 0;
	for (i=0; i<n; i++)
	{
		Point pt = ba[i]->get_atom_location("CA");
		cx += pt.x;
		cy += pt.y;
		cz += pt.z;
	}
	
	Point center(cx/n, cy/n, cz/n);
	
	float sx, sy, sz, wx, wy, wz;
	sx = sy = sz = wx = wy = wz = 0;
	for (i=0; i<n; i++)
	{
		Point pt = ba[i]->get_atom_location("CA").subtract(center);
		float mag = pt.magnitude();
		mag -= 0.666 * ba[i]->get_reach();
		pt.scale(mag);
		float lwx = (fabs(pt.x) / sqrt(pt.y*pt.y + pt.z*pt.z)) / mag;
		float lwy = (fabs(pt.y) / sqrt(pt.x*pt.x + pt.z*pt.z)) / mag;
		float lwz = (fabs(pt.z) / sqrt(pt.x*pt.x + pt.y*pt.y)) / mag;
		
		sx += lwx * fabs(pt.x);
		sy += lwy * fabs(pt.y);
		sz += lwz * fabs(pt.z);
		
		wx += lwx;
		wy += lwy;
		wz += lwz;
	}
	
	Point size(sx/wx, sy/wy, sz/wz);
	
	return size;
}












