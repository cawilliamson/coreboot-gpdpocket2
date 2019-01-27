# Objective
The aim here is to get a working build of Coreboot and possibly Libreboot down the line for the GPD Pocket 2.

# Status
So far I have discovered that the GPD Pocket 2 has a few interesting (and sometimes frustrating) quirks:

- The BIOS chip in use (GigaDevices GD25Q64CWIG) is connected using a WSON8 connector. This is extremely annoying because it means I can't use a SOIC8 test clip to flash the BIOS chip. That essentially means that in order to experiment with Coreboot builds and be able to revertback to a stable build - I would need to de-solder and remove the chip every time! (grr)

- My plan for resolving the above issue is to replace the GigaDevices GD25Q64C BIOS / UEFI chip with a GigaDevices GD25Q64CSIG which is the exact same chip but with a SOP8 connector which *can* be written to and read with a test clip. That means I can flash the BIOS in-place without removing it from the computer.

- So far I have ordered the following in order to progress with the hardware testing side of this project:

ZkeeShop CH341A 24/25 Series SPI Router LCD Flash BIOS USB Programmer (supports 3.3V BIOS chips)
WINGONEER SOIC8 SOP8 Test Clip For EEPROM 93CXX / 25CXX / 24CXX in-circuit programming+2 adapters (any SOIC8 clip is fine)
GD25Q64CSIG Memory NOR Flash Quad I/O SPI 120MHz 2.7-3.6V SOP8 GIGADEVICE (exact replacement for existing chip with SOC8 connectors)

# Notes
Most people should not require any special hardware to flash the final result of this project - I only need this stuff because in the course of development I will obviously break things and will need to be able to roll my BIOS back to either a previous build of coreboot *or* the original GPD BIOS.
