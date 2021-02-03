
cmd_utils/crash_catcher/built-in.a := ( /usr/bin/printf 'create utils/crash_catcher/built-in.a\n addmod utils/crash_catcher/CrashCatcher.o,utils/crash_catcher/HexDump.o,utils/crash_catcher/CrashCatcher_armv7m.o\n save\nend' | arm-none-eabi-ar -M )
