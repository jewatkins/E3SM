#include "catch2/catch.hpp"

//#include "share/scream_types.hpp"
#include <algorithm>
#include <array>
#include <random>
#include <thread>

#include "ekat/scream_kokkos.hpp"
#include "ekat/scream_pack.hpp"
#include "ekat/scream_types.hpp"
#include "ekat/util/scream_arch.hpp"
#include "ekat/util/scream_kokkos_utils.hpp"
#include "ekat/util/scream_utils.hpp"
#include "physics/share/physics_constants.hpp"
#include "physics/shoc/shoc_functions.hpp"
#include "physics/shoc/shoc_functions_f90.hpp"
#include "shoc_unit_tests_common.hpp"

namespace scream {
namespace shoc {
namespace unit_test {

TEST_CASE("shoc_conv_time_length", "shoc") {
  constexpr Int shcol    = 5;

  // Tests to compute the eddy turnover timescale from the 
  //   subroutine compute_conv_time_shoc_length.
  
  // FIRST TEST    
  //  Verify that no matter the input that conv_vel is always positive
  //  The main concern is that if input conv_vel is negative that
  //  we avoid taking the cube root of this value in the 
  //  subroutine call

  // PBL height [m] 
  Real pblh[shcol] = {1000.0, 400.0, 10.0, 500.0, 300.0};
  // Integrated convective velocity [m3/s3]
  Real conv_vel[shcol] = {10.0, -3.5, 0.1, -100.0, -0.4};

  // Initialzie data structure for bridgeing to F90
  SHOCConvTimeData SDS(shcol);

  // Test that the inputs are reasonable.
  REQUIRE(SDS.shcol > 0);

  // Fill in test data, all one dimensional
  for(Int s = 0; s < SDS.shcol; ++s) {
    SDS.pblh[s] = pblh[s];
    SDS.conv_vel[s] = conv_vel[s];
  }

  // Check that the inputs make sense

  for(Int s = 0; s < SDS.shcol; ++s) {
    REQUIRE(SDS.pblh[s] > 0.0);
  }

  // Call the fortran implementation
  compute_conv_time_shoc_length(nlev, SDS);

  // Check the results
  // Make sure that conv_vel is negative
  for(Int s = 0; s < SDS.shcol; ++s) {   
    REQUIRE(SDS.tscale[s] > 0.0);
  } 
  
  // SECOND TEST 
  // Constant PBL depth test
  // Given a set of inputs where PBL depth is constant but
  //  convective velocity scale changes, verify that the 
  //  turn over time scale is inversely proportional to 
  //  convective scale.  All values of convective scale
  //  must be positive for this test.  
  
  // PBL height [m] 
  Real pblh_cons1[shcol] = {500.0, 500.0, 500.0, 500.0, 500.0};
  // Integrated convective velocity [m3/s3]
  Real conv_vel_cons1[shcol] = {10.0, 3.5, 0.1, 100.0, 0.4};  

  // Fill in test data, all one dimensional
  for(Int s = 0; s < SDS.shcol; ++s) {
    SDS.pblh[s] = pblh_cons1[s];
    SDS.conv_vel[s] = conv_vel_cons1[s];
  }

  // Check that the inputs make sense

  for(Int s = 0; s < SDS.shcol; ++s) {
    REQUIRE(SDS.pblh[s] > 0.0);
    // For this test all values of conv_vel must be positive
    REQUIRE(SDS.conv_vel[s] > 0.0);
  }
  
  // Call the fortran implementation
  compute_conv_time_shoc_length(nlev, SDS);
  
  // Verify Results
  // Make sure that if conv_vel is smaller than in a particular 
  //  column, that the tscale is larger
  for (Int s = 0; s < SDS.shcol-1; ++s){
    if (SDS.tscale[s] > SDS.tscale[s+1]){
      REQUIRE(SDS.conv_vel[s] < SDS.conv_vel[s+1]);
    }
  }  
  
  // THIRD TEST 
  // Constant conv_vel test
  // Given a set of inputs where conv_vel is constant but
  //  boundary layer depth changes, verify that the 
  //  turn over time scale is proportional to 
  //  PBL height.  All values of convective scale
  //  must be positive for this test.  
  
  // PBL height [m] 
  Real pblh_cons2[shcol] = {500.0, 100.0, 10.0, 5000.0, 750.0};
  // Integrated convective velocity [m3/s3]
  Real conv_vel_cons2[shcol] = {0.5, 0.5, 0.5, 0.5, 0.5};  

  // Fill in test data, all one dimensional
  for(Int s = 0; s < SDS.shcol; ++s) {
    SDS.pblh[s] = pblh_cons2[s];
    SDS.conv_vel[s] = conv_vel_cons2[s];
  }

  // Check that the inputs make sense

  for(Int s = 0; s < SDS.shcol; ++s) {
    REQUIRE(SDS.pblh[s] > 0.0);
    // For this test all values of conv_vel must be positive
    REQUIRE(SDS.conv_vel[s] > 0.0);
  }
  
  // Call the fortran implementation
  compute_conv_time_shoc_length(nlev, SDS);
  
  // Verify Results
  // Make sure that if conv_vel is smaller than in a particular 
  //  column, that the tscale is larger
  for (Int s = 0; s < SDS.shcol-1; ++s){
    if (SDS.tscale[s] > SDS.tscale[s+1]){
      REQUIRE(SDS.pblh[s] > SDS.pblh[s+1]);
    }
  }   

}

}  // namespace unit_test
}  // namespace shoc
}  // namespace scream
