#dsp
#DSP subsystem
project(dsp)
set(NAME dsp)
set(PREFIX)

add_definitions(
  -fPIC
  -DCPU=${CPU}
  -DDPU=${DPU} -DDPU_H=<dpu/${DPU}.h>
  -DFPU=${FPU} -DFPU_H=<fpu/${FPU}.h>
  -DGPU=${GPU} -DGPU_H=<gpu/${GPU}.h>
  -DPIC
  ${HOST_DEFS}
)

include_directories(
  ${HOST_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(DSP_SRC_DIR ${GLOBL_SRC_DIR}/dsp)

set(inc
  dc.h
)

set(srcs
  runtime.cpp
  argument.cpp constant.cpp uniform.cpp
  dc.cpp
  mmu.cpp ppu.cpp apu.cpp
  core.cpp factory.cpp atom.cpp
  dsp.cpp
)

set(libs host ${HOST_LIBS})

add_library(${NAME} STATIC ${srcs})
set_target_properties(${NAME} PROPERTIES PREFIX "${PREFIX}")
target_link_libraries(${NAME} ${libs})

if(SDK)
  file(MAKE_DIRECTORY ${DSP_SRC_DIR})
  install(
    FILES
      ${inc}
    DESTINATION
      ${DSP_SRC_DIR}
  )
endif(SDK)
