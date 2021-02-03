
cmd_platform/drivers/usb/usb_dev/built-in.a := ( /usr/bin/printf 'create platform/drivers/usb/usb_dev/built-in.a\n addmod platform/drivers/usb/usb_dev/cfg/usb_dev_desc.o\n addlib platform/drivers/usb/usb_dev/libusbdev.a\nsave\nend' | arm-none-eabi-ar -M )
