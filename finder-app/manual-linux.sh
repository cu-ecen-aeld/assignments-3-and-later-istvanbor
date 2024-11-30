#!/bin/bash
# Script to install and build kernel.
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
    # Clone only if the repository does not exist.
    echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
    git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Kernel build steps
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# Make and install busybox
make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

set -e

# Path to the cross-compiler sysroot
SYSROOT_PATH=$(${CROSS_COMPILE}gcc -print-sysroot)

# Path to the busybox binary in the root filesystem
BUSYBOX_BIN="${OUTDIR}/rootfs/bin/busybox"

# Check if busybox exists in the root filesystem
if [ ! -f "$BUSYBOX_BIN" ]; then
    echo "Error: busybox not found in the root filesystem at ${BUSYBOX_BIN}"
    exit 1
fi

echo "Library dependencies"

# 1. Find the program interpreter used by busybox
PROGRAM_INTERPRETER=$(${CROSS_COMPILE}readelf -l "${BUSYBOX_BIN}" | \
    sed -n 's/.*Requesting program interpreter: \(.*\)/\1/p' | \
    xargs basename | tr -d '[]')

# Print the program interpreter
echo "Program Interpreter: ${PROGRAM_INTERPRETER}"

# 2. Locate the program interpreter in the sysroot and copy it
PROGRAM_INTERPRETER_PATH=$(find "${SYSROOT_PATH}" -name "${PROGRAM_INTERPRETER}")

if [[ -n "$PROGRAM_INTERPRETER_PATH" ]]; then
    echo "Found program interpreter at ${PROGRAM_INTERPRETER_PATH}"
    cp "$PROGRAM_INTERPRETER_PATH" "${OUTDIR}/rootfs/lib/"
else
    echo "Error: Program interpreter not found in sysroot!"
    exit 1
fi

# 3. Extract the list of shared libraries required by busybox
mapfile -t SHARED_LIBRARIES < <(${CROSS_COMPILE}readelf -d "${BUSYBOX_BIN}" | \
    awk '/NEEDED/ {print $5}' | tr -d '[]')

echo "Shared libraries required by busybox:"
for lib in "${SHARED_LIBRARIES[@]}"; do
    echo " - ${lib}"
done

# 4. Copy each shared library into the appropriate location in the root filesystem
for lib in "${SHARED_LIBRARIES[@]}"; do
    echo "Searching for $lib in $SYSROOT_PATH..."

    # Find the library in the sysroot
    lib_path=$(find "$SYSROOT_PATH" -name "$lib" | head -n 1)

    if [[ -n "$lib_path" ]]; then
        echo "Found $lib at $lib_path"
        
        # Copy the library to lib64 (or lib) directory in the root filesystem
        cp "$lib_path" "${OUTDIR}/rootfs/lib64/"
        echo "Copied $lib to ${OUTDIR}/rootfs/lib64/"
    else
        echo "Error: Library $lib not found in sysroot!"
        exit 1
    fi
done

echo "Library dependencies have been copied successfully."

# Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp writer ${OUTDIR}/rootfs/home/

# Copy the finder related scripts and executables
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home/
mkdir -p "${OUTDIR}/rootfs/home/conf"
cp ${FINDER_APP_DIR}/autorun-qemu.sh "${OUTDIR}/rootfs/home/"

# Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs

# Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

