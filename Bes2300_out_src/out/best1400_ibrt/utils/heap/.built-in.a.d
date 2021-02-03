
cmd_services/../utils/heap/built-in.a := ( /usr/bin/printf 'create services/../utils/heap/built-in.a\n addmod services/../utils/heap/pool_api.o,services/../utils/heap/multi_heap.o,services/../utils/heap/heap_api.o\n save\nend' | arm-none-eabi-ar -M )
