#
# The module set complie directories.
#

IF (UNIX)
    LINK_DIRECTORIES(${ROOT_DIR}/lib)
ELSEIF(WIN32)
	IF(BUILD_DEBUG)
	    LINK_DIRECTORIES(${ROOT_DIR}/lib/Debug)
    ELSE()
	    LINK_DIRECTORIES(${ROOT_DIR}/lib/Release)
    ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(${ROOT_DIR}/include)

SET(LIBRARY_OUTPUT_PATH ${ROOT_DIR}/lib)

SET(EXECUTABLE_OUTPUT_PATH ${ROOT_DIR}/lib)
