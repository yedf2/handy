include ../config.mk

SOURCES=conf.cc daemon.cc file.cc handy.cc http.cc logging.cc net.cc stat-svr.cc \
		threads.cc util.cc codec.cc conn.cc $(SSLFILE)

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
