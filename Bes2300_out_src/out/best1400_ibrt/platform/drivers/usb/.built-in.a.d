
cmd_platform/drivers/usb/built-in.a := ( /usr/bin/printf 'create platform/drivers/usb/built-in.a\n addmod \n addlib platform/drivers/usb/usb_dev/built-in.a\nsave\nend' | arm-none-eabi-ar -M )
