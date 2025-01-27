add_executable(demo-icypuff demo.cpp)

find_package(cxxopts CONFIG REQUIRED)

target_link_libraries(demo-icypuff
    PRIVATE
        icypuff::icypuff
        cxxopts::cxxopts
) 