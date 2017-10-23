call build-arm
call adb install -r Branch.apk
call adb logcat *:S Branch
