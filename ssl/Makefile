include ../config.mk

SOURCES=ssl-cli.cc ssl-svr.cc https-svr.cc

PROGRAMS = $(SOURCES:.cc=)

default: $(PROGRAMS)

$(PROGRAMS): $(LIBFILE) ssl-conn.o

clean:
	-rm -f $(PROGRAMS)
	-rm -f *.o

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cc:
	$(CXX) -o $@ $< $(CXXFLAGS) ssl-conn.o $(LDFLAGS) -lssl -lcrypto

