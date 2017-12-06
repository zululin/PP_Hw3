CC = mpicc
CXX = mpicxx
CFLAGS = -O3 -std=gnu99 -lm
CXXFLAGS = -O3 -std=gnu++11
TARGETS = APSP_Pthread APSP_MPI_sync

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) $(TARGETS:=.o)
