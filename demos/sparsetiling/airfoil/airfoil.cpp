/*
 * Open source copyright declaration based on BSD open source template:
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * This file is part of the PILOOP distribution.
 *
 * Copyright (c) 2014, Fabio Luporini and others. Please see the AUTHORS file in
 * the main source directory for a full list of copyright holders.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Mike Giles may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Mike Giles ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Mike Giles BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
//     Nonlinear airfoil lift calculation
//
//     This code was originally written by Mike Giles as a demo for OP2 (2010-2011),
//     a framework for unstructured mesh computations
//

//
// standard headers
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "utils.h"

//
// kernel routines for parallel loops
// some global constants need to be seen by kernel routines
//

double gam, gm1, cfl, eps, mach, alpha, qinf[4];

#include "airfoil_init.h"
#include "save_soln.h"
#include "adt_calc.h"
#include "res_calc.h"
#include "bres_calc.h"
#include "update.h"

// main program

int main(int argc, char **argv)
{
  int    *be2c, *e2c, *bound, *be2n, *e2n, *c2n;
  double  *x, *q, *qold, *adt, *res;
  int    nNodes, nCells, nEdges, nBedges;

  int nIters;
  double  rms;

  airfoil_init(&be2c, &e2c, &bound, &be2n, &e2n, &c2n,
               &x, &q, &qold, &adt, &res,
               &nNodes, &nCells, &nEdges, &nBedges,
               &nIters);

  //initialise timers for total execution wall time
  double start = time_stamp();

  // main time-marching loop

  for(int iter=1; iter<=nIters; iter++) {

    // save old flow solution
    for(int i=0; i<nCells; i++) {
      save_soln(q + i*4,
                qold + i*4);
    }

    // predictor/corrector update loop

    for(int k=0; k<2; k++) {

      // calculate area/timstep
      rms = 0.0;

      for(int i=0; i<nCells; i++) {
        adt_calc(x + c2n[4*i + 0]*2,
                 x + c2n[4*i + 1]*2,
                 x + c2n[4*i + 2]*2,
                 x + c2n[4*i + 3]*2,
                 q + i*4,
                 adt + i);
      }

      // calculate flux residual
      for(int i=0; i<nEdges; i++) {
        res_calc(x + e2n[2*i + 0]*2,
                 x + e2n[2*i + 1]*2,
                 q + e2c[2*i + 0]*4,
                 q + e2c[2*i + 1]*4,
                 adt + e2c[2*i + 0],
                 adt + e2c[2*i + 1],
                 res + e2c[2*i + 0]*4,
                 res + e2c[2*i + 1]*4);
      }

      for(int i=0; i<nBedges; i++) {
        bres_calc(x + be2n[2*i + 0]*2,
                  x + be2n[2*i + 1]*2,
                  q + be2c[i + 0]*4,
                  adt + be2c[i + 0],
                  res + be2c[i + 0]*4,
                  bound + i);
      }

      // update flow field
      for(int i=0; i<nCells; i++) {
        update(qold + i*4,
               q + i*4,
               res + i*4,
               adt + i,
               &rms);
      }
    }

    // print iteration history
    rms = sqrt(rms/(double) nCells);
    if (iter%10 == 0)
      printf(" %d  %10.5e \n",iter,rms);
  }

  double end = time_stamp();
  printf("Max total runtime = %f\n", end - start);

#ifdef AIRFOIL_DEBUG
  print_output("./output_plain.dat", q, 4, nCells);
#endif

  free(c2n);
  free(e2n);
  free(e2c);
  free(be2n);
  free(be2c);
  free(bound);
  free(x);
  free(q);
  free(qold);
  free(res);
  free(adt);
}
