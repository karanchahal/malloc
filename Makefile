all:
	g++ --std=c++11 -o build/main main.cpp
	./build/main
debug:
	g++ -g --std=c++11 -o build/main main.cpp