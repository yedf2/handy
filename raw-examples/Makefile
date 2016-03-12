SOURCES = $(shell ls *.cc)

PROGRAMS = $(SOURCES:.cc=)

default:
	@echo make \<program\>  [ program can be \"$(PROGRAMS)\" ]

clean:
	-rm -f $(PROGRAMS)
	-rm -f *.o

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cc:
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

