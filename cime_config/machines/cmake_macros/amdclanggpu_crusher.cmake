if (NOT DEBUG)
  string(APPEND CFLAGS " -O2 -I$ENV{MPICH_DIR}/include -I$ENV{ROCM_PATH}/include")
  string(APPEND FFLAGS " -O2 -I$ENV{MPICH_DIR}/include -I$ENV{ROCM_PATH}/include")
  string(APPEND HIP_FLAGS " -O3 -D__HIP_ROCclr__ -D__HIP_ARCH_GFX90A__=1 --rocm-path=$ENV{ROCM_PATH} --offload-arch=gfx90a -x hip")
endif()
if (DEBUG)
  string(APPEND HIP_FLAGS " -O0 -g -D__HIP_ROCclr__ -D__HIP_ARCH_GFX90A__=1 --rocm-path=$ENV{ROCM_PATH} --offload-arch=gfx90a -x hip")
endif()
string(APPEND SLIBS " -L$ENV{PNETCDF_PATH}/lib -lpnetcdf -L$ENV{CRAY_LIBSCI_PREFIX_DIR}/lib -lsci_amd")
set(PNETCDF_PATH "$ENV{PNETCDF_DIR}")
set(CRAY_LIBSCI_PREFIX_DIR "$ENV{CRAY_LIBSCI_PREFIX_DIR}")
string(APPEND CPPDEFS " -DNO_R16 -DCPRAMD -DFORTRANUNDERSCORE")
set(PIO_FILESYSTEM_HINTS "gpfs")
set(SUPPORTS_CXX "TRUE")
set(CXX_LINKER "FORTRAN")
set(CMAKE_HIP_COMPILER_ENV_VAR "HIPCXX")
set(CMAKE_HIP_COMPILER_LOADED 1)
set(CMAKE_HIP_COMPILER_ID_RUN 1) 
set(MPICC "cc")
set(MPICXX "CC")
set(MPIFC "ftn")
set(SCC "cc")
set(SCXX "CC")
set(SFC "ftn")
set(PIO_FILESYSTEM_HINTS "gpfs")
set(NETCDF_PATH "$ENV{NETCDF_DIR}")
set(PNETCDF_PATH "$ENV{PNETCDF_DIR}")
set(USE_HIP "TRUE")

