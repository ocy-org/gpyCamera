# Install script for directory: D:/software/otherdll/zxing-cpp-master/core

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/software/otherdll/zxing-cpp-master/core/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "D:/software/Qt5.12.10/Tools/mingw730_64/bin/objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/software/otherdll/zxing-cpp-master/core/libZXing.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ZXing" TYPE FILE FILES
    "D:/software/otherdll/zxing-cpp-master/core/src/Barcode.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/BarcodeFormat.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/BitHacks.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ByteArray.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/CharacterSet.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/Content.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/Error.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/Flags.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/GTIN.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ImageView.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/Point.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/Quadrilateral.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ReadBarcode.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ReaderOptions.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/StructuredAppend.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/TextUtfEncoding.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ZXingCpp.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ZXAlgorithms.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/ZXVersion.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/DecodeHints.h"
    "D:/software/otherdll/zxing-cpp-master/core/src/Result.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ZXing" TYPE FILE FILES "D:/software/otherdll/zxing-cpp-master/core/Version.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing/ZXingTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing/ZXingTargets.cmake"
         "D:/software/otherdll/zxing-cpp-master/core/CMakeFiles/Export/f9e04a807b27a41299a115186893fdf1/ZXingTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing/ZXingTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing/ZXingTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing" TYPE FILE FILES "D:/software/otherdll/zxing-cpp-master/core/CMakeFiles/Export/f9e04a807b27a41299a115186893fdf1/ZXingTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing" TYPE FILE FILES "D:/software/otherdll/zxing-cpp-master/core/CMakeFiles/Export/f9e04a807b27a41299a115186893fdf1/ZXingTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/software/otherdll/zxing-cpp-master/core/zxing.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ZXing" TYPE FILE FILES
    "D:/software/otherdll/zxing-cpp-master/core/ZXingConfig.cmake"
    "D:/software/otherdll/zxing-cpp-master/core/ZXingConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/software/otherdll/zxing-cpp-master/core/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
