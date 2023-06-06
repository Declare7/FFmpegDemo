QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    VideoInputUI.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    MainWindow.h \
    VideoInputUI.h

FORMS += \
    MainWindow.ui \
    VideoInputUI.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


#------------------设置程序输出路路径----------------------------------------------------------------
CONFIG(debug, debug|release){

    DESTDIR = $$PWD/../Build/Debug

}else{

    DESTDIR = $$PWD/../Build/Release

}
#--------------------------------------------------------------------------------------------------


#------------------所依赖的子项目库------------------------------------------------------------------
win32{
    LIBS += -L$$DESTDIR/ -lVideoInput

    INCLUDEPATH += $$DESTDIR/Include
    DEPENDPATH += $$DESTDIR
}
#--------------------------------------------------------------------------------------------------
