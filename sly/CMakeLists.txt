include_directories(include)

file(GLOB SLY_HEADER include/sly/*.h)
file(GLOB SLY_SRC src/*.cpp)

add_library(sly ${SLY_SRC} ${SLY_HEADER})

