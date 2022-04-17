all: cotton.o
	g++ -shared -o libcotton.so cotton.o -lpthread

cotton.o: cotton-runtime.cpp cotton-runtime.h
	g++ -c -fPIC -o cotton.o cotton-runtime.cpp

test: libcotton.so heat.cpp sor.cpp nqueens.cpp
	g++ -g -O3 -L./ -lcotton -o heat heat.cpp
	g++ -g -O3 -L./ -lcotton -o sor sor.cpp
	g++ -g -O3 -L./ -lcotton -o nqueens nqueens.cpp

clean:
	rm cotton.o libcotton.so heat