include config.mk
# OpenBench overrides both of these on the command line.
EXE ?= $(ENGINE_NAME)
CXX ?= g++

SRC_DIR := src

# Every .cpp under src/ and src/*/ (main, ChessCore, Engine, misc).
SOURCES := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)

CXXFLAGS := -std=c++23 -O3 -DNDEBUG -march=native -flto -funroll-loops \
            -I$(SRC_DIR) -Iexternal \
            -DENGINE_NAME=\"$(ENGINE_NAME)\" \
            -DENGINE_VERSION=\"$(ENGINE_VERSION)\" \
            -Wno-unknown-pragmas

LDFLAGS := -flto -pthread

ifeq ($(OS),Windows_NT)
    # MinGW defaults to a 2 MB stack; the search recurses deeply.
    LDFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--stack,16777216
endif

.PHONY: all clean
all:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXE) $(LDFLAGS)

clean:
	rm -f $(EXE) $(EXE).exe