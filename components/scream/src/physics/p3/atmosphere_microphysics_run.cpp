#include "physics/p3/atmosphere_microphysics.hpp"

namespace scream {

void P3Microphysics::run_impl (const int dt)
{
  // Assign values to local arrays used by P3, these are now stored in p3_loc.
  Kokkos::parallel_for(
    "p3_main_local_vals",
    Kokkos::RangePolicy<>(0,m_num_cols),
    p3_preproc
  ); // Kokkos::parallel_for(p3_main_local_vals)
  Kokkos::fence();

  // Update the variables in the p3 input structures with local values.

  infrastructure.dt = dt;
  infrastructure.it++;

  // WorkspaceManager for internal local variables
  const Int nk_pack = ekat::npack<Spack>(m_num_levs);
  const auto policy = ekat::ExeSpaceUtils<KT::ExeSpace>::get_default_team_policy(m_num_cols, nk_pack);
  ekat::WorkspaceManager<Spack, KT::Device> workspace_mgr(m_buffer.wsm_data, nk_pack, 52, policy);

  for(int i=0; i<16;i++){
    for(int j=0; j<16;j++){
      //std::cout<<i<<" "<<j<<" "<<prog_state.qv(i,j)<<std::endl;
    }
  }
  std::cout<<"Before P3-main "<<prog_state.qv(16,16)<<std::endl;
  // Run p3 main
  P3F::p3_main(prog_state, diag_inputs, diag_outputs, infrastructure,
               history_only, workspace_mgr, m_num_cols, m_num_levs);
  std::cout<<"Afer P3-main "<<prog_state.qv(16,16)<<std::endl;
  for(int i=0; i<16;i++){
    for(int j=0; j<16;j++){
      //std::cout<<i<<" "<<j<<" "<<prog_state.qv(i,j)<<std::endl;
    }
  }
  //std::cout<<prog_state.qv(1,1)<<std::endl;
  // Conduct the post-processing of the p3_main output.
  Kokkos::parallel_for(
    "p3_main_local_vals",
    Kokkos::RangePolicy<>(0,m_num_cols),
    p3_postproc
  ); // Kokkos::parallel_for(p3_main_local_vals)
  Kokkos::fence();
}

} // namespace scream
