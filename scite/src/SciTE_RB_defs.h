#pragma once
/*
SciTE RuBoard additions defines
*/
//-------------------------------------------------------------------------

// [BatchLexerImprovement]
#define RB_BALI

// [BetterCalltips]
#define RB_BTCT1 // Ctrl + Up/Down for switch tooltip pages
#define RB_BTCT2 // "calltip.*.automatic" disable autopopup tooltip
// not implemented
//"calltip.*.show.per.page"
//"calltip.*.word.wrap"
#define RB_BTCT3 // Fix colorize calltip

// [BufferNumber]
#define RB_BUFFNUMBER

// [CalltipBreaks]
#define RB_CTBR

// [CheckFileExist]
#define RB_CFE

// [CheckMenus]
#define RB_CheckMenus

// [EditorUnicodeMode]
#define RB_EUM

// [EncodingToLua]
// export strings functions for utf8 
#define RB_UTF8

// [English_KeyCode]
#define RB_ENKEY

// [ErrorLineBack]
#define RB_ELB

// [EventInvalid]
// disabled LuaExtension::OnSave
//#define RB_EVINV //?

// [ExtendedContextMenu]
#define RB_ECM

// [FileAttr in PROPS]
#define RB_FAINP

// [FindResultListStyle]
#define RB_FRLS

// [FixEncoding][EncodingToLua]
#define RB_ENCODING

// [FixFind]
#define RB_FixFind

// [FixReplaceOnce]
// fixed in vanilla

// [ForthImprovement]
#define RB_FRIM

// [GetApplicationProps]
#define RB_GAP

// [GetWordChars]
#define RB_GWC

// [GoMessageFix]
#define RB_GMFIX

// [GoMessageImprovement]
#define RB_GMI

// [InMultiWordsList]
#define RB_IMWL

// [InputErr]
#define RB_IERR

// [InsertAbbreviation]
#define RB_IA

// [InsertMultiPasteText]
#define RB_IMLTPT

// [LangMenuChecker]
#define RB_LangMenuChecker

// [LexersFoldFix]
#define RB_LFF

// [LexersLastWordFix]
#define RB_LLWF

// [LocalizationFromLua]
#define RB_LFL

// [LuaLexerImprovement]
// del

// [MoreRecentFiles]
#define RB_MoreRecentFiles

// [MouseClickHandled]
#define RB_MCH

// [NewBufferPosition]
#define RB_NBP

// [NewFind-MarkerDeleteAll]
#define RB_NFMDA

// [OnClick]
#define RB_ONCLICK

// [OnDoubleClick]
#define RB_ODBCLK

// [OnFinalise]
#define RB_FIN

// [OnHotSpotReleaseClick]
#define RB_OHSC

// [OnKey]
#define RB_ONKEY

// [OnMenuCommand]
#define RB_OMC

// [OnMouseButtonUp]
#define RB_OMBU

// [OnSendEditor]
#define RB_OnSendEditor

// [OpenNonExistent]
#define RB_ONE

// [ParametersDialogFromLua]
#define RB_PDFL

// [Perform]
// export editor.Perform(...)
#define RB_Perform

// [PropsColouriseFix]
#define RB_PCF
 
// [PropsKeysSets]
#define RB_PKS

// [PropsKeywords]
#define RB_PKW

// [ReadOnlyTabMarker]
#define RB_ROTM

// [ReloadStartupScript]
#define RB_RSS

// [ReturnBackAfterRALL]
#define RB_RBAFALL

// [SaveEnabled]
#define RB_SE

// [SetBookmark]
#define RB_SB

// [StartupScriptReload]
// add scite.ReloadStartupScript()
#define RB_SSR

// [StyleDefHotspot]
#define RB_HOTSPOT

// [StyleDefault]
#define RB_SD

// [SubMenu]
#define RB_SUBMENU

// [TabbarTitleMaxLength]
#define RB_TTML

// [TabsMoving]
#define RB_TM

// [ToolsMax]
#define RB_ToolsMax

// [UserListItemID]
#define RB_ULID

// [UserPropertiesFilesSubmenu]
#define RB_UserPropertiesFilesSubmenu

// [VBLexerImprovement]
#define RB_VBLI

// [WarningMessage]
#define RB_WRNM

// [Zoom]
#ifdef RB_OnSendEditor
#define RB_ZOOM
#endif

// [ZorderSwitchingOnClose]
#ifdef RB_GAP
#define RB_ZSOC
#endif

// [autocompleteword.incremental]
#define RB_ACI

// [caret]
#define RB_CARET

// [close_on_dbl_clk]
#define RB_TAB_DB_CLICK

// [cmdline.spaces.fix]
#define RB_CLSF

// [find.fillout]
#define RB_FINDFILL

// [find_in_files_no_empty]
#define RB_FFNOE

// [fix_invalid_codepage]
#define RB_FIXCP

// [ignore_overstrike_change]
#define RB_IOCH

// [import]
#define RB_IMPORT

// [macro]
#define RB_MACRO

// [new_on_dbl_clk]
#define RB_NODBCLK

// [oem2ansi]
#define RB_OEM2ANSI

// [output.caret]
#define RB_OUTCARET

// [save.session.multibuffers.only]
#define RB_SSMO

// [scite.userhome]
#define RB_SUH

// [selection.hide.on.deactivate] -> "selection.always.visible"

// [session.close.buffers.onload]
#define RB_SCBO

// [update.inno]
#define RB_UPIN

// [user.toolbar]
#define RB_UT

// [utf8.auto.check]
#define RB_UTF8AC

// [warning.couldnotopenfile.disable]
#define RB_WNRSUPRESS

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// ��������� ����, ������ ������ �����
#define RB_LINK

// [user_shortcuts]
#define RB_USERSC

// ��������� ������� ������� ����������� �������
#define RB_ONTABMOVE

// ������� ��� �������� ��������
#define RB_TABTOP

// ��������� ���������� ��� ������� ��� ������� �������
#define RB_TABMA

// �������������� ������ �������� ���� ������ 354-�� ��� ���� (SciTExmas.ico)
#define RB_DOY

// [clear_before_execute]
// �������� clearbefore:[1,0,yes,no] ���������� ��������� clear.before.execute ��� ���������� �������
#define RB_CBE

// props['GetCurrentWord']
#define RB_GETCURWORD

// �������� ������ �������������� ��� ����������� ����
#define RB_ACCANCELONMOVE

// ��� ���������������� ������� ������ ��������� �������� 'Num *' ������ 'KeypadMultiply' � �.�.
#define RB_USCNUM

// �������� ������ ������� ��� ������������ ������� props["hide.filterstrip.on.switch.tab"]
#define RB_HFS

// ������������� ������ � ����������� ��������� �������� ������
#define RB_MCIVEX

// ����������� ��������� ��� ������ ���������������� ������
#define RB_USBTT

// ����������� ���� ��������� � ������� ��� ����������� ���������
#define RB_ONMOVESPL

// ����� � �������� ���������
#define RB_CREDITS

// ��������� �������� ��������� ��� ��������� �������� � �������� ���������� ���������������� ������
#define RB_USVC

// �� ��������� ������ � ���� �� ������������������ ����� � ������ �����
#define RB_HKFIX

// fix LuaExtension::OnExecute
#define RB_LEONE

// fix loading of startupScript
// ��������� ��������� ���� � �������������� ���������
#define RB_SF

// ���������� ����������� �������� � ������ ��� ������ �� ������ ��������������
#define RB_ACMERGE
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

