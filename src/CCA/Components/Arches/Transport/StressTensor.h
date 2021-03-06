#ifndef Uintah_Component_Arches_StressTensor_h
#define Uintah_Component_Arches_StressTensor_h

#include <CCA/Components/Arches/Task/TaskInterface.h>
#include <CCA/Components/Arches/GridTools.h>

namespace Uintah{

  class StressTensor : public TaskInterface {

public:

    typedef std::vector<ArchesFieldContainer::VariableInformation> VIVec;

    StressTensor( std::string task_name, int matl_index );
    ~StressTensor();

    TaskAssignedExecutionSpace loadTaskComputeBCsFunctionPointers();

    TaskAssignedExecutionSpace loadTaskInitializeFunctionPointers();

    TaskAssignedExecutionSpace loadTaskEvalFunctionPointers();

    TaskAssignedExecutionSpace loadTaskTimestepInitFunctionPointers();

    TaskAssignedExecutionSpace loadTaskRestartInitFunctionPointers();

    void problemSetup( ProblemSpecP& db );

    void create_local_labels();

    void register_initialize( VIVec& variable_registry , const bool pack_tasks);

    void register_timestep_init( VIVec& variable_registry , const bool packed_tasks){};

    void register_restart_initialize( VIVec& variable_registry , const bool packed_tasks){};

    void register_timestep_eval( VIVec& variable_registry, const int time_substep , const bool packed_tasks);

    void register_compute_bcs( VIVec& variable_registry, const int time_substep , const bool packed_tasks){}

    template <typename ExecSpace, typename MemSpace>
    void compute_bcs( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecSpace, MemSpace>& execObj ){}

    template <typename ExecSpace, typename MemSpace>
    void initialize( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecSpace, MemSpace>& execObj );

    template <typename ExecSpace, typename MemSpace>
    void timestep_init( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecSpace, MemSpace>& execObj ){}

    template <typename ExecSpace, typename MemSpace>
    void eval( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecSpace, MemSpace>& execObj );

   template< typename grid_CT>
   struct VelocityDerivative_central{
     VelocityDerivative_central( grid_CT &_u, const Vector& _Dx) : u(_u),Dx(_Dx){}
       inline void operator()(double& dudx,double& dudy,double& dudz,int i, int j, int k)  const {
         {
           STENCIL3_1D(0);
           dudx = (u(IJK_) - u(IJK_M_))/Dx.x();
         }
         {
           STENCIL3_1D(1);
           dudy = (u(IJK_) - u(IJK_M_))/Dx.y();
         }
         {
           STENCIL3_1D(2);
           dudz = (u(IJK_) - u(IJK_M_))/Dx.z();
         }
       };
       const grid_CT& u;
       const Vector  Dx;
     };
    
     template< typename grid_CT> VelocityDerivative_central<grid_CT>
     functorCreationWrapper( grid_CT &_u, const Vector& _Dx) {
        return  VelocityDerivative_central<grid_CT>(_u,_Dx);
     } // WE have to do this because of CCVariables

#define dVeldDir(u, eps, Dx, dudx, dudy, dudz, i,  j, k ) \
         {                                           \
           STENCIL3_1D(0);                           \
           dudx = eps(IJK_)*eps(IJK_M_)*(u(IJK_) - u(IJK_M_))/Dx.x();      \
         }                                           \
         {                                           \
           STENCIL3_1D(1);                           \
           dudy = eps(IJK_)*eps(IJK_M_)*(u(IJK_) - u(IJK_M_))/Dx.y();      \
         }                                           \
         {                                           \
           STENCIL3_1D(2);                           \
           dudz = eps(IJK_)*eps(IJK_M_)*(u(IJK_) - u(IJK_M_))/Dx.z();      \
         }    
    //Build instructions for this class.
    class Builder : public TaskInterface::TaskBuilder {

      public:

      Builder( std::string task_name, int matl_index )
        : m_task_name(task_name), m_matl_index(matl_index){}
      ~Builder(){}

      StressTensor* build()
      { return scinew StressTensor( m_task_name, m_matl_index ); }

      private:

      std::string m_task_name;
      int m_matl_index;

    };

private:

    typedef std::vector<ArchesFieldContainer::VariableInformation> AVarInfo;

    std::string m_u_vel_name;
    std::string m_v_vel_name;
    std::string m_w_vel_name;
    std::string m_eps_x_name;
    std::string m_eps_y_name;
    std::string m_eps_z_name;
    std::string diff_scheme;
    std::string m_t_vis_name;
    std::vector<std::string> m_sigma_t_names;
    int Nghost_cells;


protected:



  };
}

#endif
