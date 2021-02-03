
cmd_apps/built-in.a := ( /usr/bin/printf 'create apps/built-in.a\n addmod \n addlib apps/audioplayers/built-in.a\n addlib apps/common/built-in.a\n addlib apps/main/built-in.a\n addlib apps/key/built-in.a\n addlib apps/pwl/built-in.a\n addlib apps/battery/built-in.a\n addlib apps/factory/built-in.a\n addlib apps/cmd/built-in.a\n addlib apps/mic/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
