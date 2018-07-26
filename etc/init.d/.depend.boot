TARGETS = mountkernfs.sh hostname.sh udev mountdevsubfs.sh hwclock.sh networking rpcbind nfs-common mountall.sh mountall-bootclean.sh urandom mountnfs.sh mountnfs-bootclean.sh checkroot.sh bootmisc.sh checkroot-bootclean.sh udev-finish kmod procps checkfs.sh
INTERACTIVE = udev checkroot.sh checkfs.sh
udev: mountkernfs.sh
mountdevsubfs.sh: mountkernfs.sh udev
hwclock.sh: mountdevsubfs.sh
networking: mountkernfs.sh mountall.sh mountall-bootclean.sh urandom procps
rpcbind: networking mountall.sh mountall-bootclean.sh
nfs-common: rpcbind hwclock.sh
mountall.sh: checkfs.sh checkroot-bootclean.sh
mountall-bootclean.sh: mountall.sh
urandom: mountall.sh mountall-bootclean.sh hwclock.sh
mountnfs.sh: mountall.sh mountall-bootclean.sh networking rpcbind nfs-common
mountnfs-bootclean.sh: mountall.sh mountall-bootclean.sh mountnfs.sh
checkroot.sh: hwclock.sh mountdevsubfs.sh hostname.sh
bootmisc.sh: mountall-bootclean.sh mountnfs-bootclean.sh checkroot-bootclean.sh mountall.sh mountnfs.sh udev
checkroot-bootclean.sh: checkroot.sh
udev-finish: udev mountall.sh mountall-bootclean.sh
kmod: checkroot.sh
procps: mountkernfs.sh mountall.sh mountall-bootclean.sh udev
checkfs.sh: checkroot.sh
