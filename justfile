cflags := "-O3 -Wall -Wextra -Wpedantic -Wformat=2 -Wno-unused-parameter -Wshadow -Wwrite-strings -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs -fsanitize=address"

cc := if `which clang` != "" { "clang" } else { "gcc" }
destdir := if env_var("USER") == "root" { "/usr/bin/" } else { "$HOME/.local/bin/" } 

batc:
	@[ -d release ] || mkdir release
	{{cc}} src/*.c {{cflags}} -o release/batc

debug:
	@[ -d debug ] || mkdir debug
	{{cc}} src/*.c {{cflags}} -o debug/batc -DDEBUG 

run: batc
    release/batc -l

install:
    @cp ./batc {{destdir}}
    @chmod 775 {{destdir}}/batc
    @echo "batc has been installed to: {{destdir}}"

uninstall:
	@rm {{destdir}}/batc

clean:
	@rm -rf release debug

