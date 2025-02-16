#
#//===----------------------------------------------------------------------===//
#//
#// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
#// See https://llvm.org/LICENSE.txt for license information.
#// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#//
#//===----------------------------------------------------------------------===//
#

set (CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/libompd/"
    ${CMAKE_MODULE_PATH}
)

find_package (Python3 COMPONENTS Interpreter Development)
include_directories (${OMPD_INCLUDE_PATH})
include_directories (${LIBOMP_INCLUDE_DIR})
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/python-module/ompd/__init__.py
                   DEPENDS ompdModule.c ompdAPITests.c setup.py ompd/frame_filter.py ompd/__init__.py ompd/ompd_address_space.py ompd/ompd_callbacks.py ompd/ompd_handles.py ompd/ompd.py
                   COMMAND ${CMAKE_COMMAND} -E env LIBOMP_INCLUDE_DIR=${LIBOMP_INCLUDE_DIR}
                   ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py -v bdist_wheel -b ${CMAKE_CURRENT_BINARY_DIR}/build -d ${CMAKE_CURRENT_BINARY_DIR}
                   COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py clean --all
                   COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/ompd.egg-info
                   COMMAND ${Python3_EXECUTABLE} -m pip install ${LIBOMPD_PIP_FLAGS} -U -t ${CMAKE_CURRENT_BINARY_DIR}/python-module --no-index
                   --find-links=${CMAKE_CURRENT_BINARY_DIR} ompd
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(ompd_gdb_plugin ALL
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/python-module/ompd/__init__.py
                  COMMENT "Building the OMPD GDB plugin")

install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install ${LIBOMPD_PIP_FLAGS} -U -t ${CMAKE_INSTALL_PREFIX}/share/gdb/python/gdb --no-index --find-links=${CMAKE_CURRENT_BINARY_DIR} ompd)")
