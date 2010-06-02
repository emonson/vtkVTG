package require vtkcommon

#
# Here you should pick the name of one your common local classes
# instead of vtkBar.
#

if {[info commands vtkSlab] != "" ||
    [::vtk::load_component vtkvtgCommonTCL] == ""} {
    package provide vtkvtgcommon 4.0
}
