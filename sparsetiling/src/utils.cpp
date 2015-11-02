/*
 *  utils.cpp
 *
 * Implement utility routines
 */

#include <unordered_map>

#include "utils.h"

void generate_vtk (inspector_t* insp,
                   insp_verbose level,
                   set_t* nodes,
                   double* coordinates,
                   dimension meshDim,
                   int rank)
{
#ifndef SLOPE_VTK
  std::cout << "To generate VTK files compile with -DSLOPE_VTK" << std::endl;
  return;
#endif

  // aliases
  loop_list* loops = insp->loops;
  int nNodes = nodes->size;
  std::string nodesSetName = nodes->name;

  // create directory in which the VTK files will be stored (if not already exists)
  struct stat st = {0};
  if (stat(VTK_DIR, &st) == -1) {
    mkdir(VTK_DIR, 0700);
  }

  loop_list::const_iterator it, end;
  // search for all maps towards /nodes/, which will be used to print the mesh
  std::unordered_map<set_t*, map_t*> mapCache;
  for (it = loops->begin(), end = loops->end(); it != end; it++) {
    set_t* loopSet = (*it)->set;
    desc_list* descriptors = (*it)->descriptors;
    desc_list::const_iterator dIt, dEnd;
    for (dIt = descriptors->begin(), dEnd = descriptors->end(); dIt != dEnd; dIt++) {
      map_t* map = (*dIt)->map;
      if (map != DIRECT && map->outSet->name == nodes->name) {
        mapCache[loopSet] = map;
      }
    }
  }
  int i = 0;
  for (it = loops->begin(), end = loops->end(); it != end; it++, i++) {
    // aliases
    loop_t* loop = *it;
    set_t* loopSet = loop->set;
    int loopSetSize = loopSet->size;
    std::string loopSetName = loopSet->name;
    std::string loopName = loop->name;
    desc_list* descriptors = loop->descriptors;

    if (! loop->coloring) {
      std::cout << "No coloring for loop `" << loopName
                << "`, inspection may be broken." << std::endl;
      continue;
    }

    std::stringstream stream;
    stream << VTK_DIR << "/loop" << i << "_" << loopName << "_rank" << rank << ".vtk";
    std::ofstream vtkfile;
    vtkfile.open (stream.str());
    stream.str("");
    stream.clear();

    // write header of the VTK file
    vtkfile << "# vtk DataFile Version 1.0" << std::endl;
    vtkfile << "Coloring " << loopName << std::endl;
    vtkfile << "ASCII" << std::endl << std::endl;

    // write coordinates of nodes
    vtkfile << "DATASET POLYDATA" << std::endl;
    vtkfile << "POINTS " << nNodes << " float" << std::endl;
    for (int j = 0; j < nNodes; j++) {
      vtkfile << coordinates[j*meshDim] << " " << coordinates[j*meshDim + 1];
      switch (meshDim) {
        case 1:
          vtkfile << " 0.0 0.0" << std::endl;
          break;
        case 2:
          vtkfile << " 0.0" << std::endl;
          break;
        case 3:
          vtkfile << " " << coordinates[j*meshDim + 2] << std::endl;
          break;
        default:
          std::cout << "Invalid mesh dimension (supported: 1, 2, 3)";
          vtkfile.close();
          return;
      }
    }

    vtkfile << std::endl;

    // write the map from iteration set to nodes; if not available, skip this loop
    std::unordered_map<set_t*, map_t*>::const_iterator itmap;
    itmap = (loopSetName == nodesSetName) ? mapCache.begin() : mapCache.find(loopSet);
    if (itmap != mapCache.end()) {
      map_t* map = itmap->second;
      int toNodesSize = map->inSet->size;
      int arity = map->size / toNodesSize;
      std::string shape = (arity == 2) ? "LINES " : "POLYGONS ";
      stream << shape << toNodesSize << " " << toNodesSize*(arity + 1) << std::endl;
      vtkfile << stream.str();
      for (int k = 0; k < toNodesSize; k++) {
        vtkfile << arity;
        for (int l = 0; l < arity; l++) {
          vtkfile << " " << map->values[k*arity + l];
        }
        vtkfile << std::endl;
      }
      if (level != MINIMAL) {
        std::cout << "Rank " << rank << ": written VTK for  `" << loopName << "`" << std::endl;
      }
    }
    else {
      if (level != MINIMAL) {
        std::cout << "Couldn't find a map to nodes for loop " << loopName << std::endl;
      }
      vtkfile.close();
      continue;
    }

    // write colors of the iteration set elements
    std::string type = (loopSetName == nodesSetName) ? "POINT_DATA" : "CELL_DATA";
    vtkfile << std::endl << type << " " << loopSetSize << std::endl;
    vtkfile << "SCALARS colors int" << std::endl;
    vtkfile << "LOOKUP_TABLE default" << std::endl;
    for (int k = 0; k < loopSetSize; k++) {
      vtkfile << loop->coloring[k] << std::endl;
    }

    vtkfile.close();
  }

}
