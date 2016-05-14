include ../config.mk
CXXFLAGS += $(PLATFORM_CXXFLAGS) -lhandy

SRCS=$(wildcard *.cc)
PROGS=$(SRCS:.cc=)

all:$(PROGS)
$(PROGS):%:%.cc
	$(CXX) $^ -o $@ $(CXXFLAGS)

.PHONY:clean
clean:
	rm $(PROGS)


