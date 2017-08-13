# OPT ?= -O2 -DNDEBUG
# (B) Debug mode, w/ full line-level debugging symbols
OPT ?= -g2
# (C) Profiling mode: opt, but w/debugging symbols
# OPT ?= -O2 -g2 -DNDEBUG
include ../config.mk

CFLAGS += -I.. $(PLATFORM_CCFLAGS) $(OPT)
CXXFLAGS += -I.. $(PLATFORM_CXXFLAGS) $(OPT)

LDFLAGS += $(PLATFORM_LDFLAGS)
LIBS += $(PLATFORM_LIBS)


SOURCES=proto_msg.cc test.cc 

OBJS = $(SOURCES:.cc=.o)

PROGRAMS = test

default: $(PROGRAMS)

$(PROGRAMS): $(OBJS)
	$(CXX) -o $@ $^ msg.pb.cc $(LDFLAGS) `pkg-config --libs protobuf` -lhandy

$(OBJS): middle

middle: msg.proto
	protoc --cpp_out=. $<
	@touch $@

clean:
	-rm -f $(PROGRAMS) middle
	-rm -f *.o *.pb.*

.cc.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@ `pkg-config --cflags protobuf`

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

