#ifndef UINTAH_HOMEBREW_Region_H
#define UINTAH_HOMEBREW_Region_H

#include "Array3Index.h"

#include <Uintah/Grid/SubRegion.h>
#include <Uintah/Grid/ParticleSet.h>
#include <Uintah/Grid/Box.h>

#include <SCICore/Geometry/Point.h>
#include <SCICore/Geometry/Vector.h>
#include <SCICore/Geometry/IntVector.h>
#include <SCICore/Math/MiscMath.h>
#include <iosfwd>

namespace Uintah {
    
   using SCICore::Geometry::Point;
   using SCICore::Geometry::Vector;
   using SCICore::Geometry::IntVector;
   using SCICore::Math::RoundUp;
   
   class NodeSubIterator;
   class NodeIterator;
   class CellIterator;
   
/**************************************
      
CLASS
   Region
      
   Short Description...
      
GENERAL INFORMATION
      
   Region.h
      
   Steven G. Parker
   Department of Computer Science
   University of Utah
      
   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
      
   Copyright (C) 2000 SCI Group
      
KEYWORDS
   Region
      
DESCRIPTION
   Long description...
      
WARNING
     
****************************************/
    
   class Region {
   public:
      
      //////////
      // Insert Documentation Here:
      Vector dCell() const {
	 return (d_box.upper()-d_box.lower())/d_res;
      }
      
      //////////
      // Insert Documentation Here:
      void findCell(const Vector& pos, int& ix, int& iy, int& iz) const;
      
      //////////
      // Insert Documentation Here:
      bool findCellAndWeights(const SCICore::Geometry::Vector& pos,
				Array3Index ni[8], double S[8]) const{ }
      //////////
      // Insert Documentation Here:
      bool findCellAndShapeDerivatives(const SCICore::Geometry::Vector& pos,
				       Array3Index ni[8],
				       SCICore::Geometry::Vector S[8]) const{ }
      //////////
      // Insert Documentation Here:
      inline NodeIterator begin() const;
      
      //////////
      // Insert Documentation Here:
      inline NodeIterator end() const;

      //////////
      // Insert Documentation Here:
      CellIterator getCellIterator(const Box& b) const;

      //////////
      // Insert Documentation Here:
      void subregionIteratorPair(int i, int n,
				 NodeSubIterator& iter,
				 NodeSubIterator& end) const;
      //////////
      // Insert Documentation Here:
      SubRegion subregion(int i, int n) const;
      
      Array3Index getLowIndex() const;
      Array3Index getHighIndex() const;
      
      inline Box getBox() const {
	 return d_box;
      }
      
      inline IntVector getNCells() const {
	 return d_res;
      }
      
      long totalCells() const;
      
      void performConsistencyCheck() const;
      
      //////////
      // Insert Documentation Here:
      inline bool contains(const Array3Index& idx) const {
	 return idx.i() >= 0 && idx.j() >= 0 && idx.k() >= 0
	    && idx.i() <= d_res.x() && idx.j() <= d_res.y() && idx.k() <= d_res.z();
      }
      Point nodePosition(const IntVector& idx) const {
	 return d_box.lower() + dCell()*idx;
      }
   protected:
      friend class Level;
      
      //////////
      // Insert Documentation Here:
      Region(const SCICore::Geometry::Point& min,
	     const SCICore::Geometry::Point& max,
	     const SCICore::Geometry::IntVector& res);
      ~Region();
      
   private:
      Region(const Region&);
      Region& operator=(const Region&);
      
      //////////
      // Insert Documentation Here:
      Box d_box;
      
      //////////
      // Insert Documentation Here:
      IntVector d_res;
      
      friend class NodeIterator;
   };
   
} // end namespace Uintah

std::ostream& operator<<(std::ostream& out, const Uintah::Region* r);

#include "NodeIterator.h"

namespace Uintah {
   
   inline NodeIterator Region::begin() const
      {
	 return NodeIterator(this,
			     d_box.lower().x(), 
			     d_box.lower().y(), 
			     d_box.lower().z());
      }
   
   inline NodeIterator Region::end() const
      {
	 return NodeIterator(this, 
			     d_box.upper().x()+1, 
			     d_box.upper().y()+1, 
			     d_box.upper().z()+1);
      }
   
} // end namespace Uintah

//
// $Log$
// Revision 1.10  2000/04/28 20:24:44  jas
// Moved some private copy constructors to public for linux.  Velocity
// field is now set from the input file.  Simulation state now correctly
// determines number of velocity fields.
//
// Revision 1.9  2000/04/27 23:18:50  sparker
// Added problem initialization for MPM
//
// Revision 1.8  2000/04/26 06:48:54  sparker
// Streamlined namespaces
//
// Revision 1.7  2000/04/25 00:41:21  dav
// more changes to fix compilations
//
// Revision 1.6  2000/04/13 06:51:02  sparker
// More implementation to get this to work
//
// Revision 1.5  2000/04/12 23:00:50  sparker
// Starting problem setup code
// Other compilation fixes
//
// Revision 1.4  2000/03/22 00:32:13  sparker
// Added Face-centered variable class
// Added Per-region data class
// Added new task constructor for procedures with arguments
// Use Array3Index more often
//
// Revision 1.3  2000/03/21 01:29:42  dav
// working to make MPM stuff compile successfully
//
// Revision 1.2  2000/03/16 22:08:01  dav
// Added the beginnings of cocoon docs.  Added namespaces.  Did a few other coding standards updates too
//
//

#endif
