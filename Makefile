$(shell ./build_config 1>&2)
include config.mk
default clean:
	cd handy && make $@ || exit 1
