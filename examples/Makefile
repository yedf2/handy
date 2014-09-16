include ../platform_config.mk

SOURCES=chat.cc daemon.cc echo.cc echo_cli.cc hsha.cc http1.cc http-echo.cc log-bench.cc \
		stat1.cc tiny-echo.cc 

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

