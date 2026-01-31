#
# Software License Agreement (BSD License)
#
# Copyright (c) 2014-2025 CNRS-LAAS, INRIA Author: Florent Lamiraux, Joseph
# Mirabel All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of CNRS-LAAS. nor the names of its contributors may be used
#   to endorse or promote products derived from this software without specific
#   prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME coal)
set(
  PROJECT_DESCRIPTION
  "Coal, The Collision Detection Library. Previously known as HPP-FCL, fork of FCL -- The Flexible Collision Library"
)
set(COAL_VERSION "0.0.0" CACHE STRING "Coal-lite version")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.version")
  file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/.version" COAL_VERSION)
  string(STRIP "${COAL_VERSION}" COAL_VERSION)
endif()
if(COAL_VERSION STREQUAL "")
  set(COAL_VERSION "0.0.0")
endif()

project(
  ${PROJECT_NAME}
  VERSION ${COAL_VERSION}
  DESCRIPTION "${PROJECT_DESCRIPTION}"
  LANGUAGES CXX
)

set(PROJECT_URL "https://github.com/coal-library/coal")
set(PROJECT_COMPATIBILITY_VERSION AnyNewerVersion)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# ----------------------------------------------------
# --- OPTIONS  ---------------------------------------
# ----------------------------------------------------
option(SUFFIX_SO_VERSION "Suffix library name with its version" OFF)
option(
  COAL_TURN_ASSERT_INTO_EXCEPTION
  "Turn some critical Coal asserts to exception."
  FALSE
)
option(
  COAL_ENABLE_LOGGING
  "Activate logging for warnings or error messages. Turned on by default in Debug."
  FALSE
)
option(
  COAL_USE_FLOAT_PRECISION
  "Use float precision (32-bit) instead of the default double precision (64-bit)"
  FALSE
)
option(
  COAL_BUILD_WITH_TRACY
  "Build with tracy profiler for performance analysis"
  FALSE
)
option(COAL_WITH_ASSIMP "Enable Assimp mesh importer" OFF)

function(set_standard_output_directory target)
  set_target_properties(
    ${target}
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
      LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
      ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
  )
endfunction()

if(MSVC)
  add_compile_definitions(BOOST_ALL_NO_LIB)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(
    CMAKE_BUILD_TYPE
    RelWithDebInfo
    CACHE STRING
    "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)."
    FORCE
  )
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel
  )
endif()

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_INSTALL_RPATH "@loader_path")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

set(GENERATED_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include/coal")
file(MAKE_DIRECTORY "${GENERATED_INCLUDE_DIR}")
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/coal-config.hh.in"
  "${GENERATED_INCLUDE_DIR}/config.hh"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/coal-deprecated.hh.in"
  "${GENERATED_INCLUDE_DIR}/deprecated.hh"
  @ONLY
)
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/coal-warning.hh.in"
  "${GENERATED_INCLUDE_DIR}/warning.hh"
  @ONLY
)

set(COAL_PKG_CONFIG_REQUIRES "")

find_package(Eigen3 REQUIRED)
list(APPEND COAL_PKG_CONFIG_REQUIRES "eigen3 >= 3.0.0")

# -- tracy profiling (optional)
if(COAL_BUILD_WITH_TRACY)
  # assume it is installed somewhere
  find_package(Tracy REQUIRED)
  set_target_properties(
    Tracy::TracyClient
    PROPERTIES POSITION_INDEPENDENT_CODE True
  )
endif(COAL_BUILD_WITH_TRACY)

# Required dependencies
set(Boost_USE_MULTITHREADED ON)
if(NOT DEFINED Boost_USE_STATIC_LIBS)
  set(Boost_USE_STATIC_LIBS OFF)
endif()
if(NOT DEFINED Boost_NO_BOOST_CMAKE)
  set(Boost_NO_BOOST_CMAKE ON)
endif()

find_package(Boost REQUIRED COMPONENTS serialization filesystem)
if(COAL_ENABLE_LOGGING)
  find_package(Boost REQUIRED COMPONENTS log)
endif()

if(Boost_VERSION_STRING VERSION_LESS 1.81)
  # Default C++ version should be C++11
  set(CMAKE_CXX_STANDARD 11)
else()
  # Boost.Math will be C++14 starting in July 2023 (Boost 1.82 release)
  set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional dependencies
find_package(octomap QUIET)
if(octomap_FOUND)
  set(COAL_HAS_OCTOMAP TRUE)
  list(APPEND COAL_PKG_CONFIG_REQUIRES "octomap >= 1.8")
  string(REPLACE "." ";" VERSION_LIST ${octomap_VERSION})
  list(GET VERSION_LIST 0 OCTOMAP_MAJOR_VERSION)
  list(GET VERSION_LIST 1 OCTOMAP_MINOR_VERSION)
  list(GET VERSION_LIST 2 OCTOMAP_PATCH_VERSION)
  message(STATUS "COAL uses Octomap")
else()
  set(COAL_HAS_OCTOMAP FALSE)
  message(STATUS "COAL does not use Octomap")
endif()

option(COAL_HAS_QHULL "use qhull library to compute convex hulls." FALSE)
if(COAL_HAS_QHULL)
  find_package(Qhull REQUIRED COMPONENTS qhull_r qhullcpp)
endif()

set(COAL_HAS_ASSIMP ${COAL_WITH_ASSIMP})
if(COAL_WITH_ASSIMP)
  find_package(assimp REQUIRED)
endif()

set(
  ${PROJECT_NAME}_HEADERS
  include/coal/collision_data.h
  include/coal/BV/kIOS.h
  include/coal/BV/BV.h
  include/coal/BV/RSS.h
  include/coal/BV/OBBRSS.h
  include/coal/BV/BV_node.h
  include/coal/BV/AABB.h
  include/coal/BV/OBB.h
  include/coal/BV/kDOP.h
  include/coal/broadphase/broadphase.h
  include/coal/broadphase/broadphase_SSaP.h
  include/coal/broadphase/broadphase_SaP.h
  include/coal/broadphase/broadphase_bruteforce.h
  include/coal/broadphase/broadphase_collision_manager.h
  include/coal/broadphase/broadphase_continuous_collision_manager-inl.h
  include/coal/broadphase/broadphase_continuous_collision_manager.h
  include/coal/broadphase/broadphase_dynamic_AABB_tree-inl.h
  include/coal/broadphase/broadphase_dynamic_AABB_tree.h
  include/coal/broadphase/broadphase_dynamic_AABB_tree_array-inl.h
  include/coal/broadphase/broadphase_dynamic_AABB_tree_array.h
  include/coal/broadphase/broadphase_interval_tree.h
  include/coal/broadphase/broadphase_spatialhash-inl.h
  include/coal/broadphase/broadphase_spatialhash.h
  include/coal/broadphase/broadphase_callbacks.h
  include/coal/broadphase/default_broadphase_callbacks.h
  include/coal/broadphase/detail/hierarchy_tree-inl.h
  include/coal/broadphase/detail/hierarchy_tree.h
  include/coal/broadphase/detail/hierarchy_tree_array-inl.h
  include/coal/broadphase/detail/hierarchy_tree_array.h
  include/coal/broadphase/detail/interval_tree.h
  include/coal/broadphase/detail/interval_tree_node.h
  include/coal/broadphase/detail/morton-inl.h
  include/coal/broadphase/detail/morton.h
  include/coal/broadphase/detail/node_base-inl.h
  include/coal/broadphase/detail/node_base.h
  include/coal/broadphase/detail/node_base_array-inl.h
  include/coal/broadphase/detail/node_base_array.h
  include/coal/broadphase/detail/simple_hash_table-inl.h
  include/coal/broadphase/detail/simple_hash_table.h
  include/coal/broadphase/detail/simple_interval-inl.h
  include/coal/broadphase/detail/simple_interval.h
  include/coal/broadphase/detail/sparse_hash_table-inl.h
  include/coal/broadphase/detail/sparse_hash_table.h
  include/coal/broadphase/detail/spatial_hash-inl.h
  include/coal/broadphase/detail/spatial_hash.h
  include/coal/narrowphase/narrowphase.h
  include/coal/narrowphase/gjk.h
  include/coal/narrowphase/narrowphase_defaults.h
  include/coal/narrowphase/minkowski_difference.h
  include/coal/narrowphase/support_data.h
  include/coal/narrowphase/support_functions.h
  include/coal/shape/convex.h
  include/coal/shape/convex.hxx
  include/coal/shape/geometric_shape_to_BVH_model.h
  include/coal/shape/geometric_shapes.h
  include/coal/shape/geometric_shapes.hxx
  include/coal/shape/geometric_shapes_traits.h
  include/coal/shape/geometric_shapes_utility.h
  include/coal/distance_func_matrix.h
  include/coal/collision.h
  include/coal/collision_func_matrix.h
  include/coal/contact_patch.h
  include/coal/contact_patch_func_matrix.h
  include/coal/contact_patch/contact_patch_solver.h
  include/coal/contact_patch/contact_patch_solver.hxx
  include/coal/contact_patch/contact_patch_simplifier.h
  include/coal/distance.h
  include/coal/math/matrix_3f.h
  include/coal/math/vec_3f.h
  include/coal/math/types.h
  include/coal/math/transform.h
  include/coal/data_types.h
  include/coal/BVH/BVH_internal.h
  include/coal/BVH/BVH_model.h
  include/coal/BVH/BVH_front.h
  include/coal/BVH/BVH_utility.h
  include/coal/collision_object.h
  include/coal/collision_utility.h
  include/coal/hfield.h
  include/coal/fwd.hh
  include/coal/logging.h
  include/coal/tracy.hh
  include/coal/internal/BV_fitter.h
  include/coal/internal/BV_splitter.h
  include/coal/internal/shape_shape_func.h
  include/coal/internal/shape_shape_contact_patch_func.h
  include/coal/internal/intersect.h
  include/coal/internal/intersect.hxx
  include/coal/internal/tools.h
  include/coal/internal/traversal_node_base.h
  include/coal/internal/traversal_node_bvh_shape.h
  include/coal/internal/traversal_node_bvhs.h
  include/coal/internal/traversal_node_hfield_shape.h
  include/coal/internal/traversal_node_setup.h
  include/coal/internal/traversal_node_shapes.h
  include/coal/internal/traversal_recurse.h
  include/coal/internal/traversal.h
  include/coal/serialization/fwd.h
  include/coal/serialization/serializer.h
  include/coal/serialization/archive.h
  include/coal/serialization/transform.h
  include/coal/serialization/AABB.h
  include/coal/serialization/BV_node.h
  include/coal/serialization/BV_splitter.h
  include/coal/serialization/BVH_model.h
  include/coal/serialization/collision_data.h
  include/coal/serialization/contact_patch.h
  include/coal/serialization/collision_object.h
  include/coal/serialization/convex.h
  include/coal/serialization/eigen.h
  include/coal/serialization/geometric_shapes.h
  include/coal/serialization/memory.h
  include/coal/serialization/OBB.h
  include/coal/serialization/RSS.h
  include/coal/serialization/OBBRSS.h
  include/coal/serialization/kIOS.h
  include/coal/serialization/kDOP.h
  include/coal/serialization/hfield.h
  include/coal/serialization/quadrilateral.h
  include/coal/serialization/triangle.h
  include/coal/timings.h
)

if(COAL_WITH_ASSIMP)
  list(
    APPEND ${PROJECT_NAME}_HEADERS
    include/coal/mesh_loader/assimp.h
    include/coal/mesh_loader/loader.h
  )
endif()

if(COAL_HAS_OCTOMAP)
  list(
    APPEND ${PROJECT_NAME}_HEADERS
    include/coal/octree.h
    include/coal/serialization/octree.h
    include/coal/internal/traversal_node_octree.h
  )
endif(COAL_HAS_OCTOMAP)

add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(test)
endif(BUILD_TESTING)

# Install headers
if(COAL_WITH_ASSIMP)
  install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/coal
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "*.hh"
      PATTERN "*.hpp"
      PATTERN "*.hxx"
  )
else()
  install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/coal
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "*.hh"
      PATTERN "*.hpp"
      PATTERN "*.hxx"
      PATTERN "mesh_loader/*" EXCLUDE
  )
endif()
install(
  FILES
    ${GENERATED_INCLUDE_DIR}/config.hh
    ${GENERATED_INCLUDE_DIR}/deprecated.hh
    ${GENERATED_INCLUDE_DIR}/warning.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/coal
)

# Export targets + Config.cmake
install(
  EXPORT ${TARGETS_EXPORT_NAME}
  NAMESPACE coal::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
export(
  EXPORT ${TARGETS_EXPORT_NAME}
  NAMESPACE coal::
  FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
)
configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/coalConfig.cmake.in
  ${PROJECT_BINARY_DIR}/coalConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
write_basic_package_version_file(
  ${PROJECT_BINARY_DIR}/coalConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY ${PROJECT_COMPATIBILITY_VERSION}
)
install(
  FILES
    ${PROJECT_BINARY_DIR}/coalConfig.cmake
    ${PROJECT_BINARY_DIR}/coalConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# pkg-config
if(COAL_HAS_OCTOMAP)
  # FCL_HAVE_OCTOMAP kept for backward compatibility reasons.
  set(
    COAL_PC_CFLAGS_EXTRA
    "-DCOAL_HAS_OCTOMAP -DCOAL_HAVE_OCTOMAP -DFCL_HAVE_OCTOMAP -DOCTOMAP_MAJOR_VERSION=${OCTOMAP_MAJOR_VERSION} -DOCTOMAP_MINOR_VERSION=${OCTOMAP_MINOR_VERSION} -DOCTOMAP_PATCH_VERSION=${OCTOMAP_PATCH_VERSION}"
  )
else()
  set(COAL_PC_CFLAGS_EXTRA "")
endif()
if(COAL_PKG_CONFIG_REQUIRES)
  string(JOIN ", " COAL_PC_REQUIRES ${COAL_PKG_CONFIG_REQUIRES})
  set(COAL_PC_REQUIRES_LINE "Requires: ${COAL_PC_REQUIRES}")
else()
  set(COAL_PC_REQUIRES_LINE "")
endif()
set(COAL_PC_LIBS "-L${CMAKE_INSTALL_FULL_LIBDIR} -lcoal")
set(COAL_PC_CFLAGS "-I${CMAKE_INSTALL_FULL_INCLUDEDIR} ${COAL_PC_CFLAGS_EXTRA}")
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/coal.pc.in
  ${PROJECT_BINARY_DIR}/coal.pc
  @ONLY
)
install(
  FILES ${PROJECT_BINARY_DIR}/coal.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
