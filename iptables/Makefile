XTBL := $(shell pkg-config --variable=xtlibdir xtables)

CFLAGS = -fPIC -I../kernel -g -Wall
ifneq (z$(IPTINC),'z')
	CFLAGS += -I$(IPTINC)
endif

all:  libipt_iftag.so
install:  libipt_iftag.so
	if [ -n "$(DESTDIR)$(XTBL)" -a -d $(DESTDIR)$(XTBL) ]; then \
		install libipt_iftag.so $(DESTDIR)$(XTBL); \
	else echo "No pkg-config --variable=xtlibdir xtables"; fi

lib%.so: lib%.o
	$(CC) -shared -o $@ $^;
lib%.o: lib%.c ../kernel/ipt_iftag.h
	$(CC) ${CFLAGS} -D_INIT=lib$*_init -c -o $@ $<;
clean:
	rm -rf libipt_iftag.so

