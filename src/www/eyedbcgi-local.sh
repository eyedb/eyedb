#!/bin/sh
#
# eyedbcgi-local.sh
#
# Generic Script for EyeDB CGI
#

#
# environment
#

CGIPORT=20000
CGIHOST=lovelace
CGIDIR=www.infobiogen.fr/local/test/viara/cgi-bin

#
# arguments input
#

clean()
{
    ret=`echo $1 | sed 's/=/ /g' | sed 's/_x_/ /g' | sed 's/&/ /g'`
}

read input

clean $input
ARGUMENTS=$ret

clean $QUERY_STRING
QUERY_STRING=$ret

./eyedbcgife ${CGIHOST} ${CGIPORT} \
             ${QUERY_STRING} ${ARGUMENTS} \
             -cgidir ${CGIDIR}
