all: cotton.o
	g++ -shared -o libcotton.so cotton.o -lpthread

cotton.o: cotton-runtime.cpp cotton-runtime.h
	g++ -c -fPIC -o cotton.o cotton-runtime.cpp

test: libcotton.so nqueens.cpp
	g++ -L./ -lcotton -o nqueens nqueens.cpp

clean:
	rm cotton.o libcotton.so nqueens