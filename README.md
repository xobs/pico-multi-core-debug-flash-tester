# Pico Dual Core RAM Issue Demo

Demonstrate that resetting the RP2040 is hard! Especially when there's multicore.

## Building

1. mkdir build
2. cd build
3. cmake .. -DCMAKE_BUILD_TYPE=Debug -DPICO_BOARD=pico
4. make

## Description

The standard method to reset an ARM core is to poke `AIRCR`. This works if there's only a single core, but if there are multiple cores then this might not work.

THe preferred method on RP2040 appears to be to poke the watchdog timer. This has the ability to reset the system immediately. With the proper protocol recovery this can be made to be very reliable as it immediately places core1 in WFI and resets the rest of the system

This program demonstrates why this is an issue:

```
$ make -C build; and probe-rs download build/pico_dual_core_demo.elf --chip RP2040
[  3%] Built target bs2_default
[  7%] Built target bs2_default_library
[100%] Built target pico_dual_core_demo
 WARN probe_rs::architecture::arm::core::armv6m: The core is in locked up status as a result of an unrecoverable exception
      Erasing ✔ 100% [####################]  20.00 KiB @  50.12 KiB/s (took 0s)
  Programming ✔ 100% [####################]  20.00 KiB @  21.11 KiB/s (took 1s)                                                                                 Finished in 1.35s
 WARN probe_rs::architecture::arm::core::armv6m: The core is in locked up status as a result of an unrecoverable exception

$
```

Running the command a second time produces a warning that memory is corrupt. This is because core1 is clobbering the reset vector which probe-rs uses to load its flash algorithm:

```
$ make -C build; and probe-rs download build/pico_dual_core_demo.elf --chip RP2040
[  3%] Built target bs2_default
[  7%] Built target bs2_default_library
[100%] Built target pico_dual_core_demo
ERROR probe_rs::flashing::flasher: Failed to verify flash algorithm. Data mismatch at address 0x20000000
ERROR probe_rs::flashing::flasher: Original instruction: 0xbe00be00
ERROR probe_rs::flashing::flasher: Readback instruction: 0x17591759
ERROR probe_rs::flashing::flasher: Original: [be00be00, b085b5f0, 447c4c1e, 28017820, f000d101, 2001f83b, 70209004, 46304e15, f00030f7, 9403f889, 4f134604, f0001cb8, 4605f883, f0004630, 9002f87f, f000480f, 9001f87b, f000480e, 4606f877, f0004638, 4607f873, 47a847a0, 4478480b, 9902c030, 99016001, c0c23004, 99039804, 20007008, bdf0b005, 4552, 5843, 5052, 4346, 15e, f6, 4c08b5b0, 7820447c, d1082801, 447d4d06, 47806928, 47806968, 70202000, 2001bdb0, 46c0bdb0, d8, b6, 44784805, 28007800, 2001d101, 48014770, 46c04770, 70d0, ae, 490ab510, 78094479, d10c2901, 709210f, 49071840, 688c4479, 3112201, 23d80412, 200047a0, 2001bd10, 46c0bd10, 90, 68, 460bb510, 44794908, 29017809, 210fd10a, 18400709, 44794905, 461168cc, 47a0461a, bd102000, bd102001, 5a, 32, 2114b280, 1e898809, 2a00884a, 1d09d004, d1f94282, 47708808, d4d4defe, 0, 0, 0, 0, 0, 0, d4d4d400]
ERROR probe_rs::flashing::flasher: Readback: [17591759, 9de0e49, 9dd5a577, 815ad179, 48582959, 795a5194, c878e85c, 9f89a76e, 49598950, ec5b50e1, a86c9f5d, 49597511, 9f5e51dc, 49599f89, e7594fd6, 46569e65, e7584fd2, 47579f65, 9c5c4ecd, 47579d8f, 9c5d4ec9, 9eff9ef7, 9ace9e61, ef581686, ef57b657, 1618865a, f05aef5b, 7656c65e, d400055, 505095a2, 5050a893, 5050a0a2, 50509396, 505051ae, 4f4f4f45, 9c580500, c87094cc, 20577750, 94cd9d56, 97d0b978, 97d0b9b8, c0707050, 6c4d09fc, 910b08fb, 4b4b4b23, 4c4c4c02, 8fc39350, 744cc44c, 6b4c1c4c, 944d93bc, 910b92bb, 4b4bbb1b, 4b4b4bf9, 9455005b, c45590c5, 1c57744c, 494b6351, 8c4a5b83, aace86bb, 46546544, 651a4654, 63438ae3, 6243ff52, 89030053, 90, 68, 460bb510, 44794908, 29017809, 210fd10a, 18400709, 44794905, 461168cc, 47a0461a, bd102000, bd102001, 5a, 32, 2114b280, 1e898809, 2a00884a, 1d09d004, d1f94282, 47708808, d4d4defe, 0, 0, 0, 0, 0, 0, d4d4d400]
      Erasing ✔   0% [--------------------]        0 B @        0 B/s (ETA 0s)                                                                             Error: An error with the flashing procedure has occurred.

Caused by:
    The RAM contents did not match the expected contents after loading the flash algorithm.
$
```

Note also that `probe-rs` is currently unable to reset a target that uses a second core. It's unclear why this is. This is evidenced by the fact that commenting out the `multicore_launch_core1(clobber_memory);` line.

It appears as though the following does work:

```
$ probe-rs download build/pico_dual_core_demo.elf --chip RP2040 # This results in corrupted memory
$ probe-rs download build/pico_dual_core_demo.elf --chip RP2040 # This works
$ probe-rs reset --chip RP2040 --core 0 # Reset core 0 -- core 1 is still locked up
$ probe-rs reset --chip RP2040 --core 1 # Resetting core 1 allows core 0 to continue
```