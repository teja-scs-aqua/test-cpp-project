SET(K3D_OSMESA_FOUND 0)

INCLUDE(K3DFindPkgConfig)
PKG_CHECK_MODULES(OSMESA osmesa)

IF(OSMESA_FOUND)
	SET(K3D_OSMESA_INCLUDE_DIRECTORIES
		${OSMESA_INCLUDE_DIRS}
		)

	SET(K3D_OSMESA_LIB_DIRECTORIES
		${OSMESA_LIBRARY_DIRS}
		)

	SET(K3D_OSMESA_LIBRARIES
		${OSMESA_LIBRARIES}
		)

	SET(K3D_OSMESA_FOUND 1)
ENDIF()

