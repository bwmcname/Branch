@echo off
setlocal
set CC="C:\\Program Files\\android-ndk-r15c\\toolchains\\aarch64-linux-android-4.9\\prebuilt\\windows-x86_64\\bin\\aarch64-linux-android-gcc-4.9.exe"
set DUMP="C:\\Program Files\\android-ndk-r15c\\toolchains\\aarch64-linux-androideabi-4.9\\prebuilt\\windows-x86_64\\bin\\aarch64-linux-androideabi-objdump.exe"
set SYSROOT="C:\\Program Files\\android-ndk-r15c\\platforms\\android-21\\arch-arm64"
set SDK="C:\\Program Files (x86)\\Android\\android-sdk"
set ZIPALIGN="C:\\Program Files (x86)\\Android\\android-sdk\\build-tools\\21.1.2\\zipalign.exe"

rem The earliest version that supports opengles3 is android-18
rem The earliest version that supports AArch64 is android-21
set INCLUDES="C:\\Program Files\\android-ndk-r15c\\platforms\\android-21\\arch-arm64\\usr\\include"

del Branch.apk

copy ..\assets\Packed.assets assets\assets\

call ndk-build NDK_DEBUG=1 APP_BUILD_SCRIPT=Android.mk NDK_APPLICATION_MK=Application.mk NDK_LIBS_OUT=output\lib\lib

aapt package --debug-mode -m -J output -f -M AndroidManifest.xml -S res -I %SDK%\platforms\android-21\android.jar -F BranchUnsigned.apk output\lib assets

call SignAPK

del BranchUnsigned.apk

%ZIPALIGN% 4 BranchUnaligned.apk Branch.apk

del BranchUnaligned.apk

endlocal
