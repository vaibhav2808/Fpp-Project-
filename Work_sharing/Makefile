all: stamp.o
	g++ -shared -o libstamp.so stamp.o -lpthread

stamp.o: stamp.cpp
	g++ -c -fPIC -o stamp.o stamp.cpp

clean: stamp.o libstamp.so
	rm stamp.o libstamp.so


