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
  ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/sqlite3ndk.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/tangram/src/main/cpp/tangram_platform_adapter.cpp
  
  ${NAO_MAP_DIR}src/main/jni/NativePlatformTangramImpl.cpp
  ${NAO_MAP_DIR}src/main/jni/NativeIUrlCallback.cpp
  )

target_include_directories(${LIB_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/core/deps/SQLiteCpp/sqlite3) # sqlite3ndk.cpp needs sqlite3.h


# need to include PlatformTrangramImpl.hpp  
target_include_directories(${LIB_NAME} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/../NAOMapLib/code/)


target_link_libraries(${LIB_NAME}
  PUBLIC
  ${CORE_LIBRARY}
  # android libraries
  GLESv2 djinni_support_lib log z atomic android)