$(shell ./build_config config.mk)
include config.mk
Nothing: default
default clean:
	@for d in $(SUBDIRS);\
		do \
			echo "making $@ in $$d"; \
			(cd $$d && make $@ -j2) || exit 1; \
		done
