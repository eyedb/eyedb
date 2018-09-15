#!/bin/sh
#
# eyedbcgi.sh
#
# Copyright (c) SYSRA Informatique 1995-1999
#
# Generic CGI Script for the EyeDB Web Browser
#

# --------------------------------------------------------------------------
#                                ENVIRONMENT
# --------------------------------------------------------------------------

#
# You must set the following environment variables:
#
# HOST   : the host on which the eyedbwwwd server is running.
# PORT   : the main port on which the eyedbwwwd server is listening.
# ALIAS  : the http alias for the cgi-bin script, for instance:
#          www.xxx.com/eyedb
# CGIDIR : the relative path of the directory of this cgi shell script
#          (default is cgi-bin)

PORT=10000
HOST=localhost
ALIAS=localhost/eyedb
CGIDIR=cgi-bin

# --------------------------------------------------------------------------
#                           DO NOT EDIT THIS CODE
# --------------------------------------------------------------------------

read input

./eyedbcgife ${HOST} \
             ${PORT} \
             `./eyedbcgiclean "${QUERY_STRING}"` \
             `./eyedbcgiclean "${input}"` \
             -cgidir   "${CGIDIR}" \
             -alias    "${ALIAS}" \
             -referer  "${HTTP_REFERER}" \
             -raddr    "${REMOTE_ADDR}" \
             -rhost    "${REMOTE_HOST}"
