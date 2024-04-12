| NOTE                       |
|:---------------------------|
| _You probably don't have to touch anything from here._ |

# Corrective patch

Compiling Kernel 3.4.11 with Binutils 2.24 may output errors about some 'clear_page' and 'copy_page' functions missing.

Applying [that patch](https://github.com/torvalds/linux/commit/c022630633624a75b3b58f43dd3c6cc896a56cff?diff=unified&w=0) like suggested [here](https://www.spinics.net/lists/mips/msg63437.html) works.

```console
$ wget -O clear-page-copy-page.patch https://github.com/torvalds/linux/commit/c022630633624a75b3b58f43dd3c6cc896a56cff.patch
$ cd ./buildroot-2015.02/output/build/linux-3.4.11/arch/mips
$ patch -p0 < ../../../../../../patches/clear-page-copy-page.patch
```

If for whatever reason `patch` command does not work, do this:

```console
BUILDROOT_PATCH_DIR="../buildroot-2015.02/output/build/linux-3.4.11/arch/mips"
cp mips_ksyms.c ${BUILDROOT_DIR}/kernel/
cp Makefile     ${BUILDROOT_DIR}/mm/
cp page.c       ${BUILDROOT_DIR}/mm/
cp page-funcs.S ${BUILDROOT_DIR}/mm/
```

# Kernel patch

Buildroot installs kernel 3.4.11, but I want to be as close as possible to the router system with kernel [3.4.11-rt19](https://lwn.net/Articles/516691/) (real-time patch).
This is automatically applied when using `./generate-images`.
If for whatever reason you need to apply this patch manually, download it here:

```console
$ wget https://ftp.riken.jp/Linux/kernel.org/linux/kernel/projects/rt/3.4/older/patch-3.4.11-rt19.patch.bz2
```
