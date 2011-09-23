cmake_minimum_required(VERSION 2.6)

project(test)
include_directories(../)
set(SRCDIR ../src)

add_definitions(-DASCENSION_TEST)
if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_SCL_SECURE_NO_WARNINGS /EHac /GS /MTd /nologo /RTCcus /W4 /Wp64 /Zc:forScope /Zc:wchar_t)
endif()

add_executable(range-test range-test.cpp)
add_executable(geometry-test geometry-test.cpp)

add_executable(string-test string-test.cpp)
add_executable(character-iterator-test character-iterator-test.cpp ${SRCDIR}/corelib/text/character-iterator.cpp)

add_executable(utf-iterator-test utf-iterator-test.cpp)

add_executable(break-iterator-test break-iterator-test.cpp ${SRCDIR}/corelib/text/break-iterator.cpp ${SRCDIR}/corelib/text/identifier-syntax.cpp ${SRCDIR}/corelib/text/character-property.cpp)
add_executable(document-test document-test.cpp ${SRCDIR}/kernel/document.cpp ${SRCDIR}/kernel/point.cpp ${SRCDIR}/kernel/stream.cpp ${SRCDIR}/kernel/undo.cpp ${SRCDIR}/corelib/encoder.cpp ${SRCDIR}/corelib/text/identifier-syntax.cpp ${SRCDIR}/corelib/text/character-property.cpp ${SRCDIR}/corelib/text/break-iterator.cpp)
add_executable(case-folder-test case-folder-test.cpp ${SRCDIR}/corelib/text/break-iterator.cpp ${SRCDIR}/corelib/text/case-folder.cpp ${SRCDIR}/corelib/text/identifier-syntax.cpp ${SRCDIR}/corelib/text/normalizer.cpp ${SRCDIR}/corelib/text/character-property.cpp ${SRCDIR}/corelib/text/character-iterator.cpp)
add_executable(normalizer-test normalizer-test.cpp ${SRCDIR}/corelib/text/break-iterator.cpp ${SRCDIR}/corelib/text/identifier-syntax.cpp ${SRCDIR}/corelib/text/normalizer.cpp ${SRCDIR}/corelib/text/character-property.cpp)
add_executable(regex-test regex-test.cpp ${SRCDIR}/corelib/regex.cpp ${SRCDIR}/corelib/text/break-iterator.cpp ${SRCDIR}/corelib/encoder.cpp ${SRCDIR}/corelib/text/identifier-syntax.cpp ${SRCDIR}/corelib/text/character-property.cpp)