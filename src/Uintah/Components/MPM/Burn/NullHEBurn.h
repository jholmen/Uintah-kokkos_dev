#ifndef __NULL_HEBURN_H__
#define __NULL_HEBURN_H__

#include <Uintah/Components/MPM/Burn/HEBurn.h>
#include <Uintah/Interface/DataWarehouseP.h>
#include <Uintah/Parallel/UintahParallelComponent.h>
#include <Uintah/Interface/MPMInterface.h>
#include <Uintah/Grid/GridP.h>
#include <Uintah/Grid/LevelP.h>
#include <Uintah/Interface/ProblemSpecP.h>
#include <Uintah/Interface/ProblemSpec.h>
#include <Uintah/Grid/SimulationState.h>
#include <Uintah/Grid/SimulationStateP.h>


namespace Uintah {
  namespace MPM {

/**************************************

CLASS
   NullHEBurn
   
   Short description...

GENERAL INFORMATION

   NullHEBurn.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   HEBurn_Model_Null

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

    class NullHEBurn : public HEBurn { 
    private:
      
      // Prevent copying of this class
      // copy constructor
      NullHEBurn(const NullHEBurn &burn);
      NullHEBurn & operator=(const NullHEBurn &burn);
      
    public:
      // Constructor
      NullHEBurn(ProblemSpecP& ps);
      
      // Destructor
      ~NullHEBurn();

      void checkIfIgnited(const Patch* patch,
			  const MPMMaterial* matl,
			  DataWarehouseP& old_dw,
			  DataWarehouseP& new_dw);
      void computeMassRate();
      void updatedParticleMassAndVolume(); 

      void addCheckIfComputesAndRequires( Task* task,
					  const MPMMaterial* matl,
					  const Patch* patch,
					  DataWarehouseP& old_dw,
					  DataWarehouseP& new_dw) const;

    };
    
  } // end namespace MPM
} // end namespace Uintah

// $Log$
// Revision 1.2  2000/06/06 18:04:02  guilkey
// Added more stuff for the burn models.  Much to do still.
//
// Revision 1.1  2000/06/02 22:48:26  jas
// Added infrastructure for Burn models.
//

#endif /* __NULL_HEBURN_H__*/

