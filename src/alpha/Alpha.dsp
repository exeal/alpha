# Microsoft Developer Studio Project File - Name="Alpha" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Alpha - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "Alpha.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "Alpha.mak" CFG="Alpha - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "Alpha - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "Alpha - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Alpha - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib comctl32.lib imm32.lib ..\_ManahCore\Window\Release\uWindow.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Alpha - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib comctl32.lib imm32.lib ..\_ManahCore\Window\Debug\uWindow.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "Alpha - Win32 Release"
# Name "Alpha - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AdhocEventSink.cpp
# End Source File
# Begin Source File

SOURCE=.\Alpha.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaApp.idl
# End Source File
# Begin Source File

SOURCE=.\AlphaApplicationDebugger.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaApplicationObject.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaEditController.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaHtmlTab.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaScriptHost.cpp
# End Source File
# Begin Source File

SOURCE=.\AlphaView.cpp
# End Source File
# Begin Source File

SOURCE=.\BookmarkDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\CodePageManager.cpp
# End Source File
# Begin Source File

SOURCE=.\CodePagesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\CommonOptionDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfirmUnsavedDocumentDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugExpressionCallBack.cpp
# End Source File
# Begin Source File

SOURCE=.\DocTypeOptionDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Armaiti\DraggingTextDataObject.cpp
# End Source File
# Begin Source File

SOURCE=.\ExecuteCommandDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FileOperationDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FilePropertyDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FindDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GotoLineDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyboardMap.cpp
# End Source File
# Begin Source File

SOURCE=.\MRUManager.cpp
# End Source File
# Begin Source File

SOURCE=.\NewFileFormatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\OutlineParserDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\OutputWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\PluginManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptLanguageManager.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectLanguageDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\AdhocEventSink.h
# End Source File
# Begin Source File

SOURCE=.\Alpha.h
# End Source File
# Begin Source File

SOURCE=.\AlphaApplicationDebugger.h
# End Source File
# Begin Source File

SOURCE=.\AlphaApplicationObject.h
# End Source File
# Begin Source File

SOURCE=.\AlphaDoc.h
# End Source File
# Begin Source File

SOURCE=.\AlphaEditController.h
# End Source File
# Begin Source File

SOURCE=.\AlphaHtmlTab.h
# End Source File
# Begin Source File

SOURCE=.\AlphaInterfaces.h
# End Source File
# Begin Source File

SOURCE=.\AlphaScriptHost.h
# End Source File
# Begin Source File

SOURCE=.\AlphaTab.h
# End Source File
# Begin Source File

SOURCE=.\AlphaTabCtrl.h
# End Source File
# Begin Source File

SOURCE=.\AlphaView.h
# End Source File
# Begin Source File

SOURCE=.\BookmarkDlg.h
# End Source File
# Begin Source File

SOURCE=.\CodePageManager.h
# End Source File
# Begin Source File

SOURCE=.\CodePagesDlg.h
# End Source File
# Begin Source File

SOURCE=.\CommonOptionDlg.h
# End Source File
# Begin Source File

SOURCE=.\ConfirmUnsavedDocumentDlg.h
# End Source File
# Begin Source File

SOURCE=.\DebugDlg.h
# End Source File
# Begin Source File

SOURCE=.\DebugExpressionCallBack.h
# End Source File
# Begin Source File

SOURCE=.\DocTypeOptionDlg.h
# End Source File
# Begin Source File

SOURCE=..\Armaiti\DraggingTextDataObject.h
# End Source File
# Begin Source File

SOURCE=.\ExecuteCommandDlg.h
# End Source File
# Begin Source File

SOURCE=.\FileOperationDlg.h
# End Source File
# Begin Source File

SOURCE=.\FilePropertyDlg.h
# End Source File
# Begin Source File

SOURCE=.\FindDlg.h
# End Source File
# Begin Source File

SOURCE=.\GotoLineDlg.h
# End Source File
# Begin Source File

SOURCE=.\KeyboardMap.h
# End Source File
# Begin Source File

SOURCE=.\MRUManager.h
# End Source File
# Begin Source File

SOURCE=.\NewFileFormatDlg.h
# End Source File
# Begin Source File

SOURCE=.\OutlineParserDlg.h
# End Source File
# Begin Source File

SOURCE=.\OutputWnd.h
# End Source File
# Begin Source File

SOURCE=.\PluginManager.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ScriptLanguageManager.h
# End Source File
# Begin Source File

SOURCE=.\SelectLanguageDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Resource\alpha.ico
# End Source File
# Begin Source File

SOURCE=.\Alpha.rc
# End Source File
# Begin Source File

SOURCE=.\Resource\bmp_comm.bmp
# End Source File
# Begin Source File

SOURCE=.\Resource\bmp_dcomm.bmp
# End Source File
# Begin Source File

SOURCE=.\Resource\logo.bmp
# End Source File
# End Group
# Begin Group "Ascension"

# PROP Default_Filter ""
# Begin Group "Source Files No. 1"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\Ascension\AutoCompleteWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditController.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditPoint.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditView.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditViewCommands.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\KeyMacroPlayer.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\KeywordManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\LineLayout.cpp
# End Source File
# Begin Source File

SOURCE=.\Ascension\TextSearcher.cpp
# End Source File
# End Group
# Begin Group "Header Files No. 1"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\Ascension\AscensionCommon.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\AutoCompleteWnd.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditController.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditDoc.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditPoint.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\EditView.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\KeyMacroPlayer.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\KeywordManager.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\LineLayout.h
# End Source File
# Begin Source File

SOURCE=.\Ascension\TextSearcher.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\AlphaINI.xml
# End Source File
# Begin Source File

SOURCE=.\Resource\manifest.xml
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
