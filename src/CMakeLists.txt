set(SOURCE_FILES
    main.cpp
    config.cpp
    virtualchannel.cpp
    queue_api.cpp
   )

add_executable(osdlp-operator ${SOURCE_FILES})

target_link_libraries(osdlp-operator osdlp config++ stdc++fs)