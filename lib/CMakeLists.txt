include_directories(
    ${PROJECT_SOURCE_DIR}/lib/osdlp/include
)

list(APPEND osdlp_sources
	osdlp/src/osdlp_clcw.c
        osdlp/src/osdlp_cop.c
        osdlp/src/osdlp_crc.c
        osdlp/src/osdlp_tc.c
        osdlp/src/osdlp_tm.c
)
set(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
add_library(osdlp SHARED ${osdlp_sources}) 

target_link_libraries(osdlp m)
