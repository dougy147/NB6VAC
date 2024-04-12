Emulate the NB6VAC with `qemu`.

| NOTE                       |
|:---------------------------|
| _Like the rest, this is still an early attempt, do not expect much._ |

* Dependencies
    - Host: `qemu`, `qemu-system-mips`, `docker`
    - Docker: `buildroot` (2015.02)

Reasons we need all that will be detailed further below.

**TODO**: import binaries into the emulation envrionment.

# Semi-automatic install

## Set up Docker

Install and enter a Docker environment from which we will build our kernel and filesystem with Buildroot 2015.02:

```console
$ ./enter-emulation-env
```

From this Docker environment, start the build process:

```console
$ cd this/repo/path
$ ./generate-env
```

There will be interactive procedures for the Linux kernel and uClibc configuration, so, please read their respective manual steps below for adequat configuration.

When done, simply:

```console
$ ./emulate
```

# Manual install

Generate the Docker environment and start building

```console
$ ./enter-emulation-env
$ cd this/repo/path/buildroot-2015.02
```
The pursuing procedure is interactive, so, please read on the following steps.

## Buildroot config

```console
$ make qemu_mips_malta_defconfig
```

```console
$ make menuconfig
    >>>> Either load "savedefconfig" and save on exit.
    >>>> Either set everything yourself:
        Target options > MIPS (big endian)
        Filesystem > ubifs root filesystem
        Build options > Enable compiler cache
        Build options > Show options and packages that are deprecated or obsolete
        Build options > build packages with debugging symbols (level 3)
        Build options > strip command for binaries on target = none
        Build options > gcc optimization level 0
        Toolchain > Buildroot toolchain
        Toolchain > Manually specified Linux version
        Toolchain > linux version: 3.4.11
        Toolchain > Custom kernel headers series: 3.4.x
        Toolchain > uClibc C library Version: uClibc 0.9.33.x
        Toolchain > Enable large file (files > 2 GB) support
        Toolchain > Enable IPv6 support
        Toolchain > Enable RPC support
        Toolchain > Enable WCHAR support
        Toolchain > Thread library implementation: linuxthreads (stable/old)
        Toolchain > Thread library debugging
        Toolchain > Build cross gdb for the host
        Toolchain >       - TUI support
        Toolchain >       - Python support
        Toolchain >       * GDB debugger Version:
        Toolchain >             - gdb 7.8.x
        Toolchain > Buildroot > Toolchain > Additional gcc options > '--enable-shared'
        System configuration > Passwords encoding md5 (or sha-256 ???)
        System configuration > Init system: BusyBox
        System configuration > /dev management: Dynamic using devtmpfs (or dynamic using mdev)
        System configuration > Instal timezone info
        Kernel: Kernel version: 3.4.11
        Kernel: Kernel patch > "../patches/patch-3.4.11-rt19.patch"
        Target packages > Compressors and ceompressors: bzip2, xz-utils
        Target packages > Debugging, profiling and benchmark: gdb (full debugger)
        Target packages > Filesystem and flash utilities: mtd, jffs2, ubifs tools
        Libraries > Crypto > libsha1, libssh2, openssl
        Libraries > JSON/XML > expat, json-c
        Networking applications > rsync
        Shell and utilities > file
        Filesystem images > ext2 (rev0)
        Host utilities > host mtd, jffs2 and ubi/ubifs tools
        Host utilities > host util-linux
    >>>> Save > "savedefconfig"
    >>>> Exit (save "Yes")
```

```console
$ make savedefconfig
```

## Linux kernel config

```console
$ make linux-menuconfig 
        Kernel type > Preemption model > Preemptible Kernel
        [M] Device drivers > Memory technology > NAND Device Support > Support for NAND Flash Simulator
        [M] Device drivers > Memory technology > OneNAND Device Support
        [M] Device drivers > Memory technology > UBI - Unsorted block images > Enable UBI
        [M] File systems > Miscellaneous filesystems > Journalling Flash File System v2 (JFFS2) support
        [M] File systems > Miscellaneous filesystems > UBIFS file system support
```

## uClibc config

```console
$ make uclibc-menuconfig 
        Development/debugging options > (-Wall) Compiler Warnings > "-Wall -ggdb -g3"
```

## Build images

```console
$ make
```

# Emulate

Once done building, you have a `vmlinux` and a filesystem in `./buildroot-2015.02/output/images`.
This path is read by the `emulate` script, so:

```console
$ ./emulate
```

Voilà!

# Troubleshoot

## Apply patch for `clear_pages` error

If you get `clear_pages` errors, do:

```console
$ cp ./patches/mips_ksyms.c  ./buildroot-2015.02/output/build/linux-3.4.11/arch/mips/kernel/mips_ksyms.c
$ cp ./patches/page-funcs.S  ./buildroot-2015.02/output/build/linux-3.4.11/arch/mips/mm/
$ cp ./patches/page.c        ./buildroot-2015.02/output/build/linux-3.4.11/arch/mips/mm/
$ cp ./patches/Makefile      ./buildroot-2015.02/output/build/linux-3.4.11/arch/mips/mm/Makefile
```

## Missing `crt1.o`, `crti.o` and `crtn.o`

To circumvent some errors about `crt1.o`, `crti.o` and `crtn.o` not being found, I had to `yay -S musl kernel-headers-musl` and `cp /lib/musl/lib/crt* ./buildroot-2015.02/output/build/host-gcc-initial-4.8.4/build/gcc/`.

The problem sometimes persists with `/bin/ld: cannot find -lc`, and unfortunately `cp /lib/musl/lib/libc.a output/build/host-gcc-initial-4.8.4/build/gcc/libc.a` does not solve...
I still don't get why I sometimes get this error.
To be checked.

# Sources

* Testing an ubi+ubifs on qemu-system-mipsel - [https://discussion.en.qi-hardware.narkive.com/TFJ05wcr/testing-an-ubi-ubifs-on-qemu-system-mipsel](https://discussion.en.qi-hardware.narkive.com/TFJ05wcr/testing-an-ubi-ubifs-on-qemu-system-mipsel)
