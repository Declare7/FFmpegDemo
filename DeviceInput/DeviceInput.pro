CONFIG -= qt

TEMPLATE = lib
DEFINES += DEVICEINPUT_LIBRARY

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    DeviceInput.cpp

HEADERS += \
    DeviceInput_global.h \
    DeviceInput.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target


#--------------------指定目标名称-----------------------------------------------------------------
#CONFIG(debug, debug|release){
#    AFTER_FIX = d
#    TARGET = $$TARGET$$AFTER_FIX
#}else{

#}
#-----------------------------------------------------------------------------------------------

#------------------设置程序输出路径----------------------------------------------------------------
CONFIG(debug, debug|release){

    DESTDIR = $$PWD/../Build/Debug

}else{

    DESTDIR = $$PWD/../Build/Release

}
#-------------------------------------------------------------------------------------------------

#-----------------复制依赖文件到输出路径-------------------------------------------------------------
win32{

    SRC_DIR = $$PWD/*.h
    DST_DIR = $$DESTDIR/Include/
    # Replace slashes in paths with backslashes for Windows
    SRC_DIR ~= s,/,\\,g
    DST_DIR ~= s,/,\\,g

    QMAKE_POST_LINK += "xcopy /y $$SRC_DIR $$DST_DIR" &&

}
#-------------------------------------------------------------------------------------------------


#-----------------依赖环境-------------------------------------------------------------------------
win32{

    LIBS += -L$$PWD/../Libs/Windows/ffmpeg6/lib/ -lavdevice \
            -L$$PWD/../Libs/Windows/ffmpeg6/lib/ -lavformat

    INCLUDEPATH += $$PWD/../Libs/Windows/ffmpeg6/include
    DEPENDPATH += $$PWD/../Libs/Windows/ffmpeg6/include

}
#-------------------------------------------------------------------------------------------------



#-----------------复制依赖库到输出路径---------------------------------------------------------------
win32{

    SRC_DIR = $$PWD/../Libs/Windows/ffmpeg6/bin
    DST_DIR = $$DESTDIR/
    # Replace slashes in paths with backslashes for Windows
    SRC_DIR ~= s,/,\\,g
    DST_DIR ~= s,/,\\,g

    QMAKE_POST_LINK += "xcopy /y $$SRC_DIR\\avformat-60.dll $$DST_DIR" && \
                        "xcopy /y $$SRC_DIR\\avdevice-60.dll $$DST_DIR"
}
#-------------------------------------------------------------------------------------------------
