inline void airfoil_init (
  int** be2c, int** e2c, int** bound, int** be2v, int** e2v, int** c2v,
  double** x, double** q, double** qold, double** adt, double** res,
  int* nNodes, int* nCells, int* nEdges, int* nBedges, int* nIters
)
{
  // read in grid
  printf("reading in grid \n");

  FILE *fp;
  if ( (fp = fopen("./new_grid.dat","r")) == NULL) {
    printf("can't open file new_grid.dat\n"); exit(-1);
  }

  if (fscanf(fp,"%d %d %d %d \n", nNodes, nCells, nEdges, nBedges) != 4) {
    printf("error reading from new_grid.dat\n"); exit(-1);
  }

  *c2v   = (int *) malloc(4*(*nCells)*sizeof(int));
  *e2v   = (int *) malloc(2*(*nEdges)*sizeof(int));
  *e2c  = (int *) malloc(2*(*nEdges)*sizeof(int));
  *be2v  = (int *) malloc(2*(*nBedges)*sizeof(int));
  *be2c = (int *) malloc(1*(*nBedges)*sizeof(int));
  *bound  = (int *) malloc(1*(*nBedges)*sizeof(int));

  *x      = (double *) malloc(2*(*nNodes)*sizeof(double));
  *q      = (double *) malloc(4*(*nCells)*sizeof(double));
  *qold   = (double *) malloc(4*(*nCells)*sizeof(double));
  *res    = (double *) malloc(4*(*nCells)*sizeof(double));
  *adt    = (double *) malloc(1*(*nCells)*sizeof(double));

  for (int n=0; n<(*nNodes); n++) {
    if (fscanf(fp,"%lf %lf \n", &(*x)[2*n], &(*x)[2*n+1]) != 2) {
      printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  for (int n=0; n<(*nCells); n++) {
    if (fscanf(fp,"%d %d %d %d \n", &(*c2v)[4*n  ], &(*c2v)[4*n+1],
                                    &(*c2v)[4*n+2], &(*c2v)[4*n+3]) != 4) {
      printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  for (int n=0; n<(*nEdges); n++) {
    if (fscanf(fp,"%d %d %d %d \n", &(*e2v)[2*n], &(*e2v)[2*n+1],
                                    &(*e2c)[2*n], &(*e2c)[2*n+1]) != 4) {
      printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  for (int n=0; n<(*nBedges); n++) {
    if (fscanf(fp,"%d %d %d %d \n", &(*be2v)[2*n], &(*be2v)[2*n+1],
                                    &(*be2c)[n], &(*bound)[n]) != 4) {
      printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  fclose(fp);

  // set constants and initialise flow field and residual

  printf("initialising flow field \n");

  gam = 1.4f;
  gm1 = gam - 1.0f;
  cfl = 0.9f;
  eps = 0.05f;

  double mach  = 0.4f;
  double alpha = 3.0f*atan(1.0f)/45.0f;
  double p     = 1.0f;
  double r     = 1.0f;
  double u     = sqrt(gam*p/r)*mach;
  double e     = p/(r*gm1) + 0.5f*u*u;

  qinf[0] = r;
  qinf[1] = r*u;
  qinf[2] = 0.0f;
  qinf[3] = r*e;

  for (int n=0; n<(*nCells); n++)
  {
    for (int m=0; m<4; m++)
    {
      (*q)[4*n+m] = qinf[m];
      (*res)[4*n+m] = 0.0f;
    }
  }

  *nIters = 1000;
}

inline void print_output(char* filename, double* q, int dim, int nCells)
{
  FILE *fp;
  if ( (fp = fopen(filename,"w")) == NULL)
  {
    printf("can't open file %s\n", filename);
    exit(-1);
  }
  fprintf(fp, "%d %d\n", nCells, dim);
  for (int i = 0; i < nCells; i++)
  {
    for (int j = 0; j < dim-1; j++)
    {
      fprintf(fp, "%e ", q[i*dim + j]);
    }
    fprintf(fp, "%e\n", q[i*dim + (dim-1)]);
  }
  fclose(fp);
}
