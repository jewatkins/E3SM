#ifndef PYATMPROC_HPP
#define PYATMPROC_HPP

#include "share/atm_process/atmosphere_process.hpp"
#include "share/io/scorpio_input.hpp"
#include "pygrid.hpp"
#include "pyfield.hpp"

#include <pybind11/pybind11.h>

namespace scream {

struct PyAtmProc {
  std::shared_ptr<AtmosphereProcess> ap;
  PyGrid pygrid;
  std::map<std::string,PyField> fields;
  util::TimeStamp t0;
  util::TimeStamp time;
  ATMBufferManager buffer;

  PyAtmProc (const PyGrid& pyg)
   : pygrid(pyg)
   , t0({2000,1,1},{0,0,0})
  {
    // TODO: make t0 configurable?
  }

  std::map<std::string,PyField> create_fields () {
    std::map<std::string,PyField> pyfields;
    for (const auto& req : ap->get_required_field_requests()) {
      if (pyfields.count(req.fid.name())==0) {
        pyfields.emplace(req.fid.name(),PyField(req.fid,req.pack_size));
      }
    }
    for (const auto& req : ap->get_computed_field_requests()) {
      if (pyfields.count(req.fid.name())==0) {
        pyfields.emplace(req.fid.name(),PyField(req.fid,req.pack_size));
      }
    }

    return pyfields;
  }

  void set_fields(const std::map<std::string,PyField>& pyfields) {
    for (const auto& it : pyfields) {
      const auto& f = it.second.f;
      const auto& fid = f.get_header().get_identifier();
      if (ap->has_required_field(fid)) {
        ap->set_required_field(f.get_const());
        fields[fid.name()] = it.second;   
      }
      if (ap->has_computed_field(fid)) {
        ap->set_computed_field(f);
        fields[fid.name()] = it.second;   
      }
    }
  }

  pybind11::array get_arr(const std::string& name) {
    auto it = fields.find(name);

    auto fnames = [&]() {
      std::string s;
      for (auto it : fields) {
        if (s=="") {
          s += it.first;
        } else {
          s += ", " + it.first;
        }
      }
      return s;
    };
    EKAT_REQUIRE_MSG (it!=fields.end(),
        "Error! Field not found in list of P3 fields.\n"
        "  - field name: " + name + "\n"
        "  - p3 fields: " + fnames() + "\n");

    return it->second.get();
  }

  // If running as part of a process group, call the second function, after
  // manually creating/setting the fields
  void initialize (const std::string& t0_str) {
    int nbytes = ap->requested_buffer_size_in_bytes ();
    buffer.request_bytes(nbytes);
    buffer.allocate();
    ap->init_buffers(buffer);

    time = t0 = util::str_to_time_stamp(t0_str);
    for (auto it : fields) {
      auto& f = it.second.f;
      if (ap->has_required_field(f.get_header().get_identifier())) {
        f.get_header().get_tracking().update_time_stamp(t0);
      }
    }

    ap->initialize(t0,RunType::Initial);
  }

  pybind11::list read_ic (const std::string& ic_filename) {
    // Get input fields, and read them from file (if present).
    // If field is not in the IC, user is responsible for setting
    // it to an initial value
    std::vector<std::string> missing;
    std::vector<Field> ic_fields;
    scorpio::register_file(ic_filename,scorpio::Read);
    for (auto it : fields) {
      auto& f = it.second.f;
      if (ap->has_required_field(f.get_header().get_identifier())) {
        if (scorpio::has_var(ic_filename,f.name())) {
          ic_fields.push_back(f);
        } else {
          missing.push_back(f.name());
        }
      }
    }
    AtmosphereInput reader (ic_filename,pygrid.grid,ic_fields,true);
    reader.read_variables();
    scorpio::release_file(ic_filename);

    return pybind11::cast(missing);
  }

  void run (double dt) {
    ap->run(dt);
    time += dt;
  }
};

} // namespace scream

#endif // PYATMPROC_HPP
