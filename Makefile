# OPT ?= -O2 -DNDEBUG
# (B) Debug mode, w/ full line-level debugging symbols
OPT ?= -g2
# (C) Profiling mode: opt, but w/debugging symbols
# OPT ?= -O2 -g2 -DNDEBUG
$(shell ./build_config 1>&2)
include config.mk

CFLAGS += -I. -I./include $(PLATFORM_CCFLAGS) $(OPT)
CXXFLAGS += -I. -I./include $(PLATFORM_CXXFLAGS) $(OPT)

LDFLAGS += $(PLATFORM_LDFLAGS)
LIBS += $(PLATFORM_LIBS)

HANDY_SOURCES += $(shell find handy -name '*.cc')
HANDY_OBJECTS = $(HANDY_SOURCES:.cc=.o)

TEST_SOURCES = $(shell find test -name '*.cc')
TEST_OBJECTS = $(TEST_SOURCES:.cc=.o)

EXAMPLE_SOURCES += $(shell find examples -name '*.cc')
EXAMPLES = $(EXAMPLE_SOURCES:.cc=)

LIBRARY = libhandy.a

TARGETS = $(LIBRARY) handy_test $(EXAMPLES)

default: $(TARGETS)
handy_examples: $(EXAMPLES)
$(EXAMPLES): $(LIBRARY)

clean:
			-rm -f $(TARGETS)
			-rm -f */*.o

$(LIBRARY): $(HANDY_OBJECTS)
		rm -f $@
			$(AR) -rs $@ $(HANDY_OBJECTS)

handy_test: $(TEST_OBJECTS) $(LIBRARY)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBRARY) $(LIBS)

.cc.o:
		$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
		$(CC) $(CFLAGS) -c $< -o $@

.cc:
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LIBRARY) $(LIBS)
