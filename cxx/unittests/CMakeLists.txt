add_custom_target(S2020UnitTests)
set_target_properties(S2020UnitTests PROPERTIES FOLDER "Tests")

function(add_s2020_unittest test_dirname)
  add_unittest(S2020UnitTests ${test_dirname} ${ARGN})
  target_compile_definitions(${test_dirname} PUBLIC UNIT_TEST)
endfunction()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR
    "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # without this, because we build -Wextra, gtest warns when comparing
  # against integer constants
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  # use of std::tr1 namespace (in GTest headers)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
endif()

add_subdirectory(Support)
add_subdirectory(Parser)
