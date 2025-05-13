ifneq ($(shell which clang 2> /dev/null),)
	CC := clang
else
	CC := gcc
endif

CFLAGS := -O3 -Wall -Wextra -Wpedantic \
          -Wformat=2 -Wno-unused-parameter -Wshadow \
          -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
          -Wredundant-decls -Wnested-externs -Wmissing-include-dirs

SRC := $(wildcard src/*.c)

ifeq (${USER},root)
	DESTDIR := /usr/bin/
else
	DESTDIR := ~/.local/bin/
endif

batc: $(SRC)
	@[ -d release ] || mkdir release
	$(CC) $(SRC) $(CFLAGS) -o release/batc 

debug: $(SRC)
	@[ -d debug ] || mkdir release
	$(CC) $(SRC) $(CFLAGS) -o debug/batc -DDEBUG

install: release/batc $(DESTDIR)
	@cp release/batc $(DESTDIR)
	@chmod 775 $(DESTDIR)/batc
	@echo "batc has been installed to: $(DESTDIR)"

uninstall: $(DESTDIR)
	@rm $(DESTDIR)/batc
