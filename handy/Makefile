include ../platform_config.mk
include srcs.mk
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
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
