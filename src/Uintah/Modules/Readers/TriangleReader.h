//static char *id="@(#) $Id$";

/*
 *  TriangleReader.cc: Triangle Reader class
 *
 *  Written by:
 *   Kurt Zimmerman
 *   Department of Computer Science
 *   University of Utah
 *   July 1999
 *
 *  Copyright (C) 1999 SCI Group
 */

/*  Specialty class written for reading simple triangle that were
    output from the crack propogation work of Honglai Tan.  The file format
    for this reader is:
    -------------------------------------------
    HONGLAI TRIANGLES
    <nPoints>
    p0-0 p0-1 p0-2 s0
    p1-0 p1-1 p1-2 s1
    p2-0 p2-1 p2-2 s2
    .
    .
    .
    <ntriangles>
    t0-0 t0-1 t0-2
    t1-0 t1-1 t1-2
    .
    .
    .
    -------------------------------------------
    An example file containting 4 points and two triangles looks like this:
    
    -------------------------------------------
    HONGLAI_TRIANGLES
    4 
    0.0 0.0 0.0 0.1 
    2.0 0.0 0.0 0.4 
    2.0 1.0 0.3 0.8 
    -0.2 0.1 1.0 0.3 

    2 
    0 1 2 
    0 2 3 
    ------------------------------------------- */

#include <Util/NotFinished.h>
#include <Dataflow/Module.h>
#include <CommonDatatypes/GeometryPort.h>
#include <CommonDatatypes/ColorMapPort.h>
#include <CoreDatatypes/ColorMap.h>
#include <Geom/GeomTri.h>
#include <Geom/GeomGroup.h>

#include <Malloc/Allocator.h>
#include <TclInterface/TCLTask.h>
#include <TclInterface/TCLvar.h>
#include <iostream.h>

namespace Uintah {
namespace Modules {

using namespace PSECommon::Dataflow;
using namespace PSECommon::CommonDatatypes;
using namespace SCICore::TclInterface;
using namespace SCICore::GeomSpace;

class TriangleReader : public Module {
    ColorMapIPort* inport;
    GeometryOPort* outport;
    clString old_filename;
public:
    TriangleReader(const clString& id);
    TriangleReader(const TriangleReader&, int deep=0);
    virtual ~TriangleReader();
    virtual Module* clone(int deep);
    virtual void execute();
    TCLstring filename;

private:
  bool Read(istream& is, ColorMapHandle cmh, GeomGroup* tris);

};
} // End namespace Modules
} // End namespace Uintah


//
// $Log$
// Revision 1.1  1999/08/02 20:00:39  kuzimmer
// checked in Triangle Reader for Honlai's Triangles.
//
// Revision 1.3  1999/07/07 21:10:26  dav
// added beginnings of support for g++ compilation
//
// Revision 1.2  1999/04/27 22:57:53  dav
// updates in Modules for CoreDatatypes
//
// Revision 1.1  1999/04/25 02:38:10  dav
// more things that should have been there but were not
//
//
