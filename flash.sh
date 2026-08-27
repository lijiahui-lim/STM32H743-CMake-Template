#!/bin/bash
set -e

ELF="build/fire_stm32.elf"

if [ ! -f "$ELF" ]; then
    echo "ELF not found: $ELF"
    echo "Run ./build.sh first"
    exit 1
fi

echo "==> Flashing $ELF"
sudo openocd -f interface/cmsis-dap.cfg -c "cmsis_dap_backend hid" \
    -f target/stm32h7x.cfg \
    -c "adapter speed 2000" \
    -c "reset_config srst_only srst_nogate connect_assert_srst" \
    -c "program $ELF verify reset" \
    -c "shutdown"

echo "==> Done"
