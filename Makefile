#
# The following environment variable(s) should be predefined:
#
# CXX
# CXX_OPTS
# SLOPE_ARCH (mac,linux)

#
# The following environment variable(s) can be predefined
#
# DEBUG: run in debug mode, printing additional information

#
# Set paths for various files
#

ST = sparsetiling
LIB = lib
OBJ = obj
DEMOS = demos
BIN = bin
TESTS = tests

ST_SRC = $(ST)/src
ST_INC = $(ST)/include
ST_BIN = $(BIN)/sparsetiling
ST_DEMOS = $(DEMOS)/sparsetiling
ST_TESTS = $(TESTS)/sparsetiling

#
# Compiler settings
#

CXX      := $(CXX)
CXXFLAGS := -std=c++0x $(VTKON) $(CXX_OPTS)

ifeq ($(SLOPE_ARCH),linux)
  CLOCK_LIB = -lrt
endif


.PHONY: clean mklib

all: clean mklib sparsetiling tests demos

mklib:
	@mkdir -p $(LIB) $(OBJ) $(ST_BIN)/airfoil $(ST_BIN)/tests

sparsetiling: mklib
	@echo "Compiling the library"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/executor.cpp -o $(OBJ)/executor.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/coloring.cpp -o $(OBJ)/coloring.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/map.cpp -o $(OBJ)/map.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tile.cpp -o $(OBJ)/tile.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tiling.cpp -o $(OBJ)/tiling.o
	ar -r $(LIB)/libst.a $(OBJ)/inspector.o $(OBJ)/partitioner.o $(OBJ)/coloring.o $(OBJ)/tile.o $(OBJ)/tiling.o \
		$(OBJ)/map.o $(OBJ)/executor.o
	ranlib $(LIB)/libst.a

tests: mklib
	@echo "Compiling the tests"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_TESTS)/test_inspector.cpp -o $(ST_BIN)/tests/test_inspector $(LIB)/libst.a $(CLOCK_LIB)

demos: mklib
	@echo "Compiling the demos"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_DEMOS)/airfoil/airfoil.cpp -o $(ST_BIN)/airfoil/airfoil $(CLOCK_LIB)

clean:
	-rm -if $(OBJ)/*.o
	-rm -if $(OBJ)/*~
	-rm -if $(LIB)/*.a
	-rm -if $(LIB)/*.so
	-rm -if $(ST_SRC)/*~
	-rm -if $(ST_INC)/*~
	-rm -if $(ST_DEMOS)/airfoil/*~
	-rm -if $(ST_BIN)/airfoil/airfoil_*
	-rm -if $(ST_BIN)/tests/test_*
