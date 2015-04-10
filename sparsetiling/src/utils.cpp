/*
 *  utils.cpp
 *
 * Implement utility routines
 */

#include "utils.h"

void generate_vtk (inspector_t* insp,
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

  // create directory in which VTK files will be stored, if not exists
  struct stat st = {0};
  if (stat(VTK_DIR, &st) == -1) {
    mkdir(VTK_DIR, 0700);
  }

  loop_list::const_iterator it, end;
  // first, search for a map towards coordinates that can be used for those
  // direct loops over the coordinates' set
  map_t* toCoordinates = NULL;
  for (it = loops->begin(), end = loops->end(); !toCoordinates && (it != end); it++) {
    desc_list* descriptors = (*it)->descriptors;
    desc_list::const_iterator dIt, dEnd;
    for (dIt = descriptors->begin(), dEnd = descriptors->end(); dIt != dEnd; dIt++) {
      map_t* map = (*dIt)->map;
      if (map != DIRECT && map->outSet->name == nodes->name) {
        toCoordinates = map;
        break;
      }
    }
  }
  int i = 0;
  for (it = loops->begin(), end = loops->end(); it != end; it++, i++) {
    // aliases
    loop_t* loop = *it;
    int loopSetSize = loop->set->size;
    std::string loopSetName = loop->set->name;
    std::string loopName = loop->name;
    desc_list* descriptors = loop->descriptors;

    if (! loop->coloring) {
      std::cout << "No coloring for loop `" << loopName
                << "`, check output of inspector." << std::endl;
      continue;
    }
    std::cout << "Rank " << rank
              << ": generating VTK file for loop `" << loopName << "`..." << std::flush;

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

    // write iteration set map to nodes; if not available in the loop's descriptors,
    // just skip the file generation for the loop, unless this is a loop over the
    // toCoordinates' set
    std::string type;
    map_t* map = NULL;
    if (loopSetName == nodesSetName && toCoordinates) {
      map = toCoordinates;
      type = "POINT_DATA";
    }
    else {
      desc_list::const_iterator dIt, dEnd;
      for (dIt = descriptors->begin(), dEnd = descriptors->end(); dIt != dEnd; dIt++) {
        if ((*dIt)->map != DIRECT && (*dIt)->map->outSet->name == nodesSetName) {
          map = (*dIt)->map;
          type = "CELL_DATA";
          break;
        }
      }
    }
    if (map) {
      int toNodesSize = map->inSet->size;
      int ariety = map->size / toNodesSize;
      std::string shape = (ariety == 2) ? "LINES " : "POLYGONS ";
      stream << shape << toNodesSize << " " << toNodesSize*(ariety + 1) << std::endl;
      vtkfile << stream.str();
      for (int k = 0; k < toNodesSize; k++) {
        vtkfile << ariety;
        for (int l = 0; l < ariety; l++) {
          vtkfile << " " << map->values[k*ariety + l];
        }
        vtkfile << std::endl;
      }
    }
    else {
      std::cout << "Couldn't find a map to nodes for loop " << loopName << std::endl;
      vtkfile.close();
      continue;
    }

    // write colors of the iteration set elements
    vtkfile << std::endl << type << " " << loopSetSize << std::endl;
    vtkfile << "SCALARS colors int" << std::endl;
    vtkfile << "LOOKUP_TABLE default" << std::endl;
    for (int k = 0; k < loopSetSize; k++) {
      vtkfile << loop->coloring[k] << std::endl;
    }

    std::cout << "Done!" << std::endl;
    vtkfile.close();
  }

}
