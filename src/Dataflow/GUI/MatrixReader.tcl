#
#  The contents of this file are subject to the University of Utah Public
#  License (the "License"); you may not use this file except in compliance
#  with the License.
#  
#  Software distributed under the License is distributed on an "AS IS"
#  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#  License for the specific language governing rights and limitations under
#  the License.
#  
#  The Original Source Code is SCIRun, released March 12, 2001.
#  
#  The Original Source Code was developed by the University of Utah.
#  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
#  University of Utah. All Rights Reserved.
#

# example of reader
# by Samsonov Alexei
# October 2000


catch {rename SCIRun_DataIO_MatrixReader ""}

itcl_class SCIRun_DataIO_MatrixReader {
    inherit Module
    constructor {config} {
	set name MatrixReader
	set_defaults
    }

    method set_defaults {} {
	global $this-filetype
    }

    method ui {} {
	set w .ui[modname]

	if {[winfo exists $w]} {
	    return
	}

	toplevel $w -class TkFDialog
	set initdir ""
	
	# place to put preferred data directory
	# it's used if $this-filename is empty

	global SCIRUN_DATA SCI_DATA PSE_DATA
	if { $SCIRUN_DATA != "" } {
	    set initdir $SCIRUN_DATA
	} elseif { $SCI_DATA != "" } {
	    set initdir $SCI_DATA
	} elseif { $PSE_DATA != "" } {
	    set initdir PSE_DATA
	}
	
	#######################################################
	# to be modified for particular reader

	# extansion to append if no extension supplied by user
	set defext ".mat"
	set title "Open matrix file"
	
	# file types to appers in filter box
	set types {
	    {{Matrix}          {.mat} }
	    {{All Files}       {.*}   }
	}
	
	######################################################
	
	makeOpenFilebox \
		-parent $w \
		-filevar $this-filename \
		-command "$this-c needexecute; wm withdraw $w" \
		-cancel "wm withdraw $w" \
		-title $title \
		-filetypes $types \
		-initialdir $initdir \
		-defaultextension $defext

	moveToCursor $w	
    }
}
