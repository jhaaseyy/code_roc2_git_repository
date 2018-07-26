TARGETS = mountall.sh mountall-bootclean.sh mountnfs.sh mountnfs-bootclean.sh checkroot.sh urandom kmod checkroot-bootclean.sh bootmisc.sh checkfs.sh udev-finish procps
INTERACTIVE = checkroot.sh checkfs.sh
mountall.sh: checkfs.sh checkroot-bootclean.sh
mountall-bootclean.sh: mountall.sh
mountnfs.sh: mountall.sh mountall-bootclean.sh
mountnfs-bootclean.sh: mountall.sh mountall-bootclean.sh mountnfs.sh
urandom: mountall.sh mountall-bootclean.sh
kmod: checkroot.sh
checkroot-bootclean.sh: checkroot.sh
bootmisc.sh: checkroot-bootclean.sh mountall-bootclean.sh mountall.sh mountnfs.sh mountnfs-bootclean.sh
checkfs.sh: checkroot.sh
udev-finish: mountall.sh mountall-bootclean.sh
procps: mountall.sh mountall-bootclean.sh
