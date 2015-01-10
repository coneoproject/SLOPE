/*
 *  coloring.h
 *
 */

#ifndef _COLORING_H_
#define _COLORING_H_

#include "parloop.h"
#include "tile.h"

/*
 * Assign increasing colors to the various tiles.
 *
 * @param iter2tile
 *   map from iteration set to tiles
 * @param tiles
 *   list of tiles, used to assign a color to each tile
 * @return
 *   a map from iteration set to colors
 */
map_t* color_sequential (map_t* iter2tile, tile_list* tiles);

/*
 * Assign colors to tiles such that two adjacent tiles are not assigned the same color.
 *
 * @param loop
 *   the loop to be colored
 * @param iter2tile
 *   map from iteration set to tiles
 * @param tiles
 *   list of tiles, used to assign a color to each tile
 * @return
 *   a map from iteration set to colors, where tiles' colors are enforced to be
 *   different if the tiles are adjacent (e.g. there is an edge connecting them)
 */
map_t* color_shm (loop_t* loop, map_t* iter2tile, tile_list* tiles);

#endif
