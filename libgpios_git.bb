SUMMARY = "Thin convenience wrapper over the Linux gpio-v2 chardev interface"
HOMEPAGE = ""
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "https://github.com/cozzyd/libgpios;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

# Pass Yocto's CFLAGS through
EXTRA_OEMAKE = ""


do_install() {
    oe_runmake install DESTDIR=${D} PREFIX=${prefix}
}

PACKAGES =+ "${PN}-examples"

FILES:${PN}          = "${libdir}/libgpios.so"
FILES:${PN}-dev      = "${includedir}/libgpios.h"
FILES:${PN}-examples = "${bindir}/gpios-get ${bindir}/gpios-set"
