#!/usr/bin/env tclsh

# Execute this script each time you add a new directory or file.

# Packages

pkg_mkIndex -direct -verbose vtkvtg
pkg_mkIndex -direct -verbose vtkvtgcommon
pkg_mkIndex -direct -verbose vtkvtgfiltering
pkg_mkIndex -direct -verbose vtkvtggraphics
pkg_mkIndex -direct -verbose vtkvtgwidgets

exit
