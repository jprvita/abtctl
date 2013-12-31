#! /system/bin/sh

export EXTERNAL_STORAGE=/mnt/sdcard
export LANG=en

PYTHONPATH=${EXTERNAL_STORAGE}/com.googlecode.pythonforandroid/extras/python
PYTHONPATH=${PYTHONPATH}:/data/data/com.googlecode.pythonforandroid/files/python/lib/python2.6/lib-dynload
PYTHONPATH=${PYTHONPATH}:/data/data/com.googlecode.pythonforandroid/files/python/lib/python2.6
export PYTHONPATH

export TEMP=${EXTERNAL_STORAGE}/com.googlecode.pythonforandroid/extras/python/tmp
export PYTHON_EGG_CACHE=$TEMP
export PYTHONHOME=/data/data/com.googlecode.pythonforandroid/files/python

LD_LIBRARY_PATH=/data/data/com.googlecode.pythonforandroid/files/python/lib
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/data/data/com.googlecode.pythonforandroid/files/python/lib/python2.6/lib-dynload
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${EXTERNAL_STORAGE}/com.googlecode.pythonforandroid/extras/python
export LD_LIBRARY_PATH

/data/data/com.googlecode.pythonforandroid/files/python/bin/python "$@"
