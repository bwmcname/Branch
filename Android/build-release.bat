@echo off
setlocal

set SDK="C:\\Program Files (x86)\\Android\\android-sdk"
set ZIPALIGN="C:\\Program Files (x86)\\Android\\android-sdk\\build-tools\\21.1.2\\zipalign.exe"

del Branch.apk

copy ..\ProcessedAssets\Packed.assets assets\assets\

call ndk-build APP_BUILD_SCRIPT=AndroidRelease.mk NDK_APPLICATION_MK=ApplicationRelease.mk NDK_LIBS_OUT=output\lib\lib

aapt package -m -J output -f -M AndroidManifest.xml -S res -I %SDK%\platforms\android-21\android.jar -F BranchUnsigned.apk output\lib assets

call SignAPK

del BranchUnsigned.apk

%ZIPALIGN% 4 BranchUnaligned.apk Branch.apk

del BranchUnaligned.apk

endlocal
