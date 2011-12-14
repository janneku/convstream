CXX = g++
CXXFLAGS = -W -Wall -O2 -g -ansi -pedantic

all: testcases

testcases: testcases.o encoding.o
	$(CXX) $(CXXFLAGS) -o $@ testcases.o encoding.o
