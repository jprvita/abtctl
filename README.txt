Android Bluetooth Control tool

This is a tool to provide control of the Android Bluetooth stack from the
system level. Being a system-level tool it can be used from any Android-based
system, since it doesn't depend on Dalvik or the Android Java framework.

Cloning
=======

abtctl is available via git from git://people.freedesktop.org/~jprvita/abtctl on
branch master. The repository URL may change at some point in the future.

The repository should be cloned in external/bluetooth/ inside an AOSP forest.

Building
========

abtctl uses the Android's build system, what implies that you have to initialize
the AOSP environment ('source build/envsetup.sh' and lunch with the proper
options) to be able to build it.

If you have the source code in right place (see Cloning) all eng builds will
already include the btctl tool. If you want to build only the tool you can run
'make btctl' from the AOSP root or 'mm' from the abtctl directory, with the last
option being noticeably faster.
