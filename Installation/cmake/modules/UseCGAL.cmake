#
# UseCGAL.cmake can be included in a project to set the needed compiler and linker
# settings to use CGAL in a program.
#
# The variables used here are defined in the CGALConfig.cmake generated when CGAL was installed.
#
#
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

include(${CGAL_MODULES_DIR}/CGAL_Macros.cmake)

cgal_setup_module_path()

if(NOT USE_CGAL_FILE_INCLUDED)
  set(USE_CGAL_FILE_INCLUDED 1)

  include(CGAL_Common)
  include(CGAL_SetupFlags)
  include(CGAL_GeneratorSpecificSettings)
  include(CGAL_TweakFindBoost)

  set(CGAL_INSTALLED_SCM_BRANCH_NAME ${CGAL_SCM_BRANCH_NAME})
  set(CGAL_SCM_BRANCH_NAME "")

  if( NOT "${CGAL_INSTALLED_SCM_BRANCH_NAME}" STREQUAL "" )
    include(CGAL_SCM)
    if ( NOT "${CGAL_SCM_BRANCH_NAME}" STREQUAL "" )
      message ( STATUS "Code taken from Git branch: ${CGAL_SCM_BRANCH_NAME}" )
      if ( NOT "${CGAL_SCM_BRANCH_NAME}" STREQUAL "${CGAL_INSTALLED_SCM_BRANCH_NAME}")
        message (AUTHOR_WARNING "Branch '${CGAL_SCM_BRANCH_NAME}' does not match branch '${CGAL_INSTALLED_SCM_BRANCH_NAME}' from which CGAL has been installed. Please consider rebuilding CGAL from this branch.")
      endif()
    endif()
  endif()

  set( CGAL_LIBRARIES )

  foreach ( component ${CGAL_REQUESTED_COMPONENTS} )
  
  	if(${component} STREQUAL "Qt4or5")
	    
		message(STATUS "The more recent Qt version configured for CGAL is libCGAL_Qt${CGAL_QT_RECENT}.")
		
	   	set (CGAL_QT_VERSION "${CGAL_QT_RECENT}" CACHE STRING "Choice of libCGAL Qt version")
		
	    if(NOT (${CGAL_QT_VERSION} STREQUAL ""))
	      list(APPEND QT_VERSIONS_ALLOW 4 5)
		else ()
		  list(APPEND QT_VERSIONS_ALLOW "" )
	    endif()

		SET_PROPERTY(CACHE CGAL_QT_VERSION PROPERTY STRINGS ${QT_VERSIONS_ALLOW})
		set(CGAL_Qt_version Qt${CGAL_QT_VERSION})
		use_component( ${CGAL_Qt_version} )
	else()
	    use_component( ${component} )
	endif()
  endforeach()

  use_essential_libs()

  include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )

  # need to get variable from cache while compiling CGAL, while in a demo it is set in CGALConfig.cmake
  if ( NOT CGAL_LIBRARY )
    cache_get(CGAL_LIBRARY)
  endif()
  if(TARGET CGAL)
    add_to_list( CGAL_LIBRARIES CGAL )
  else()
    add_to_list( CGAL_LIBRARIES ${CGAL_LIBRARY} )
  endif()

  #message (STATUS "LIB: ${CGAL_LIBRARY}")
  #message (STATUS "LIBS: ${CGAL_LIBRARIES}")

  include_directories ( ${CGAL_INCLUDE_DIRS})
  include_directories ( SYSTEM ${CGAL_3RD_PARTY_INCLUDE_DIRS} )
  add_definitions     ( ${CGAL_3RD_PARTY_DEFINITIONS}  ${CGAL_DEFINITIONS}  )

  link_directories    ( ${CGAL_LIBRARIES_DIR} ${CGAL_3RD_PARTY_LIBRARIES_DIRS} )
  link_libraries      ( ${CGAL_LIBRARIES}     ${CGAL_3RD_PARTY_LIBRARIES}      )


endif()
