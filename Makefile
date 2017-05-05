# Change target name
TARGET = NXT_LINE
TARGET_SOURCES = \
	grand_finale.c
TOPPERS_OSEK_OIL_SOURCE = ./grand_finale.oil

# nxtOSEK root path
# Modify accordingly
#NXTOSEKROOT = /cygdrive/c/cygwin/nxtOSEK

#################################################################
# You should not need to modify below this line
O_PATH ?= build
include ../../ecrobot/ecrobot.mak
