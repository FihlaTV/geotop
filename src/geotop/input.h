
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
    
    
typedef struct {
	double swe0;
	double Tsnow0;
	double agesnow0;
    double rhosnow0;	
	double rhoglac0;
	double Dglac0;
	double Tglac0;
	char **met_col_names;
	char **soil_col_names;
	char **horizon_col_names;
	char **point_col_names;
	char **lapserates_col_names;
	char **meteostations_col_names;
	DOUBLEMATRIX *bed;
	DOUBLETENSOR *pa_bed;
	DOUBLEVECTOR *init_water_table_depth;
} INIT_TOOLS;


void get_all_input(long argc, char *argv[], TOPO *top, SOIL *sl, LAND *land, METEO *met, WATER *wat, CHANNEL *cnet, 
					PAR *par, ENERGY *egy, SNOW *snow, GLACIER *glac, TIMES *times, TRANSPORT *transport);	//transport by Flo

void read_inputmaps(TOPO *top, LAND *land, SOIL *sl, PAR *par, INIT_TOOLS *IT, FILE *flog);

void read_optionsfile_point(PAR *par, TOPO *top, LAND *land, SOIL *sl, TIMES *times, INIT_TOOLS *IT, FILE *flog);

void set_bedrock(INIT_TOOLS *IT, SOIL *sl, CHANNEL *cnet, PAR *par, TOPO *top, DOUBLEMATRIX *LC, FILE *flog);

DOUBLETENSOR *find_Z_of_any_layer(DOUBLEMATRIX *Zsurface, DOUBLEMATRIX *slope, DOUBLEMATRIX *LC, SOIL *sl, short point);

short file_exists(short key, FILE *flog);

double peat_thickness(double dist_from_channel);

void initialize_soil_state(SOIL_STATE *S, long n, long nl);

void copy_soil_state(SOIL_STATE *from, SOIL_STATE *to);

void initialize_veg_state(STATE_VEG *V, long n);

void copy_veg_state(STATE_VEG *from, STATE_VEG *to);


