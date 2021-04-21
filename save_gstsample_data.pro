QT += core
QT -= gui
QT += sql

ROOTFS_PATH = /home/tx2/tegra_tx2/nvidia/nvidia_sdk/JetPack_4.4_DP_Linux_DP_JETSON_TX2/Linux_for_Tegra/rootfs

CONFIG += c++11 console
CONFIG -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp

INCLUDEPATH += $${ROOTFS_PATH}/usr/include
INCLUDEPATH += $${ROOTFS_PATH}/usr/include/gstreamer-1.0/
INCLUDEPATH += $${ROOTFS_PATH}/usr/include/gstreamer-1.0/gst/good
INCLUDEPATH += $${ROOTFS_PATH}/usr/include/glib-2.0
INCLUDEPATH += $${ROOTFS_PATH}/usr/lib/aarch64-linux-gnu/glib-2.0/include
INCLUDEPATH += $${ROOTFS_PATH}/usr/include/include

LIBS += -L$${ROOTFS_PATH}/usr/lib/aarch64-linux-gnu \
        -lglib-2.0 -lgobject-2.0 -lgstapp-1.0

LIBS += -L$${ROOTFS_PATH}/usr/lib/aarch64-linux-gnu -lgstrtspserver-1.0

LIBS += -L$${ROOTFS_PATH}/usr/local/gst_1.18.1/lib/aarch64-linux-gnu \
        -lgstreamer-1.0 -lgstvideo-1.0 -lgstrtp-1.0 -lgstrtsp-1.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/nvidia/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#HEADERS +=
#    global.h \
#    thread/*.h
