# -*- coding: utf-8 -*-
"""
Created on Thursday May 8 2014

@author: tsaad

This script will remove zero velocity and zero momentum BCs from Velocity boundaries

      <Face side="x-" name="inlet" type="Velocity"/>
        <BCType label="x-mom" var="Dirichlet" value="0.1"/>
        <BCType label="y-mom" var="Dirichlet" value="0.0"/>
        <BCType label="z-mom" var="Dirichlet" value="0.0"/>
        <BCType label="u" var="Dirichlet" value="0.1"/>
        <BCType label="v" var="Dirichlet" value="0.0"/>
        <BCType label="w" var="Dirichlet" value="0.0"/>
      </Face>

becomes

      <Face side="x-" name="inlet" type="Velocity"/>
        <BCType label="x-mom" var="Dirichlet" value="0.1"/>
        <BCType label="u" var="Dirichlet" value="0.1"/>
      </Face>

This will reduce the number of lines in the BC specification by up to 66.6%.

WARNING: THIS ASSUMES THAT THE NAME OF THE MOMENTUM AND VELOCITY VARIABLES ARE:
x-mom, y-mom, z-mom, u, v, and w. THIS HAS BEEN THE CASE FOR ALL OF OUR INPUT FILES

***************
USAGE:
remove-zero-velocity-bcs.py -i <inputfile> -d <path>

This script can be used to convert a single ups file OR all ups files in a given
directory, specified by -d. You cannot specify BOTH -i and -d.

***************
EXAMPLE:
remove-zero-velocity-bcs -d /Users/me/development/uintah/src/StandAlone/inputs/Wasatch
will process all ups files in the Wasatch inputs directory

"""
import lxml.etree as et
import sys, getopt
import glob

# Function that indents elements in a xml tree. 
# Thanks to: http://effbot.org/zone/element-lib.htm#prettyprint
def indent(elem, level=0):
    i = "\n" + level*"  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def help():
  print 'Usage: remove-zero-velocity-bcs -i <inputfile>'

# Commandline argument parser
def main(argv):
   inputfile = ''
   inputdir  = ''
   try:
      opts, args = getopt.getopt(argv,"hi:d:",["ifile=","dir="])
   except getopt.GetoptError:
       help()
       sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         help()
         sys.exit()
      elif opt in ("-i", "--ifile"):
         inputfile = arg
      elif opt in ("-d", "--dir"):
         inputdir = arg
   print inputfile
   print inputdir      
   if inputfile=='' and inputdir=='':
     print 'Please specify a valid input file or directory!'
     help()
     sys.exit(2)

   if (inputfile !='' and inputdir!=''):
     print 'You cannot specify both inputfile and inputdir!'
     help()
     sys.exit(2)

   return (inputdir, inputfile)

# converts <value> to an attribute
def remove_zero_velocity(fname):
		print 'Processing file: ', fname
		tree = et.parse(fname)
		root = tree.getroot()
		hasBCs = True
		# check if this file has boundary conditions
		if (root.find('Grid')).find('BoundaryConditions') is None:
				hasBCs=False
				return
				
		hasValue = True
		for face in root.iter('Face'):
				if face.attrib.has_key('type'):
						bndType = face.attrib['type']
				else:
						continue

				if bndType == "Velocity":
						print bndType
						for bc in face.iter('BCType'):            
								bcvar = bc.attrib['label']
								bcval = bc.attrib['value']
								removebc = False
								
								try:
									bcval = float(bcval)
								except ValueError:
									print "Not a float"
									bcval = 999
								
								if (bcvar == "x-mom") and (bcval == 0.0):
										removebc = True

								if (bcvar == "y-mom") and (bcval == 0.0):
										removebc = True

								if (bcvar == "z-mom") and (bcval == 0.0):
										removebc = True

								if (bcvar == "u") and (bcval == 0.0):
										removebc = True

								if (bcvar == "v") and (bcval == 0.0):
										removebc = True

								if (bcvar == "w") and (bcval == 0.0):
										removebc = True

								
								if removebc:
										face.remove(bc)		
																
    # only write the file when it has boundary conditions
		if hasBCs is True:
				tree.write(fname,pretty_print=True)
				print '  done!'

# start here
if __name__ == "__main__":
   inputdir, fname = main(sys.argv[1:])
   if inputdir != '':
     for fname in glob.glob(inputdir + "/*.ups"):
          remove_zero_velocity(fname)
   elif fname != '':
     remove_zero_velocity(fname)
        