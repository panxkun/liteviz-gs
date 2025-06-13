if(NOT TARGET depends::tinyply)
  if(NOT TARGET options::modern-cpp)
    message(FATAL_ERROR "depends::tinyply expects options::modern-cpp")
  endif()
  FetchContent_Declare(
    depends-tinyply
    GIT_REPOSITORY https://github.com/ddiakopoulos/tinyply.git
    GIT_TAG        2.2
  )
  FetchContent_GetProperties(depends-tinyply)
  if(NOT depends-tinyply_POPULATED)
    message(STATUS "Fetching tinyply sources")
    FetchContent_Populate(depends-tinyply)
    message(STATUS "Fetching tinyply sources - done")
  endif()

  add_library(depends_tinyply STATIC
    ${depends-tinyply_SOURCE_DIR}/source/tinyply.cpp
  )
  
  target_include_directories(depends_tinyply PUBLIC
    ${depends-tinyply_SOURCE_DIR}/source
  )
  
  target_link_libraries(depends_tinyply PUBLIC options::modern-cpp)
  add_library(depends::tinyply ALIAS depends_tinyply) 
  set(depends-tinyply-source-dir ${depends-tinyply_SOURCE_DIR} CACHE INTERNAL "" FORCE)
  mark_as_advanced(depends-tinyply-source-dir)
endif()
