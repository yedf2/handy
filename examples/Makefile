include ../config.mk

SOURCES=chat.cc daemon.cc echo.cc hsha.cc http-echo.cc \
		stat.cc codec-cli.cc codec-svr.cc

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

