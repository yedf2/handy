#OPT ?= -O2 -DNDEBUG
# (B) Debug mode, w/ full line-level debugging symbols
OPT ?= -g2
# (C) Profiling mode: opt, but w/debugging symbols
# OPT ?= -O2 -g2 -DNDEBUG
$(shell CC="$(CC)" CXX="$(CXX)" TARGET_OS="$(TARGET_OS)" ./build_config 1>&2)
include config.mk

CFLAGS += -I. $(PLATFORM_CCFLAGS) $(OPT)
CXXFLAGS += -I. $(PLATFORM_CXXFLAGS) $(OPT)

LDFLAGS += $(PLATFORM_LDFLAGS)
LIBS += $(PLATFORM_LIBS)

HANDY_SOURCES += $(shell find handy -name '*.cc')
HANDY_OBJECTS = $(HANDY_SOURCES:.cc=.o)

TEST_SOURCES = $(shell find test -name '*.cc')
TEST_OBJECTS = $(TEST_SOURCES:.cc=.o)

EXAMPLE_SOURCES += $(shell find examples -name '*.cc')
EXAMPLES = $(EXAMPLE_SOURCES:.cc=)

KW_SOURCES += $(shell find 10m -name '*.cc')
KW = $(KW_SOURCES:.cc=)

LIBRARY = libhandy.a

TARGETS = $(LIBRARY) handy_test $(EXAMPLES) $(KW)

default: $(TARGETS)
handy_examples: $(EXAMPLES)
$(EXAMPLES): $(LIBRARY)
$(KW): $(LIBRARY)

install: libhandy.a
	mkdir -p $(PREFIX)/usr/local/include/handy
	cp -f handy/*.h $(PREFIX)/usr/local/include/handy
	cp -f libhandy.a $(PREFIX)/usr/local/lib

uninstall:
	rm -rf $(PREFIX)/usr/local/include/handy $(PREFIX)/usr/local/lib/libhandy.a
clean:
			-rm -f $(TARGETS)
			-rm -f */*.o

$(LIBRARY): $(HANDY_OBJECTS)
		rm -f $@
			$(AR) -rs $@ $(HANDY_OBJECTS)

handy_test: $(TEST_OBJECTS) $(LIBRARY)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBS)

.cc.o:
		$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
		$(CC) $(CFLAGS) -c $< -o $@

.cc:
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LIBRARY) $(LIBS)
