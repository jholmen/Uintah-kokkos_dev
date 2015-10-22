#ifndef Uintah_Component_Arches_RateDeposition_h
#define Uintah_Component_Arches_RateDeposition_h

#include <CCA/Components/Arches/Task/TaskInterface.h>

namespace Uintah{ 

  class Operators; 
  class Discretization_new; 
  class RateDeposition : public TaskInterface { 

public: 

    RateDeposition( std::string task_name, int matl_index, const int N ); 
    ~RateDeposition(); 

    void problemSetup( ProblemSpecP& db ); 

    void register_initialize( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry );

    void register_timestep_init( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry ); 

    void register_timestep_eval( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry, const int time_substep ); 

    void register_compute_bcs( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry, const int time_substep ); 

    void compute_bcs( const Patch* patch, ArchesTaskInfoManager* tsk_info, 
                      SpatialOps::OperatorDatabase& opr ); 

    void initialize( const Patch* patch, ArchesTaskInfoManager* tsk_info, 
                     SpatialOps::OperatorDatabase& opr );
    
    void timestep_init( const Patch* patch, ArchesTaskInfoManager* tsk_info, 
                        SpatialOps::OperatorDatabase& opr );

    void eval( const Patch* patch, ArchesTaskInfoManager* tsk_info, 
               SpatialOps::OperatorDatabase& opr ); 

    void create_local_labels(); 

    const std::string get_env_name( const int i, const std::string base_name ){
      std::stringstream out;
      std::string env;
      out << i;
      env = out.str();
      return base_name + "_" + env;
    }
         

    //Build instructions for this (RateDeposition) class. 
    class Builder : public TaskInterface::TaskBuilder { 

      public: 

      Builder( std::string task_name, int matl_index, const int N ) : _task_name(task_name), _matl_index(matl_index), _Nenv(N){}
      ~Builder(){}

      RateDeposition* build()
      { return scinew RateDeposition( _task_name, _matl_index , _Nenv ); }
      private: 
      std::string _task_name; 
      int _matl_index; 
      const int _Nenv;
    };

private: 
    int _Nenv;
    double _Tmelt;
    std::string _ParticleTemperature_base_name;
    std::string _ProbParticleX_base_name;    
    std::string _ProbParticleY_base_name;    
    std::string _ProbParticleZ_base_name;    

    std::string _ProbDepositionX_base_name;    
    std::string _ProbDepositionY_base_name;    
    std::string _ProbDepositionZ_base_name;    
  
    std::string _RateDepositionX_base_name;
    std::string _RateDepositionY_base_name;
    std::string _RateDepositionZ_base_name;
    std::string _diameter_base_name;
   

    std::string _ProbSurfaceX_name; 
    std::string _ProbSurfaceY_name; 
    std::string _ProbSurfaceZ_name; 
    std::string _xvel_base_name;
    std::string _yvel_base_name;
    std::string _zvel_base_name;
   
    std::string  _weight_base_name;
    std::string  _rho_base_name;
  
    std::string _FluxPx_base_name;
    std::string _FluxPy_base_name;
    std::string _FluxPz_base_name;
 
   std::string  _WallTemperature_name;

   template <class faceT, class velT> void
   compute_prob_stick(  SpatialOps::OperatorDatabase& opr, 
                                       const SpatialOps::SpatFldPtr<velT> normal,
                                       const SpatialOps::SpatFldPtr<velT> areaFraction,
                                       SpatialOps::SpatFldPtr<SpatialOps::SVolField> Tvol, 
                                       SpatialOps::SpatFldPtr<velT> ProbStick );

   template <class faceT, class velT> void
   flux_compute(  SpatialOps::OperatorDatabase& opr, 
                                  const SpatialOps::SpatFldPtr<velT> normal,
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> rho,
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> u,
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> weg, 
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> dia, 
                                  SpatialOps::SpatFldPtr<velT> flux );


  };

  template <class faceT, class velT> void 
  RateDeposition::compute_prob_stick(  SpatialOps::OperatorDatabase& opr, 
                                       const SpatialOps::SpatFldPtr<velT> normal,
                                       const SpatialOps::SpatFldPtr<velT> areaFraction,
                                       SpatialOps::SpatFldPtr<SpatialOps::SVolField> Tvol, 
                                       SpatialOps::SpatFldPtr<velT> ProbStick)
  { 
     const double Aprepontional= 2.1*pow(10,-13);  const double Bactivational= 47800;
     const double ReferVisc=10000;
    
    //interp the XVol to XSurf
    typedef typename SpatialOps::OperatorTypeBuilder< SpatialOps::Interpolant, velT, faceT>::type InterpVFtoSF;
    const InterpVFtoSF* const interp_xv_to_xf = opr.retrieve_operator<InterpVFtoSF>();
    SpatialOps::SpatFldPtr<faceT> normal_face = SpatialOps::SpatialFieldStore::get<faceT>( *normal);
    SpatialOps::SpatFldPtr<faceT> areaFraction_face = SpatialOps::SpatialFieldStore::get<faceT>( *normal);

    *normal_face <<= (*interp_xv_to_xf)(*normal);
    *areaFraction_face <<= (*interp_xv_to_xf)(*areaFraction);

    //create a volume to face interpolant
    typedef typename SpatialOps::OperatorTypeBuilder< SpatialOps::Interpolant, SpatialOps::SVolField, faceT>::type InterpSVtoSF; 
    const InterpSVtoSF* const i_v_to_s = opr.retrieve_operator<InterpSVtoSF>(); 

    //create some temp variables modeled on ufx
    SpatialOps::SpatFldPtr<faceT> Tvol_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );
    SpatialOps::SpatFldPtr<faceT> Fmelt_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );
    SpatialOps::SpatFldPtr<faceT> ProbMelt_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );
    SpatialOps::SpatFldPtr<faceT> ProbVisc_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );

    //get upwind (low) interpolant and the flux limiter operator
    typedef UpwindInterpolant<SpatialOps::SVolField, faceT> Upwind; 
    Upwind* upwind = opr.retrieve_operator<Upwind>(); 
 
    //compute the upwind differenced term
    upwind->set_advective_velocity( *normal_face );
    upwind->apply_to_field( *Tvol, *Tvol_temp );
    //interp the XSurf to XVol
    typedef typename SpatialOps::OperatorTypeBuilder< SpatialOps::Interpolant, faceT,velT>::type InterpSFtoVF;
    const InterpSFtoVF* const interp_xf_to_xv = opr.retrieve_operator<InterpSFtoVF>();
    
   //-----------------------Actual work here----------------------
   // Compute the melting probability
     *ProbMelt_temp <<= cond(*Tvol_temp >= _Tmelt, 1)
                            (0 );
   
   //compute the viscosity probability
    *ProbVisc_temp <<= Aprepontional* (*Tvol_temp)* exp(Bactivational /(*Tvol_temp) );
    *ProbVisc_temp <<= cond( ReferVisc/(*ProbVisc_temp) > 1, 1  )
                           ( ReferVisc/(*ProbVisc_temp));
    *ProbStick <<= (*interp_xf_to_xv)((1-*areaFraction_face) * *ProbVisc_temp * *ProbMelt_temp); 
     
  }
  //-------------------------------------------------------------------------------------
   template <class faceT, class velT> void
   RateDeposition::flux_compute(  SpatialOps::OperatorDatabase& opr,
                                  const SpatialOps::SpatFldPtr<velT> normal,
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> rho,
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> u,
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> weg, 
                                  const SpatialOps::SpatFldPtr<SpatialOps::SVolField> dia, 
                                  SpatialOps::SpatFldPtr<velT> flux )

   {
        //interp the XVol to XSurf
    typedef typename SpatialOps::OperatorTypeBuilder< SpatialOps::Interpolant, velT, faceT>::type InterpVFtoSF;
    const InterpVFtoSF* const interp_xv_to_xf = opr.retrieve_operator<InterpVFtoSF>();
    SpatialOps::SpatFldPtr<faceT> normal_face = SpatialOps::SpatialFieldStore::get<faceT>( *normal);

    *normal_face <<= (*interp_xv_to_xf)(*normal);

    //create a volume to face interpolant
    typedef typename SpatialOps::OperatorTypeBuilder< SpatialOps::Interpolant, SpatialOps::SVolField, faceT>::type InterpSVtoSF; 
    const InterpSVtoSF* const i_v_to_s = opr.retrieve_operator<InterpSVtoSF>(); 

    //create some temp variables modeled on ufx
    SpatialOps::SpatFldPtr<faceT> rho_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );
    SpatialOps::SpatFldPtr<faceT> u_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );
    SpatialOps::SpatFldPtr<faceT> weg_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );
    SpatialOps::SpatFldPtr<faceT> dia_temp = SpatialOps::SpatialFieldStore::get<faceT>( *normal_face );


    //get upwind (low) interpolant and the flux limiter operator
    typedef UpwindInterpolant<SpatialOps::SVolField, faceT> Upwind; 
    Upwind* upwind = opr.retrieve_operator<Upwind>(); 
 
    //compute the upwind differenced term
    upwind->set_advective_velocity( *normal_face );
    upwind->apply_to_field( *rho, *rho_temp );

    upwind->set_advective_velocity( *normal_face );
    upwind->apply_to_field( *rho, *rho_temp );
    
    upwind->set_advective_velocity( *normal_face );
    upwind->apply_to_field( *u, *u_temp );

    upwind->set_advective_velocity( *normal_face );
    upwind->apply_to_field( *weg, *weg_temp );
  
    upwind->set_advective_velocity( *normal_face );
    upwind->apply_to_field( *dia, *dia_temp );
   //interp the XSurf to XVol
    typedef typename SpatialOps::OperatorTypeBuilder< SpatialOps::Interpolant, faceT,velT>::type InterpSFtoVF;
    const InterpSFtoVF* const interp_xf_to_xv = opr.retrieve_operator<InterpSFtoVF>();
    *flux <<= (*interp_xf_to_xv)( *rho_temp * *u_temp * *weg_temp * 0.52 * pow(*dia_temp,3) );
   
   }
}
#endif 
