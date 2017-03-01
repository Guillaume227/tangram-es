add_definitions(-DPLATFORM_ANDROID)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

set(ANDROID_PROJECT_DIR ${PROJECT_SOURCE_DIR}/platforms/android/tangram)

set(LIB_NAME tangram) # in order to have libtangram.so

set(NAO_ANDROID_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../AndroidApps/AndroidStudioProjects/NAO/)

set(NAO_MAP_DIR ${NAO_ANDROID_DIR}naomap/)

add_library(${LIB_NAME} SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/platforms/common/platform_gl.cpp
  
  #${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/jniExports.cpp
  #${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/platform_android.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/tangram_platform_adapter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/djinni_main.cpp
  
  ${NAO_MAP_DIR}src/main/jni/NativePlatformTangramImpl.cpp
  ${NAO_MAP_DIR}src/main/jni/NativeIUrlCallback.cpp
  )

 
target_include_directories(${LIB_NAME} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/../NAOMapLib/code/)
  
 
set_target_properties(djinni PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/../utils/djinni/support-lib/jni)

target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libaries
  GLESv2 djinni log z atomic android)