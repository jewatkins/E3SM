set(CXX_LINKER "CXX")
string(APPEND FC_AUTO_R8 " -fdefault-real-8")
string(APPEND FFLAGS " -O -fconvert=big-endian -ffree-line-length-none -ffixed-line-length-none -fno-range-check")
string(APPEND FIXEDFLAGS " -ffixed-form")
string(APPEND FREEFLAGS " -ffree-form")
set(HDF5_PATH "$ENV{HDF5_PATH}")
set(NETCDF_PATH "$ENV{NETCDF_PATH}")
set(NETCDF_C_PATH "$ENV{NETCDF_PATH}")
set(NETCDF_FORTRAN_PATH "$ENV{NETCDF_PATH}")
set(PNETCDF_PATH "$ENV{PNETCDF_PATH}")
set(LAPACK_LIBDIR "$ENV{BLASLAPACK_LIBDIR}")
if (MPILIB STREQUAL openmpi3)
  string(APPEND SLIBS " -L${NETCDF_FORTRAN_PATH}/lib64 -Wl,-rpath=${NETCDF_FORTRAN_PATH}/lib64 -lnetcdff")
endif()
if (MPILIB STREQUAL openmpi)
  string(APPEND SLIBS " -L${NETCDF_FORTRAN_PATH}/lib -Wl,-rpath=${NETCDF_FORTRAN_PATH}/lib -lnetcdff")
endif()
if (NOT MPILIB STREQUAL mpi-serial)
  string(APPEND SLIBS " -L${NETCDF_C_PATH}/lib -Wl,-rpath=${NETCDF_C_PATH}/lib -lnetcdf")
endif()
if (MPILIB STREQUAL mpi-serial)
  string(APPEND SLIBS " -L$ENV{NETCDF_PATH}/lib -Wl,-rpath=$ENV{NETCDF_PATH}/lib -lnetcdff -lnetcdf")
endif()
set(MPICC "mpicc")
set(MPICXX "mpic++")
set(MPIFC "mpif90")
set(SCC "gcc")
set(SCXX "g++")
set(SFC "gfortran")
set(SUPPORTS_CXX "TRUE")
