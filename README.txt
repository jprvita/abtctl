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

Running
=======

abtctl can be run from an adb shell simply typing btctl as root. It will present
a prompt were commands can be entered, and the 'help' command will print a list
of all the available commands. Each command also have its own help, which can be
accessed passing 'help' as the first argument of the command. For example, the
help of the connect command is accessible through 'connect help'.

Limitations
===========

btctl
-----

* We only support 10 connections at same time (easily extendable changing
  MAX_CONNECTIONS value).
* We are using a static buffer for search_result_cb, so we have a limit of 128
  services that can be handled.

bluedroid
---------

* The stack doesn't support more than one pending connection at same time. So
  if some connection is stuck, cancel that connection attempt before trying a
  new connection.
* Bluedroid doesn't handle the bonded devices list very well. This means it may
  fail to realize that the link already have the necessary security level and
  try to re-authenticate a link that is already bonded when there is no need.
  Also, it may fail to pair with a unbonded device because it thinks it is
  already bonded (this have been noticed after removing the bond with a
  previously bonded device). To force the removal of all bonded devices the
  files /data/misc/bluedroid/bt_config.xml and
  /data/misc/bluedroid/bt_config.old should be deleted.
