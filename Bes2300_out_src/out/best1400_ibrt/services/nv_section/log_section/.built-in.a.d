
cmd_services/nv_section/log_section/built-in.a := ( /usr/bin/printf 'create services/nv_section/log_section/built-in.a\n addmod services/nv_section/log_section/crash_dump_section.o,services/nv_section/log_section/coredump_section.o,services/nv_section/log_section/log_section.o\n save\nend' | arm-none-eabi-ar -M )
