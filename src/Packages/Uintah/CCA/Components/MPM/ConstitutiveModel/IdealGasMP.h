//  IdealGasMP.h 
//  class ConstitutiveModel ConstitutiveModel data type -- 3D - 
//  holds ConstitutiveModel
//  information for the FLIP technique:
//    This is for Compressible NeoHookean materials
//    Features:
//      Usage:



#ifndef __IDEALGAS_CONSTITUTIVE_MODEL_H__
#define __IDEALGAS_CONSTITUTIVE_MODEL_H__


#include <math.h>
#include "ConstitutiveModel.h"	
#include <Packages/Uintah/Core/Math/Matrix3.h>
#include <vector>
#include <Packages/Uintah/Core/Disclosure/TypeDescription.h>

namespace Uintah {
      class IdealGasMP : public ConstitutiveModel {
      private:
         // Create datatype for storing model parameters
      public:
         struct CMData {
            double gamma;
            double cv;
         };
      private:
         CMData d_initialData;

         // Prevent copying of this class
         // copy constructor
         IdealGasMP(const IdealGasMP &cm);
         IdealGasMP& operator=(const IdealGasMP &cm);
         int d_8or27;

      public:
         // constructors
         IdealGasMP(ProblemSpecP& ps,  MPMLabel* lb, int n8or27);
       
         // destructor
         virtual ~IdealGasMP();
         // compute stable timestep for this patch
         virtual void computeStableTimestep(const Patch* patch,
                                            const MPMMaterial* matl,
                                            DataWarehouse* new_dw);

         // compute stress at each particle in the patch
         virtual void computeStressTensor(const PatchSubset* patches,
                                          const MPMMaterial* matl,
                                          DataWarehouse* old_dw,
                                          DataWarehouse* new_dw);

         // initialize  each particle's constitutive model data
         virtual void initializeCMData(const Patch* patch,
                                       const MPMMaterial* matl,
                                       DataWarehouse* new_dw);

         virtual void addComputesAndRequires(Task* task,
                                             const MPMMaterial* matl,
                                             const PatchSet* patches) const;

         virtual double computeRhoMicroCM(double pressure,
                                          const double p_ref,
                                          const MPMMaterial* matl);

         virtual void computePressEOSCM(double rho_m, double& press_eos,
                                        const double p_ref,
                                        double& dp_drho, double& ss_new,
                                        const MPMMaterial* matl);

         virtual double getCompressibility();

         // class function to read correct number of parameters
         // from the input file
         static void readParameters(ProblemSpecP ps, double *p_array);

         // class function to write correct number of parameters
         // from the input file, and create a new object
         static ConstitutiveModel* readParametersAndCreate(ProblemSpecP ps);

         // member function to read correct number of parameters
         // from the input file, and any other particle information
         // need to restart the model for this particle
         // and create a new object
         static ConstitutiveModel* readRestartParametersAndCreate(
                                                        ProblemSpecP ps);

	 virtual void addParticleState(std::vector<const VarLabel*>& from,
				       std::vector<const VarLabel*>& to);
         // class function to create a new object from parameters
         static ConstitutiveModel* create(double *p_array);

	const VarLabel* bElBarLabel;
	const VarLabel* bElBarLabel_preReloc;

      };
} // End namespace Uintah
      


#endif  // __IDEALGAS_CONSTITUTIVE_MODEL_H__ 

