cmake_minimum_required(VERSION 3.16)

project(DinoRun VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appDinoRun
    main.cpp 
    GameManager.h GameManager.cpp
)

set_target_properties(appDinoRun PROPERTIES
   QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")

qt_add_qml_module(appDinoRun
    URI DinoRun
    QML_FILES
        Main.qml
    RESOURCES
            images/dinosaur1.png
            images/dinosaur2.png
            images/dinosaur3.png
            images/dinosaur4.png
            images/dinosaur5.png
            images/dinosaur6.png
            images/dinosaur7.png
            images/dinosaur8.png
            images/dinosaur9.png
            images/cactus1.png
            images/cactus2.png
            images/cactus3.png
            images/cactus4.png
            images/cactus5.png
            images/cactus6.png
            images/pterosaurs1.png
            images/pterosaurs2.png
            images/moon1.png
            images/moon2.png
            images/moon3.png
            images/moon4.png
            images/moon5.png
            images/moon6.png
            images/moon7.png
            images/cloud.png
            sounds/bgm.mp3
            sounds/jump.wav
            sounds/gameStart.wav
            sounds/gameOver.wav
            sounds/button.wav
            sounds/score.wav
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appDinoRun PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appDinoRun
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appDinoRun
    PRIVATE Qt6::Quick
            Qt6::Multimedia
)

install(TARGETS appDinoRun
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
