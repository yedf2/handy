include ../platform_config.mk
include srcs.mk

PROGRAMS = $(SOURCES:.cc=)

default: $(PROGRAMS)

$(PROGRAMS): $(LIBFILE)

clean:
	-rm -f $(PROGRAMS)
	-rm -f *.o

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cc:
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

