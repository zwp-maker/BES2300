
cmd_platform/drivers/norflash/built-in.a := ( /usr/bin/printf 'create platform/drivers/norflash/built-in.a\n addmod platform/drivers/norflash/norflash_gd25q32c.o,platform/drivers/norflash/norflash_drv.o,platform/drivers/norflash/norflash_gd25lq32c.o,platform/drivers/norflash/norflash_en25s80b.o\n save\nend' | arm-none-eabi-ar -M )
