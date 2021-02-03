
cmd_services/tws_ibrt/built-in.a := ( /usr/bin/printf 'create services/tws_ibrt/built-in.a\n addmod \n addlib services/tws_ibrt/libtws_ibrt_enhanced_stack.a\nsave\nend' | arm-none-eabi-ar -M )
