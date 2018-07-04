#find_package(Java REQUIRED)
include(UseJava)

#enable_testing()

# compile AlexandriaJNI.java to class file
#set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6")
#add_jar(AlexandriaJNI AlexandriaJNI.java)
#get_target_property(_jarFile AlexandriaJNI JAR_FILE)
#get_target_property(_classDir AlexandriaJNI CLASSDIR)
#
## generate AlexandriaJNI.h stub
#set (_stubDir "${CMAKE_CURRENT_BINARY_DIR}")
#add_custom_command(
#        OUTPUT AlexandriaJNI.h
#        COMMAND ${Java_JAVAH_EXECUTABLE} -verbose
#        -classpath ${_classDir}
#        -d ${_stubDir}
#        -jni AlexandriaJNI
#        DEPENDS AlexandriaJNI
#)


# generate libalexandriaJNIb
#include_directories(${JNI_INCLUDE_DIRS} ${_classDir} ${_stubDir})
include_directories(${JNI_INCLUDE_DIRS} )
add_library(alexandriaJNI MODULE AlexandriaJNI.cpp AlexandriaJNI.h )
target_link_libraries(alexandriaJNI PRIVATE
        sophiatx_protocol sophiatx_utilities fc
        ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${JNI_LIBRARIES})

# add test to run JNIFoo
#add_test(NAME TestAlexandriaJNI
#        COMMAND ${Java_JAVA_EXECUTABLE}
#        -Djava.library.path=${CMAKE_CURRENT_BINARY_DIR}
#        -cp ${_jarFile} AlexandriaJNI)


if( CLANG_TIDY_EXE )
    set_target_properties(
            alexandriaJNI PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
    )
endif( CLANG_TIDY_EXE )


install( TARGETS
        alexandriaJNI
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION lib
        )
