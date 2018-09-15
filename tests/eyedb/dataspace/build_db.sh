#
# build_db.sh
#

if [ $# != 1 ]
then
    echo "usage: $0 <dbname>"
    exit 1
fi

db=$1

set -e
dbcreate $db -datfiles ${db}_00.dat:DEFAULT \
                       ${db}_01.dat:DAT01 \
                       ${db}_02.dat:DAT02 \
                       ${db}_03.dat:DAT03 \
                       ${db}_04.dat:DAT04 \
                       ${db}_05.dat:DAT05 \
                       ${db}_06.dat:DAT06 \
                       ${db}_07.dat:DAT07 \
                       ${db}_08.dat:DAT08 \
                       ${db}_09.dat:DAT09:10000:32:phy \
                       ${db}_10.dat:DAT10:10000:32:phy

dspcreate $db DSP01 DAT01 DAT02
dspcreate $db DSP02 DAT03 DAT04 DAT05
dspcreate $db DSP03 DAT06 DAT07
dspcreate $db DSP04 DAT08
dspcreate $db DSP05 DAT09 DAT10

odl -d $db -u dspsch.odl

setdefdsp $db DSP04
setdefinstdsp $db O1 DSP01
setdefinstdsp $db O2 DSP02
setdefinstdsp $db O3 DSP03
setdefinstdsp $db O4 DSP04
setdefinstdsp $db O5 DSP05


