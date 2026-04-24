SUMMARY = "Thin convenience wrapper over the Linux gpio-v2 chardev interface"
HOMEPAGE = ""
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "https://github.com/cozzyd/libgpios;protocol=https;branch=v0.1.x"
SRCREV = "543bd16a9c602a394cc1e8d82f034eb17a3d49d0"

S = "${WORKDIR}/git"

# Pass Yocto's CFLAGS through
EXTRA_OEMAKE = "CFLAGS='${CFLAGS} -Iinclude -fPIC'"


do_install() {
    oe_runmake install DESTDIR=${D} PREFIX=${prefix}
}

PACKAGES =+ "${PN}-examples"

FILES:${PN}          = "${libdir}/libgpios.so"
FILES:${PN}-dev      = "${includedir}/libgpios.h"
FILES:${PN}-examples = "${bindir}/gpios-get ${bindir}/gpios-set"
