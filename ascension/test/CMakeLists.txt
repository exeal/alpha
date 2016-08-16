cmake_minimum_required(VERSION 3.0)
enable_testing()
set(Project_BINARY_DIR ./build)

# using Ascension
include_directories(../)
set(Ascension_SOURCE_DIR ../src)

# using Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARIES})

add_definitions(-DASCENSION_TEST)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_definitions(-std=c++11 -stdlib=libc++)
elseif(MSVC)
	add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS /DNOMINMAX /EHac /GS /MTd /nologo /RTCcus /W4 /wd4127 /wd4510 /wd4512 /wd4706 /Zc:forScope /Zc:wchar_t)
endif()

set(boost_uses_auto_link FALSE)
if(MSVC)
	set(boost_uses_auto_link TRUE)
endif()

# (toplevel)
add_executable(
	direction-test
	src/direction-test.cpp)
add_test(
	NAME direction
	COMMAND $<TARGET_FILE:direction-test>
	CONFIGURATIONS Debug)

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
if(NOT boost_uses_auto_link)
	target_link_libraries(
		regex-test
		libboost_filesystem-mt.a
		libboost_regex-mt.a
		libboost_system-mt.a)
endif()
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
	character-test
	src/character-test.cpp)
add_test(
	NAME character
	COMMAND $<TARGET_FILE:character-test>
	CONFIGURATIONS Debug)
add_executable(
	newline-test
	src/newline-test.cpp
	${Ascension_SOURCE_DIR}/corelib/text/newline.cpp)
add_test(
	NAME newline
	COMMAND $<TARGET_FILE:newline-test>
	CONFIGURATIONS Debug)
add_executable(
	utf-test
	src/utf-test.cpp)
add_test(
	NAME utf
	COMMAND $<TARGET_FILE:utf-test>
	CONFIGURATIONS Debug)
add_executable(
	utf-iterator-test
	src/utf-iterator-test.cpp)
add_test(
	NAME utf_iterator
	COMMAND $<TARGET_FILE:utf-iterator-test>
	CONFIGURATIONS Debug)

# graphics
add_executable(
	physical-directional-test
	src/physical-directional-test.cpp)
add_test(
	NAME physical_directions
	COMMAND $<TARGET_FILE:physical-directional-test>
	CONFIGURATIONS Debug)

# graphics.font
add_executable(
	line-relative-directional-test
	src/line-relative-directional-test.cpp)
add_test(
	NAME line_relative_directions
	COMMAND $<TARGET_FILE:line-relative-directional-test>
	CONFIGURATIONS Debug)

# graphics.geometry
add_executable(
	geometry-test
	src/geometry-test.cpp)
add_test(
	NAME geometry
	COMMAND $<TARGET_FILE:geometry-test>
	CONFIGURATIONS Debug)

# kernel
add_executable(
	document-test
	src/document-test.cpp
	${Ascension_SOURCE_DIR}/corelib/text/character-property.cpp
	${Ascension_SOURCE_DIR}/corelib/text/identifier-syntax.cpp
	${Ascension_SOURCE_DIR}/corelib/text/newline.cpp
	${Ascension_SOURCE_DIR}/kernel/abstract-point.cpp
	${Ascension_SOURCE_DIR}/kernel/bookmarker.cpp
	${Ascension_SOURCE_DIR}/kernel/document.cpp
	${Ascension_SOURCE_DIR}/kernel/point.cpp
	${Ascension_SOURCE_DIR}/kernel/stream.cpp
	${Ascension_SOURCE_DIR}/kernel/undo.cpp)
add_test(
	NAME document
	COMMAND $<TARGET_FILE:document-test>
	CONFIGURATIONS Debug)

# presentation
add_executable(
	flow-relative-directional-test
	src/flow-relative-directional-test.cpp)
add_test(
	NAME flow_relative_directions
	COMMAND $<TARGET_FILE:flow-relative-directional-test>
	CONFIGURATIONS Debug)

# rules
add_executable(
	token-rules-test
	src/token-rules-test.cpp
	${Ascension_SOURCE_DIR}/corelib/encoder.cpp
	${Ascension_SOURCE_DIR}/corelib/regex.cpp
	${Ascension_SOURCE_DIR}/corelib/text/character-property.cpp
	${Ascension_SOURCE_DIR}/corelib/text/identifier-syntax.cpp
	${Ascension_SOURCE_DIR}/rules/number-token-rule.cpp
	${Ascension_SOURCE_DIR}/rules/regex-token-rule.cpp
	${Ascension_SOURCE_DIR}/rules/region-token-rule.cpp
	${Ascension_SOURCE_DIR}/rules/token-rule.cpp
	${Ascension_SOURCE_DIR}/rules/uri-detector.cpp
	${Ascension_SOURCE_DIR}/rules/uri-token-rule.cpp
	${Ascension_SOURCE_DIR}/rules/word-set-token-rule.cpp)
add_test(
	NAME token_rules
	COMMAND $<TARGET_FILE:token-rules-test>
	CONFIGURATIONS Debug)
