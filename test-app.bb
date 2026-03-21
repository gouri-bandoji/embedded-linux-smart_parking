SUMMARY = "User applications for smart parking system"
DESCRIPTION = "Applications to interface with the smart parking kernel module"
SECTION = "applications"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://test_app.c \
           file://user_app.c \
           file://Makefile"

# Use standard WORKDIR
S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

DEPENDS += "libgpiod"

TARGET_CC_ARCH = "-march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard"
CFLAGS:append = " ${TARGET_CC_ARCH}"
LDFLAGS:append = " ${TARGET_CC_ARCH} -Wl,--hash-style=gnu -Wl,--as-needed"

inherit pkgconfig

do_compile() {
    oe_runmake CC="${CC}" CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}"
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/user_app ${D}${bindir}/
    install -m 0755 ${B}/test_app ${D}${bindir}/

    # Remove unnecessary debug files
    rm -rf ${D}${bindir}/.debug
}

FILES:${PN} = "${bindir}/user_app ${bindir}/test_app"

INSANE_SKIP:${PN} += "ldflags"
