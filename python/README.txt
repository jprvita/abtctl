How to use the Python bindings on AOSP
======================================

1. Build AOSP and flash on the device.

2. Enable installation of apps from unknown sources under Settings, Security.

3. Install Scripting Layer for Android (SL4A) [1]. SL4A is licensed under the
Apache License 2.0 [2].
  $ adb install sl4a_r6.apk

4. Open the SL4A app through the UI, swipe right for the "Interpreters" screen,
click on "Menu", "Add", "Python 2.6.2". When the download finishes click on the
notification of the finished download to install it.

5. Open the "Python for Android" application and click "Install".

6. Install the libble Python bindings.
  $ adb push ble.py /mnt/sdcard/com.googlecode.pythonforandroid/extras/python/

7. Install the standalone_python.sh script.
  $ adb push standalone_python.sh /system/bin
  $ adb shell
  root@mako:/ # chmod 755 /system/bin/standalone_python.sh

8. Run the Python interpreter.
  root@mako:/ # standalone_python.sh

Links:
------

[1] http://code.google.com/p/android-scripting/
[2] Source code can be found on https://github.com/damonkohler/sl4a
