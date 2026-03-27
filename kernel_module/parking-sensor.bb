SUMMARY = "Smart Parking Kernel Module"
DESCRIPTION = "A Linux kernel module for managing parking slots with ultrasonic sensors and LEDs"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

# Source files
SRC_URI = " \
    file://parking-sensor.c \
    file://Makefile \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

# Use Bitbake's module class
inherit module

# Compile the kernel module
do_compile() {
    oe_runmake -C ${STAGING_KERNEL_DIR} M=${S} modules
}

# Install the compiled kernel module
do_install() {
    install -d ${D}${base_libdir}/modules/${KERNEL_VERSION}/extra/
    install -m 0644 ${S}/parking-sensor.ko ${D}${base_libdir}/modules/${KERNEL_VERSION}/extra/
}

# Ensure the module is loaded at boot
KERNEL_MODULE_AUTOLOAD += "parking-sensor"
