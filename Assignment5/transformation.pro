TEMPLATE = app
TARGET = transformation

QT += widgets core gui
CONFIG += c++17
CONFIG += warn_on
macx: CONFIG += app_bundle

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    my_label.cpp

HEADERS += \
    mainwindow.h \
    my_label.h

FORMS += \
    mainwindow.ui

# Make headers in the source dir visible to uic-generated files
INCLUDEPATH += .
