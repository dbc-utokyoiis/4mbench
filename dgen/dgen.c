/* dgen.c */

#include "dgen.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <time.h>


/*
 * Declaration
 */

#define NBUF 512
#define TS_BASE 1648767600 /* 2022-04-01 08:00:00 at Tokyo */
#define NPACKAGE_TO_PRODUCE 100
#define NITERATION 1

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

struct procedure_t {
  int pid;          /* procedure id */
  char pname[NBUF]; /* procedure name */
};
#define NPROCEDURE 9
struct procedure_t procedure[NPROCEDURE];
int nprocedure = 0;

struct operationlog_t {
  long olid;       /* operation log id */
  int lid;         /* line id */
  int wid;         /* worker id */
  int eid;         /* equipment id */
  int pid;         /* process id */
  double ts_begin; /* timestamp in starting */
  double ts_end;   /* timestamp in ending */
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

/*
 * put_operationlog(), put_materiallog()
 */

long put_operationlog(int lid,
		      int wid, int eid, int pid,
		      double ts_begin, double ts_end)
{  
  if(noperationlog >= NOPERATIONLOG){ fprintf(stderr, "overflow (operationlog)"); exit(EXIT_FAILURE); }
  
  operationlog[noperationlog].olid = noperationlog;
  operationlog[noperationlog].lid = lid;
  operationlog[noperationlog].wid = wid;
  operationlog[noperationlog].eid = eid;
  operationlog[noperationlog].pid = pid;
  operationlog[noperationlog].ts_begin = ts_begin;
  operationlog[noperationlog].ts_end = ts_end;
  noperationlog++;
  return(operationlog[noperationlog-1].olid);
}

void update_operationlog_ts(long olid,
			    double ts_begin, double ts_end)
{
  if(olid != operationlog[olid].olid){ fprintf(stderr, "overflow (operationlog)");  exit(EXIT_FAILURE); }

  operationlog[olid].ts_begin = ts_begin;
  operationlog[olid].ts_end = ts_end;
}
			    

long put_materiallog(int lid,
		     int mtype, long olid_src, long olid_dst)
{
  if(nmateriallog >= NMATERIALLOG){ fprintf(stderr, "overflow (materiallog)");  exit(EXIT_FAILURE); }

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
    snprintf(worker[i].wname, NBUF-1, "%s%05u%05d", "WORKER", lid, i);
  }

  /* generate EQUIPMENT */
  printf("Generating %s for lid=%u ...\n", "EQUIPMENT", lid);
  nequipment = NEQUIPMENT;
  for(i=0; i<nequipment; i++){
    equipment[i].eid = i;
    equipment[i].lid = lid;
    snprintf(equipment[i].ename, NBUF-1, "%s%05u%05d", "EQUIPMENT", lid, i);
  }

  /* generate PROCEDURE */
  printf("Generating %s for lid=%u ...\n", "PROCEDURE", lid);
  nprocedure = NPROCEDURE;
  for(i=0; i<nprocedure; i++){
    procedure[i].pid = i;
    snprintf(procedure[i].pname, NBUF-1, "%s%05d", "PROCEDURE", i);
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

  init_operationoutput();
}

/*
 * Simulate to generate OPERATIONLOG and MATERIALLOG
 */

double ts_backup = 0.0;

void sim(int lid, int np)
{
  int i, j;
  double ts;
  long olid_src, olid_dst, mlid = 0;
  struct operationoutput_t *p;
  int wid, eid, pid, mtype;
  int n;
  
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

  ts = ts_backup;
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
  } /* for(i) */
  ts_backup = ts;
  
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
	if((p = get_operationoutput(pid - 1)) == NULL){ exit(EXIT_FAILURE); }
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
      if((p = get_operationoutput(pid - 1)) == NULL){ exit(EXIT_FAILURE); }
      olid_src = p->olid;
      mlid = put_materiallog(lid, mtype, olid_src, olid_dst);
    }
  } /* while(1) */
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

  /* unload WORKER */
  snprintf(fn, NBUF-1, "%s%05u.dat", "WORKER", lid);
  printf("Unloading %s (%d records) for lid=%u ...\n", "WORKER", nworker, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  for(i=0; i<nworker; i++){
    assert(lid == worker[i].lid);
    fprintf(fp, "%d|%d|%s\n", worker[i].wid, worker[i].lid,
	    worker[i].wname);
  }
  fclose(fp);

  /* unload EQUIPMENT */
  snprintf(fn, NBUF-1, "%s%05u.dat", "EQUIPMENT", lid);
  printf("Unloading %s (%d records) for lid=%u ...\n", "EQUIPMENT", nequipment, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  for(i=0; i<nequipment; i++){
    assert(lid == equipment[i].lid);
    fprintf(fp, "%d|%d|%s\n", equipment[i].eid, equipment[i].lid,
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
  for(i=0; i<nprocedure; i++)
    fprintf(fp, "%d|%s\n", procedure[i].pid,
	    procedure[i].pname);
  fclose(fp);

  /* unload OPERATIONLOG */
  snprintf(fn, NBUF-1, "%s%05u.dat", "OPERATIONLOG", lid);
  printf("Unloading %s (%ld records) for lid=%u ...\n", "OPERATIONLOG", noperationlog, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  for(i=0; i<noperationlog; i++){
    assert(lid == operationlog[i].lid);
    t1 = TS_BASE + operationlog[i].ts_begin;
    strftime(dt1, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t1));
    t2 = TS_BASE + operationlog[i].ts_end;
    strftime(dt2, NBUF-1, "%Y-%m-%d %H:%M:%S", localtime(&t2));
    fprintf(fp, "%ld|%d|%d|%d|%d|%s|%s\n", operationlog[i].olid, operationlog[i].lid,
	    operationlog[i].wid, operationlog[i].eid, operationlog[i].pid,
	    dt1, dt2);
  }
  fclose(fp);

  /* unload MATERIALLOG */
  snprintf(fn, NBUF-1, "%s%05u.dat", "MATERIALLOG", lid);
  printf("Unloading %s (%ld records) for lid=%u ...\n", "MATERIALLOG", nmateriallog, lid);
  if((fp = fopen(fn, "w")) == NULL){
    perror("fopen"); exit(EXIT_FAILURE);
  }
  fprintf(fp, "# %s\n", fn);
  for(i=0; i<nmateriallog; i++){
    assert(lid == materiallog[i].lid);
    /*
    fprintf(fp, "%ld|%d|%d|%d|%d\n", materiallog[i].mlid, materiallog[i].lid,
	    materiallog[i].mtype, materiallog[i].olid_src, materiallog[i].olid_dst);
    */
    fprintf(fp, "%ld|%d|%s|%d|%d\n", materiallog[i].mlid, materiallog[i].lid,
	    mtypename[materiallog[i].mtype], materiallog[i].olid_src, materiallog[i].olid_dst);
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

int main(int argc, char **argv)
{
  int opt;
  int i;
  int lid = 0; /* line id */
  int np = NPACKAGE_TO_PRODUCE; /* package number to produce */
  int ni = NITERATION;          /* number of iterations */
  
  /* process command options */
  while(1){
    if((opt = getopt(argc, argv, "l:p:i:")) == EOF)
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
