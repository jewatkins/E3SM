#include "ekat/ekat_assert.hpp"
#include "physics/rrtmgp/scream_rrtmgp_interface.hpp"
#include "physics/rrtmgp/atmosphere_radiation.hpp"
#include "cpp/rrtmgp/mo_gas_concentrations.h"
#include "YAKL/YAKL.h"

namespace scream {

RRTMGPRadiation::RRTMGPRadiation (const ekat::Comm& comm, const ekat::ParameterList& params) 
  : AtmosphereProcess::AtmosphereProcess(), m_rrtmgp_comm (comm), m_rrtmgp_params (params) {
}  // RRTMGPRadiation::RRTMGPRadiation

void RRTMGPRadiation::set_grids(const std::shared_ptr<const GridsManager> grids_manager) {

  using namespace ekat::units;

  auto kgkg = kg/kg;
  kgkg.set_string("kg/kg");
  auto m3 = m * m * m;
  auto Wm2 = W / m / m;
  Wm2.set_string("Wm2");
  auto nondim = m/m;  // dummy unit for non-dimensional fields
  auto micron = m / 1000000;

  using namespace ShortFieldTagsNames;

  auto grid = grids_manager->get_grid("Physics");
  m_ncol = grid->get_num_local_dofs();
  m_nlay = grid->get_num_vertical_levels();

  // Set up dimension layouts
  FieldLayout scalar2d_layout     { {COL   }, {m_ncol    } };
  FieldLayout scalar3d_layout_mid { {COL,LEV}, {m_ncol,m_nlay} };
  FieldLayout scalar3d_layout_int { {COL,LEV}, {m_ncol,m_nlay+1} };
  // Use VAR field tag for gases for now; consider adding a tag?
  FieldLayout gas_layout          { {COL,LEV,NGAS}, {m_ncol,m_nlay,m_ngas} };
  FieldLayout scalar2d_swband_layout { {COL,SWBND}, {m_ncol,m_nswbands} };

  // Set required (input) fields here
  add_field<Required>("p_mid" , scalar3d_layout_mid, Pa, grid->name());
  add_field<Required>("pint", scalar3d_layout_int, Pa, grid->name());
  add_field<Required>("T_mid" , scalar3d_layout_mid, K , grid->name());
  add_field<Required>("tint" , scalar3d_layout_int, K , grid->name());
  add_field<Required>("gas_vmr", gas_layout, kgkg, grid->name());
  add_field<Required>("surf_alb_direct", scalar2d_swband_layout, nondim, grid->name());
  add_field<Required>("surf_alb_diffuse", scalar2d_swband_layout, nondim, grid->name());
  add_field<Required>("cos_zenith", scalar2d_layout, nondim, grid->name());
  add_field<Required>("lwp", scalar3d_layout_mid, kg/m3, grid->name());
  add_field<Required>("iwp", scalar3d_layout_mid, kg/m3, grid->name());
  add_field<Required>("eff_radius_qc", scalar3d_layout_mid, micron, grid->name());
  add_field<Required>("eff_radius_qi", scalar3d_layout_mid, micron, grid->name());

  // Set computed (output) fields
  add_field<Computed>("SW_flux_dn", scalar3d_layout_int, Wm2, grid->name());
  add_field<Computed>("SW_flux_up", scalar3d_layout_int, Wm2, grid->name());
  add_field<Computed>("SW_flux_dn_dir", scalar3d_layout_int, Wm2, grid->name());
  add_field<Computed>("LW_flux_up", scalar3d_layout_int, Wm2, grid->name());
  add_field<Computed>("LW_flux_dn", scalar3d_layout_int, Wm2, grid->name());

}  // RRTMGPRadiation::set_grids

void RRTMGPRadiation::initialize_impl(const util::TimeStamp& /* t0 */) {
  // Names of active gases
  // TODO: this needs to be not hard-coded...I wanted to get these from
  // input files, but need to get around the rrtmgp_initializer logic.
  // Maybe can store the gas names somewhere else?
  string1d gas_names_1d("gas_names",m_ngas);
  for (int igas = 1; igas <= m_ngas; igas++) {
      gas_names_1d(igas) = m_gas_names[igas-1];
  }

  // Initialize GasConcs object to pass to RRTMGP initializer;
  // This is just to provide gas names
  // Make GasConcs from gas_vmr and gas_names_1d
  GasConcs gas_concs;
  gas_concs.init(gas_names_1d,1,1);
  rrtmgp::rrtmgp_initialize(gas_concs);
}

void RRTMGPRadiation::run_impl (const Real /* dt */) {
  // Get data from AD; RRTMGP wants YAKL views
  // TODO: how can I just keep these around without having to create every time?
  // They are just pointers, so should be able to keep them somewhere else and just associate them once?
  // Get device views
  auto d_pmid = m_rrtmgp_fields_in.at("p_mid").get_reshaped_view<const Real**>();
  auto d_pint = m_rrtmgp_fields_in.at("pint").get_reshaped_view<const Real**>();
  auto d_tmid = m_rrtmgp_fields_in.at("T_mid").get_reshaped_view<const Real**>();
  auto d_tint = m_rrtmgp_fields_in.at("tint").get_reshaped_view<const Real**>();
  auto d_gas_vmr = m_rrtmgp_fields_in.at("gas_vmr").get_reshaped_view<const Real***>();
  auto d_sfc_alb_dir = m_rrtmgp_fields_in.at("surf_alb_direct").get_reshaped_view<const Real**>();
  auto d_sfc_alb_dif = m_rrtmgp_fields_in.at("surf_alb_diffuse").get_reshaped_view<const Real**>();
  auto d_mu0 = m_rrtmgp_fields_in.at("cos_zenith").get_reshaped_view<const Real*>();
  auto d_lwp = m_rrtmgp_fields_in.at("lwp").get_reshaped_view<const Real**>();
  auto d_iwp = m_rrtmgp_fields_in.at("iwp").get_reshaped_view<const Real**>();
  auto d_rel = m_rrtmgp_fields_in.at("eff_radius_qc").get_reshaped_view<const Real**>();
  auto d_rei = m_rrtmgp_fields_in.at("eff_radius_qi").get_reshaped_view<const Real**>();
  auto d_sw_flux_up = m_rrtmgp_fields_out.at("SW_flux_up").get_reshaped_view<Real**>();
  auto d_sw_flux_dn = m_rrtmgp_fields_out.at("SW_flux_dn").get_reshaped_view<Real**>();
  auto d_sw_flux_dn_dir = m_rrtmgp_fields_out.at("SW_flux_dn_dir").get_reshaped_view<Real**>();
  auto d_lw_flux_up = m_rrtmgp_fields_out.at("LW_flux_up").get_reshaped_view<Real**>();
  auto d_lw_flux_dn = m_rrtmgp_fields_out.at("LW_flux_dn").get_reshaped_view<Real**>();

  // Map to YAKL
  yakl::Array<double,2,memDevice,yakl::styleFortran> p_lay  ("p_lay", const_cast<Real*>(d_pmid.data()), m_ncol, m_nlay);
  yakl::Array<double,2,memDevice,yakl::styleFortran> t_lay  ("t_lay", const_cast<Real*>(d_tmid.data()), m_ncol, m_nlay);
  yakl::Array<double,2,memDevice,yakl::styleFortran> p_lev  ("p_lev",const_cast<Real*>(d_pint.data()), m_ncol, m_nlay+1);
  yakl::Array<double,2,memDevice,yakl::styleFortran> t_lev  ("t_lev",const_cast<Real*>(d_tint.data()), m_ncol, m_nlay+1);
  yakl::Array<double,3,memDevice,yakl::styleFortran> gas_vmr("gas_vmr",const_cast<Real*>(d_gas_vmr.data()), m_ncol, m_nlay, m_ngas);
  yakl::Array<double,2,memDevice,yakl::styleFortran> sfc_alb_dir("surf_alb_direct",const_cast<Real*>(d_sfc_alb_dir.data()), m_ncol, m_nswbands);
  yakl::Array<double,2,memDevice,yakl::styleFortran> sfc_alb_dif("surf_alb_diffuse",const_cast<Real*>(d_sfc_alb_dif.data()), m_ncol, m_nswbands);
  yakl::Array<double,1,memDevice,yakl::styleFortran> mu0("cos_zenith",const_cast<Real*>(d_mu0.data()), m_ncol);
  yakl::Array<double,2,memDevice,yakl::styleFortran> lwp("lwp",const_cast<Real*>(d_lwp.data()), m_ncol, m_nlay);
  yakl::Array<double,2,memDevice,yakl::styleFortran> iwp("iwp",const_cast<Real*>(d_iwp.data()), m_ncol, m_nlay);
  yakl::Array<double,2,memDevice,yakl::styleFortran> rel("eff_radius_qc",const_cast<Real*>(d_rel.data()), m_ncol, m_nlay);
  yakl::Array<double,2,memDevice,yakl::styleFortran> rei("eff_radius_qi",const_cast<Real*>(d_rei.data()), m_ncol, m_nlay);
  yakl::Array<double,2,memDevice,yakl::styleFortran> sw_flux_up("SW_flux_up", d_sw_flux_up.data(), m_ncol, m_nlay+1);
  yakl::Array<double,2,memDevice,yakl::styleFortran> sw_flux_dn("SW_flux_dn", d_sw_flux_dn.data(), m_ncol, m_nlay+1);
  yakl::Array<double,2,memDevice,yakl::styleFortran> sw_flux_dn_dir("SW_flux_dn_dir", d_sw_flux_dn_dir.data(), m_ncol, m_nlay+1);
  yakl::Array<double,2,memDevice,yakl::styleFortran> lw_flux_up("LW_flux_up", d_lw_flux_up.data(), m_ncol, m_nlay+1);
  yakl::Array<double,2,memDevice,yakl::styleFortran> lw_flux_dn("LW_flux_dn", d_lw_flux_dn.data(), m_ncol, m_nlay+1);

  // Make GasConcs from gas_vmr and gas_names
  // TODO: only allocate at initialization and 
  // just update values here
  string1d gas_names_1d("gas_names",m_ngas);
  for (int igas = 1; igas <= m_ngas; igas++) {
      gas_names_1d(igas) = m_gas_names[igas-1];
  }
  
  // Create and populate a GasConcs object to pass to RRTMGP driver
  GasConcs gas_concs;
  gas_concs.init(gas_names_1d,m_ncol,m_nlay);
  real2d tmp2d;
  tmp2d = real2d("tmp", m_ncol, m_nlay);
  for (int igas = 1; igas <= m_ngas; igas++) {
      parallel_for(Bounds<2>(m_ncol,m_nlay), YAKL_LAMBDA(int icol, int ilay) {
          tmp2d(icol,ilay) = gas_vmr(icol,ilay,igas);
      });
      gas_concs.set_vmr(gas_names_1d(igas), tmp2d);
  }

  // Run RRTMGP driver
  rrtmgp::rrtmgp_main( 
    p_lay, t_lay, p_lev, t_lev,
    gas_concs,
    sfc_alb_dir, sfc_alb_dif, mu0,
    lwp, iwp, rel, rei,
    sw_flux_up, sw_flux_dn, sw_flux_dn_dir,
    lw_flux_up, lw_flux_dn
  );
}

void RRTMGPRadiation::finalize_impl  () {
  rrtmgp::rrtmgp_finalize();
}

// Private function to check that fields are not padded
void RRTMGPRadiation::require_unpadded(const Field<const Real>& f) {
  const auto& fid = f.get_header().get_identifier();
  const auto& layout = fid.get_layout();
  auto v = f.get_view();
  EKAT_REQUIRE_MSG (
      static_cast<size_t>(layout.size()) == v.size(), 
      "ERROR: field " << fid.name() << " was padded (to allow packing), but currently RRTMGP does not work with padded views."
  );
}

void RRTMGPRadiation::set_required_field_impl(const Field<const Real>& f) {
  const auto& name = f.get_header().get_identifier().name();
  m_rrtmgp_fields_in.emplace(name,f);
  m_rrtmgp_host_views_in[name] = Kokkos::create_mirror_view(f.get_view());
  m_raw_ptrs_in[name] = m_rrtmgp_host_views_in[name].data();

  // Add myself as customer to the field
  add_me_as_customer(f);

  // Check to make sure that fields are not padded because we are not 
  // sure how to handle that with RRTMGP yet
  require_unpadded(f);
}

void RRTMGPRadiation::set_computed_field_impl(const Field<      Real>& f) {
  const auto& name = f.get_header().get_identifier().name();
  m_rrtmgp_fields_out.emplace(name,f);
  m_rrtmgp_host_views_out[name] = Kokkos::create_mirror_view(f.get_view());
  m_raw_ptrs_out[name] = m_rrtmgp_host_views_out[name].data();

  // Add myself as provider for the field
  add_me_as_provider(f);

  // Check to make sure that fields are not padded because we are not 
  // sure how to handle that with RRTMGP yet
  require_unpadded(f);
}

}  // namespace scream
