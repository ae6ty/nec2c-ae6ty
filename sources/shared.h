#ifndef SHARED_H
#define SHARED_H	1

#include "nec2c.h"

/*------------------------------------------------------------------------*/

// ae6ty 2-17-22
extern int matrixMode;

// ae6ty 12-10-21
extern int YYNums[];
extern int numYYNums;

// ae6ty 12-7-21
#define Complex complex double

/* common  /crnt/ */
extern crnt_t crnt;

/* common  /dataj/ */
extern dataj_t dataj;

/* common  /data/ */
extern data_t data;

/* pointers to input/output files */
extern FILE *input_fp, *output_fp, *plot_fp;

//ae6ty 12-7-21 added Yflag and Q flag
extern int Yflag;
#define notQfprintf  if (!Qflag) fprintf
extern int Qflag;

/* common  /fpat/ */
extern fpat_t fpat;

/*common  /ggrid/ */
extern ggrid_t ggrid;

/* common  /gnd/ */
extern gnd_t gnd;

/* common  /gwav/ */
extern gwav_t gwav;

/* common  /incom/ */
extern incom_t incom;

/* common  /matpar/ */
extern matpar_t matpar;

/* common  /netcx/ */
extern netcx_t netcx;

/* common  /plot/ */
extern plot_t plot;

/* common  /save/ */
extern save_t save;

/* common  /segj/ */
extern segj_t segj;

/* common  /smat/ */
extern smat_t smat;

/* common  /tmi/ */
extern tmi_t tmi;

/* common  /vsorc/ */
extern vsorc_t vsorc;

/* common  /yparm/ */
extern yparm_t yparm;

/* common  /zload/ */
extern zload_t zload;

/*------------------------------------------------------------------------*/

#endif
