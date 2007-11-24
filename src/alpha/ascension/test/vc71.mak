# vc71.mak
# (c) 2007 exeal

!if "$(MSVCDIR)" == ""
MSVCDIR=$(VS71COMNTOOLS)..\..\VC7
!endif
!if "$(MSVCDIR)" == ""
!error Variable MSVCDIR not set.
!endif

CXX_FLAGS=/c /DWIN32 /D_WINDOWS /D_DEBUG /EHac /GS /GX /MTd /nologo /RTCcus /W4 /Wp64 /Zc:forScope /Zc:wchar_t
XS_FLAGS=/DEBUG /nologo ole32.lib odbc32.lib odbccp32.lib shlwapi.lib
ALL_HEADER=

all: bin document-test break-iterator-test case-folder-test normalizer-test regex-test unicode-iterator-test

bin:
	@if not exist "vc71" mkdir vc71

clean:
	del vc71\*.obj

test: all


# common object files #######################################################

./vc71/encoder.obj: ../encoder.cpp ../encodings/win32cp.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/encoder.obj ../encoder.cpp

./vc71/unicode-property.obj: ../unicode-property.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/unicode-property.obj ../unicode-property.cpp

./vc71/break-iterator.obj: ../unicode/break-iterator.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/break-iterator.obj ../unicode/break-iterator.cpp

./vc71/identifier-syntax.obj: ../unicode/identifier-syntax.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/identifier-syntax.obj ../unicode/identifier-syntax.cpp

./vc71/normalizer.obj: ../unicode/normalizer.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/normalizer.obj ../unicode/normalizer.cpp


# document-test #############################################################

./vc71/document.obj: ../document.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/document.obj ../document.cpp

./vc71/document-test.obj: document-test.cpp vc71/document.obj $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/document-test.obj document-test.cpp document.obj

./vc71/document-test.exe: vc71/document-test.obj vc71/encoder.obj
	link $(XS_FLAGS) /out:vc71/document-test.exe vc71/document-test.obj vc71/encoder.obj

document-test: vc71/document-test.exe
	vc71\document-test.exe



# break-iterator-test #######################################################

./vc71/break-iterator-test.obj: break-iterator-test.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/break-iterator-test.obj break-iterator-test.cpp

./vc71/break-iterator-test.exe: vc71/break-iterator-test.obj vc71/break-iterator.obj vc71/identifier-syntax.obj vc71/unicode-property.obj
	link $(XS_FLAGS) /out:vc71/break-iterator-test.exe vc71/break-iterator-test.obj vc71/break-iterator.obj vc71/identifier-syntax.obj vc71/unicode-property.obj

break-iterator-test: vc71/break-iterator-test.exe
	vc71\break-iterator-test.exe


# unicode-iterator-test #####################################################

./vc71/unicode-iterator-test.obj: unicode-iterator-test.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/unicode-iterator-test.obj unicode-iterator-test.cpp

./vc71/unicode-iterator-test.exe: vc71/unicode-iterator-test.obj vc71/unicode-property.obj vc71/identifier-syntax.obj
	link $(XS_FLAGS) /out:vc71/unicode-iterator-test.exe vc71/unicode-iterator-test.obj vc71/unicode-property.obj vc71/identifier-syntax.obj

unicode-iterator-test: vc71/unicode-iterator-test.exe
	vc71\unicode-iterator-test.exe


# case-folder-test ##########################################################

./vc71/case-folder-test.obj: case-folder-test.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/case-folder-test.obj case-folder-test.cpp

./vc71/case-folder-test.exe: vc71/case-folder-test.obj vc71/break-iterator.obj vc71/identifier-syntax.obj vc71/normalizer.obj vc71/unicode-property.obj
	link $(XS_FLAGS) /out:vc71/case-folder-test.exe vc71/case-folder-test.obj vc71/break-iterator.obj vc71/identifier-syntax.obj vc71/normalizer.obj vc71/unicode-property.obj

case-folder-test: vc71/case-folder-test.exe
	vc71\case-folder-test.exe


# normalizer-test ###########################################################

./vc71/normalizer-test.obj: normalizer-test.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/normalizer-test.obj normalizer-test.cpp

./vc71/normalizer-test.exe: vc71/normalizer-test.obj vc71/break-iterator.obj vc71/identifier-syntax.obj vc71/normalizer.obj vc71/unicode-property.obj
	link $(XS_FLAGS) /out:vc71/normalizer-test.exe vc71/normalizer-test.obj vc71/break-iterator.obj vc71/identifier-syntax.obj vc71/normalizer.obj vc71/unicode-property.obj

normalizer-test: vc71/normalizer-test.exe
	vc71\normalizer-test.exe

# regex-test ################################################################

./vc71/regex.obj: ../regex.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/regex.obj ../regex.cpp

./vc71/regex-test.obj: regex-test.cpp $(ALL_HEADER)
	cl $(CXX_FLAGS) /Fovc71/regex-test.obj regex-test.cpp

./vc71/regex-test.exe: vc71/regex-test.obj vc71/regex.obj vc71/break-iterator.obj vc71/encoder.obj vc71/identifier-syntax.obj vc71/unicode-property.obj
	link $(XS_FLAGS) /out:vc71/regex-test.exe vc71/regex-test.obj vc71/regex.obj vc71/break-iterator.obj vc71/encoder.obj vc71/identifier-syntax.obj vc71/unicode-property.obj

regex-test: vc71/regex-test.exe
	vc71\regex-test.exe
