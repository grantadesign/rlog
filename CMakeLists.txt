cmake_minimum_required (VERSION 2.6)

message("=== rlog ===")

set(RLOG_SRCS_COMPILED _SCU_RLog.cpp)
set(RLOG_SRCS
	common.h
	Error.h
	Lock.h
	Mutex.h
	rlog.h
	rlog-c99.h
	RLogChannel.h
	rloginit.h
	rloglocation.h
	rlog-novariadic.h
	rlog-prec99.h
	RLogPublisher.h
	RLogNode.h
	Error.cpp
	rlog.cpp
	RLogChannel.cpp
	rloginit.cpp
	rloglocation.cpp
	RLogNode.cpp
	RLogPublisher.cpp
	StreamNodes.cpp
    StreamNodes.h
)
set_source_files_properties(${RLOG_SRCS} PROPERTIES HEADER_FILE_ONLY true)

#SyslogNode.h
#RLogTime.h
#RLogTime.cpp	
#SyslogNode.cpp
#test.cpp
#testlog.cpp
#StdioNode.cpp
#StdioNode.h
    
add_definitions(
	-DRLOG_COMPONENT="rlog"
	-D_CRT_SECURE_NO_WARNINGS
	-DRLOGAPI_EXPORTS
    "/wd4996" #disable MSVC "unsafe" complaints due to using raw pointers with STL algos.
)

add_library(rlog SHARED ${RLOG_SRCS_COMPILED}
						${RLOG_SRCS}
)

set_property(TARGET rlog PROPERTY FOLDER "rLog")

install(TARGETS rlog
        RUNTIME DESTINATION bin
        COMPONENT CORE)

install(TARGETS rlog
        ARCHIVE DESTINATION ../../libs
        COMPONENT CORE)
