//static char *id="@(#) $Id$";

/*
 *  Datatype.cc: The Datatype Data type
 *
 *  Written by:
 *   David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   July 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <SCICore/Datatypes/Datatype.h>
#include <SCICore/Thread/AtomicCounter.h>

namespace SCICore {
namespace Datatypes {

static SCICore::Thread::AtomicCounter current_generation("Datatypes generation counter", 1);

Datatype::Datatype()
: lock("Datatype ref_cnt lock")
{
    ref_cnt=0;
    generation=current_generation++;
}

Datatype::Datatype(const Datatype&)
    : lock("Datatype ref_cnt lock")
{
    ref_cnt=0;
    generation=current_generation++;
}

Datatype::~Datatype()
{
}

} // End namespace Datatypes
} // End namespace SCICore

//
// $Log$
// Revision 1.4  1999/08/28 17:54:36  sparker
// Integrated new Thread library
//
// Revision 1.3  1999/08/25 03:48:32  sparker
// Changed SCICore/CoreDatatypes to SCICore/Datatypes
// Changed PSECore/CommonDatatypes to PSECore/Datatypes
// Other Misc. directory tree updates
//
// Revision 1.2  1999/08/17 06:38:45  sparker
// Merged in modifications from PSECore to make this the new "blessed"
// version of SCIRun/Uintah.
//
// Revision 1.1  1999/07/27 16:56:20  mcq
// Initial commit
//
// Revision 1.1  1999/04/25 04:07:06  dav
// Moved files into Datatypes
//
// Revision 1.1.1.1  1999/04/24 23:12:52  dav
// Import sources
//
//
