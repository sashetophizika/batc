ifneq ($(shell which clang 2> /dev/null),)
	CC := clang
else
	CC := gcc
endif

CFLAGS := -O3 -Wall -Wextra -Wpedantic \
          -Wformat=2 -Wno-unused-parameter -Wshadow \
          -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
          -Wredundant-decls -Wnested-externs -Wmissing-include-dirs

ifeq (${USER},root)
	DESTDIR := /usr/bin/
else
	DESTDIR := ~/.local/bin/
endif

batc: main.c
	[ -d release ] || mkdir release
	$(CC) main.c -o ./release/batc $(CFLAGS)

debug: main.c
	[ -d debug ] || mkdir release
	$(CC) main.c -o ./debug/batc $(CFLAGS) -DDEBUG

install:
	@cp ./batc $(DESTDIR)
	@chmod 775 $(DESTDIR)/batc
	@echo "batc has been installed to: $(DESTDIR)"

uninstall:
	@rm $(DESTDIR)/batc
