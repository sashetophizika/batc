CC = clang
CFLAGS += -O3 -Wall -Wextra -Wpedantic \
          -Wformat=2 -Wno-unused-parameter -Wshadow \
          -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
          -Wredundant-decls -Wnested-externs -Wmissing-include-dirs

ifeq (${USER},root)
	DESTDIR := /usr/bin/
else
	DESTDIR := ~/.local/bin/
endif

batc: main.c
	$(CC) main.c -o batc $(CFLAGS)

install:
	@cp ./batc $(DESTDIR)
	@chmod 775 $(DESTDIR)/batc
	@echo "batc has been installed to: $(DESTDIR)"

uninstall:
	@rm $(DESTDIR)/batc
