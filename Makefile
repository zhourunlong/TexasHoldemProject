CXX = g++
OPT = -O2 -std=c++14

PROGRAMS = dealer vzagent vzagent_fight

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

clean_log:
	rm -f ./logs/*

dealer: game.cpp game.h evalHandTables rng.cpp rng.h dealer.cpp net.cpp net.h
	$(CXX) -o $@ game.cpp rng.cpp dealer.cpp net.cpp $(OPT)

vzagent: game.cpp game.h evalHandTables rng.cpp rng.h vzagent.cpp net.cpp net.h
	$(CXX) -o $@ game.cpp rng.cpp vzagent.cpp net.cpp $(OPT)
