/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


/*
 * Manual template instantiations
 */


/*
 * These aren't used by Datatypes directly, but since they are used in
 * a lot of different modules, we instantiate them here to avoid bloat
 *
 * Find the bloaters with:
find . -name "*.ii" -print | xargs cat | sort | uniq -c | sort -nr | more
 */

#include <Core/Containers/LockingHandle.h>
#include <Core/Malloc/Allocator.h>


using namespace SCIRun;

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma set woff 1468
#endif

#include <Core/Geometry/Tensor.h>
#include <Core/Geometry/Vector.h>
#include <Core/Datatypes/GenericField.h>
#include <Core/Basis/CrvLinearLgn.h>
#include <Core/Basis/HexTrilinearLgn.h>
#include <Core/Basis/QuadBilinearLgn.h>
#include <Core/Containers/FData.h>
#include <Core/Datatypes/StructCurveMesh.h>
#include <Core/Datatypes/StructQuadSurfMesh.h>
#include <Core/Datatypes/StructHexVolMesh.h>

typedef CrvLinearLgn<Tensor>                CFDTensorBasis;
typedef CrvLinearLgn<Vector>                CFDVectorBasis;
typedef CrvLinearLgn<double>                CFDdoubleBasis;
typedef CrvLinearLgn<float>                 CFDfloatBasis;
typedef CrvLinearLgn<int>                   CFDintBasis;
typedef CrvLinearLgn<short>                 CFDshortBasis;
typedef CrvLinearLgn<char>                  CFDcharBasis;
typedef CrvLinearLgn<unsigned int>          CFDuintBasis;
typedef CrvLinearLgn<unsigned short>        CFDushortBasis;
typedef CrvLinearLgn<unsigned char>         CFDucharBasis;
typedef CrvLinearLgn<unsigned long>         CFDulongBasis;

typedef StructCurveMesh<CrvLinearLgn<Point> > CMesh;
template class GenericField<CMesh, CFDTensorBasis, vector<Tensor> >;       
template class GenericField<CMesh, CFDVectorBasis, vector<Vector> >;       
template class GenericField<CMesh, CFDdoubleBasis, vector<double> >;       
template class GenericField<CMesh, CFDfloatBasis,  vector<float> >;        
template class GenericField<CMesh, CFDintBasis,    vector<int> >;          
template class GenericField<CMesh, CFDshortBasis,  vector<short> >;        
template class GenericField<CMesh, CFDcharBasis,   vector<char> >;         
template class GenericField<CMesh, CFDuintBasis,   vector<unsigned int> >; 
template class GenericField<CMesh, CFDushortBasis, vector<unsigned short> >;
template class GenericField<CMesh, CFDucharBasis,  vector<unsigned char> >;
template class GenericField<CMesh, CFDulongBasis,  vector<unsigned long> >;


typedef QuadBilinearLgn<Tensor>                QFDTensorBasis;
typedef QuadBilinearLgn<Vector>                QFDVectorBasis;
typedef QuadBilinearLgn<double>                QFDdoubleBasis;
typedef QuadBilinearLgn<float>                 QFDfloatBasis;
typedef QuadBilinearLgn<int>                   QFDintBasis;
typedef QuadBilinearLgn<short>                 QFDshortBasis;
typedef QuadBilinearLgn<char>                  QFDcharBasis;
typedef QuadBilinearLgn<unsigned int>          QFDuintBasis;
typedef QuadBilinearLgn<unsigned short>        QFDushortBasis;
typedef QuadBilinearLgn<unsigned char>         QFDucharBasis;
typedef QuadBilinearLgn<unsigned long>         QFDulongBasis;

typedef StructQuadSurfMesh<QuadBilinearLgn<Point> > SQMesh;
template class GenericField<SQMesh, QFDTensorBasis, FData2d<Tensor,SQMesh> >;
template class GenericField<SQMesh, QFDVectorBasis, FData2d<Vector,SQMesh> >;
template class GenericField<SQMesh, QFDdoubleBasis, FData2d<double,SQMesh> >;
template class GenericField<SQMesh, QFDfloatBasis,  FData2d<float,SQMesh> >;
template class GenericField<SQMesh, QFDintBasis,    FData2d<int,SQMesh> >;
template class GenericField<SQMesh, QFDshortBasis,  FData2d<short,SQMesh> >;
template class GenericField<SQMesh, QFDcharBasis,   FData2d<char,SQMesh> >;
template class GenericField<SQMesh, QFDuintBasis,   FData2d<unsigned int,
							    SQMesh> >; 
template class GenericField<SQMesh, QFDushortBasis, FData2d<unsigned short,
							    SQMesh> >;
template class GenericField<SQMesh, QFDucharBasis,  FData2d<unsigned char,
							    SQMesh> >;
template class GenericField<SQMesh, QFDulongBasis,  FData2d<unsigned long,
							    SQMesh> >;


typedef HexTrilinearLgn<Tensor>                HFDTensorBasis;
typedef HexTrilinearLgn<Vector>                HFDVectorBasis;
typedef HexTrilinearLgn<double>                HFDdoubleBasis;
typedef HexTrilinearLgn<float>                 HFDfloatBasis;
typedef HexTrilinearLgn<int>                   HFDintBasis;
typedef HexTrilinearLgn<short>                 HFDshortBasis;
typedef HexTrilinearLgn<char>                  HFDcharBasis;
typedef HexTrilinearLgn<unsigned int>          HFDuintBasis;
typedef HexTrilinearLgn<unsigned short>        HFDushortBasis;
typedef HexTrilinearLgn<unsigned char>         HFDucharBasis;
typedef HexTrilinearLgn<unsigned long>         HFDulongBasis;

typedef StructHexVolMesh<HexTrilinearLgn<Point> > SHMesh;
template class GenericField<SHMesh, HFDTensorBasis, FData3d<Tensor,SHMesh> >;
template class GenericField<SHMesh, HFDVectorBasis, FData3d<Vector,SHMesh> >;
template class GenericField<SHMesh, HFDdoubleBasis, FData3d<double,SHMesh> >;
template class GenericField<SHMesh, HFDfloatBasis,  FData3d<float,SHMesh> >;
template class GenericField<SHMesh, HFDintBasis,    FData3d<int,SHMesh> >;
template class GenericField<SHMesh, HFDshortBasis,  FData3d<short,SHMesh> >;
template class GenericField<SHMesh, HFDcharBasis,   FData3d<char,SHMesh> >;
template class GenericField<SHMesh, HFDuintBasis,   FData3d<unsigned int,
							    SHMesh> >;
template class GenericField<SHMesh, HFDushortBasis, FData3d<unsigned short,
							    SHMesh> >;
template class GenericField<SHMesh, HFDucharBasis,  FData3d<unsigned char,
							    SHMesh> >;
template class GenericField<SHMesh, HFDulongBasis,  FData3d<unsigned long,
							    SHMesh> >;

#if defined(__sgi) && !defined(__GNUC__) && (_MIPS_SIM != _MIPS_SIM_ABI32)
#pragma reset woff 1468
#endif


