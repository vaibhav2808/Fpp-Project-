include $(HCLIB_ROOT)/include/hclib.mak

EXE=sor heat nqueens

all: $(EXE)

nqueens: nqueens.cpp
	$(CXX) -I./ $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

heat: heat.cpp
	$(CXX) -I./ $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

sor: sor.cpp
	$(CXX) -I./ $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^ $(PROJECT_LDLIBS)

clean:
	rm -rf *.o $(EXE)
