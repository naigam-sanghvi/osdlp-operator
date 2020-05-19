include_directories(
    ${PROJECT_SOURCE_DIR}/lib/osdlp/include
)

list(APPEND osdlp_sources
        osdlp/src/clcw.c
        osdlp/src/cop.c
        osdlp/src/crc.c
        osdlp/src/tc.c
        osdlp/src/tm.c
)

add_library(osdlp SHARED ${osdlp_sources}) 

#target_link_libraries(osdlp m)
