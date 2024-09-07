cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(tra_pred_model)

find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_executable(tra_pred_model main.cpp)
target_link_libraries(tra_pred_model torch)
set_property(TARGET tra_pred_model PROPERTY CXX_STANDARD 17)
include_directories("${PROJECT_SOURCE_DIR}/../include")


if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET tra_pred_model
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:tra_pred_model>)
endif (MSVC)
