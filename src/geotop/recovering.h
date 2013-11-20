
/* STATEMENT:
 
 Geotop MODELS THE ENERGY AND WATER FLUXES AT THE LAND SURFACE
 Geotop 1.225-15 - 20 Jun 2013
 
 Copyright (c), 2013 - Stefano Endrizzi 
 
 This file is part of Geotop 1.225-15
 
 Geotop 1.225-15  is a free software and is distributed under GNU General Public License v. 3.0 <http://www.gnu.org/licenses/>
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE
 
 Geotop 1.225-15  is distributed as a free software in the hope to create and support a community of developers and users that constructively interact.
 If you just use the code, please give feedback to the authors and the community.
 Any way you use the model, may be the most trivial one, is significantly helpful for the future development of the Geotop model. Any feedback will be highly appreciated.
 
 If you have satisfactorily used the code, please acknowledge the authors.
 
 */

void assign_recovered_map(short old, long n, char *name, DOUBLEMATRIX *assign, PAR *par, DOUBLEMATRIX *Zdistr);

void assign_recovered_map_vector(short old, long n, char *name, DOUBLEVECTOR *assign, LONGMATRIX *rc, PAR *par, DOUBLEMATRIX *Zdistr);

void assign_recovered_map_long(short old, long n, char *name, LONGMATRIX *assign, PAR *par, DOUBLEMATRIX *Zdistr);

void assign_recovered_tensor(short old, long n, char *name, DOUBLETENSOR *assign, PAR *par, DOUBLEMATRIX *Zdistr);

void assign_recovered_tensor_vector(short old, long n, char *name, DOUBLEMATRIX *assign, LONGMATRIX *rc, PAR *par, DOUBLEMATRIX *Zdistr);

void assign_recovered_tensor_channel(short old, long n, char *name, DOUBLEMATRIX *assign, LONGVECTOR *r, LONGVECTOR *c, DOUBLEMATRIX *Zdistr);

void recover_run_averages(short old, DOUBLEMATRIX *A, char *name, DOUBLEMATRIX *LC, LONGMATRIX *rc, PAR *par, long n);

void print_run_averages_for_recover(DOUBLEMATRIX *A, char *name, long **j_cont, PAR *par, long n, long nr, long nc);
