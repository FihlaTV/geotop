
/* STATEMENT:
 
 Geotop MODELS THE ENERGY AND WATER FLUXES AT THE LAND SURFACE
 Geotop 2.0.0 - 31 Oct 2013
 
 Copyright (c), 2013 - Stefano Endrizzi 
 
 This file is part of Geotop 2.0.0
 
 Geotop 2.0.0  is a free software and is distributed under GNU General Public License v. 3.0 <http://www.gnu.org/licenses/>
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE
 
 Geotop 2.0.0  is distributed as a free software in the hope to create and support a community of developers and users that constructively interact.
 If you just use the code, please give feedback to the authors and the community.
 Any way you use the model, may be the most trivial one, is significantly helpful for the future development of the Geotop model. Any feedback will be highly appreciated.
 
 If you have satisfactorily used the code, please acknowledge the authors.
 
 */
    
    
void write_output(TIMES *times,WATER *wat,CHANNEL *cnet,PAR *par,TOPO *top,LAND *land,SOIL *sl,ENERGY *egy,SNOW *snow,GLACIER *glac,METEO *met);

void write_output_headers(long n, TIMES *times, WATER *wat, PAR *par, TOPO *top, LAND *land, SOIL *sl, ENERGY *egy, SNOW *snow, GLACIER *glac);

void write_soil_output(long i, long iname, double init_date, double end_date, double JDfrom0, double JD, long day, long month, long year, 
					   long hour, long minute, SOIL *sl, PAR *par, double psimin, double cosslope, long j);
					   
void write_snow_output(long i, long iname, long r, long c, double init_date, double end_date, double JDfrom0, double JD, 
					   long day, long month, long year, long hour, long minute, DOUBLEVECTOR *n, STATEVAR_3D *snow, PAR *par, double cosslope);

void write_soil_file(long lmin, long i, FILE *f, long d, long m, long y, long h, long mi, double JDfrom0, double JDfrom0init, 
					 double JDfrom0end, double *var, PAR *par, double *dz, double cosslope, DOUBLEMATRIX *excess_ice, long i_cont);

void write_snow_file(short a, long i, long r, long c, long lmax, FILE *f, long d, long m, long y, long h, long mi, double JDfrom0, 
					 double JDfrom0init, double JDfrom0end, DOUBLEVECTOR *n, DOUBLETENSOR *snowDz, DOUBLETENSOR *var, double cosslope);

void write_soil_header(FILE *f, DOUBLEVECTOR *n, double *dz);

void write_snow_header(short a, long r, long c, FILE *f, DOUBLEVECTOR *n, DOUBLETENSOR *Dz);

void plot(char *name, long i_plot, DOUBLEVECTOR *V, short format, long **J);

//double interpolate_soil(long lmin, double h, long max, double *Dz, double *Q);

double interpolate_soil(long lmin, double h, long max, double *Dz, double *Q);

void from_real_to_default(DOUBLEMATRIX *default_depths, long max, double *Dz, double *real_depths, double ice_density, double **excess_ice, long i, long iplot);

void from_default_to_real(double *real_depths, long n, long max, double *Dz, double *default_depths, double ice_density, double **excess_ice, long i);

double interpolate_soil2(long lmin, double h, long max, double *Dz, DOUBLEMATRIX *Q, long iplot);

void write_tensorseries_soil(long lmin, char *suf, char *filename, short type, short format, DOUBLEMATRIX *T, DOUBLEVECTOR *n, long **J, 
							 LONGMATRIX *RC, double *dz, DOUBLEMATRIX *slope, short vertical);

void fill_output_vectors(double Dt, double W, ENERGY *egy, SNOW *snow, GLACIER *glac, WATER *wat, METEO *met, PAR *par, TIMES *time, TOPO *top,
						 SOIL *sl);

void print_run_average(SOIL *sl, TOPO *top, PAR *par);

void init_run(SOIL *sl, PAR *par);

void end_period_1D(SOIL *sl, TOPO *top, PAR *par);

void change_grid(long previous_sim, long next_sim, PAR *par, TOPO *top, LAND *land, WATER *wat, CHANNEL *cnet);
