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
//     Sparse-tiled version of the nonlinear airfoil lift calculation in
//     airfoil.cpp, using tiles' local maps instead of global maps
//
//     Written by Fabio Luporini, 2014
//

//
// standard headers
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//
// sparse tiling headers
//

#include "executor.h"
#include "inspector.h"

#define TILE_SIZE 5000

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
  int *be2c, *e2c, *bound, *be2n, *e2n, *c2n;
  double *x, *q, *qold, *adt, *res;
  int nNodes, nCells, nEdges, nBedges;

  int nIters;
  double rms;

  airfoil_init(&be2c, &e2c, &bound, &be2n, &e2n, &c2n,
               &x, &q, &qold, &adt, &res,
               &nNodes, &nCells, &nEdges, &nBedges,
               &nIters);

  //
  // inspector
  //

  const int avgTileSize = (argc == 1) ? TILE_SIZE : atoi(argv[1]);
  const int nLoops = 6;
  const int seedTilePoint = nLoops / 2;

  printf("running inspector\n");

  // sets
  // note: the 0s indicate that MPI execution is not supported
  set_t* nodes = set("nodes", nNodes);
  set_t* edges = set("edges", nEdges);
  set_t* bedges = set("bedges", nBedges, 0, 0, edges);
  set_t* cells = set("cells", nCells);

  // maps
  map_t* c2nMap = map("c2n", cells, nodes, c2n, nCells*4);
  map_t* e2nMap = map("e2n", edges, nodes, e2n, nEdges*2);
  map_t* e2cMap = map("e2c", edges, cells, e2c, nEdges*2);
  map_t* be2nMap = map("be2n", bedges, nodes, be2n, nBedges*2);
  map_t* be2cMap = map("be2c", bedges, cells, be2c, nBedges*1);

  // descriptors
  desc_list adtCalcDesc ({desc(c2nMap, READ),
                          desc(DIRECT, WRITE)});
  desc_list resCalcDesc ({desc(e2nMap, READ),
                          desc(e2cMap, READ),
                          desc(e2cMap, INC)});
  desc_list bresCalcDesc ({desc(be2nMap, READ),
                           desc(be2cMap, READ),
                           desc(be2cMap, INC)});
  desc_list updateDesc ({desc(DIRECT, READ),
                         desc(DIRECT, WRITE)});

  // inspector
  inspector_t* insp = insp_init(avgTileSize, OMP);

  insp_add_parloop (insp, "adtCalc1", cells, &adtCalcDesc);
  insp_add_parloop (insp, "resCalc1", edges, &resCalcDesc);
  insp_add_parloop (insp, "bresCalc", bedges, &bresCalcDesc);
  insp_add_parloop (insp, "update", cells, &updateDesc);
  insp_add_parloop (insp, "adtCalc2", cells, &adtCalcDesc);
  insp_add_parloop (insp, "resCalc2", edges, &resCalcDesc);

  insp_run (insp, seedTilePoint);

  insp_print (insp, VERY_LOW);

  //
  // plot tiled mesh, for each parallel loop in the loop chain
  //

  generate_vtk (insp, VERY_LOW, nodes, x, DIM2);

  //
  // executor
  //

  printf("running executor\n");
  executor_t* exec = exec_init (insp);
  int nColors = exec_num_colors (exec);;

  // initialise timers for total execution wall time
  double start = time_stamp();

  // main time-marching loop

  for(int iter=1; iter<=nIters; iter++) {

    // save old flow solution
    for(int i=0; i<nCells; i++) {
      save_soln(q + 4*i,
                qold + 4*i);
    }

    // predictor/corrector update loop

    // notice k<1 instead of <2 due to unrolling
    for(int k = 0; k < 1; k++) {

      rms = 0.0;

      //for each colour
      for (int i = 0; i < nColors; i++) {
        // for all tiles of this color
        const int nTilesPerColor = exec_tiles_per_color (exec, i);

#ifdef SLOPE_OMP
        #pragma omp parallel for
#endif
        for (int j = 0; j < nTilesPerColor; j++) {
          // execute the tile
          tile_t* tile = exec_tile_at (exec, i, j);
          int tileLoopSize;

          // loop adt_calc (calculate area/timstep)
          iterations_list& lc2n_0 = tile_get_local_map (tile, 0, "c2n");
          iterations_list& iterations_0 = tile_get_iterations (tile, 0);
          tileLoopSize = iterations_0.size();
          #pragma ivdep
          for (int k = 0; k < tileLoopSize; k++) {
            adt_calc (x + lc2n_0[k*4 + 0]*2,
                      x + lc2n_0[k*4 + 1]*2,
                      x + lc2n_0[k*4 + 2]*2,
                      x + lc2n_0[k*4 + 3]*2,
                      q + iterations_0[k]*4,
                      adt + iterations_0[k]);
          }

          // loop res_calc
          iterations_list& le2n_1 = tile_get_local_map (tile, 1, "e2n");
          iterations_list& le2c_1 = tile_get_local_map (tile, 1, "e2c");
          iterations_list& iterations_1 = tile_get_iterations (tile, 1);
          tileLoopSize = iterations_1.size();
          for (int k = 0; k < tileLoopSize; k++) {
            res_calc (x + le2n_1[k*2 + 0]*2,
                      x + le2n_1[k*2 + 1]*2,
                      q + le2c_1[k*2 + 0]*4,
                      q + le2c_1[k*2 + 1]*4,
                      adt + le2c_1[k*2 + 0]*1,
                      adt + le2c_1[k*2 + 1]*1,
                      res + le2c_1[k*2 + 0]*4,
                      res + le2c_1[k*2 + 1]*4);
          }

          // loop bres_calc
          iterations_list& lbe2n_2 = tile_get_local_map (tile, 2, "be2n");
          iterations_list& lbe2c_2 = tile_get_local_map (tile, 2, "be2c");
          iterations_list& iterations_2 = tile_get_iterations (tile, 2);
          tileLoopSize = iterations_2.size();
          for (int k = 0; k < tileLoopSize; k++) {
            bres_calc (x + lbe2n_2[k*2 + 0]*2,
                       x + lbe2n_2[k*2 + 1]*2,
                       q + lbe2c_2[k + 0]*4,
                       adt + lbe2c_2[k + 0]*1,
                       res + lbe2c_2[k + 0]*4,
                       bound + iterations_2[k]);
          }

          // loop update
          iterations_list& iterations_3 = tile_get_iterations (tile, 3);
          tileLoopSize = iterations_3.size();
          for (int k = 0; k < tileLoopSize; k++) {
            update    (qold + iterations_3[k]*4,
                       q + iterations_3[k]*4,
                       res + iterations_3[k]*4,
                       adt + iterations_3[k],
                       &rms);
          }

          // loop adt_calc (k = 2)
          iterations_list& lc2n_4 = tile_get_local_map (tile, 4, "c2n");
          iterations_list& iterations_4 = tile_get_iterations (tile, 4);
          tileLoopSize = iterations_4.size();
          #pragma ivdep
          for (int k = 0; k < tileLoopSize; k++) {
            adt_calc (x + lc2n_4[k*4 + 0]*2,
                      x + lc2n_4[k*4 + 1]*2,
                      x + lc2n_4[k*4 + 2]*2,
                      x + lc2n_4[k*4 + 3]*2,
                      q + iterations_4[k]*4,
                      adt + iterations_4[k]);
          }

          // loop res_calc (k = 2)
          iterations_list& le2n_5 = tile_get_local_map (tile, 5, "e2n");
          iterations_list& le2c_5 = tile_get_local_map (tile, 5, "e2c");
          iterations_list& iterations_5 = tile_get_iterations (tile, 5);
          tileLoopSize = iterations_5.size();
          for (int k = 0; k < tileLoopSize; k++) {
            res_calc (x + le2n_5[k*2 + 0]*2,
                      x + le2n_5[k*2 + 1]*2,
                      q + le2c_5[k*2 + 0]*4,
                      q + le2c_5[k*2 + 1]*4,
                      adt + le2c_5[k*2 + 0]*1,
                      adt + le2c_5[k*2 + 1]*1,
                      res + le2c_5[k*2 + 0]*4,
                      res + le2c_5[k*2 + 1]*4);
          }

        }
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

      rms = 0.0;

      for(int i=0; i<nCells; i++) {
        update(qold + 4*i,
               q + 4*i,
               res + 4*i,
               adt + i,
               &rms);
      }

    }

    // print iteration history
    rms = sqrt(rms/(double) nCells);
#ifdef SLOPE_OMP
    if (iter%100 == 0)
#else
    if (iter%10 == 0)
#endif
      printf(" %d  %10.5e \n",iter,rms);
  }

  double end = time_stamp();
  printf("Max total runtime = %f\n", end - start);

#ifdef AIRFOIL_DEBUG
  print_output("./output_tiled.dat", q, 4, nCells);
#endif

  insp_free (insp);
  printf ("inspector destroyed\n");
  exec_free (exec);
  printf ("executor destroyed\n");

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
