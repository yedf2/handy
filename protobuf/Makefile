include ../platform_config.mk
include srcs.mk

OBJS = $(SOURCES:.cc=.o)

PROGRAMS = test

default: $(PROGRAMS)

$(PROGRAMS): $(LIBFILE) $(OBJS)
	$(CXX) -o $@ $(OBJS) msg.pb.cc $(LDFLAGS) `pkg-config --libs protobuf`

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

