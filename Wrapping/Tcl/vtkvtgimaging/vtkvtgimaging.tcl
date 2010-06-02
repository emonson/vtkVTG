package require vtkvtgcommon
package require vtkimaging

#
# Here you should pick the name of one your imaging local classes
# instead of vtkImageFoo.
#

if {[info commands vtkImageFoo] != "" ||
    [::vtk::load_component vtkvtgImagingTCL] == ""} {
    package provide vtkvtgimaging 4.0
}
