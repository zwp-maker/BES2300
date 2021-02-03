
cmd_utils/intersyshci/built-in.a := ( /usr/bin/printf 'create utils/intersyshci/built-in.a\n addmod \n addlib utils/intersyshci/libintersyshci_enhanced_stack.a\nsave\nend' | arm-none-eabi-ar -M )
