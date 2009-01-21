
/* STATEMENT:

GEO_TOP MODELS THE ENERGY AND WATER FLUXES AT LAND SURFACE
GEOtop-Version 0.9375-Subversion KMackenzie

Copyright, 2008 Stefano Endrizzi, Emanuele Cordano, Riccardo Rigon, Matteo Dall'Amico

 LICENSE:

 This file is part of GEOtop 0.9375 KMackenzie.
 GEOtop is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GEOtop is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#include "constant.h"
#include "keywords_file.h"
#include "struct.geotop.09375.h"
#include "liston.h"
#include "energy.balance.h"
#include "write_dem.h"
#include "t_datamanipulation.h"
#include "t_utilities.h"
#include "meteo.09375.h"
#include "snow.09375.h"
#include "pedo.funct.h"
#include "util_math.h"
#include "rw_maps.h"
#include "times.h"
#include "tabs.h"
#include "output.09375.h"
#include "extensions.h"

void checkErrorSize(char *errfilepath);

extern T_INIT *UV;
extern char *WORKING_DIRECTORY;
extern STRINGBIN *files;
extern long Nl, Nr, Nc;
extern double NoV;

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void energy_balance(TIMES *times, PAR *par,	LAND *land, TOPO *top, SOIL *sl, METEO *met, WATER *wat, ENERGY *egy, SNOW *snow, GLACIER *glac, LISTON *liston)

{

//1.DICHIARAZIONE DELLE VARIABILI
//*******************************

//Counters of rows,coulumns,layers of the basin
long r,c,l,j;

//Number of rows,coulumns,layers of the soil in the whole basin, Maximum number of snow layers, Maximum number of glacier layers
long Ns,Ng;

//Number of snow layer in a pixel, number of glacier layer in a pixel
long ns,ng=0;

//soil-earth correction [-]
double E0;

//SURFACE TEMPERATURE
double Ts;

//Snow depth [mm] and adimensional snow age [] for a pixel, snow density [kg/m3]
double snowD, tausn=0.0, glacD=0.0;

//Precipitation as rain [mm] and as snow [mm water equivalent]
double Prain_over=0.0, Psnow_over=0.0;	//above canopy
double Psoil, Prain=0.0, Prain_on_snow, Psnow=0.0;	//below canopy

//Air density [kg/mc]
double rho_air;

//Short wave radiation [W/mq], cosine of the incidence angle [-], albedo [-] for a pixel
double SWin, SWout, SWbeam=0.0, SWdiff=0.0, cosinc=0.0, albedo_value=0.0;

//Atmospheric absorbance for long wave radiation [-], long wave radiation diffused towards land [W/m2] for a pixel
double epsa=0.0, LWin, LWout, dLWout_dT, Tsurr;

//Sensible heat fluxes [W/m2] and its derivative respect to surface temperature for a pixel
double H, dH_dT, LE, dLE_dT, r_v;

//Evaporation fluxes [kg/(s*m2)]
double E, dE_dT, Epc=0.0, dEpc_dT=0.0, fec=0.0, ftc=0.0, Rge;

//Net surface energy balance[W/m2] and its derivative respect to surface temperature (SW radiation is not included in surfEB, since it is in SWabsf:short wave radiation absorption fraction)
double surfEB, dsurfEB_dT;

//Surface Soil heat flux[W/m2]
double G;

//Adimensional water content
DOUBLEVECTOR *theta;

//Thermal capacity [J m^-3 K^-1], thermal conductivity [W m^-1 K^-1] for a snow/soil layer, and thermal conductivity [W m^-1 K^-1] at the interface between 2 layers (for a pixel).
DOUBLEVECTOR *c_heat, *k_thermal, *k_thermal_interface;
//The 1st component is for the highest layer (be it of snow or soil), the (nl+ns)th component for the deepest soil layer

//Snow/soil layer thickness [m], liquid water content [kg/mq], ice content [kg/mq], temperature [Celsius], melting ice (if>0) or icing water (if<0) [kg/mq] in Dt
DOUBLEVECTOR *D, *wliq, *wice, *Temp, *deltaw;
//The 1st component is for the highest layer (be it of snow or soil), the (nl+ns)th component for the deepest soil layer

//Other auxilairy variables
double z0, d0, z0_z0t, eps, ee=0.0, dee=0.0, Tdew=0.0, Qa, evap_c, drip_c, maxstorage_c, LEs, LEc, dLEs_dT, dLEc_dT;
double epsa_min, epsa_max, tau_atm, RHpoint, Vpoint;
DOUBLEVECTOR *turbulence;
DOUBLEVECTOR *ftcl, *SWabsf, *froot;
double DTcorr=0.0;
double Mr_snow,Er_snow,Sr_snow,Mr_glac,Er_glac,Sr_glac,Er_soil,Sr_soil;
double zmeas_T, zmeas_u;
double fcloud, tau_atm_st, Hadv;
long cont=0;
short sy,lu;

//Cloud variables
static short need_cloud_next_day;
static short check_rain_night;
static double cloud_p_t;
static double cloud_p_v;
static double cloud_n_t;
static double cloud_n_v;

FILE *f;

//ALLOCATION
Ns=par->snowlayer_max; /*maximum number of snow layers*/
Ng=par->glaclayer_max; /*maximum number of glacier layers*/

//The 1st component is for the highest layer (be it of snow or glacier or soil), the (Nl+ns)th component for the deepest soil layer
c_heat=new_doublevector(Nl+Ns+Ng);
k_thermal=new_doublevector(Nl+Ns+Ng);
k_thermal_interface=new_doublevector(Nl+Ns+Ng);
D=new_doublevector(Nl+Ns+Ng);
wliq=new_doublevector(Nl+Ns+Ng);
wice=new_doublevector(Nl+Ns+Ng);
Temp=new_doublevector(Nl+Ns+Ng);
deltaw=new_doublevector(Nl+Ns+Ng);
SWabsf=new_doublevector(Ns+1);
theta=new_doublevector(Nl);
ftcl=new_doublevector(Nl);
froot=new_doublevector(Nl);
turbulence=new_doublevector(11);

initialize_doublematrix(snow->rho_newsnow,UV->V->co[2]);
snow->melted_basin=0.0; snow->evap_basin=0.0; snow->subl_basin=0.0; glac->melted_basin=0.0; glac->evap_basin=0.0; glac->subl_basin=0.0;
initialize_doublevector(snow->CR1,0.0); initialize_doublevector(snow->CR2,0.0); initialize_doublevector(snow->CR3,0.0);

//SHADOW & CLOUDINESS
sun((double)(times->hh+times->mm/60.0), times->JD, &(egy->hsun), &(egy->dsun), &E0, par->latitude, par->longitude, par->ST);

//shadows calculated only when shortwave is not calculated with micromet
if(par->micromet2!=1) shadow_n(par->point_sim, top, egy->hsun, egy->dsun, land->shadow);

//cloudiness calculated only when longwave is not calculated with micromet
if( (par->micromet2!=1 || par->micromet3!=1) && (met->column[met->nstsrad-1][iSW]!=-1 || (met->column[met->nstsrad-1][iSWb]!=-1 && met->column[met->nstsrad-1][iSWd]!=-1)) ){

	if(times->time==0){
		printf("Warning: Cloudiness data are not provided, but cloudiness is inferred from shortwave radiation\n");
		f=fopen(files->co[ferr]+1,"a");
		fprintf(f,"Warning: Cloudiness data are not provided, but cloudiness is inferred from shortwave radiation\n");
		fclose(f);

		f=t_fopen(join_strings(files->co[I_METEO_CLOUDCOVER]+1,textfile),"w");
		//fprintf(f,"Daily cloud cover inferred by shortwave radiation measurements\n");
		fprintf(f,"time;date;alpha;direction;sin(alpha);SWmeas;SWmax;kd;tau_atm;tau_cloud;fcloud;SWcorr;\n");
		t_fclose(f);

		cloud_p_t=0.0;
		cloud_p_v=0.5;
		need_cloud_next_day=1;

		if(met->column[met->nstsrad-1][iRh]==-1) printf("WARNING: No RH data for the radiation station\n");
		if(met->column[met->nstsrad-1][iT]==-1) printf("WARNING: No T data for the radiation station\n");
		if(met->column[met->nstsrad-1][iPt]==-1) printf("WARNING: No precipitation data for the radiation station\n");

	}

	cloudiness(met->nstsrad, met, Asurr, egy->hsun, egy->dsun, E0, &tau_atm_st, &need_cloud_next_day, &cloud_p_v, &cloud_p_t, &cloud_n_v, &cloud_n_t, &check_rain_night, times, par, &fcloud);

	if(met->column[met->nstcloud-1][iC]!=-1){
		fcloud=met->var[met->nstcloud-1][met->column[met->nstcloud-1][iC]];
		if(fcloud>1) fcloud=1.0;
	}

}else if( (par->micromet2!=1 || par->micromet3!=1) && (met->column[met->nstcloud-1][iC]!=-1) ){

	fcloud=met->var[met->nstcloud-1][met->column[met->nstcloud-1][iC]];
	if(fcloud>1) fcloud=1.0;


}else{

	if(times->time==0){
		printf("Warning: Cloudiness data are not provided, neither are shortwave radiation data. Cloud fraction always set at 0.5!!!!\n");
		f=fopen(files->co[ferr]+1,"a");
		fprintf(f,"Warning: Cloudiness data are not provided, neither are shortwave radiation data. Cloud fraction always set at 0.5!!!!\n");
		fclose(f);
	}

	fcloud=0.5;	//default value in case no data of cloudiness are available

}


//COMPUTATIONS FOR EACH CELL
for(r=1;r<=Nr;r++){
	for(c=1;c<=Nc;c++){

        if(top->Z0->co[r][c]!=NoV){//if the pixel is not a novalue

			//value used to calculate soil characteristics
			sy=sl->type->co[r][c];
			lu=land->use->co[r][c];

			//INITIALIZATION
			//Sr=sublimation rate Er=evaporation rate Mr=melting rate [mm/s]
			Sr_snow=0.0; Er_snow=0.0; Mr_snow=0.0; Sr_glac=0.0;	Er_glac=0.0; Mr_glac=0.0; Er_soil=0.0; Sr_soil=0.0;
			initialize_doublevector(deltaw,0.0);

			//initial condition
			if(times->time==0.0){

				if(par->recover!=1){
					//snow layer resetting
					snow_layer_combination(r, c, snow, met->Tgrid->co[r][c], par->snowlayer_inf, par->Dmin, par->Dmax, times->time);

					//initial non-dimensional snow age
					non_dimensionalize_snowage(&(snow->age->co[r][c]), met->Tgrid->co[r][c]);

					//glacier
					if(par->glaclayer_max>0) glacier_init_t0(r, c, met->Tgrid->co[r][c], glac, snow, par, times->time);
				}

				for(j=1;j<=par->rc->nrh;j++){
					if(r==par->rc->co[j][1] && c==par->rc->co[j][2])
						write_soil_output(0, j, times->time, 0.0, par->year0, par->JD0, par->rc, sl, par->psimin, par->Esoil);
				}
			}

			//snow
			ns=snow->lnum->co[r][c];
			snowD=DEPTH(r, c, snow->lnum, snow->Dzl);
			if(snowD!=snowD){
				printf("Novalue in egy balance(a): r:%ld c:%ld SnowD:%f lnum:%ld\n",r,c,DEPTH(r, c, snow->lnum, snow->Dzl),snow->lnum->co[r][c]);
				write_snow_all(r, c, snow);
			}

			//calculates theta from psi(state variable)
			for(l=1;l<=Nl;l++){
				theta->co[l]=teta_psi(sl->P->co[l][r][c], sl->thice->co[l][r][c], sl->pa->co[sy][jsat][l], sl->pa->co[sy][jres][l], sl->pa->co[sy][ja][l],
					sl->pa->co[sy][jns][l], 1-1/sl->pa->co[sy][jns][l], fmin(sl->pa->co[sy][jpsimin][l], Psif(sl->T->co[l][r][c], par->psimin)), par->Esoil);
			}

			//glacier
			if(par->glaclayer_max>0){
				ng=glac->lnum->co[r][c];
				glacD=DEPTH(r, c, glac->lnum, glac->Dzl);
			}

			//METEOROLOGICAL COMPUTATIONS

			//RH and V
			if(par->micromet1==1){
				RHpoint=met->RHgrid->co[r][c];
				Vpoint=met->Vgrid->co[r][c];
			}else{
				RHpoint=met->RH;
				Vpoint=met->V;
			}

			if(Vpoint<par->Vmin) Vpoint=par->Vmin;
			if(RHpoint<0.01*par->RHmin) RHpoint=0.01*par->RHmin;

			//Skin temperature
			Ts=surface(r, c, ns, ng, snow->T, glac->T, sl->T);

			//RAIN AND SNOW PRECIPITATION [mm]
			//calculate dew temperature (otherwise replace Tdew with met->Tgrid) to distinguish between rain and snow
			sat_vap_pressure(&ee,&dee,met->Tgrid->co[r][c],met->Pgrid->co[r][c]);
			ee=RHpoint*0.622*ee/(met->Pgrid->co[r][c]-0.378*ee);
			ee=ee*met->Pgrid->co[r][c]/(0.622+ee*0.378);
			sat_vap_pressure_inv(&Tdew,ee,met->Pgrid->co[r][c]);
			//convert total precipitation to [mm]
			wat->total->co[r][c]*=(par->Dt/3600.0);	//from [mm/h] to [mm]
			//distinguish between rain and snow
			part_snow(wat->total->co[r][c], &Prain_over, &Psnow_over, Tdew, par->T_rain, par->T_snow);
			//modify rain and snow using correction factors
			Prain_over*=par->raincorrfact;
			Psnow_over*=par->snowcorrfact;
			wat->total->co[r][c]=Prain_over+Psnow_over;
			//precipitation reaching sl (in the fraction of the grid surface not covered by canopy)
			Psoil=(1.0-land->ty->co[lu][jfc])*wat->total->co[r][c];	//below canopy

			//INCIDENT SHORTWAVE RADIATION
			if(par->micromet2==1){
				SWbeam=UV->V->co[2];
				SWdiff=UV->V->co[2];
				SWin=egy->SWin->co[r][c];
				cosinc=cos(top->slopes->co[r][c])*sin(egy->hsun)+sin(top->slopes->co[r][c])*cos(egy->hsun)*cos(-top->aspect->co[r][c]+egy->dsun);
			}else{
				tau_atm=atm_transmittance(egy->hsun, met->Pgrid->co[r][c], RHpoint, met->Tgrid->co[r][c], Asurr, par->Vis, par->Lozone);
				shortwave_radiation(r, c, egy->hsun, egy->dsun, E0, land->shadow->co[r][c], top->sky->co[r][c], fcloud, top->slopes->co[r][c],
									top->aspect->co[r][c], tau_atm, met->var[met->nstsrad-1], met->column[met->nstsrad-1], met->st->sky->co[met->nstsrad], tau_atm_st, Asurr,
									&SWbeam, &SWdiff, &cosinc, egy->nDt_shadow, egy->nDt_sun);
				SWin=SWbeam+SWdiff;
			}

			//ALBEDO
			tausn=snow->age->co[r][c];
			albedo(Ts, cosinc, Psnow_over, land->ty->co[lu][jalbedo], snowD, &albedo_value, &tausn, top->pixel_type->co[r][c], egy->hsun, par->aep, par->avo, par->airo,
					par->Dt, fcloud);


			//LONGWAVE RADIATION
			//soil-snow emissivity
			if(snowD>10){
				eps=par->epsilon_snow;
			}else{
				eps=land->ty->co[lu][jem];
			}
			if(par->micromet3==1){
				epsa=UV->V->co[2];
				epsa_max=UV->V->co[2];
				epsa_min=UV->V->co[2];
				LWin=egy->LWin->co[r][c];
			}else{
				longwave_radiation(par->state_lwrad, ee, met->Tgrid->co[r][c], fcloud, &epsa, &epsa_max, &epsa_min);
				//from Halberstam and Schieldge (1981)
				/*if( (par->meteo_stations->co[1][8]-1.0E-3*snowD)/stab2>5 ) epsa=0.95;
				if( (par->meteo_stations->co[1][8]-1.0E-3*snowD)/stab2>10 ) epsa=0.99;*/
				Tsurr=Ts;	//Temperature of surrounding surfaces
				LWin=top->sky->co[r][c]*epsa*SB(met->Tgrid->co[r][c])+(1-top->sky->co[r][c])*eps*SB(Tsurr);
			}

			//TURBULENT HEAT FLUXES
			if(snowD>land->ty->co[lu][jz0thres]){
				z0=par->z0_snow;
				d0=0.0;
				z0_z0t=0.0;
			}else{
				z0=land->ty->co[lu][jz0];
				d0=land->ty->co[lu][jd0];
				z0_z0t=land->ty->co[lu][jz0zt];
			}

			//correction of measured temperature (Arck and Scherer, 2001)
			/*for(l=-15;l<=0;l++){
				if(Vpoint<=0.95){
					DTcorr=0.0007*(albedo_value*SWin)+(1-0.127*(0.5*pow(Vpoint/0.95,2.0)))*DTcorr;
				}else{
					DTcorr=0.0007*(albedo_value*SWin)+(1-0.127*(2*pow(Vpoint/0.95,0.5)-1.5))*DTcorr;
				}
			}
			if(DTcorr<0) DTcorr=0.0;
			met->Tgrid->co[r][c]-=DTcorr;*/

			//temperature and wind velocity measurement heights
			zmeas_u=met->st->Vheight->co[1]-1.0E-3*snowD;
			zmeas_T=met->st->Theight->co[1]-1.0E-3*snowD;
			//zmeas_u=met->st->Vheight->co[1];
			//zmeas_T=met->st->Theight->co[1];
			if(zmeas_u<0.1) zmeas_u=0.1;
			if(zmeas_T<0.1) zmeas_T=0.1;

			turbulent_fluxes(r, c, zmeas_u, zmeas_T,  z0, d0, z0_z0t, Vpoint, met->Pgrid->co[r][c], RHpoint, met->LapseRate, met->Tgrid->co[r][c],
							 Ts, &H, &E, &dH_dT, &dE_dT, &r_v, &rho_air, &Qa, turbulence, par);

			//EVAPOTRANSPIRATION
			evaporation_soil(fmax(ns,ng), E, dE_dT, theta->co[1], sl->pa->co[sy][jres][1], sl->pa->co[sy][jsat][1], r_v, Ts, &Rge, &LEs, &dLEs_dT, &evap_c, &drip_c, &maxstorage_c,
				&Epc, &dEpc_dT, &fec, &ftc, ftcl, &LEc, &dLEc_dT);

			if(land->ty->co[lu][jfc]>0){
				root(land->ty->co[lu][jroot], sl->pa->co[sy][jdz], froot->co);
				evaporation_canopy(r, c, wat->total->co[r][c], theta->co, r_v, rho_air, met->Tgrid->co[r][c], met->Tgrid->co[r][c], Qa, met->Pgrid->co[r][c], SWin,
					land->LAI->co[sy], land->ty->co[lu], froot->co, &(wat->wt->co[r][c]), &LEc, &dLEc_dT, &evap_c, &drip_c, &maxstorage_c, &Epc, &dEpc_dT, &fec, &ftc,
					ftcl->co, par);
					//precipitation reaching soil(in the fraction of the grid surface covered by canopy)
					Psoil+=land->ty->co[lu][jfc]*drip_c;
			}

			//latent heat flux
			LE=(1-land->ty->co[lu][jfc])*LEs + land->ty->co[lu][jfc]*LEc;
			dLE_dT=(1-land->ty->co[lu][jfc])*dLEs_dT + land->ty->co[lu][jfc]*dLEc_dT;

			//rough partition of precipitation reaching the sl (below canopy) in rain and snow
			part_snow(Psoil, &Prain, &Psnow, Tdew, par->T_rain, par->T_snow);
			wat->Psnow->co[r][c]=Psnow;

			//SOIL AND SNOW PROPERTIES
			soil_properties(r, c, ns+ng+1, ns+ng+Nl, theta->co, sl->thice->co, D->co, wliq->co, wice->co, k_thermal->co, c_heat->co,
							Temp->co, sl);

			if(ns>0) snow_properties(r, c, 1, ns, D->co, wliq->co, wice->co, k_thermal->co, c_heat->co, Temp->co, snow->Dzl->co, snow->w_liq->co,
							snow->w_ice->co, snow->T->co, (*k_thermal_snow_Sturm));

			if(ng>0) snow_properties(r, c, ns+1, ns+ng, D->co, wliq->co, wice->co, k_thermal->co, c_heat->co, Temp->co, glac->Dzl->co, glac->w_liq->co,
							glac->w_ice->co, glac->T->co, (*k_thermal_snow_Yen));

			k_interface(ns+ng+Nl, k_thermal->co, D->co, k_thermal_interface->co);

			//ENERGY FLUXES AT THE ATMOSPHERE INTERFACE

			H=flux(1, iH, met->column, met->var, 1.0, H);
			dH_dT=flux(1, iH, met->column, met->var, 0.0, dH_dT);

			LE=flux(1, iLE, met->column, met->var, 1.0, LE);
			dLE_dT=flux(1, iLE, met->column, met->var, 0.0, dLE_dT);

			SWin=flux(1, iSWi, met->column, met->var, 1.0, SWin);
			SWout=flux(1, iSWo, met->column, met->var, 1.0, SWin*albedo_value);

			LWin=flux(met->nstlrad, iLWi, met->column, met->var, 1.0, LWin);
			LWout=flux(1, iLWo, met->column, met->var, 1.0, eps*SB(Ts));
			dLWout_dT=flux(1, iLWo, met->column, met->var, 0.0, eps*dSB_dT(Ts));

			//Extinction coefficient for SW in the snow layers
			rad_snow_absorption(r, c, SWabsf, SWin-SWout, snow);

			//surface egy flux (shortwave radiation not included)
			surfEB=eps*LWin-LWout - H - LE;
			dsurfEB_dT=-dLWout_dT -dH_dT -dLE_dT;

			//Advection
			Hadv=0.0;
			if(snow->Dzl->co[1][r][c]>0 || (snow->Dzl->co[1][r][c]==0 && (egy->Hgrid->co[r][c]<=0 || egy->Tsgrid->co[r][c]<=Tfreezing))){
				if(egy->VSFA!=1.0) Hadv=egy->HSFA*egy->VSFA*adv_efficiency(egy->VSFA)/(1.0-egy->VSFA);
				if(Hadv>egy->HSFA) Hadv=egy->HSFA;
				cont++;
			}
			surfEB+=Hadv;

			//Find errors
			if(dsurfEB_dT!=dsurfEB_dT){
				printf("NOvalue in egy balance1 : r:%ld c:%ld surfEB:%f dsurfEB_dT:%f dLWout_dT:%f dH_dT:%f dLE_dT:%f\n",r,c,surfEB,dsurfEB_dT,dLWout_dT,dH_dT,dLE_dT);
				stop_execution();
			}

			if(surfEB!=surfEB){
				printf("NOvalue in egy balance2 : r:%ld c:%ld surfEB:%f SWin:%f SWout:%f albedo:%f Ts:%f LWin:%f eps:%f LWout:%f H:%f LE:%f LEs:%f LEc:%f\n",r,c,surfEB,SWin,SWout,albedo_value,Ts,LWin,eps,LWout,H,LE,LEs,LEc);
				printf("SW(1-a):%f SWabs:%f\n",surfEB,SWabsf->co[1]);
				write_snow_all(r,c,snow);
				stop_execution();
			}

			if(fabs(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)))>1500){
				f=fopen(files->co[ferr]+1,"a");
				fprintf(f,"\ntime=%10.1f r=%4ld c=%4ld Rn=%10.3f Rsw=%10.3f Rlwdiff=%10.3f albedo=%10.8f eps=%10.8fTa=%10.5f Ts=%10.5f Rsw_meas=%f sin(alpha)=%f cos(inc)=%f\n",
					times->time,r,c,SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)),SWin,LWin,albedo_value,
					eps,met->Tgrid->co[r][c],Ts,met->var[0][iSW],sin(egy->hsun),cosinc);
				printf("ERROR: r:%ld c:%ld surfEB:%f SWin:%f SWout:%f albedo:%f Ts:%f LWin:%f eps:%f LWout:%f H:%f LE:%f LEs:%f LEc:%f\n",r,c,surfEB,SWin,SWout,albedo_value,Ts,LWin,eps,LWout,H,LE,LEs,LEc);
				printf("ERROR: r:%ld c:%ld surfEB:%f dsurfEB_dT:%f dLWout_dT:%f dH_dT:%f dLE_dT:%f\n",r,c,surfEB,dsurfEB_dT,dLWout_dT,dH_dT,dLE_dT);
				write_snow_all(r,c,snow);
				//stop_execution();
			}

			//Too shallow snow layers can give numerical instabilities. Just get rid of them  at the beginning if there is enough energy
			if(snow->type->co[r][c]==1){
				if(surfEB+SWabsf->co[1]>Lf*wice->co[1]){

					Mr_snow=wice->co[1]/par->Dt; //[mm/h]
					snow->w_ice->co[1][r][c]=0.0;
					snow->w_liq->co[1][r][c]=0.0;
					snow->T->co[1][r][c]=-99.0;
					snow->Dzl->co[1][r][c]=0.0;
					snow->lnum->co[r][c]=0;
					snow->type->co[r][c]=0;

					SWabsf->co[1]-=Lf*wice->co[1];
					for(l=2;l<=ns+ng+Nl;l++){
						k_thermal_interface->co[l-1]=k_thermal_interface->co[l];
						c_heat->co[l-1]=c_heat->co[l];
						D->co[l-1]=D->co[l];
						Temp->co[l-1]=Temp->co[l];
						wice->co[l-1]=wice->co[l];
						wliq->co[l-1]=wliq->co[l];
					}
					ns=0;
				}
			}

			SolveEB(ns+ng, Nl, par->Dt, surfEB, dsurfEB_dT, SWabsf, ns+1, land->ty->co[lu][jGbottom], k_thermal_interface->co, c_heat->co, D->co,
					Temp->co, deltaw->co, wice->co, wliq->co, snow->type->co[r][c], met->Tgrid->co[r][c], sl, r, c, times->time, par);

			//THERMAL FLUX UPDATE
			H+=dH_dT*(Temp->co[1]-Ts);
			LE+=dLE_dT*(Temp->co[1]-Ts);
			E+=dE_dT*(Temp->co[1]-Ts);
			Epc+=dEpc_dT*(Temp->co[1]-Ts);
			LWout=flux(1, iLWo, met->column, met->var, 1.0, eps*SB(Temp->co[1]));
			surfEB=SWin-SWout + eps*LWin-LWout - H - LE + Hadv;

			if(ns==0){
				G=surfEB;
			}else{
				G=k_thermal_interface->co[ns]*(Temp->co[ns]-Temp->co[ns+1])/(0.5*D->co[ns]+0.5*D->co[ns+1]);
			}

			//UPDATE SNOW AND SOIL PROPERTIES
			evaporation(r,c,ns+ng, E, Rge, wice->co, wliq->co, D->co, par->Dt, &Sr_snow, &Er_snow, &Er_soil);
			if(ng>0){
				Sr_glac=Sr_snow;
				Er_glac=Er_snow;
				Sr_snow=0.0;
				Er_snow=0.0;
			}

			update_soil(ng+ns+Nl, r, c, sl, Temp->co, deltaw->co, wice->co, theta->co, land->ty->co[lu][jfc], ftcl->co, Epc, Er_soil, par);

			liqWBsnow(r, c, snow, &Mr_snow, &Prain_on_snow, par, top->slopes->co[r][c], Prain, wice->co, wliq->co, Temp->co);

			snow->rho_newsnow->co[r][c]=rho_newlyfallensnow(Vpoint, met->Tgrid->co[r][c], Tfreezing);

			iceWBsnow(r, c, snow, Psnow, met->Tgrid->co[r][c]);

			if(par->glaclayer_max>0){
				WBglacier(snow->lnum->co[r][c], r, c, glac, &Mr_glac, par, wice->co, wliq->co, Temp->co);
				glac2snow(r, c, snow, glac, par->Dmin, par->Dmax);
				if(par->glaclayer_max>1)glac_layer_combination(r, c, glac, met->Tgrid->co[r][c], Ng, par->Dmin_glac, par->Dmax_glac, times->time);
				ng=glac->lnum->co[r][c];
			}

			snow_layer_combination(r, c, snow, met->Tgrid->co[r][c], par->snowlayer_inf, par->Dmin, par->Dmax, times->time);
			ns=snow->lnum->co[r][c];

			Ts=surface(r, c, ns, ng, snow->T, glac->T, sl->T);
			if(Ts!=Ts){
				printf("NOvalue in egy balance3: r:%ld c:%ld ns:%ld ng:%ld Ts:%f\n",r,c,ns,ng,Ts);
				stop_execution();
			}

			snowD=DEPTH(r, c, snow->lnum, snow->Dzl);
			if(snowD!=snowD){
				printf("Novalue in egy balance(b): r:%ld c:%ld SnowD:%f lnum:%ld\n",r,c,DEPTH(r, c, snow->lnum, snow->Dzl),snow->lnum->co[r][c]);
				write_snow_all(r, c, snow);
			}

			//OUTPUT DATA
			prepare_output(Er_soil, Mr_snow, Er_snow, Sr_snow, Mr_glac, Er_glac, Sr_glac, Prain, Psnow_over, egy, wat, snow, glac, land, top, sl, met, times,
				par, r, c, tausn, albedo_value, LE, surfEB, H, G, Ts, SWin, SWbeam, eps, LWin, egy->hsun, cosinc);

			output_pixel(r, c, Psnow, Prain-Prain_on_snow, Prain_on_snow, Sr_soil, Er_soil, Mr_snow, Er_snow, Sr_snow, Mr_glac, Er_glac, Sr_glac, Epc, fec, ftc, LE, H,
				surfEB, G, Ts, albedo_value, eps, LWin, SWin, LWout, SWout, epsa, epsa_min, epsa_max, SWbeam, SWdiff, DTcorr, Tdew, times->n_pixel, turbulence, par,
				wat, egy, top, met, snow, glac, land, Vpoint, RHpoint, Psnow_over, Prain_over, maxstorage_c, evap_c, drip_c, z0, d0);

			output_basin(Prain, Psnow, Ts, Er_soil, Sr_soil, Epc, fec, ftc, LE, H, surfEB, 0, SWin, LWin, albedo_value, eps, top->sky->co[r][c], par->Dt,
				times->n_basin, wat->out2->co, egy->out2->co);

			if(par->ES_num>0) output_altrank(par->ES_num, Prain, Psnow, Ts, Er_soil, Sr_soil, Epc, fec, ftc, LE, H, surfEB, 0, SWin, LWin, albedo_value, eps,
				top->sky->co[r][c], wat->Pn->co[r][c], wat->Pn->co[r][c]-sl->Jinf->co[r][c], Er_snow, Sr_snow, Mr_snow, Er_glac, Sr_glac, Mr_glac, par->Dt,
				times->n_basin, top->Z0->co[r][c], top->Zmin, top->Zmax, glacD, par->glac_thr, egy->out3->co);

			output_map_plots(r, c, par, times->time, times->n_plot, egy, met, snow, H, LE, SWin, SWout, LWin, LWout, Ts);

			//ERROR WRITING
			if(surfEB!=surfEB){
				f=fopen(files->co[ferr]+1,"a");
				fprintf(f,"\nNOVALUE in ENERGY BALANCE, cell r=%4ld c=%4ld time=%10.3f",r,c,times->time);
				fclose(f);
			}

			egy->Hgrid->co[r][c]=H;
			egy->Tsgrid->co[r][c]=Ts;

		}
	}
}

//CALCULATE ADVECTION PARAMETERS FROM SNOW FREE TO SNOW COVERED AREA
if(fabs(cont/(double)par->total_pixel-(1.0-egy->VSFA))>1.0E-5){
	if(times->time>0) printf("INCONSISTENCY: %f %f\n",cont/(double)par->total_pixel,1.0-egy->VSFA);
}
snow_fluxes_H(snow, egy->Tsgrid, egy->Hgrid, top->Z0, met->Tgrid, times, par, &(egy->VSFA), &(egy->HSFA));

//ACCOUNT FOR SNOW WIND TRANSPORT
if(par->snowtrans==1){
	get_softsnow(snow, top->Z0->co);
	call_SnowTrans(times->time, par->Dt, top->Z0, snow, wat->Psnow, liston);
	set_windtrans_snow(snow, top->Z0->co, met->Tgrid->co, wat->Psnow->co, times->time, par, met);
}

//PREPARE SNOW OUTPUT
output_snow(snow, top->Z0->co, par);

//DEALLOCATION
free_doublevector(c_heat);
free_doublevector(k_thermal);
free_doublevector(k_thermal_interface);
free_doublevector(D);
free_doublevector(wliq);
free_doublevector(wice);
free_doublevector(Temp);
free_doublevector(deltaw);
free_doublevector(theta);
free_doublevector(ftcl);
free_doublevector(turbulence);
free_doublevector(SWabsf);
free_doublevector(froot);

}
//end of "energy_balance" subroutine







/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
// C in [J m^-3 K^-1]

void soil_freezing(long l, long r, long c, DOUBLEVECTOR *e0, DOUBLEVECTOR *e1, DOUBLEVECTOR *dw, DOUBLEVECTOR *T, DOUBLEVECTOR *C, SOIL *sl, double psimin){

	double P,Q,th,th1,thi,thi1,theq,A,Tin,ct,Teq,d,Cs,sat,res,a,n,m,h;
	long ri=0, ci=0;
	short sy=sl->type->co[r][c];
	//FILE *f;

	d=sl->pa->co[sy][jdz][l];
	Cs=sl->pa->co[sy][jct][l];
	sat=sl->pa->co[sy][jsat][l];
	res=sl->pa->co[sy][jres][l];
	a=sl->pa->co[sy][ja][l];
	n=sl->pa->co[sy][jns][l];
	m=1-1/n;

	if( (e0->co[l]>Tfreezing && e1->co[l]<Tfreezing) || e0->co[l]<=Tfreezing){

		A=rho_w*0.001*d;		//kg m^-2
		ct=C->co[l]*0.001*d;	//J K-1 m-2
		Tin=fmin(e0->co[l]-Tfreezing, Tfreezing);
		thi=sl->thice->co[l][r][c];
		P=fmin(sl->P->co[l][r][c], psi_saturation(thi, sat, res, a, n, m));
	    theq=teta_psi(Psif2(Tin), 0.0, sat, res, a, n, m, psimin, 1.0);
		th=teta_psi(P, thi, sat, res, a, n, m, fmin(sl->pa->co[sy][jpsimin][l], Psif(sl->T->co[l][r][c], psimin)), 1.0);

		if(r==ri && c==ci) printf("l:%ld r:%ld c:%ld thi:%f th:%f P:%f \n",l,r,c,thi,th,P);

		//heat gained(+) or lost(-)
		Q=(e1->co[l]-e0->co[l])*ct;							//K * J * m^-2 * K^-1 = J * m^-2

		//if heat lost
		if(Q<=0){

			if(th>=theq){
				if(Lf*(theq-th)*A<Q){						//J kg^-1 kg m^-2 = j m^-2
					dw->co[l]=Q/Lf;							//J m^-2 J^-1 kg
					T->co[l]=e0->co[l];
					if(r==ri && c==ci) printf("l:%ld case1 Q:%f e0:%f e1:%f T:%f dw:%f wliq:%f wice:%f th:%f theq:%f \n",l,Q,e0->co[l],e1->co[l],T->co[l],dw->co[l],th*A,thi*A,th,theq);
				}else{
					//heat to reach the curve from above
					dw->co[l]=(theq-th)*A;	//<0
					if(r==ri && c==ci) printf("case2 :Q:%f dw:%f th:%f theq:%f qadd:%f\n",Q,dw->co[l],th,theq,Lf*dw->co[l]);
					Q-=(Lf*dw->co[l]);
					//move on the curve
					th1=th+dw->co[l]/A;
					thi1=thi-dw->co[l]/A;
					h=internal_energy_soil(th1, thi1, Tin, d, Cs, sat);
					if(r==ri && c==ci) printf("l:%ld case2a) theq:%e thi:%e th:%e h:%e Q:%e dw:%e T:%e\n",l,theq,thi1,th1,h,Q,dw->co[l],Tin);
					from_internal_soil_energy(r, c, l, h+Q, &th1, &thi1, &(T->co[l]), sl->pa->co[sy], psimin);
					dw->co[l]+=(th1-theq)*A;
					if(r==ri && c==ci) printf("l:%ld case2b) thi:%e th:%e T:%e dw:%e theq:%e\n",l,thi1,th1,T->co[l],dw->co[l],teta_psi(Psif2(T->co[l]), 0.0, sat, res, a, n, m, psimin, 1.0));
				}

			}else{
				Teq=P*(g*(tk+Tfreezing))/(1.0E3*Lf);
				if(e0->co[l]+Q/ct >= Teq){
					dw->co[l]=0.0;
					T->co[l]=e1->co[l];
					if(r==ri && c==ci) printf("l:%ld case3 Q:%f e0:%f e1:%f T:%f dw:%f wliq:%f wice:%f th:%f theq:%f \n",l,Q,e0->co[l],e1->co[l],T->co[l],dw->co[l],th*A,thi*A,th,theq);
				}else{
					Q-=ct*(Teq-e0->co[l]);
					//move on the curve
					th1=th;
					thi1=thi;
					h=internal_energy_soil(th1, thi1, Teq, d, Cs, sat);
					if(r==ri && c==ci) printf("l:%ld case4a) theq:%f thi:%f th:%f h:%f Q:%f dw:%f T:%f Teq:%f\n",l,theq,thi1,th1,h,Q,dw->co[l],Tin,Teq);
					from_internal_soil_energy(r, c, l, h+Q, &th1, &thi1, &(T->co[l]), sl->pa->co[sy], psimin);
					dw->co[l]=(th1-th)*A;
					if(r==ri && c==ci) printf("l:%ld case4b) thi:%f th:%f T:%f dw:%f theq:%f\n",l,thi1,th1,T->co[l],dw->co[l],teta_psi(Psif2(T->co[l]), 0.0, sat, res, a, n, m, psimin, 1.0));
				}
			}

		//if heat gained
		}else{

			if(th<=theq){

				//there is not ice enough for th to reach theq
				if(thi<theq-th){
					if(Lf*thi*A<Q){	//all ice is melted
						dw->co[l]=thi*A;
						T->co[l]=e0->co[l]+(Q-Lf*dw->co[l])/ct;
						if(r==ri && c==ci) printf("l:%ld case5 Q:%f e0:%f e1:%f T:%f dw:%f wliq:%f wice:%f th:%f theq:%f \n",l,Q,e0->co[l],e1->co[l],T->co[l],dw->co[l],th*A,thi*A,th,theq);
					}else{	//part of ice is melted
						dw->co[l]=Q/Lf;
						T->co[l]=e0->co[l];
						if(r==ri && c==ci) printf("l:%ld case6 Q:%f e0:%f e1:%f T:%f dw:%f wliq:%f wice:%f th:%f theq:%f \n",l,Q,e0->co[l],e1->co[l],T->co[l],dw->co[l],th*A,thi*A,th,theq);
					}
				//there is ice enough for th to reach theq
				}else{
					if(Lf*(theq-th)*A>Q){	//theq can't be reached
						dw->co[l]=Q/Lf;
						T->co[l]=e0->co[l];
						if(r==ri && c==ci) printf("l:%ld case7 Q:%f e0:%f e1:%f T:%f dw:%f wliq:%f wice:%f th:%f theq:%f \n",l,Q,e0->co[l],e1->co[l],T->co[l],dw->co[l],th*A,thi*A,th,theq);
					}else{	//theq is reached
						dw->co[l]=(theq-th)*A;
						Q-=(Lf*dw->co[l]);
						//move on the curve
						th1=th+dw->co[l]/A;
						thi1=thi-dw->co[l]/A;
						h=internal_energy_soil(th1, thi1, Tin, d, Cs, sat);
						if(r==ri && c==ci) printf("l:%ld case8a) theq:%f thi:%f th:%f h:%f Q:%f dw:%f T:%f\n",l,theq,thi1,th1,h,Q,dw->co[l],Tin);
						from_internal_soil_energy(r, c, l, h+Q, &th1, &thi1, &(T->co[l]), sl->pa->co[sy], psimin);
						dw->co[l]+=(th1-theq)*A;
						if(r==ri && c==ci) printf("l:%ld case8b) thi:%f th:%f T:%f dw:%f theq:%f\n",l,thi1,th1,T->co[l],dw->co[l],teta_psi(Psif2(T->co[l]), 0.0, sat, res, a, n, m, psimin, 1.0));
					}
				}

			}else{
				Teq=P*(g*(tk+Tfreezing))/(1.0E3*Lf);
				if(e0->co[l]+Q/ct <= Teq){
					dw->co[l]=0.0;
					T->co[l]=e1->co[l];
					if(r==ri && c==ci)printf("l:%ld case9 Q:%f e0:%f Teq:%f e1:%f T:%f dw:%f wliq:%f wice:%f th:%f theq:%f \n",l,Q,e0->co[l],Teq,e1->co[l],T->co[l],dw->co[l],th*A,thi*A,th,theq);
				}else{
					Q-=ct*(Teq-e0->co[l]);
					//move on the curve
					th1=th;
					thi1=thi;
					h=internal_energy_soil(th1, thi1, Teq, d, Cs, sat);
					if(r==ri && c==ci) printf("l:%ld case10a) theq:%f thi:%f th:%f h:%f Q:%f dw:%f T:%f Teq:%f\n",l,theq,thi1,th1,h,Q,dw->co[l],Tin,Teq);
					from_internal_soil_energy(r, c, l, h+Q, &th1, &thi1, &(T->co[l]), sl->pa->co[sy], psimin);
					dw->co[l]=(th1-th)*A;
					if(r==ri && c==ci) printf("l:%ld case10b) thi:%f th:%f T:%f dw:%f theq:%f\n",l,thi1,th1,T->co[l],dw->co[l],teta_psi(Psif2(T->co[l]), 0.0, sat, res, a, n, m, psimin, 1.0));
				}
			}
		}

		/*if(e1->co[l]<=e0->co[l]){
			if(T->co[l]<e1->co[l]-1.E-5 || T->co[l]>e0->co[l]+1.E-5){
				printf("ERROR 1 in freezing sl r:%ld c:%ld l:%ld e0:%f e1:%f T:%f dw:%f th:%f thi:%f\n",r,c,l,e0->co[l],e1->co[l],T->co[l],dw->co[l],th,thi);
				f=fopen(files->co[ferr]+1,"a");
				fprintf(f,"ERROR 1 in freezing sl r:%ld c:%ld l:%ld e0:%f e1:%f T:%f dw:%f th:%f thi:%f\n",r,c,l,e0->co[l],e1->co[l],T->co[l],dw->co[l],th,thi);
				fclose(f);
				//stop_execution();
			}
		}else{
			if(T->co[l]<e0->co[l]-1.E-5 || T->co[l]>e1->co[l]+1.E-5){
				printf("ERROR 2 in freezing sl r:%ld c:%ld l:%ld e0:%f e1:%f T:%f dw:%f th:%f thi:%f\n",r,c,l,e0->co[l],e1->co[l],T->co[l],dw->co[l],th,thi);
				f=fopen(files->co[ferr]+1,"a");
				fprintf(f,"ERROR 2 in freezing sl r:%ld c:%ld l:%ld e0:%f e1:%f T:%f dw:%f th:%f thi:%f\n",r,c,l,e0->co[l],e1->co[l],T->co[l],dw->co[l],th,thi);
				fclose(f);
				//stop_execution();
			}
		}*/
		if(r==ri && c==ci) printf("\n");

	}else{
		T->co[l]=e1->co[l];
		dw->co[l]=0.0;
	}

}



/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void cloudinessSWglob(double alpha, double E0, double P, double RH, double T, double SW, double A, double sky, double Vis, double L03, double *fcloud, double *tau_atm, double *kd, double *sa, double *tau_cloud){

	double kd0;

	*tau_atm=atm_transmittance(alpha, P, RH, T, A, Vis, L03);
	*kd=0.2;
	*sa=sin(alpha);
	if(*sa<0.05) *sa=0.05;
	if(*sa<0.10) *kd=(*kd*(*sa-0.05)+1.0*(0.10-(*sa)))/(0.10-0.05);

	do{
		kd0=*kd;
		//SW = (1-kd(T))*Isc*T*sin + sky*Kd(T)*Isc*T*sin + (1-sky)*A*Isc*T*sin
		*tau_cloud=(SW/(Isc*E0*(*tau_atm))) / ( (1-(*kd))*(*sa) + sky*(*kd)*(*sa) + (1-sky)*A*(*sa) );
		*kd=diff2glob((*tau_cloud)*(*tau_atm));
		if(*sa<0.10) *kd=(*kd*(*sa-0.05)+1.0*(0.10-(*sa)))/(0.10-0.05);
	}while(fabs(kd0-(*kd))>0.05);

	if(*tau_cloud>=1){
		*fcloud=0.0;
	}else{
		*fcloud=pow((1.0-(*tau_cloud))/0.75,1/3.4);
		if(*fcloud>1) *fcloud=1.0;
	}

}

void cloudinessSWdiff(double alpha, double E0, double P, double RH, double T, double SWd, double SWb, double A, double sky, double Vis, double L03, double *fcloud, double *tau_atm, double *kd, double *sa, double *tau_cloud){

	*sa=sin(alpha);
	if(*sa<0.01) *sa=0.01;
	*tau_atm=atm_transmittance(alpha, P, RH, T, A, Vis, L03);
	if(SWd+SWb>0){
		*kd=SWd/(SWd+SWb);
		*tau_cloud=(SWd/(Isc*E0*(*tau_atm)*(*sa))) / ( sky*(*kd)+(1-sky)*A );
	}else{
		*kd=0.0;
		*tau_cloud=0.0;
	}

	if(*tau_cloud>=1){
		*fcloud=0.0;
	}else{
		*fcloud=pow((1.0-(*tau_cloud))/0.75,1/3.4);
		if(*fcloud>1) *fcloud=1.0;
	}

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void cloudiness(long i, METEO *met, double A, double alpha, double direction, double E0, double *tau_atm, short *need_cloud_next_day,
		double *cloud_p_v, double *cloud_p_t, double *cloud_n_v, double *cloud_n_t, short *check_rain_night, TIMES *times, PAR *par, double *fcloud){

	short shad_st;
	double kd, sa, tau_cloud, t_station, P, RH, T, SW;
	FILE *f;
	double r2d=180.0/Pi;	//from rad to degree

	if(met->column[i-1][iPs]!=-1){
		P=met->var[i-1][iPs];
	}else{
		P=pressure(met->st->Z->co[i], Pa0, 0.0);
	}

	if(met->column[i-1][iRh]!=-1){
		RH=0.01*met->var[i-1][met->column[i-1][iRh]];
	}else{
		RH=0.5;
	}

	if(met->column[i-1][iT]!=-1){
		T=met->var[i-1][met->column[i-1][iT]];
	}else{
		T=0.0;
	}

	shad_st=shadows_point(met->horizon[i-1], alpha*r2d, direction*r2d, 2.0, 5.0);
	time_conversion(par->JD0, par->year0, times->time+0.5*par->Dt, met->st->JD0->co[i], met->st->Y0->co[i], &t_station);
	t_station+=(met->st->ST->co[i]-par->ST)*3600.0;

	if(shad_st==0){	// the met station is NOT in shadow, therefore cloudiness can be trusted

		if(met->column[i-1][iSWb]!=-1 && met->column[i-1][iSWd]!=-1){
			cloudinessSWdiff(alpha, E0, P, RH, T, met->var[i-1][met->column[i-1][iSWd]], met->var[i-1][met->column[i-1][iSWb]],  A, met->st->sky->co[i], par->Vis, par->Lozone, fcloud, tau_atm, &kd, &sa, &tau_cloud);
		}else if(met->column[i-1][iSW]!=-1){
			cloudinessSWglob(alpha, E0, P, RH, T, met->var[i-1][met->column[i-1][iSW]], A, met->st->sky->co[i], par->Vis, par->Lozone, fcloud, tau_atm, &kd, &sa, &tau_cloud);
		}else{
			t_error("Cannot calculate cloudiness, either SWglobal, or both SWdirect and SWdiffuse are required");
		}

		*need_cloud_next_day=1;// during the day we trust the cloudiness and set nee_cloud_next_day=1 so at sunset it calls the routine check_clouds_nextday()
		*check_rain_night=0;

		*cloud_p_v=*fcloud; // record the cloud value
		*cloud_p_t=t_station; // record the time value

	}else{

		if(alpha>0){
			if(met->column[i-1][iSWb]!=-1 && met->column[i-1][iSWd]!=-1){
				cloudinessSWdiff(alpha, E0, P, RH, T, met->var[i-1][met->column[i-1][iSWd]], met->var[i-1][met->column[i-1][iSWb]],  A, met->st->sky->co[i], par->Vis, par->Lozone, fcloud, tau_atm, &kd, &sa, &tau_cloud);
			}else if(met->column[i-1][iSW]!=-1){
				cloudinessSWglob(alpha, E0, P, RH, T, met->var[i-1][met->column[i-1][iSW]], A, met->st->sky->co[i], par->Vis, par->Lozone, fcloud, tau_atm, &kd, &sa, &tau_cloud);
			}else{
				t_error("Cannot calculate cloudiness, either SWglobal, or both SWdirect and SWdiffuse are required");
			}
		}else{
			kd=0.0;sa=0.0;tau_cloud=0.0;*tau_atm=0.0;
		}

		if(*need_cloud_next_day==1)  {// sun near to sunset: go to find the next value of cloud for the next morning
			// function that goes forward in time to find the nearest reliable cloud value and records the respective time and cloud value
			// now we know the cloudiness of the next day, therefore we don't have to do again this loop
			check_clouds_nextday(t_station, i, met->st, met->column, met->data, cloud_n_t, cloud_n_v, check_rain_night, A, met->horizon[i-1], 2.0, 5.0, par); // now we know the cloudiness of the next day, therefore we don't have to do again this loop
			*need_cloud_next_day=0;
		}

		*fcloud=interpolate_function(*cloud_p_t, *cloud_p_v, *cloud_n_t, *cloud_n_v, t_station);
		if(*check_rain_night==1) *fcloud=1.0; // if during the night it has been raining, than the cloudiness is set to the maximum
	}


	if(met->column[i-1][iSWb]!=-1 && met->column[i-1][iSWd]!=-1){
		SW=met->var[i-1][met->column[i-1][iSWd]];
	}else if(met->column[i-1][iSW]!=-1){
		SW=met->var[i-1][met->column[i-1][iSW]];
	}

	f=fopen(join_strings(files->co[I_METEO_CLOUDCOVER]+1,textfile),"a");
	if(times->mm<10){
		fprintf(f,"%f;%ld/%ld/%ld %ld:0%ld;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f\n",
				times->time,times->DD,times->MM,times->AAAA,times->hh,times->mm,alpha*180.0/Pi,direction*180.0/Pi,sa,SW,Isc*E0*sa,kd,*tau_atm,tau_cloud,*fcloud,
				(1.0-0.75*pow(*fcloud,3.4))*(*tau_atm)*Isc*E0*sa);
	}else{
		fprintf(f,"%f;%ld/%ld/%ld %ld:%ld;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f\n",
				times->time,times->DD,times->MM,times->AAAA,times->hh,times->mm,alpha*180.0/Pi,direction*180.0/Pi,sa,SW,Isc*E0*sa,kd,*tau_atm,tau_cloud,*fcloud,
				(1.0-0.75*pow(*fcloud,3.4))*(*tau_atm)*Isc*E0*sa);
	}
	fclose(f);

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/* Authors: Matteo Dall'Amico and Stefano Endrizzi, May 2008
 * function that goes forward in time to find the nearest reliable cloud value and records the respective time and cloud value
 * Input:double time_now: time counter that updates to retrieve the first sunny hour
		 TIMES* times: to have the day and hour
		 PAR* par
		 hor_height: matrix with horizon elevation at the met station
		 DATA_METEOROLOGICAL *data_meteo: met data (radiation, pressure...)
		 double tol_mount: tolerance over a mountaneous horizon to get a reliable radiation
		 double tol_flat: tolerance over a flat horizon to get a reliable radiation

 * Output: double *cloud_n_t: time when the cloud in the next morning is reliable
 * 		 double *cloud_n_v: cloud value at the next morning
 * 		 short *check_rain: 1: during the night there has been precipitation, 0 otherwise
 * */

void check_clouds_nextday(double t, long i, METEO_STATIONS *stm, long **col, double ***data_meteo, double *cloud_n_t, double *cloud_n_v, short *check_rain_night, double A, double **hor_height,
						  double tol_mount, double tol_flat, PAR *par)
{

	double alpha;// sun elevation
	double azimuth;// solar azimuth
	double E0; // Earth-Sun distance
	double tau_atm_st; // trasmittance through the atmosphere
	short shad_st=1; // 1 if the Meteo Station is in shadow, 0 otherwise
	double kd, sa, tau_cloud;

	double time_now=stm->Dt->co[i]*ceil(t/stm->Dt->co[i]); //current time
	long hour, min, day, month, year;
	double JD;

	long n, ndata=dim2(data_meteo[i-1]);// n:index (1 to ndata) of the met data in the meteo_data matrix
	long rain_count=0; // counter of rainy times in the night
	short alarm=0; // 1 if the routine is looking for data beyond the scope of the meteo_data matrix, 0 otherwise
	double r2d=180.0/Pi;	//from rad to degree

	double P, RH, T;

	do {
		time_now+=stm->Dt->co[i];
		n=(long)(time_now/stm->Dt->co[i]);
		if (n >= ndata) alarm=1;
		date_time(time_now-0.5*stm->Dt->co[i], stm->Y0->co[i], stm->JD0->co[i], 0.0, &JD, &day, &month, &year, &hour, &min);
		sun((double)(hour+min/60.0), JD, &alpha, &azimuth, &E0, stm->lat->co[i], stm->lon->co[i], stm->ST->co[i]);
		if (alpha>0) shad_st=shadows_point(hor_height, alpha*r2d, azimuth*r2d, tol_mount, tol_flat);
		// find if there is rain in the night
		if(col[i-1][iPt]!=-1){ if(data_meteo[i-1][n][col[i-1][iPt]]>0.1) rain_count++; }
	} while (shad_st==1 && alarm==0); // do the loop if the point is in shadow; when it is in sun (shad_Mst=0), exit the loop

	if (rain_count!=0) *check_rain_night=1;

	if (alarm==0) { // we didn't go beyond the matrix of meteo_data

		if(col[i-1][iPs]!=-1){
			P=data_meteo[i-1][n][col[i-1][iPs]];
		}else{
			P=pressure(stm->Z->co[i], Pa0, 0.0);
		}

		if(col[i-1][iRh]!=-1){
			RH=0.01*data_meteo[i-1][n][col[i-1][iRh]];
		}else{
			RH=0.5;
		}

		if(col[i-1][iT]!=-1){
			T=data_meteo[i-1][n][col[i-1][iT]];
		}else{
			T=0.0;
		}

		if(col[i-1][iSWb]!=-1 && col[i-1][iSWd]!=-1){
			cloudinessSWdiff(alpha, E0, P, RH, T, data_meteo[i-1][n][col[i-1][iSWd]], data_meteo[i-1][n][col[i-1][iSWb]],  A, stm->sky->co[i], par->Vis, par->Lozone, cloud_n_v, &tau_atm_st, &kd, &sa, &tau_cloud);
		}else if(col[i-1][iSW]!=-1){
			cloudinessSWglob(alpha, E0, P, RH, T, data_meteo[i-1][n][col[i-1][iSW]], A, stm->sky->co[i], par->Vis, par->Lozone, cloud_n_v, &tau_atm_st, &kd, &sa, &tau_cloud);
		}else{
			t_error("Cannot calculate cloudiness, either SWglobal, or both SWdirect and SWdiffuse are required");
		}

		*cloud_n_t=time_now-stm->Dt->co[i]; // record the time value

	}else{

		*cloud_n_v=0.5; // record the cloud value at the intermediate value
		*cloud_n_t=time_now-stm->Dt->co[i]; // record the time value
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double interpolate_function(double x1, double y1, double x2, double y2, double x)
// Author: Matteo Dall'Amico, May 2008

{
	double y;
	y=y1+(y2-y1)/(x2-x1)*(x-x1);
	return (y);

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double k_thermal_snow_Sturm(double density){	//W m^-1 K^-1 (Sturm, 1997)

	double kt;
	density*=0.001;
	if(density<0.156){
		kt=0.023+0.234*density;
	}else{
		kt=0.138-1.01*density+3.233*pow(density,2.0);
	}
	return(kt);
}
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double k_thermal_snow_Yen(double density){	//W m^-1 K^-1 (Yen, 1981)

	double kt;
	kt=k_ice*pow((density/rho_w),1.88);
	return(kt);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double SB(double T){	//Stefan-Boltzmann law
	double R;
	R=5.67E-8*pow(T+tk,4.0);
	return(R);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double dSB_dT(double T){
	double dR_dT;
	dR_dT=4.0*5.67E-8*pow(T+tk,3.0);
	return(dR_dT);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double surface(long r, long c, long ns, long ng, DOUBLETENSOR *snow, DOUBLETENSOR *ice, DOUBLETENSOR *sl){
	double S;
	if(ns>0){
		S=snow->co[ns][r][c];
	}else if(ng>0){
		S=ice->co[ng][r][c];
	}else{
		S=sl->co[1][r][c];
	}
	return(S);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void non_dimensionalize_snowage(double *snowage, double Ta){

	double r1, r2, r3;

	r1=exp(5000.0*(1.0/273.16-1.0/(Ta+273.16)));
	r2=pow(r1,10);
	if(r2>1.0) r2=1.0;
	r3=0.3;

	*snowage*=((r1+r2+r3)*1.0E-6);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void glacier_init_t0(long r, long c, double Ta, GLACIER *glac, SNOW *snow, PAR *par, double time){

	double D, h;
	long l;

	D=DEPTH(r, c, glac->lnum, glac->Dzl);

	//when glacier depth is too low, glacier becomes a snow layer
	if(D>0 && D<par->Dmin_glac->co[1]){
		h=(c_ice*glac->w_ice->co[1][r][c] + c_liq*glac->w_liq->co[1][r][c])*(glac->T->co[1][r][c]-Tfreezing) + Lf*glac->w_liq->co[1][r][c];
		for(l=2;l<=glac->lnum->co[r][c];l++){
			glac->Dzl->co[1][r][c]+=glac->Dzl->co[l][r][c];
			glac->w_liq->co[1][r][c]+=glac->w_liq->co[l][r][c];
			glac->w_ice->co[1][r][c]+=glac->w_ice->co[l][r][c];
			//means that glac->T is not a novalue
			if(glac->T->co[l][r][c]>-98.999) h+=(c_ice*glac->w_ice->co[l][r][c] + c_liq*glac->w_liq->co[l][r][c])*(glac->T->co[l][r][c]-Tfreezing) + Lf*glac->w_liq->co[l][r][c];

			glac->Dzl->co[l][r][c]=0.0;
			glac->w_liq->co[l][r][c]=0.0;
			glac->w_ice->co[l][r][c]=0.0;
			glac->T->co[l][r][c]=-99.0;
		}

		h+=(c_ice*snow->w_ice->co[1][r][c] + c_liq*snow->w_liq->co[1][r][c])*(snow->T->co[1][r][c]-Tfreezing) + Lf*snow->w_liq->co[1][r][c];
		snow->Dzl->co[1][r][c]+=glac->Dzl->co[1][r][c];
		snow->w_ice->co[1][r][c]+=glac->w_ice->co[1][r][c];
		snow->w_liq->co[1][r][c]+=glac->w_liq->co[1][r][c];
		if(h<0){
			snow->T->co[1][r][c]=Tfreezing + h/(c_ice*snow->w_ice->co[1][r][c]+c_liq*snow->w_liq->co[1][r][c]);
		}else{
			snow->T->co[1][r][c]=0.0;
		}

		glac->Dzl->co[1][r][c]=0.0;
		glac->w_liq->co[1][r][c]=0.0;
		glac->w_ice->co[1][r][c]=0.0;
		glac->T->co[1][r][c]=-99.0;
		glac->lnum->co[r][c]=0;

		snow_layer_combination(r, c, snow, Ta, par->snowlayer_inf, par->Dmin, par->Dmax, time);
	}

	//setting
	if(par->glaclayer_max>1)glac_layer_combination(r, c, glac, Ta, par->glaclayer_max, par->Dmin_glac, par->Dmax_glac, time);
}



/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void SolveEB(long n1, long n2, double dt, double h, double dhdT, DOUBLEVECTOR *abs, long nlim, double Gb, double *k, double *C, double *D, double *T, double *dw, double *wi, double *wl,
			 short snowtype, double Ta, SOIL *sl, long r, long c, double t, PAR *par){

	//n1: snow (and glacier)
	//n2: sl
	//n=n1+n2: total

	short occuring, sy=sl->type->co[r][c];
	long l, n=n1+n2;
	double Dcorr;
	SHORTVECTOR *mf;
	DOUBLEVECTOR *ad, *ads, *adi, *b, *ad1, *ads1, *adi1, *b1, *e;
	DOUBLEVECTOR *e0g,*e1g, *dwg, *Tg, *Cg;
	long ri=0,ci=0;
	//FILE *f;

	mf=new_shortvector(n);
	initialize_shortvector(mf,0);

	ad=new_doublevector(n);
	ads=new_doublevector(n-1);
	adi=new_doublevector(n-1);
	b=new_doublevector(n);
	ad1=new_doublevector(n);
	ads1=new_doublevector(n-1);
	adi1=new_doublevector(n-1);
	b1=new_doublevector(n);

	e=new_doublevector(n);

	e0g=new_doublevector(n2);
	e1g=new_doublevector(n2);
	dwg=new_doublevector(n2);
	Tg=new_doublevector(n2);
	Cg=new_doublevector(n2);

	Dcorr=D[1];
	//Dcorr=0.5*(0.5*D->co[1]+CA*(D->co[1]+0.5*D->co[2]));

	ad->co[1]=-dhdT + 2.0*(1-KNe)*k[1]/(D[1]+D[2]) + C[1]*Dcorr/dt;
	ads->co[1]=2.0*(KNe-1)*k[1]/(D[1]+D[2]);
	b->co[1]=C[1]*Dcorr*T[1]/dt + (h+abs->co[1]) + 2.0*KNe*k[1]*(T[2]-T[1])/(D[1]+D[2]) - dhdT*T[1];
	if(r==ri && c==ci) printf("B l:%ld b:%f 1:%f 2:%f 3:%f 4:%f\n",1,b->co[1],C[1]*Dcorr*T[1]/dt,(h+abs->co[1]),2.0*KNe*k[1]*(T[2]-T[1])/(D[1]+D[2]),-dhdT*T[1]);

	for(l=2;l<=n-1;l++){
		ad->co[l]=C[l]*D[l]/dt + 2.0*(1-KNe)*k[l-1]/(D[l-1]+D[l]) + 2.0*(1-KNe)*k[l]/(D[l]+D[l+1]);
		ads->co[l]=2.0*(KNe-1)*k[l]/(D[l]+D[l+1]);
		adi->co[l-1]=ads->co[l-1];	//la matrice e' simmetrica
		b->co[l]=C[l]*D[l]*T[l]/dt - 2.0*KNe*k[l-1]*(T[l]-T[l-1])/(D[l-1]+D[l]) + 2.0*KNe*k[l]*(T[l+1]-T[l])/(D[l]+D[l+1]);
		if(l<=nlim) b->co[l]+=abs->co[l];
		if(r==ri && c==ci) printf("B l:%ld b:%f 1:%f 2:%f 3:%f ",l,b->co[l],C[l]*D[l]*T[l]/dt,-2.0*KNe*k[l-1]*(T[l]-T[l-1])/(D[l-1]+D[l]),2.0*KNe*k[l]*(T[l+1]-T[l])/(D[l]+D[l+1]));
		if(l<=nlim){
			if(r==ri && c==ci) printf("4:%f\n",abs->co[l]);
		}else{
			if(r==ri && c==ci) printf("\n");
		}
	}

	ad->co[n]=C[n]*D[n]/dt + 2.0*(1-KNe)*k[n-1]/(D[n-1]+D[n]);
	adi->co[n-1]=ads->co[n-1];		//la matrice e' simmetrica
	b->co[n]=C[n]*D[n]*T[n]/dt - 2.0*KNe*k[n-1]*(T[n]-T[n-1])/(D[n-1]+D[n]) + Gb;
	if(r==ri && c==ci)printf("B l:%ld b:%f 1:%f 2:%f 3:%f\n",n,b->co[n],C[n]*D[n]*T[n]/dt,-2.0*KNe*k[n-1]*(T[n]-T[n-1])/(D[n-1]+D[n]),Gb);

	tridiag(1,r,c,n,adi,ad,ads,b,e);
	check_errors(r,c,n,adi,ad,ads,b,e,T,mf);

	for(l=1;l<=n;l++){
		if(r==ri && c==ci){
			printf("1. l:%ld h:%f abs:%f sum:%f ad:%f b:%f T:%f dw:%f wliq:%f wice:%f e:%f mf:%ld",l,h,abs->co[1],h+abs->co[1],ad->co[l],b->co[l],T[l],dw[l],wl[l],wi[l],e->co[l],mf->co[l]);
			if(l>=n1+1 && l<=n1+n2) printf(" wice2:%f ",sl->thice->co[l-n1][r][c]*sl->pa->co[sy][jdz][l-n1]);
			printf("\n");
		}
	}

	//nel caso neve semplificato (type=1), se non si verifica scioglimento, non si fa il bilancio di energia!!
	if(snowtype==1 && e->co[1]<Ta-20){
		mf->co[1]=99;		//Codice che memorizza questo passaggio
		ad->co[1]=1.0;
		ads->co[1]=0.0;
		if(Ta-20<Tfreezing){	//Si pone la temperatura della neve pari a quella dell'aria
			b->co[1]=Ta-20;
		}else{
			b->co[1]=Tfreezing;
		}
		tridiag(1,r,c,n,adi,ad,ads,b,e);
		check_errors(r,c,n,adi,ad,ads,b,e,T,mf);
		for(l=1;l<=n;l++){
			if(r==ri && c==ci) printf("1b. l:%ld h:%f sum:%f abs:%f ad:%f b:%f T:%f dw:%f wliq:%f wice:%f e:%f mf:%ld \n",l,h,abs->co[1],h+abs->co[1],ad->co[l],b->co[l],T[l],dw[l],wl[l],wi[l],e->co[l],mf->co[l]);
		}
	}

	//melting e freezing
	//riinizializzazione
	occuring=0;
	for(l=1;l<=n;l++){
		b1->co[l]=b->co[l];
		ad1->co[l]=ad->co[l];
		if(l!=n){
			ads1->co[l]=ads->co[l];
			adi1->co[l]=adi->co[l];
		}
	}

	//snow and glacier
	for(l=1;l<=n1;l++){
		if(mf->co[l]!=99){
			if(e->co[l]>Tfreezing && wi[l]>0.0){
				mf->co[l]=1;//melting
				occuring=1;
			}else if(e->co[l]<Tfreezing && wl[l]>0.0){
				mf->co[l]=2;//freezing
				occuring=1;
			}
		}
	}

	//sl freezing
	for(l=1;l<=n2;l++){
		e0g->co[l]=T[l+n1];
		e1g->co[l]=e->co[l+n1];
		Cg->co[l]=C[l+n1];
		if(sl->pa->co[sy][jsf][l]==1){
			occuring=1;
			soil_freezing(l, r, c, e0g, e1g, dwg, Tg, Cg, sl, par->psimin);
			if(r==ri && c==ci) printf("frez1. l:%ld e0g:%f e1g:%f Tg:%f dwg:%f\n",l,e0g->co[l],e1g->co[l],Tg->co[l],dwg->co[l]);
			if(Tg->co[l]!=Tg->co[l]) printf("NOvalue Tg1: r:%ld c:%ld l:%ld wi:%f wl:%f dw:%f mf:%ld T:%f\n",r,c,l,wi[l],wl[l],dw[l],mf->co[l],T[l]);
		}
	}

	if(occuring==0){	//non devo fare altri calcoli, scrivo i risultati

		for(l=1;l<=n;l++){
			T[l]=e->co[l];
			dw[l]=0.0;
		}

	}else{  //cioe' se accade melting o freezing

		for(l=1;l<=n;l++){

			/*se AVVIENE MELTING O FREEZING, l'equazione cambia*/

			//NEVE O GHIACCIO: l'incognita diventa l'acqua che si scioglie
			if(mf->co[l]==1 || mf->co[l]==2){
				ad1->co[l]=Lf/dt;
				b1->co[l]-=ad->co[l]*Tfreezing;
			}

			/*vengono corretti i termini di un layer se il layer soprastante e' soggetto a melt/freez*/
			if(l!=1){
				if(mf->co[l-1]==1 || mf->co[l-1]==2){
					if(mf->co[l]!=99){
						adi1->co[l-1]=0.0;
						b1->co[l]-=adi->co[l-1]*Tfreezing;
					}
				}
			}

			/*vengono corretti i termini di un layer se il layer sottostante e' soggetto a melt/freez*/
			if(l!=n){
				if(mf->co[l+1]==1 || mf->co[l+1]==2){
					if(mf->co[l]!=99){
						ads1->co[l]=0.0;
						b1->co[l]-=ads->co[l]*Tfreezing;
					}
				}
			}

			/*sl freezing*/
			if(l>n1){
				if(sl->pa->co[sy][jsf][l-n1]==1){
					if(l>1){
						b1->co[l-1]-=ads->co[l-1]*Tg->co[l-n1];
						ads1->co[l-1]=0.0;
						if(r==ri && c==ci) printf("#. l:%ld B:%f B1:%f Tgbelow:%f m:%ld Ads:%f\n",l-1,b->co[l-1],b1->co[l-1],Tg->co[l-n1],l-n1,ads->co[l-1]);
					}
					if(l<n){
						b1->co[l+1]-=adi->co[l]*Tg->co[l-n1];
						adi1->co[l]=0.0;
					}
				}
			}
		}

		tridiag(1,r,c,n,adi1,ad1,ads1,b1,e);
		check_errors(r,c,n,adi1,ad1,ads1,b1,e,T,mf);

		for(l=1;l<=n;l++){
			if(r==ri && c==ci) printf("2. l:%ld h:%f abs:%f sum:%f ad:%f b:%f T:%f dw:%f wliq:%f wice:%f e:%f mf:%ld \n",l,h,abs->co[1],h+abs->co[1],ad1->co[l],b1->co[l],T[l],dw[l],wl[l],wi[l],e->co[l],mf->co[l]);
		}

		//riinizializzazione
		occuring=0;
		for(l=1;l<=n;l++){
			b1->co[l]=b->co[l];
			ad1->co[l]=ad->co[l];
			if(l!=n){
				ads1->co[l]=ads->co[l];
				adi1->co[l]=adi->co[l];
			}
		}

		//snow and glacier
		for(l=1;l<=n1;l++){

			//MELTING
			if(mf->co[l]==1){
				//a. si verifica melting (mf=1), ma e e' negativo (FALSO MELTING)
				/*if(e->co[l]<0.0){
					mf->co[l]=100;
					occuring=1;
				//b. c'e' meno ghiaccio di quanto potrebbe sciogliersi
				}else*/ if(e->co[l]>wi[l]){
					mf->co[l]=10;
					occuring=1;
				}

			//FREEZING (snow and glacier)
			}else if(mf->co[l]==2){
				//a. si verifica freezing (mf=2), ma e e' positivo (FALSO FREEZING)
				/*if(e->co[l]>0.0){
					mf->co[l]=200;
					occuring=1;
				//c'e' meno acqua liquida di quanta ne potrebbe ghiacciare
				}else*/if(-e->co[l]>wl[l]){
					mf->co[l]=20;
					occuring=1;
				}
			}
		}

		//sl freezing
		for(l=1;l<=n2;l++){
			//e0g->co[l]=T[l+n1];
			e1g->co[l]=e->co[l+n1];
			//Cg->co[l]=C[l+n1];
			if(sl->pa->co[sy][jsf][l]==1){
				occuring=1;
				soil_freezing(l, r, c, e0g, e1g, dwg, Tg, Cg, sl, par->psimin);
				if(r==ri && c==ci) printf("frez2. l:%ld e0g:%f e1g:%f Tg:%f dwg:%f\n",l,e0g->co[l],e1g->co[l],Tg->co[l],dwg->co[l]);
			}
		}

		if(occuring==0){	//scrivo i risultati

			for(l=1;l<=n;l++){
				if(mf->co[l]==0 || mf->co[l]==99){
					T[l]=e->co[l];
					dw[l]=0.0;
				}else if(mf->co[l]==1 || mf->co[l]==2){	//melting or freezing
					T[l]=Tfreezing;
					dw[l]=e->co[l];
				}
			}

		}else{		//allora le equazioni cambiano

			for(l=1;l<=n;l++){

				if(mf->co[l]==1 || mf->co[l]==2){
					ad1->co[l]=Lf/dt;
					b1->co[l]-=ad->co[l]*Tfreezing;

				}else if(mf->co[l]==10){
					b1->co[l]-=Lf*wi[l]/dt;

				}else if(mf->co[l]==20){
					b1->co[l]+=Lf*wl[l]/dt;
				}

				/*vengono corretti i termini di un layer se il layer soprastante e' soggetto a melt/freez*/
				if(l!=1){
					if(mf->co[l-1]==1 || mf->co[l-1]==2){
						if(mf->co[l]!=99){
							adi1->co[l-1]=0.0;
							b1->co[l]-=adi->co[l-1]*Tfreezing;
						}
					}
				}

				/*vengono corretti i termini di un layer se il layer sottostante e' soggetto a melt/freez*/
				if(l!=n){
					if(mf->co[l+1]==1 || mf->co[l+1]==2){
						if(mf->co[l]!=99){
							ads1->co[l]=0.0;
							b1->co[l]-=ads->co[l]*Tfreezing;
						}
					}
				}

				/*sl freezing*/
				if(l>n1){
					if(sl->pa->co[sy][jsf][l-n1]==1){
						if(l>1){
							b1->co[l-1]-=ads->co[l-1]*Tg->co[l-n1];
							ads1->co[l-1]=0.0;
							if(r==ri && c==ci) printf("#. l:%ld B:%f B1:%f Tgbelow:%f m:%ld Ads:%f\n",l-1,b->co[l-1],b1->co[l-1],Tg->co[l-n1],l-n1,ads->co[l-1]);
						}
						if(l<n){
							b1->co[l+1]-=adi->co[l]*Tg->co[l-n1];
							adi1->co[l]=0.0;
						}
					}
				}
			}

			tridiag(1,r,c,n,adi1,ad1,ads1,b1,e);
			check_errors(r,c,n,adi1,ad1,ads1,b1,e,T,mf);

			for(l=1;l<=n;l++){
				if(r==ri && c==ci) printf("3. l:%ld h:%f abs:%f sum:%f ad:%f b:%f T:%f dw:%f wliq:%f wice:%f e:%f mf:%ld \n",l,h,abs->co[1],h+abs->co[1],ad1->co[l],b1->co[l],T[l],dw[l],wl[l],wi[l],e->co[l],mf->co[l]);
			}

			occuring=0;
			for(l=1;l<=n;l++){
				if(e->co[l]>Tfreezing && mf->co[l]==10){
					occuring=1;
					mf->co[l]=98;
					ad1->co[l]=1.0;
					b->co[l]=Tfreezing;
					if(l!=1){
						adi1->co[l-1]=0.0;
						if(mf->co[l-1]!=99){
							b1->co[l-1]-=ads->co[l-1]*Tfreezing;
							ads1->co[l-1]=0.0;
						}
					}
					if(l!=n){
						ads1->co[l]=0.0;
						if(mf->co[l+1]!=99){
							b1->co[l+1]-=adi->co[l]*Tfreezing;
							adi1->co[l]=0.0;
						}
					}
				}
			}

			if(occuring==1){
				tridiag(1,r,c,n,adi1,ad1,ads1,b,e);
				check_errors(r,c,n,adi1,ad1,ads1,b,e,T,mf);
				for(l=1;l<=n;l++){
					if(r==ri && c==ci)printf("3b. l:%ld h:%f abs:%f sum:%f ad:%f b:%f T:%f dw:%f wliq:%f wice:%f e:%f mf:%ld\n ",l,h,abs->co[1],h+abs->co[1],ad1->co[l],b1->co[l],T[l],dw[l],wl[l],wi[l],e->co[l],mf->co[l]);
				}
			}

			//scrivo i risultati

			//snow and glacier
			for(l=1;l<=n1;l++){
				if(mf->co[l]==0 || mf->co[l]==99){
					T[l]=e->co[l];
					dw[l]=0.0;
				}else if(mf->co[l]==1){
					T[l]=Tfreezing;
					dw[l]=e->co[l];
				}else if(mf->co[l]==2){
					T[l]=Tfreezing;
					dw[l]=e->co[l];
				}else if(mf->co[l]==10 || mf->co[l]==98){
					T[l]=e->co[l];
					dw[l]=wi[l];
				}else if(mf->co[l]==20){
					T[l]=e->co[l];
					dw[l]=-wl[l];
				}else if(mf->co[l]==100){	//FALSO MELTING
					T[l]=e->co[l];
					dw[l]=0.0;
				}else if(mf->co[l]==200){	//FALSO FREEZING
					T[l]=e->co[l];
					dw[l]=0.0;
				}
				if(wl[l]!=wl[l]) printf("NOvalue wl[i]snow: r:%ld c:%ld l:%ld wi:%f wl:%f dw:%f mf:%ld T:%f\n",r,c,l,wi[l],wl[l],dw[l],mf->co[l],T[l]);
			}

			//sl freezing
			for(l=1;l<=n2;l++){
				//e0g->co[l]=T[l+n1];
				e1g->co[l]=e->co[l+n1];
				//Cg->co[l]=C[l+n1];
				if(sl->pa->co[sy][jsf][l]==1){
					soil_freezing(l, r, c, e0g, e1g, dwg, Tg, Cg, sl, par->psimin);
					if(r==ri && c==ci) printf("frez3. l:%ld e0g:%f e1g:%f Tg:%f dwg:%f\n",l,e0g->co[l],e1g->co[l],Tg->co[l],dwg->co[l]);
				}
			}

			for(l=n1+1;l<=n1+n2;l++){
				if(sl->pa->co[sy][jsf][l-n1]==1){
					T[l]=Tg->co[l-n1];
					dw[l]=dwg->co[l-n1];
					if(wl[l]!=wl[l]) printf("NOvalue wl[i]soil: r:%ld c:%ld l:%ld wi:%f wl:%f dw:%f mf:%ld T:%f\n",r,c,l,wi[l],wl[l],dw[l],mf->co[l],T[l]);
					if(r==ri && c==ci) printf("l:%ld m:%ld T:%f dw:%f\n",l,l-n1,T[l],dw[l]);
				}else{
					T[l]=e->co[l];
					dw[l]=0.0;
				}
			}



		}
	}

	for(l=1;l<=n;l++){

		if(r==ri && c==ci) printf("END l:%ld h:%f abs:%f ad:%f b:%f T:%f dw:%f wliq:%f wice:%f e:%f mf:%ld ",l,h,abs->co[1],ad->co[l],b->co[l],T[l],dw[l],wl[l],wi[l],e->co[l],mf->co[l]);

		//Continuity Check
		//1
		if(wl[l]+dw[l]<0){
			//f=fopen(files->co[ferr]+1,"a");
			//fprintf(f,"\nLIQ CONTENT IN SNOW TOO LOW OR NEGATIVE, cell r=%4ld c=%4ld l=%4ld time=%10.3f, CORRECTED, mf=%4ld tetaliq=%f dth:%f\n",r,c,l,t,mf->co[l],wl[l]/(rho_w*D[l]),dw[l]/(rho_w*D[l]));
			//fclose(f);
			dw[l]=-wl[l];
		}
		//2
		if(wi[l]-dw[l]<0){
			//f=fopen(files->co[ferr]+1,"a");
			//fprintf(f,"\nICE CONTENT IN SNOW NEGATIVE, cell r=%4ld c=%4ld l=%4ld time=%10.3f, CORRECTED, mf=%4ld tetaice=%f dth:%f\n",r,c,l,t,mf->co[l],wi[l]/(rho_w*D[l]),dw[l]/(rho_w*D[l]));
			//fclose(f);
			dw[l]=wi[l];
		}

		//Assign
		wl[l]+=dw[l];
		wi[l]-=dw[l];

		if(r==ri && c==ci) printf(" dw:%f\n",dw[l]);

		if(wi[l]!=wi[l]){
			printf("NOvalue in egy balance4: r:%ld c:%ld l:%ld wi:%f wl:%f dw:%f mf:%ld T:%f\n",r,c,l,wi[l],wl[l],dw[l],mf->co[l],T[l]);
			stop_execution();
		}

		if(wl[l]!=wl[l]){
			printf("NOvalue in egy balance5: r:%ld c:%ld l:%ld wi:%f wl:%f dw:%f mf:%ld T:%f\n",r,c,l,wi[l],wl[l],dw[l],mf->co[l],T[l]);
			stop_execution();
		}

	}

	free_shortvector(mf);
	free_doublevector(ad);
	free_doublevector(ads);
	free_doublevector(adi);
	free_doublevector(b);
	free_doublevector(ad1);
	free_doublevector(ads1);
	free_doublevector(adi1);
	free_doublevector(b1);
	free_doublevector(e);
	free_doublevector(e0g);
	free_doublevector(e1g);
	free_doublevector(dwg);
	free_doublevector(Tg);
	free_doublevector(Cg);


}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void soil_properties(long r, long c, long beg, long end, double *th_l, double ***th_i, double *D, double *wl, double *wi, double *k, double *C, double *T, SOIL *sl){

	long l, m;
	short sy=sl->type->co[r][c];

	for(l=beg;l<=end;l++){

		//sl index
		m=l-beg+1;

		//liquid and ice content
		D[l]=1.0E-3*sl->pa->co[sy][jdz][m];	//[m]
		wl[l]=rho_w*D[l]*th_l[m];			//[kg m^(-2)]
		wi[l]=rho_w*D[l]*th_i[m][r][c];		//[kg m^(-2)]

		//thermal conductivity [W m^-1 K^-1] - from Farouki
		k[l]=k_thermal_soil(th_l[m], th_i[m][r][c], sl->pa->co[sy][jsat][m], sl->T->co[m][r][c], sl->pa->co[sy][jkt][m]);

		//sl heat capacity [J m^-3 K^-1]
		C[l]=sl->pa->co[sy][jct][m]*(1-sl->pa->co[sy][jsat][m]) + c_ice*th_i[m][r][c]*rho_w + c_liq*th_l[m]*rho_w;

		//temperatura [Deg Celsius]
		T[l]=sl->T->co[m][r][c];
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double k_thermal_soil(double th_liq, double th_ice, double th_sat, double T, double k_solid){

	double rho_dry, sr, ke, k_dry, k_sat, ir, k;

	//dry sl density [kg/mc]
	rho_dry=2700.0*(1.0-th_sat);

	//saturation degree
	sr=(th_liq+th_ice)/th_sat;

	//Kersten number
	if(T>=Tfreezing){
		ke=log(sr)+1.0;
		if(ke<=0.0) ke=0.0;
	}else{
		ke=sr;
	}

	//dry soil thermal conductivity [W m^-1 K^-1]
	k_dry=(0.135*rho_dry+64.7)/(2700.0-0.947*rho_dry);

	//soil thermal conductivity [W m^-1 K^-1] Farouki (1981)
	if(sr>1.0E-7){
		//saturated sl thermal conductivity [W m^-1 K^-1]
		if(T>Tfreezing){
			k_sat=pow(k_solid,1.0-th_sat)*pow(k_liq,th_sat);
		}else{
			ir=th_ice/(th_liq+th_ice);
			k_sat=pow(k_solid,1.0-th_sat)*pow(k_liq,th_sat*(1-ir))*pow(k_ice,th_sat*ir);
		}
		//soil thermal conductivity [W m^-1 K^-1]
		k=ke*k_sat + (1.0-ke)*k_dry;
	}else{
		k=k_dry;
	}

	return(k);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void snow_properties(long r, long c, long beg, long end, double *D, double *wl, double *wi, double *k, double *C, double *T, double ***tdz, double ***twl, double ***twi,
					 double ***tT, double (* kfunct)(double r)){

	long l,m;
	double rho;

	for(l=beg;l<=end;l++){

		m=end-l+1;

		D[l]=1.0E-3*tdz[m][r][c];
		wl[l]=twl[m][r][c];
		wi[l]=twi[m][r][c];
		T[l]=tT[m][r][c];
		C[l]=(c_ice*wi[l]+c_liq*wl[l])/D[l];

		rho=(wl[l]+wi[l])/D[l];
		k[l]=(*kfunct)(rho);
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void k_interface(long n, double *k, double *D, double *ki){

	long l;

	for(l=1;l<=n;l++){
		if(l<n){
			ki[l]=(k[l]*k[l+1]*0.5*(D[l]+D[l+1]))/(k[l]*0.5*D[l+1]+k[l+1]*0.5*D[l]);
		}else{
			ki[l]=k[l];
		}
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void prepare_output(double Er_soil, double Mr_snow, double Er_snow, double Sr_snow, double Mr_glac, double Er_glac, double Sr_glac, double prec_rain, double prec_snow_atm,
			ENERGY *egy, WATER *wat, SNOW *snow, GLACIER *glac, LAND *land, TOPO *top, SOIL *sl, METEO *met, TIMES *times, PAR *par, long r, long c, double tausn, double albedo_value,
			double LE, double surfEB, double H, double G, double Ts, double SWin, double SWbeam, double eps, double LWin, double alpha, double cosinc){

	FILE *f;
	long l;

	//STRUCT WATER
	//precipitazione disponibile nel terreno (liquida) [mm/s]
	wat->Pn->co[r][c]=Mr_snow + Mr_glac + prec_rain/par->Dt;

	//STRUCT SNOW
	snow->age->co[r][c]=tausn;
	snow->melted_basin+=Mr_snow*par->Dt;	//[mm]
	snow->evap_basin+=Er_snow*par->Dt;
	snow->subl_basin+=Sr_snow*par->Dt;

	//STRUCT GLAC
	glac->melted_basin+=Mr_glac*par->Dt;
	glac->evap_basin+=Er_glac*par->Dt;
	glac->subl_basin+=Sr_glac*par->Dt;

	//OUTPUT DISTRIBUITI OPZIONALI
	if(par->output_balancesn>0){
		snow->MELTED->co[r][c]+=Mr_snow*par->Dt;
		snow->SUBL->co[r][c]+=(Sr_snow+Er_snow)*par->Dt;
		if(snow->type->co[r][c]==2){
			snow->t_snow->co[r][c]+=par->Dt/86400.0;
			for(l=1;l<=snow->lnum->co[r][c];l++){
				snow->totav_snow->co[r][c]+=snow->Dzl->co[l][r][c];
			}
		}
	}

	if(par->output_balancegl>0 && par->glaclayer_max>0) glac->MELTED->co[r][c]+=Mr_glac*par->Dt;
	if(par->output_balancegl>0 && par->glaclayer_max>0) glac->SUBL->co[r][c]+=(Sr_glac+Er_glac)*par->Dt;

	if(par->output_P>0){
		wat->PrTOT_mean->co[r][c]+=wat->total->co[r][c];	/*[mm]*/
		wat->PrSNW_mean->co[r][c]+=prec_snow_atm;								/*[mm]*/
	}

	if(par->output_albedo>0) land->albedo->co[r][c]+=albedo_value/((par->output_albedo*3600.0)/(par->Dt));

	if(par->output_ET>0){
		egy->ET_mean->co[r][c]+=LE/((par->output_ET*3600.0)/(par->Dt)); //[W/m^2]
		if(par->distr_stat==1){
			if(egy->ET_max->co[r][c]<LE) egy->ET_max->co[r][c]=LE;
			if(egy->ET_min->co[r][c]>LE) egy->ET_min->co[r][c]=LE;
		}
	}

	if(par->output_G>0){
		egy->G_mean->co[r][c]+=surfEB/((par->output_G*3600.0)/(par->Dt)); //[W/m^2]
		if(par->distr_stat==1){
			if(egy->G_max->co[r][c]<surfEB) egy->G_max->co[r][c]=surfEB;
			if(egy->G_min->co[r][c]>surfEB) egy->G_min->co[r][c]=surfEB;
		}
		egy->G_snowsoil->co[r][c]+=G/((par->output_G*3600.0)/(par->Dt)); //[W/m^2]
	}

	if(par->output_H>0){
		egy->H_mean->co[r][c]+=H/((par->output_H*3600.0)/(par->Dt)); //[W/m^2]
		if(par->distr_stat==1){
			if(egy->H_max->co[r][c]<H) egy->H_max->co[r][c]=H;
			if(egy->H_min->co[r][c]>H) egy->H_min->co[r][c]=H;
		}
	}

	if(par->output_Ts>0){
		egy->Ts_mean->co[r][c]+=Ts/((par->output_Ts*3600.0)/(par->Dt)); /*update of surface temperature [Celsius]*/
		if(par->distr_stat==1){
			if(egy->Ts_max->co[r][c]<Ts) egy->Ts_max->co[r][c]=Ts;
			if(egy->Ts_min->co[r][c]>Ts) egy->Ts_min->co[r][c]=Ts;
		}
	}

	if(par->output_Rswdown>0){
		egy->Rswdown_mean->co[r][c]+=SWin/((par->output_Rswdown*3600.0)/(par->Dt));
		egy->Rswbeam->co[r][c]+=SWbeam/((par->output_Rswdown*3600.0)/(par->Dt));
		if(par->distr_stat==1){
			if(egy->Rswdown_max->co[r][c]<SWin) egy->Rswdown_max->co[r][c]=SWin;
		}
	}

	if(par->output_meteo>0){
		egy->Ta_mean->co[r][c]+=met->Tgrid->co[r][c]/((par->output_meteo*3600.0)/(par->Dt));
		if(par->distr_stat==1){
			if(egy->Ta_max->co[r][c]<met->Tgrid->co[r][c]) egy->Ta_max->co[r][c]=met->Tgrid->co[r][c];
			if(egy->Ta_min->co[r][c]>met->Tgrid->co[r][c]) egy->Ta_min->co[r][c]=met->Tgrid->co[r][c];
		}
		if(par->micromet1==1){
			met->Vspdmean->co[r][c]+=met->Vgrid->co[r][c]/((par->output_meteo*3600.0)/(par->Dt));
			met->Vdirmean->co[r][c]+=met->Vdir->co[r][c]/((par->output_meteo*3600.0)/(par->Dt));
			met->RHmean->co[r][c]+=met->RHgrid->co[r][c]/((par->output_meteo*3600.0)/(par->Dt));
		}
	}

	if(par->output_Rn>0){
		egy->Rn_mean->co[r][c]+=(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)))/((par->output_Rn*3600.0)/(par->Dt)); //[W/m^2]
		if(fabs(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)))>1500){
			f=fopen(files->co[ferr]+1,"a");
			fprintf(f,"\ntime=%10.1f r=%4ld c=%4ld Rn=%10.3f Rsw=%10.3f Rlwdiff=%10.3f albedo=%10.8f eps=%10.8fTa=%10.5f Ts=%10.5f Rsw_meas=%f sin(alpha)=%f cos(inc)=%f\n",
				times->time,r,c,SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)),SWin,LWin,albedo_value,
				eps,met->Tgrid->co[r][c],Ts,met->var[0][iSW],sin(alpha),cosinc);
			fprintf(f,"\nH:%f LE:%f\n",H,LE);
			fclose(f);
		}
		egy->LW_in->co[r][c]+=( top->sky->co[r][c]*5.67E-8*eps*pow((Ts+tk),4.0) )/((par->output_Rn*3600.0)/(par->Dt));
		egy->LW_out->co[r][c]+=( top->sky->co[r][c]*eps*LWin )/((par->output_Rn*3600.0)/(par->Dt));
		egy->SW->co[r][c]+=( SWin*(1.0-albedo_value) )/((par->output_Rn*3600.0)/(par->Dt));
		if(par->distr_stat==1){
			if(egy->LW_max->co[r][c]<top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)))
				egy->LW_max->co[r][c]=top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0));
			if(egy->LW_min->co[r][c]>top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)))
				egy->LW_min->co[r][c]=top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0));
			if(egy->Rn_max->co[r][c]<(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0))))
				egy->Rn_max->co[r][c]=(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)));
			if(egy->Rn_min->co[r][c]>(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0))))
				egy->Rn_min->co[r][c]=(SWin*(1.0-albedo_value) + top->sky->co[r][c]*(eps*LWin - 5.67E-8*eps*pow((Ts+tk),4.0)));
			if(egy->SW_max->co[r][c]<(SWin*(1.0-albedo_value))) egy->SW_max->co[r][c]=SWin*(1.0-albedo_value);
		}
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void output_pixel(long r, long c, double prec_snow, double prec_rain_on_soil, double prec_rain_on_snow, double Sr_soil, double Er_soil, double Mr_snow, double Er_snow, double Sr_snow,
			double Mr_glac, double Er_glac, double Sr_glac, double Epc, double fec, double ftc, double LE, double H, double surfEB, double G, double Ts, double albedo_value, double eps,
			double LWin, double SWin, double LWout, double SWout, double epsa, double epsa_min, double epsa_max, double SWbeam, double SWdiff, double DTcorr, double Tdew,
			long n, DOUBLEVECTOR *turbulence, PAR *par, WATER *wat, ENERGY *egy, TOPO *top, METEO *met, SNOW *snow, GLACIER *glac, LAND *land, double Vpoint, double RHpoint,
			double prec_snow_atm, double prec_rain_atm, double maxstorage_c, double evap_c, double drip_c, double z0, double d0){

	long i, j, l;
	double ea, es, de_dT, Q;

	if(par->state_pixel==1){
		for(i=1;i<=par->chkpt->nrh;i++){
			if(r==par->rc->co[i][1] && c==par->rc->co[i][2]){

				wat->out1->co[3][i]+=prec_snow;
				wat->out1->co[4][i]+=(prec_rain_on_soil+prec_rain_on_snow);

				wat->out1->co[16][i]+=prec_snow_atm;
				wat->out1->co[17][i]+=prec_rain_atm;
				wat->out1->co[18][i]+=maxstorage_c/(double)n;
				wat->out1->co[19][i]+=evap_c;
				wat->out1->co[20][i]+=drip_c;

				wat->out1->co[21][i]+=prec_snow;
				wat->out1->co[22][i]+=prec_rain_on_soil;
				wat->out1->co[23][i]+=prec_snow_atm;
				wat->out1->co[24][i]+=prec_rain_atm;
				wat->out1->co[25][i]+=evap_c;
				wat->out1->co[26][i]+=drip_c;
				wat->out1->co[27][i]+=prec_rain_on_snow;

				egy->out1->co[1][i]+=Er_soil*par->Dt;	//Eg[mm]
				egy->out1->co[2][i]+=Sr_soil*par->Dt;	//Sg[mm]
				//egy->out1->co[3][i]+=Epc*fec*(1000.0/rho_w)*par->Dt;	//Evc[mm]
				egy->out1->co[4][i]+=Epc*ftc*(1000.0/rho_w)*par->Dt;	//Etc[mm]
				egy->out1->co[5][i]+=LE/(double)n; //ET[W/m^2]
				egy->out1->co[6][i]+=H/(double)n; //H[W/m^2]
				egy->out1->co[7][i]+=surfEB/(double)n;
				egy->out1->co[8][i]+=G/(double)n;
				egy->out1->co[9][i]+=((prec_rain_on_soil+prec_rain_on_snow)*Lf/par->Dt)/(double)n; //Qrain[W/m^2]
				egy->out1->co[10][i]+=Ts/(double)n; //Ts[C]
				//Rnet and Rnet_cumulated
				egy->out1->co[11][i]+=(SWin-SWout+eps*LWin-LWout)/(double)n;
				egy->out1->co[21][i]+=(SWin-SWout+eps*LWin-LWout)*par->Dt*1.0E-6;
				//Rlw_in and Rlw_in_cumulated
				egy->out1->co[12][i]+=eps*LWin/(double)n;
				egy->out1->co[22][i]+=eps*LWin*par->Dt*1.0E-6;
				//Rlw_out and Rlw_out_cumulated
				egy->out1->co[13][i]-=LWout/(double)n;
				egy->out1->co[23][i]-=LWout*par->Dt*1.0E-6;
				//Rsw, Rsw_incoming_cumulated, albedo, Rsw_outcoming_cumulated
				egy->out1->co[14][i]+=SWin/(double)n;
				egy->out1->co[24][i]+=SWin*par->Dt*1.0E-6;
				if(SWout>0){
					egy->out1->co[15][i]+=(SWout/SWin)/(double)n;
				}else{
					egy->out1->co[15][i]+=albedo_value/(double)n;
				}
				egy->out1->co[28][i]-=SWout*par->Dt*1.0E-6;
				egy->out1->co[45][i]-=SWout/(double)n;
				//atmosphere emissivity
				egy->out1->co[16][i]+=epsa/(double)n;
				//wind speed
				egy->out1->co[17][i]+=Vpoint/(double)n;
				//relative humidity
				egy->out1->co[18][i]+=RHpoint/(double)n;
				//atmospheric pressure
				egy->out1->co[19][i]+=met->Pgrid->co[r][c]/(double)n;
				//air temperature
				egy->out1->co[20][i]+=met->Tgrid->co[r][c]/(double)n;
				//ET cumulated
				egy->out1->co[25][i]+=LE*par->Dt*1.0E-6;
				//H cumulated
				egy->out1->co[26][i]+=H*par->Dt*1.0E-6;
				//G cumulated
				egy->out1->co[27][i]+=surfEB*par->Dt*1.0E-6;

				//L Obukhov
				egy->out1->co[29][i]+=turbulence->co[2]/(double)n;
				//number of iteration
				egy->out1->co[30][i]+=turbulence->co[1]/(double)n;
				//CH, CL (transfer coefficient for H and L)
				egy->out1->co[35][i]+=turbulence->co[10]/(double)n;
				egy->out1->co[36][i]+=turbulence->co[11]/(double)n;
				egy->out1->co[37][i]+=DTcorr/(double)n;
				egy->out1->co[38][i]+=turbulence->co[3]/(double)n;
				egy->out1->co[39][i]+=turbulence->co[4]/(double)n;
				egy->out1->co[40][i]+=turbulence->co[5]/(double)n;
				egy->out1->co[41][i]+=turbulence->co[6]/(double)n;
				egy->out1->co[42][i]+=turbulence->co[7]/(double)n;
				egy->out1->co[43][i]+=turbulence->co[8]/(double)n;
				egy->out1->co[44][i]+=turbulence->co[9]/(double)n;

				//ea(mbar) Q(-)
				egy->out1->co[46][i]+=Tdew/(double)n;
				sat_vap_pressure(&ea, &de_dT, met->Tgrid->co[r][c], met->Pgrid->co[r][c]);
				Q=RHpoint*0.622*ea/(met->Pgrid->co[r][c]-0.378*ea);
				ea=Q*met->Pgrid->co[r][c]/(0.622+Q*0.378);
				egy->out1->co[47][i]+=ea/(double)n;
				egy->out1->co[48][i]+=Q/(double)n;
				sat_vap_pressure(&es, &de_dT, Ts, met->Pgrid->co[r][c]);
				egy->out1->co[49][i]+=es/(double)n;
				egy->out1->co[50][i]+=0.622*es/(met->Pgrid->co[r][c]-0.378*es)/(double)n;

				if(epsa_min>0){
					egy->out1->co[51][i]+=(eps*epsa_min*5.67E-8*pow(met->Tgrid->co[r][c]+tk,4.0))/(double)n;
				}else{
					egy->out1->co[51][i]=UV->V->co[2];
				}
				if(epsa_max>0){
					egy->out1->co[52][i]+=(eps*epsa_max*5.67E-8*pow(met->Tgrid->co[r][c]+tk,4.0))/(double)n;
				}else{
					egy->out1->co[52][i]=UV->V->co[2];
				}
				egy->out1->co[53][i]+=(SWbeam/(double)n);
				egy->out1->co[54][i]+=(SWdiff/(double)n);

				if(par->micromet1==1){
					egy->out1->co[55][i]+=(met->Vdir->co[r][c])/(double)n;
				}else{
					egy->out1->co[55][i]=UV->V->co[2];
				}
				egy->out1->co[56][i]+=(land->LAI->co[land->use->co[r][c]])/(double)n;

				egy->out1->co[57][i]+=z0/(double)n;
				egy->out1->co[58][i]+=d0/(double)n;

				snow->melted->co[i]+=Mr_snow*par->Dt;	//[mm]
				snow->evap->co[i]+=Er_snow*par->Dt;	//[mm]
				snow->subl->co[i]+=Sr_snow*par->Dt;	//[mm]

				glac->melted->co[i]+=Mr_glac*par->Dt;	//[mm]
				glac->evap->co[i]+=Er_glac*par->Dt;	//[mm]
				glac->subl->co[i]+=Sr_glac*par->Dt;	//[mm]

				for(l=1;l<=par->snowlayer_max;l++){
					snow->CR1m->co[l]+=(snow->CR1->co[l]*3600.0/(double)n);
					snow->CR2m->co[l]+=(snow->CR2->co[l]*3600.0/(double)n);
					snow->CR3m->co[l]+=(snow->CR3->co[l]*3600.0/(double)n);
				}

			}
		}
	}

	//OUTPUT precipitation+melting for each land use (split in snow covered and snow free)
	for(i=1;i<=land->clax->nh;i++){
		if(land->use->co[r][c]==land->clax->co[i]) j=i;
	}
	if(snow->type->co[r][c]==2){
		wat->outfluxes->co[1][j]+=prec_rain_on_soil;
		wat->outfluxes->co[2][j]+=Mr_snow*par->Dt;
		wat->outfluxes->co[3][j]+=Mr_glac*par->Dt;
		land->cont->co[j][2]+=1;
	}else{
		wat->outfluxes->co[4][j]+=prec_rain_on_soil;
		wat->outfluxes->co[5][j]+=Mr_snow*par->Dt;
		wat->outfluxes->co[6][j]+=Mr_glac*par->Dt;
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void output_basin(double prec_rain, double prec_snow, double Ts, double Er_soil, double Sr_soil, double Epc, double fec, double ftc, double LE, double H, double surfEB,
				double Eimm, double SWin, double LWin, double albedo, double eps, double V, double Dt, long n, double *wat, double *en){

	//OUTPUT MEDI PER BACINO
	wat[1]+=prec_rain;																/*[mm]*/
	wat[2]+=prec_snow;																/*[mm]*/
	en[1]+=Ts/(double)n;															/*Ts[C]*/
	en[2]+=Er_soil*Dt;																/*Eg[mm]*/
	en[3]+=Sr_soil*Dt;																/*Sg[mm]*/
	en[4]+=Epc*fec*Dt;																/*Evc[mm]*/
	en[5]+=Epc*ftc*Dt;																/*Etc[mm]*/
	en[6]+=LE/(double)n;															/*ET[W/m2]*/
	en[7]+=H/(double)n;																/*H[W/m2]*/
	en[8]+=surfEB/(double)n;															/*G[W/m2]*/
	en[9]+=Eimm/(double)n;															/*Ecanopy[W/m2]*/
	en[10]+=(prec_rain*Lf/Dt)/(double)n;											/*Qrain[W/m2]*/
	en[11]+=(SWin*(1.0-albedo) + V*(eps*LWin - eps*SB(Ts)))/(double)n;				/*Rn[W/m2]*/
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void output_altrank(long ES, double prec_rain, double prec_snow, double Ts, double Er_soil, double Sr_soil, double Epc, double fec, double ftc, double LE, double H, double surfEB,
	double Eimm, double SWin, double LWin, double albedo, double eps, double V, double Pn, double runoff, double Er_snow, double Sr_snow, double Mr_snow, double Er_glac,
	double Sr_glac, double Mr_glac, double Dt, long n, double Z, double Zmin, double Zmax, double glacD, double glacDmin, double **out){

	long i;

	//OUTPUT MEDI PER FASCE ALTIMETRICHE
	for(i=1;i<=ES;i++){
		if( ( (Z>=Zmin+(i-1)*(Zmax-Zmin)/(double)ES && Z<Zmin+i*(Zmax-Zmin)/(double)ES) || (Z==Zmin+i*(Zmax-Zmin)/(double)ES && i==ES) ) && glacD>=glacDmin ){
			out[1][i]+=Er_soil*Dt;					//Eg[mm]
			out[2][i]+=Sr_soil*Dt;					//Sg[mm]
			out[3][i]+=Epc*fec*(1000.0/rho_w)*Dt;	//Evc[mm]
			out[4][i]+=Epc*ftc*(1000.0/rho_w)*Dt;	//Etc[mm]
			out[5][i]+=LE/(double)n;				//ET[W/m^2]
			out[6][i]+=H/(double)n;					//H[W/m^2]
			out[7][i]+=surfEB/(double)n;				//G[W/m^2]
			out[8][i]+=Eimm/(double)n;				//Ecanopy[W/m^2]
			out[9][i]+=(prec_rain*Lf/Dt)/(double)n; //Qrain[W/m^2]
			out[10][i]+=Ts/(double)n;				//Ts[C]
			out[11][i]+=(SWin*(1.0-albedo) + V*(eps*LWin - eps*SB(Ts)))/(double)n; //[W/m^2]
			out[12][i]+=prec_rain;					//[mm]
			out[13][i]+=prec_snow;					//[mm]
			out[14][i]+=Pn*Dt;						//[mm]
			out[15][i]+=runoff*Dt;					//[mm]
			out[16][i]+=Mr_snow*Dt;
			out[17][i]+=Er_snow*Dt;
			out[18][i]+=Sr_snow*Dt;
			out[20][i]+=Mr_glac*Dt;
			out[21][i]+=Er_glac*Dt;
			out[22][i]+=Sr_glac*Dt;
		}
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
double flux(long i, long icol, long **col, double **met, double k, double est){

	double F;

	if(col[i-1][icol]!=-1){
		F=k*met[i-1][icol];
	}else{
		F=est;
	}
	return(F);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void evaporation(long r, long c, long n, double E, double Rg, double *wi, double *wl, double *D, double dt, double *Ss, double *Es, double *Eg){

	double thres=0.20;	//threshold on liquid content in the snow after which evaporation (not sublimation) occurs
	double th_liq;		//liquid content

	*Es=0.0;
	*Ss=0.0;
	*Eg=0.0;

	if(n>0){ //snow or glacier
		th_liq=wl[1]/(rho_w*D[1]);
		if(th_liq>thres){
			wl[1]-=E*dt;
			if(wl[1]<0){
				wi[1]+=wl[1];
				E+=wl[1]/dt;
				wl[1]=0.0;
				if(wi[1]<0){
					E+=wi[1]/dt;
					wi[1]=0.0;
				}
			}
			*Es=E;	//[mm/s] water equivalent
		}else{
			wi[1]-=E*dt;
			if(wi[1]<0){
				E+=wi[1]/dt;
				wi[1]=0.0;
			}
			*Ss=E;
		}
	}else{	//sl
		*Eg=Rg*E;
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void update_soil(long ntot, long r, long c, SOIL *sl, double *T, double *dw, double *wi, double *th, double fc, double *ft, double Ep, double Eg, PAR *par){

	long l, m;
	double source,theta,A,dt=par->Dt;
	short sy=sl->type->co[r][c];

	for(l=1;l<=Nl;l++){
		m=l+ntot-Nl;
		A=sl->pa->co[sy][jdz][l]*0.001*rho_w;

		sl->T->co[l][r][c]=T[m];
		sl->thice->co[l][r][c]=wi[m]/A;		//[-]

	}

	for(l=1;l<=Nl;l++){
		m=l+ntot-Nl;
		A=sl->pa->co[sy][jdz][l]*0.001*rho_w;

		if(l==1){
			source=dw[m] - dt*( (1-fc)*Eg + fc*Ep*ft[1] );	//[kg/m2]=[mm water]
			//source=dw[m];
		}else{
			source=dw[m] - dt*fc*Ep*ft[l];
			//source=dw[m];
		}

		theta=th[l]+source/A;
		if(th[l]>=sl->pa->co[sy][jsat][l]){
			if(theta>th[l]) theta=th[l];
		}else{
			if(theta>sl->pa->co[sy][jsat][l]) theta=sl->pa->co[sy][jsat][l];
		}

		A=sl->pa->co[sy][jdz][l]*0.001*rho_w;

		sl->P->co[l][r][c]=psi_teta(theta, sl->thice->co[l][r][c], sl->pa->co[sy][jsat][l], sl->pa->co[sy][jres][l], sl->pa->co[sy][ja][l],
				sl->pa->co[sy][jns][l], 1-1/sl->pa->co[sy][jns][l], fmin(sl->pa->co[sy][jpsimin][l], Psif(sl->T->co[l][r][c], par->psimin)), par->Esoil);

	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void liqWBsnow(long r, long c, SNOW *snow, double *Mr, double *PonS, PAR *par, double slope, double P, double *wi, double *wl, double *T){

	long l, m;
	double Se, theta_liq, thice, Dw=P, wice_old;
	DOUBLEVECTOR *q;

	//rain on snow
	*PonS=0.0;	//initialization

	//a. ordinary case
	if(snow->type->co[r][c]==2){

		q=new_doublevector(snow->lnum->co[r][c]);

		for(l=snow->lnum->co[r][c];l>=1;l--){

			m=snow->lnum->co[r][c] - l + 1;	//increasing downwards (while l increases upwards)

			//ADD LIQUID WATER FROM ABOVE AND ACCOUNT FOR POSSIBLE REFREEZING
			if(l<snow->lnum->co[r][c]) Dw=q->co[l+1]*par->Dt;

			wl[m]+=Dw;

			//find new internal energy, taking into account the latent heat flux of the incoming water flux
			from_internal_energy(r, c, internal_energy(wi[m], wl[m], T[m]), &(wi[m]),&(wl[m]),&(T[m]));

			//assign snow state variables
			wice_old=snow->w_ice->co[l][r][c];
			snow->T->co[l][r][c]=T[m];
			snow->w_ice->co[l][r][c]=wi[m];
			snow->w_liq->co[l][r][c]=wl[m];

			//ACCOUNT FOR SNOW COMPACTION
			//a)destructive metamorphism and overburden
			snow_compactation(r, c, l, snow, slope, par, &(snow->CR1->co[l]), &(snow->CR2->co[l]));

			//b)melting: snow depth decreases maintaining the same density
			if(wi[m]/wice_old<1){
				snow->Dzl->co[l][r][c]*=(wi[m]/wice_old);
				snow->CR3->co[l]=(wi[m]-wice_old)/(wice_old*par->Dt);
			}else{
				snow->CR3->co[l]=0.0;
			}

			//check that snow porosity is not too large
			thice=snow->w_ice->co[l][r][c]/(1.0E-3*snow->Dzl->co[l][r][c]*rho_i);		//[-]
			if(thice>par->snow_maxpor){
				snow->Dzl->co[l][r][c]=1000.0*snow->w_ice->co[l][r][c]/(rho_i*par->snow_maxpor);
				thice=par->snow_maxpor;
			}

			//CALCULATE LIQUID WATER GOING BELOW
			if(snow->w_ice->co[l][r][c]<0.001){
				q->co[l]=snow->w_liq->co[l][r][c]/par->Dt;
				snow->w_liq->co[l][r][c]=0.0;
			}else{
				theta_liq=snow->w_liq->co[l][r][c]/(1.0E-3*snow->Dzl->co[l][r][c]*rho_w);		//[-]
				Se=(theta_liq - par->Sr*(1.0-thice))/(1.0-thice - par->Sr*(1.0-thice));
				if(Se<0) Se=0.0;
				if(Se>1) Se=1.0;
				q->co[l]=fabs(cos(slope))*kidrmax_snow((snow->w_liq->co[l][r][c]+snow->w_ice->co[l][r][c])/(0.001*snow->Dzl->co[l][r][c]))*pow(Se,3.0);

				if(theta_liq>par->Sr*(1.0-thice)){		//when theta_liq is larger than the capillary retenction
					snow->w_liq->co[l][r][c]-=q->co[l]*par->Dt;
					theta_liq=snow->w_liq->co[l][r][c]/(1.0E-3*snow->Dzl->co[l][r][c]*rho_w);
					if(theta_liq<=par->Sr*(1.0-thice)){
						q->co[l]-=((par->Sr*(1.0-thice) - theta_liq)*1.0E-3*snow->Dzl->co[l][r][c]*rho_w)/par->Dt;
						snow->w_liq->co[l][r][c]=par->Sr*(1.0-thice)*1.0E-3*snow->Dzl->co[l][r][c]*rho_w;
					}else if(theta_liq > (1.0-thice) ){
						snow->w_liq->co[l][r][c]=(1.0-thice)*rho_w*1.0E-3*snow->Dzl->co[l][r][c];
						q->co[l]+=( theta_liq - (1.0-thice) )*(rho_w*1.0E-3*snow->Dzl->co[l][r][c])/par->Dt;  //liquid excess in snow goes downwards
					}
				}
			}

		}

		//melting rate
		*Mr=q->co[1]-P/par->Dt;	//[mm/s]

		//rain on snow
		*PonS=P;	//[mm]

		free_doublevector(q);



	//b. simplified case
	}else if(snow->type->co[r][c]==1){

		snow->T->co[1][r][c]=T[1];
		*Mr=1.0E+3*wl[1]/(rho_w*par->Dt);	//[mm/s]
		snow->w_liq->co[1][r][c]=0.0;
		snow->Dzl->co[1][r][c]*=(wi[1]/snow->w_ice->co[1][r][c]);
		snow->w_ice->co[1][r][c]=wi[1];

		snow->CR1->co[1]=0.0;
		snow->CR2->co[1]=0.0;
		snow->CR3->co[1]=(wi[1]-snow->w_ice->co[1][r][c])/(snow->w_ice->co[1][r][c]*par->Dt);
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void iceWBsnow(long r, long c, SNOW *snow, double P, double Ta){

	long ns;
	double Dz, h;

	if(P>0){

		Dz=P*rho_w/snow->rho_newsnow->co[r][c];	//snow depth addition due to snow precipitation

		if(snow->type->co[r][c]==0){

			snow->Dzl->co[1][r][c]+=Dz;
			snow->w_ice->co[1][r][c]+=P;

		}else if(snow->type->co[r][c]==1){

			snow->Dzl->co[1][r][c]+=Dz;
			snow->w_ice->co[1][r][c]+=P;

		}else if(snow->type->co[r][c]==2){

			ns=snow->lnum->co[r][c];

			h=(c_ice*snow->w_ice->co[ns][r][c] + c_liq*snow->w_liq->co[ns][r][c])*(snow->T->co[ns][r][c] - Tfreezing) + Lf*snow->w_liq->co[ns][r][c];
			h+=(c_ice*P)*(Ta - Tfreezing);
			snow->Dzl->co[ns][r][c]+=Dz;
			snow->w_ice->co[ns][r][c]+=P;
			if(h<0.0){
				snow->T->co[ns][r][c]=Tfreezing + h/(c_ice*snow->w_ice->co[ns][r][c] + c_liq*snow->w_liq->co[ns][r][c]);
			}else{
				snow->T->co[ns][r][c]=Tfreezing;
			}

		}


	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void WBglacier(long ns, long r, long c, GLACIER *glac, double *Mr, PAR *par, double *wi, double *wl, double *T){

	long l, m;
	double Dz, thice, theta_liq;

	*Mr=0.0;

	for(l=glac->lnum->co[r][c];l>=1;l--){

		m=ns + glac->lnum->co[r][c] - l + 1;

		if(wi[m]/glac->w_ice->co[l][r][c]<1 && glac->w_ice->co[l][r][c]/(1.0E-3*glac->Dzl->co[l][r][c]*rho_i)<0.95 )
			glac->Dzl->co[l][r][c]*=(wi[m]/glac->w_ice->co[l][r][c]);

		glac->T->co[l][r][c]=T[m];
		glac->w_ice->co[l][r][c]=wi[m];
		glac->w_liq->co[l][r][c]=wl[m];

		Dz=1.0E-3*glac->Dzl->co[l][r][c];				//[m]
		thice=glac->w_ice->co[l][r][c]/(Dz*rho_i);	//[-]
		theta_liq=glac->w_liq->co[l][r][c]/(Dz*rho_w);	//[-]

		if(thice>0.95){
			glac->Dzl->co[l][r][c]=1000.0*glac->w_ice->co[l][r][c]/(rho_i*par->snow_maxpor);
			Dz=1.0E-3*glac->Dzl->co[l][r][c];
			thice=par->snow_maxpor;
			theta_liq=glac->w_liq->co[l][r][c]/(Dz*rho_w);
		}

		if(theta_liq>par->Sr_glac*(1.0-thice)){
			*Mr+=( rho_w*Dz*(theta_liq-par->Sr_glac*(1.0-thice))/par->Dt );	//in [mm/s]
			glac->w_liq->co[l][r][c]=rho_w*Dz*par->Sr_glac*(1.0-thice);
		}
	}

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void glac2snow(long r, long c, SNOW *snow, GLACIER *glac, DOUBLEVECTOR *Dmin, DOUBLEVECTOR *Dmax){

	long l;
	double D, h;

	D=DEPTH(r, c, glac->lnum, glac->Dzl);

	//if ice is less thick than Dmin[1], ice is considered as snow
	if(D<Dmin->co[1] && D>0.1){
		h=(c_ice*glac->w_ice->co[1][r][c] + c_liq*glac->w_liq->co[1][r][c])*(glac->T->co[1][r][c]-Tfreezing) + Lf*glac->w_liq->co[1][r][c];

		for(l=2;l<=glac->lnum->co[r][c];l++){
			glac->Dzl->co[1][r][c]+=glac->Dzl->co[l][r][c];
			glac->w_liq->co[1][r][c]+=glac->w_liq->co[l][r][c];
			glac->w_ice->co[1][r][c]+=glac->w_ice->co[l][r][c];
			if(glac->T->co[l][r][c]>-98.999) h+=(c_ice*glac->w_ice->co[l][r][c] + c_liq*glac->w_liq->co[l][r][c])*(glac->T->co[l][r][c]-Tfreezing) + Lf*glac->w_liq->co[l][r][c];
			glac->Dzl->co[l][r][c]=0.0;
			glac->w_liq->co[l][r][c]=0.0;
			glac->w_ice->co[l][r][c]=0.0;
			glac->T->co[l][r][c]=-99.0;
		}

		if(snow->lnum->co[r][c]==0){
			snow->lnum->co[r][c]=1;
			snow->type->co[r][c]=2;
		}else{
			h+=(c_ice*snow->w_ice->co[1][r][c] + c_liq*snow->w_liq->co[1][r][c])*(snow->T->co[1][r][c]-Tfreezing) + Lf*snow->w_liq->co[2][r][c];
		}

		snow->Dzl->co[1][r][c]+=glac->Dzl->co[1][r][c];
		snow->w_ice->co[1][r][c]+=glac->w_ice->co[1][r][c];
		snow->w_liq->co[1][r][c]+=glac->w_liq->co[1][r][c];
		if(h<0){
			snow->T->co[1][r][c]=Tfreezing + h/(c_ice*snow->w_ice->co[1][r][c]+c_liq*snow->w_liq->co[1][r][c]);
		}else{
			snow->T->co[1][r][c]=0.0;
		}

		glac->Dzl->co[1][r][c]=0.0;
		glac->w_liq->co[1][r][c]=0.0;
		glac->w_ice->co[1][r][c]=0.0;
		glac->T->co[1][r][c]=-99.0;	//novalue
		glac->lnum->co[r][c]=0;
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void output_snow(SNOW *snow, double **Z, PAR *par){

	long r, c, i, l, nr, nc;
	double D, DW, n=(par->output_snow*3600.0)/(par->Dt);

	nr=snow->T->nrh;
	nc=snow->T->nch;

	for(r=1;r<=nr;r++){
		for(c=1;c<=nc;c++){
			if(Z[r][c]!=UV->V->co[2]){

				if(par->snowtrans==1){

					DW=snow->Wsalt->co[r][c]+snow->Wsalt->co[r][c]+snow->Wsubl->co[r][c]+snow->Wsubgrid->co[r][c];

					if(par->output_snow>0){
						snow->Wtot_cum->co[r][c]+=DW;
						snow->Wsalt_cum->co[r][c]+=snow->Wsalt->co[r][c];
						snow->Wsusp_cum->co[r][c]+=snow->Wsusp->co[r][c];
						snow->Wsubl_cum->co[r][c]+=snow->Wsubl->co[r][c];
						snow->Wsubgrid_cum->co[r][c]+=snow->Wsubgrid->co[r][c];
					}

					if(par->state_pixel==1){
						for(i=1;i<=par->chkpt->nrh;i++){
							if(r==par->rc->co[i][1] && c==par->rc->co[i][2]){
								snow->out_bs->co[1][i]+=snow->Wsalt->co[r][c];
								snow->out_bs->co[2][i]+=snow->Wsalt->co[r][c];
								snow->out_bs->co[3][i]+=snow->Wsusp->co[r][c];
								snow->out_bs->co[4][i]+=snow->Wsusp->co[r][c];
								snow->out_bs->co[5][i]+=snow->Wsubl->co[r][c];
								snow->out_bs->co[6][i]+=snow->Wsubl->co[r][c];
								snow->out_bs->co[7][i]+=snow->Wsubgrid->co[r][c];
								snow->out_bs->co[8][i]+=snow->Wsubgrid->co[r][c];
								snow->out_bs->co[9][i]+=DW;
								snow->out_bs->co[10][i]+=DW;
							}
						}
					}
				}

				if(par->output_snow>0){
					D=0.0;
					for(l=1;l<=snow->lnum->co[r][c];l++){
						//D+=(snow->w_liq->co[l][r][c]+snow->w_ice->co[l][r][c]);
						D+=snow->Dzl->co[l][r][c];
					}
					if(D>snow->max->co[r][c]) snow->max->co[r][c]=D;
					snow->average->co[r][c]+=D/n;
				}
			}
		}
	}
}


/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void output_map_plots(long r, long c, PAR *par, double t, long n, ENERGY *egy, METEO *met, SNOW *snow, double H, double LE, double SWin, double SWout, double LWin, double LWout, double Ts){

	long j, d, M, y, h, m, N;
	double tmin, tmax, JD;

	if(par->JD_plots->co[1]!=0){

		date_time(t, par->year0, par->JD0, 0.0, &JD, &d, &M, &y, &h, &m);

		for(j=1;j<=par->JD_plots->nh;j++){

			tmin=get_time( (double)(par->JD_plots->co[j]-1), y, par->JD0, par->year0 );
			//tmin=0.0;
			tmax=get_time( (double)(par->JD_plots->co[j]  ), y, par->JD0, par->year0 );
			if(fmod(tmax-tmin,n*par->Dt)!=0.0){
				N=floor(tmax-tmin/(n*par->Dt))+1;
				tmax=tmin+N*n*par->Dt;
			}

			if(t>=tmin && t<tmax){
				egy->Hplot->co[r][c]+=H/(double)n;
				egy->LEplot->co[r][c]+=LE/(double)n;
				egy->SWinplot->co[r][c]+=SWin/(double)n;
				egy->SWoutplot->co[r][c]+=SWout/(double)n;
				egy->LWinplot->co[r][c]+=LWin/(double)n;
				egy->LWoutplot->co[r][c]+=LWout/(double)n;
				egy->Tsplot->co[r][c]+=Ts/(double)n;
				snow->Dplot->co[r][c]+=DEPTH(r,c,snow->lnum,snow->Dzl)/(double)n;
				met->Taplot->co[r][c]+=met->Tgrid->co[r][c]/(double)n;
				if(par->micromet1==1){
					met->Vspdplot->co[r][c]+=met->Vgrid->co[r][c]/(double)n;
					met->Vdirplot->co[r][c]+=met->Vdir->co[r][c]/(double)n;
					met->RHplot->co[r][c]+=met->RHgrid->co[r][c]/(double)n;
				}
			}
		}
	}
}


/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void evaporation_soil(long nsnow, double E, double dEdT, double theta, double res, double sat, double Ratm, double Ts, double *RG, double *LE_s, double *dLEdT_s, double *Evc,
    double *Dc, double *Msc, double *Epc, double *dEpcdT, double *fec, double *ftc, DOUBLEVECTOR *ftcl, double *LE_c, double *dLEdT_c){

	double L;

	//surface resistance
	if(nsnow>0){
		*RG=1.0;
	}else{
		*RG=Eg_Epot(Ratm, theta, res, sat);
		if(*RG!=*RG) printf("NOvalue in evaporation sl: Ratm:%f theta:%f thetar:%f thetasat:%f\n",Ratm, theta, res, sat);
	}
	//latent heat flux from bare sl surfaces
	L=latent(Ts, Levap(Ts));
	*LE_s=(*RG)*E*L;
	*dLEdT_s=(*RG)*dEdT*L;
	if(*LE_s!=*LE_s || *dLEdT_s!=*dLEdT_s){
		printf("NO VALUE in evaporation LEs:%f DLEDT:%f L:%f RG:%F E:%f\n",*LE_s,*dLEdT_s,L,*RG,E);
		stop_execution();
	}
	//set at 0 canopy fluxes
	*Evc=0.0;	//evaporation canopy
	*Dc=0.0;	//drip canopy
	*Msc=0.0;	//max storage canopy
	*Epc=0.0;	//potential evaporation canopy
	*dEpcdT=0.0;
	*fec=0.0;
	*ftc=0.0;
	initialize_doublevector(ftcl,0.0);
	*LE_c=0.0;	//latent heat flux from canopy surfaces
	*dLEdT_c=0.0;

}
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
void evaporation_canopy(long r, long c, double Ptot, double *theta, double Ratm, double rho, double Tv, double Ta, double Qa, double Pa, double SW, double LAI,
	double *land, double *root, double *Wc, double *LE, double *dLEdT, double *Evc, double *Dc, double *Msc, double *Epc, double *dEpcdT, double *fec, double *ftc,
	double *ftcl, PAR *par){

	double Lv, fec_i, fec_w, fW, dt=par->Dt ;

	*Msc=0.2*LAI;	//Maximum storage in canopy [mm] from Dickinson
	*Wc+=Ptot;		//increase water on canopy
	if(*Wc>*Msc){
		*Dc=*Wc-*Msc;	//canopy drip
		*Wc=*Msc;
	}
	if(*Msc>0){
		fW=*Wc/(*Msc);
	}else{
		fW=0.0;
	}

	canopy_fluxes(Ratm, rho, Tv, Ta, Qa, Pa, SW, fW, theta, LAI, land, root, Epc, dEpcdT, fec, ftc, ftcl);

	Lv=Levap(Tv);
	*Wc-=(*fec)*(*Epc)*dt;	//evaporation from canopy
	if(*Wc<0){
		//decrease evaporation and maintain Wc=0
		*fec+=(*Wc/(dt*(*Epc)));
		*Wc=0.0;
	}
	*Evc=(*fec)*(*Epc)*dt;

	//very rough partition of storage on leaves between water and snow (to be improved)
	part_snow(*fec, &fec_w, &fec_i, Ta, par->T_rain, par->T_snow);

	//LE evapotranspiration in W/m2
	*LE=(*Epc)*( (Lv+Lf)*fec_i + Lv*fec_w + Lv*(*ftc) );
	*dLEdT=(*dEpcdT)*( (Lv+Lf)*fec_i + Lv*fec_w + Lv*(*ftc) );
	if((*LE)!=(*LE)) printf("NOvalue in evaporation_canopy: r:%ld c:%ld Epc:%f fec:%f fec_i:%f fec_w:%f ftc:%f Wc:%f Msc:%f Dc:%f Tv:%f Ta:%f fW:%f Qa:%f rho:%f SW:%f Lv:%f\n",r,c,*Epc,*fec,fec_i,fec_w,*ftc,*Wc,*Msc,*Dc,Tv,Ta,fW,Qa,rho,SW,Lv);
}
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void check_errors(long r, long c, long n, DOUBLEVECTOR *adi, DOUBLEVECTOR *ad, DOUBLEVECTOR *ads, DOUBLEVECTOR *b, DOUBLEVECTOR *e, double *T, SHORTVECTOR *mf){

	long l;
	short occ=0;

	for(l=1;l<=n;l++){
		if(e->co[l]!=e->co[l] && e->co[1]>50) occ=1;
	}

	if(occ==1){
		printf("NOvalue in Energy Balance: r:%ld c:%ld\n",r,c);
		printf("l:%ld adi:--- ad:%f ads:%f b:%f e:%f T:%f mf:%ld\n",1,ad->co[1],ads->co[1],b->co[1],e->co[1],T[1],mf->co[1]);
		for(l=2;l<=n-1;l++){
			printf("l:%ld adi:%f ad:%f ads:%f b:%f e:%f T:%f mf:%ld\n",l,adi->co[l-1],ad->co[l],ads->co[l],b->co[l],e->co[l],T[l],mf->co[l]);
		}
		printf("l:%ld adi:%f ad:%f ads:--- b:%f e:%f T:%f mf:%ld\n",n,adi->co[n-1],ad->co[n],b->co[n],e->co[n],T[n],mf->co[n]);
	}
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void rad_snow_absorption(long r, long c, DOUBLEVECTOR *frac, double R, SNOW *snow){

	double z=0.0, res=R, rho, k;
	long l, m;

	for(l=snow->lnum->co[r][c];l>=1;l--){
		m=snow->lnum->co[r][c]-l+1;
		z+=0.001*snow->Dzl->co[l][r][c];
		rho=(snow->w_ice->co[l][r][c]+snow->w_liq->co[l][r][c])/(0.001*snow->Dzl->co[l][r][c]);
		k=rho/3.0+50.0;
		//k*=1.E10;
		frac->co[m]=res-R*exp(-k*z);
		res=R*exp(-k*z);
		if(l==1){
			frac->co[m]+=res;
			res=0.0;
		}
	}

	frac->co[snow->lnum->co[r][c]+1]=res;

}


/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void snow_fluxes_H(SNOW *snow, DOUBLEMATRIX *Ts, DOUBLEMATRIX *H, DOUBLEMATRIX *Z, DOUBLEMATRIX *Ta, TIMES *times, PAR *par, double *VSFA, double *Hv){

	long r,c,nr,nc,cont=0,contT=0,rmin=0,cmin=0,rmax=0,cmax=0;
	double Hsca=0.0,Hsfa=0.0,Hmax=-10000.0,Hmin=10000.0,Hrsm=0.0,Tsmax=-300.0,Tsmin=300.0,Tsmean=0.0,Tamean=0.0,SFA;
	double Hvmean=0.0,Hvmax=-10000.0,Hvmin=10000.0,Hvrsm=0.0,Tsvmean=0.0;
	double JD;
	long d2,mo2,y2,h2,mi2;
	FILE *f;

	nr=Z->nrh;
	nc=Z->nch;

	for(r=1;r<=nr;r++){
		for(c=1;c<=nc;c++){
			if(Z->co[r][c]!=UV->V->co[2]){
				if(snow->Dzl->co[1][r][c]>0){
					//printf("%e %ld %ld %ld %ld\n",snow->Dzl->co[1][r][c],snow->type->co[r][c],snow->lnum->co[r][c],r,c);
					cont++;
					Hsca+=H->co[r][c];
				}else{
					Hsfa+=H->co[r][c];
					Tsmean+=Ts->co[r][c];
					if(H->co[r][c]>Hmax) Hmax=H->co[r][c];
					if(H->co[r][c]<Hmin) Hmin=H->co[r][c];
					if(Ts->co[r][c]>Tsmax) Tsmax=Ts->co[r][c];
					if(Ts->co[r][c]<Tsmin) Tsmin=Ts->co[r][c];
				}
				Tamean+=Ta->co[r][c];
				if(Ts->co[r][c]>Tfreezing && Ts->co[r][c]>Ta->co[r][c] && snow->Dzl->co[1][r][c]==0){
					contT++;
					Hvmean+=H->co[r][c];
					Tsvmean+=Ts->co[r][c];
					if(H->co[r][c]>Hvmax) Hvmax=H->co[r][c];
					if(H->co[r][c]<Hvmin) Hvmin=H->co[r][c];
				}
			}
		}
	}

	if(cont>0) Hsca/=(double)cont;
	if(cont<par->total_pixel) Hsfa/=(double)(par->total_pixel-cont);
	if(cont<par->total_pixel) Tsmean/=(double)(par->total_pixel-cont);
	Tamean/=(double)par->total_pixel;
	SFA=(par->total_pixel-cont)/(double)par->total_pixel;

	if(contT>0){
		Hvmean/=(double)contT;
		Tsvmean/=(double)contT;
	}

	if(cont<par->total_pixel){
		for(r=1;r<=nr;r++){
			for(c=1;c<=nc;c++){
				if(Z->co[r][c]!=UV->V->co[2]){
					if(snow->Dzl->co[1][r][c]==0.0){
						Hrsm+=pow(H->co[r][c]-Hsfa,2.0)/(double)(par->total_pixel-cont);
						if(Ts->co[r][c]==Tsmin){
							rmin=r;
							cmin=c;
						}
						if(Ts->co[r][c]==Tsmax){
							rmax=r;
							cmax=c;
						}
					}
				}
			}
		}
		Hrsm=pow(Hrsm,0.5);
	}

	if(contT>0){
		for(r=1;r<=nr;r++){
			for(c=1;c<=nc;c++){
				if(Z->co[r][c]!=UV->V->co[2]){
					if(Ts->co[r][c]>Tfreezing && Ts->co[r][c]>Ta->co[r][c] && snow->Dzl->co[1][r][c]==0) Hvrsm+=pow(H->co[r][c]-Hvmean,2.0)/(double)contT;
				}
			}
		}
		Hvrsm=pow(Hvrsm,0.5);
	}

	date_time(times->time, par->year0, par->JD0, 0.0, &JD, &d2, &mo2, &y2, &h2, &mi2);
	f=fopen(join_strings(files->co[fHpatch]+1,textfile),"a");
	write_date(f, d2, mo2, y2, h2, mi2);
	fprintf(f,",%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,MIN:%ld %ld,MAX:%ld %ld\n",JD+(double)(daysfrom0(y2)),SFA,1-SFA,Hsca,Hsfa,Hmax,Hmin,Hrsm,Tsmean,Tsmin,Tsmax,Tamean,contT/(double)par->total_pixel,Hvmean,Hvmin,Hvmax,Hvrsm,Tsvmean,rmin,cmin,rmax,cmax);
	fclose(f);

	*Hv=Hvmean;
	*VSFA=contT/(double)par->total_pixel;

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

double adv_efficiency(double SFA){

	double Y;

	Y=-0.043+1.0859*exp(-2.9125*SFA);

	return(Y);
}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/

void root(double d, double *D, double *root_fraction){

	long l;
	double z=0.0;

	for(l=1;l<=Nl;l++){
		z+=D[l];
		if(d>z){
			root_fraction[l]=D[l]/d;
		}else{
			if(d>(z-D[l])){
				root_fraction[l]=(d-(z-D[l]))/d;
			}else{
				root_fraction[l]=0.0;
			}
		}
	}

}

/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/
/******************************************************************************************************************************************/


