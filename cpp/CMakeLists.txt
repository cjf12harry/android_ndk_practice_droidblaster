# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Enable verbose mode")
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

include_directories(Libraries)
add_subdirectory(Libraries/libpng)

# Include the box 2d library
set(BOX2D_INSTALL OFF CACHE BOOL "Not install box2d")
set(BOX2D_BUILD_SHARED OFF CACHE BOOL "Not building shared lib")
set(BOX2D_BUILD_STATIC ON CACHE BOOL "Building static 2d")
add_subdirectory(Libraries/Box2D)



add_library(nativedroidblaster SHARED
        Log.cpp
        EventLoop.cpp
        Main.cpp
        DroidBlaster.cpp
        GraphicsManager.cpp
        Ship.cpp
        TimeManager.cpp
        PhysicsManager.cpp
        Asteroid.cpp
        Resource.cpp
        Sprite.cpp
        SpriteBatch.cpp
        StarField.cpp
        SoundManager.cpp
        Sound.cpp
        SoundQueue.cpp
        InputHandler.cpp
        InputManager.cpp
        MoveableBody.cpp
        Configuration.cpp
        )

target_include_directories(nativedroidblaster PRIVATE
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
        )
# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        nativedroidblaster
        # Links the target library to the log library
        # included in the NDK.
        # Sets the library as a shared library.
        android
        native_app_glue
        log
        EGL
        GLESv2
        z
        png
        OpenSLES
        Box2D
        )
