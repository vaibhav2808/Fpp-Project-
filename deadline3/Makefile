all: cotton.o
	g++ -shared -o libcotton.so cotton.o -lpthread

cotton.o: cotton-runtime.cpp cotton-runtime.h
	g++ -c -fPIC -o cotton.o cotton-runtime.cpp

test: libcotton.so heat.cpp
	g++ -g -L./ -lcotton -o heat heat.cpp

clean:
	rm cotton.o libcotton.so heat