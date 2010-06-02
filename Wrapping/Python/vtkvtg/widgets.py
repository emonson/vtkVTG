""" This module loads all the classes from the VTK Widgets library
into its namespace.  This is an optional module."""

import os  

if os.name == 'posix':
    from libvtkvtgWidgetsPython import *
else:
    from vtkvtgWidgetsPython import *
