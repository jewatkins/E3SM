set(ALBANY_PATH "/projects/ccsm/AlbanyTrilinos_20190904/albany-build/install")
if (COMP_NAME STREQUAL gptl)
  string(APPEND CPPDEFS " -DHAVE_SLASHPROC")
endif()
set(ESMF_LIBDIR "/projects/ccsm/esmf-6.3.0rp1/lib/libO/Linux.intel.64.openmpi.default")
if (MPILIB STREQUAL openmpi)
  set(MPI_PATH "$ENV{MPIHOME}")
endif()
set(NETCDF_PATH "$ENV{NETCDFROOT}")
if (MPILIB STREQUAL mpi-serial AND NOT compile_threaded)
  set(PFUNIT_PATH "/projects/ccsm/pfunit/3.2.9/mpi-serial")
endif()
set(PIO_FILESYSTEM_HINTS "lustre")
set(PNETCDF_PATH "$ENV{PNETCDFROOT}")
execute_process(COMMAND ${NETCDF_PATH}/bin/nf-config --flibs OUTPUT_VARIABLE SHELL_CMD_OUTPUT_BUILD_INTERNAL_IGNORE0 OUTPUT_STRIP_TRAILING_WHITESPACE)
string(APPEND SLIBS " ${SHELL_CMD_OUTPUT_BUILD_INTERNAL_IGNORE0} -L/projects/ccsm/BLAS-intel -lblas_LINUX -L$ENV{MKL_LIBS} -lmkl_rt")
if (MPILIB STREQUAL openmpi)
  string(APPEND SLIBS " -mkl=cluster")
endif()
if (MPILIB STREQUAL mpi-serial)
  string(APPEND SLIBS " -mkl")
endif()
