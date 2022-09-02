/*
 *  executor.cpp
 *
 */

#include "executor.h"
#include "utils.h"

executor_t* exec_init (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  int nTiles = tiles->size();
  set_t* tileSet = set_cpy (insp->iter2tile->outSet);
  set_t* colorSet = set_cpy (insp->iter2color->outSet);

  executor_t* exec = new executor_t;

  // compute a map from colors to tiles IDs
  int* tile2colorIndMap = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    tile2colorIndMap[i] = tiles->at(i)->color;
  }
#ifdef OP2
  map_t* tile2color = map ("t2c", tileSet, colorSet, tile2colorIndMap, nTiles, -1);
#else
  map_t* tile2color = map ("t2c", tileSet, colorSet, tile2colorIndMap, nTiles);
#endif
  exec->tiles = tiles;
  exec->color2tile = map_invert (tile2color, NULL);

  map_free (tile2color, true);

  return exec;
}

int exec_num_colors (executor_t* exec)
{
  ASSERT(exec != NULL, "Invalid NULL pointer to executor");

  return exec->color2tile->inSet->size;
}

int exec_tiles_per_color (executor_t* exec, int color)
{
  ASSERT ((color >= 0) && (color < exec_num_colors(exec)), "Invalid color provided");

  int* offsets = exec->color2tile->offsets;
  return offsets[color + 1] - offsets[color];
}

tile_t* exec_tile_at (executor_t* exec, int color, int ithTile, tile_region region)
{
  int tileID = exec->color2tile->values[exec->color2tile->offsets[color] + ithTile];
  tile_t* tile = exec->tiles->at (tileID);
  return (tile->region == region) ? tile : NULL;
}

void exec_free (executor_t* exec)
{
  tile_list* tiles = exec->tiles;
  tile_list::const_iterator it, end;
  for (it = tiles->begin(), end = tiles->end(); it != end; it++) {
    tile_free (*it);
  }
  delete tiles;
  map_free (exec->color2tile, true);
  delete exec;
}


void create_mapped_iterations(inspector_t* insp, executor_t* exec){

  tile_list* tiles = exec->tiles;
  tile_list::const_iterator tit, tend;
  map_list* meshMaps = insp->meshMaps;

  loop_list* loops = insp->loops;

  for (tit = tiles->begin(), tend = tiles->end(); tit != tend; tit++) {
    tile_t* tile = *tit;

    for (int c = 0; c < tile->crossedLoops; c++) {
      mapname_iterations* localMap = tile->localMaps[c];
      mapname_iterations::iterator it, end;
      map_t* loopMap = NULL;
      for (it = localMap->begin(), end = localMap->end(); it != end; it++) {
        map_list::const_iterator mit, mend;
        for (mit = meshMaps->begin(), mend = meshMaps->end(); mit != mend; mit++) {
          if (it->first.compare((*mit)->name) == 0) {
            loopMap = *mit;
            break;
          }
        }

        //map found
        if(loopMap){
          iterations_list* itList = it->second;
          if(itList){
            if(itList->size() == 0){
              continue;
            }
            // core | size | imp exec 0 | imp nonexec 0 | imp exec 1 | imp nonexec 1 |
            // core | size | imp exec 0 | imp exec 1 | imp nonexec 1 |
            iterations_list* mappedVals = new iterations_list (itList->size());
            for(int i = 0; i < itList->size(); i++){
              set_t* outSet = loopMap->outSet;
              int haloLevel = outSet->curHaloLevel;
              // check for non exec values
              int nonExecOffset = 0;
              for(int j = 0; j < haloLevel - 1; j++){
                nonExecOffset += outSet->nonExecSizes[j];
              }
              int execOffset = outSet->execSizes[haloLevel - 1];
              int totalOffset = outSet->setSize + execOffset;

              int mapVal = itList->at(i);

              if(mapVal > totalOffset - 1){
                mappedVals->at(i) = mapVal + nonExecOffset;
              }else{
                // check for exec values
                int execAvail = 0;
                for(int j = haloLevel - 1; j > 0; j--){
                  nonExecOffset = 0;
                  for(int k = 0; k < j; k ++){
                    nonExecOffset += outSet->nonExecSizes[k];
                  }
                  execOffset = outSet->execSizes[j - 1];
                  totalOffset = outSet->setSize + execOffset;
                  if(mapVal > totalOffset - 1){
                    mappedVals->at(i) = mapVal + nonExecOffset;
                    execAvail = 1;
                    break;
                  }
                }
                if(execAvail == 0){
                  mappedVals->at(i) = mapVal;
                }
              }
            }
            localMap->find(loopMap->name)->second->clear();
            delete localMap->find(loopMap->name)->second;
            localMap->find(loopMap->name)->second = mappedVals;
          }
        }
      }

      // iterations
      loop_t* loop = loops->at(c);
      set_t* loopSet = loop->set;
      iterations_list* unmappedDirectVals = tile->iterations[c];
      int nhalos = loop->nhalos;
      int maxElement = loopSet->setSize + loopSet->execSizes[nhalos - 1];

      if(unmappedDirectVals->size() > 0){
        iterations_list* mappedDirectVals = new iterations_list();
        mappedDirectVals->clear();
        //up to set size
        for(int i = 0; i < unmappedDirectVals->size(); i++){
          int haloLevel = loopSet->curHaloLevel;
          // check for non exec values
          int nonExecOffset = 0;
          for(int j = 0; j < haloLevel - 1; j++){
            nonExecOffset += loopSet->nonExecSizes[j];
          }

          int execOffset = loopSet->execSizes[haloLevel - 1];
          int totalOffset = loopSet->setSize + execOffset;
          int mapVal = unmappedDirectVals->at(i);

          if(mapVal > maxElement - 1){
            continue;
          }

          if(mapVal > totalOffset - 1){
            mappedDirectVals->push_back(mapVal + nonExecOffset);
          }else{
            // check for exec values
            int execAvail = 0;
            for(int j = haloLevel - 1; j > 0; j--){
              nonExecOffset = 0;
              for(int k = 0; k < j; k ++){
                nonExecOffset += loopSet->nonExecSizes[k];
              }
              execOffset = loopSet->execSizes[j - 1];
              totalOffset = loopSet->setSize + execOffset;
              if(mapVal > totalOffset - 1){
                mappedDirectVals->push_back(mapVal + nonExecOffset);
                execAvail = 1;
                break;
              }
            }
            if(execAvail == 0){
              mappedDirectVals->push_back(mapVal);
            }
          }
        }
        delete tile->iterations[c];
        tile->iterations[c] = NULL;
        tile->iterations[c] = mappedDirectVals;
      }
    } 
  }
}