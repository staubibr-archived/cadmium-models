CC=g++
CFLAGS=-std=c++17

INCLUDECADMIUM=-I C:/Cadmium-Simulation-Environment/cadmium/include
INCLUDEDESTIMES=-I C:/Cadmium-Simulation-Environment/DESTimes/include
INCLUDEJSON=-I C:/Cadmium-Simulation-Environment/cadmium/json/include
INCLUDEWEB=-I C:/Cadmium-Simulation-Environment/cadmium-web-extension/include

#CREATE BIN AND BUILD FOLDERS TO SAVE THE COMPILED FILES DURING RUNTIME
bin_folder := $(shell mkdir -p bin)
build_folder := $(shell mkdir -p build)
results_folder := $(shell mkdir -p simulation_results)

#TARGET TO COMPILE ALL THE TESTS TOGETHER (NOT SIMULATOR)
main_top.o: top_model/main.cpp
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(INCLUDEWEB) top_model/main.cpp -o build/main_top.o

#TARGET TO COMPILE ONLY ABP SIMULATOR
simulator: main_top.o
	$(CC) -g -o bin/ABP build/main_top.o
	
#TARGET TO COMPILE EVERYTHING (ABP SIMULATOR + TESTS TOGETHER)
all: simulator

#CLEAN COMMANDS
clean: 
	rm -f bin/* build/*