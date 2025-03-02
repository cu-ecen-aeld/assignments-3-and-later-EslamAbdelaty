#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
# CROSS_COMPILE_PATH=/home/eslama/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc
CROSS_COMPILE_BIN=$(which aarch64-none-linux-gnu-gcc)
CROSS_COMPILE_PATH=$(dirname $(dirname $(dirname $CROSS_COMPILE_BIN)))/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc
echo "CROSS_COMPILE_PATH: $CROSS_COMPILE_PATH"
# Add cross-compiler to PATH
export PATH=/home/eslama/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin:$PATH

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "clear .config"
    # Clear .config
    echo "Clearing .config"
    if [ -e .config ]; then
        make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    fi

    echo "make defconfig"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    if [ ! -e .config ]; then
        echo "Error: .config file not found. Trying to generate it again."
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    fi

    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all


fi

echo "Adding the Image in outdir"
cp -rl ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp var usr 
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig

    else
    cd busybox
fi

echo "Make and install busybox"
# TODO: Make and install busybox
# Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

# Verify the busybox binary
if [ -f "${OUTDIR}/rootfs/bin/busybox" ]; then
    echo "BusyBox binary found at ${OUTDIR}/rootfs/bin/busybox"
else
    echo "Error: BusyBox binary not found. Check the build and installation steps."
    exit 1
fi


echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Library dependencies"
# Find the program interpreter (dynamic linker) required by busybox
PROGRAM_INTERPRETER=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter" | awk -F'[][]' '{print $2}' | awk -F': ' '{print $2}')
echo "Program interpreter: ${PROGRAM_INTERPRETER}"

# Find the shared libraries required by busybox
SHARED_LIBS=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library" | awk -F'[][]' '{print $2}')
echo "Shared libraries: ${SHARED_LIBS}"

# Copy the program interpreter to the root filesystem
if [ -n "${PROGRAM_INTERPRETER}" ]; then
    echo "Copying program interpreter: ${PROGRAM_INTERPRETER}"
    echo "${PROGRAM_INTERPRETER}"
    mkdir -p ${OUTDIR}/rootfs$(dirname ${PROGRAM_INTERPRETER})
    cp ${CROSS_COMPILE_PATH}${PROGRAM_INTERPRETER} ${OUTDIR}/rootfs${PROGRAM_INTERPRETER}
else
    echo "Error: Program interpreter not found!"
    exit 1
fi

# Copy shared libraries to the root filesystem
for LIB in ${SHARED_LIBS}; do
    echo "Copying shared library: ${LIB}"
    mkdir -p ${OUTDIR}/rootfs$(dirname ${LIB})
    cp ${CROSS_COMPILE_PATH}/lib64/${LIB} ${OUTDIR}/rootfs/lib64
done


# TODO: Make device nodes
# Make device nodes
sudo mkdir -p ${OUTDIR}/rootfs/dev
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty c 5 0
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/random c 1 8
# TODO: Clean and build the writer utility

cd $FINDER_APP_DIR

make clean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home
# Ensure the conf directory exists in the rootfs
mkdir -p ${OUTDIR}/rootfs/home/conf
# Copy the conf directory without hard links
cp -r ${FINDER_APP_DIR}/conf/* ${OUTDIR}/rootfs/home/conf/
# cp -rl ${FINDER_APP_DIR}/conf/assignment.txt  ${OUTDIR}/rootfs/home
# TODO: Chown the root directory

echo " Chown the root directory"
sudo chown -R root:root ${OUTDIR}/*
cd ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip ${OUTDIR}/initramfs.cpio
