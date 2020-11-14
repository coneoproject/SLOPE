module SLOPE_Fortran_Declarations

    use, intrinsic :: ISO_C_BINDING

    integer(c_int) :: SL_READ = 0
    integer(c_int) :: SL_WRITE = 1
    integer(c_int) :: SL_RW = 2
    integer(c_int) :: SL_INC = 3

    integer(c_int) :: SL_SEQUENTIAL = 0
    integer(c_int) :: SL_OMP = 1
    integer(c_int) :: SL_ONLY_MPI = 2
    integer(c_int) :: SL_OMP_MPI = 3

    integer(c_int) :: SL_COL_DEFAULT = 0
    integer(c_int) :: SL_COL_RAND = 1
    integer(c_int) :: SL_COL_MINCOLS = 2

    integer(c_int) :: SL_MINIMAL = 1
    integer(c_int) :: SL_VERY_LOW = 5
    integer(c_int) :: SL_LOW = 20
    integer(c_int) :: SL_MEDIUM = 40
    integer(c_int) :: SL_HIGH = 50

    integer(c_int) :: SL_DIM1 = 1
    integer(c_int) :: SL_DIM2 = 2
    integer(c_int) :: SL_DIM3 = 3


    type, BIND(C) :: set_t

        type(c_ptr) :: name
        integer(kind=c_int) :: core
        integer(kind=c_int) :: execHalo
        integer(kind=c_int) :: nonExecHalo
        integer(kind=c_int) :: size
        type(c_ptr) :: superset

    end type set_t

    type :: sl_set

        type(set_t), pointer :: setPtr => null()
        type(c_ptr) :: setCptr

    end type sl_set
    
    type, BIND(C) :: map_t

        type(c_ptr) :: name
        type(set_t) :: inSet
        type(set_t) :: outSet
        type(c_ptr) :: values 
        integer(kind=c_int) :: size
        type(c_ptr) :: offsets

    end type map_t

    type :: sl_map

        type(map_t), pointer :: mapPtr => null()
        type(c_ptr) :: mapCptr

    end type sl_map

    type(sl_map) :: DIRECT = sl_map(mapCptr=C_NULL_PTR)

    type, BIND(C) :: descriptor_t

        type(map_t) :: map_c
        integer(kind=c_int) :: mode

    end type descriptor_t

    type :: sl_descriptor

        type(descriptor_t), pointer :: descPtr => null()
        type(c_ptr) :: descCptr

    end type sl_descriptor

    type :: sl_desc_list
        type(c_ptr) :: descListCPtr
    end type sl_desc_list

    type :: sl_map_list
        type(c_ptr) :: mapListCPtr
    end type sl_map_list

    type, BIND(C) :: inspector_t

    type(c_ptr) :: name
    integer(kind=c_int) :: strategy
    integer(kind=c_int) :: seed
    type(c_ptr) :: iter2tile
    type(c_ptr) :: iter2color
    integer(kind=c_int) :: avgTileSize
    type(c_ptr) :: loops
    type(c_ptr) :: tiles
    type(c_ptr) :: tileRegions
    integer(kind=c_int) :: nSweeps
    type(c_ptr) :: partitioningMode
    integer(kind=c_int) :: coloring
    type(c_ptr) :: meshMaps
    type(c_ptr) :: partitionings
    integer(kind=c_int) :: prefetchHalo
    integer(kind=c_int) ::ignoreWAR
    real(kind=c_double) :: totalInspectionTime
    real(kind=c_double) :: partitioningTime
    integer(kind=c_int) :: nThreads

    end type inspector_t

    type :: sl_inspector

    type(inspector_t), pointer :: inspPtr => null()
    type(c_ptr) :: inspCptr

    end type sl_inspector

    type, BIND(C) :: executor_t

    type(c_ptr) :: tiles
    type(c_ptr) :: color2tile

    end type executor_t

    type :: sl_executor

    type(executor_t), pointer :: execPtr => null()
    type(c_ptr) :: execCptr

    end type sl_executor

    type, BIND(C) :: tile_t

    integer(kind=c_int) :: crossedLoops
    type(c_ptr) :: iterations
    type(c_ptr) :: localMaps
    integer(kind=c_int) :: color
    integer(kind=c_int) :: prefetchHalo
    integer(kind=c_int) :: region

    end type tile_t

    type :: sl_tile

    type(tile_t), pointer :: tilePtr => null()
    type(c_ptr) :: tileCptr

    end type sl_tile

    type :: sl_iterations_list
        type(c_ptr) :: itListCPtr
    end type sl_iterations_list


    interface

        type(c_ptr) function set_c ( name, core, execHalo, nonExecHalo, superset ) BIND(C,name='set_f')

            use, intrinsic :: ISO_C_BINDING

            import :: set_t

            character(kind=c_char,len=1), intent(in)    :: name(*)
            integer(kind=c_int), value, intent(in)      :: core       
            integer(kind=c_int), value, intent(in)      :: execHalo
            integer(kind=c_int), value, intent(in)      :: nonExecHalo
            type(c_ptr), value, intent(in) :: superset

        end function set_c


        type(c_ptr) function map_c ( name, inSet, outSet, values, size) BIND(C,name='map_f')

            use, intrinsic :: ISO_C_BINDING

            import :: map_t

            character(kind=c_char,len=1), intent(in) :: name(*)
            type(c_ptr), value, intent(in) :: inSet
            type(c_ptr), value, intent(in) :: outSet
            type(c_ptr), value, intent(in) :: values
            integer(kind=c_int), value, intent(in) :: size

        end function map_c


        type(c_ptr) function desc_c (map, mode) BIND(C,name='desc_f')

            use, intrinsic :: ISO_C_BINDING

            import :: descriptor_t

            type(c_ptr), value, intent(in) :: map
            integer(kind=c_int), value, intent(in) :: mode

        end function desc_c


        type(c_ptr) function desc_list_c () BIND(C,name='desc_list_f')

            use, intrinsic :: ISO_C_BINDING

        end function desc_list_c


        subroutine insert_desc_c (descList, desc) BIND(C,name='insert_descriptor_to_f')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: descList
            type(c_ptr), value, intent(in) :: desc

        end subroutine insert_desc_c


        type(c_ptr) function map_list_c () BIND(C,name='map_list_f')

            use, intrinsic :: ISO_C_BINDING

        end function map_list_c


        subroutine insert_map_to_c (mapList, map) BIND(C,name='insert_map_to_f')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: mapList
            type(c_ptr), value, intent(in) :: map

        end subroutine insert_map_to_c


        type(c_ptr) function insp_init_c (avgTileSize, strategy, coloring, meshMaps, partitionings, &
        prefetchHalo, ignoreWAR, name) BIND(C,name='insp_init_f')

            use, intrinsic :: ISO_C_BINDING

            integer(kind=c_int), value, intent(in) :: avgTileSize
            integer(kind=c_int), value, intent(in) :: strategy
            integer(kind=c_int), value, intent(in):: coloring
            type(c_ptr), value, intent(in) :: meshMaps
            type(c_ptr), value, intent(in) :: partitionings
            integer(kind=c_int), value, intent(in):: prefetchHalo
            integer(kind=c_int), value, intent(in):: ignoreWAR
            character(kind=c_char,len=1), intent(in) :: name(*)

        end function insp_init_c


        integer(kind=c_int) function insp_add_parloop_c(insp, name, set, descriptors) BIND(C,name='insp_add_parloop_f')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: insp
            character(kind=c_char,len=1), intent(in) :: name(*)
            type(c_ptr), value, intent(in) :: set
            type(c_ptr), value, intent(in) :: descriptors
  
        end function insp_add_parloop_c


        integer(kind=c_int) function insp_run_c(insp, suggestedSeed) BIND(C,name='insp_run')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: insp
            integer(kind=c_int), value, intent(in) :: suggestedSeed
  
        end function insp_run_c


        subroutine insp_print_c(insp, level, loopIndex) BIND(C,name='insp_print')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: insp
            integer(kind=c_int), value, intent(in) :: level
            integer(kind=c_int), value, intent(in) :: loopIndex
  
        end subroutine insp_print_c


        type(c_ptr) function exec_init_c (insp) BIND(C,name='exec_init')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: insp

        end function exec_init_c


        integer(kind=c_int) function exec_num_colors_c (exec) BIND(C,name='exec_num_colors')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: exec

        end function exec_num_colors_c


        integer(kind=c_int) function exec_tiles_per_color_c (exec, color) BIND(C,name='exec_tiles_per_color')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: exec
            integer(kind=c_int), value, intent(in) :: color

        end function exec_tiles_per_color_c


        type(c_ptr) function exec_tile_at_c (exec, color, ithTile, region) BIND(C,name='exec_tile_at')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: exec
            integer(kind=c_int), value, intent(in) :: color
            integer(kind=c_int), value, intent(in) :: ithTile
            integer(kind=c_int), value, intent(in) :: region

        end function exec_tile_at_c


        type(c_ptr) function tile_get_local_map_c (tile, loopIndex, mapName) BIND(C,name='tile_get_local_map_f')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: tile
            integer(kind=c_int), value, intent(in) :: loopIndex
            character(kind=c_char,len=1), intent(in)    :: mapName(*)

        end function tile_get_local_map_c


        type(c_ptr) function tile_get_iterations_c (tile, loopIndex) BIND(C,name='tile_get_iterations_f')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: tile
            integer(kind=c_int), value, intent(in) :: loopIndex

        end function tile_get_iterations_c


        integer(kind=c_int) function tile_loop_size_c (tile, loopIndex) BIND(C,name='tile_loop_size')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: tile
            integer(kind=c_int), value, intent(in) :: loopIndex

        end function tile_loop_size_c


        subroutine generate_vtk_c (insp, level, nodes, coordinates, meshDim, rank) BIND(C,name='generate_vtk')

            use, intrinsic :: ISO_C_BINDING

            type(c_ptr), value, intent(in) :: insp
            integer(kind=c_int), value, intent (in) :: level
            type(c_ptr), target :: nodes
            type(c_ptr), value, intent(in) :: coordinates
            integer(kind=c_int), value, intent (in) :: meshDim
            integer(kind=c_int), value, intent (in) :: rank

        end subroutine generate_vtk_c


    end interface

    contains

        subroutine set (inSet, name, core, execHalo, nonExecHalo, superset)

            type(sl_set) :: inSet
            character(kind=c_char,len=*), intent(in) :: name
            integer(kind=c_int), value, intent(in) :: core       
            integer(kind=c_int), optional :: execHalo
            integer(kind=c_int), optional :: nonExecHalo
            type(sl_set), optional, target :: superset

            if(.not.(present(execHalo))) then
                inSet%setCPtr = set_c(name, core, 0, 0, C_NULL_PTR)
            else if(.not.(present(nonExecHalo))) then
                inSet%setCPtr = set_c(name, core, execHalo, 0, C_NULL_PTR)
            else if(.not.(present(superset))) then
                inSet%setCPtr = set_c(name, core, execHalo, nonExecHalo, C_NULL_PTR)
            else
                inSet%setCPtr = set_c(name, core, execHalo, nonExecHalo, c_loc(superset))
            end if

            call c_f_pointer(inSet%setCPtr, inSet%setPtr)

        end subroutine set


        subroutine map (inMap, name, inSet, outSet, values, size)

            type(sl_map) :: inMap
            character(kind=c_char,len=*), intent(in) :: name
            type(sl_set), target :: inSet
            type(sl_set), target :: outSet
            integer(4), dimension(*), intent(in), target :: values
            integer(kind=c_int), value, intent(in) :: size

            inMap%mapCPtr = map_c(name, inSet%setCPtr, outSet%setCPtr, c_loc(values), size)
            call c_f_pointer(inMap%mapCPtr, inMap%mapPtr)
        
        end subroutine map


        subroutine desc (inDesc, map, mode)

            type(sl_descriptor) :: inDesc
            type(sl_map), target :: map
            integer(kind=c_int), value, intent(in) :: mode

            inDesc%descCPtr = desc_c(map%mapCPtr, mode)
            call c_f_pointer(inDesc%descCPtr, inDesc%descPtr)
        
        end subroutine desc


        subroutine desc_list (inDescList)

            type(sl_desc_list) :: inDescList

            inDescList%descListCPtr = desc_list_c()
        
        end subroutine desc_list


        subroutine insert_descriptor_to (descList, desc)

            type(sl_desc_list) :: descList
            type(sl_descriptor), target :: desc

            call insert_desc_c(descList%descListCPtr, desc%descCPtr)

        end subroutine insert_descriptor_to


        subroutine map_list (inMapList)

            type(sl_map_list) :: inMapList

            inMapList%mapListCPtr = map_list_c()
        
        end subroutine map_list


        subroutine insert_map_to (mapList, map)

            type(sl_map_list) :: mapList
            type(sl_map), target :: map

            call insert_map_to_c(mapList%mapListCPtr, map%mapCPtr)

        end subroutine insert_map_to


        subroutine insp_init (insp, avgTileSize, strategy, coloring, meshMaps, partitionings, prefetchHalo, ignoreWAR, name)

            type(sl_inspector) :: insp
            integer(kind=c_int), value, intent(in) :: avgTileSize
            integer(kind=c_int), value, intent(in) :: strategy
            integer(kind=c_int), value, intent(in), optional :: coloring
            type(sl_map_list), target, optional :: meshMaps
            type(sl_map_list), target, optional :: partitionings
            integer(kind=c_int), value, intent(in), optional :: prefetchHalo
            integer(kind=c_int), value, intent(in), optional:: ignoreWAR
            ! logical(kind=c_bool), value, intent(in), optional :: ignoreWAR
            character(kind=c_char,len=*), intent(in), optional :: name

            ! TODO: ignoreWAR is bool. Change it too bool from int
            if(.not.(present(coloring))) then
                insp%inspCPtr = insp_init_c(avgTileSize, strategy, 0, C_NULL_PTR, C_NULL_PTR, 1, 0, C_NULL_CHAR)
            else if (.not.(present(partitionings))) then
                insp%inspCPtr = insp_init_c(avgTileSize, strategy, coloring, C_NULL_PTR, C_NULL_PTR, 1, 0, C_NULL_CHAR)
            else if(.not.(present(prefetchHalo))) then
                insp%inspCPtr = insp_init_c(avgTileSize, strategy, coloring, meshMaps%mapListCPtr, partitionings%mapListCPtr, 1, 0, C_NULL_CHAR)
            else if(.not.(present(ignoreWAR))) then
                insp%inspCPtr = insp_init_c(avgTileSize, strategy, coloring, meshMaps%mapListCPtr, partitionings%mapListCPtr, prefetchHalo, 0, C_NULL_CHAR)
            else if(.not.(present(name))) then
                insp%inspCPtr = insp_init_c(avgTileSize, strategy, coloring, meshMaps%mapListCPtr, partitionings%mapListCPtr, prefetchHalo, ignoreWAR, C_NULL_CHAR)
            else
                insp%inspCPtr = insp_init_c(avgTileSize, strategy, coloring, meshMaps%mapListCPtr, partitionings%mapListCPtr, prefetchHalo, ignoreWAR, name)
            end if
           
            call c_f_pointer(insp%inspCPtr, insp%inspPtr)

        end subroutine insp_init


        subroutine insp_add_parloop(insp, name, set, descriptors)

            type(sl_inspector), target :: insp
            character(kind=c_char,len=*), intent(in) :: name
            type(sl_set), target, intent(in) :: set
            type(sl_desc_list), target, intent(in) :: descriptors
            integer(kind=c_int) :: result

            result = insp_add_parloop_c(insp%inspCPtr, name, set%setCPtr, descriptors%descListCPtr)

        end subroutine insp_add_parloop


        subroutine insp_run(insp, suggestedSeed)

            type(sl_inspector), target :: insp
            integer(kind=c_int), value, intent(in) :: suggestedSeed
            integer(kind=c_int) :: result

            result = insp_run_c(insp%inspCPtr, suggestedSeed)

        end subroutine insp_run


        subroutine insp_print(insp, level, loopIndex)

            type(sl_inspector), target :: insp
            integer(kind=c_int), value, intent(in) :: level
            integer(kind=c_int), value, intent(in), optional :: loopIndex

            call insp_print_c(insp%inspCPtr, level, -1)

        end subroutine insp_print


        subroutine exec_init (exec, insp)

            type(sl_executor) :: exec
            type(sl_inspector), target, optional :: insp

            exec%execCPtr = exec_init_c(insp%inspCPtr)

            call c_f_pointer(exec%execCPtr, exec%execPtr)
    
        end subroutine exec_init


        subroutine exec_num_colors (exec, nColors)

            type(sl_executor), target :: exec
            integer(4), intent (out) :: nColors

            nColors = exec_num_colors_c(exec%execCPtr)

        end subroutine exec_num_colors


        subroutine exec_tiles_per_color(exec, color, nTilesPerColor)

            type(sl_executor), target :: exec
            integer(kind=c_int), value, intent(in) :: color
            integer(4), intent (out) :: nTilesPerColor

            nTilesPerColor = exec_tiles_per_color_c(exec%execCPtr, color)
        
        end subroutine exec_tiles_per_color


        subroutine exec_tile_at(tile, exec, color, ithTile, region)

            type(sl_tile), target :: tile
            type(sl_executor), target :: exec
            integer(kind=c_int), value, intent(in) :: color
            integer(kind=c_int), value, intent(in) :: ithTile
            integer(kind=c_int), value, intent(in), optional :: region

            if(present(region)) then
                tile%tileCPtr = exec_tile_at_c(exec%execCPtr, color, ithTile, region)
            else
                tile%tileCPtr = exec_tile_at_c(exec%execCPtr, color, ithTile, 0)
            end if
        
        end subroutine exec_tile_at


        subroutine tile_get_local_map(iterationsList, tile, loopIndex, mapName, loopSize)

            integer(4), pointer, dimension(:), intent (out) :: iterationsList
            type(sl_tile), target :: tile
            integer(kind=c_int), value, intent(in) :: loopIndex
            character(kind=c_char,len=*), intent(in) :: mapName
            integer(kind=c_int), value, intent(in) :: loopSize

            CALL c_f_pointer(tile_get_local_map_c(tile%tileCPtr, loopIndex, mapName),iterationsList, (/loopSize/))
        
        end subroutine tile_get_local_map


        subroutine tile_get_iterations(iterationsList, tile, loopIndex, loopSize)

            integer(4), pointer, dimension(:), intent (out) :: iterationsList
            type(sl_tile), target :: tile
            integer(kind=c_int), value, intent(in) :: loopIndex
            integer(kind=c_int), value, intent(in) :: loopSize

            CALL c_f_pointer(tile_get_iterations_c(tile%tileCPtr, loopIndex),iterationsList, (/loopSize/))

        end subroutine tile_get_iterations


        subroutine tile_loop_size(loopSize, tile, loopIndex)

            integer(4), intent (out) :: loopSize
            type(sl_tile), target :: tile
            integer(kind=c_int), value, intent(in) :: loopIndex
            
            loopSize = tile_loop_size_c(tile%tileCPtr, loopIndex)
        
        end subroutine tile_loop_size

        subroutine generate_vtk(insp, level, nodes, coordinates, meshDim, rank)

            type(sl_inspector), target :: insp
            integer(4), intent (in) :: level
            type(sl_set), target :: nodes
            real(8), dimension(:), target, intent (in) :: coordinates
            integer(4), intent (in) :: meshDim
            integer(4), intent (in), optional :: rank

            if(present(rank)) then
                call generate_vtk_c(insp%inspCPtr, level, nodes%setCPtr, c_loc(coordinates), meshDim, rank)
            else
                call generate_vtk_c(insp%inspCPtr, level, nodes%setCPtr, c_loc(coordinates), meshDim, 0)
            end if
        
        end subroutine generate_vtk

end module SLOPE_Fortran_Declarations

