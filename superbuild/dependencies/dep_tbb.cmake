## Copyright 2019-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME tbb)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

set(TBB_HASH_ARGS "")
if (NOT "${TBB_HASH}" STREQUAL "")
  set(TBB_HASH_ARGS URL_HASH SHA256=${TBB_HASH})
endif()

if (BUILD_TBB_FROM_SOURCE)
  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}/build
    LIST_SEPARATOR | # Use the alternate list separator
    URL ${TBB_URL}
    ${TBB_HASH_ARGS}
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      -DCMAKE_BUILD_TYPE=${DEPENDENCIES_BUILD_TYPE}
      -DTBB_TEST=OFF
      -DTBB_STRICT=OFF
      -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS ${ALWAYS_REBUILD}
  )
else()
  # handle changed paths in TBB 2021
  if (TBB_VERSION VERSION_LESS 2021)
    set(TBB_SOURCE_INCLUDE_DIR tbb/include)
    set(TBB_SOURCE_LIB_DIR tbb/lib/${TBB_LIB_SUBDIR})
    set(TBB_SOURCE_BIN_DIR tbb/bin/${TBB_LIB_SUBDIR})
  else()
    set(TBB_SOURCE_INCLUDE_DIR include)
    set(TBB_SOURCE_LIB_DIR lib/${TBB_LIB_SUBDIR})
    set(TBB_SOURCE_BIN_DIR redist/${TBB_LIB_SUBDIR})
  endif()

  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}
    URL ${TBB_URL}
    ${TBB_HASH_ARGS}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory
      <SOURCE_DIR>/${TBB_SOURCE_INCLUDE_DIR}
      ${COMPONENT_PATH}/include
    BUILD_ALWAYS OFF
  )

  # We copy the libraries into the main lib dir. This makes it easier
  # to set the correct library path.
  ExternalProject_Add_Step(${COMPONENT_NAME} install_lib
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    <SOURCE_DIR>/${TBB_SOURCE_LIB_DIR} ${COMPONENT_PATH}/lib
    DEPENDEES install
  )

  if (WIN32)
    # DLLs on Windows are in the bin subdirectory.
    ExternalProject_Add_Step(${COMPONENT_NAME} install_dll
      COMMAND "${CMAKE_COMMAND}" -E copy_directory
      <SOURCE_DIR>/${TBB_SOURCE_BIN_DIR} ${COMPONENT_PATH}/bin
      DEPENDEES install_lib
    )
  endif()
endif()

set(TBB_PATH ${COMPONENT_PATH})

add_to_prefix_path(${COMPONENT_PATH})
