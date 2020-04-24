inline void update1(double *qold, double *q, double *res, double *adt, double *rms)
{
  double del[4];
  double adti;

  adti = 1.0f/(*adt);

  #pragma omp simd
  for (int n = 0; n < 4; n++) {
    del[n] = qold[n] - q[n];
  }

  for (int n = 0; n < 4; n++) {
    *rms += del[n]*del[n];
  }
}