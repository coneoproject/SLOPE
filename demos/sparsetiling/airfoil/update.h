inline void update(double *qold, double *q, double *res, double *adt, double *rms)
{
  double del[4];
  double adti;

  adti = 1.0f/(*adt);

  #pragma simd
  for (int n = 0; n < 4; n++) {
    del[n] = adti*res[n];
    q[n] = qold[n] - del[n];
    res[n] = 0.0f;
  }

  for (int n = 0; n < 4; n++) {
    *rms += del[n]*del[n];
  }
}
