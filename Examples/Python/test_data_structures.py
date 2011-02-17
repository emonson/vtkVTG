import vtk
import sys
import numpy as N
from vtk.util import numpy_support as VN
import scipy.io

data_dir = '/Users/emonson/Data/Fodava/EMoGWDataSets/'
data_file = data_dir + 'sciNews_20110216.mat'
	
print 'Trying to load data set from .mat file...'

try:
	MatInput = scipy.io.loadmat(data_file, struct_as_record=True, chars_as_strings=True)
except:
	raise IOError, "Can't load supplied matlab file"

mat_terms = MatInput['terms']

WavBases = []	# Wavelet bases
Centers = []	# Center of each node
# NodeWavCoeffs = []
# NodeScalCoeffs = []
for ii in range(MatInput['PointsInNet'].shape[1]):
	WavBases.append(N.mat(MatInput['WavBases'][0,ii]))			# matrix
	Centers.append(N.mat(MatInput['Centers'][0,ii][0])) 		# matrix

