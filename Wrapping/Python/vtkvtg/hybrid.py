""" This module loads all the classes from the VTK Hybrid library into its
namespace.  This is a required module."""

import os

if os.name == 'posix':
    from libvtkvtgHybridPython import *
else:
    from vtkvtgHybridPython import *
