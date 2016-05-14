CXX=g++
CXXFLAGS += -pthread -DOS_LINUX -DLITTLE_ENDIAN=1 -std=c++11 -lhandy

SRCS=$(wildcard *.cc)
PROGS=$(SRCS:.cc=)

all:$(PROGS)
$(PROGS):%:%.cc
	$(CXX) $^ -o $@ $(CXXFLAGS)

.PHONY:clean
clean:
	rm $(PROGS)
