""" This module loads all the classes from the VTK Infovis library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkvtgInfovisPython import *
else:
    from vtkvtgInfovisPython import *
