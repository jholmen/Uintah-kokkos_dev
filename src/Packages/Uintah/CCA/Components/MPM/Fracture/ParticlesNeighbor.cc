#include "ParticlesNeighbor.h"

#include "CellsNeighbor.h"
#include "Lattice.h"
#include "Cell.h"
#include "LeastSquare.h"
#include <Core/Exceptions/InternalError.h>

#include <Packages/Uintah/CCA/Components/MPM/Util/Matrix3.h>

#include <Packages/Uintah/Core/Grid/Patch.h>

#include <iostream>

namespace Uintah {
using namespace SCIRun;

ParticlesNeighbor::ParticlesNeighbor()
: std::vector<particleIndex>()
{
}

void ParticlesNeighbor::buildIn(const IntVector& cellIndex,const Lattice& lattice)
{
  CellsNeighbor cellsNeighbor;
  cellsNeighbor.buildIncluding(cellIndex,lattice);
  
  for(CellsNeighbor::const_iterator iter_cell = cellsNeighbor.begin();
    iter_cell != cellsNeighbor.end();
    ++iter_cell )
  {
    std::vector<particleIndex>& parts = lattice[*iter_cell].particles;
    for( std::vector<particleIndex>::const_iterator iter_p = parts.begin();
         iter_p != parts.end();
         ++iter_p )
    {
      push_back(*iter_p);
    }
  }
}

/*
void  ParticlesNeighbor::interpolateVector(LeastSquare& ls,
                          const particleIndex& pIdx,
                          const ParticleVariable<Vector>& pVector,
                          Vector& data,
                          Matrix3& gradient) const
{
  Vector v;
  for(int i=0;i<3;++i) {
    ls.clean();
    for(const_iterator pIter = begin(); pIter != end(); pIter++) {
      ls.input( (*d_pX)[*pIter]-(*d_pX)[pIdx], pVector[*pIter](i) );
    }
    ls.output( data(i),v );
    for(int j=0;j<3;++j) {
      gradient(i,j) = v(j);
    }
  }
}

void  ParticlesNeighbor::interpolatedouble(LeastSquare& ls,
                          const particleIndex& pIdx,
                          const ParticleVariable<double>& pdouble,
                          double& data,
                          Vector& gradient) const
{
  ls.clean();
  for(const_iterator pIter = begin(); pIter != end(); pIter++) {
    ls.input( (*d_pX)[*pIter]-(*d_pX)[pIdx], pdouble[*pIter] );
  }
  ls.output( data,gradient );
}

void  ParticlesNeighbor::interpolateInternalForce(LeastSquare& ls,
                          const particleIndex& pIdx,
                          const ParticleVariable<Matrix3>& pStress,
                          Vector& pInternalForce) const
{
  Vector v;
  double data;
  for(int i=0;i<3;++i) {
    pInternalForce(i) = 0;
    for(int j=0;j<3;++j) {
      ls.clean();
      for(const_iterator pIter = begin(); pIter != end(); pIter++) {
        ls.input( (*d_pX)[*pIter]-(*d_pX)[pIdx], pStress[*pIter](i,j) );
      }
    }
    ls.output( data,v );
    pInternalForce(i) -= v(i);
  }
}
*/

bool ParticlesNeighbor::visible(particleIndex idx,
                                const Point& B,
				const ParticleVariable<Point>& pX,
				const ParticleVariable<int>& pIsBroken,
				const ParticleVariable<Vector>& pCrackSurfaceNormal,
				const ParticleVariable<double>& pVolume) const
{
  const Point& A = pX[idx];
  if(pIsBroken[idx]) {
    if( Dot( B - A, pCrackSurfaceNormal[idx] ) > 0 ) return false;
  }
  
  for(int i=0; i<(int)size(); i++) {
      int index = (*this)[i];
      
      if( index != idx && pIsBroken[index] ) {
        const Vector& N = pCrackSurfaceNormal[index];
        double size2 = pow(pVolume[index] *0.75/M_PI,0.666666667);
        const Point& O = pX[index];

	double A_N = Dot(A,N);
	
        double a = A_N - Dot(O,N);
        double b = A_N - Dot(B,N);
	
	if(b != 0) {
	  double lambda = a/b;
	  if( lambda>=0 && lambda<=1 ) {
	    Point p( A.x() * (1-lambda) + B.x() * lambda,
                     A.y() * (1-lambda) + B.y() * lambda,
		     A.z() * (1-lambda) + B.z() * lambda );
 	    if( (p - O).length2() < size2 ) return false;
	  }
	}
      }
  }
  return true;
}

bool ParticlesNeighbor::visible(particleIndex idxA,
                                particleIndex idxB,
				const ParticleVariable<Point>& pX,
				const ParticleVariable<int>& pIsBroken,
				const ParticleVariable<Vector>& pCrackSurfaceNormal,
				const ParticleVariable<double>& pVolume) const
{
  const Point& A = pX[idxA];
  const Point& B = pX[idxB];
  Vector d = B - A;
  d.normalize();
  
  if(pIsBroken[idxA] && pIsBroken[idxB])  {
    if( Dot( d, pCrackSurfaceNormal[idxA] ) > 0.5 && 
        Dot( d, pCrackSurfaceNormal[idxB] ) < -0.5 ) return false;
  }
  else if(pIsBroken[idxA]) {
    if( Dot( d, pCrackSurfaceNormal[idxA] ) > 0.5 ) return false;
  }
  else if(pIsBroken[idxB]) {
    if( Dot( d, pCrackSurfaceNormal[idxB] ) < -0.5 ) return false;
  }
  
  for(int i=0; i<(int)size(); i++) {
      int index = (*this)[i];
      
      if( index != idxA && index != idxB && pIsBroken[index] ) {
        const Vector& N = pCrackSurfaceNormal[index];
        double size2 = pow(pVolume[index] *0.75/M_PI,0.666666667);
        const Point& O = pX[index];

	double A_N = Dot(A,N);
	
        double a = A_N - Dot(O,N);
        double b = A_N - Dot(B,N);
	
	if(b != 0) {
	  double lambda = a/b;
	  if( lambda>=0 && lambda<=1 ) {
	    Point p( A.x() * (1-lambda) + B.x() * lambda,
                     A.y() * (1-lambda) + B.y() * lambda,
		     A.z() * (1-lambda) + B.z() * lambda );
 	    if( (p - O).length2() < size2 ) return false;
	  }
	}
      }
  }
  return true;
}

} // End namespace Uintah
