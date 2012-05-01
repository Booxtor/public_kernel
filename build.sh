#!/bin/bash

set -e

kernel_config=onyx_defconfig
initramfs_config=onyx_initramfs_defconfig
init_config=default

while [ $# -gt 0 ]
do
    case $1
        in
        --config-prefix)
            CONFIG_PREFIX=$2
            kernel_config="${CONFIG_PREFIX}_defconfig"
            initramfs_config="${CONFIG_PREFIX}_initramfs_defconfig"
            shift 2
            ;;

        --init-profile)
            INIT_PROFILE=$2
            shift 2
            ;;

        --memory-size)
            MEM_SIZE=$2
            shift 2
            ;;

        *)
            echo "Invalid arguement: $1"
            echo "Valid arguments are"
            echo "--config-prefix prefix: Prefix of the kernel config files."
            echo "--aes-password password: The password used to encrypt update packages."
            echo "--init-config name: The configuration to prepend to the init script."
            exit 1
            ;;
    esac
done

echo "Using kernel config: $kernel_config"
echo "Using initramfs config: $initramfs_config"
echo "Using password: $PASSWORD"

SCRIPT_PATH=`readlink -f $0`
REPO_DIR=`dirname "${SCRIPT_PATH}"`
echo "Repository path: ${REPO_DIR}"
export KERNEL_DIR="$REPO_DIR/linux"
KERNEL_IMAGE="$KERNEL_DIR/arch/arm/boot/uImage"
ROOTFS_DIR="$REPO_DIR/rootfs_patch"

# Configure options.
export ONYX_SDK_ROOT=/opt/onyx/arm
export PATH=$PATH:$ONYX_SDK_ROOT/bin:/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin
HOST=arm-linux
BUILD=i686

GUESSED_BRANCH=`cd "${REPO_DIR}" && git branch | grep '^\*' | cut -d ' ' -f 2`
GIT_BRANCH=${GIT_BRANCH-master}
BRANCH=${GIT_BRANCH-${GUESSED_BRANCH}}
echo "Building branch: $BRANCH"

if [ ! -d "$REPO_DIR" ]; then
    echo "$REPO_DIR does not exist."
    exit 1
fi

sed "s|REPO_DIR|$REPO_DIR|" "$KERNEL_DIR/arch/arm/configs/${initramfs_config}.in" \
    > "$KERNEL_DIR/arch/arm/configs/${initramfs_config}"

# Check init's profile
if [ "$INIT_PROFILE" != "" ] && [ "$INIT_PROFILE" != "none" ]; then
    echo "$INIT_PROFILE detected, use this profile."
    cp -f "$REPO_DIR"/onyx/initramfs/profiles/$INIT_PROFILE/init "$REPO_DIR"/onyx/initramfs/
fi

cd "$REPO_DIR"/linux/usr && make gen_init_cpio
cd "$REPO_DIR"/onyx/initramfs && make clean
cd "$REPO_DIR"/onyx/initramfs && make all n=3 s=PASSWORD\=\"`cat ${PASSWORD}_PASS`\"

rm -rf "$ROOTFS_DIR"
mkdir "$ROOTFS_DIR"

# Build uImage (with and without initramfs)
cd "$KERNEL_DIR"
ARCH=arm CROSS_COMPILE=arm-linux- make $initramfs_config
ARCH=arm CROSS_COMPILE=arm-linux- make uImage LINUX_PRODUCT=$PASSWORD
cp "$KERNEL_IMAGE" "$REPO_DIR/uImage-initramfs-$PASSWORD"
ARCH=arm CROSS_COMPILE=arm-linux- make $kernel_config
ARCH=arm CROSS_COMPILE=arm-linux- make uImage LINUX_PRODUCT=$PASSWORD
cp "$KERNEL_IMAGE" "$REPO_DIR/uImage-$PASSWORD"

# Build kernel modules, and put them into rootfs
ARCH=arm CROSS_COMPILE=arm-linux- make modules
ARCH=arm CROSS_COMPILE=arm-linux- make modules_install INSTALL_MOD_PATH=$ROOTFS_DIR

# Build Atheros WiFi driver
cd $REPO_DIR/AR6kSDK.build_3.1_RC.667/host
make clean
make ATH_LINUXPATH=$KERNEL_DIR
make install ATH_LINUXPATH=$KERNEL_DIR INSTALL_MOD_PATH=$ROOTFS_DIR

# Build u-boot
u_boot_config=mx50_arm2_config
if [ ! -z  `expr $CONFIG_PREFIX : '\(.*\)hd_pvi'` ]; then
  u_boot_config=mx50_arm2_128_60_hd_pvi_config
  if [ ! -z $MEM_SIZE ] ; then
      u_boot_config=mx50_arm2_${MEM_SIZE}_60_hd_pvi_config
  fi
elif [ ! -z  `expr $CONFIG_PREFIX : '\(.*\)hd'` ]; then
  u_boot_config=mx50_arm2_128_60_hd_config
elif [ ! -z  `expr $CONFIG_PREFIX : '\(.*\)97'` ]; then
  u_boot_config=mx50_arm2_256_97_config
elif [ ! -z  `expr $CONFIG_PREFIX : '\(.*\)60'` ]; then
  u_boot_config=mx50_arm2_128_60_config
elif [ ! -z  `expr $CONFIG_PREFIX : '\(.*\)ir_a'` ]; then
  u_boot_config=mx50_arm2_128_60_hd_pvi_config
fi

cd $REPO_DIR/u-boot-2009.08
echo "Using uboot config: $u_boot_config"
make $u_boot_config
make
