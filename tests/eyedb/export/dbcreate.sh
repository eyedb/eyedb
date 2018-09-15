#!/bin/sh

db=$1

dbcreate $1

phy=log

datcreate $1 $1_01.dat:DAT_01:110:32:log
datcreate $1 $1_02.dat:DAT_02:120:32:log
datcreate $1 $1_03.dat:DAT_03:110:16:log
datcreate $1 $1_04.dat:DAT_04:200:64:log
datcreate $1 $1_05.dat:DAT_05:300:32:$phy
datcreate $1 $1_06.dat:DAT_06:200:64:log
datcreate $1 $1_07.dat:DAT_07:300:64:$phy
datcreate $1 $1_08.dat:DAT_08:100:64:log
datcreate $1 $1_09.dat:DAT_09:100:64:log
datcreate $1 $1_10.dat:DAT_10:100:64:log
datcreate $1 $1_11.dat:DAT_11:100:64:log

dspcreate $1 DSP_01 1 2
dspcreate $1 DSP_02 DAT_03 DAT_04
dspcreate $1 DSP_03 DAT_05 DAT_07
dspcreate $1 DSP_04 DAT_06 DAT_08
dspcreate $1 DSP_05 DAT_09 DAT_10 DAT_11

odl -d $1 -u export.odl

setdefinstdsp $1 C0 DEFAULT
setdefinstdsp $1 C1 DSP_01
setdefinstdsp $1 C2 DSP_02
setdefinstdsp $1 C3 DSP_03
setdefinstdsp $1 C4 DSP_04
setdefinstdsp $1 C5 DSP_05

oql -d $1 -w export.oql

# dbexport/dbimport: pb pour les datafiles physiques 



