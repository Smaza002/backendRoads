add_executable(backend
    main.cpp
)

target_link_libraries(backend
    PRIVATE
        httplib::httplib
)
