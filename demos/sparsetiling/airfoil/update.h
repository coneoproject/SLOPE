inline void update(double *qold, double *q, double *res, double *adt)
{
  double del[4];
  double adti;

  adti = 1.0f/(*adt);

  #pragma omp simd
  for (int n = 0; n < 4; n++) {
    del[n] = adti*res[n];
    q[n] = qold[n] - del[n];
    res[n] = 0.0f;
  }
}