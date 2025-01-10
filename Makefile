OPT = /opt/homebrew
#OPT = /opt/local
#OPT = /usr/local

all:
	c++ -std=c++17 -O3 -I$(OPT)/include -L$(OPT)/lib *.cpp -o emu9995 -lSDL2
