#ifndef __CONTACT_H__
#define __CONTACT_H__

#include <Uintah/Interface/DataWarehouseP.h>

#include <math.h>

namespace Uintah {

  namespace Grid {
    class Region;
  }
  namespace Parallel {
    class ProcessorContext;
  }

namespace Components {

using Uintah::Grid::Region;
using Uintah::Interface::DataWarehouseP;
using Uintah::Parallel::ProcessorContext;

/**************************************

CLASS
   Contact
   
   Short description...

GENERAL INFORMATION

   Contact.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   Contact_Model

DESCRIPTION
   Long description...
  
WARNING

****************************************/

class Contact {
public:
  // Basic contact methods
  virtual void exMomInterpolated(const ProcessorContext*,
				 const Region* region,
                                 const DataWarehouseP& old_dw,
                                 DataWarehouseP& new_dw) = 0;

  virtual void exMomIntegrated(const ProcessorContext*,
			       const Region* region,
                               const DataWarehouseP& old_dw,
                               DataWarehouseP& new_dw) = 0;


  // Auxilliary methods to supply data needed by some of the
  // advanced contact models
  virtual void computeSurfaceNormals()
  {
    // Null function is the default.  Particular contact
    // classes will define these functions when needed.
    return;
  };
  virtual void computeTraction()
  {
    // Null function is the default.  Particular contact
    // classes will define these functions when needed.
    return;
  };
};

inline bool compare(double num1, double num2)
{
  double EPSILON=1.e-8;

  return (fabs(num1-num2) <= EPSILON);
}


} // end namespace Components
} // end namespace Uintah

// $Log$
// Revision 1.5  2000/04/12 16:57:27  guilkey
// Converted the SerialMPM.cc to have multimaterial/multivelocity field
// capabilities.  Tried to guard all the functions against breaking the
// compilation, but then who really cares?  It's not like sus has compiled
// for more than 5 minutes in a row for two months.
//
// Revision 1.4  2000/03/21 01:29:41  dav
// working to make MPM stuff compile successfully
//
// Revision 1.3  2000/03/20 23:50:44  dav
// renames SingleVel to SingleVelContact
//
// Revision 1.2  2000/03/20 17:17:12  sparker
// Made it compile.  There are now several #idef WONT_COMPILE_YET statements.
//
// Revision 1.1  2000/03/16 01:05:13  guilkey
// Initial commit for Contact base class, as well as a NullContact
// class and SingleVel, a class which reclaims the single velocity
// field result from a multiple velocity field problem.
//

#endif // __CONTACT_H__

