""" This module loads all the classes from the VTK Rendering library into
its namespace.  This is an optional module."""

import os

if os.name == 'posix':
    from libvtkvtgRenderingPython import *
else:
    from vtkvtgRenderingPython import *
