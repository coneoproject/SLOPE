#
# The following environment variable(s) should be predefined:
#
# CXX
# CXX_OPTS

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
CXXFLAGS := -O3 -std=c++0x $(VTK_ON) $(CXX_OPTS)


.PHONY: clean mklib

all: clean mklib sparsetiling tests demos

mklib:
	@mkdir -p $(LIB) $(OBJ) $(ST_BIN)/airfoil $(ST_BIN)/tests

sparsetiling: mklib
	@echo "Compiling the library"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	ar -r $(LIB)/libst.a $(OBJ)/inspector.o $(OBJ)/partitioner.o
	ranlib $(LIB)/libst.a

tests: mklib
	@echo "Compiling the tests"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_TESTS)/test_inspector.cpp -o $(ST_BIN)/tests/test_inspector $(LIB)/libst.a -lrt

demos: mklib
	@echo "Compiling the demos"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_DEMOS)/airfoil/airfoil.cpp -o $(ST_BIN)/airfoil/airfoil -lrt

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
