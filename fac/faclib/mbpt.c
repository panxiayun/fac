#include "mbpt.h"
#include "cf77.h"

static char *rcsid="$Id: mbpt.c,v 1.7 2005/07/27 21:57:21 mfgu Exp $";
#if __GNUC__ == 2
#define USE(var) static void * use_##var = (&use_##var, (void *) &var) 
USE (rcsid);
#endif

static int mbpt_extra = 0;
static int mbpt_nlev = 0;
static int *mbpt_ilev = NULL;
static double mbpt_mcut = EPS3;
static int mbpt_n3 = 0;

void SetOptMBPT(int n3, double c) {
  mbpt_n3 = n3;
  mbpt_mcut = c;
}

void SetExtraMBPT(int m) {
  mbpt_extra = m;
}

void SetSymMBPT(int nlev, int *ilev) {
  if (mbpt_nlev > 0) free(mbpt_ilev);  
  if (nlev > 0 && ilev[0] < 0) nlev = 0;
  mbpt_nlev = nlev;
  if (nlev > 0) {
    mbpt_ilev = malloc(sizeof(int)*nlev);
    memcpy(mbpt_ilev, ilev, sizeof(int)*nlev);
    qsort(mbpt_ilev, nlev, sizeof(int), CompareInt);
  }
}

int AddMBPTConfig(int kgp, CORR_CONFIG *ccp) {
  CONFIG *c1, cp;

  c1 = ccp->c;

  if (ccp->kp == 0) {
    cp.n_shells = c1->n_shells;
    cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
    memcpy(cp.shells, c1->shells, sizeof(SHELL)*c1->n_shells);
    Couple(&cp);
    AddConfigToList(kgp, &cp);
    return 0;
  }
  if (ccp->kq == 0) {
    if (c1->shells[0].nq != 0) {
      cp.n_shells = c1->n_shells + 1;
      cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
      memcpy(cp.shells+1, c1->shells, sizeof(SHELL)*c1->n_shells);
    } else {
      cp.n_shells = 1;
      cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
    }
    cp.shells[0].nq = 1;
    cp.shells[0].n = ccp->np;
    cp.shells[0].kappa = ccp->kp;
    Couple(&cp);
    AddConfigToList(kgp, &cp);
  } else {
    if (ccp->np == ccp->nq && ccp->kp == ccp->kq) {
      if (c1->shells[0].nq != 0) {
	cp.n_shells = c1->n_shells + 1;
	cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
	memcpy(cp.shells+1, c1->shells, 
	       sizeof(SHELL)*c1->n_shells);
      } else {
	cp.n_shells = 1;
	cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
      }
      cp.shells[0].nq = 2;
      cp.shells[0].n = ccp->np;
      cp.shells[0].kappa = ccp->kp;
    } else {
      if (c1->shells[0].nq != 0) {
	cp.n_shells = c1->n_shells + 2;
	cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
	memcpy(cp.shells+2, c1->shells, 
	       sizeof(SHELL)*c1->n_shells);
      } else {
	cp.n_shells = 2;
	cp.shells = malloc(sizeof(SHELL)*cp.n_shells);
      }
      cp.shells[1].nq = 1;
      cp.shells[1].n = ccp->np;
      cp.shells[1].kappa = ccp->kp;
      cp.shells[0].nq = 1;
      cp.shells[0].n = ccp->nq;
      cp.shells[0].kappa = ccp->kq;
    }
    Couple(&cp);
    AddConfigToList(kgp, &cp);
  }
  
  return 0;
}    

void ConfigChangeNE(int *m, CONFIG *c, int k, int *ik, int n, int *em) {
  int na, i, j, p;
  int nn, jj, ka;
  SHELL *s;

  na = abs(n);
  for (j = 0; j < na; j++) {
    if (n < 0) {
      ik[em[j]]--;
    } else {
      ik[em[j]]++;
    }
  }
  for (j = 0; j < na; j++) {
    if (ik[em[j]] < 0) {
      goto OUT;
    }
    IntToShell(em[j], &nn, &ka);
    jj = GetJFromKappa(ka);
    if (ik[em[j]] > jj+1) goto OUT;
  }
  p = 0;
  for (i = 0; i < k; i++) {
    if (ik[i] >= 0) p++;
  }
  s = malloc(sizeof(SHELL)*p);
  j = 0;
  for (i = k-1; i >= 0; i--) {
    if (ik[i] >= 0) {
      IntToShell(i, &nn, &ka);
      s[j].n = nn;
      s[j].kappa = ka;
      s[j].nq = ik[i];
      j++;
    }
  }
  nn = 0;
  for (i = 0; i < *m; i++) {
    if (p == c[i].n_shells) {
      ka = memcmp(c[i].shells, s, sizeof(SHELL)*p);
      if (ka == 0) {
	nn = 1;
	break;
      }
    }
  }
  if (nn == 1) {
    free(s);
  } else {
    c[*m].shells = s;
    c[*m].n_shells = p;
    (*m)++;
  }

 OUT:  
  for (j = 0; j < na; j++) {
    if (n < 0) ik[em[j]]++;
    else ik[em[j]]--;
  }
}
	
void RemoveEmpty(CONFIG *c) {
  int i, n;
  SHELL *s;
  
  n = 0;
  for (i = 0; i < c->n_shells; i++) {
    if (c->shells[i].nq > 0) {
      n++;
    }
  }
  if (n == 0) {
    n = 1;
    s = malloc(sizeof(SHELL));
    PackShell(s, 1, 0, 1, 0);
    goto END;
  }
  s = malloc(sizeof(SHELL)*n);
  n = 0;
  for (i = 0; i < c->n_shells; i++) {
    if (c->shells[i].nq > 0) {
      memcpy(s+n, c->shells+i, sizeof(SHELL));
      n++;
    }
  }
 END:
  free(c->shells);
  c->shells = s;
  c->n_shells = n;
}

void BaseConfig(int n, int *kg, int n3, int *nbc, CONFIG **bc, 
		int *nbc1, CONFIG **bc1, int *nb, int **bk, FILE *f) {
  int nmax, nsm, ncs, i, j, km, k, t, m, p, jp, nq;
  int mcs, m1e, m2e, mcs1, *ik, em[2], b1e, b2e;
  CONFIG *c, *c1, **c0;
  CONFIG_GROUP *g;
  char sc[2000];

  nmax = 0;
  nsm = 0;
  ncs = 0;
  for (i = 0; i < n; i++) {
    g = GetGroup(kg[i]);
    ncs += g->n_cfgs;
    for (j = 0; j < g->n_cfgs; j++) {
      c = GetConfigFromGroup(kg[i], j);
      if (nmax < c->shells[0].n) nmax = c->shells[0].n;
      if (nsm < c->n_shells) nsm = c->n_shells;
    }
  }

  k = nmax*nmax;
  ik = malloc(sizeof(int)*k);
  for (i = 0; i < k; i++) {
    ik[i] = -1;
  }
  c0 = malloc(sizeof(CONFIG *)*ncs);
  m = 0;
  for (i = 0; i < n; i++) {
    g = GetGroup(kg[i]);
    nq = g->n_electrons;
    for (j = 0; j < g->n_cfgs; j++) {
      c = GetConfigFromGroup(kg[i], j);
      for (km = 0; km < c->n_shells; km++) {
	t = ShellToInt(c->shells[km].n, c->shells[km].kappa);
	ik[t] = 0;
      }
      c0[m++] = c;
    }
  }

  *nb = 0;
  for (j = 0; j < k; j++) {
    if (ik[j] == 0) (*nb)++;
  }
  (*bk) = malloc(sizeof(int)*(*nb));
  i = 0;
  for (j = 0; j < k; j++) {
    if (ik[j] == 0) {
      IntToShell(j, &p, &m);
      t = OrbitalIndex(p, m, 0.0);
      (*bk)[i++] = t;
    }
  }
  qsort(*bk, *nb, sizeof(int), CompareInt);

  mcs = ncs*nsm*(1+nsm);
  (*bc) = malloc(sizeof(CONFIG)*mcs);
  c = *bc;

  m = 0;
  for (i = 0; i < ncs; i++) {    
    for (j = 0; j < k; j++) {
      if (ik[j] >= 0) ik[j] = 0;
    }
    for (j = 0; j < c0[i]->n_shells; j++) {
      t = ShellToInt(c0[i]->shells[j].n, c0[i]->shells[j].kappa);
      ik[t] = c0[i]->shells[j].nq;
    }
    for (j = 0; j < c0[i]->n_shells; j++) {
      if (c0[i]->shells[j].n < n3) continue;
      em[0] = ShellToInt(c0[i]->shells[j].n, c0[i]->shells[j].kappa);      
      ConfigChangeNE(&m, c, k, ik, -1, em);      
    }
  }
  m1e = m;
  c = c + m;
  m = 0;
  for (i = 0; i < ncs; i++) {
    for (j = 0; j < k; j++) {
      if (ik[j] >= 0) ik[j] = 0;
    }
    for (j = 0; j < c0[i]->n_shells; j++) {
      t = ShellToInt(c0[i]->shells[j].n, c0[i]->shells[j].kappa);
      ik[t] = c0[i]->shells[j].nq;
    }
    for (j = 0; j < c0[i]->n_shells; j++) {
      if (c0[i]->shells[j].n < n3) continue;
      em[0] = ShellToInt(c0[i]->shells[j].n, c0[i]->shells[j].kappa);
      for (jp = 0; jp <= j; jp++) {
	em[1] = ShellToInt(c0[i]->shells[jp].n, c0[i]->shells[jp].kappa);
	ConfigChangeNE(&m, c, k, ik, -2, em);
      }
    }
  }
  m2e = m;
  
  mcs1 = m1e*nsm + m2e*nsm*nsm;
  *bc1 = malloc(sizeof(CONFIG)*mcs1);
  c1 = *bc1;
  m = 0;
  c = *bc;
  for (i = 0; i < m1e; i++) {
    for (j = 0; j < k; j++) {
      if (ik[j] >= 0) ik[j] = 0;
    }
    for (j = 0; j < c[i].n_shells; j++) {
      t = ShellToInt(c[i].shells[j].n, c[i].shells[j].kappa);
      ik[t] = c[i].shells[j].nq;
    }
    for (j = 0; j < c[i].n_shells; j++) {
      em[0] = ShellToInt(c[i].shells[j].n, c[i].shells[j].kappa);
      ConfigChangeNE(&m, c1, k, ik, 1, em);
    }
  }
  c = c + m1e;
  for (i = 0; i < m2e; i++) {
    for (j = 0; j < k; j++) {
      if (ik[j] >= 0) ik[j] = 0;
    }
    for (j = 0; j < c[i].n_shells; j++) {
      t = ShellToInt(c[i].shells[j].n, c[i].shells[j].kappa);
      ik[t] = c[i].shells[j].nq;
    }
    for (j = 0; j < c[i].n_shells; j++) {
      em[0] = ShellToInt(c[i].shells[j].n, c[i].shells[j].kappa);
      for (jp = 0; jp <= j; jp++) {
	em[1] = ShellToInt(c[i].shells[jp].n, c[i].shells[jp].kappa);
	ConfigChangeNE(&m, c1, k, ik, 2, em);
      }
    }
  }
  *nbc = m1e + m2e;  
  c = *bc;
  for (i = 0; i < *nbc; i++) {
    RemoveEmpty(c+i);
    if (i < m1e) c[i].n_electrons = nq-1;
    else c[i].n_electrons = nq-2;
  }

  c1 = *bc1;
  *nbc1 = m;
  *bc1 = malloc(sizeof(CONFIG)*m);
  c = *bc1;
  k = 0;
  for (i = 0; i < *nbc1; i++) {
    RemoveEmpty(c1+i);
    p = 0;
    for (j = 0; j < ncs; j++) {      
      if (c1[i].n_shells == c0[j]->n_shells) {
	t = memcmp(c1[i].shells, c0[j]->shells, sizeof(SHELL)*c0[j]->n_shells);
	if (t == 0) {
	  p = 1;
	  break;
	}
      }
    }
    if (p == 0) {
      c[k].n_shells = c1[i].n_shells;
      c[k].shells = c1[i].shells;
      c[k].n_electrons = nq;
      k++;
    } else {
      free(c1[i].shells);
    }
  }
  *nbc1 = k;
  free(ik);  
  free(c1);

  if (f) {
    for (i = 0; i < ncs; i++) {
      ConstructConfigName(sc, 2000, c0[i]);
      fprintf(f, "0E: %4d   %s\n", i, sc);
    }
    fprintf(f, "\n");
    c = *bc;
    for (i = 0; i < m1e; i++) {
      ConstructConfigName(sc, 2000, c+i);
      fprintf(f, "1E: %4d   %s\n", i, sc);
    }
    fprintf(f, "\n");
    c += m1e;
    for (i = 0; i < m2e; i++) {
      ConstructConfigName(sc, 2000, c+i);
      fprintf(f, "2E: %4d   %s\n", i, sc);
    }
    fprintf(f, "\n");
    c = *bc1;
    for (i = 0; i < *nbc1; i++) {
      ConstructConfigName(sc, 2000, c+i);
      fprintf(f, "EX: %4d   %s\n", i, sc);
    }
    fprintf(f, "\n");
    fflush(f);
  }
  free(c0);
}

int ConstructNGrid(int n, int **g) {
  int i, *p, *q, n0, n1, nt, m, di, dm;

  if (n <= 0) return n;
  p = *g;
  if (p[0] >= 0) {
    if (n > 1) {
      qsort(p, n, sizeof(int), CompareInt);
    }
    return n;
  }
  
  if (n != 5) return -1;
  n1 = -p[0];
  n0 = p[1];
  nt = p[2];
  m = p[3];
  dm = p[4];

  n = (n1-n0+1) + nt;
  q = malloc(sizeof(int)*n);
  for (i = n0; i < n1; i++) {
    q[i] = i;
  }
  di = 2;
  for (; i < n; i++) {
    q[i] = q[i-1] + di;
    if (di < dm) {
      if (m == 0) {
	di *= 2;
      } else {
	di += m;
      }
      if (di >= dm) di = dm;
    }
  }

  free(*g);
  *g = q;
  return n;
}
  
int StructureMBPT0(char *fn, double ccut, int n, int *s0, int kmax,
		   int n1, int *nm, int n2, int *nmp, int n3, char *gn) {
  CONFIG_GROUP *g1, *g0;
  CONFIG *c1, *bc, *bc1;
  SYMMETRY *sym;
  STATE *st;
  LEVEL *lev;
  HAMILTON *ha;
  int nbc, nbc1, k;
  int nele0, nele1, nk, m, t, r;
  int i, p, np, nq, inp, inq, k0, k1, q;
  int jp, jq, kap, kaq, kp, kq, kp2, kq2;
  int ic, kgp, nb, *bk;
  int ncc, ncc0, ncc1, ncc2;
  int *bs1, nbs1, *bs0, nbs0, *s;
  double a1, a2, d, a, b, *e1, *ham;
  CORR_CONFIG cc, *ccp, *ccp1;
  ARRAY ccfg;
  int *icg, ncg, icg0, icg1, rg;
  typedef struct _MBPT_BASE_ {
    int isym;
    int nbasis, nb2, bmax;
    int *basis;
    double *ene;
  } MBPT_BASE;
  MBPT_BASE mb, *mbp;
  ARRAY base;
  char cname[2000];
  FILE *f;
  double t0, t1, t2;
  int sr, nr;
#ifdef USE_MPI
  int *ics0, *ics1;
  double *de0, *de1;
  MPI_Comm_rank(MPI_COMM_WORLD, &sr);
  MPI_Comm_size(MPI_COMM_WORLD, &nr);
  printf("RANK: %2d, TOTAL: %2d\n", sr, nr);
#else
  sr = 0;
  nr = 1;
#endif
  
  t0 = clock();
  t0 /= CLOCKS_PER_SEC;

  f = NULL;
  if (sr == 0) {
    f = fopen(fn, "w");
    if (f == NULL) {
      printf("cannot open file %s\n", fn);
      return -1;
    }
  }

  n1 = ConstructNGrid(n1, &nm);
  n2 = ConstructNGrid(n2, &nmp);

  ha = GetHamilton();
  BaseConfig(n, s0, n3, &nbc, &bc, &nbc1, &bc1, &nb, &bk, f);

  for (inp = 0; inp < n1; inp++) {
    np = nm[inp];
    for (kp = 0; kp <= kmax; kp++) {
      if (kp >= np) break;
      kp2 = 2*kp;
      for (jp = kp2-1; jp <= kp2+1; jp += 2) {
	if (jp < 0) continue;
	kap = GetKappaFromJL(jp, kp2);
	i = OrbitalIndex(np, kap, 0.0);
	if (i < 0) return -1;
	for (inq = 0; inq < n2; inq++) {
	  nq = np + nmp[inq];
	  for (kq = 0; kq <= kmax; kq++) {
	    if (kq >= nq) break;
	    kq2 = 2*kq;
	    for (jq = kq2-1; jq <= kq2+1; jq += 2) {
	      if (jq < 0) continue;
	      kaq = GetKappaFromJL(jq, kq2);
	      i = OrbitalIndex(nq, kaq, 0.0);
	      if (i < 0) return -1;
	    }
	  }
	}
      }
    }
  }

  ArrayInit(&ccfg, sizeof(CORR_CONFIG), 10000);
  ArrayInit(&base, sizeof(MBPT_BASE), MAX_SYMMETRIES);
  k = nbc;
  icg = malloc(sizeof(int)*(n1*(1+n2)*k+2));

  icg[0] = 0;
  t = 1;
  g0 = GetGroup(s0[0]);
  nele0 = g0->n_electrons;
  for (p = 0; p < nbc1; p++) {
    c1 = bc1 + p;
    cc.c = c1;
    cc.ig = p;
    cc.ncs = 0;
    cc.np = c1->shells[0].n;
    cc.inp = IBisect(cc.np, n1, nm);
    if (cc.inp < 0) continue;
    cc.inq = 0;
    cc.nq = 0;
    cc.kp = 0;
    cc.kq = 0;
    ccp = ArrayAppend(&ccfg, &cc, NULL);
  } 
  icg[t++] = ccfg.dim;      
  for (p = 0; p < nbc; p++) {
    c1 = bc + p;
    nele1 = c1->n_electrons;
    if (nele1 == nele0-1) {
      for (inp = 0; inp < n1; inp++) {
	np = nm[inp];
	for (kp = 0; kp <= kmax; kp++) {
	  if (kp >= np) break;
	  kp2 = 2*kp;
	  for (jp = kp2 - 1; jp <= kp2 + 1; jp += 2) {
	    if (jp < 0) continue;
	    cc.c = c1;
	    cc.ncs = 0;
	    cc.ig = p;
	    cc.inp = inp;
	    cc.np = np;
	    cc.inq = 0;
	    cc.nq = 0;
	    cc.kp = GetKappaFromJL(jp, kp2);
	    k0 = OrbitalIndex(np, cc.kp, 0);
	    k1 = IBisect(k0, nb, bk);
	    if (k1 >= 0) continue;
	    cc.kq = 0;
	    ccp = ArrayAppend(&ccfg, &cc, NULL);
	  }
	}
	icg[t++] = ccfg.dim;
      }
    } else if (nele1 == nele0-2) {
      for (inp = 0; inp < n1; inp++) {
	np = nm[inp];
	for (inq = 0; inq < n2; inq++) {
	  nq = np + nmp[inq];
	  for (kp = 0; kp <= kmax; kp++) {
	    if (kp >= np) break;
	    kp2 = 2*kp;
	    for (jp = kp2-1; jp <= kp2+1; jp += 2) {
	      if (jp < 0) continue;
	      kap = GetKappaFromJL(jp, kp2);
	      k0 = OrbitalIndex(np, kap, 0);
	      k1 = IBisect(k0, nb, bk);
	      if (k1 >= 0) continue;
	      for (kq = 0; kq <= kmax; kq++) {
		if (kq >= nq) break;
		kq2 = 2*kq;
		for (jq = kq2-1; jq <= kq2+1; jq += 2) {
		  if (jq < 0) continue;
		  kaq = GetKappaFromJL(jq, kq2);
		  if (np == nq && kaq > kap) continue;
		  k0 = OrbitalIndex(nq, kaq, 0);
		  k1 = IBisect(k0, nb, bk);
		  if (k1 >= 0) continue;
		  cc.c = c1;
		  cc.ncs = 0;
		  cc.ig = p;
		  cc.inp = inp;
		  cc.np = np;
		  cc.inq = inq;
		  cc.nq = nq;
		  cc.kp = kap;
		  cc.kq = kaq;
		  ccp = ArrayAppend(&ccfg, &cc, NULL);
		}
	      }
	    }
	  }
	  icg[t++] = ccfg.dim;
	}
      }
    }
  }

  if (ccut <= 0) goto ADDCFG;
  ncg = t-1;
  r = ncg/nr;
  icg0 = sr*r;
  if (sr < nr-1) {
    icg1 = (sr+1)*r;
  } else {
    icg1 = ncg;
  }
  /*
  printf("RANK: %2d, %d %d %d %d\n", sr, ncg, icg0, icg1, n1*n2*k+1);
  */
  for (i = 0; i < MAX_SYMMETRIES; i++) {
    nk = ConstructHamiltonDiagonal(i, n, s0, 0);    
    if (nk < 0) continue;
    mb.nbasis = ha->dim;
    mb.isym = i;
    mb.basis = malloc(sizeof(int)*mb.nbasis);
    mb.ene = malloc(sizeof(double)*mb.nbasis);
    memcpy(mb.basis, ha->basis, sizeof(int)*mb.nbasis);
    memcpy(mb.ene, ha->hamilton, sizeof(double)*mb.nbasis);
    mb.bmax = mb.basis[mb.nbasis-1];
    ArrayAppend(&base, &mb, NULL);
  }

  ncc1 = 0;  
  for (rg = icg0; rg < icg1; rg++) {
    ncc0 = icg[rg];
    ncc = 0;
    ccp = ArrayGet(&ccfg, icg[rg]);
    /*
    printf("RANK: %2d, %d %d %d %d %d %d Complex: %d %d %d\n", 
	   sr, rg, icg0, icg1, icg[rg], icg[rg+1], ccfg.dim, 
	   ccp->ig, ccp->np, ccp->nq);
    */
    for (p = icg[rg]; p < icg[rg+1]; p++) {
      ccp = ArrayGet(&ccfg, p);
      kgp = GroupIndex(gn);
      AddMBPTConfig(kgp, ccp);
      ncc++;
      if (ncc == ccfg.block || p == icg[rg+1]-1) {
	t2 = clock();
	t2 /= CLOCKS_PER_SEC;
	/*
	printf("RANK: %2d, %5d %7d %7d %7d %10.3E %10.3E\n", sr, ncc, p+1, 
	       icg[rg+1], ccfg.dim, t2-t1, t2-t0);	
	fflush(stdout);
	*/
	t1 = t2;
	for (i = 0; i < base.dim; i++) {
	  mbp = ArrayGet(&base, i);
	  nk = ConstructHamiltonDiagonal(mbp->isym, 1, &kgp, 0);
	  if (nk < 0) continue;
	  nbs1 = ha->dim;
	  bs1 = ha->basis;
	  e1 = ha->hamilton;	
	  /*
	  printf("RANK: %2d, sym: %3d %3d %3d %6d\n", 
		 sr, i, mbp->isym, mbp->nbasis, nbs1);
	  fflush(stdout);
	  */
	  sym = GetSymmetry(mbp->isym);
	  nbs0 = mbp->nbasis;
	  bs0 = mbp->basis;
	  ham = malloc(sizeof(double)*nbs0*nbs1);
	  m = 0;
	  for (q = 0; q < nbs1; q++) {
	    k1 = bs1[q];
	    st = ArrayGet(&(sym->states), k1);
	    r = st->kcfg + ncc0;
	    ccp1 = ArrayGet(&ccfg, r);
	    for (t = 0; t < nbs0; t++) {
	      k0 = bs0[t];
	      ham[m] = HamiltonElement(mbp->isym, k0, k1);
	      m++;
	    }	    
	  }
	  for (q = 0; q < nbs1; q++) {
	    st = ArrayGet(&(sym->states), bs1[q]);
	    r = st->kcfg + ncc0;
	    ccp1 = ArrayGet(&ccfg, r);
	    if (ccp1->ncs) continue;
	    for (r = 0; r < nbs0; r++) {
	      m = q*nbs0 + r;
	      a1 = ham[m];
	      b = a1/(mbp->ene[r] - e1[q]);
	      b = b*b;
	      if (b >= ccut) {
		ccp1->ncs = 1;
		ncc1++;		
		/*
		printf("RANK: %2d, %4d %4d %12.5E %12.5E %2d %2d\n",
		       sr, bs0[r], bs1[q], a1, b, ccp1->inp, ccp1->inq);
		*/
		break;
	      }
	    }
	  }
	  free(ham);
	}
	RemoveGroup(kgp);
	ReinitRecouple(0);
	ncc = 0;
	ncc0 += ccfg.block;
      }
    }
  }
#ifdef USE_MPI
  ncc1 = 0;
  for (p = 0; p < ccfg.dim; p++) {
    ccp = ArrayGet(&ccfg, p);
    if (ccp->ncs) ncc1++;
  }
  MPI_Allreduce(&ncc1, &ncc0, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  if (ncc0 > 0) {
    ics1 = malloc(sizeof(int)*ncs0);
    ics0 = malloc(sizeof(int)*ncs0*nr);
  }
  for (i = 0; i < ncc0; i++) ics1[i] = -1;
  i = 0;
  for (p = 0; p < ccfg.dim; p++) {
    ccp = ArrayGet(&ccfg, p);
    if (ccp->ncs) ics1[i++] = p;
  }
  /*
  printf("RANK: %2d, %d %d\n", sr, ncc1, ncc0);
  */
  MPI_Allgather(ics1, ncc0, MPI_INT, ics0, ncc0, MPI_INT, MPI_COMM_WORLD);
  for (p = 0; p < ncc0*nr; p++) {
    if (ics0[p] >= 0) {
      ccp = ArrayGet(&ccfg, ics0[p]);
      ccp->ncs = 1;
    }
  }
  if (ncc0) {
    free(ics0);
    free(ics1);
  }
#endif

 ADDCFG:  
  if (sr == 0) {
    kgp = GroupIndex(gn);
    ncc = 0;
    for (p = 0; p < ccfg.dim; p++) {
      ccp = ArrayGet(&ccfg, p);
      if (ccut <= 0 || ccp->ncs) {
	AddMBPTConfig(kgp, ccp);
	ncc++;
      }
    }   
    g1 = GetGroup(kgp);    
    for (p = 0; p < g1->n_cfgs; p++) {
      c1 = GetConfigFromGroup(kgp, p);
      ConstructConfigName(cname, 2000, c1);
      fprintf(f, "CC: %4d   %s\n", p, cname);
    }
    fclose(f);
  }

 DONE:
  free(bc);
  free(bc1);
  free(bk);
  free(icg);
  ArrayFree(&ccfg, NULL);
  for (i = 0; i < base.dim; i++) {
    mbp = ArrayGet(&base, i);
    free(mbp->basis);
    free(mbp->ene);
  }
  ArrayFree(&base, NULL);

  return 0;
}

int RadialBasisMBPT(int kmax, int n, int *ng, int **bas) {
  int nb, k, j, k2, i, m, ka;
  
  nb = n*(kmax+1)*2;
  (*bas) = malloc(sizeof(int)*nb);
  m = 0;
  for (k = 0; k <= kmax; k++) {
    k2 = 2*k;
    for (j = k2-1; j <= k2+1; j += 2) {
      if (j < 0) continue;
      ka = GetKappaFromJL(j, k2);
      for (i = 0; i < n; i++) {
	if (ng[i] <= k) continue;
	(*bas)[m] = OrbitalIndex(ng[i], ka, 0);
	/*
	printf("%2d %2d %2d %2d\n", (*bas)[m], ng[i], k, j);
	*/
	m++;
      }
    }
  }
  nb = m;
  (*bas) = ReallocNew(*bas, nb*sizeof(int));

  return nb;
}

int PadStates(CONFIG *c1, CONFIG *c2, SHELL **bra, SHELL **ket,
	      SHELL_STATE **sbra, SHELL_STATE **sket) {
  int i, j, m, k, ns;
  SHELL *bra1, *ket1;
  SHELL_STATE *sbra1, *sket1, *s1, *s2;

  s1 = c1->csfs;
  s2 = c2->csfs;

  ns = c1->n_shells + c2->n_shells + 2;
  *bra = malloc(sizeof(SHELL)*ns);
  *ket = malloc(sizeof(SHELL)*ns);
  *sbra = malloc(sizeof(SHELL_STATE)*ns);
  *sket = malloc(sizeof(SHELL_STATE)*ns);
  for (i = 0; i < 2; i++) {
    (*bra)[i].nq = 0;
    (*ket)[i].nq = 0;
    (*sbra)[i].shellJ = 0;
    (*sbra)[i].totalJ = s1[0].totalJ;
    (*sket)[i].shellJ = 0;
    (*sket)[i].totalJ = s2[0].totalJ;
    (*sbra)[i].nu = 0;
    (*sbra)[i].Nr = 0;
    (*sket)[i].nu = 0;
    (*sket)[i].Nr = 0;
  }
  bra1 = *bra + 2;
  ket1 = *ket + 2;
  sbra1 = *sbra + 2;
  sket1 = *sket + 2;
  m = 0;
  i = 0;
  j = 0;
  while (i < c1->n_shells || j < c2->n_shells) {
    if (i == c1->n_shells) {
      k = -1;
    } else if (j == c2->n_shells) {
      k = 1;
    } else {
      k = CompareShell(c1->shells+i, c2->shells+j);
    }
    if (k == 0) {
      memcpy(bra1+m, c1->shells+i, sizeof(SHELL));
      memcpy(ket1+m, c2->shells+j, sizeof(SHELL));
      memcpy(sbra1+m, s1+i, sizeof(SHELL_STATE));
      memcpy(sket1+m, s2+j, sizeof(SHELL_STATE));
      i++;
      j++;
      m++;
    } else if (k < 0) {
      memcpy(ket1+m, c2->shells+j, sizeof(SHELL));
      memcpy(sket1+m, s2+j, sizeof(SHELL_STATE));
      memcpy(bra1+m, c2->shells+j, sizeof(SHELL));
      memcpy(sbra1+m, s2+j, sizeof(SHELL_STATE));
      bra1[m].nq = 0;
      sbra1[m].shellJ = 0;
      sbra1[m].nu = 0;
      sbra1[m].Nr = 0;
      if (i == c1->n_shells) {
	sbra1[m].totalJ = 0;
      } else {
	sbra1[m].totalJ = s1[i].totalJ;
      }
      j++;
      m++;
    } else if (k > 0) {
      memcpy(ket1+m, c1->shells+i, sizeof(SHELL));
      memcpy(sket1+m, s1+i, sizeof(SHELL_STATE));
      memcpy(bra1+m, c1->shells+i, sizeof(SHELL));
      memcpy(sbra1+m, s1+i, sizeof(SHELL_STATE));
      ket1[m].nq = 0;
      sket1[m].shellJ = 0;
      sket1[m].nu = 0;
      sket1[m].Nr = 0;
      if (j == c2->n_shells) {
	sket1[m].totalJ = 0;
      } else {
	sket1[m].totalJ = s2[j].totalJ;
      }
      i++;
      m++;
    }
  }
  ns = m;

  return ns;
}

int CheckInteraction(int ns, SHELL *bra, SHELL *ket, 
		     int np, int *op, int nm, int *om) {
  int i, k, nqk, ph, j;

  for (i = 0; i < ns; i++) {
    nqk = ket[i].nq;
    k = IsPresent(i, np, op);
    nqk += k;
    k = IsPresent(i, nm, om);
    nqk -= k;
    if (nqk != bra[i].nq) return -1;
  }
  ph = 0;
  for (i = 0; i < np; i++) {
    for (j = op[i]+1; j < ns; j++) {
      ph += ket[j].nq;
    }
  }
  for (i = 0; i < nm; i++) {
    for (j = om[i]+1; j < ns; j++) {
      ph += ket[j].nq;
    }
  }

  return ph;
}

int CheckConfig(int ns, SHELL *ket, int np, int *op, int nm, int *om,
		int nc, CONFIG **cs) {
  int i, k, m, nq;
  CONFIG *c;

  k = 0;
  if (nc > 0) {
    c = cs[nc];
  }

  for (i = 0; i < ns; i++) {
    nq = ket[i].nq;
    m = IsPresent(i, nm, om);
    nq -= m;
    if (nq < 0) return 0;
    m = IsPresent(i, np, op);
    nq += m;
    if (nq > 0) {
      if (nq > GetJFromKappa(ket[i].kappa)+1.0) return 0;
      if (nc > 0) {
	memcpy(c->shells+k, ket+i, sizeof(SHELL));
	c->shells[k].nq = nq;
	k++;
      }
    }
  }
   
  if (k > 0) {
    qsort(c->shells, k, sizeof(SHELL), CompareShellInvert);
    if (c->shells[0].nq > 1) {
      if (c->shells[0].n > cs[nc]->nnrs) -1;
    } else if (c->n_shells > 1) {
      if (c->shells[1].n > cs[nc]->nnrs) -1;
    }
    for (i = 0; i < nc; i++) {
      if (k != cs[i]->n_shells) continue;
      m = memcmp(c->shells, cs[i]->shells, sizeof(SHELL)*k);
      if (m == 0) return i;
    }
  }
  return -1;
}

double SumInterp1D(int n, double *z, double *x, double *t, double *y) {
  int i, k0, k1, nk;
  double r, a, b, c, d, e, f, g, h;
  double p1, p2, p3, q1, q2, q3;

  r = 0.0;
  for (i = 0; i < n; i++) {
    r += z[i];
  }
  if (1+r == 1) return 0.0;
  if (n == 1) return r;

  for (k0 = 1; k0 < n; k0++) {
    if (x[k0]-x[k0-1] > 1) break;
  }
  if (k0 == n) goto END;
  k0--;
  
  for (k1 = n-1; k1 >= k0; k1--) {
    if (z[n-1] > 0) {
      if (z[k1] <= 0) {
	k1++;
	break;
      }
    } else {
      if (z[k1] >= 0) {
	k1++;
	break;
      }
    }
  }
  if (k1 < 0) k1 = 0;

  for (i = Min(k0, k1); i < n; i++) {
    t[i] = log(x[i]);
  }
  nk = n - k0;
  for (i = k0; i < k1; i++) {    
    for (a = x[i]+1; a < x[i+1]; a += 1.0) {      
      d = log(a);
      UVIP3P(3, nk, t+k0, z+k0, 1, &d, &b);
      r += b;
    }
  }
  for (i = k1; i < n; i++) {
    y[i] = log(fabs(z[i]));
  }
  nk = n - k1;
  for (i = k1+1; i < n; i++) {
    for (a = x[i-1]+1; a < x[i]; a += 1.0) {     
      d = log(a);
      UVIP3P(3, nk, t+k1, y+k1, 1, &d, &b);
      b = exp(b);
      if (z[n-1] < 0) b = -b;
      r += b;
    }
  }

 END:
  h = 0.0;
  a = 0.0;
  b = 0.0;
  c = 0.0;
  if (mbpt_extra) {
    if (nk > 2) {
      k0 = n-3;
      k1 = n-2;
      i = n-1;
      p1 = t[k0] - t[k1];
      p2 = t[k0]*t[k0] - t[k1]*t[k1];
      p3 = y[k0] - y[k1];
      q1 = t[k1] - t[i];
      q2 = t[k1]*t[k1] - t[i]*t[i];
      q3 = y[k1] - y[i];
      c = (p3*q1 - q3*p1)/(q1*p2 - p1*q2);
      b = (p3 - c*p2)/p1;
      a = y[k0] - b*t[k0] - c*t[k0]*t[k0];      
      if (c <= 0 && 2.0*log(x[i])*c+b < -1.1) {
	g = 1.0;
	d = x[i]+1.0;
	while (g > EPS3) {
	  e = log(d);
	  f = exp(a + b*e + c*e*e);
	  if (z[i] < 0) f = -f;
	  h += f;
	  g = f*d/(-(2.0*e*c+b)-1.0);
	  g = fabs(g/r);
	  d += 1.0;
	}
	g *= fabs(r);
	if (f < 0) g = -g;
	h += g-f;	
      }
    }
    if (h == 0 && nk > 1) {
      k0 = n-2;
      k1 = n-1;
      a = y[k1] - y[k0];
      b = t[k1] - t[k0];
      a = -a/b;      
      if (a > 1.2) {
	a -= 1.0;
	b = pow(x[k1]/(1.0+x[k1]), a);
	h = b*z[k1]*x[k1]/a;	
      }
      b = 0.0;
      c = 0.0;
    }
    if (mbpt_extra == 2) {
      for (i = 0; i < n; i++) {
	printf("  %12.5E %12.5E\n", x[i], z[i]);
      }
      printf("# %12.5E %12.5E %12.5E %12.5E %12.5E\n", a, b, c, h, r);
    }
    r += h;
  }
  return r;
}

double SumInterpH(int n, int *ng, int n2, int *ng2, 
		  double *h, double *h1, double *w) {
  double r, *p, *x, *y, *z;
  int m, i0, i1;
  
  i0 = n+n2;
  x = w + i0;
  y = x + i0;
  z = y + i0;
  for (i0 = 0; i0 < n; i0++) {
    x[i0] = ng[i0];
  }
  r = SumInterp1D(n, h1, x, w, y); 
  p = h;
  for (i0 = 0; i0 < n; i0++) {
    for (i1 = 0; i1 < n2; i1++) {
      x[i1] = ng2[i1] + ng[i0];
    }
    z[i0] = SumInterp1D(n2, p, x, w, y);
    p += n2;
  }
  for (i0 = 0; i0 < n; i0++) {
    x[i0] = ng[i0];
  }
  r += SumInterp1D(n, z, x, w, y);
  
  return r;
}

#define MKK 20

void FixTotalJ(int ns, SHELL_STATE *st, SHELL *s, CONFIG *c, int m) {
  SHELL_STATE *st0;
  int i, j;

  st0 = c->csfs + m*c->n_shells;
  i = 0;
  j = 0;
  while (i < ns && j < c->n_shells) {
    st[i].totalJ = st0[j].totalJ;
    if (s[i].nq > 0) {
      st[i].shellJ = st0[j].shellJ;
      st[i].nu = st0[j].nu;
      st[i].Nr = st0[j].Nr;
      j++;
    }
    i++;    
  }
  if (i < ns || j < c->n_shells) {
    printf("Error in FixTotalJ, aborting\n");
    exit(1);
  }
}

void H22Term(MBPT_EFF **meff, CONFIG *c0, CONFIG *c1,
	     int ns, SHELL *bra, SHELL *ket, 
	     SHELL_STATE *sbra, SHELL_STATE *sket,
	     int mst, int *bst, int *kst,
	     INTERACT_SHELL *s, int ph, int *ks1, int *ks2,
	     FORMULA *fm, double **a, int md, int i0) {
  int m, kk1, kk2, kmin1, kmin2, kmax1, kmax2;
  int mkk1, mkk2, mkk, k, i1;
  int q0, q1, m0, m1, ms0, ms1, s0, s1;
  double c, y, sd1, sd2, se1, se2;
  double a1[MKK], a2[MKK], *h1, *h2, d1, d2;

  if (md <= 0) {
    TriadsZ(2, 2, fm);	      
    RecoupleTensor(8, s, fm);
  }
  kmin1 = abs(s[0].j-s[1].j);
  kk1 = abs(s[2].j-s[3].j);
  kmin1 = Max(kmin1, kk1);
  kmax1 = s[0].j + s[1].j;
  kk1 = s[2].j + s[3].j;
  kmax1 = Min(kmax1, kk1);
  kmin2 = abs(s[4].j-s[5].j);
  kk2 = abs(s[6].j-s[7].j);
  kmin2 = Max(kmin2, kk2);
  kmax2 = s[4].j + s[5].j;
  kk2 = s[6].j + s[7].j;
  kmax2 = Min(kmax2, kk2);
  
  kk1 = 2*(MKK-1);
  kmax1 = Min(kmax1, kk1);
  kmax2 = Min(kmax2, kk1);
  if (kmax1 < kmin1) return;
  if (kmax2 < kmin2) return;
  if (md <= 1) {
    FixJsZ(s, fm);
    for (kk1 = kmin1; kk1 <= kmax1; kk1 += 2) {
      mkk1 = kk1/2;
      mkk = mkk1*MKK;
      for (kk2 = kmin2; kk2 <= kmax2; kk2 += 2) {
	mkk2 = kk2/2;
	fm->js[9] = kk1;
	fm->js[10] = kk1;
	fm->js[11] = kk2;
	fm->js[12] = kk2;
	fm->js[13] = 0;
	fm->js[14] = 0;
	fm->js[15] = 0;	
	for (k = 0; k < mst; k++) {
	  q0 = bst[k];
	  q1 = kst[k];
	  FixTotalJ(ns, sbra, bra, c0, q0);
	  FixTotalJ(ns, sket, ket, c1, q1);
	  ms0 = c0->symstate[q0];
	  UnpackSymState(ms0, &s0, &m0);
	  ms1 = c1->symstate[q1];
	  UnpackSymState(ms1, &s1, &m1);	  
	  EvaluateTensor(ns, sbra, sket, s, 1, fm);
	  if (IsOdd(ph)) fm->coeff = -fm->coeff;
	  a[mkk+mkk2][k] = fm->coeff;
	}
      }
    }
  }
  /* 
  ** these conditions must be placed after the above block,
  ** so that the recouping coeff. are calculated the first
  ** time even these parity conditions are not satisfied.
  */
  if (IsOdd((s[0].kl+s[1].kl+s[2].kl+s[3].kl)/2)) return;
  if (IsOdd((s[4].kl+s[5].kl+s[6].kl+s[7].kl)/2)) return;
  for (kk1 = kmin1; kk1 <= kmax1; kk1 += 2) {
    m = 0;
    mkk1 = kk1/2;
    mkk = mkk1*MKK;
    for (kk2 = kmin2; kk2 <= kmax2; kk2 += 2) {
      mkk2 = kk2/2;
      for (k = 0; k < mst; k++) {
	if (fabs(a[mkk+mkk2][k]) > EPS30) {
	  m = 1;
	  break;
	}
      }
      if (m == 1) break;
    }
    if (m) {
      SlaterTotal(&sd1, &se1, NULL, ks1, kk1, 0);
      a1[mkk1] = sd1 + se1;
    } else {
      a1[mkk1] = 0.0;
    }
  }
  for (kk2 = kmin2; kk2 <= kmax2; kk2 += 2) {
    m = 0;
    mkk2 = kk2/2;
    for (kk1 = kmin1; kk1 <= kmax1; kk1 += 2) {
      mkk1 = kk1/2;
      mkk = mkk1*MKK+mkk2;
      for (k = 0; k < mst; k++) {
	if (fabs(a[mkk][k]) > EPS30) {
	  m = 1;
	  break;
	}
      }      
      if (m == 1) break;
    }
    if (m) {
      SlaterTotal(&sd2, &se2, NULL, ks2, kk2, 0);
      a2[mkk2] = sd2 + se2;
    } else {
      a2[mkk2] = 0.0;
    }
  }

  d1 = GetOrbital(ks2[2])->energy + GetOrbital(ks2[3])->energy;
  d1 -= GetOrbital(ks2[0])->energy + GetOrbital(ks2[1])->energy;
  d2 = GetOrbital(ks1[0])->energy + GetOrbital(ks1[1])->energy;
  d2 -= GetOrbital(ks1[2])->energy + GetOrbital(ks1[3])->energy;
  for (k = 0; k < mst; k++) {
    q0 = bst[k];
    q1 = kst[k];
    ms0 = c0->symstate[q0];
    UnpackSymState(ms0, &s0, &m0);
    ms1 = c1->symstate[q1];
    UnpackSymState(ms1, &s1, &m1);
   
    c = 0.0;
    for (kk1 = kmin1; kk1 <= kmax1; kk1 += 2) {
      mkk1 = kk1/2;
      mkk = mkk1*MKK;
      for (kk2 = kmin2; kk2 <= kmax2; kk2 += 2) {
	mkk2 = kk2/2;
	y = a[mkk+mkk2][k];
	if (fabs(y) < EPS30) continue;
	if (fabs(a1[mkk1]) < EPS30) continue;
	if (fabs(a2[mkk2]) < EPS30) continue;	  
	y /= sqrt((kk1+1.0)*(kk2+1.0));
	if (IsOdd((kk1+kk2)/2)) y = -y;      
	c += y*a1[mkk1]*a2[mkk2];
      }
    }
    if (fabs(c) < EPS30) continue;
    m = m1*(m1+1)/2 + m0;
    if (i0 >= 0) {
      h1 = meff[s0]->hab[m];
      h2 = meff[s0]->hba[m];
      i1 = i0;
    } else {
      h1 = meff[s0]->hab1[m];
      h2 = meff[s0]->hba1[m];
      i1 = -(i0+1);
    }
    h1[i1] += c/d1;
    h2[i1] += c/d2;
  }
}

void H12Term(MBPT_EFF **meff, CONFIG *c0, CONFIG *c1,
	     int ns, SHELL *bra, SHELL *ket, 
	     SHELL_STATE *sbra, SHELL_STATE *sket,
	     int mst, int *bst, int *kst,
	     INTERACT_SHELL *s, int ph, int *ks, int k0, int k1,
	     FORMULA *fm, double **a, int md, int i0) {  
  int kk, kk2, kmin, kmax;
  int q0, q1, k, m0, m1, s0, s1, m, ms0, ms1;  
  double c, r1, y, sd, se, yk[MKK], *h1, *h2, d1, d2;

  if (md <= 0) {
    TriadsZ(1, 2, fm);
    RecoupleTensor(6, s, fm);
  }
  if (s[0].j != s[1].j) return;
  kmin = abs(s[2].j-s[3].j);
  kk = abs(s[4].j-s[5].j);
  kmin = Max(kmin, kk);
  kmax = s[2].j + s[3].j;
  kk = s[4].j + s[5].j;
  kmax = Min(kmax, kk);
  kk = 2*(MKK-1);
  kmax = Min(kmax, kk);
  if (kmax < kmin) return;
  if (md <= 1) {
    FixJsZ(s, fm);
    for (kk = kmin; kk <= kmax; kk += 2) {
      kk2 = kk/2;
      fm->js[7] = 0;
      fm->js[8] = kk;
      fm->js[9] = kk;
      fm->js[10] = 0;
      fm->js[11] = 0;
      for (k = 0; k < mst; k++) {
	q0 = bst[k];
	q1 = kst[k];
	FixTotalJ(ns, sbra, bra, c0, q0);
	FixTotalJ(ns, sket, ket, c1, q1);
	EvaluateTensor(ns, sbra, sket, s, 1, fm);
	if (IsOdd(ph)) fm->coeff = -fm->coeff;
	a[kk2][k] = fm->coeff;
      }
    }
  }
  /* 
  ** these conditions must be placed after the above block,
  ** so that the recouping coeff. are calculated the first
  ** time even these parity conditions are not satisfied.
  */
  if (IsOdd((s[0].kl+s[1].kl)/2)) return;
  if (IsOdd((s[2].kl+s[3].kl+s[4].kl+s[5].kl)/2)) return;

  d1 = GetOrbital(ks[2])->energy + GetOrbital(ks[3])->energy;
  d1 -= GetOrbital(ks[0])->energy + GetOrbital(ks[1])->energy;
  d2 = GetOrbital(k0)->energy - GetOrbital(k1)->energy;
  for (kk = kmin; kk <= kmax; kk += 2) {
    kk2 = kk/2;
    yk[kk2] = 0.0;
    for (k = 0; k < mst; k++) {
      if (a[kk2][k]) break;
    }
    if (k == mst) continue;
    SlaterTotal(&sd, &se, NULL, ks, kk, 0);  
    yk[kk2] = sd + se;
    if (IsOdd(kk2)) yk[kk2] = -yk[kk2];
  }
  for (k = 0; k < mst; k++) {
    q0 = bst[k];
    q1 = kst[k];
    ms0 = c0->symstate[q0];
    UnpackSymState(ms0, &s0, &m0);
    ms1 = c1->symstate[q1];
    UnpackSymState(ms1, &s1, &m1);
    c = 0.0;
    for (kk = kmin; kk <= kmax; kk += 2) {
      kk2 = kk/2;
      if (a[kk2][k] == 0) continue;
      if (yk[kk2] == 0) continue;
      y = a[kk2][k];
      y /= sqrt(kk+1.0);
      c += y*yk[kk2];
    }
    if (fabs(c) < EPS30) continue;
    ResidualPotential(&r1, k0, k1);
    r1 += QED1E(k0, k1);
    r1 *= sqrt(s[0].j+1.0);
    c *= r1;
    /* minus sign is from the definition of Z^k */
    c = -c;    
    if (m0 <= m1) {
      m = m1*(m1+1)/2 + m0;
      h1 = meff[s0]->hab1[m];
      h2 = meff[s0]->hba1[m];
    } else {
      m = m0*(m0+1)/2 + m1;
      h1 = meff[s0]->hba1[m];
      h2 = meff[s0]->hab1[m];
    }
    h1[i0] += c/d1;
    h2[i0] += c/d2;
  }
}

void H11Term(MBPT_EFF **meff, CONFIG *c0, CONFIG *c1, 
	     int ns, SHELL *bra, SHELL *ket,
	     SHELL_STATE *sbra, SHELL_STATE *sket, 
	     int mst, int *bst, int *kst,
	     INTERACT_SHELL *s, int ph, int k0, int k1, int k2, int k3,
	     FORMULA *fm, double *a, int md, int i0) {
  double y, d1, d2, r1, r2, *h1, *h2;
  int q0, q1, k, m0, m1, m, ms0, ms1, s0, s1;

  if (md <= 0) {
    TriadsZ(1, 1, fm);
    RecoupleTensor(4, s, fm);
  }
  if (s[0].j != s[1].j) return;
  if (s[2].j != s[3].j) return;
  if (md <= 1) {
    FixJsZ(s, fm);
    fm->js[5] = 0;
    fm->js[6] = 0;
    fm->js[7] = 0;
    for (k = 0; k < mst; k++) {
      q0 = bst[k];
      q1 = kst[k];
      FixTotalJ(ns, sbra, bra, c0, q0);
      FixTotalJ(ns, sket, ket, c1, q1);	
      EvaluateTensor(ns, sbra, sket, s, 1, fm);
      a[k] = fm->coeff;
      if (IsOdd(ph)) a[k] = -a[k];
    }
  }
  
  /* 
  ** these conditions must be placed after the above block,
  ** so that the recouping coeff. are calculated the first
  ** time even these parity conditions are not satisfied.
  */
  if (IsOdd((s[0].kl+s[1].kl)/2)) return;
  if (IsOdd((s[2].kl+s[3].kl)/2)) return;

  y = 0;
  for (k = 0; k < mst; k++) {
    if (a[k]) break;
  }
  if (k == mst) return;
  
  ResidualPotential(&r1, k0, k1);
  r1 += QED1E(k0, k1);
  r1 *= sqrt(s[0].j+1.0);
  if (k3 != k0 || k2 != k1) {
    ResidualPotential(&r2, k2, k3);
    r2 += QED1E(k2, k3);
    r2 *= sqrt(s[3].j+1.0);
  } else {
    r2 = r1;
  }
  
  d1 = GetOrbital(k3)->energy - GetOrbital(k2)->energy;
  d2 = GetOrbital(k0)->energy - GetOrbital(k1)->energy;
  for (k = 0; k < mst; k++) {
    q0 = bst[k];
    q1 = kst[k];
    ms0 = c0->symstate[q0];
    UnpackSymState(ms0, &s0, &m0);
    ms1 = c1->symstate[q1];
    UnpackSymState(ms1, &s1, &m1);
    if (a[k] == 0) continue;      
    m = m1*(m1+1)/2 + m0;
    h1 = meff[s0]->hab1[m];
    h2 = meff[s0]->hba1[m];
    y = r1*r2*a[k];
    h1[i0] += y/d1;
    h2[i0] += y/d2;
  }
}

void DeltaH22M2Loop(MBPT_EFF **meff, CONFIG *c0, CONFIG *c1, int ns, 
		    SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		    int mst, int *bst, int *kst,
		    int n0, int *b0, int n1, int *b1, int n, int *ng, 
		    int n2, int *ng2, int ph,
		    int ia, int ib, int ic, int id, int ik, int im,
		    FORMULA *fm, double **a, int *j1, int *j2) {
  double c, d1, d2;
  int ip, iq;
  int ks1[4], ks2[4];
  int i, i1, i2, md, m1, m2;
  INTERACT_SHELL s[8];
  ORBITAL *o[8];
  
  ip = im;
  iq = ik;
  ks1[0] = b0[ia];
  ks1[1] = b0[ib];
  ks1[2] = b1[ik];
  ks1[3] = b1[im];
  ks2[0] = b1[iq];
  ks2[1] = b1[ip];
  ks2[2] = b0[ic];
  ks2[3] = b0[id];
  s[0].index = ia+2;
  s[1].index = 0;
  s[2].index = ib+2;
  s[3].index = 1;
  s[4].index = 0;
  s[5].index = ic+2;
  s[6].index = 1;
  s[7].index = id+2;
  if (ik == im) {
    s[1].index = 1;
    s[4].index = 1;
  }
  for (i = 0; i < 8; i++) {
    if (i == 1 || i == 4) {
      o[i] = GetOrbital(b1[ik]);
    } else if (i == 3 || i == 6) {
      o[i] = GetOrbital(b1[im]);
    } else {
      o[i] = GetOrbital(b0[s[i].index-2]);
    }
    if (IsEven(i)) s[i].n = 1;
    else s[i].n = -1;
    s[i].kappa = o[i]->kappa;
    GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
    s[i].nq_bra = bra[s[i].index].nq;
    s[i].nq_ket = ket[s[i].index].nq;
    s[i].index = ns-s[i].index-1;
  }
  
  m1 = o[1]->n;
  m2 = o[3]->n;
  if (m1 > m2) {
    i = m1;
    m1 = m2;
    m2 = i;
  }  
  i1 = IBisect(m1, n, ng);
  if (i1 < 0) return;
  i2 = IBisect(m2-m1, n2, ng2);
  if (i2 < 0) return;
  if (s[1].j != *j1 || s[3].j != *j2) {
    md = (*j1 < 0 || *j2 < 0)?0:1;
    *j1 = s[1].j;
    *j2 = s[3].j;
  } else {
    md = 2;
  }
  i = i1*n2 + i2;
  H22Term(meff, c0, c1, ns, bra, ket, sbra, sket, mst, bst, kst,
	  s, ph, ks1, ks2, fm, a, md, i);
}

void DeltaH22M2(MBPT_EFF **meff, int ns,
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst,
		CONFIG *c0, CONFIG *c1, 
		int n0, int *b0, int n1, int *b1, int n, int *ng, 
		int n2, int *ng2, int nc, CONFIG **cs) {
  int ia, ib, ic, id, ik, im, j1, j2;
  int m1, m2, nj, *jp, i, k;
  int op[4], om[4], ph;
  double *a[MKK*MKK];
  ORBITAL *o;
  FORMULA fm;

  if (n1 <= 0) return;
  k = MKK*MKK;
  for (i = 0; i < k; i++) {
    a[i] = malloc(sizeof(double)*mst);
  }
  jp = malloc(sizeof(int)*(n1+1));
  jp[0] = 0;
  nj = 1;
  o = GetOrbital(b1[0]);
  j1 = GetJFromKappa(o->kappa);
  for (i = 1; i < n1; i++) {
    o = GetOrbital(b1[i]);
    j2 = GetJFromKappa(o->kappa);
    if (j2 != j1) {
      jp[nj] = i;
      nj++;
      j1 = j2;
    }
  }
  jp[nj] = n1;
  fm.js[0] = 0;
  for (ia = 0; ia < n0; ia++) {
    for (ib = 0; ib <= ia; ib++) {
      for (ic = 0; ic < n0; ic++) {
	for (id = 0; id <= ic; id++) {
	  om[0] = id;
	  om[1] = ic;
	  k = CheckConfig(ns-2, ket+2, 0, op, 2, om, 0, NULL);
	  if (k >= 0) continue;
	  om[0] = ia;
	  om[1] = ib;
	  k = CheckConfig(ns-2, bra+2, 0, op, 2, om, 0, NULL);
	  if (k >= 0) continue;
	  op[0] = ib;
	  op[1] = ia;
	  om[0] = ic;
	  om[1] = id;
	  ph = CheckInteraction(ns-2, bra+2, ket+2, 2, op, 2, om);
	  if (ph < 0) continue;
	  if (ng2[0] == 0) {
	    j1 = -1;
	    j2 = -1;
	    for (im = 0; im < n1; im++) {
	      ik = im;
	      o = GetOrbital(b1[im]);
	      ket[1].n = o->n;
	      ket[1].kappa = o->kappa;
	      ket[1].nq = 0;
	      if (ket[1].n <= cs[nc]->n_csfs) {
		om[0] = id+1;
		om[1] = ic+1;
		op[0] = 0;
		op[1] = 0;
		k = CheckConfig(ns-1, ket+1, 2, op, 2, om, nc, cs);
		if (k >= 0) continue;
	      }
	      DeltaH22M2Loop(meff, c0, c1, ns, bra, ket, sbra, sket, 
			     mst, bst, kst,
			     n0, b0, n1, b1, n, ng, n2, ng2, ph,
			     ia, ib, ic, id, ik, im, &fm, a, &j1, &j2);
	    }
	  }
	  j1 = -1;
	  j2 = -1;
	  for (m1 = 0; m1 < nj; m1++) {
	    for (m2 = 0; m2 <= m1; m2++) {
	      for (im = jp[m1]; im < jp[m1+1]; im++) {
		o = GetOrbital(b1[im]);
		ket[0].n = o->n;
		ket[0].kappa = o->kappa;
		ket[0].nq = 0;
		for (ik = jp[m2]; ik < jp[m2+1]; ik++) {
		  if (im <= ik) continue;
		  o = GetOrbital(b1[ik]);
		  ket[1].n = o->n;
		  ket[1].kappa = o->kappa;
		  ket[1].nq = 0;
		  
		  if (Max(ket[0].n, ket[1].n) <= cs[nc]->n_csfs) {
		    om[0] = id+2;
		    om[1] = ic+2;
		    op[0] = 0;
		    op[1] = 1;
		    k = CheckConfig(ns, ket, 2, op, 2, om, nc, cs);
		    if (k >= 0) continue;
		  }
		  DeltaH22M2Loop(meff, c0, c1, ns, bra, ket, sbra, sket, 
				 mst, bst, kst,
				 n0, b0, n1, b1, n, ng, n2, ng2, ph,
				 ia, ib, ic, id, ik, im, &fm, a, &j1, &j2);
		}
	      }
	    }
	  }
	}
      }
    }
  }
  free(jp);
  k = MKK*MKK;
  for (i = 0; i < k; i++) {
    free(a[i]);
  }
}

void DeltaH22M1(MBPT_EFF **meff, int ns,
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst, CONFIG *c0, CONFIG *c1, 
		int n0, int *b0, int n1, int *b1, int n, int *ng,
		int nc, CONFIG **cs) {
  int ia, ib, ic, id, ik, im, ip, iq;
  int op[4], om[4], ph, k, ks1[4], ks2[4];
  int i, m1, i1, j1, md;
  double *a[MKK*MKK];
  INTERACT_SHELL s[8];
  ORBITAL *o[8];
  FORMULA fm;

  fm.js[0] = 0;
  if (n1 <= 0) return;
  k = MKK*MKK;
  for (i = 0; i < k; i++) {
    a[i] = malloc(sizeof(double)*mst);
  }
  for (ia = 0; ia < n0; ia++) {
    for (ib = 0; ib <= ia; ib++) {
      for (ic = 0; ic < n0; ic++) {
	for (id = 0; id <= ic; id++) {
	  for (ik = 0; ik < n0; ik++) {
	    for (iq = 0; iq < n0; iq++) {
	      op[0] = iq;
	      om[0] = id;
	      om[1] = ic;
	      k = CheckConfig(ns-1, ket+1, 1, op, 2, om, 0, NULL);
	      if (k >= 0) continue;
	      op[0] = ik;
	      om[0] = ia;
	      om[1] = ib;
	      k = CheckConfig(ns-1, bra+1, 1, op, 2, om, 0, NULL);
	      if (k >= 0) continue;
	      op[0] = ia;
	      op[1] = ib;
	      op[2] = iq;
	      om[0] = ic;
	      om[1] = id;
	      om[2] = ik;	      
	      ph = CheckInteraction(ns-1, bra+1, ket+1, 3, op, 3, om);
	      if (ph < 0) continue;
	      j1 = -1;
	      for (ip = 0; ip < n1; ip++) {
		o[1] = GetOrbital(b1[ip]);
		ket[0].n = o[1]->n;
		ket[0].kappa = o[1]->kappa;
		ket[0].nq = 0;
		if (ket[0].n <= cs[nc]->n_csfs) {
		  om[0] = id+1;
		  om[1] = ic+1;
		  op[0] = iq+1;
		  op[1] = 0;
		  k = CheckConfig(ns, ket, 2, op, 2, om, nc, cs);
		  if (k >= 0) continue;
		}
		im = ip;
		ks1[0] = b0[ia];
		ks1[1] = b0[ib];
		ks1[2] = b1[im];
		ks1[3] = b0[ik];
		ks2[0] = b0[iq];
		ks2[1] = b1[ip];
		ks2[2] = b0[ic];
		ks2[3] = b0[id];
		s[0].index = ia+1;
		s[1].index = 0;
		s[2].index = ib+1;
		s[3].index = ik+1;
		s[4].index = iq+1;
		s[5].index = ic+1;
		s[6].index = 0;
		s[7].index = id+1;
		i1 = 0;
		for (i = 0; i < 8; i++) {
		  if (i == 1 || i == 6) {
		    o[i] = GetOrbital(b1[ip]);
		    if (i == 1) {
		      m1 = o[1]->n;
		      i1 = IBisect(m1, n, ng);	    
		      if (i1 < 0) break;
		    }
		  } else {
		    o[i] = GetOrbital(b0[s[i].index-1]);
		  }		  
		  if (IsEven(i)) s[i].n = 1;
		  else s[i].n = -1;
		  s[i].kappa = o[i]->kappa;
		  GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
		  s[i].nq_bra = bra[s[i].index].nq;
		  s[i].nq_ket = ket[s[i].index].nq;
		  s[i].index = ns-s[i].index-1;
		}
		if (i1 < 0) continue;
		if (s[1].j != j1) {
		  md = j1<0?0:1;
		  j1 = s[1].j;
		} else {
		  md = 2;
		}
		H22Term(meff, c0, c1, ns, bra, ket, sbra, sket, 
			mst, bst, kst, s, ph, 
			ks1, ks2, &fm, a, md, -(i1+1));
	      }
	    }
	  }
	}
      }
    }
  }
  k = MKK*MKK;
  for (i = 0; i < k; i++) {
    free(a[i]);
  }
}
		  
void DeltaH22M0(MBPT_EFF **meff, int ns,
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst, CONFIG *c0, CONFIG *c1, 
		int n0, int *b0, int n, int *ng, int nc, CONFIG **cs) {
  int ia, ib, ic, id, ik, im, ip, iq, i, i1;
  int op[4], om[4], ph, k, ks1[4], ks2[4];
  double *a[MKK*MKK];
  INTERACT_SHELL s[8];
  ORBITAL *o[8];  
  FORMULA fm;

  i1 = bra[0].n;
  i1 = IBisect(i1, n, ng);
  if (i1 < 0) return;
  k = MKK*MKK;
  for (i = 0; i < k; i++) {
    a[i] = malloc(sizeof(double)*mst);
  }
  fm.js[0] = 0;
  for (ia = 0; ia < n0; ia++) {
    for (ib = 0; ib <= ia; ib++) {
      for (ic = 0; ic < n0; ic++) {
	for (id = 0; id <= ic; id++) {
	  for (ik = 0; ik < n0; ik++) {
	    for (im = 0; im <= ik; im++) {
	      for (ip = 0; ip < n0; ip++) {
		for (iq = 0; iq <= ip; iq++) {
		  op[0] = ip;
		  op[1] = iq;
		  om[0] = ic;
		  om[1] = id;
		  k = CheckConfig(ns, ket, 2, op, 2, om, nc, cs);
		  if (k >= 0) continue;
		  op[0] = ik;
		  op[1] = im;
		  om[0] = ia;
		  om[1] = ib;
		  k = CheckConfig(ns, bra, 2, op, 2, om, 0, NULL);
		  if (k >= 0) continue;
		  op[0] = ia;
		  op[1] = ib;
		  op[2] = ip;
		  op[3] = iq;
		  om[0] = ic;
		  om[1] = id;
		  om[2] = ik;
		  om[3] = im;
		  ph = CheckInteraction(ns, bra, ket, 4, op, 4, om);
		  if (ph < 0) continue;
		  ks1[0] = b0[ia];
		  ks1[1] = b0[ib];
		  ks1[2] = b0[ik];
		  ks1[3] = b0[im];
		  ks2[0] = b0[ip];
		  ks2[1] = b0[iq];
		  ks2[2] = b0[ic];
		  ks2[3] = b0[id];
		  s[0].index = ia;
		  s[1].index = ik;
		  s[2].index = ib;
		  s[3].index = im;
		  s[4].index = ip;
		  s[5].index = ic;
		  s[6].index = iq;
		  s[7].index = id;
		  if (ib == ik) {
		    if (im != ik) {
		      i = ks1[2];
		      ks1[2] = ks1[3];
		      ks1[3] = i;
		      i = s[1].index;
		      s[1].index = s[3].index;
		      s[3].index = i;
		    } else {
		      i = ks1[0];
		      ks1[0] = ks1[1];
		      ks1[1] = i;
		      i = s[0].index;
		      s[0].index = s[2].index;
		      s[2].index = i;
		    }
		  }
		  if (iq == ic) {
		    if (ip != iq) {
		      i = ks2[0];
		      ks2[0] = ks2[1];
		      ks2[1] = i;
		      i = s[4].index;
		      s[4].index = s[6].index;
		      s[6].index = i;
		    } else {
		      i = ks2[2];
		      ks2[2] = ks2[3];
		      ks2[3] = i;
		      i = s[5].index;
		      s[5].index = s[7].index;
		      s[7].index = i;
		    }
		  }
		  for (i = 0; i < 8; i++) {
		    o[i] = GetOrbital(b0[s[i].index]);
		    if (IsEven(i)) s[i].n = 1;
		    else s[i].n = -1;
		    s[i].kappa = o[i]->kappa;
		    GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
		    s[i].nq_bra = bra[s[i].index].nq;
		    s[i].nq_ket = ket[s[i].index].nq;
		    s[i].index = ns-s[i].index-1;
		  }
		  H22Term(meff, c0, c1, ns, bra, ket, sbra, sket, 
			  mst, bst, kst, s, ph,
			  ks1, ks2, &fm, a, 0, -(i1+1));
		}
	      }
	    }
	  }
	}
      }
    }
  }
  k = MKK*MKK;
  for (i = 0; i < k; i++) {
    free(a[i]);
  }
}

void DeltaH12M1(MBPT_EFF **meff, int ns,
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst, CONFIG *c0, CONFIG *c1, 
		int n0, int *b0, int n1, int *b1, int n, int *ng,
		int nc, CONFIG **cs) {
  int ia, ib, ic, id, ik, im, j1, md;
  int m1, i1, k, i, op[3], om[3], ks[4], ph;
  double *a[MKK];
  INTERACT_SHELL s[6];
  ORBITAL *o[6];
  FORMULA fm;

  fm.js[0] = 0;
  if (n1 <= 0) return;
  k = MKK;
  for (i = 0; i < k; i++) {
    a[i] = malloc(sizeof(double)*mst);
  }
  for (ia = 0; ia < n0; ia++) {
    if (bra[ia+1].nq == 0) continue;
    for (ib = 0; ib < n0; ib++) {
      for (ic = 0; ic <= ib; ic++) {
	for (id = 0; id < n0; id++) {	  
	  op[0] = id;
	  om[0] = ic;
	  om[1] = ib;
	  k = CheckConfig(ns-1, ket+1, 1, op, 2, om, 0, NULL);
	  if (k >= 0) continue;
	  op[0] = ia;
	  op[1] = id;
	  om[0] = ic;
	  om[1] = ib;
	  ph = CheckInteraction(ns-1, bra+1, ket+1, 2, op, 2, om);
	  if (ph < 0) continue;
	  j1 = -1;
	  for (ik = 0; ik < n1; ik++) {
	    o[1] = GetOrbital(b1[ik]);
	    ket[0].n = o[1]->n;
	    ket[0].kappa = o[1]->kappa;
	    ket[0].nq = 0;
	    if (ket[0].n <= cs[nc]->n_csfs) {
	      om[0] = ic+1;
	      om[1] = ib+1;
	      op[0] = id+1;
	      op[1] = 0;
	      k = CheckConfig(ns, ket, 2, op, 2, om, nc, cs);
	      if (k >= 0) continue;
	    }
	    im = ik;
	    ks[0] = b0[id];
	    ks[1] = b1[im];
	    ks[2] = b0[ib];
	    ks[3] = b0[ic];
	    s[0].index = ia+1;
	    s[1].index = 0;
	    s[2].index = id+1;
	    s[3].index = ib+1;
	    s[4].index = 0;
	    s[5].index = ic+1;
	    i1 = 0;
	    for (i = 0; i < 6; i++) {
	      if (i == 1 || i == 4) {		
		o[i] = GetOrbital(b1[ik]);
		if (i == 1) {
		  m1 = o[1]->n;
		  i1 = IBisect(m1, n, ng);	    
		  if (i1 < 0) break;
		}
	      } else {
		o[i] = GetOrbital(b0[s[i].index-1]);
	      }
	      if (IsEven(i)) s[i].n = 1;
	      else s[i].n = -1;
	      s[i].kappa = o[i]->kappa;
	      GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
	      s[i].nq_bra = bra[s[i].index].nq;
	      s[i].nq_ket = ket[s[i].index].nq;
	      s[i].index = ns-s[i].index-1;
	    }
	    if (i1 < 0) continue;
	    if (s[1].j != j1) {
	      md = j1<0?0:1;
	      j1 = s[1].j;
	    } else {
	      md = 2;
	    }
	    H12Term(meff, c0, c1, ns, bra, ket, sbra, sket, 
		    mst, bst, kst, s, ph, 
		    ks, b0[ia], b1[ik], &fm, a, md, i1);
	  }
	}
      }
    }
  }
  k = MKK;
  for (i = 0; i < k; i++) {
    free(a[i]);
  }
}

void DeltaH12M0(MBPT_EFF **meff, int ns,
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst, CONFIG *c0, CONFIG *c1, 
		int n0, int *b0, int n, int *ng, int nc, CONFIG **cs) {
  int ia, ib, ic, id, ik, im;
  int i1, k, i, op[3], om[3], ks[4], ph;
  double *a[MKK];
  INTERACT_SHELL s[6];
  ORBITAL *o[6];
  FORMULA fm;
  
  i1 = bra[0].n;
  i1 = IBisect(i1, n, ng);
  if (i1 < 0) return;
  k = MKK;
  for (i = 0; i < k; i++) {
    a[i] = malloc(sizeof(double)*mst);
  }
  fm.js[0] = 0;
  for (ia = 0; ia < n0; ia++) {
    if (bra[ia].nq == 0) continue;
    for (ib = 0; ib < n0; ib++) {
      if (ket[ib].nq == 0) continue;
      for (ic = 0; ic <= ib; ic++) {
	if (ket[ic].nq == 0) continue;
	if (ic == ib && ket[ic].nq == 1) continue;
	for (id = 0; id < n0; id++) {
	  for (ik = 0; ik < n0; ik++) {
	    for (im = 0; im <= id; im++) {
	      if (ik == ia) continue;	      
	      op[0] = ia;
	      op[1] = im;
	      op[2] = id;
	      om[0] = ic;
	      om[1] = ib;
	      om[2] = ik;
	      ph = CheckInteraction(ns, bra, ket, 3, op, 3, om);
	      if (ph < 0) continue;
	      op[0] = im;
	      op[1] = id;
	      om[0] = ic;
	      om[1] = ib;
	      k = CheckConfig(ns, ket, 2, op, 2, om, nc, cs);
	      if (k >= 0) continue;
	      op[0] = ik;
	      om[0] = ia;
	      k = CheckConfig(ns, bra, 1, op, 1, om, 0, NULL);
	      if (k >= 0) continue;
	      ks[0] = b0[id];
	      ks[1] = b0[im];
	      ks[2] = b0[ib];
	      ks[3] = b0[ic];
	      s[0].index = ia;
	      s[1].index = ik;
	      s[2].index = id;
	      s[3].index = ib;
	      s[4].index = im;
	      s[5].index = ic;
	      if (im == ib) {
		if (id != im) {
		  i = ks[0];
		  ks[0] = ks[1];
		  ks[1] = i;
		  i = s[2].index;
		  s[2].index = s[4].index;
		  s[4].index = i;
		} else {
		  i = ks[2];
		  ks[2] = ks[3];
		  ks[3] = i;
		  i = s[3].index;
		  s[3].index = s[5].index;
		  s[5].index = i;
		}
	      }
	      for (i = 0; i < 6; i++) {
		o[i] = GetOrbital(b0[s[i].index]);
		if (IsEven(i)) s[i].n = 1;
		else s[i].n = -1;
		s[i].kappa = o[i]->kappa;
		GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
		s[i].nq_bra = bra[s[i].index].nq;
		s[i].nq_ket = ket[s[i].index].nq;
		s[i].index = ns-s[i].index-1;
	      }
	      H12Term(meff, c0, c1, ns, bra, ket, sbra, sket,
		      mst, bst, kst, s, ph,
		      ks, b0[ia], b0[ik], &fm, a, 0, i1);
	    }
	  }
	}
      }
    }
  }
  k = MKK;
  for (i = 0; i < k; i++) {
    free(a[i]);
  }
}

void DeltaH11M1(MBPT_EFF **meff, int ns, 
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst, CONFIG *c0, CONFIG *c1, 
		int n0, int *b0, int n1, int *b1, int n, int *ng,
		int nc, CONFIG **cs) {
  int ia, ib, ik, im, k, k0, k1, k2, k3, i, i1, m1;
  int op[2], om[2], ph, j1, md;
  INTERACT_SHELL s[4];
  ORBITAL *o0, *o1, *o2, *o3;
  double *a;
  FORMULA fm;

  fm.js[0] = 0;
  if (n1 <= 0) return;
  a = malloc(sizeof(double)*mst);
  for (ia = 0; ia < n0; ia++) {
    if (bra[ia+1].nq == 0) continue;
    for (ib = 0; ib < n0; ib++) { 
      if (ket[ib+1].nq == 0) continue;
      op[0] = ia;
      om[0] = ib;
      ph = CheckInteraction(ns-1, bra+1, ket+1, 1, op, 1, om);
      if (ph < 0) continue;
      j1 = -1;
      for (ik = 0; ik < n1; ik++) {
	o1 = GetOrbital(b1[ik]);
	ket[0].n = o1->n;
	ket[0].kappa = o1->kappa;
	ket[0].nq = 0;
	if (ket[0].n <= cs[nc]->n_csfs) {
	  om[0] = ib+1;
	  op[0] = 0;
	  k = CheckConfig(ns, ket, 1, op, 1, om, nc, cs);
	  if (k >= 0) continue;
	}
	im = ik;
	k0 = b0[ia];
	k1 = b1[ik];
	k2 = b1[im];
	k3 = b0[ib];
	s[0].index = ia+1;
	s[0].n = 1;
	s[1].index = 0;
	s[1].n = -1;
	s[2].index = 0;
	s[2].n = 1;
	s[3].index = ib+1;
	s[3].n = -1;	  
	s[0].nq_bra = bra[s[0].index].nq;
	s[0].nq_ket = ket[s[0].index].nq;
	s[1].nq_bra = s[1].nq_ket = 0;
	s[2].nq_bra = s[1].nq_ket = 0;
	s[3].nq_bra = bra[s[3].index].nq;
	s[3].nq_ket = ket[s[3].index].nq;
	o0 = GetOrbital(k0);
	o1 = GetOrbital(k1);
	m1 = o1->n;	
	i1 = IBisect(m1, n, ng);
	if (i1 < 0) continue;
	o2 = GetOrbital(k2);
	o3 = GetOrbital(k3);
	s[0].kappa = o0->kappa;
	s[1].kappa = o1->kappa;
	s[2].kappa = o2->kappa;
	s[3].kappa = o3->kappa;
	for (i = 0; i < 4; i++) {
	  GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
	  s[i].index = ns-s[i].index-1;
	}
	if (s[1].j != j1) {
	  md = j1<0?0:1;
	  j1 = s[1].j;
	} else {
	  md = 2;
	}
	H11Term(meff, c0, c1, ns, bra, ket, sbra, sket, mst, bst, kst, 
		s, ph, k0, k1, k2, k3, &fm, a, md, i1);
      }
    }
  }
  free(a);
}
  
void DeltaH11M0(MBPT_EFF **meff, int ns, 
		SHELL *bra, SHELL *ket, SHELL_STATE *sbra, SHELL_STATE *sket,
		int mst, int *bst, int *kst,
		CONFIG *c0, CONFIG *c1, int n0, int *b0, int n, int *ng,
		int nc, CONFIG **cs) {
  int ia, ib, ik, im, k, k0, k1, k2, k3, i, i1;
  int op[2], om[2], ph;
  INTERACT_SHELL s[4];
  ORBITAL *o0, *o1, *o2, *o3;
  double *a;
  FORMULA fm;
   
  i1 = bra[0].n;
  i1 = IBisect(i1, n, ng);
  if (i1 < 0) return;
  a = malloc(sizeof(double)*mst);
  fm.js[0] = 0;
  for (ia = 0; ia < n0; ia++) {
    if (bra[ia].nq == 0) continue;
    for (ib = 0; ib < n0; ib++) { 
      if (ket[ib].nq == 0) continue;
      for (ik = 0; ik < n0; ik++) {
	for (im = 0; im < n0; im++) {
	  if (ia == ik) {
	    continue;
	  }
	  if (ib == im) {
	    continue;
	  }
	  op[0] = ia;
	  op[1] = im;
	  om[0] = ib;
	  om[1] = ik;
	  ph = CheckInteraction(ns, bra, ket, 2, op, 2, om);
	  if (ph < 0) continue;
	  op[0] = im;
	  om[0] = ib;
	  k = CheckConfig(ns, ket, 1, op, 1, om, nc, cs);	  
	  if (k >= 0) continue;
	  op[0] = ik;
	  om[0] = ia;
	  k = CheckConfig(ns, bra, 1, op, 1, om, 0, NULL);
	  if (k >= 0) continue;
	  k0 = b0[ia];
	  k1 = b0[ik];
	  k2 = b0[im];
	  k3 = b0[ib];
	  s[0].index = ia;
	  s[0].n = 1;
	  s[1].index = ik;
	  s[1].n = -1;
	  s[2].index = im;
	  s[2].n = 1;
	  s[3].index = ib;
	  s[3].n = -1;
	  for (i = 0; i < 4; i++) {
	    s[i].nq_bra = bra[s[i].index].nq;
	    s[i].nq_ket = ket[s[i].index].nq;
	  }
	  o0 = GetOrbital(k0);
	  o1 = GetOrbital(k1);
	  o2 = GetOrbital(k2);
	  o3 = GetOrbital(k3);
	  s[0].kappa = o0->kappa;
	  s[1].kappa = o1->kappa;
	  s[2].kappa = o2->kappa;
	  s[3].kappa = o3->kappa;
	  for (i = 0; i < 4; i++) {
	    GetJLFromKappa(s[i].kappa, &(s[i].j), &(s[i].kl));
	    s[i].index = ns-s[i].index-1;
	  }
	  H11Term(meff, c0, c1, ns, bra, ket, sbra, sket, mst, bst, kst,
		  s, ph, k0, k1, k2, k3, &fm, a, 0, i1);
	}
      }
    }
  }
  free(a);
}
  
void FreeEffMBPT(MBPT_EFF **meff) {
  int i, j, k, m;

  for (i = 0; i < MAX_SYMMETRIES; i++) {
    if (meff[i] == NULL) continue;
    if (meff[i]->nbasis > 0) {
      k = meff[i]->nbasis;
      k = k*(k+1)/2;
      for (m = 0; m < k; m++) {
	if (meff[i]->hab1[m]) {
	  free(meff[i]->hab1[m]);
	  free(meff[i]->hba1[m]);
	  free(meff[i]->hab[m]);
	  free(meff[i]->hba[m]);
	}
      }
      free(meff[i]->hab1);
      free(meff[i]->hba1);
      free(meff[i]->hab);
      free(meff[i]->hba);
      free(meff[i]->h0);
      free(meff[i]->heff);
      free(meff[i]->basis);
    }
    free(meff[i]);
  }
}
	    
int StructureMBPT1(char *fn, char *fn1, int nkg, int *kg, 
		   int kmax, int n, int *ng, int n2, int *ng2, int nkg0) {
  int *bas, *bas0, *bas1, *bas2, nlevels, nb, n3, q;
  int i, j, k, i0, i1, n0, n1, isym, ierr, nc, m, mks, *ks;
  int pp, jj, nmax, na, *ga, k0, k1, m0, m1, nmax1, mst;
  int p0, p1, q0, q1, ms0, ms1, *bst, *kst, *bst0, *kst0;
  SYMMETRY *sym;
  STATE *st;
  HAMILTON *h;
  SHELL *bra, *ket, *bra1, *ket1, *bra2, *ket2;
  SHELL_STATE *sbra, *sket, *sbra1, *sket1, *sbra2, *sket2;
  CONFIG **cs, cfg, *c0, *c1, *ct0, *ct1;
  CONFIG_GROUP *g;
  MBPT_EFF *meff[MAX_SYMMETRIES];
  double a, b, c, *mix, *hab, *hba;
  double *h0, *heff, *hab1, *hba1, *dw, dt, dtt;
  clock_t tt0, tt1, tbg;
  FILE *f;
					  
  ierr = 0;
  n3 = mbpt_n3;
  if (nkg0 <= 0 || nkg0 > nkg) nkg0 = nkg;
  nc = 0;
  for (i = 0; i < nkg; i++) {
    g = GetGroup(kg[i]);
    nc += g->n_cfgs;
  }
  cs = malloc(sizeof(CONFIG *)*(nc+2));
  k = 0;
  i1 = 0;
  nmax = 0;
  nmax1 = 0;
  for (i = 0; i < nkg; i++) {
    g = GetGroup(kg[i]);    
    for (j = 0; j < g->n_cfgs; j++) {
      cs[k] = GetConfigFromGroup(i, j);
      if (cs[k]->n_shells > i1) i1 = cs[k]->n_shells;
      if (cs[k]->shells[0].n > nmax) nmax = cs[k]->shells[0].n;
      if (cs[k]->shells[0].nq > 1) {
	if (cs[k]->shells[0].n > nmax1) nmax1 = cs[k]->shells[0].n;
      } else if (cs[k]->n_shells > 1) {
	if (cs[k]->shells[1].n > nmax1) nmax1 = cs[k]->shells[1].n;
      }
      k++;
    }
  }
  /* use cfg.n_csfs to store the maximum n-value of the configuration in kg */
  cfg.n_csfs = nmax;
  cfg.nnrs = nmax1;
  cs[k] = &cfg;
  cs[k]->n_shells = i1;
  cs[k]->shells = malloc(sizeof(SHELL)*(i1+2));  
  
  tt0 = clock();
  tbg = tt0;
  printf("Construct Radial Basis.\n");
  fflush(stdout);
  n = ConstructNGrid(n, &ng);
  n2 = ConstructNGrid(n2, &ng2);
  na = n*n2+n+nmax;
  ga = malloc(sizeof(int)*na);
  k = 0;
  for (i = 1; i <= nmax; i++) {
    ga[k++] = i;
  }
  nb = RadialBasisMBPT(nmax-1, k, ga, &bas0);
  bas2 = malloc(sizeof(int)*nb);
  k = 0;
  for (i = 0; i < n; i++) {
    ga[k++] = ng[i];
    for (j = 0; j < n2; j++) {
      if (ng2[j] == 0) continue;
      ga[k++] = ng[i]+ng2[j];
    }
  }
  na = SortUnique(k, ga);
  nb = RadialBasisMBPT(kmax, na, ga, &bas);
  bas1 = malloc(sizeof(int)*nb);
  free(ga);

  tt1 = clock();
  dt = (tt1-tt0)/CLOCKS_PER_SEC;
  tt0 = tt1;
  printf("Time = %12.5E\n", dt);
  fflush(stdout);
  if (nb < 0) return -1;

  f = fopen(fn1, "w");
  if (f == NULL) {
    printf("cannot open file %s\n", fn1);
    free(bas);
    return -1;
  }
  fwrite(&n, sizeof(int), 1, f);
  fwrite(ng, sizeof(int), n, f);
  fwrite(&n2, sizeof(int), 1, f);
  fwrite(ng2, sizeof(int), n2, f);
  fwrite(&n3, sizeof(int), 1, f);

  dw = malloc(sizeof(double)*(n+n2)*4);

  printf("CI Structure.\n");
  fflush(stdout);
  nlevels = GetNumLevels();
  for (isym = 0; isym < MAX_SYMMETRIES; isym++) {
    meff[isym] = NULL;
    k = ConstructHamilton(isym, nkg0, nkg, kg, 0, NULL, 110);
    if (k == -1) continue;
    meff[isym] = malloc(sizeof(MBPT_EFF));
    if (k < 0) {
      meff[isym]->nbasis = 0;
      continue;
    }
    sym = GetSymmetry(isym);
    h = GetHamilton();
    meff[isym]->h0 = malloc(sizeof(double)*h->hsize);
    memcpy(meff[isym]->h0, h->hamilton, sizeof(double)*h->hsize);
    meff[isym]->basis = malloc(sizeof(int)*h->n_basis);
    meff[isym]->nbasis = h->n_basis;
    memcpy(meff[isym]->basis, h->basis, sizeof(int)*h->n_basis);
    meff[isym]->heff = malloc(sizeof(double)*h->n_basis*h->n_basis);
    meff[isym]->hab1 = malloc(sizeof(double *)*h->hsize);
    meff[isym]->hba1 = malloc(sizeof(double *)*h->hsize);
    meff[isym]->hab = malloc(sizeof(double *)*h->hsize);
    meff[isym]->hba = malloc(sizeof(double *)*h->hsize);  
    printf("sym: %3d %d\n", isym, h->dim);
    fflush(stdout);    
    if (DiagnolizeHamilton() < 0) {
      printf("Diagnolizing Hamiltonian Error\n");
      fflush(stdout);
      exit(1);
    }    
    tt1 = clock();
    dt = (tt1-tt0)/CLOCKS_PER_SEC;
    tt0 = tt1;
    printf("Time = %12.5E\n", dt);   
    ks = malloc(sizeof(int)*h->dim);
    mks = 0;
    m = 0;
    mix = h->mixing + h->dim;
    for (k = 0; k < h->dim; k++) {
      if (nkg0 != nkg) {
	q = GetPrincipleBasis(mix, h->dim, NULL);
	st = (STATE *) ArrayGet(&(sym->states), h->basis[q]);
	if (InGroups(st->kgroup, nkg0, kg)) {	  
	  if (mbpt_nlev <= 0 || IBisect(m, mbpt_nlev, mbpt_ilev) >= 0) {
	    ks[mks++] = k;
	  }
	  m++;
	}
      } else {
	if (mbpt_nlev <= 0 || IBisect(m, mbpt_nlev, mbpt_ilev) >= 0) {
	  ks[mks++] = k;
	}
	m++;
      }
      mix += h->dim;
    }
    for (j = 0; j < h->dim; j++) {
      for (i = 0; i <= j; i++) {
	c = 0;
	for (m = 0; m < mks; m++) {
	  k = ks[m];
	  mix = h->mixing + h->dim*(k+1);
	  a = fabs(mix[i]*mix[j]);
	  if (a > c) c = a;
	}
	k = j*(j+1)/2 + i;
	a = mbpt_mcut;
	if (i == j) a *= 0.2;
	if (c >= a) {
	  meff[isym]->hab1[k] = malloc(sizeof(double)*n);
	  meff[isym]->hba1[k] = malloc(sizeof(double)*n);
	  i1 = n*n2;
	  meff[isym]->hab[k] = malloc(sizeof(double)*i1);
	  meff[isym]->hba[k] = malloc(sizeof(double)*i1);
	  for (i0 = 0; i0 < n; i0++) {
	    meff[isym]->hab1[k][i0] = 0.0;
	    meff[isym]->hba1[k][i0] = 0.0;
	  } 
	  for (i0 = 0; i0 < i1; i0++) {
	    meff[isym]->hab[k][i0] = 0.0;
	    meff[isym]->hba[k][i0] = 0.0;
	  }
	} else {
	  meff[isym]->hab1[k] = NULL;
	}
      }
    }
    free(ks);
  }
  tt1 = clock();
  dt = (tt1-tt0)/CLOCKS_PER_SEC;
  tt0 = tt1;
  printf("Time = %12.5E\n", dt);

  printf("Construct Effective Hamiltonian.\n");
  fflush(stdout);
  for (k0 = 0; k0 < nc; k0++) {
    c0 = cs[k0];
    p0 = ConfigParity(c0);
    for (k1 = k0; k1 < nc; k1++) {
      c1 = cs[k1];
      p1 = ConfigParity(c1);
      if (p0 != p1) continue;
      m = 0;
      bst0 = malloc(sizeof(int)*c0->n_csfs*c1->n_csfs);
      kst0 = malloc(sizeof(int)*c0->n_csfs*c1->n_csfs);
      for (m0 = 0; m0 < c0->n_csfs; m0++) {
	ms0 = c0->symstate[m0];
	UnpackSymState(ms0, &i0, &q0);	
	if (c0 == c1) q = m0;
	else q = 0;
	for (m1 = q; m1 < c1->n_csfs; m1++) {
	  ms1 = c1->symstate[m1];
	  UnpackSymState(ms1, &i1, &q1);
	  if (i0 != i1) continue;
	  if (q0 <= q1) {
	    k = q1*(q1+1)/2 + q0;
	  } else {
	    k = q0*(q0+1)/2 + q1;
	  }
	  if (meff[i0] && meff[i0]->nbasis > 0 && meff[i0]->hab1[k]) {
	    bst0[m] = m0;
	    kst0[m] = m1;
	    m++;
	  }
	}
      }
      if (m == 0) {
	continue;
      }
      mst = m;
      ms0 = c0->symstate[bst0[0]];
      ms1 = c1->symstate[kst0[0]];
      UnpackSymState(ms0, &i0, &q0);
      UnpackSymState(ms1, &i1, &q1);
      if (q0 <= q1) {
	ct0 = c0;
	ct1 = c1;
	bst = bst0;
	kst = kst0;
      } else {
	ct0 = c1;
	ct1 = c0;
	bst = kst0;
	kst = bst0;
      }
      n0 = PadStates(ct0, ct1, &bra, &ket, &sbra, &sket);
      bra1 = bra + 1;
      ket1 = ket + 1;
      bra2 = bra + 2;
      ket2 = ket + 2;
      sbra1 = sbra + 1;
      sket1 = sket + 1;
      sbra2 = sbra + 2;
      sket2 = sket + 2;	
      for (k = 0; k < n0; k++) {	  
	bas0[k] = OrbitalIndex(bra2[k].n, bra2[k].kappa, 0.0);
	bas2[k] = bas0[k];
      }
      qsort(bas2, n0, sizeof(int), CompareInt);
      n1 = 0;
      for (m = 0; m < nb; m++) {
	k = IBisect(bas[m], n0, bas2);
	if (k >= 0) continue;
	bas1[n1] = bas[m];
	n1++;
      }
      if (n3 != 2) {
	DeltaH12M0(meff, n0, bra2, ket2, sbra2, sket2, mst, bst, kst,
		   ct0, ct1, n0, bas0, n, ng, nc, cs);
	DeltaH12M1(meff, n0+1, bra1, ket1, sbra1, sket1, mst, bst, kst,
		   ct0, ct1, n0, bas0, n1, bas1, n, ng, nc, cs);
	
	DeltaH12M0(meff, n0, ket2, bra2, sket2, sbra2, mst, kst, bst,
		   ct1, ct0, n0, bas0, n, ng, nc, cs);	  
	DeltaH12M1(meff, n0+1, ket1, bra1, sket1, sbra1, mst, kst, bst,
		   ct1, ct0, n0, bas0, n1, bas1, n, ng, nc, cs);
		
	DeltaH11M0(meff, n0, bra2, ket2, sbra2, sket2, mst, bst, kst,
		   ct0, ct1, n0, bas0, n, ng, nc, cs);	
	DeltaH11M1(meff, n0+1, bra1, ket1, sbra1, sket1, mst, bst, kst, 
		   ct0, ct1, n0, bas0, n1, bas1, n, ng, nc, cs);
	
	DeltaH22M0(meff, n0, bra2, ket2, sbra2, sket2, mst, bst, kst,
		   ct0, ct1, n0, bas0, n, ng, nc, cs);
	DeltaH22M1(meff, n0+1, bra1, ket1, sbra1, sket1, mst, bst, kst,
		   ct0, ct1, n0, bas0, n1, bas1, n, ng, nc, cs);	
      }      
      if (n3 != 1) {
	DeltaH22M2(meff, n0+2, bra, ket, sbra, sket, mst, bst, kst,
		   ct0, ct1, n0, bas0, n1, bas1, n, ng, n2, ng2, nc, cs);	
      }
      tt1 = clock();
      dt = (tt1-tt0)/CLOCKS_PER_SEC;
      dtt = (tt1-tbg)/CLOCKS_PER_SEC;
      tt0 = tt1;
      printf("%3d %3d %3d %3d %3d %3d ... %12.5E %12.5E\n", 
	     k0, k1, nc, mst, n0, n1, dt, dtt);
      fflush(stdout);
      
      free(bra);
      free(ket);
      free(sbra);
      free(sket);
      free(bst0);
      free(kst0);
    }
  }
  printf("MBPT Structure.\n");
  fflush(stdout);
  i1 = n*n2;
  for (isym = 0; isym < MAX_SYMMETRIES; isym++) {
    if (meff[isym] == NULL) continue;
    if (meff[isym]->nbasis == 0) {
      k = 0;
      fwrite(&isym, sizeof(int), 1, f);
      fwrite(&k, sizeof(int), 1, f);
      continue;
    }
    h = GetHamilton();
    h0 = meff[isym]->h0;
    heff = meff[isym]->heff;
    h->pj = isym;
    h->n_basis = meff[isym]->nbasis;
    h->dim = h->n_basis;
    memcpy(h->basis, meff[isym]->basis, sizeof(int)*h->n_basis);
    h->hsize = h->dim*(h->dim+1)/2;
    k = ConstructHamilton(isym, nkg0, nkg, kg, 0, NULL, 1);
    h->heff = heff;
    DecodePJ(isym, &pp, &jj);
    fwrite(&isym, sizeof(int), 1, f);
    fwrite(&(h->dim), sizeof(int), 1, f);
    fflush(f);
    printf("sym: %3d %d\n", isym, h->dim);
    fflush(stdout);
    for (j = 0; j < h->dim; j++) {
      for (i = 0; i <= j; i++) {
	k = j*(j+1)/2 + i;
	if (meff[isym]->hab1[k] == NULL) {
	  a = h0[k];
	  m = j*h->dim + i;
	  heff[m] = a;
	  if (i < j) {
	    m = i*h->dim + j;
	    heff[m] = a;
	  }
	  b = 0.0;
	  c = 0.0;
	  q = -i-1;
	  k = -j-1;
	  fwrite(&q, sizeof(int), 1, f);
	  fwrite(&k, sizeof(int), 1, f);
	  fwrite(&a, sizeof(double), 1, f);
	  fwrite(&b, sizeof(double), 1, f);
	  fwrite(&c, sizeof(double), 1, f);
	  continue;
	}
	hab1 = meff[isym]->hab1[k];
	hba1 = meff[isym]->hba1[k];
	hab = meff[isym]->hab[k];
	hba = meff[isym]->hba[k];
	a = sqrt(jj+1.0);
	for (i0 = 0; i0 < n; i0++) {
	  hab1[i0] /= a;
	  hba1[i0] /= a;
	}
	for (i0 = 0; i0 < i1; i0++) {
	  hab[i0] /= a;
	  hba[i0] /= a;
	}
	a = h0[k];
	m = j*h->dim + i;
	b = SumInterpH(n, ng, n2, ng2, hab, hab1, dw);
	heff[m] = a+b;
	if (i < j) {
	  m = i*h->dim + j;
	  c = SumInterpH(n, ng, n2, ng2, hba, hba1, dw);
	  heff[m] = a+c;
	} else {
	  c = b;
	}
	fwrite(&i, sizeof(int), 1, f);
	fwrite(&j, sizeof(int), 1, f);
	fwrite(&a, sizeof(double), 1, f);
	fwrite(&b, sizeof(double), 1, f);
	fwrite(&c, sizeof(double), 1, f);
	if (n3 != 2) {
	  fwrite(hab1, sizeof(double), n, f);
	  if (i != j) {
	    fwrite(hba1, sizeof(double), n, f);
	  }
	}
	if (n3 != 1) {
	  fwrite(hab, sizeof(double), i1, f);
	  if (i != j) {
	    fwrite(hba, sizeof(double), i1, f);
	  }
	}
      }
    }
    fflush(f);
    if (DiagnolizeHamilton() < 0) {
      printf("Diagnolizing Effective Hamiltonian Error\n");
      ierr = -1;
      goto ERROR;
    }
    AddToLevels(nkg0, kg);
    h->heff = NULL;
  }
  SortLevels(nlevels, -1);
  SaveLevels(fn, nlevels, -1);

  tt1 = clock();
  dt = (tt1-tt0)/CLOCKS_PER_SEC;
  printf("Time = %12.5E\n", dt);
  dt = (tt1 - tbg)/CLOCKS_PER_SEC;
  printf("Total Time = %12.5E\n", dt);

 ERROR:
  fclose(f);
  free(bas);
  free(bas0);
  free(bas1);
  free(bas2);
  free(cs[nc]->shells);
  free(cs);
  free(dw);
  FreeEffMBPT(meff);

  return ierr;
}

int ReadMBPT(int nf, FILE *f[], MBPT_HAM *mbpt, int m) {
  int i, nb, nn2;

  if (m == 0) {
    for (i = 0; i < nf; i++) {
      nb = fread(&(mbpt[i].n), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
      mbpt[i].ng = malloc(sizeof(int)*mbpt[i].n);
      nb = fread(mbpt[i].ng, sizeof(int), mbpt[i].n, f[i]);
      if (nb != mbpt[i].n) return -1;      
      nb = fread(&(mbpt[i].n2), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
      mbpt[i].ng2 = malloc(sizeof(int)*mbpt[i].n2);
      nb = fread(mbpt[i].ng2, sizeof(int), mbpt[i].n2, f[i]);
      if (nb != mbpt[i].n2) return -1;
      nb = fread(&(mbpt[i].n3), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
      mbpt[i].hab1 = malloc(sizeof(double)*mbpt[i].n);
      mbpt[i].hba1 = malloc(sizeof(double)*mbpt[i].n);
      mbpt[i].hab = malloc(sizeof(double)*mbpt[i].n*mbpt[i].n2);
      mbpt[i].hba = malloc(sizeof(double)*mbpt[i].n*mbpt[i].n2);
    }
    return 0;
  }

  if (m == 1) {
    for (i = 0; i < nf; i++) {
      nb = fread(&(mbpt[i].isym), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
      nb = fread(&(mbpt[i].dim), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
    }
    return 0;
  }
  
  if (m == 2) {
    for (i = 0; i < nf; i++) {
      if (mbpt[i].dim == 0) continue;
      nb = fread(&(mbpt[i].ibra), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
      nb = fread(&(mbpt[i].iket), sizeof(int), 1, f[i]);
      if (nb != 1) return -1;
      nb = fread(&(mbpt[i].a), sizeof(double), 1, f[i]);
      if (nb != 1) return -1;
      nb = fread(&(mbpt[i].b), sizeof(double), 1, f[i]);
      if (nb != 1) return -1;
      nb = fread(&(mbpt[i].c), sizeof(double), 1, f[i]);
      if (nb != 1) return -1;
      if (mbpt[i].ibra < 0 || mbpt[i].iket < 0) continue;
      if (mbpt[i].n3 != 2) {
	nb = fread(mbpt[i].hab1, sizeof(double), mbpt[i].n, f[i]);
	if (nb != mbpt[i].n) return -1;
	if (mbpt[i].ibra != mbpt[i].iket) {
	  nb = fread(mbpt[i].hba1, sizeof(double), mbpt[i].n, f[i]);
	  if (nb != mbpt[i].n) return -1;
	} else {
	  memcpy(mbpt[i].hba1, mbpt[i].hab1, sizeof(double)*mbpt[i].n);
	}
      } 
      if (mbpt[i].n3 != 1) {
	nn2 = mbpt[i].n*mbpt[i].n2;
	nb = fread(mbpt[i].hab, sizeof(double), nn2, f[i]);
	if (nb != nn2) return -1;
	if (mbpt[i].ibra != mbpt[i].iket) {
	  nb = fread(mbpt[i].hba, sizeof(double), nn2, f[i]);
	  if (nb != nn2) return -1;
	} else {
	  memcpy(mbpt[i].hba, mbpt[i].hab, sizeof(double)*nn2);
	}
      }
    }
    return 0;
  }
  
  return 0;
}

void CombineMBPT(int nf, MBPT_HAM *mbpt, double *hab1, double *hba1,
		 double **hab, double **hba, int n0, int *ng0,
		 int n, int *ng, int *n2, int **ng2) {
  int m, i, j, k, q, r;

  for (m = 0; m < nf; m++) {
    if (mbpt[m].ibra < 0 || mbpt[m].iket < 0) continue;
    if (mbpt[m].n3 != 2) {
      for (i = 0; i < mbpt[m].n; i++) {
	k = IBisect(mbpt[m].ng[i], n0, ng0);
	if (k < 0) continue;
	hab1[k] = mbpt[m].hab1[i];
	hba1[k] = mbpt[m].hba1[i];
      }
    }
    if (mbpt[m].n3 != 1) {
      for (i = 0; i < mbpt[m].n; i++) {
	k = IBisect(mbpt[m].ng[i], n, ng);
	if (k < 0) continue;
	r = i*mbpt[m].n2;
	for (j = 0; j < mbpt[m].n2; j++) {
	  q = IBisect(mbpt[m].ng2[j], n2[k], ng2[k]);
	  if (q < 0) continue;
	  hab[k][q] = mbpt[m].hab[r+j];
	  hba[k][q] = mbpt[m].hba[r+j];
	}
      }
    }
  }
}

int StructureReadMBPT(char *fn, char *fn2, int nf, char *fn1[],
		      int nkg, int *kg, int nkg0) {
  int ierr, m, i, j, k, k0, k1, nlevels, n2m;
  int isym, pp, jj, r, q, n, *ng, n0, *ng0, *n2, **ng2;
  double *dw, *z1, *z2, *x, *y, *z, *t, *heff, **hab, **hba;
  HAMILTON *h;
  SYMMETRY *sym;
  MBPT_HAM *mbpt;
  FILE **f1, *f2;
  
  ierr = 0;
  if (nkg0 <= 0 || nkg0 > nkg) nkg0 = nkg;
  mbpt = malloc(sizeof(MBPT_HAM)*nf);
  f1 = malloc(sizeof(FILE *)*nf);
  for (m = 0; m < nf; m++) {
    f1[m] = fopen(fn1[m], "r");
    if (f1[m] == NULL) {
      printf("cannot open file %s\n", fn1[m]);
      return -1;
    }
  }
  f2 = fopen(fn2, "w");
  if (f2 == NULL) {
    printf("cannot open file %s\n", fn2);
    return -1;
  }
  
  ierr = ReadMBPT(nf, f1, mbpt, 0);
  if (ierr < 0) return -1;

  n = 0;
  n0 = 0;
  for (m = 0; m < nf; m++) {
    if (mbpt[m].n3 != 2) n0 += mbpt[m].n;
    if (mbpt[m].n3 != 1) n += mbpt[m].n;
  }    
  if (n0 > 0) ng0 = malloc(sizeof(int)*n0);
  if (n > 0) ng = malloc(sizeof(int)*n);
  i = 0;
  j = 0;
  for (m = 0; m < nf; m++) {
    if (mbpt[m].n3 != 1) {
      for (k = 0; k < mbpt[m].n; k++) {      
	ng[i++] = mbpt[m].ng[k];
      }
    }
    if (mbpt[m].n3 != 2) {
      for (k = 0; k < mbpt[m].n; k++) {
	ng0[j++] = mbpt[m].ng[k];
      }
    }
  }
  if (i > 0) {
    n = SortUnique(i, ng);
  }
  if (j > 0) {
    n0 = SortUnique(j, ng0);  
  }
  n2m = 0;
  if (n > 0) {
    n2 = malloc(sizeof(int)*n);
    ng2 = malloc(sizeof(int *)*n);    
    for (i = 0; i < n; i++) {
      n2[i] = 0;
      for (m = 0; m < nf; m++) {
	if (mbpt[m].n3 != 1) {
	  n2[i] += mbpt[m].n2;
	}
      }
      if (n2[i] > 0) {
	ng2[i] = malloc(sizeof(int)*n2[i]);
	j = 0;
	for (m = 0; m < nf; m++) {
	  if (mbpt[m].n3 != 1) {
	    for (k = 0; k < mbpt[m].n2; k++) {
	      ng2[i][j++] = mbpt[m].ng2[k];
	    }
	  }
	}
	if (j > 0) {
	  n2[i] = SortUnique(j, ng2[i]);
	}
      }
      if (n2[i] > n2m) n2m = n2[i];
    }
    hab = malloc(sizeof(double *)*n);
    hba = malloc(sizeof(double *)*n);
    for (i = 0; i < n; i++) {
      if (n2[i] > 0) {
	hab[i] = malloc(sizeof(double)*n2[i]);
	hba[i] = malloc(sizeof(double)*n2[i]);
      }
    }
  }
  n2m += n0;
  dw = malloc(sizeof(double)*n2m*5);
  z1 = dw;
  z2 = z1 + n2m;
  x = z2 + n2m;
  t = x + n2m;
  y = t + n2m;

  nlevels = GetNumLevels();
  for (isym = 0; isym < MAX_SYMMETRIES; isym++) {
    k0 = ConstructHamilton(isym, nkg0, nkg, kg, 0, NULL, 101);
    if (k0 == -1) continue;
    h = GetHamilton();
    sym = GetSymmetry(isym);
    DecodePJ(isym, &pp, &jj);
    ierr = ReadMBPT(nf, f1, mbpt, 1);
    if (ierr < 0) {
      printf("Error reading MBPT 1: %d\n", isym);
      goto ERROR;
    }
    k = 1;
    for (m = 0; m < nf; m++) {
      if (mbpt[m].dim == 0) {
	k = 0;
	break;
      }
    }
    if (k0 == 0) printf("sym: %3d %d %2d %d %3d\n", isym, pp, jj, k, h->dim);
    if (k == 0 || k0 < 0) {
      for (j = 0; j < h->dim; j++) {
	for (i = 0; i <= j; i++) {	  
	  ierr = ReadMBPT(nf, f1, mbpt, 2);
	}
      }
      continue;
    }
    heff = malloc(sizeof(double)*h->dim*h->dim);
    h->heff = heff;
    for (j = 0; j < h->dim; j++) {
      for (i = 0; i <= j; i++) {
	k = j*(j+1)/2 + i;
	k0 = j*h->dim + i;
	k1 = i*h->dim + j;
	ierr = ReadMBPT(nf, f1, mbpt, 2);
	if (ierr < 0) {
	  printf("Error reading MBPT 2: %d %d %d\n", isym, i, j);
	  goto ERROR;	
	}
	heff[k0] = mbpt[0].a;
	heff[k1] = mbpt[0].a;
	if (mbpt[0].ibra < 0 || mbpt[0].iket < 0) goto OUT;
	CombineMBPT(nf, mbpt, z1, z2, hab, hba, n0, ng0, n, ng, n2, ng2);
	for (q = 0; q < n0; q++) {
	  fprintf(f2, "  %3d %3d %3d %3d %3d %12.5E %12.5E\n", 
		  isym, i, j, 0, ng0[q], z1[q], z2[q]);
	}
	for (r = 0; r < n; r++) {
	  for (q = 0; q < n2[r]; q++) {
	    fprintf(f2, "  %3d %3d %3d %3d %3d %12.5E %12.5E\n", 
		    isym, i, j, ng[r], ng[r]+ng2[r][q], 
		    hab[r][q], hba[r][q]);
	  }
	}
	fflush(f2);
	if (n0 > 0) {
	  for (q = 0; q < n0; q++) {
	    x[q] = ng0[q];
	  }
	  heff[k0] += SumInterp1D(n0, z1, x, t, y);
	  if (i < j) heff[k1] += SumInterp1D(n0, z2, x, t, y);
	}
	if (n > 0) {
	  for (r = 0; r < n; r++) {
	    if (n2[r] > 0) {
	      for (q = 0; q < n2[r]; q++) {
		x[q] = ng[r] + ng2[r][q];
	      }
	      z1[r] = SumInterp1D(n2[r], hab[r], x, t, y);
	      if (i < j) z2[r] = SumInterp1D(n2[r], hba[r], x, t, y);
	      else z2[r] = z1[r];		
	    }
	  }
	  for (q = 0; q < n; q++) {
	    x[q] = ng[q];
	    fprintf(f2, "  %3d %3d %3d %3d %3d %12.5E %12.5E\n",
		    isym, i, j, -1, ng[q], z1[q], z2[q]);
	  }
	  heff[k0] += SumInterp1D(n, z1, x, t, y);
	  if (i < j) heff[k1] += SumInterp1D(n, z2, x, t, y);
	}
      OUT:
	fprintf(f2, "# %3d %3d %3d %3d %3d %12.5E %12.5E %15.8E\n",
		isym, pp, jj, i, j,
		heff[k0]-mbpt[0].a, heff[k1]-mbpt[0].a, mbpt[0].a);
	fflush(f2);
      }
    }
    if (DiagnolizeHamilton() < 0) {
      printf("Diagnolizing Hamiltonian Error\n");
      ierr = -1;
      goto ERROR;
    }
    AddToLevels(nkg0, kg);
    free(heff);
    h->heff = NULL;
  }
  SortLevels(nlevels, -1);
  SaveLevels(fn, nlevels, -1);
  
 ERROR:
  fclose(f2);
  for (m = 0; m < nf; m++) {
    fclose(f1[m]);    
    free(mbpt[m].ng);
    free(mbpt[m].ng2);
    free(mbpt[m].hab1);
    free(mbpt[m].hba1);
    free(mbpt[m].hab);
    free(mbpt[m].hba);
  }
  free(f1);
  free(mbpt);
  free(dw);
  if (n > 0) {
    free(ng);
    for (i = 0; i < n; i++) {
      if (n2[i] > 0) {
	free(ng2[i]);
	free(hab[i]);
	free(hba[i]);
      }
    }
    free(ng2);
    free(hab);
    free(hba);
    free(n2);
  }
  if (n0 > 0) {
    free(ng0);
  }

  return ierr;
}