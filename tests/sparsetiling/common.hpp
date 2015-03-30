/*
 * common.h
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <iostream>

enum mesh_t {TRI = 3, RECT, RECT_MPI};

class ExampleMesh
{
public:
  // Topology
  int vertices;
  int edges;
  int cells;
  int *e2v;
  int e2vSize;
  int *c2v;
  int c2vSize;

  // Data
  double *coords;

  ExampleMesh(int vertices, int edges, int cells, int* e2v, int* c2v,
              double* coords, mesh_t type)
  {
    this->vertices = vertices;
    this->edges = edges;
    this->cells = cells;
    this->e2v = e2v;
    this->c2v = c2v;
    this->coords = coords;
    this->e2vSize = edges*2;
    this->c2vSize = cells*type;
  }
};

// Meshes expressed through edges2vertices maps

/*
 * Mesh 1
 *
 *  0 --  1 --  2 --  3 --  4
 *  |     |     |     |     |
 *  5 --  6 --  7 --  8 --  9
 *  |     |     |     |     |
 * 10 -- 11 -- 12 -- 13 -- 14
 *
 */
static int mesh1_e2v[] = {0,1 , 1,2 , 2,3 , 3,4 , 0,5 , 5,10, 1,6 , 6,11 , 2,7,
                          7,12 , 3,8, 8,13 , 4,9 , 9,14 , 5,6 , 6,7 , 7,8 , 8,9,
                          10, 11 , 11,12 , 12,13, 13,14};
static int mesh1_c2v[] = {0,1,6,5 , 1,2,7,6 , 2,3,8,7 , 3,4,9,8 , 5,6,11,10,
                          6,7,12,11 , 7,8,13,12 , 8,9,14,13};
static double mesh1_coords[] = {0,0 , 0,1 , 0,2 , 0,3 , 0,4 , 1,0 , 1,1 , 1,2,
                                1,3 , 1,4 , 2,0, 2,1 , 2,2 , 2,3 , 2,4};
/*
 * Mesh 2
 *
 *  0 -- 1
 *  |  \ | \
 *  2 -- 3 - 4
 *  |  \ | /
 *  5 -- 6
 *
 */
static int mesh2_e2v[] = {0,1 , 0,2 , 1,3 , 0,3 , 1,4 , 2,3 , 3,4 , 2,5 , 3,6,
                          2,6 , 6,4 , 5,6};
static int mesh2_c2v[] = {0,2,3 , 0,1,3 , 1,3,4 , 2,5,6 , 2,3,6 , 3,4,6};

// Meshes for MPI execution

/*
 * Mesh MPI 1 - For 2 MPI processes
 *
 * Global mesh:
 *
 *  0 --  1 --  2 --  3 --  4 --  5 --  6
 *  |  0  |  1  |  2  |  3  |  4  |  5  |
 *  6 --  7 --  8 --  9 -- 10 -- 11 -- 12
 *  |  3  |  4  |  5  | 11  | 12  | 13  |
 * 13 -- 14 -- 15 -- 16 -- 17 -- 18 -- 19
 *  | 14  |  15  | 16 | 17  | 18  | 19  |
 * 20 -- 21 -- 22 -- 23 -- 24 -- 25 -- 26
 *
 * RANK 0:
 *
 *  0 --  1 --  2 --  3 -- 16 -- 17
 *  |  0  |  1  |  2  |  9  | 10  |
 *  4 --  5 --  6 --  7 -- 18 -- 19
 *  |  3  |  4  |  5  | 11  | 12  |
 *  8 --  9 -- 10 -- 11 -- 20 -- 21
 *  |  6  |  7  |  8  | 13  | 14  |
 * 12 -- 13 -- 14 -- 15 -- 22 -- 23
 *
 * RANK 1:
 *
 * 16 -- 17 --  0 --  1 --  2 --  3
 *  |  9  | 10  |  0  |  1  |  2  |
 * 18 -- 19 --  4 --  5 --  6 --  7
 *  | 11  | 12  |  3  |  4  |  5  |
 * 20 -- 21 --  8 --  9 -- 10 -- 11
 *  | 13  | 14  |  6  |  7  |  8  |
 * 22 -- 23 -- 12 -- 13 -- 14 -- 15
 *
 * Where vertices {0, 1, 2, 3, 6, 7, 8, 9, 13, 14, 15, 16, 20, 21, 21 , 23} are
 * owned by Rank 0, while {4, 5, 6, 10, 11, 12, 17, 18, 19, 24, 25, 26} by Rank 1.
 *
 * The halo regions are such that:
 * Cells:
 *   Rank 0:
 *     Local --- Global  --- Rank 1
 *       9         3           0
 *      10         4           1
 *      11        11           3
 *      12        12           4
 *      13        17           6
 *      14        18           7
 *   Rank 1:
 *     Local --- Global  --- Rank 1
 *       9         1           1
 *      10         2           2
 *      11         4           4
 *      12         5           5
 *      13        15           7
 *      14        16           8
 */

static int mesh_mpi1_c2v[][60] = {{0,1,4,5 , 1,2,5,6 , 2,3,6,7, 3,16,7,18 , 16,17,18,19,
                                   4,5,8,9 , 5,6,9,10 , 6,7,10,11 , 7,18,11,20, 18,19,20,21,
                                   8,9,12,13 , 9,10,13,14 , 10,11,14,15 , 11,20,15,22 , 20,21,22,3},
                                  {16,17,18,19 , 17,0,19,4 , 0,1,4,5 , 1,2,5,6 , 2,3,6,7,
                                   18,19,20,21 , 19,4,21,8 , 4,5,8,9 , 5,6,9,10 , 6,7,10,11,
                                   20,21,22,23 , 21,8,23,12 , 8,9,12,13 , 9,10,13,14 , 10,11,14,15}};
static double mesh_mpi1_coords[][60] = {{0,0 , 0,1 , 0,2 , 0,3 , 0,4 , 0,5,
                                         1,0 , 1,1 , 1,2 , 1,3 , 1,4 , 1,5,
                                         2,0 , 2,1 , 2,2 , 2,3 , 2,4 , 2,5,
                                         3,0 , 3,1 , 3,2 , 3,3 , 3,4 , 3,5},
                                        {0,1 , 0,2 , 0,3 , 0,4 , 0,5 , 0,6,
                                         1,1 , 1,2 , 1,3 , 1,4 , 1,5 , 1,6,
                                         2,1 , 2,2 , 2,3 , 2,4 , 2,5 , 2,6,
                                         3,1 , 3,2 , 3,3 , 3,4 , 3,5 , 3,6}};

inline ExampleMesh example_mesh(mesh_t type, int rank=0)
{
  switch(type)
  {
    case RECT:
    {
      return ExampleMesh(15, 22, 8, mesh1_e2v, mesh1_c2v, mesh1_coords, RECT);
    }
    case RECT_MPI:
    {
      return ExampleMesh(24, -1, 15, NULL, mesh_mpi1_c2v[rank], mesh_mpi1_coords[rank], RECT);
    }
    case TRI:
    {
      return ExampleMesh(7, 12, 6, mesh2_e2v, mesh2_c2v, NULL, TRI);
    }
  }
}

#endif
