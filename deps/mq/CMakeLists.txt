include_directories(/usr/local/ActiveMQ-CPP)
include_directories(/usr/local/ActiveMQ-CPP/include/activemq-cpp-3.9.4)
include_directories(/usr/local/apr/include/apr-1)
link_directories(/usr/local/ActiveMQ-CPP/lib)
link_libraries(activemq-cpp pthread boost_serialization)

add_definitions(-std=c++11)
aux_source_directory(./src DIR_LIB_SRCS)
add_library(mq ${DIR_LIB_SRCS})
