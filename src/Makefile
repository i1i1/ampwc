CFLAGS  += -Wall -ggdb
LDFLAGS +=

SUBDIRS  = compositor

all: build

include $(SUBDIRS:%=%/local.mk)

build: $(TARGETS)

clean:
	rm $(CLEANUP) $(TARGETS)

