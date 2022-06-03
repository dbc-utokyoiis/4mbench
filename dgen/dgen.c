/* dgen.c */

#include "dgen.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <limits.h>


/*
 * Declaration
 */

#define NBUF 512
#define TSBASE 1648771200 /* 2022-04-01 09:00:00 at Tokyo */
#define TSEQUIPMENTWARMUP (30*60)
#define NPACKAGE_TO_PRODUCE 100
#define NITERATION 1

int is_millisecond = 0;

struct worker_t {
  int wid;          /* worker id */
  int lid;          /* line id */
  char wname[NBUF]; /* workername */
};
#define NWORKER 9
struct worker_t worker[NWORKER];
int nworker = 0;

struct equipment_t {
  int eid;          /* equipment id */
  int lid;          /* line id */
  char ename[NBUF]; /* equipment name */
};
#define NEQUIPMENT 9
struct equipment_t equipment[NEQUIPMENT];
int nequipment = 0;
char *ename2[NEQUIPMENT] =
  {
   "DOUGH ADJUSTMENT EQUIPMENT",
   "BAKING EQUIPMENT",
   "FINISHING EQUIPMENT",
   "BOXING EQUIPMENT",
   "WRAPPING EQUIPMENT",
   "PACKING EQUIPMENT",
   "PRINTING EQUIPMENT",
   "PALLETIZING EQUIPMENT",
   "STORAGING EQUIPMENT"
  };

struct procedure_t {
  int pid;          /* procedure id */
  char pname[NBUF]; /* procedure name */
};
#define NPROCEDURE 9
struct procedure_t procedure[NPROCEDURE];
int nprocedure = 0;
char *pname2[NPROCEDURE] =
  {
   "DOUGH ADJUSTMENT",
   "BAKING",
   "FINISHING",
   "BOXING",
   "WRAPPING",
   "PACKING",
   "PRINTING",
   "PALLETIZING",
   "STORAGING"
  };

struct operationlog_t {
  long olid;       /* operation log id */
  int lid;         /* line id */
  int wid;         /* worker id */
  int eid;         /* equipment id */
  int pid;         /* process id */
  double tsbegin;  /* timestamp in starting */
  double tsend;    /* timestamp in ending */
};
#define NOPERATIONLOG (1024 * 1024 * 100)
struct operationlog_t *operationlog;
long noperationlog = 0;

struct materiallog_t {
  long mlid;       /* material log id */
  int lid;         /* line id */
  int mtype;       /* material type */
  int olid_src;    /* source operation log */
  int olid_dst;    /* destination operation log */
};
#define NMATERIALLOG (1024 * 1024 * 100)
struct materiallog_t *materiallog;
long nmateriallog = 0;
#define NMTYPE 20
char *mtypename[NMTYPE] =
  {
   "MT00",
   "MT01",
   "MT02",
   "MT03",
   "MT04",
   "MT05",
   "MT06",
   "MT07",
   "MT08",
   "MT09",
   "",
   "",
   "",
   "MT13",
   "",
   "MT15",
   "",
   "MT17",
   "",
   ""
  };


double processinglatency[NPROCEDURE] =
  {
   0.1,
   0.1,
   0.1,
   1.0,
   0.1,
   1.0,
   0.1,
   1.0,
   1.0
  };

double errorrate[NPROCEDURE] =
  {
   0.001,
   0.0,
   0.0,
   0.0,
   0.001,
   0.0,
   0.001,
   0.0,
   0.0
  };

int nmaterial_to_pack[NPROCEDURE] =
  {
   1,
   1,
   1,
   6,
   1,
   24,
   1,
   48,
   1
  };

int nmaterial_to_append[NPROCEDURE] =
  {
   0,
   0,
   0,
   1,
   0,
   1,
   0,
   1,
   0
  };

#define NMSENSOR 6
#define MSENSOR_WGHT 0
#define MSENSOR_DIMX 1
#define MSENSOR_DIMY 2
#define MSENSOR_DIMZ 3
#define MSENSOR_SHPS 4
#define MSENSOR_TEMP 5
struct msensormodel_t {
  int modeltype;
  double median;
  double errorrange;
};
#define MODELTYPE_FULLNDIST 11
#define MODELTYPE_HALFNDIST 21
struct msensormodel_t msensormodel[NMTYPE][NMSENSOR] =
  {
   { /* MT00 */
    {MODELTYPE_FULLNDIST, 100.0, 2.0},  /* MSENSOR_WGHT */
    {MODELTYPE_FULLNDIST, 5.0,   0.5},  /* MSENSOR_DIMX */
    {MODELTYPE_FULLNDIST, 5.0,   0.5},  /* MSENSOR_DIMY */
    {MODELTYPE_FULLNDIST, 1.0,   0.1},  /* MSENSOR_DIMZ */
    {MODELTYPE_HALFNDIST, 1.00,  0.02}, /* MSENSOR_SHPS */
    {MODELTYPE_FULLNDIST, 3.5,   1.5}   /* MSENSOR_TEMP */
   },
   { /* MT01 */
    {MODELTYPE_FULLNDIST, 100.0, 2.0},
    {MODELTYPE_FULLNDIST, 5.0,   0.5},
    {MODELTYPE_FULLNDIST, 5.0,   0.5},
    {MODELTYPE_FULLNDIST, 1.0,   0.1},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 3.5,   1.5}
   },
   { /* MT02 */
    {MODELTYPE_FULLNDIST, 90.0,  1.0},
    {MODELTYPE_FULLNDIST, 4.5,   0.4},
    {MODELTYPE_FULLNDIST, 4.5,   0.4},
    {MODELTYPE_FULLNDIST, 0.0,   0.09},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 180.0, 2.0}
   },
   { /* MT03 */
    {MODELTYPE_FULLNDIST, 90.0,  1.0},
    {MODELTYPE_FULLNDIST, 4.5,   0.4},
    {MODELTYPE_FULLNDIST, 4.5,   0.4},
    {MODELTYPE_FULLNDIST, 0.0,   0.09},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   },
   { /* MT04 */
    {MODELTYPE_FULLNDIST, 600.0, 7.0},
    {MODELTYPE_FULLNDIST, 10.0,  0.1},
    {MODELTYPE_FULLNDIST, 5.0,   0.1},
    {MODELTYPE_FULLNDIST, 5.0,   0.1},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   },
   { /* MT05 */
    {MODELTYPE_FULLNDIST, 600.0, 7.0},
    {MODELTYPE_FULLNDIST, 10.0,  0.1},
    {MODELTYPE_FULLNDIST, 5.0,   0.1},
    {MODELTYPE_FULLNDIST, 5.0,   0.1},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   },
   { /* MT06 */
    {MODELTYPE_FULLNDIST, 14400.0, 160.0},
    {MODELTYPE_FULLNDIST, 10.0,  0.1},
    {MODELTYPE_FULLNDIST, 31.0,  0.6},
    {MODELTYPE_FULLNDIST, 21.0,  0.4},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   },
   { /* MT07 */
    {MODELTYPE_FULLNDIST, 14400.0, 160.0},
    {MODELTYPE_FULLNDIST, 10.0,  0.1},
    {MODELTYPE_FULLNDIST, 31.0,  0.6},
    {MODELTYPE_FULLNDIST, 21.0,  0.4},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   },
   { /* MT08 */
    {MODELTYPE_FULLNDIST, 692000.0, 7680.0},
    {MODELTYPE_FULLNDIST, 42.0,  0.4},
    {MODELTYPE_FULLNDIST, 95.0,  1.8},
    {MODELTYPE_FULLNDIST, 85.0,  1.6},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   },
   { /* MT09 */
    {MODELTYPE_FULLNDIST, 692000.0, 7680.0},
    {MODELTYPE_FULLNDIST, 42.0,  0.4},
    {MODELTYPE_FULLNDIST, 95.0,  1.8},
    {MODELTYPE_FULLNDIST, 85.0,  1.6},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 20.0,  15.0}
   }
  };
#define MSENSOR_ERRORSCALE 5

struct equipmentlog_t {
  long elid;       /* equipment log id */
  int lid;         /* line id */
  int eid;         /* equipment id */
  double ts;       /* timestamp */
  int esensor;     /* sensor */
  int ereading;    /* reading */
};
#define NEQUIPMENTLOG (3600 * 2 * 1024)
struct equipmentlog_t *equipmentlog;
long nequipmentlog /* = 0 */;
int esensors[NEQUIPMENT][2] =
  {
   {0, 9}, /* 0 */
   {1, 9}, /* 1 */
   {2, 9}, /* 2 */
   {3, 9}, /* 3 */
   {4, 9}, /* 4 */
   {5, 9}, /* 5 */
   {6, 9}, /* 6 */
   {7, 9}, /* 7 */
   {8, 9}, /* 8 */
  };

struct esensormodel_t {
  char *sensorname;
  int modeltype;
  double median;
  double errorrange;
};
struct esensormodel_t esensormodel[NEQUIPMENT + 1] =
  {
   {"PRESSURE",    MODELTYPE_FULLNDIST, 125.0, 15.0},  /* 0 */
   {"TEMPERATURE", MODELTYPE_FULLNDIST, 180.0, 2.0},   /* 1 */
   {"TEMPERATURE", MODELTYPE_FULLNDIST, 47.5,  7.5},   /* 2 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 3 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 4 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 5 */
   {"INK LEFT",    MODELTYPE_FULLNDIST, 55.0,  45.0},  /* 6 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 7 */
   {"TEMPERATURE", MODELTYPE_FULLNDIST, 20.0,  15.0},  /* 8 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.01,  0.02}   /* 9 */
  };
#define ESENSOR_ERRORSCALE 5

#define NCMNTWORD 25
char *cmntword[NCMNTWORD] =
  {
   /* frequently used nouns, https://en.wikipedia.org/wiki/Most_common_words_in_English */
   "time",
   "person",
   "year",
   "way",
   "day",
   "thing",
   "man", 
   "world", 
   "life", 
   "hand",
   "part", 
   "child", 
   "eye", 
   "woman", 
   "place", 
   "work", 
   "week", 
   "case", 
   "point", 
   "government", 
   "company", 
   "number", 
   "group", 
   "problem", 
   "fact"
  };

/*
 * put_operationlog(), put_materiallog()
 */

long put_operationlog(int lid,
		      int wid, int eid, int pid,
		      double tsbegin, double tsend)
{  
  if(noperationlog >= NOPERATIONLOG){ fprintf(stderr, "overflow (operationlog)\n"); exit(EXIT_FAILURE); }
  
  operationlog[noperationlog].olid = noperationlog;
  operationlog[noperationlog].lid = lid;
  operationlog[noperationlog].wid = wid;
  operationlog[noperationlog].eid = eid;
  operationlog[noperationlog].pid = pid;
  operationlog[noperationlog].tsbegin = tsbegin;
  operationlog[noperationlog].tsend = tsend;
  noperationlog++;
  return(operationlog[noperationlog-1].olid);
}

void update_operationlog_ts(long olid,
			    double tsbegin, double tsend)
{
  if(olid != operationlog[olid].olid){ fprintf(stderr, "overflow (operationlog)\n");  exit(EXIT_FAILURE); }

  operationlog[olid].tsbegin = tsbegin;
  operationlog[olid].tsend = tsend;
}
			    
long put_materiallog(int lid,
		     int mtype, long olid_src, long olid_dst)
{
  if(nmateriallog >= NMATERIALLOG){ fprintf(stderr, "overflow (materiallog)\n");  exit(EXIT_FAILURE); }

  materiallog[nmateriallog].mlid = nmateriallog;
  materiallog[nmateriallog].lid = lid;
  materiallog[nmateriallog].mtype = mtype;
  materiallog[nmateriallog].olid_src = olid_src;
  materiallog[nmateriallog].olid_dst = olid_dst; 
  nmateriallog++;
  return(materiallog[nmateriallog-1].mlid);
}

/*
 * operation output queue
 */

struct operationoutput_t {
  long mlid;
  long olid;
  int lid;
  int wid;
  int eid;
  int pid;
  double ts;
};

struct operationoutput_t *operationoutput[NPROCEDURE];
long operationoutput_to_put[NPROCEDURE]; /* reference to put */
long operationoutput_to_get[NPROCEDURE]; /* reference to get */

void init_operationoutput(void)
{
  int i;
  for(i=0; i<NPROCEDURE; i++)
    if((operationoutput[i] = malloc(sizeof(struct operationoutput_t) * NMATERIALLOG)) == NULL){
      perror("malloc");
      exit(EXIT_FAILURE);
    }
}

void fin_operationoutput(void)
{
  int i;
  for(i=0; i<NPROCEDURE; i++)
    free(operationoutput[i]);
}

void put_operationoutput(long mlid, long olid,
			 int lid, int wid, int eid, int pid,
			 double ts)
{
  if(operationoutput_to_put[pid] >= NMATERIALLOG){ fprintf(stderr, "overflow (operationoutput)");  exit(EXIT_FAILURE); }
  
  operationoutput[pid][operationoutput_to_put[pid]].mlid = mlid;
  operationoutput[pid][operationoutput_to_put[pid]].olid = olid;
  operationoutput[pid][operationoutput_to_put[pid]].lid = lid;
  operationoutput[pid][operationoutput_to_put[pid]].wid = wid;
  operationoutput[pid][operationoutput_to_put[pid]].eid = eid;
  operationoutput[pid][operationoutput_to_put[pid]].pid = pid;
  operationoutput[pid][operationoutput_to_put[pid]].ts = ts;
  operationoutput_to_put[pid]++;
}

long get_navailable(int pid)
{
  return(operationoutput_to_put[pid] - operationoutput_to_get[pid]);
}

struct operationoutput_t *get_operationoutput(int pid)
{
  if(operationoutput_to_get[pid] >= NMATERIALLOG){ fprintf(stderr, "overflow (operationoutput)");  exit(EXIT_FAILURE); }
  if(operationoutput_to_get[pid] >= operationoutput_to_put[pid]){ return(NULL); }
  struct operationoutput_t *p = &operationoutput[pid][operationoutput_to_get[pid]];
  operationoutput_to_get[pid]++;
  return(p);
}

/*
 * hash (based on FNV hash algorithm)
 */

#define FNV_PRIME 16777619U
#define FNV_BASE  2166136261U

unsigned int hash32_mlid_lid(long mlid, int lid, int key)
{
  unsigned int h;

  h = FNV_BASE;
  h = (FNV_PRIME * h) ^ ((mlid >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 24) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 32) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 40) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 48) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((mlid >> 56) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 24) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 24) & 0x000000ff);
  return(h);
}

unsigned int hash32_t_lid(long t, int lid, int key)
{
  unsigned int h;

  h = FNV_BASE;
  h = (FNV_PRIME * h) ^ ((t >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 24) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 32) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 40) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 48) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((t >> 56) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((lid >> 24) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((key >> 24) & 0x000000ff);
  return(h);
}

/*
 * generate comment
 */

void generate_olcomment(char *s, int l, long olid, int lid)
{
  snprintf(s, l,
	   "%s %s %s %s %s",
	   cmntword[hash32_mlid_lid(olid, lid, 100) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 101) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 102) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 103) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 1044) % NCMNTWORD]);
}

void generate_mlcomment(char *s, int l, long mlid, int lid)
{
  snprintf(s, l,
	   "%s %s %s %s %s",	   
	   cmntword[hash32_mlid_lid(mlid, lid, 201) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(mlid, lid, 202) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(mlid, lid, 203) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(mlid, lid, 204) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(mlid, lid, 205) % NCMNTWORD]);
}

/*
 * generate sensor reading
 */

inline double ndist_mlid_lid(double average, double deviation, long mlid, int lid, int key)
{
  double x, y, z = 0.0;
  x = (double)(hash32_mlid_lid(mlid, lid, key)) / (UINT_MAX + 1.0);
  y = (double)(hash32_mlid_lid(mlid, lid, key << 7)) / (UINT_MAX + 1.0);
  if(x != 0.0 && y != 0.0)
    z = sqrt(-2 * log(x)) * cos(2 * M_PI * y); /* Box-Muller approximation */
  return(average + deviation * z);
}

double generate_msensor_reading(int mtype, int msensor, long mlid, int lid)
{
  double r = 0.0;
  
  switch(msensormodel[mtype][msensor].modeltype){
  case MODELTYPE_FULLNDIST:
    r = ndist_mlid_lid(msensormodel[mtype][msensor].median,
		       msensormodel[mtype][msensor].errorrange / MSENSOR_ERRORSCALE,
		       mlid, lid, msensor);
    break;
  case MODELTYPE_HALFNDIST:
    r = ndist_mlid_lid(msensormodel[mtype][msensor].median,
		       msensormodel[mtype][msensor].errorrange / MSENSOR_ERRORSCALE,
		       mlid, lid, msensor);
    if(r > msensormodel[mtype][msensor].median)
      r = msensormodel[mtype][msensor].median;
    break;
  };
  
  return(r);
}

inline double ndist_t_lid(double average, double deviation, int t, int lid, int key)
{
  double x, y, z = 0.0;
  x = (double)(hash32_t_lid(t, lid, key)) / (UINT_MAX + 1.0);
  y = (double)(hash32_t_lid(t, lid, key << 7)) / (UINT_MAX + 1.0);
  if(x != 0.0 && y != 0.0)
    z = sqrt(-2 * log(x)) * cos(2 * M_PI * y); /* Box-Muller approximation */
  return(average + deviation * z);
}

double generate_esensor_reading(int esensor, long t, int lid)
{
  double r = 0.0;
  
  switch(esensormodel[esensor].modeltype){
  case MODELTYPE_FULLNDIST:
    r = ndist_t_lid(esensormodel[esensor].median,
		    esensormodel[esensor].errorrange / ESENSOR_ERRORSCALE,
		    t, lid, esensor);
    break;
  case MODELTYPE_HALFNDIST:
    r = ndist_t_lid(esensormodel[esensor].median,
		    esensormodel[esensor].errorrange / ESENSOR_ERRORSCALE,
		    t, lid, esensor);
    if(r > esensormodel[esensor].median)
      r = esensormodel[esensor].median;
    break;
  };
  
  return(r);
}

/*
 * put_equipmentlog()
 */

long put_equipmentlog(int lid,
		      int eid,
		      double ts,
		      int esensor, int ereading)
{  
  if(nequipmentlog >= NEQUIPMENTLOG){ fprintf(stderr, "overflow (equipmentlog)\n"); exit(EXIT_FAILURE); }
  
  equipmentlog[nequipmentlog].elid = nequipmentlog;
  equipmentlog[nequipmentlog].lid = lid;
  equipmentlog[nequipmentlog].eid = eid;
  equipmentlog[nequipmentlog].ts  = ts;
  equipmentlog[nequipmentlog].esensor  = esensor;
  equipmentlog[nequipmentlog].ereading = ereading;
  nequipmentlog++;
  return(equipmentlog[nequipmentlog-1].elid);
}

/*
 * Initialize
 */

void init(int lid)
{
  int i;

  /* generate WORKER */
  printf("Generating %s for lid=%u ...\n", "WORKER", lid);
  nworker = NWORKER;
  for(i=0; i<NWORKER; i++){
    worker[i].wid = i;
    worker[i].lid = lid;
    snprintf(worker[i].wname, NBUF-1, "%s%06u%06d", "WORKER", lid, i);
  }

  /* generate EQUIPMENT */
  printf("Generating %s for lid=%u ...\n", "EQUIPMENT", lid);
  nequipment = NEQUIPMENT;
  for(i=0; i<nequipment; i++){
    equipment[i].eid = i;
    equipment[i].lid = lid;
    snprintf(equipment[i].ename, NBUF-1, "%s%06u%06d (%s)", "EQUIPMENT", lid, i, ename2[i]);
  }

  /* generate PROCEDURE */
  printf("Generating %s for lid=%u ...\n", "PROCEDURE", lid);
  nprocedure = NPROCEDURE;
  for(i=0; i<nprocedure; i++){
    procedure[i].pid = i;
    snprintf(procedure[i].pname, NBUF-1, "%s%06d (%s)", "PROCEDURE", i, pname2[i]);
  }

  /* allocate OPERATIONLOG */
  printf("Allocating %s for lid=%u ...\n", "OPERATIONLOG", lid);
  if((operationlog = malloc(sizeof(struct operationlog_t) * NOPERATIONLOG)) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  /* allocate MATERIALLOG */
  printf("Allocating %s for lid=%u ...\n", "MATERIALLOG", lid);
  if((materiallog = malloc(sizeof(struct materiallog_t) * NMATERIALLOG)) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  /* allocate EQUIPMENTLOG */
  printf("Allocating %s for lid=%u ...\n", "EQUIPMENTLOG", lid);
  if((equipmentlog = malloc(sizeof(struct equipmentlog_t) * NEQUIPMENTLOG)) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  init_operationoutput();
}

/*
 * Simulate to generate OPERATIONLOG and MATERIALLOG
 */

double tsbackup = 0.0;

void sim_equipmentlog(int lid,
		      double ts_latest)
{
  int i;
  long t;
  long t_latest = ts_latest + TSEQUIPMENTWARMUP;

  if(nequipmentlog)
    t = equipmentlog[nequipmentlog-1].ts;
  else
    t = -TSEQUIPMENTWARMUP;

#if 0
  printf("%ld\n", nequipmentlog);
  printf("%ld - %ld\n", t, t_latest);
#endif
  
  for(; t <= t_latest; t++){
    for(i = 0; i < NEQUIPMENT; i++){
      put_equipmentlog(lid,
		       i,
		       (double)t,
		       i,
		       generate_esensor_reading(i, t, lid));
      put_equipmentlog(lid,
		       i,
		       (double)t,
		       NEQUIPMENT,
		       generate_esensor_reading(NEQUIPMENT, t, lid));
    }
  }
}

void sim(int lid, int np)
{
  int i, j;
  double ts;
  long olid_src, olid_dst, mlid = 0;
  struct operationoutput_t *p;
  int wid, eid, pid, mtype;
  int n;
  double ts_max = 0.0;
  
  printf("Generating %s for lid=%u ...\n", "OPERATIONLOG and MATERIALLOG", lid);

  /* Calculate initial production amount */

  n = 1;
  for(i=0;i<NPROCEDURE;i++){
    n *= nmaterial_to_pack[i];
  }
  n *= np;

  /*
   * #0 (PROCEDURE00000) takes MT00
   */

  ts = tsbackup;
  wid = 0, eid = 0, pid = 0, mtype = 0;
  for(i=0; i<n; i++){
    olid_src = -1;
    olid_dst = put_operationlog(lid, wid, eid, pid, ts, ts + processinglatency[pid]);
    mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
    if(((double)rand() / (RAND_MAX + 1.0)) >= errorrate[pid])
      /* Only qualified materials go to the next step */
      put_operationoutput(mlid, olid_dst,
			  lid, wid, eid, pid,
			  ts + processinglatency[pid]);
    ts += processinglatency[pid];
    if(ts > ts_max){ ts_max = ts; }
  } /* for(i) */
  tsbackup = ts;
  
  /*
   * #{1...9} (PROCEDURE{00001...00009}) takes MT{01...09}
   */

  for(i=0;i<NPROCEDURE;i++){
    wid = i, eid = i, pid = i, mtype = i;
    ts = 0;
    while(1){
      if(get_navailable(pid - 1) < nmaterial_to_pack[pid]){ break; }
      olid_dst = put_operationlog(lid, wid, eid, pid, ts, ts + processinglatency[pid]);
      for(j=0; j<nmaterial_to_pack[pid]; j++){
	if((p = get_operationoutput(pid - 1)) == NULL){
	  fprintf(stderr, "oprationlog broken.\n");
	  exit(EXIT_FAILURE);
	}
	olid_src = p->olid;
	ts = p->ts;
	mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
      }
      for(j=0; j<nmaterial_to_append[pid]; j++){
	mlid = put_materiallog(lid, mtype + 10, -1, olid_dst);
      }
      update_operationlog_ts(olid_dst, ts, ts + processinglatency[pid]);
      if(((double)rand() / (RAND_MAX + 1.0)) >= errorrate[pid])
	/* Only qualified materials go to the next step */
	put_operationoutput(mlid, olid_dst,
			    lid, wid, eid, pid,
			    ts + processinglatency[pid]);
      if(ts > ts + processinglatency[pid]){ ts_max = ts + processinglatency[pid]; }
    } /* while(1) */
  } /* for(i) */

  /*
   *  #last (DEPOSITORY) takes MT09
   */

  i=NPROCEDURE;
  wid = i, eid = i, pid = i, mtype = i;
  ts = 0;
  while(1){
    if(get_navailable(pid - 1) < 1){ break; }
    olid_dst = -1;
    for(j=0; j<1; j++){
      if((p = get_operationoutput(pid - 1)) == NULL){
	fprintf(stderr, "oprationlog broken.\n");
	exit(EXIT_FAILURE);
      }
      olid_src = p->olid;
      mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
    }
  } /* while(1) */


  printf("Generating %s for lid=%u ...\n", "EQUIPMENTLOG", lid);

  sim_equipmentlog(lid, ts_max);
}

/*
 * Unload
 */

void unload(int lid)
{
  int i;
  FILE *fp;
  char fn[NBUF];
  time_t t1, t2;
  char dt1[NBUF], dt2[NBUF];
  char cmnt[NBUF];
  
  /* unload WORKER */
  snprintf(fn, NBUF-1, "%s%06u.dat", "WORKER", lid);
  printf("Unloading %s (%d records) for lid=%u ...\n", "WORKER", nworker, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  fprintf(fp, "# WID, LID, WNAME\n");
  for(i=0; i<nworker; i++){
    assert(lid == worker[i].lid);
    fprintf(fp, "%d|%d|%s\n",
	    worker[i].wid, worker[i].lid,
	    worker[i].wname);
  }
  fclose(fp);

  /* unload EQUIPMENT */
  snprintf(fn, NBUF-1, "%s%06u.dat", "EQUIPMENT", lid);
  printf("Unloading %s (%d records) for lid=%u ...\n", "EQUIPMENT", nequipment, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  fprintf(fp, "# EID, LID, ENAME\n");
  for(i=0; i<nequipment; i++){
    assert(lid == equipment[i].lid);
    fprintf(fp, "%d|%d|%s\n",
	    equipment[i].eid, equipment[i].lid,
	    equipment[i].ename);
  }
  fclose(fp);

  /* unload PROCEDURE */
  snprintf(fn, NBUF-1, "%s.dat", "PROCEDURE");
  printf("Unloading %s (%d records) ...\n", "PROCEDURE", nprocedure);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  fprintf(fp, "# PID, LID, PNAME\n");
  for(i=0; i<nprocedure; i++)
    fprintf(fp, "%d|%s\n", procedure[i].pid,
	    procedure[i].pname);
  fclose(fp);

  /* unload OPERATIONLOG */
  snprintf(fn, NBUF-1, "%s%06u.dat", "OPERATIONLOG", lid);
  printf("Unloading %s (%ld records) for lid=%u ...\n", "OPERATIONLOG", noperationlog, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  fprintf(fp, "# OLID, LID, WID, EID, PID, TSBEGIN, TSEND, COMMENT\n");
  for(i=0; i<noperationlog; i++){
    assert(lid == operationlog[i].lid);
    t1 = TSBASE + operationlog[i].tsbegin;
    strftime(dt1, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t1));
    t2 = TSBASE + operationlog[i].tsend;
    strftime(dt2, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t2));
    generate_olcomment(cmnt, NBUF-1, operationlog[i].olid, operationlog[i].lid);
    if(is_millisecond)
      fprintf(fp, "%ld|%d|%d|%d|%d|%s.%03u|%s.%03u|%s\n",
	      operationlog[i].olid, operationlog[i].lid,
	      operationlog[i].wid, operationlog[i].eid, operationlog[i].pid,
	      dt1, (unsigned int)(operationlog[i].tsbegin * 1000) % 1000,
	      dt2, (unsigned int)(operationlog[i].tsend   * 1000) % 1000,
	      cmnt);
    else
      fprintf(fp, "%ld|%d|%d|%d|%d|%s|%s|%s\n",
	      operationlog[i].olid, operationlog[i].lid,
	      operationlog[i].wid, operationlog[i].eid, operationlog[i].pid,
	      dt1,
	      dt2,
	      cmnt);
      
  }
  fclose(fp);

  /* unload MATERIALLOG */
  snprintf(fn, NBUF-1, "%s%06u.dat", "MATERIALLOG", lid);
  printf("Unloading %s (%ld records) for lid=%u ...\n", "MATERIALLOG", nmateriallog, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  fprintf(fp, "# MLID, LID, MTYPE, OLID_SRC, OLID_DST, SERIAL, WEIGHT, DIMENSIONX, DIMENSIONY, DIMENSIONZ, SHAPESOCRE, TEMPERATURE, COMMENT\n");
  for(i=0; i<nmateriallog; i++){
    assert(lid == materiallog[i].lid);
    generate_mlcomment(cmnt, NBUF-1, materiallog[i].mlid, materiallog[i].lid);
    fprintf(fp, "%ld|%d|%s|%d|%d|D%06d-L%06d-M%010ld|%09.3f|%09.3f|%09.3f|%09.3f|%09.3f|%09.3f|%s\n",
	    materiallog[i].mlid, materiallog[i].lid,
	    mtypename[materiallog[i].mtype],
	    materiallog[i].olid_src,
	    materiallog[i].olid_dst,
	    0, materiallog[i].lid, materiallog[i].mlid,
	    generate_msensor_reading(materiallog[i].mtype, MSENSOR_WGHT,
				     materiallog[i].mlid, materiallog[i].lid),
	    generate_msensor_reading(materiallog[i].mtype, MSENSOR_DIMX,
				     materiallog[i].mlid, materiallog[i].lid),
	    generate_msensor_reading(materiallog[i].mtype, MSENSOR_DIMY,
				     materiallog[i].mlid, materiallog[i].lid),
	    generate_msensor_reading(materiallog[i].mtype, MSENSOR_DIMZ,
				     materiallog[i].mlid, materiallog[i].lid),
	    generate_msensor_reading(materiallog[i].mtype, MSENSOR_SHPS,
				     materiallog[i].mlid, materiallog[i].lid),
	    generate_msensor_reading(materiallog[i].mtype, MSENSOR_TEMP,
				     materiallog[i].mlid, materiallog[i].lid),
	    cmnt);
  }
  fclose(fp);

  /* unload EQUIPMENTLOG */
  snprintf(fn, NBUF-1, "%s%06u.dat", "EQUIPMENTLOG", lid);
  printf("Unloading %s (%ld records) for lid=%u ...\n", "EQUIPMENTLOG", nequipmentlog, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  fprintf(fp, "# MLID, LID, MTYPE, OLID_SRC, OLID_DST, SERIAL, WEIGHT, DIMENSIONX, DIMENSIONY, DIMENSIONZ, SHAPESOCRE, TEMPERATURE, COMMENT\n");
  for(i=0; i<nequipmentlog; i++){
    assert(lid == equipmentlog[i].lid);
    t1 = TSBASE + equipmentlog[i].ts;
    strftime(dt1, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t1));
    if(is_millisecond)
      fprintf(fp, "%ld|%d|%d|%s.%03u|%s|%d\n",
	      equipmentlog[i].elid, equipmentlog[i].lid, equipmentlog[i].eid,
	      dt1, 0,
	      esensormodel[equipmentlog[i].esensor].sensorname,
	      equipmentlog[i].ereading);
    else
      fprintf(fp, "%ld|%d|%d|%s|%s|%d\n",
	      equipmentlog[i].elid, equipmentlog[i].lid, equipmentlog[i].eid,
	      dt1,
	      esensormodel[equipmentlog[i].esensor].sensorname,
	      equipmentlog[i].ereading);      
  }
  fclose(fp);
}

/*
 * Finalize
 */

void fin(int lid)
{
  fin_operationoutput();

  printf("Releasing %s for lid=%u ...\n", "MATERIALLOG", lid);
  free(materiallog);
  printf("Releasing %s for lid=%u ...\n", "OPERATIONLOG", lid);
  free(operationlog);
}

/* 
 * Main
 */

void print_usage(void)
{
  puts("\
Usage: dgen [options]\n\
Description:\n\
  The 4mbench dataset generator\n\
Options:\n\
  -l <n> : specify LID (line id) (range: 0 to 999999) (default: 0)\n\
  -d <n> : specify the number of simulation days (range: 1 to 1000) (default: 1)\n\
  -n <n> : specify the maximim number of final products to be produced\n\
  -M     : enable millisecond-scale dataaset generation (default: second-scale)\n\
");
}

int main(int argc, char **argv)
{
  int opt;
  int i;
  int lid = 0; /* line id */
  int np = NPACKAGE_TO_PRODUCE; /* package number to produce */
  int ni = NITERATION;          /* number of iterations */
  
  /* process command options */
  while(1){
    if((opt = getopt(argc, argv, "l:p:i:M")) == EOF)
      break;
    switch(opt){
    case 'l':
      lid = atoi(optarg);
      break;
    case 'p':
      np = atoi(optarg);
      break;
    case 'i':
      ni = atoi(optarg);
      break;
    case 'M':
      is_millisecond = 1;
      ni = atoi(optarg);
      break;
    default:
      print_usage();
      exit(EXIT_FAILURE);
      break;
    }
  }

  /* set random seed */
  srand(lid);
  
  /* conduct simulation */
  init(lid);
  for(i=0; i<ni; i++)
    sim(lid, np);
  unload(lid);
  fin(lid);

  return(EXIT_SUCCESS);
}

/* eof of dgen.c */
