cmake_minimum_required(VERSION 3.0)
enable_testing()
set(Project_BINARY_DIR ./build)

# using Ascension
include_directories(../)
set(Ascension_SOURCE_DIR ../src)

# using Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_definitions(-DASCENSION_TEST)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_definitions(-std=c++11 -stdlib=libc++)
elseif(MSVC)
	add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS /DNOMINMAX /EHac /GS /MTd /nologo /RTCcus /W4 /wd4127 /wd4510 /wd4512 /wd4706 /Zc:forScope /Zc:wchar_t)
endif()

# corelib
add_executable(
	numeric-range-test
	src/numeric-range-test.cpp)
add_test(
	NAME numeric_range
	COMMAND $<TARGET_FILE:numeric-range-test>
	CONFIGURATIONS Debug)
add_executable(
	regex-test
	src/regex-test.cpp
	${Ascension_SOURCE_DIR}/corelib/encoder.cpp
	${Ascension_SOURCE_DIR}/corelib/regex.cpp
	${Ascension_SOURCE_DIR}/corelib/text/character-property.cpp
	${Ascension_SOURCE_DIR}/corelib/text/grapheme-break-iterator.cpp
	${Ascension_SOURCE_DIR}/corelib/text/identifier-syntax.cpp)
add_test(
	NAME regex
	COMMAND $<TARGET_FILE:regex-test>
	CONFIGURATIONS Debug)

# corelib.detail
add_executable(
	gap-vector-test
	src/gap-vector-test.cpp)
add_test(
	NAME gap_vector
	COMMAND $<TARGET_FILE:gap-vector-test>
	CONFIGURATIONS Debug)

# corelib.text
add_executable(
	newline-test
	src/newline-test.cpp
	${Ascension_SOURCE_DIR}/corelib/text/newline.cpp)
add_test(
	NAME newline
	COMMAND $<TARGET_FILE:newline-test>
	CONFIGURATIONS Debug)
add_executable(
	utf-iterator-test
	src/utf-iterator-test.cpp)
add_test(
	NAME utf_iterator
	COMMAND $<TARGET_FILE:utf-iterator-test>
	CONFIGURATIONS Debug)

#graphics.geometry
add_executable(
	geometry-test
	src/geometry-test.cpp)
add_test(
	NAME geometry
	COMMAND $<TARGET_FILE:geometry-test>
	CONFIGURATIONS Debug)
