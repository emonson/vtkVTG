package require vtkvtgimaging
package require vtk

#
# Here you should pick the name of one your imaging local classes
# instead of vtkBar2.
#

if {[info commands vtkBar2] != "" ||
    [::vtk::load_component vtkvtgUnsortedTCL] == ""} {
    package provide vtkvtgunsorted 4.0
}
