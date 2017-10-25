
pushd ..\..\android-ndk-r15c\simpleperf
python app_profiler.py
copy perf.data ..\..\Branch\Android
start /B python report.py
popd
