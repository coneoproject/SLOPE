/*
 * common.h
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <iostream>

enum mesh_t {TRI = 3, RECT};

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
                          10,11 , 11,12 , 12,13, 13,14};
static int mesh1_c2v[] = {0,1,6,5 , 1,2,7,6 , 2,3,8,7 , 3,4,9,8 , 5,6,11,10,
                          6,7,12,11 , 7,8,13,12 , 8,9,14,13};
static double mesh1_coords[] = {0,0 , 0,1 , 0,2 , 0,3 , 0,4 , 1,0 , 1,1 , 1,2,
                                1,3, 1,4 , 2,0, 2,1 , 2,2 , 2,3 , 2,4};
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


inline ExampleMesh example_mesh(mesh_t type)
{
  switch(type)
  {
    case RECT:
    {
      return ExampleMesh(15, 22, 8, mesh1_e2v, mesh1_c2v, mesh1_coords, RECT);
      break;
    }
    case TRI:
    {
      return ExampleMesh(7, 12, 6, mesh2_e2v, mesh2_c2v, NULL, TRI);
      break;
    }
  }
}

#endif
