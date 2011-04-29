""" This module loads the entire VTK library into its namespace.  It
also allows one to use specific packages inside the vtk directory.."""

import os
import sys


# The dl module is used to force the symbols in the loaded VTK modules to
# be global, that is, to force symbols to be shared between modules.  This
# used to be necessary in VTK 4 but might not be with VTK 5 and later.

# The first "except" is because systems like AIX don't have the dl module.
# The second "except" is because the dl module raises a system error on
# ia64 and x86_64 systems because "int" and addresses are different sizes.
try:
    import dl
except ImportError:
    # do not give up too early:
    # are we on AMD64 ?
    try:
      import DLFCN as dl
    except ImportError:
      dl = None
except SystemError:
    dl = None

# set the dlopen flags so that VTK does not run into problems with
# shared symbols.
try:
    # only Python >= 2.2 has this functionality
    orig_dlopen_flags = sys.getdlopenflags()
except AttributeError:
    orig_dlopen_flags = None

if dl and (os.name == 'posix'):
    sys.setdlopenflags(dl.RTLD_NOW|dl.RTLD_GLOBAL)    

# Load all required kits.
from charts import *
from common import *
from filtering import *
from graphics import *
from hybrid import *
from infovis import *
from qvtk import *
from qvtkdebug import *
from rendering import *
from widgets import *

# the vtkvtg.kits variable tells us which kits we actually have
kits = ['charts', 'common', 'filtering', 'graphics', 'hybrid', 'infovis', 'qvtk', 'qvtkdebug', 'rendering', 'widgets']


# import useful VTK related constants.
# from util.vtkConstants import *

# reset the dlopen flags to the original state if possible.
if dl and (os.name == 'posix') and orig_dlopen_flags:
    sys.setdlopenflags(orig_dlopen_flags)

# removing things the user shouldn't have to see.
del orig_dlopen_flags
del sys, dl, os
