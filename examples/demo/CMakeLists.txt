add_executable(demo-icypuff demo.cpp)

target_link_libraries(demo-icypuff
    PRIVATE
        icypuff::icypuff
) 