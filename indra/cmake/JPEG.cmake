# -*- cmake -*-
include(Prebuilt)

include(Linking)
set(JPEG_FIND_QUIETLY ON)
set(JPEG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindJPEG)
else (STANDALONE)
  use_prebuilt_binary(jpeglib)
  if (LINUX)
    set(JPEG_LIBRARIES jpeg)
  elseif (DARWIN)
    set(JPEG_LIBRARIES
      debug libjpeg.a
      optimized libjpeg.a
      )
  elseif (WINDOWS)
    set(JPEG_LIBRARIES jpeglib)
  endif (LINUX)
  set(JPEG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
