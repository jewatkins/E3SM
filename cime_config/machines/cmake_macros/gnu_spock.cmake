set(SUPPORTS_CXX "TRUE")
string(APPEND FFLAGS " -Wno-implicit-interface ")
string(APPEND CXX_LIBS " -lstdc++")
if (NOT DEBUG)
  string(APPEND FFLAGS " -O2")
  string(APPEND CFLAGS " -O2")
endif()
string(APPEND SLIBS " -L$ENV{PNETCDF_PATH}/lib -lpnetcdf")
set(PNETCDF_PATH "$ENV{PNETCDF_PATH}")
set(PIO_FILESYSTEM_HINTS "gpfs")
