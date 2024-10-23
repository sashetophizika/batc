cflags := "-O3 -Wall -Wextra -Wpedantic -Wformat=2 -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs"

cc := if `which clang` != "" { "clang" } else { "gcc" }
destdir := if env_var("USER") == "root" { "/usr/bin/" } else { "$HOME/.local/bin/" } 

batc:
	{{cc}} main.c -o batc {{cflags}}

install:
    @cp ./batc {{destdir}}
    @chmod 775 {{destdir}}/batc
    @echo "batc has been installed to: {{destdir}}"

uninstall:
	@rm {{destdir}}/batc
