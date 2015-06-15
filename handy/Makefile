include ../config.mk

SOURCES = $(shell ls *.cc)

OBJECTS = $(SOURCES:.cc=.o)

LIBRARY = lib$(LIBNAME).a

default: $(LIBRARY)

clean:
	-rm -f $(LIBRARY)
	-rm -f *.o

$(LIBRARY): $(OBJECTS)
	rm -f $@
	$(AR) -rs $@ $(OBJECTS)

.cc.o:
	$(CXX) $(PLATFORM_CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(PLATFORM_CFLAGS) -c $< -o $@
