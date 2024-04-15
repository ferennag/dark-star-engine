add_executable(dark_star_testbed src/main.cpp)
target_link_libraries(dark_star_testbed dark_star_engine)
target_compile_options(dark_star_testbed PRIVATE -g -Wall)