The goal is to flash an open firmware or to gain root access on a NB6VAC router.
Any help welcome!

| CURRENT STATE                |
|:-----------------------------|
| ✅ root access done!       |
| ⏲  trying to setup OpenWrt |

# Material

| Router                | Value                  |
|-----------------------|------------------------|
| Model                 | NB6VAC-FXC-r1          |
| Firmware              | `NB6VAC-MAIN-R4.0.45d` |
| Emergency firmware    | `NB6VAC-MAIN-R4.0.44k` |


| ![](assets/router.png)| ![](assets/router2.png)|
|:---------------------:|:----------------------:|
|                       |                        |

# Forensics on the serial console at boot

This is a work in progress.
Verbosity is intended.
Trying to follow along an expert's method on a [similar project](https://github.com/digiampietro/hacking-gemtek).

The UART pins, supposing the router lays on its WiFi and WPS buttons, go as follow (top to bottom):

> GND (TP_UART_GND)
TX (TP_UART_SOUT)
RX (TP_UART_SIN)
3V3 (TP_UART_3V3) <= do not connect this one

Let's start looking at what the serial console prints when booting:

* The boot loader is CFE version 1.0.39

```console
CFE version 1.0.39-116.174 for BCM963268 (32bit,SP,BE)
```
[https://openwrt.org/docs/techref/bootloader/cfe](https://openwrt.org/docs/techref/bootloader/cfe)

* The CPU is a BCM63168D0 (400MHz), MIPS architecture. The NAND flash chip is 128 mibibytes (128 MiB = 131072 KiB = 134217728 bytes). Here are some more parameters:

```console
Boot Strap Register:  0x1ff97bf
Chip ID: BCM63168D0, MIPS: 400MHz, DDR: 400MHz, Bus: 200MHz
Main Thread: TP0
Memory Test Passed
Total Memory: 134217728 bytes (128MB)
Boot Address: 0xb8000000

NAND ECC BCH-4, page size 0x800 bytes, spare size used 64 bytes
NAND flash device: , id 0xeff1 block 128KB size 131072KB
...
Broadcom NAND controller (BrcmNand Controller)
mtd->oobsize=0, mtd->eccOobSize=0
NAND_CS_NAND_XOR=00000000
B4: NandSelect=40000001, nandConfig=15142200, chipSelect=0
brcmnand_read_id: CS0: dev_id=eff10095
After: NandSelect=40000001, nandConfig=15142200
DevId eff10095 may not be supported.  Will use config info
Spare Area Size = 16B/512B
Block size=00020000, erase shift=17
NAND Config: Reg=15142200, chipSize=128 MB, blockSize=128K, erase_shift=11
busWidth=1, pageSize=2048B, page_shift=11, page_mask=000007ff
timing1 not adjusted: 6574845b
timing2 not adjusted: 00001e96
ECC level changed to 4
OOB size changed to 16
BrcmNAND mfg 0 0 UNSUPPORTED NAND CHIP 128MB on CS0

Found NAND on CS0: ACC=e3441010, cfg=15142200, flashId=eff10095, tim1=6574845b, tim2=00001e96
BrcmNAND version = 0x0400 128MB @00000000
brcmnand_scan: B4 nand_select = 40000001
brcmnand_scan: After nand_select = 40000001
handle_acc_control: default CORR ERR threshold  1 bits
ACC: 16 OOB bytes per 512B ECC step; from ID probe: 16
page_shift=11, bbt_erase_shift=17, chip_shift=27, phys_erase_shift=17
Brcm NAND controller version = 4.0 NAND flash size 128MB @18000000
ECC layout=brcmnand_oob_bch4_2k
brcmnand_scan:  mtd->oobsize=64
brcmnand_scan: oobavail=35, eccsize=512, writesize=2048
brcmnand_scan, eccsize=512, writesize=2048, eccsteps=4, ecclevel=4, eccbytes=7
-->brcmnand_default_bbt
brcmnand_default_bbt: bbt_td = bbt_slc_bch4_main_descr
Bad block table Bbt0 found at page 0000ffc0, version 0x01 for chip on CS0
Bad block table 1tbB found at page 0000ff80, version 0x01 for chip on CS0
brcmnand_reset_corr_threshold: default CORR ERR threshold  1 bits for CS0
brcmnand_reset_corr_threshold: CORR ERR threshold changed to 3 bits for CS0
brcmnandCET: Status -> Deferred
```

[https://openwrt.org/docs/techref/hardware/soc/soc.broadcom.bcm63xx](https://openwrt.org/docs/techref/hardware/soc/soc.broadcom.bcm63xx)

* The OTP EPROM-based MCU (microcontroller unit) is a [C8051T634](https://www.keil.com/dd/docs/datashts/silabs/c8051t63x.pdf). 
OTP stands for "One Time Programmable".
It could store factory settings such as keys (see section [About Secure Boot](#About-Secure-Boot)).

```console
[    0.611000] sb1_mcu: [MCUProtocolInitialize] OTP MCU (C8051T634) detected.
```

* There are multiple WiFi devices (identified by `wl0` and `wl1`), probably for the 2.4GHz and 5GHz frequencies.
Chips could be a Broadcom BCM43602 (wl0) and a Broadcom BCM435f (wl1).
The dhd (dongle host driver) has been compiled with OpenWRT.
There are lines I don't understand but that could be of interest (checking a resource availability in SPROM and OTP; updating SROM from flash if not found; ...).

```console
[   21.068000] Dongle Host Driver, version 7.14.89.14.cpe4.16L03.0-kdb
[   21.068000] Compiled from /home/crd/dev/trunk-next/openwrt/build_dir/linux-nova_sb1_sfr/broadcom-dhd/dhd//dhd/sys/dhd_common.c
[   21.068000] Compiled on Jan 23 2022 at 21:14:37
[   21.102000] +++++ Added gso loopback support for dev=wl0 <85023800>
[   21.113000] nvram_get_internal: variable boardrev defaulted to 0x10
[   21.125000] dhdpcie_download_code_file: download firmware /etc/wlan/dhd/43602a1/rtecdc.bin
[   21.583000] Neither SPROM nor OTP has valid image
[   21.588000] wl:srom/otp not programmed, using main memory mapped srom info(wombo board)
[   21.597000] wl: ID=pci/2/0/
[   21.600000] wl: ID=pci/2/0/
[   21.605000] wl: loading /etc/wlan/bcm43602_map.bin
[   21.610000] wl: updating srom from flash...
[   21.614000] srom rev:11
[   21.629000] dhdpcie_bus_write_vars: Download, Upload and compare of NVRAM succeeded.
[   21.682000] PCIe shared addr (0x001e8b0c) read took 42023 usec before dongle is ready
...
[   22.055000] CONSOLE: 000000.036 wl0: Broadcom BCM43602 802.11 Wireless Controller 7.35.177.83 (r669233)
...
[   24.360000] wl1: Broadcom BCM435f 802.11 Wireless Controller 7.14.89.14.cpe4.16L03.0-kdb
```

* CFE offers an interactive menu when a key is pressed from the serial at startup. It has interesting functions. For example, you can dump memory content (see section [NAND Dump](#NAND-Dump)), change boot parameters, try to run images from a TFTP server, etc. We could make further good use of this.

```console
*** Press any key to stop auto run (1 seconds) ***
CFE> help
Available commands:

fe                  Erase a selected partition of the flash (use fi to display informations).
fi                  Display informations about the flash partitions.
fb                  Find NAND bad blocks
dn                  Dump NAND contents along with spare area
phy                 Set memory or registers.
sm                  Set memory or registers.
dm                  Dump memory or registers.
db                  Dump bytes.
dh                  Dump half-words.
dw                  Dump words.
ww                  Write the 2 partition, you must choose the 0 wfi tagged image.
w                   Write the whole image with wfi_tag on the previous partition if wfiFlags field in tag is set to default
e                   Erase NAND flash
ws                  Write whole image (priviously loaded by kermit) to flash .
r                   Run program from flash image or from host depend on [f/h] flag
p                   Print boot line and board parameter info
c                   Change booline parameters
i                   Erase persistent storage data
a                   Change board AFE ID
b                   Change board parameters
reset               Reset the board
pmdio               Pseudo MDIO access for external switches.
spi                 Legacy SPI access of external switch.
force               override chipid check for images.
help                Obtain help for CFE commands
```

* The kernel is probably lzma compressed (infered from `binwalk`), and is authenticated before decompression.
There are useful memory addresses at that stage.
The latest boot image starts at `0xba080000` with a flash offset of `0x02080000`. 

```console
Boot Nand Image using OOB Tag
Booting from latest image (address 0xba080000, flash offset 0x02080000) ...
Authenticating vmlinux.lz ... pass
Decompression OK!
Entry at 0x80351e30
Starting program at 0x80351e30
```

The flash offset `0x02080000` corresponds to the location of a JFFS2 filesystem (big endian) on the NAND (byte `34078720`). 
This is observed applying `binwalk` to the full dump (note that `0x02` == `0x2` in hexa):

```console
$ binwalk MY-DUMP-FXC-r1-4.0.45d

DECIMAL       HEXADECIMAL     DESCRIPTION
...
34078720      0x2080000       JFFS2 filesystem, big endian
```

The starting program is the kernel. It is entered at `0x80351e30`.

* Kernel version is `3.4.11-rt19` (_rt_ means real-time) with command line `ro noinitrd  console=ttyS0,115200 earlyprintk debug irqaffinity=0`.
The CPU instruction set is `Broadcom BMIPS4350`.

```console
Starting program at 0x80351e30
[    0.000000] Linux version 3.4.11-rt19 (crd@docker-fortycore2) (gcc version 4.7.0 (GCC) ) #2 SMP PREEMPT Sun Jan 23 21:26:31 UTC 2022
...
[    0.000000] CPU revision is: 0002a080 (Broadcom BMIPS4350)
...
[    0.019000] --Kernel Config--
[    0.020000]   SMP=1
[    0.021000]   PREEMPT=1
[    0.022000]   DEBUG_SPINLOCK=0
[    0.023000]   DEBUG_MUTEXES=0
```

Kernel build number is `#2`. 
It is built to function on a multiprocessor computer (`SMP`; for Symmetric Multi-Processing).
The kernel is preemptible (`PREEMPT`), meaning it can be interrupted in the middle of executing code to handle other threads.
You can take a look at the extracted kernel configuration file [here](misc/kernel_config).

* The root filesystem is a `ubifs filesystem` :

```console
[    1.957000] VFS: Mounted root (ubifs filesystem) readonly on device 0:11.
```

* The 128MiB NAND device is partitioned as follow:

```console
[    0.894000] Creating 6 MTD partitions on "brcmnand.0":
[    0.899000] 0x000000000000-0x000000020000 : "bootloader"
[    0.907000] 0x000002080000-0x0000040e0000 : "rootfs-image"
[    0.915000] 0x0000022e0000-0x0000040e0000 : "rootfs"
[    0.923000] 0x000000020000-0x000002080000 : "upgrade-image"
[    0.931000] 0x000000280000-0x000002080000 : "upgrade"
[    0.938000] 0x000004100000-0x000007f00000 : "data"
```

Here is a transposition of the above partition table in number of blocks of 1024 bytes (= 1KiB = 1 kibibyte):

```console
  P   Device  Start    End    Length   Name
  -   ------ ------- -------- -------- ------------
  1   mtd0        0      128      128  bootloader
  2   mtd1   33,280   66,432   33,152  rootfs-image
  3   mtd2   35,712   66,432   30,720  rootfs
  4   mtd3      128   33,280   33,152  upgrade-image
  5   mtd4    2,560   33,280   30,720  upgrade
  6   mtd5   66,560  130,048   66,488  data
```

I know nothing about MTD or [UBIFS](https://www.sciencedirect.com/science/article/pii/S2666281723002081), but we can see overlaps of `30720` blocks between `*-images` and their matching filesystems.
For example, `rootfs-image` and `rootfs` both ends at block `66432`, but `rootfs-image` starts earlier, containing a portion of `2432` unshared blocks with `rootfs` (2490368 bytes). The same goes for `upgrade-image` and `upgrade`.

Extracting those unshared portions from `rootfs-image` and `upgrade-image` with `dd`, we can see they perfectly match JFFS2 partitions contained in some official firmwares (`cferam.000` + `secram.000` + `vmlinux.lz` + `vmlinux.sig`).
Respectively, `rootfs-image` and `upgrade-image`  match firmwares `4.0.45d` and `4.0.44k`.
Notice that `4.0.45d` is the main firmware of this router, and `4.0.44k` the emergency firmware.

```console
$ dd if=MY_DUMP of=rootfs-image-jffs2 skip=128 count=2432 bs=1024
$ dd if=NB6VAC-MAIN-R4.0.45d of=firmware-4.04.45d-jffs2 count=2432 bs=1024
$ diff -s firmware-4.04.45d-jffs2 rootfs-image-jffs2
Files firmware-4.04.45d-jffs2 and rootfs-image-jffs2 are identical
```

Further comparisons could be interesting.

* NAND representation (**not the partition table!**)

Reordering the above table, considering images with only their unshared slices, and taking empty spaces into accounts gives us the new following table:

```console
 P   Device  Start    End    Length   Name
--  ------ ------- -------- -------- ------------
 1   mtd0        0      128      128  bootloader
 2  ~mtd3      128    2,560    2,432  upgrade-image [part1]
 3   mtd4    2,560   33,280   30,720  upgrade       (and upgrade-image [part2])
 4  ~mtd1   33,280   35,712    2,432  rootfs-image  [part1]
 5   mtd2   35,712   66,432   30,720  rootfs        (and rootfs-image [part2])
 6    -     66,432   66,560      128  empty space
 7   mtd5   66,560  130,048   66,488  data
 8    -    130,048  131,072    1,024  bbt (bad block table see CFE below)
```

In this table and the diagram below, the tidle `~` before a mtd device indicates we have truncated its end block address (ignoring overlaping data with its filesystem).
Be cautious not to interpret this as the real partitioning table (see above).

![](assets/nand_partitions.png)

The `fi` command in the CFE interactive menu also informs us about NAND partitions:

```console
CFE> fi
0 : boot    offset=0x00000000, size=131072 (128 ko), block=1
1 : rootfs1 offset=0x00020000, size=33947648 (32 Mo), blocks=259
2 : rootfs2 offset=0x02080000, size=33947648 (32 Mo), blocks=259
3 : data    offset=0x04100000, size=65011712 (62 Mo), blocks=496
4 : bbt     offset=0x07f00000, size=1048576 (1024 ko), blocks=8
flash_end   offset=0x07f00400, blocks=1024
```

On the filesystem, `/etc/fstab` is simply populated with:

```console
proc	/proc		proc	defaults	0 0
none	/dev/pts	devpts	defaults	0 0
sysfs	/sys		sysfs	defaults	0 0
tmpfs	/tmp		tmpfs	defaults,mode=1777	0 0
```

* UBIFS Markers are announced `256` bytes before the `rootfs` and `upgrade` start.

```console
[    0.872000] ***** Found UBIFS Marker at 0x022dff00
[    0.889000] ***** Found UBIFS Marker at 0x0027ff00
```

An extracted UBIFS Marker looks like this: four times the string `BcmFs-ubifs.` (`42 63 6d 46 73 2d 75 62 69 66 73 00`).

```console
$ dd if=MY_DUMP of=rootfs-image_ubifs-marker bs=1 skip=$(( (35712 * 1024) - 256)) count=256
256+0 records in
256+0 records out
256 bytes copied, 0.000307293 s, 833 kB/s

$ hexdump -C rootfs-image_ubifs-marker
00000000  42 63 6d 46 73 2d 75 62  69 66 73 00 42 63 6d 46  |BcmFs-ubifs.BcmF|
00000010  73 2d 75 62 69 66 73 00  42 63 6d 46 73 2d 75 62  |s-ubifs.BcmFs-ub|
00000020  69 66 73 00 42 63 6d 46  73 2d 75 62 69 66 73 00  |ifs.BcmFs-ubifs.|
00000030  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000100
```

* Partition 3 (mtd2; rootfs) is a UBI file system using `zlib` compression attached to `ubi0` (UBI device 0, volume 0, name = "rootfs_ubifs").

```console
[    0.947000] UBI: attaching mtd2 to ubi0
[    0.950000] UBI: physical eraseblock size:   131072 bytes (128 KiB)
[    0.957000] UBI: logical eraseblock size:    126976 bytes
[    0.962000] UBI: smallest flash I/O unit:    2048
[    0.967000] UBI: VID header offset:          2048 (aligned 2048)
[    0.973000] UBI: data offset:                4096
[    1.290000] UBI: max. sequence number:       2
[    1.313000] UBI: attached mtd2 to ubi0
[    1.316000] UBI: MTD device name:            "rootfs"
[    1.321000] UBI: MTD device size:            30 MiB
[    1.326000] UBI: number of good PEBs:        240
[    1.331000] UBI: number of bad PEBs:         0
[    1.336000] UBI: number of corrupted PEBs:   0
[    1.340000] UBI: max. allowed volumes:       128
[    1.345000] UBI: wear-leveling threshold:    4096
[    1.350000] UBI: number of internal volumes: 1
[    1.355000] UBI: number of user volumes:     1
[    1.359000] UBI: available PEBs:             0
[    1.364000] UBI: total number of reserved PEBs: 240
[    1.369000] UBI: number of PEBs reserved for bad PEB handling: 2
[    1.375000] UBI: max/mean erase counter: 1/0
[    1.379000] UBI: image sequence number:  1558008590
[    1.384000] UBI: background thread "ubi_bgt0d" started, PID 235
...
[    1.912000] UBIFS: mounted UBI device 0, volume 0, name "rootfs_ubifs"
[    1.918000] UBIFS: mounted read-only
[    1.921000] UBIFS: file system size:   28315648 bytes (27652 KiB, 27 MiB, 223 LEBs)
[    1.929000] UBIFS: journal size:       9023488 bytes (8812 KiB, 8 MiB, 72 LEBs)
[    1.937000] UBIFS: media format:       w4/r0 (latest is w4/r0)
[    1.943000] UBIFS: default compressor: zlib
[    1.947000] UBIFS: reserved for root:  0 bytes (0 KiB)
```

* Partition 6 (mtd5; data) is a UBI file system using `lzo` compression attached to `ubi1` (UBI device 1, volume 0, name = "data_ubifs").

```console
[    2.775000] UBI: attaching mtd5 to ubi1
[    2.778000] UBI: physical eraseblock size:   131072 bytes (128 KiB)
[    2.784000] UBI: logical eraseblock size:    126976 bytes
[    2.790000] UBI: smallest flash I/O unit:    2048
[    2.795000] UBI: VID header offset:          2048 (aligned 2048)
[    2.801000] UBI: data offset:                4096
[    3.449000] UBI: max. sequence number:       543
[    3.473000] UBI: attached mtd5 to ubi1
[    3.476000] UBI: MTD device name:            "data"
[    3.481000] UBI: MTD device size:            62 MiB
[    3.486000] UBI: number of good PEBs:        496
[    3.491000] UBI: number of bad PEBs:         0
[    3.496000] UBI: number of corrupted PEBs:   0
[    3.500000] UBI: max. allowed volumes:       128
[    3.505000] UBI: wear-leveling threshold:    4096
[    3.510000] UBI: number of internal volumes: 1
[    3.514000] UBI: number of user volumes:     1
[    3.519000] UBI: available PEBs:             0
[    3.524000] UBI: total number of reserved PEBs: 496
[    3.529000] UBI: number of PEBs reserved for bad PEB handling: 4
[    3.535000] UBI: max/mean erase counter: 3/1
[    3.539000] UBI: image sequence number:  1307933915
[    3.544000] UBI: background thread "ubi_bgt1d" started, PID 325
[    3.928000] UBIFS: recovery needed
[    4.186000] UBIFS: recovery completed
[    4.189000] UBIFS: mounted UBI device 1, volume 0, name "data_ubifs"
[    4.196000] UBIFS: file system size:   60821504 bytes (59396 KiB, 58 MiB, 479 LEBs)
[    4.204000] UBIFS: journal size:       3047424 bytes (2976 KiB, 2 MiB, 24 LEBs)
[    4.211000] UBIFS: media format:       w4/r0 (latest is w4/r0)
[    4.217000] UBIFS: default compressor: lzo
[    4.222000] UBIFS: reserved for root:  2872749 bytes (2805 KiB)
```

* In **between** mtd2 (ubi0) and mtd5 (ubi1) mounting, unused kernel memory is freed and "pre init" scripts are executed:

```console
...
[    1.957000] VFS: Mounted root (ubifs filesystem) readonly on device 0:11.
[    1.967000] devtmpfs: mounted
[    1.971000] Freeing unused kernel memory: 204k freed
[    2.340000] [INFO] # - pre init -
[    2.719000] [_OK_] # /etc/init.d/early-devices boot
[    2.753000] [INFO] - flash union...
[    2.775000] UBI: attaching mtd5 to ubi1
...
```

Let's quickly deviate from serial console to look at those "pre init" scripts.
The most interesting script is the first called: `/etc/preinit`. It sources a bunch of logging functions from `/etc/functions` then executes this:

```console
mount proc /proc -t proc
mount sysfs /sys -t sysfs
hinfo "- pre init -"
/etc/init.d/early-devices boot
```

`/etc/init.d/early-devices boot` calls `/etc/rc.common`, which in turns calls back `/etc/init.d/early-devices start`. 
Here is a transformed but relevant portion of code detailing its actions:

```console
# /etc/init.d/early-devices start

# Make devices nodes:
mknod /dev/gmac c 249 0
mknod -m 640 /dev/bpm c 244 0
mknod -m 666 /dev/bcmendpoint0 c 209 0
mknod /dev/brcmboard c 206 0

# MTD symlinks
# create MTD link: bootloader
chmod a+w /dev/mtd0
chmod a+w /dev/mtdblock0
ln -sf /dev/mtd0 /dev/mtd-bootloader
ln -sf /dev/mtdblock0 /dev/mtdblock-bootloader

# create MTD link: rootfs
ln -sf /dev/mtd2 /dev/mtd-rootfs
ln -sf /dev/mtdblock2 /dev/mtdblock-rootfs

# create MTD link: rootfs-image
ln -sf /dev/mtd1 /dev/mtd-rootfs-image
ln -sf /dev/mtdblock1 /dev/mtdblock-rootfs-image

# create MTD link: upgrade
ln -sf /dev/mtd4 /dev/mtd-upgrade
ln -sf /dev/mtdblock4 /dev/mtdblock-upgrade

# create MTD link: upgrade-image
ln -sf /dev/mtd3 /dev/mtd-upgrade-image
ln -sf /dev/mtdblock3 /dev/mtdblock-upgrade-image

# create MTD link: data
chmod a+w /dev/mtd5
chmod a+w /dev/mtdblock5
ln -sf /dev/mtd5 /dev/mtd-data
ln -sf /dev/mtdblock5 /dev/mtdblock-data
```

Notice write permissions for all users (`chmod a+w`) are added to `bootloader` and `data`'s respective devices (`/dev/mtd{0,5}` and `/dev/mtdblock{0,5}`).

**NOTE**: part numbers for `/dev/mtdXXX` are infered from our partition table, but might be different on the running router.
Here is the function used in the script to extract them: 

```console
part_number=$(grep "\"$2\"" /proc/mtd|sed 's/^mtd\(.*\):.*/\1/')
if [ -z "$part_number" ]
then
    return
else
    create MTD link ...
fi
```

`$2` being the partition name (`bootloader`, `rootfs`, `rootfs-image`, `upgrade`, `upgrade-image`, or `data`).
Also note that if there is no match for `grep "$2" /proc/mtd` the symbolic links are not created.

The main script `/etc/preinit` then looks for an executable that I could not find in any firmware or filesystem I have extracted (a lot): `/usb/bin/usb-boot`.
If it fails (and it will), it proceeds with the `flash_union` function that could be of interest later.
It's already hard to keep it short, but basically, it mounts `data_ubifs` to `/overlay`:

```console
ubiattach /dev/ubi_ctrl -m 5 -d 1
mount -t ubifs -o noatime ubi1:data_ubifs /overlay/
```

Continuing with `/etc/preinit`, there are switches between directories, and noticably a `pivot_root` between `/root` and `/rom`:

```console
mount -t overlayfs overlayfs -olowerdir=/,upperdir=/overlay /root
cd /root
pivot_root . rom
mount -o move /rom/dev /dev 2>&-
mount -o move /rom/proc /proc 2>&-
mount -o move /rom/sys /sys 2>&-
mount -o move /rom/overlay/ /overlay 2>&-
```

Finally, `/etc/preinit` will fail to find another executable (`/sbin/bootchartd`) before starting `/sbin/init` which is, in fact, Busybox:

```console
if [ -x /sbin/bootchartd ]; then
    exec /usr/sbin/chroot . /sbin/bootchartd
else
    exec /usr/sbin/chroot . /sbin/init
fi
```

Let's go back to the serial console.

* Init program is `BusyBox 1.22.1`

```console
[    4.347000] init started: BusyBox v1.22.1 (2022-01-23 21:14:10 UTC)
```

It immediately proposes a console we can interact with (while the boot process continues):

```
Please press Enter to activate this console.

[NB6VAC-FXC-r1][NB6VAC-MAIN-R4.0.45d][NB6VAC-XDSL-A2pv6F039p]
nb6vac login: root
Password: 
Login incorrect
```

We will take a look later at how to circumvent it by flashing a crafted firmware containing our own `root` hashed password in `/etc/shadow`.

* After Busybox, scripts continue to be called in that order (leaving it here just in case):

```console
[   10.107000] [_OK_] # /etc/rc.d/S10boot boot
[   10.160000] [_OK_] # /etc/rc.d/S11watchdog boot
[   10.212000] [_OK_] # /etc/rc.d/S12usb boot
[   10.288000] [_OK_] # /etc/rc.d/S20cron boot
[   10.336000] [_OK_] # /etc/rc.d/S21dropbear boot
[   10.413000] [_OK_] # /etc/rc.d/S21plc-detect boot
[   10.470000] [_OK_] # /etc/rc.d/S22inetd boot
[   10.616000] [_OK_] # /etc/rc.d/S23syslog-ng boot
[   10.693000] kernel.hotplug = /sbin/hotplug-call
[   10.956000] [_OK_] # /etc/rc.d/S24hotplug boot
[   11.768000] input: uinput-neufbox as /devices/virtual/input/input1
[   12.350000] [_OK_] # /etc/rc.d/S25nbd boot
[   12.485000] net.nf_conntrack_max = 16384
[   12.491000] [_OK_] # /etc/rc.d/S26sysctl boot
[   12.870000] [_OK_] # /etc/rc.d/S30eco boot
[   13.324000] [_OK_] # /etc/init.d/dnsmasq reload
[   13.359000] [_OK_] # /etc/rc.d/S31hosts boot
[   18.174000] [_OK_] # /etc/rc.d/S32firewall boot
[   19.619000] [INFO] # boot phy
...
[   31.113000] [_OK_] # /etc/rc.d/S34phy boot
[   31.226000] bcmxtmcfg: bcmxtmcfg_init entry
[   31.257000] [_OK_] # /etc/rc.d/S35xtm boot
[   31.982000] NB6VAC-XDSL-A2pv6F039p [fw - size:983k cc:367b34d8h]
...
[   32.843000] [_OK_] # /etc/init.d/xtm start
[   32.865000] [_OK_] # /etc/rc.d/S36adsl boot
...
[   48.792000] [_OK_] # /etc/rc.d/S37topology boot
[   51.037000] [_OK_] # /etc/rc.d/S38qos boot
...
[   51.290000] [_OK_] # /etc/rc.d/S39data boot
[   51.390000] [_OK_] # /etc/rc.d/S40ppp-xdsl boot
[   51.852000] [_OK_] # /etc/rc.d/S41dhcpc boot
[   51.989000] [_OK_] # /etc/rc.d/S42ipv6 boot
[   53.177000] [_OK_] # /etc/rc.d/S43lan boot
[   53.312000] [_OK_] # /etc/rc.d/S44route boot
[   53.459000] [_OK_] # /etc/rc.d/S49wol boot
[   53.800000] [_OK_] # /etc/rc.d/S50miniupnpd boot
[   53.948000] [_OK_] # /etc/rc.d/S51lan-topology boot
[   54.045000] iptables: Bad rule (does a matching rule exist in that chain?).
[   54.484000] [_OK_] # /etc/rc.d/S52dnsmasq boot
[   54.593000] sh: 44k: bad number
...
[   67.749000] [_OK_] # /etc/rc.d/S53wlan boot
[   67.904000] [_OK_] # /etc/rc.d/S54wifisched boot
[   68.590000] RTNETLINK answers: File exists
[   68.612000] [_OK_] # /etc/rc.d/S55hotspot boot
[   68.751000] 1970-01-01 00:01:08: (network.c.252) warning: please use server.use-ipv6 only for hostnames, not without server.bind / empty address; your config will break if the kernel default for IPV6_V6ONLY changes
[   68.778000] [_OK_] # /etc/rc.d/S55lighttpd boot
[   69.145000] RTNETLINK answers: File exists
[   69.367000] [_OK_] # /etc/rc.d/S56guest boot
[   69.634000] [_OK_] # /etc/rc.d/S57iptv boot
[   69.709000] [_OK_] # /etc/rc.d/S58voip boot
[   75.102000] [_OK_] # /etc/rc.d/S59ont boot
[   75.207000] [_OK_] # /etc/rc.d/S99boot-terminated boot
```

And the boot process is over.

# Open ports and protocols

Further exploration when the router is up and running.

| Protocol | Port   |
|:---------|:------:|
|http      |`80`    | 
|telnet    |`1287`  | 
|ssh       |`1288`  | 
|upnp      |`49152` | 

### Shaking the router with `nmap`

See [nmap.log](https://github.com/dougy147/nb6vac/tree/master/logs/nmap.log) for full log.

```console
$ nmap -p0- -Pn -v -A -T4 192.168.1.1
Scanning 192.168.1.1 [1 port]
sendto in send_ip_packet_sd: sendto(6, packet, 44, 0, 192.168.1.1, 16) => Operation not permitted
Offending packet: TCP localhost:42087 > 192.168.1.1:53 S ttl=57 id=39784 iplen=44  seq=2250069918 win=1024 <mss 1460>
sendto in send_ip_packet_sd: sendto(6, packet, 44, 0, 192.168.1.1, 16) => Operation not permitted
Offending packet: TCP localhost:42089 > 192.168.1.1:53 S ttl=41 id=16251 iplen=44  seq=2250200988 win=1024 <mss 1460>
Discovered open port 80/tcp on 192.168.1.1
Discovered open port 1288/tcp on 192.168.1.1
Discovered open port 49152/tcp on 192.168.1.1
Discovered open port 1287/tcp on 192.168.1.1
WARNING: Service 192.168.1.1:49152 had already soft-matched upnp, but now soft-matched rtsp; ignoring second value
WARNING: Service 192.168.1.1:49152 had already soft-matched upnp, but now soft-matched sip; ignoring second value
Initiating OS detection (try #1) against 192.168.1.1
PORT      STATE SERVICE     VERSION
80/tcp    open  http        lighttpd
|_http-server-header: Server
|_http-title: Box -&nbsp;Accueil
|_http-favicon: Unknown favicon MD5: 94D3927A1DC46C1DAF81F8A48D49BC43
| http-methods:
|   Supported Methods: GET POST PUT DELETE OPTIONS HEAD
|_  Potentially risky methods: PUT DELETE
1287/tcp  open  routematch?
| fingerprint-strings:
|   NULL:
|_    diaglog[4093] [push/] aborted: no default route defined
1288/tcp  open  ssh         Dropbear sshd 2014.65 (protocol 2.0)
49152/tcp open  upnp        MiniUPnP 1.9 (UPnP 1.1)
| fingerprint-strings:
...
2 services unrecognized despite returning data. If you know the service/version, please submit the following fingerprints at https://nmap.org/cgi-bin/submit.cgi?new-service :
... check full log for more ...
```

### SSH

Even with root access, I did not manage to connect with `ssh` to the router:

```console
$ ssh 192.168.1.1 -p 1288
Unable to negotiate with 192.168.1.1 port 1288: no matching host key type found. Their offer: ssh-rsa,ssh-dss
```

On the router, the `dropbear` version is `v2014.65`.
We need to build that version on our machine to succeed.
Using `dbclient` instead of `ssh` prompts us for a password.
To connect with this method, check the section on how to build a root-access firmware and follow this:

```console
$ wget https://src.fedoraproject.org/repo/pkgs/dropbear/dropbear-2014.65.tar.bz2/1918604238817385a156840fa2c39490/dropbear-2014.65.tar.bz2
$ tar xvjf ./dropbear-2014.65.tar.bz2
$ cd ./dropbear-2014.65
$ ./configure && make
# no need to install
$ ./dbclient 192.168.1.1 -p 1288
# prompted for a password!
```

### telnet

```console
$ telnet 192.168.1.1 1287
Trying 192.168.1.1...
Connected to 192.168.1.1.
Escape character is '^]'.
diaglog[14373] [push/] aborted: no default route defined
Connection closed by foreign host.
```

The "_aborted: no default route defined_" message originates from the script `/usr/sbin/diaglog`.

## Root access

Getting root access requires to build a custom flashable firmware.
Here I describe how to proceed to build your own (I can share you mine if you ask).

1. Identify the current firmware version installed on your router, and [download it](#Available-firmwares). I'll proceed with `NB6VAC-MAIN-R4.0.45d`.

2. Remove WFI tags from the firmware file:

```console
$ FIRMWARE_SIZE=$(du -b NB6VAC-MAIN-R4.0.45d | awk '{print $1}')
$ dd if=NB6VAC-MAIN-R4.0.45d of=custom-firmware.bin count=$(( FIRMWARE_SIZE - 20 )) bs=1
```

3. At the moment, we are forced to keep the original JFFS2 partition (containing bootloader and authenticated kernel).
It is located before the first UBI partition. Let's separate the JFFS2 and UBI partition:

```console
$ CUT_SECTION=$(binwalk custom-firmware.bin 2>/dev/null | grep UBI | awk '{print $1}')
$ dd if=custom-firmware.bin of=jffs2-custom.bin count=${CUT_SECTION} bs=1
$ dd if=custom-firmware.bin of=ubi-custom.ubi skip=${CUT_SECTION} bs=1
```

4. Mount the UBI image on your host (some actions below are superfluous but harmless, we will trim them later):

```console
$ modprobe nandsim first_id_byte=0xef  \
                   second_id_byte=0xf1 \
                   third_id_byte=0x00  \
                   fourth_id_byte=0x95 \
                   parts=1,259,259,496,8
$ cat /proc/mtd | grep -i "NAND Simulator"
    ...
mtd3: 02060000 00020000 "NAND simulator partition 2" # rootfs2
    ...
$ modprobe ubi
$ flash_erase /dev/mtd3 0 259
$ nandwrite /dev/mtd3 ubi-custom.ubi
$ ubiattach -O 2048 -m 3 -d 3
$ mkdir /mnt/ubifs
$ mount -tubifs /dev/ubi3_0 /mnt/ubifs
$ cd /mnt/ubifs
```

5. Set your own `root` password

To gain root access we have to change this file in the UBI filesystem: `./etc/shadow`.
It contains the `root` password hash.
We will generate our own hash and replace it.

```console
$ YOUR_HASH=$(openssl passwd -6 -salt your_salt your_password)
$ sed -i "s#^root:.*#root:${YOUR_HASH}#" ./etc/shadow
```

6. Build back a UBI image from the filesystem:

```console
$ cd -
$ mkfs.ubifs -m 2048 -e 126976 -c 131072 -v -r /mnt/ubifs -o new-ubi.img --compr="zlib"
$ cat > ubinize.cfg << EOF
[ubifs]
mode=ubi
image=new-ubi.img
vol_id=0
vol_type=dynamic
vol_name="rootfs_ubifs"
vol_flags=autoresize
EOF
$ IMG_SEQ=$(ubireader_display_info NB6VAC-MAIN-R4.0.45d| grep -Eo "Sequence Num: .*" | sed "s/Sequence Num: //")
$ ubinize -O 2048 -p 128KiB -m 2048 -s 2048 -o new-ubi-custom.ubi -v ubinize.cfg --image-seq=${IMG_SEQ}
```

7. Merge the JFFS2 with the new UBI image:

```console
$ dd if=jffs2-custom.bin of=custom-firmware.bin bs=1 status=progress
$ dd if=new-ubi-custom.ubi of=custom-firmware.bin bs=1 seek=$(du -b jffs2-custom.bin | awk '{print $1}') status=progress
```

8. Append new correct WFI tags (use `mk-wfi` available from this repo):

```console
$ ./mk-wfi -i custom-firmware.bin -o your-firmware.bin
```

9. Clean up some files before flashing `your-firmware.bin`:

```console
$ rm jffs2-custom.bin ubi-custom.ubi new-ubi.img new-ubi-custom.ubi custom-firmware.bin
```

After flashing, you can root access your router via UART (or SSH with `dropbear`, see further below).

You can also use [this script](scripts/root-firmware-NB6VAC) that just automates this whole process (`./root-firmware-NB6VAC <firmware>`; root password = `root` by default).

# Miscellaneous

Sections here are not ordered and of various interest.

## NAND Dump

To make a full dump of your NAND:

1) `git clone https://github.com/danitool/bootloader-dump-tools && cd ./bootloader-dump-tools`
2) turn the router off and connect its serial to your machine
3) launch `python3 -m cfenand -D /dev/ttyUSB0 -O nand.bin -t 0.05 nand`
4) turn the router on

```console
$ python3 -m cfenand -D /dev/ttyUSB0 -O nand.bin -t 0.05 nand

Waiting for a prompt...

 ⠟ [1506/65536 pages] [2.9MB/128.0MB] [1.1B/s] [ETA: 15h 32m 48s]
```

- [https://github.com/danitool/bootloader-dump-tools](https://github.com/danitool/bootloader-dump-tools)


## Build back JFFS2 images from filesystem

Official firmwares come with a JFFS2 partition containing the CFE, secured since `4.0.35`, and a signed kernel.
This partition is extractable with `binwalk`.

At the moment, I implore you **not touching** the `cferam.000` file unless you know what you are doing.
And if you do, please contact me!

To repack the JFFS2 image from the filesystem:

```console
$ ORIGINAL_JFFS2_SIZE=$(du -b 0.jffs) # usually 2490368
$ mkfs.jffs2 -b -r ./_jffs2-root.extracted -o repacked-no-ubifs-marker.jffs2 -e 128 -p -n -x zlib -x lzo -x rtime --pad=$(( ORIGINAL_JFFS2_SIZE - 256 ))
$ dd if=0.jffs of=ubifs-headers bs=1 skip=$(( ORIGINAL_JFFS2_SIZE - 256 ))
$ dd if=repacked-no-ubifs-marker.jffs2 of=repacked.jffs2 bs=1 status=progress
$ dd if=ubifs-headers of=repacked.jffs2 bs=1 seek=$(du -b repacked-no-ubifs-marker.jffs2 | awk '{print $1}') status=progress
```

Notice we include UBIFS markers (announcing the UBIFS partition 256 bytes before it) at the end of the JFFS2 partition.

This crafted JFFS2 filesystem may differ from the original (`diff 0.jffs2 repacked.jffs2`) but it is working and bootable when merged back with a valid UBIFS.

## Build back UBIFS images from filesystem

Take your NAND dump and slice it according to the partition table of the CFE (`boot`,`rootfs1`,`rootfs2`,`data`,`bbt`).

1. Simulate a NAND MT-device with `nandsim`

```console
$ modprobe nandsim first_id_byte=0xef  \
                   second_id_byte=0xf1 \
                   third_id_byte=0x00  \
                   fourth_id_byte=0x95 \
                   parts=1,259,259,496,8
```

We use the NAND id for the bytes values:

```console
[    0.672000] brcmnand_read_id: CS0: dev_id=eff10095
```

2. Find out MT-devices id, and identify the one you want to mount (in my case it is going to be `rootfs1` for the emergency firmware filesystem for testing, so `mtd4`):

```console
$ cat /proc/mtd | grep -i "NAND Simulator"
mtd3: 00020000 00020000 "NAND simulator partition 0" # boot
mtd4: 02060000 00020000 "NAND simulator partition 1" # rootfs1
mtd5: 02060000 00020000 "NAND simulator partition 2" # rootfs2
mtd6: 03e00000 00020000 "NAND simulator partition 3" # data
mtd7: 00100000 00020000 "NAND simulator partition 4" # bbt
mtd8: 00020000 00020000 "NAND simulator partition 5" # flash_end (I guess?)
```

3. Load UBI kernel module

```console
$ modprobe ubifs #?
$ modprobe ubi
```

4. Erase MT-device (you may skip this step)

```console
$ flash_erase /dev/mtd3 0 1
$ flash_erase /dev/mtd4 0 259
$ flash_erase /dev/mtd5 0 259
$ flash_erase /dev/mtd6 0 496
$ flash_erase /dev/mtd7 0 8
```

<!-- 5. Flash the UBI image with ubiformat. -->
<!--  -->
<!-- The `--vid-hdr-offset` parameter is findable with `binwalk NB6VAC-MAIN-R4.0.45d`. -->
<!--  -->
<!-- ```console -->
<!-- $ ubiformat /dev/mtd3 -y -f ./_MY_DUMP.extracted/280000.ubi --vid-hdr-offset=0x800 #--sub-page-size=??? -->
<!-- ``` -->

5. Write the UBIFS image you want to modify in the MTD device:

```console
$ ./cd <path_to_sliced_firmware>
$ nandwrite /dev/mtd4 rootfs1
```

6. Attach MT-device to UBI with `ubiattach` and note the UBI device number

```console
$ ubiattach -O 2048 -m 4 -d 4 # or # ubiattach -p /dev/mtd4
UBI device number 0, total 259 LEBs (32886784 bytes, 31.3 MiB), available 1 LEBs (126976 bytes, 124.0 KiB), LEB size 126976 bytes (124.0 KiB)
```

7. Mount it with `mount`:

```console
$ mkdir /mnt/ubifs
$ mount -tubifs /dev/ubi4_0 /mnt/ubifs
```

8. Make the changes to the filesystem.

9. Create UBIFS with `mkfs.ubifs`:

```console
$ mkfs.ubifs -m 2048 -e 126976 -c 131072 -v -r /mnt/ubifs -o new-rootfs1.img --compr="zlib"
```

10. Create UBI image with `ubinize`. First genereate a config file:

```console
$ cat > ubinize.cfg << EOF
[ubifs]
mode=ubi
image=new-rootfs1.img
vol_id=0
vol_type=dynamic
vol_name="rootfs_ubifs"
vol_flags=autoresize
EOF
```

Then run:

```console
$ ubinize -O 2048 -p 128KiB -m 2048 -s 2048 -o final.ubi -v ubinize.cfg --image-seq=1558008590
```

(the `--image-seq` is copied from the one in the official firmware; see `ubireader_display_info NB6VAC-MAIN-R4.0.45d | grep Sequence`)

The resulting `final.ubi` can then be merged with the `jffs2` partition of a firmware (see my `./merge` script) and need to be added TAGS before flashing.

If we were to specify a `vol_size=30MiB` in `ubinize.cfg`, we would have the following error (followed by a kernel panic):

```console
[    1.254000] UBI error: vtbl_check: volume table check failed: record 0, error 9
[    1.262000] UBI error: ubi_init: cannot attach mtd2
...
```

It would be due to [a too big UBI image](https://wsym.tistory.com/entry/UBI-error-vtblcheck-volume-table-check-failed-record-0-error-9).*
Removing `vol_size` is the key.

[https://unix.stackexchange.com/questions/428238/how-can-i-change-a-single-file-from-an-ubi-image](https://unix.stackexchange.com/questions/428238/how-can-i-change-a-single-file-from-an-ubi-image)
[https://mytechrants.wordpress.com/2010/01/20/ubiubifs-on-nandsim/](https://mytechrants.wordpress.com/2010/01/20/ubiubifs-on-nandsim/)

## An upgrade script

The `/usr/sbin/upgrade` script seems to be called when an upgrade is available for the router (how is that checked?).

It takes two arguments :

- `$1` : `${PROTO}://${HOST}:${FILE}` (e.g. `tftp://192.168.1.100:firmware.bin`)
- `$2` : `${SHA256SUM}` (e.g. `sha256sum firmware.bin`)

Interesting functions are : `upgrade_bootloader`, `upgrade_main` and potentially `upgrade_mcu`.

Steps for an upgrade are : 

- verify CRC : `wfi-tag-verif`
    * use `qemu-system-mips -L . usr/sbin/wfi-tag-verif <path-to-firmware>` to extract infos
        * for `NB6VAC-MAIN-R4.0.45d` it outputs:
        ```console
        CRC compare Success, truncate the wfi tag
        CRC=0x23fdfc84
        BTRM=1
        LENGTH=19660800
        ```
- erase flash : `flash_eraseall -j /dev/mtd-upgrade-image`
- nandwrite from second block : `tail $1 -c +131073 | nandwrite -m -s 131072 -q /dev/mtd-upgrade-image`
- write OOB (out of band) sequence prior writing first block : `nandseq -u /dev/mtd-upgrade-image /dev/mtd-rootfs-image -c 0 -l 0`
    * _NAND flash also may contain an 'out of band (OOB) area' which usually is a fraction of the block size. This is dedicated for meta information (like information about bad blocks, ECC data, erase counters, etc.) and not supposed to be used for your actual data payload._ ([source](https://openwrt.org/docs/techref/flash))
- write first block : `head $1 -c 131072 | nandwrite -m -q /dev/mtd-upgrade-image`
- compute sha256sum to check
    ```console
	local length=$(ls -l $1 |awk '{print $5}')
	local flash=$(nanddump --bb=skipbad -l ${length} /dev/mtd-upgrade-image|sha256sum|awk '{print $1}')
	local file=$(sha256sum $1|awk '{print $1}')
	if [ "${file}" != "${flash}" ]; then
        abort ........
	fi
    ```

What is interesting is comparing the secure boot image checking from two distant versions.
For example version `R4.0.45d` and version `R4.0.16` :

```console
[...] version R4.0.45d
	# NB6VAC Secure Boot image check
	# Allowing upgrade only if OTP SECBOOT_ENABLE correspond to WFI_TAG BTRM flag
	if [ -z "$BTRM" -o "$BTRM" != "$(status get version_secureboot)" ]; then
		abort "wfi-tag doesn't match SecBoot OTP value" 6
	fi
```

```console
[...] version R4.0.16
	# NB6VAC Secure Boot image check
	# Allowing upgrade only if OTP SECBOOT_ENABLE correspond to WFI_TAG BTRM flag
	if [ -z "$BTRM" -o "$BTRM" != "$(($(iomem 0x10000480 4 |cut -d: -f2) >> 1))" ]; then
		abort "wfi-tag doesn't match SecBoot OTP value" 6
	fi
```

What's the difference between `status get version_secureboot` and `iomem 0x10000480 4 |cut -d: -f2) >> 1` ?

To be continued.

## Stored passwords

### `/etc/shadow` (high interest)

In the `4.0.45d` firmware filesystem in `/etc/shadow` we find hashes:

```console
$ cat ./etc/shadow
root:$6$4aPeJ3IPS5unYRi6$PRE.k.g30N8zSifGSD7351UFiThinMYqi8M6d79iOllC78q1LLCeppcXHF68dcV5sWFL4xXeBJPHsHrM1YUdl1:14550:0:99999:7:::
diag:$1$eJHnidSI$926kGYEiwQaUHKrLIdgRc/:13367:0:99999:7:::
fabprocess:$1$/Q5sKs4V$UdR/AJfdBKw7i4mlU.WgV1:13367:0:99999:7:::
```

| Password value | Type    |
|----------------|---------|
| `$1`           | MD5     |
| `$6`           | SHA-512 |

Decomposing `root` password:

- `$6`: SHA-512
- `$4aPeJ3IPS5unYRi6$`: the salt used to encrypt the password (chosen at random; 6 to 96 bits).
- `PRE.k.g30N8zSifGSD7351UFiThinMYqi8M6d79iOllC78q1LLCeppcXHF68dcV5sWFL4xXeBJPHsHrM1YUdl1`: encrypted hash of the unencrypted password and the salt (the salt avoids duplicate hash if users have same password).

Supposing a 6 characters password, `hashcat` would take 115 years on my machine to epxlore that surface.

```console
hashcat -m 1800 '$6$4aPeJ3IPS5unYRi6$PRE.k.g30N8zSifGSD7351UFiThinMYqi8M6d79iOllC78q1LLCeppcXHF68dcV5sWFL4xXeBJPHsHrM1YUdl1' -a 3 "?a?a?a?a?a?a"
```

This salt+SHA-512 hash was first introduced in firmware version `R4.0.40`.
Previous versions were shipping a salt+md5 hash (recognized as md5crypt by hashcat; `500 | MD5 (Unix), Cisco-IOS $1$ (MD5)`).
Downgrading to version `R4.0.39` if we could find this one would be worth it:

```console
OLD_ROOT_HASH='$1$WYM.G17C$m59xNA2lvQ2b0seQUmfGA0'
hashcat -m 500 $OLD_ROOT_HASH ...
```

As of today, I have tried every 4-char passwords (mask = `?a?a?a?a`), and 5-char password matching that mask `?1?a?a?a?a` (with `?1` = `?l?u?d`).

### Randomly found hashes

[That guy](https://github.com/Cyril-Meyer/NB6VAC-FXC/tree/main/firmware-reverse-engineering) found two SHA256 hashes investigating the firmware `NB6VAC-MAIN-R4.0.40`:

```
ff85a28634619b414f1c6e8d14c4c545a822720884536a7032e57debfbebb904
5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8
```

Here is how to find their input strings:

```console
$ wget http://downloads.skullsecurity.org/passwords/rockyou.txt.bz2
$ bzip2 -d rockyou.txt.bz2
$ hashcat -m 1400 "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8" rockyou.txt
    The password is: password
$ echo -n "password" | sha256sum
5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8  -
```

The first one is out of reach for my GPU hashing power, but the string it obfuscates is stored in clear at the beginning (line 6) of the NAND :^).
It is the sha256sum of the WiFi WPA password. 
I've found it by dumping my own router NAND.

```console
$ strings NB6VAC_DUMP.BIN | head -n6 | tail -n1
xdqf5wyg74puwj9rvt74
$ echo -n "xdqf5wyg74puwj9rvt74" | sha256sum
ff85a28634619b414f1c6e8d14c4c545a822720884536a7032e57debfbebb904  -
```

## SHA256 sum of filesystems

The root filesystem contains this file: `./ubifs-root/802285087/rootfs_ubifs/etc/file_system_checksums.sha256`.
This file contains sha256 sums of lots of files and directories of the filesystem.
I compared directories sha256 sum in specific directories with the ones stored in this files and they matched.

```console
$ FILE="./ubifs-root/802285087/rootfs_ubifs/www/fcgiroot/apid"
$ find $FILE -type f -exec sha256sum {} \;
```

I thought modifying some of those files would require to change sha256sums accordingly, but that does not seem the case.

## Execute binaries from the UBIFS without emulating

Architecture is `MIPS` and we can use `qemu` to emulate it.
Here is an example with `/bin/busybox cat` (inside the directory `rootfs_ubifs`) :

```console
$ qemu-mips -L . ./bin/busybox cat
```

Note that we need to link a library contained in the root directory of the UBIFS with `-L .`.
To check if `/bin/busybox` needs a specific library we can `readelf -d ./bin/busybox | grep NEEDED`.

## Vulnerability with `samba`?

A simple `grep -rni vulnerab` in the root filesystem brings this:

```console
$ grep -rni vulnerab
etc/init.d/samba:13:    #         don't follow symlinks. !! fix vulnerability issue !!
etc/services:265:nessus         1241/tcp                        # Nessus vulnerability
```

Might take a look inside those files.

## Provoke a kernel panic

On page `http://192.168.1.1/maintenance/system`, loading an excessively heavy file in "Importer mes paramètres/Import my parameters" provoke the following kernel panic (see [kernel_panic.log](logs/kernel_panic.log)).
And the router reboots.

## Difficult to run from CFE and TFTP

Someone from OpenWrt told me official firmwares lack initramfs, and that initramfs is useful to try firmwares on-the-fly without flashing it.
The CFE offers a `r` command to run firmwares on RAM.
I have turned that in every way but always failed.
Someone?

## About Secure Boot

As of today, I am not sure how secure boot works hardware-speaking, but we can read about a common implementation on [Silicon Lab](https://www.silabs.com/security/secure-boot)'s website:

> A common implementation of Secure Boot consists of storing the public key used for code authentication into one-time programmable memory. As the public key becomes irreversible, only code signed with the corresponding private key can be authenticated and executed. Silicon Labs enhanced Secure Boot implementation is called Secure Boot with Root of Trust and Secure Loader (RTSL). Secure Boot with RTSL takes additional steps by following a full chain of trust process. With a dual core architecture, the process starts at the secure element. The code starts from secure immutable ROM and confirms authenticity of the first stage bootloader. It is also checks for updates via a secure loader. Once the secure element is fully verified and available, the second core initiates the second stage authentication and updates are applied, if required. In the final stage, the second stage bootloader checks, updates (if applicable) and authenticates the application code.

I am not sure this is the way our device is conceived, but I have high priors about that hypothesis.
Interestingly, Silicon Labs is the manufacturer of our device's MCU (OTP MCU C8051T634; if someone wants to check [its datasheet](https://eu.mouser.com/datasheet/2/368/C8051T63x-1397986.pdf)).

So, what probably bothers us about secure boot is we cannot change the first stage bootloader (contained in `cferam.000`) because it is authentified by the ROM.

Completely bypassing the authentication seems the more obvious road to take, but I have found no evidence it is easily feasible.
Resetting/modifying the OTP appears to be necessary.
If we could place our own public key in the OTP (resetting them appears to be doable; read further), then we could sign our own bootloader with our matching private key.

Here are additionnal notes about CFE Secure Boot from is what [OpenWrt](https://openwrt.org/docs/techref/bootloader/cfe):

> 1. The SoC has as factory settings, most probably in the OTP fuses, the private key unique per each model and also 2 keys AES CBC (ek & iv). This is the Root of Trust which is known by OEM.

> 2. During boot, the PBL (Primary Boot Loader coded in the SoC) will search for storage peripherals e.g. NAND or NOR SPI. If found then loads a small portion from start of storage into memory. Exact amount may depend on model and storage but most typically 64kb. In the sources this chunk is called CFEROM.

> 3. Once loaded the CFEROM, the PBL will analyse the structure, which is a compound of different chunks: valid header, magic numbers, signed credentials, CRC32, actual compiled code, etc. In the end, the PBL will decide if CFEROM meets the structure required and it is properly signed. If this is so, then the PBL will execute the compiled code encapsulated. Note that this code is usually not encrypted and therefore can be detected with naked eyes.

> 4. Typically, CFEROM will start PLL's and full memory span. Most probably doesn't need to run a storage driver since it is already working. Then it will jump to CFERAM location as coded

> 5. CFERAM binary is encoded in JFFS2 filesystem. It must meet a certain structure as CFEROM. The compiled code is usually LZMA compressed and AES CBC encrypted, rendering the resulting binary absolutely meaningless.


Reading what the content of our C8051T634 could be doable according to [that exchange](https://electronics.stackexchange.com/questions/198274/storing-a-secure-key-in-an-embedded-devices-memory):

> QUESTION (truncated):
    [I could] Generate keys and stored them in the flash memory in programming MCU. MCU flash memory's support CRP (code read protection) which prevent from code mining and with assist of its internal AES engine and RNG (random number generation) engine we can make a random key and encrypt flash memory and stored that random key in the OTP (one time programmable memory -a 128 bit encrypted memory), then in code execution we decode flash memory with RNG key and access to initial key and codes. Disadvantage: Keys stored in a non volatile memory, tampers will be useless and attacker have a lot of time to mine keys.

> ANSWER (truncated):
    Don't store keys in nonvolatile memory, you are correct on this. It doesn't matter if you protect the EEPROM or flash memory from being read. That code read protection fuse is easily reversed. An attacker need only decap (remove or chemically etch away the black epoxy packaging to expose the silicon die inside). At this point, they can cover up the part of the die that is non volatile memory cells (these sections are very regular and while individual memory cells are much to small to be seen, the larger structure can be) and a small piece of something opaque to UV is masked over that section. Then the attacker can just shine a UV light on the chip for 5-10 minutes, and reset all the fuses, including the CRP fuse. The OTP memory can now be read by any standard programmer. 

From naive perspective, the best approach is to start reading the [not-so-long datasheet](https://www.keil.com/dd/docs/datashts/silabs/c8051t63x.pdf) 🥲.
You can also take a look at the front [circuit board picture](assets/front_nb6vac-fxc-r0.png) (from a FXC-r0 but it uses the same chip).

- [https://electronics.stackexchange.com/questions/198274/storing-a-secure-key-in-an-embedded-devices-memory](https://electronics.stackexchange.com/questions/198274/storing-a-secure-key-in-an-embedded-devices-memory)
- [https://www.keil.com/dd/docs/datashts/silabs/c8051t63x.pdf](https://www.keil.com/dd/docs/datashts/silabs/c8051t63x.pdf)
- [https://reversepcb.com/what-is-one-time-programmable-memory/](https://reversepcb.com/what-is-one-time-programmable-memory/)
- [https://www.silabs.com/security/secure-boot](https://www.silabs.com/security/secure-boot)


## Hidden interface pages

Just in case, as reported [here](https://github.com/Cyril-Meyer/NB6VAC-FXC) the router's interface has some hidden but accessible pages:

- [http://192.168.1.1/network/lan](http://192.168.1.1/network/lan)
- [http://192.168.1.1/state/lan/extra](http://192.168.1.1/state/lan/extra)
- [http://192.168.1.1/rootfs](http://192.168.1.1/rootfs)
- [http://192.168.1.1/state/device/plug](http://192.168.1.1/state/device/plug)
- [http://192.168.1.1/maintenance/dsl/config](http://192.168.1.1/maintenance/dsl/config)

The first one can be useful to change router's IP.

## Available firmwares

Here are available firmwares for the NB6VAC (NB6VAC-MAIN-R4.x.xx).
Please note the announced bootloader **is not** necessarily installed along its the firmware it is attached to in this list.
On a rooted router, you can check your bootloader version with `status get version_bootloader`.
Also note that I had a bad experience with downgrading but haven't troubleshooted yet, so unless you know what you are doing, **do not downgrade**.

| Firmware             | Bootloader                     |
|:--------------------:|:------------------------------:|
| NB6VAC-MAIN-R4.0.16  | NB6VAC-BOOTLOADER-R4.0.6       |  
| NB6VAC-MAIN-R4.0.17  | NB6VAC-BOOTLOADER-R4.0.6       |  
| NB6VAC-MAIN-R4.0.18  | NB6VAC-BOOTLOADER-R4.0.8       |  
| NB6VAC-MAIN-R4.0.19  | NB6VAC-BOOTLOADER-R4.0.8       |  
| NB6VAC-MAIN-R4.0.20  | NB6VAC-BOOTLOADER-R4.0.8       |  
| NB6VAC-MAIN-R4.0.21  | NB6VAC-BOOTLOADER-R4.0.8       |  
| NB6VAC-MAIN-R4.0.35  | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.39  | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.40  | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.41  | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.42  | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.43  | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44b | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44c | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44d | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44e | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44f | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44i | NB6VAC-BOOTLOADER-R4.1.0-PROD01|        
| NB6VAC-MAIN-R4.0.44j | NB6VAC-BOOTLOADER-R4.1.0-PROD01|       
| NB6VAC-MAIN-R4.0.44k | NB6VAC-BOOTLOADER-R4.1.0-PROD01| 
| NB6VAC-MAIN-R4.0.45  | NB6VAC-BOOTLOADER-R4.1.0-PROD01| 
| NB6VAC-MAIN-R4.0.45c | NB6VAC-BOOTLOADER-R4.1.0-PROD01| 
| NB6VAC-MAIN-R4.0.45d | NB6VAC-BOOTLOADER-R4.1.0-PROD01| 
| NB6VAC-MAIN-R4.0.46  | NB6VAC-BOOTLOADER-R4.1.0-PROD01| 

To download one of them :

```console
$ FIRMWARE_VERSION="4.0.19"
$ wget -O firmware "https://ncdn.nb4dsl.neufbox.neuf.fr/nb6vac_Version%20${FIRMWARE_VERSION}/NB6VAC-MAIN-R${FIRMWARE_VERSION}" --no-check-certificate
$ BOOTLOADER_VERSION="4.0.6"
$ wget -O bootloader "https://ncdn.nb4dsl.neufbox.neuf.fr/nb6vac_Version%20${FIRMWARE_VERSION}/NB6VAC-BOOTLOADER-R${BOOTLOADER_VERSION}" --no-check-certificate
```

(TODO: release date + DSL firmware...)

- [https://web.archive.org/web/20220228112552/https://www.vincentalex.fr/neufbox4.org/forum/viewtopic.php?id=2981](https://web.archive.org/web/20220228112552/https://www.vincentalex.fr/neufbox4.org/forum/viewtopic.php?id=2981)

# WFI tags and CRC

Official firmwares come with WFI tags ("whole flash image" tags).
Those tags are used for the [CRC32 check](https://github.com/Michaelangel007/crc32): if they don't match the content of the firmware, flash is aborted.
WFI tags are located at the end of the firmware file (last `20` bytes), and are read and extracted (i.e. removed) before flashing.

So, WFI tags serve to authentify a firmware image (either by the CFE bootloader, either by `/usr/sbin/upgrade` or `/etc/init.d/firmware`).

The CRC32 table is hardcoded in `cferam.000` and is easily extractable with `binwalk`. 

# References

- [https://github.com/digiampietro/hacking-gemtek](https://github.com/digiampietro/hacking-gemtek)
- [https://www.coresecurity.com/core-labs/articles/next-generation-ubi-and-ubifs](https://www.coresecurity.com/core-labs/articles/next-generation-ubi-and-ubifs)
- [https://piped.video/watch?v=Ai8J3FG8kys](https://piped.video/watch?v=Ai8J3FG8kys) 
- [https://stackoverflow.com/questions/33853333/compiling-the-linux-kernel-for-mips](https://stackoverflow.com/questions/33853333/compiling-the-linux-kernel-for-mips)
- [https://github.com/danitool/bootloader-dump-tools](https://github.com/danitool/bootloader-dump-tools)
- [https://jg.sn.sg/ontdump/](https://jg.sn.sg/ontdump/)
- [https://github.com/Michaelangel007/crc32](https://github.com/Michaelangel007/crc32)
- [https://www.batronix.com/shop/electronic/eprom-programming.html](https://www.batronix.com/shop/electronic/eprom-programming.html)
