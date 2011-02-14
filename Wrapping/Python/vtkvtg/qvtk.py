""" This module loads all the classes from the VTKVTG QVTK library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkvtgQVTKPython import *
else:
    from vtkvtgQVTKPython import *
