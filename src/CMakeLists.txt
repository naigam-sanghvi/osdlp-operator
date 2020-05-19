include_directories(
    ${PROJECT_SOURCE_DIR}/osdlp/include
)

set(SOURCE_FILES
    main.cpp
)

add_executable(osdlp-operator ${SOURCE_FILES})

#target_link_libraries(osdlp-operator osdlp)