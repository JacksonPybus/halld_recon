#!/bin/tcsh
 
# This file was generated by the script "mk_setenv.csh"
#
# Generation date: Sun Jan 29 19:07:40 EST 2012
# User: gluex
# Host: jlabl3.jlab.org
# Platform: Linux jlabl3.jlab.org 2.6.18-274.3.1.el5 #1 SMP Fri Aug 26 18:45:04 EDT 2011 i686 i686 i386 GNU/Linux
# BMS_OSNAME: Linux_RHEL5-i686-gcc4.1.2
 
if ( ! $?BMS_OSNAME ) then
   setenv BMS_OSNAME `/group/halld/Software/scripts/osrelease.pl`
endif
 
if ( -e /group/halld/Software/builds/sim-recon/sim-recon-2012-01-27/setenv_${BMS_OSNAME}.csh ) then
    # Try prepending path of cwd used in generating this file
    source /group/halld/Software/builds/sim-recon/sim-recon-2012-01-27/setenv_${BMS_OSNAME}.csh
else if ( -e setenv_${BMS_OSNAME}.csh ) then
    source setenv_${BMS_OSNAME}.csh
endif 
 
