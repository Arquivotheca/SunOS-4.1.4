#! /bin/sh
#
# @(#)ypxfr_1perhour.sh 1.1 94/10/31 Copyr 1990 Sun Microsystems, Inc.  
#
# ypxfr_1perhour.sh - Do hourly NIS map check/updates
#

PATH=/bin:/usr/bin:/usr/etc:/usr/etc/yp:$PATH
export PATH

# set -xv
ypxfr passwd.byname
ypxfr passwd.byuid 
