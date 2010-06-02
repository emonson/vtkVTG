#!/usr/bin/env python
""" copyPVslabFiles.py -- copy modified slab vtk/paraview files to test directories"""

import sys, os, glob

srcRootDir = '/Users/emonson/Programming/VTK_cvs/vtkVTG'
destRootDir = '/Users/emonson/Programming/ParaView_vtg/ParaView3'

if not os.path.isdir(srcRootDir) or not os.path.isdir(destRootDir):
	sys.exit(1)

# Straight VTK Stuff
VTKlist = ['Common','Widgets']
for currDir in VTKlist:
	os.chdir(os.path.join(srcRootDir,currDir))
	fileList = glob.glob('vtk*.*')
	for file in fileList:
		cmd = 'rsync -ut ' + os.path.join(srcRootDir,currDir,file) + ' ' + os.path.join(destRootDir,'VTK',currDir,file)
		os.system(cmd)
		# print os.path.join(srcRootDir,currDir,file), \
		# 		os.path.join(destRootDir,'VTK',currDir,file)

# Qt/Components/pqSlabWidget.*
currAbsDir = os.path.join(srcRootDir,'Qt','Components')
destAbsDir = os.path.join(destRootDir,'Qt','Components')
os.chdir(currAbsDir)
fileList = glob.glob('pq*.*')
for file in fileList:
	cmd = 'rsync -ut ' + os.path.join(currAbsDir,file) + ' ' + os.path.join(destAbsDir,file)
	os.system(cmd)
	# print os.path.join(currAbsDir,file), os.path.join(destAbsDir,file)
	
# Qt/Components/Resources/UI/pqSlabWidget.ui
currAbsDir = os.path.join(currAbsDir,'Resources','UI')
destAbsDir = os.path.join(destAbsDir,'Resources','UI')
os.chdir(currAbsDir)
fileList = glob.glob('pq*.*')
for file in fileList:
	cmd = 'rsync -ut ' + os.path.join(currAbsDir,file) + ' ' + os.path.join(destAbsDir,file)
	os.system(cmd)
	# print os.path.join(currAbsDir,file), os.path.join(destAbsDir,file)

# Servers/ServerManager/vtkSM*.*
currAbsDir = os.path.join(srcRootDir,'Servers','ServerManager')
destAbsDir = os.path.join(destRootDir,'Servers','ServerManager')
os.chdir(currAbsDir)
fileList = glob.glob('vtkSM*.*')
for file in fileList:
	cmd = 'rsync -ut ' + os.path.join(currAbsDir,file) + ' ' + os.path.join(destAbsDir,file)
	os.system(cmd)
	# print os.path.join(currAbsDir,file), os.path.join(destAbsDir,file)

# Servers/ServerManager/Resources/*.xml
currAbsDir = os.path.join(currAbsDir,'Resources')
destAbsDir = os.path.join(destAbsDir,'Resources')
os.chdir(currAbsDir)
fileList = glob.glob('*.xml')
for file in fileList:
	cmd = 'rsync -ut ' + os.path.join(currAbsDir,file) + ' ' + os.path.join(destAbsDir,file)
	os.system(cmd)
	# print os.path.join(currAbsDir,file), os.path.join(destAbsDir,file)
