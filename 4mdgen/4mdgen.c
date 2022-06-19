/* 4mdgen.c */

#include "4mdgen.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
#define EQUIPMENTSTANDBYTIME (30*60)
#define BUSINESSHOURS 8
#define NPACKAGE_TO_PRODUCE 100

#define LID_MIN 0
#define LID_MAX 999999
#define NDAY_MIN 1
#define NDAY_MAX 1000

int is_millisecond = 0;
int verbose = 0;
char *dr_out = "./";

struct worker_t {
  int wid;          /* worker id */
  int lid;          /* line id */
  char wname[NBUF]; /* workername */
};
#define NWORKER (9+4)
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
#define NOPERATIONLOG (1024 * 1024 * 10)
struct operationlog_t *operationlog;
long noperationlog = 0, noperationlog_unload = 0;

struct materiallog_t {
  long mlid;       /* material log id */
  int lid;         /* line id */
  int mtype;       /* material type */
  int olid_src;    /* source operation log */
  int olid_dst;    /* destination operation log */
};
#define NMATERIALLOG (1024 * 1024 * 10)
struct materiallog_t *materiallog;
long nmateriallog = 0, nmateriallog_unload = 0;
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
    {MODELTYPE_FULLNDIST, 0.9,   0.09},
    {MODELTYPE_HALFNDIST, 1.00,  0.02},
    {MODELTYPE_FULLNDIST, 180.0, 2.0}
   },
   { /* MT03 */
    {MODELTYPE_FULLNDIST, 90.0,  1.0},
    {MODELTYPE_FULLNDIST, 4.5,   0.4},
    {MODELTYPE_FULLNDIST, 4.5,   0.4},
    {MODELTYPE_FULLNDIST, 0.9,   0.09},
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
#define MSENSOR_ERRORSCALE 4.5

struct equipmentlog_t {
  long elid;       /* equipment log id */
  int lid;         /* line id */
  int eid;         /* equipment id */
  double ts;       /* timestamp */
  int esensor;     /* sensor */
  int ereading;    /* reading */
};
#define NEQUIPMENTLOG (3600 * 2 * NEQUIPMENT * BUSINESSHOURS * 2)
struct equipmentlog_t *equipmentlog;
long nequipmentlog = 0, nequipmentlog_unload = 0;
int esensors[NEQUIPMENT][2] =
  {
   {0, 9},  /* 0 */
   {1, 10}, /* 1 */
   {2, 11}, /* 2 */
   {3, 12}, /* 3 */
   {4, 13}, /* 4 */
   {5, 14}, /* 5 */
   {6, 15}, /* 6 */
   {7, 16}, /* 7 */
   {8, 17}, /* 8 */
  };

struct esensormodel_t {
  char *sensorname;
  int modeltype;
  double median;
  double errorrange;
};
struct esensormodel_t esensormodel[NEQUIPMENT * 2] =
  {
   {"PRESSURE",    MODELTYPE_FULLNDIST, 125.0, 15.0},  /* 0 */
   {"TEMPERATURE", MODELTYPE_FULLNDIST, 180.0, 2.0},   /* 1 */
   {"TEMPERATURE", MODELTYPE_FULLNDIST, 47.5,  7.5},   /* 2 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 3 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 4 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 5 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 6 */
   {"PRESSURE",    MODELTYPE_FULLNDIST, 15.0,  5.0},   /* 7 */
   {"TEMPERATURE", MODELTYPE_FULLNDIST, 20.0,  15.0},  /* 8 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 9 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 10 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 11 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 12 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 13 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 14 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 15 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02},  /* 16 */
   {"STATUS",      MODELTYPE_HALFNDIST, 1.02,  0.02}   /* 17 */
  };
#define ESENSOR_ERRORSCALE 4.0

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
  int idx = noperationlog % NOPERATIONLOG;
  if(noperationlog >= noperationlog_unload + NOPERATIONLOG + 1){
    fprintf(stderr, "Overflow (operationlog)\n");
    exit(EXIT_FAILURE);
  }
  
  operationlog[idx].olid = noperationlog;
  operationlog[idx].lid = lid;
  operationlog[idx].wid = wid;
  operationlog[idx].eid = eid;
  operationlog[idx].pid = pid;
  operationlog[idx].tsbegin = tsbegin;
  operationlog[idx].tsend = tsend;
  noperationlog++;
  return(operationlog[idx].olid);
}

void update_operationlog_ts(long olid,
			    double tsbegin, double tsend)
{
  int idx = olid % NOPERATIONLOG;
  if(olid != operationlog[idx].olid){
    fprintf(stderr, "Overflow (operationlog)\n");
    exit(EXIT_FAILURE);
  }

  operationlog[idx].tsbegin = tsbegin;
  operationlog[idx].tsend = tsend;
}
			    
long put_materiallog(int lid,
		     int mtype, long olid_src, long olid_dst)
{
  int idx = nmateriallog % NMATERIALLOG;
  if(nmateriallog >= nmateriallog_unload + NMATERIALLOG + 1){
    fprintf(stderr, "Overflow (materiallog)\n");
    exit(EXIT_FAILURE);
  }

  materiallog[idx].mlid = nmateriallog;
  materiallog[idx].lid = lid;
  materiallog[idx].mtype = mtype;
  materiallog[idx].olid_src = olid_src;
  materiallog[idx].olid_dst = olid_dst; 
  nmateriallog++;
  return(materiallog[idx].mlid);
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
    if((operationoutput[i] = (struct operationoutput_t *)malloc(sizeof(struct operationoutput_t) * NMATERIALLOG)) == NULL){
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
  int idx = operationoutput_to_put[pid] % NOPERATIONLOG;
  if(operationoutput_to_put[pid] >= operationoutput_to_get[pid] + NMATERIALLOG + 1){
    fprintf(stderr, "Overflow (operationoutput)");
    exit(EXIT_FAILURE);
  }
  
  operationoutput[pid][idx].mlid = mlid;
  operationoutput[pid][idx].olid = olid;
  operationoutput[pid][idx].lid = lid;
  operationoutput[pid][idx].wid = wid;
  operationoutput[pid][idx].eid = eid;
  operationoutput[pid][idx].pid = pid;
  operationoutput[pid][idx].ts = ts;
  operationoutput_to_put[pid]++;
}

long get_navailable(int pid)
{
  return(operationoutput_to_put[pid] - operationoutput_to_get[pid]);
}

struct operationoutput_t *get_operationoutput(int pid)
{
  int idx = operationoutput_to_get[pid] % NOPERATIONLOG;
  if(operationoutput_to_get[pid] >= operationoutput_to_put[pid]){
    return(NULL);
  }
  
  struct operationoutput_t *p = &operationoutput[pid][idx];
  operationoutput_to_get[pid]++;
  return(p);
}

/*
 * hash (based on FNV hash algorithm)
 */

#define FNV_PRIME 16777619U
#define FNV_BASE  2166136261U

unsigned int hash32_id_lid(int id, int lid, int key)
{
  unsigned int h;

  h = FNV_BASE;
  h = (FNV_PRIME * h) ^ ((id >> 0) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((id >> 8) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((id >> 16) & 0x000000ff);
  h = (FNV_PRIME * h) ^ ((id >> 24) & 0x000000ff);
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

void generate_wcomment(char *s, int l, int wid, int lid)
{
  snprintf(s, l,
	   "%s %s %s %s %s",
	   cmntword[hash32_id_lid(wid, lid, 1000) % NCMNTWORD],
	   cmntword[hash32_id_lid(wid, lid, 1001) % NCMNTWORD],
	   cmntword[hash32_id_lid(wid, lid, 1002) % NCMNTWORD],
	   cmntword[hash32_id_lid(wid, lid, 1003) % NCMNTWORD],
	   cmntword[hash32_id_lid(wid, lid, 1004) % NCMNTWORD]);
}

void generate_ecomment(char *s, int l, int eid, int lid)
{
  snprintf(s, l,
	   "%s %s %s %s %s",
	   cmntword[hash32_id_lid(eid, lid, 2000) % NCMNTWORD],
	   cmntword[hash32_id_lid(eid, lid, 2001) % NCMNTWORD],
	   cmntword[hash32_id_lid(eid, lid, 2002) % NCMNTWORD],
	   cmntword[hash32_id_lid(eid, lid, 2003) % NCMNTWORD],
	   cmntword[hash32_id_lid(eid, lid, 2004) % NCMNTWORD]);
}

void generate_pcomment(char *s, int l, int pid, int lid)
{
  snprintf(s, l,
	   "%s %s %s %s %s",
	   cmntword[hash32_id_lid(pid, lid, 3000) % NCMNTWORD],
	   cmntword[hash32_id_lid(pid, lid, 3001) % NCMNTWORD],
	   cmntword[hash32_id_lid(pid, lid, 3002) % NCMNTWORD],
	   cmntword[hash32_id_lid(pid, lid, 3003) % NCMNTWORD],
	   cmntword[hash32_id_lid(pid, lid, 3004) % NCMNTWORD]);
}

void generate_olcomment(char *s, int l, long olid, int lid)
{
  snprintf(s, l,
	   "%s %s %s %s %s",
	   cmntword[hash32_mlid_lid(olid, lid, 100) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 101) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 102) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 103) % NCMNTWORD],
	   cmntword[hash32_mlid_lid(olid, lid, 104) % NCMNTWORD]);
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
  y = (double)(hash32_mlid_lid(mlid + NMATERIALLOG * 0.1, lid, key)) / (UINT_MAX + 1.0);
  if(x != 0.0 && y != 0.0)
    z = sqrt(-2.0 * log(x)) * cos(2.0 * M_PI * y); /* Box-Muller approximation */
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
  y = (double)(hash32_t_lid(t + 3600 * 24 * 365, lid, key)) / (UINT_MAX + 1.0);

  if(x != 0.0 && y != 0.0)
    z = sqrt(-2.0 * log(x)) * cos(2.0 * M_PI * y); /* Box-Muller approximation */
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
  int idx = nequipmentlog % NEQUIPMENTLOG;
  if(nequipmentlog >= nequipmentlog_unload + NEQUIPMENTLOG + 1){
    fprintf(stderr, "Overflow (equipmentlog)\n");
    exit(EXIT_FAILURE);
  }
  
  equipmentlog[idx].elid = nequipmentlog;
  equipmentlog[idx].lid = lid;
  equipmentlog[idx].eid = eid;
  equipmentlog[idx].ts  = ts;
  equipmentlog[idx].esensor  = esensor;
  equipmentlog[idx].ereading = ereading;
  nequipmentlog++;
  return(equipmentlog[idx].elid);
}

/*
 * equipment status index
 */

int *equipmentstatusindex[NEQUIPMENT];
#define NEQUIPMENTSTATUSINDEX (3600 * 24 * NDAY_MAX / 32)

void init_equipmentstatusindex(void)
{
  int i;
  for(i=0; i<NEQUIPMENT; i++){
    if((equipmentstatusindex[i] = (int *)malloc(sizeof(int) * NEQUIPMENTSTATUSINDEX)) == NULL){
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    memset(equipmentstatusindex[i], 0, sizeof(int) * NEQUIPMENTSTATUSINDEX);
  }
}

void fin_equipmentstatusindex(void)
{
  int i;
  for(i=0; i<NEQUIPMENT; i++)
    free(equipmentstatusindex[i]);
}


void set_equipmentstatusindex(int eid, int t)
{
  t += EQUIPMENTSTANDBYTIME;
  if(t / 32 >= NEQUIPMENTSTATUSINDEX){ fprintf(stderr, "Overflow (equipmentstatusindex)\n"); exit(EXIT_FAILURE); }
  equipmentstatusindex[eid][t / 32] = equipmentstatusindex[eid][t / 32] | (1 << (t % 32));
}

int get_equipmentstatusindex(int eid, int t)
{
  t += EQUIPMENTSTANDBYTIME;
  if(t / 32 >= NEQUIPMENTSTATUSINDEX){ fprintf(stderr, "Overflow (equipmentstatusindex)\n"); exit(EXIT_FAILURE); }
  return((equipmentstatusindex[eid][t / 32] >> (t % 32)) & 1);
}

/*
 * Initialize
 */

void init(int lid)
{
  int i;

  /* generate WORKER */
  printf("Generating %s for LID=%u ...\n", "WORKER", lid);
  nworker = NWORKER;
  for(i=0; i<NWORKER; i++){
    worker[i].wid = i;
    worker[i].lid = lid;
    snprintf(worker[i].wname, NBUF-1, "%s%06u%06d", "WORKER", lid, i);
  }

  /* generate EQUIPMENT */
  printf("Generating %s for LID=%u ...\n", "EQUIPMENT", lid);
  nequipment = NEQUIPMENT;
  for(i=0; i<nequipment; i++){
    equipment[i].eid = i;
    equipment[i].lid = lid;
    snprintf(equipment[i].ename, NBUF-1, "%s%06u%06d (%s)", "EQUIPMENT", lid, i, ename2[i]);
  }

  /* generate PROCEDURE */
  printf("Generating %s for LID=%u ...\n", "PROCEDURE", lid);
  nprocedure = NPROCEDURE;
  for(i=0; i<nprocedure; i++){
    procedure[i].pid = i;
    snprintf(procedure[i].pname, NBUF-1, "%s%06d (%s)", "PROCEDURE", i, pname2[i]);
  }

  /* allocate OPERATIONLOG */
  printf("Allocating %s for LID=%u ...\n", "OPERATIONLOG", lid);
  if((operationlog = malloc(sizeof(struct operationlog_t) * NOPERATIONLOG)) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  /* allocate MATERIALLOG */
  printf("Allocating %s for LID=%u ...\n", "MATERIALLOG", lid);
  if((materiallog = malloc(sizeof(struct materiallog_t) * NMATERIALLOG)) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  /* allocate EQUIPMENTLOG */
  printf("Allocating %s for LID=%u ...\n", "EQUIPMENTLOG", lid);
  if((equipmentlog = malloc(sizeof(struct equipmentlog_t) * NEQUIPMENTLOG)) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  init_operationoutput();
  init_equipmentstatusindex();
}

/*
 * Simulate to generate OPERATIONLOG and MATERIALLOG
 */

int worker_pid_iday(int pid, int iday)
{
  return((pid + iday) % NWORKER);
}

double ts_max_equipmentlog = 0.0;

void sim_equipmentlog(int lid,
		      int iday,
		      double ts_latest)
{
  int i;
  long t;
  long t_latest = ts_latest + EQUIPMENTSTANDBYTIME;

  if(nequipmentlog  == 0)
    t = iday * 3600 * 24 - EQUIPMENTSTANDBYTIME;
  else if((long)equipmentlog[(nequipmentlog-1) % NEQUIPMENTLOG].ts / 3600 / 24 != t_latest / 3600 / 24)
    t = iday * 3600 * 24 - EQUIPMENTSTANDBYTIME;
  else    
    t = equipmentlog[(nequipmentlog-1) % NEQUIPMENTLOG].ts;
  
  /* printf("sim_equipmentlog[%u:%u]: %ld-%ld\n", lid, iday, t, t_latest); */

  t++;
  for(; t <= t_latest; t++){
    for(i = 0; i < NEQUIPMENT; i++){
      int r1, r2;
      r1 = generate_esensor_reading(esensors[i][0], t, lid);
      r2 = generate_esensor_reading(esensors[i][1], t, lid);
      put_equipmentlog(lid, i, (double)t, esensors[i][0], r1);
      put_equipmentlog(lid, i, (double)t, esensors[i][1], r2);
      if(r2){ set_equipmentstatusindex(i, t); }
    }
  }

  ts_max_equipmentlog = (double)t_latest;
}

int nm0max = 0, nm[NPROCEDURE];

void sim(int lid, int iday, int pmax)
{
  int i, j;
  double ts, ts_overhead;
  long t;
  long olid_src, olid_dst, mlid = 0;
  struct operationoutput_t *p;
  int wid, eid, pid, mtype;
  double ts_max = 0.0, ts_day_start = 0.0;
  int nm_day[NPROCEDURE];
  
  printf("Generating %s for LID=%u, DAY=%u ...\n", "OPERATIONLOG, MATERIALLOG and EQUIPMENTLOG", lid, iday);

  /* Arrange timestamps */
  
  ts_day_start = iday * 3600 * 24;
  
  /* Calculate maximum production amounts roughly */

  nm0max = 1;
  for(i=0;i<NPROCEDURE;i++){
    nm0max *= nmaterial_to_pack[i];
  }
  nm0max *= pmax;
  
  /*
   * #0 (PROCEDURE00000) takes MT00
   */

  ts = ts_day_start;
  wid = worker_pid_iday(0, iday); eid = 0; pid = 0; mtype = 0;
  nm_day[pid] = 0;
  while(1){
    if(ts > ts_day_start + 3600 * BUSINESSHOURS){
      if(verbose)
	printf("  sim[LID:%u, DAY:%u, PID:%u]: exceeded the designated business hours of day.\n",
	       lid, iday, pid);
      break;
    }
    if(nm0max)
      if(nm[pid] >= nm0max){
	if(verbose)
	  printf("  sim[LID:%u, DAY:%u, PID:%u]: exceeded the designated total production amount.\n",
		 lid, iday, pid);
	break;
      }
    if(ts + processinglatency[pid] >= ts_max_equipmentlog)
      sim_equipmentlog(lid, iday, ts + processinglatency[pid]);
    ts_overhead = 0.0;
    for(t = (long)ts; t <= (long)ts + processinglatency[pid]; t++)
      if(!get_equipmentstatusindex(eid, t)){ ts_overhead += 1.0; }
    olid_src = -1;
    olid_dst = put_operationlog(lid, wid, eid, pid, ts, ts + processinglatency[pid]);
    mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
    nm[pid]++; nm_day[pid]++;
    if(((double)hash32_mlid_lid(mlid, lid, 1) / (UINT_MAX + 1.0)) >= errorrate[pid])
      /* Only qualified materials go to the next step */
      put_operationoutput(mlid, olid_dst,
			  lid, wid, eid, pid,
			  ts + processinglatency[pid] + ts_overhead);
    if(ts + processinglatency[pid] + ts_overhead > ts_max){ ts_max = ts + processinglatency[pid] + ts_overhead; }
    ts += processinglatency[pid] + ts_overhead;
  } /* for(i) */
  if(verbose)
    printf("  sim[LID:%u, DAY:%u, PID:%u]: produced %u materials in a day, %u materials in total\n",
	   lid, iday, pid, nm_day[pid], nm[pid]);
  
  /*
   * #{1...9} (PROCEDURE{00001...00009}) takes MT{01...09}
   */
  
  for(i=1;i<NPROCEDURE;i++){
    ts = ts_day_start;
    wid = worker_pid_iday(i, iday); eid = i; pid = i; mtype = i;
    nm_day[pid] = 0;
    while(1){
      if(get_navailable(pid - 1) < nmaterial_to_pack[pid]){ break; }
      olid_dst = put_operationlog(lid, wid, eid, pid, ts, ts + processinglatency[pid]);
      for(j=0; j<nmaterial_to_pack[pid]; j++){
	if((p = get_operationoutput(pid - 1)) == NULL){
	  fprintf(stderr, "Oprationlog broken (pid=%d).\n", pid);
	  exit(EXIT_FAILURE);
	}
	olid_src = p->olid;
	ts = p->ts;
	mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
      }
      for(j=0; j<nmaterial_to_append[pid]; j++){
	mlid = put_materiallog(lid, mtype + 10, -1, olid_dst);
      }
      ts_overhead = 0.0;
      for(t = (long)ts; t <= (long)ts + processinglatency[pid]; t++)
	if(!get_equipmentstatusindex(eid, t)){ ts_overhead += 1.0; }
      update_operationlog_ts(olid_dst, ts, ts + processinglatency[pid] + ts_overhead);
      nm[pid]++; nm_day[pid]++;
      if(((double)hash32_mlid_lid(mlid, lid, 1) / (UINT_MAX + 1.0)) >= errorrate[pid])
	/* Only qualified materials go to the next step */
	put_operationoutput(mlid, olid_dst,
			    lid, wid, eid, pid,
			    ts + processinglatency[pid] + ts_overhead);
      if(ts + processinglatency[pid] + ts_overhead > ts_max){ ts_max = ts + processinglatency[pid] + ts_overhead; }
    } /* while(1) */
    if(verbose)
      printf("  sim[LID:%u, DAY:%u, PID:%u]: produced %u materials in a day, %u materials in total\n",
	     lid, iday, pid, nm_day[pid], nm[pid]);
  } /* for(i) */

  /*
   *  #last (DEPOSITORY) takes MT09
   */

  i=NPROCEDURE;
  ts = ts_day_start;
  wid = worker_pid_iday(i, iday); eid = i, pid = i, mtype = i;
  while(1){
    if(get_navailable(pid - 1) < 1){ break; }
    olid_dst = -1;
    for(j=0; j<1; j++){
      if((p = get_operationoutput(pid - 1)) == NULL){
	fprintf(stderr, "Oprationlog broken (pid=%d).\n", pid);
	exit(EXIT_FAILURE);
      }
      olid_src = p->olid;
      mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
    }
  } /* while(1) */

  sim_equipmentlog(lid, iday, ts_max);
}

/*
 * {open,unload,close}_files
 */

FILE *fp_worker, *fp_equipment, *fp_procedure;
FILE *fp_operationlog, *fp_materiallog, *fp_equipmentlog;

void open_files(int lid)
{
  int i;
  char fn[NBUF];
  char cmnt[NBUF];
  
  /* open and unload WORKER */
  snprintf(fn, NBUF-1, "%s/%s%06u.dat", dr_out, "WORKER", lid);
  printf("Unloading %s (%d records) for LID=%u ...\n", "WORKER", nworker, lid);
  if((fp_worker = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp_worker, "# %s\n", fn);
  fprintf(fp_worker, "# WID, LID, WNAME, COMMENT\n");
  for(i=0; i<nworker; i++){
    assert(lid == worker[i].lid);
    generate_pcomment(cmnt, NBUF-1, worker[i].wid, worker[i].lid);
    fprintf(fp_worker, "%d|%d|%s|%s\n",
	    worker[i].wid, worker[i].lid,
	    worker[i].wname,
	    cmnt);
  }

  /* open and unload EQUIPMENT */
  snprintf(fn, NBUF-1, "%s/%s%06u.dat", dr_out, "EQUIPMENT", lid);
  printf("Unloading %s (%d records) for LID=%u ...\n", "EQUIPMENT", nequipment, lid);
  if((fp_equipment = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp_equipment, "# %s\n", fn);
  fprintf(fp_equipment, "# EID, LID, ENAME, COMMENT\n");
  for(i=0; i<nequipment; i++){
    assert(lid == equipment[i].lid);
    generate_ecomment(cmnt, NBUF-1, equipment[i].eid, equipment[i].lid);
    fprintf(fp_equipment, "%d|%d|%s|%s\n",
	    equipment[i].eid, equipment[i].lid,
	    equipment[i].ename,
	    cmnt);
  }

  /* open and unload PROCEDURE */
  snprintf(fn, NBUF-1, "%s/%s.dat", dr_out, "PROCEDURE");
  printf("Unloading %s (%d records) ...\n", "PROCEDURE", nprocedure);
  if((fp_procedure = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp_procedure, "# %s\n", fn);
  fprintf(fp_procedure, "# PID, LID, PNAME, COMMENT\n");
  for(i=0; i<nprocedure; i++){
    generate_pcomment(cmnt, NBUF-1, procedure[i].pid, 0);
    fprintf(fp_procedure, "%d|%s|%s\n", procedure[i].pid,
	    procedure[i].pname,
	    cmnt);
  }
  
  /* open OPERATIONLOG */
  snprintf(fn, NBUF-1, "%s/%s%06u.dat", dr_out, "OPERATIONLOG", lid);
  if((fp_operationlog = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp_operationlog, "# %s\n", fn);
  fprintf(fp_operationlog, "# OLID, LID, WID, EID, PID, TSBEGIN, TSEND, COMMENT\n");

  /* open MATERIALLOG */
  snprintf(fn, NBUF-1, "%s/%s%06u.dat", dr_out, "MATERIALLOG", lid);
  if((fp_materiallog = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp_materiallog, "# %s\n", fn);
  fprintf(fp_materiallog, "# MLID, LID, MTYPE, OLID_SRC, OLID_DST, SERIAL, WEIGHT, DIMENSIONX, DIMENSIONY, DIMENSIONZ, SHAPESOCRE, TEMPERATURE, COMMENT\n");

  /* open EQUIPMENTLOG */
  snprintf(fn, NBUF-1, "%s/%s%06u.dat", dr_out, "EQUIPMENTLOG", lid);
  if((fp_equipmentlog = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp_equipmentlog, "# %s\n", fn);
  fprintf(fp_equipmentlog, "# ELID, LID, EID, TS, ESENSOR, EREADING\n");
}

void unload_files(int lid, int iday)
{
  int i, idx;
  time_t t1, t2;
  char dt1[NBUF], dt2[NBUF];
  char cmnt[NBUF];
  
  /* unload OPERATIONLOG */
  printf("Unloading %s (%ld records, %ld records in total) for LID=%u, DAY=%u...\n", "OPERATIONLOG",
	 noperationlog - noperationlog_unload, noperationlog, lid, iday);
  for(i=noperationlog_unload; i<noperationlog; i++){
    idx = i % NOPERATIONLOG;
    assert(lid == operationlog[idx].lid);
    t1 = TSBASE + operationlog[idx].tsbegin;
    strftime(dt1, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t1));
    t2 = TSBASE + operationlog[idx].tsend;
    strftime(dt2, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t2));
    generate_olcomment(cmnt, NBUF-1, operationlog[idx].olid, operationlog[idx].lid);
    if(is_millisecond)
      fprintf(fp_operationlog, "%ld|%d|%d|%d|%d|%s.%03u|%s.%03u|%s\n",
	      operationlog[idx].olid, operationlog[idx].lid,
	      operationlog[idx].wid, operationlog[idx].eid, operationlog[idx].pid,
	      dt1, (unsigned int)(operationlog[idx].tsbegin * 1000) % 1000,
	      dt2, (unsigned int)(operationlog[idx].tsend   * 1000) % 1000,
	      cmnt);
    else
      fprintf(fp_operationlog, "%ld|%d|%d|%d|%d|%s|%s|%s\n",
	      operationlog[idx].olid, operationlog[idx].lid,
	      operationlog[idx].wid, operationlog[idx].eid, operationlog[idx].pid,
	      dt1,
	      dt2,
	      cmnt);    
  }
  noperationlog_unload = noperationlog;

  /* unload MATERIALLOG */
  printf("Unloading %s (%ld records, %ld records in total) for LID=%u, DAY=%u ...\n", "MATERIALLOG",
	 nmateriallog - nmateriallog_unload, nmateriallog, lid, iday);
  for(i=nmateriallog_unload; i<nmateriallog; i++){
    idx = i % NMATERIALLOG;
    assert(lid == materiallog[idx].lid);
    t1 = TSBASE + 3600 * 24 * iday;
    strftime(dt1, NBUF-1, "%y%m%d", localtime(&t1));
    generate_mlcomment(cmnt, NBUF-1, materiallog[idx].mlid, materiallog[idx].lid);
    fprintf(fp_materiallog, "%ld|%d|%s|%d|%d|D%6s-L%06d-M%010ld|%09.3f|%09.3f|%09.3f|%09.3f|%09.3f|%09.3f|%s\n",
	    materiallog[idx].mlid, materiallog[idx].lid,
	    mtypename[materiallog[idx].mtype],
	    materiallog[idx].olid_src,
	    materiallog[idx].olid_dst,
	    dt1, materiallog[idx].lid, materiallog[idx].mlid,
	    generate_msensor_reading(materiallog[idx].mtype, MSENSOR_WGHT,
				     materiallog[idx].mlid, materiallog[idx].lid),
	    generate_msensor_reading(materiallog[idx].mtype, MSENSOR_DIMX,
				     materiallog[idx].mlid, materiallog[idx].lid),
	    generate_msensor_reading(materiallog[idx].mtype, MSENSOR_DIMY,
				     materiallog[idx].mlid, materiallog[idx].lid),
	    generate_msensor_reading(materiallog[idx].mtype, MSENSOR_DIMZ,
				     materiallog[idx].mlid, materiallog[idx].lid),
	    generate_msensor_reading(materiallog[idx].mtype, MSENSOR_SHPS,
				     materiallog[idx].mlid, materiallog[idx].lid),
	    generate_msensor_reading(materiallog[idx].mtype, MSENSOR_TEMP,
				     materiallog[idx].mlid, materiallog[idx].lid),
	    cmnt);
  }
  nmateriallog_unload = nmateriallog;

  /* unload EQUIPMENTLOG */
  printf("Unloading %s (%ld records, %ld records in total) for LID=%u, DAY=%u ...\n", "EQUIPMENTLOG",
	 nequipmentlog - nequipmentlog_unload, nequipmentlog, lid, iday);
  for(i=nequipmentlog_unload; i<nequipmentlog; i++){
    idx = i % NEQUIPMENTLOG;
    assert(lid == equipmentlog[idx].lid);
    t1 = TSBASE + equipmentlog[idx].ts;
    strftime(dt1, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t1));
    if(is_millisecond)
      fprintf(fp_equipmentlog, "%ld|%d|%d|%s.%03u|%s|%d\n",
	      equipmentlog[idx].elid, equipmentlog[idx].lid, equipmentlog[idx].eid,
	      dt1, 0,
	      esensormodel[equipmentlog[idx].esensor].sensorname,
	      equipmentlog[idx].ereading);
    else
      fprintf(fp_equipmentlog, "%ld|%d|%d|%s|%s|%d\n",
	      equipmentlog[idx].elid, equipmentlog[idx].lid, equipmentlog[idx].eid,
	      dt1,
	      esensormodel[equipmentlog[idx].esensor].sensorname,
	      equipmentlog[idx].ereading);      
  }
  nequipmentlog_unload = nequipmentlog;
}

void close_files(int lid)
{
  fclose(fp_worker);
  fclose(fp_equipment);
  fclose(fp_procedure);
  fclose(fp_operationlog);
  fclose(fp_materiallog);
  fclose(fp_equipmentlog);
}

/*
 * Finalize
 */

void fin(int lid)
{
  fin_equipmentstatusindex();
  fin_operationoutput();

  printf("Releasing %s for LID=%u ...\n", "MATERIALLOG", lid);
  free(materiallog);
  printf("Releasing %s for LID=%u ...\n", "OPERATIONLOG", lid);
  free(operationlog);
}

/* 
 * Main
 */

void print_usage(void)
{
  printf("\
Usage: 4mdgen [options]\n\
Description:\n\
  The 4mbench dataset generator\n\
Options:\n\
  -l <num> : specify LID (production line id) (range: %u to %u) (default: 0)\n\
  -d <num> : specify the number of simulation days (range: %u to %u) (default: 1)\n\
  -n <num> : limit the maximim total number of final products to be produced (FOR DEBUGGING)\n\
             (default: disabled)\n\
  -o <dir> : specify output directory\n\
             (default: ./)\n\
  -M     : enable millisecond-scale dataset generation (default: second-scale)\n\
  -v     : increase verbose levels\n\
Example:\n\
  4mdgen -l 101 -d 10 -o /tmp\n\
    generates a dataset of 10-day business operations of the production line #101 in /tmp.\n\
",
	 LID_MIN, LID_MAX,
	 NDAY_MIN, NDAY_MAX);
}

int main(int argc, char **argv)
{
  int opt;
  int i;
  int lid = 0;
  int nday = 1;
  int pmax = 0;
  
  /* process command options */
  while(1){
    if((opt = getopt(argc, argv, "l:d:n:o:hMv")) == EOF)
      break;
    switch(opt){
    case 'l':
      lid = atoi(optarg);
      break;
    case 'd':
      nday = atoi(optarg);
      break;
    case 'n':
      pmax = atoi(optarg);
      break;
    case 'o':
      dr_out = strdup(optarg);
      break;
    case 'h':
      print_usage();
      exit(EXIT_SUCCESS);
      break;
    case 'M':
      is_millisecond = 1;
      break;
    case 'v':
      verbose++;
      break;
    default:
      print_usage();
      exit(EXIT_FAILURE);
      break;
    }
  }

  if(lid < LID_MIN || lid > LID_MAX){
    fprintf(stderr, "Parameter (LID) is out of range.\n");
    print_usage();
    exit(EXIT_FAILURE);
  }
  if(nday < NDAY_MIN || nday > NDAY_MAX){
    fprintf(stderr, "Parameter (number of days) is out of range.\n");
    print_usage();
    exit(EXIT_FAILURE);
  }
  
  init(lid);
  open_files(lid);
  for(i=0; i<nday; i++){
    sim(lid, i, pmax);
    unload_files(lid, i);
  }
  close_files(lid);
  fin(lid);

  return(EXIT_SUCCESS);
}

/* eof of 4mdgen.c */
