// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"
#include "Exports.h"

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.

/*
* 
armeabi-v7a
x86_64

export ANDROID_NDK=C:/Microsoft/AndroidNDK/android-ndk-r23c
export PATH=$ANDROID_NDK/toolchains/llvm/prebuilt/windows-x86_64/bin:$PATH

cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -G Ninja .

export ANDROID_NDK="C:/Program Files (x86)/Android/AndroidNDK/android-ndk-r27c"
export PATH="$ANDROID_NDK/toolchains/llvm/prebuilt/windows-x86_64/bin:$PATH"

cmake -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a \
      -G Ninja .


/---------------------------------------------------


export ANDROID_NDK="C:/Program Files (x86)/Android/AndroidNDK/android-ndk-r23c"
export PATH="$ANDROID_NDK/toolchains/llvm/prebuilt/windows-x86_64/bin:$PATH"

cmake -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a \
      -G Ninja .


export ANDROID_NDK="C:/Program Files (x86)/Android/AndroidNDK/android-ndk-r23c"
export PATH="$ANDROID_NDK/toolchains/llvm/prebuilt/windows-x86_64/bin:$PATH"

cmake -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=x86_64 \
      -G Ninja .


//NDK 26

export ANDROID_NDK="C:/Program Files (x86)/Android/AndroidNDK/android-ndk-r26d"
export PATH="$ANDROID_NDK/toolchains/llvm/prebuilt/windows-x86_64/bin:$PATH"

cmake -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a \
      -G Ninja .


export ANDROID_NDK="C:/Program Files (x86)/Android/AndroidNDK/android-ndk-r26d"
export PATH="$ANDROID_NDK/toolchains/llvm/prebuilt/windows-x86_64/bin:$PATH"

cmake -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=x86_64 \
      -G Ninja .







*/