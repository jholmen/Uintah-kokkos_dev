#
# Makefile fragment for this subdirectory
# $Id$
#

SRCDIR := SCIRun/Datatypes

SUBDIRS := $(SRCDIR)/Image

include $(OBJTOP_ABS)/scripts/recurse.mk

#
# $Log$
# Revision 1.1  2000/03/17 09:28:52  sparker
# New makefile scheme: sub.mk instead of Makefile.in
# Use XML-based files for module repository
# Plus many other changes to make these two things work
#
#
