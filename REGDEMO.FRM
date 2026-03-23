VERSION 2.00
Begin Form Form1 
   BackColor       =   &H00C0C0C0&
   Caption         =   "Win32 RegDemo"
   ClientHeight    =   4980
   ClientLeft      =   1200
   ClientTop       =   1668
   ClientWidth     =   7584
   FontBold        =   -1  'True
   FontItalic      =   0   'False
   FontName        =   "Courier New"
   FontSize        =   7.8
   FontStrikethru  =   0   'False
   FontUnderline   =   0   'False
   Height          =   5304
   Icon            =   REGDEMO.FRX:0000
   Left            =   1152
   LinkTopic       =   "Form1"
   ScaleHeight     =   4980
   ScaleWidth      =   7584
   Top             =   1392
   Width           =   7680
   Begin CommandButton Command1 
      Caption         =   "Exit"
      Height          =   372
      Left            =   6180
      TabIndex        =   8
      Top             =   4560
      Width           =   972
   End
   Begin ListBox List2 
      FontBold        =   -1  'True
      FontItalic      =   0   'False
      FontName        =   "Courier New"
      FontSize        =   7.8
      FontStrikethru  =   0   'False
      FontUnderline   =   0   'False
      Height          =   3864
      Left            =   3540
      TabIndex        =   1
      Top             =   300
      Width           =   3972
   End
   Begin ListBox List1 
      FontBold        =   -1  'True
      FontItalic      =   0   'False
      FontName        =   "Courier New"
      FontSize        =   7.8
      FontStrikethru  =   0   'False
      FontUnderline   =   0   'False
      Height          =   3864
      Left            =   0
      TabIndex        =   0
      Top             =   300
      Width           =   3492
   End
   Begin Label Label5 
      BackColor       =   &H00C0C0C0&
      Caption         =   "* means key cannot be opened"
      Height          =   252
      Index           =   1
      Left            =   240
      TabIndex        =   7
      Top             =   4680
      Width           =   2772
   End
   Begin Label Label5 
      BackColor       =   &H00C0C0C0&
      Caption         =   "+ means additional keys"
      Height          =   252
      Index           =   0
      Left            =   240
      TabIndex        =   6
      Top             =   4440
      Width           =   2772
   End
   Begin Label Label4 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Double-Click a Value to edit it"
      Height          =   252
      Left            =   4020
      TabIndex        =   5
      Top             =   4200
      Width           =   3312
   End
   Begin Label Label3 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Double-Click a Key to expand it"
      Height          =   252
      Left            =   120
      TabIndex        =   4
      Top             =   4200
      Width           =   3612
   End
   Begin Label Label2 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Value Data"
      Height          =   192
      Left            =   3660
      TabIndex        =   3
      Top             =   60
      Width           =   1692
   End
   Begin Label Label1 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Key Name"
      Height          =   192
      Left            =   60
      TabIndex        =   2
      Top             =   60
      Width           =   1272
   End
End
Option Explicit
Dim lMainKey&
Dim sSubkey$
Dim iLevel%
Dim iX%
Dim iY%
Dim sLevelSpaces$
Dim iListIdx%

Sub Command1_Click ()
    'We want out!
    Unload Form1
End Sub

Sub Form_Load ()
    Dim lVerNum&
    Dim iVerWord%
    Dim iVersNum%
    
    CenterForm Me
    
    '-----------------------------------------------------
    'First we find out what Windows is running.  This will
    'not work with 16-bit Windows 3.x, and there are a
    'couple of registry keys in Win95 that are not present
    'in Windows NT
    '-----------------------------------------------------
    lVerNum = GetVersion()
    iVerWord = CInt(lVerNum And &HFFFF&)
    iVersNum = (iVerWord And &HFF) * 100 + CInt(iVerWord / 256)
	If GetWinFlags() And WF_WINNT Then
	    iWinVers = WinNT
	Else
	    'At this time, Win95 reports version 3.95
	    'when queried by 16-bit programs.  It returns
	    '4.00 when queried by 32-bit programs.
	    If iVersNum < 390 Then
		iWinVers = Win16
	    Else
		iWinVers = Win32
	    End If
	End If
    If iWinVers = Win16 Then
	MsgBox "This program is intended only for use with 32-bit Windows versions."
	Unload Form1
    End If
    
    '-----------------------------------------------------
    'Set all fonts to same, so horizontal scrollbars will work
    'Fixed fonts like Small Fonts or Courier will provide even tabs.
    '-----------------------------------------------------
    List1.FontName = FontName
    List1.FontSize = FontSize
    List2.FontName = FontName
    List2.FontSize = FontSize
    
    '-----------------------------------------------------
    'Put in global main keys
    '-----------------------------------------------------
    List1.AddItem "HKEY_CLASSES_ROOT"
    List1.AddItem "HKEY_CURRENT_USER"
    List1.AddItem "HKEY_LOCAL_MACHINE"
    List1.AddItem "HKEY_USERS"
    
    '-----------------------------------------------------
    'Put in version-specific main keys
    'HKEY_PERFORMANCE_DATA does not respond in a normal fashion
    'to key queries.  Note it does not appear in REGEDT32.
    'If iWinVers = WinNT Then
	'list1.AddItem "HKEY_PERFORMANCE_DATA"
    'End If
    '-----------------------------------------------------
    If iWinVers = Win32 Then
	List1.AddItem "HKEY_CURRENT_CONFIG"
	List1.AddItem "HKEY_DYN_DATA"
    End If
    
    '-----------------------------------------------------
    'Setup hourglass cursor
    '-----------------------------------------------------
    iWaitCursor = loadcursor(0, IDC_WAIT)

End Sub

Sub Form_Resize ()

    '-----------------------------------------------------
    'Resize controls when the user resizes the form.
    '-----------------------------------------------------
    On Error GoTo ReSizeError
    List1.Move 60, List1.Top, (ScaleWidth / 2) - 300, ScaleHeight - 1200
    List2.Move List1.Width + 90, List1.Top, List1.Width + 480, List1.Height
    
    Label1.Left = List1.Left + 60
    Label2.Left = List2.Left + 60
    
    Label3.Move 90, List1.Top + List1.Height + 150
    Label4.Move List2.Left + 90, Label3.Top
    
    Label5(0).Top = Label3.Top + 240
    Label5(1).Top = Label5(0).Top + 240
    
    Command1.Move ScaleWidth - Command1.Width - 120, Label4.Top + 330
    Exit Sub
ReSizeError:
    Exit Sub

End Sub

Sub List1_Click ()

    '-----------------------------------------------------
    'Empty the value list.
    '-----------------------------------------------------
    List2.Clear
    
    '-----------------------------------------------------
    'Remember which line was clicked when we start
    '   moving up and down the list.
    '-----------------------------------------------------
    iListIdx = List1.ListIndex
    
    '-----------------------------------------------------
    'Start by assuming toplevel key:
    '-----------------------------------------------------
    iLevel = 0
    
    If Left$(List1.List(List1.ListIndex), 1) <> Chr$(32) Then
	
	'-------------------------------------------------
	'Subkeys will all have spaces at their beginning (because
	'that is how we will add them), so this is a top level key.
	'-------------------------------------------------
	lMainKey = GetMainKey(CStr(List1.List(List1.ListIndex)))
	sSubkey = ""
    Else
	
	'-------------------------------------------------
	'This must be a subkey.
	'-------------------------------------------------
	sSubkey = List1.List(List1.ListIndex)
    
	'-------------------------------------------------
	'Strip the leading spaces and count how many
	'-------------------------------------------------
	Do Until Left$(sSubkey, 1) <> Chr$(32)
	    iLevel = iLevel + 1
	    sSubkey = Right$(sSubkey, Len(sSubkey) - 1)
	Loop
    
	'-------------------------------------------------
	'if there are any indicator characters which we have
	'introduced, strip them off.
	'-------------------------------------------------
	If Left$(sSubkey, 1) = "+" Or Left$(sSubkey, 1) = "*" Then
	    sSubkey = Right$(sSubkey, Len(sSubkey) - 1)
	End If
    
	'-------------------------------------------------
	'We indented 4 spaces per level when we added this subkey.
	'-------------------------------------------------
	iLevel = iLevel / 4
	For iX = iLevel To 1 Step -1
	    
	    '---------------------------------------------
	    'Build the complete key name by moving up the
	    'tree getting subkey names each time the indentation
	    'changes, until we run into a main key (no leading spaces).
	    '---------------------------------------------
	    sLevelSpaces = Space$(iX * 4)
	    iY = iListIdx
	    Do Until Left$(List1.List(iY), Len(sLevelSpaces)) <> sLevelSpaces
		iY = iY - 1
	    Loop
	    If Left$(List1.List(iY), 1) <> Chr$(32) Then
		lMainKey = GetMainKey(CStr(List1.List(iY)))
	    Else
		sSubkey = LTrim$(List1.List(iY)) & "\" & sSubkey
		If Left$(sSubkey, 1) = "+" Then
		    sSubkey = Right$(sSubkey, Len(sSubkey) - 1)
		End If
	    End If
	Next
    End If
    
    '-----------------------------------------------------
    'Now enumerate all values belonging to this key
    '-----------------------------------------------------
    lTempLong = RegEnumValues(lMainKey, sSubkey)
    
    '-----------------------------------------------------
    'If the user double-clicked, we will proceed on
    '    to that event.
    '-----------------------------------------------------
End Sub

Sub List1_DblClick ()
    
    '----------------------------------------------------------
    'Double-Click event occurs after the Click event, so
    'values for lMainKey and sSubKey have already been determined
    'there.  Now we will deal with additional keys under this
    'one.
    '----------------------------------------------------------
    Dim bAlreadyExpanded%
    Dim iListWidth%
    
    sLevelSpaces = Space$((iLevel + 1) * 4)
    iX = iListIdx + 1
    
    '----------------------------------------------------------
    'If the next item in the list box has more spaces than
    'our current subkey, the tree is already expanded.  Collapse
    'the tree until there are no subkeys to the current subkey.
    '----------------------------------------------------------
    Do Until Left$(List1.List(iX), Len(sLevelSpaces)) <> sLevelSpaces
	List1.RemoveItem iX
	bAlreadyExpanded = True
    Loop
    
    '----------------------------------------------------------
    'If we collapsed the tree, we are done
    '----------------------------------------------------------
    If bAlreadyExpanded Then
	Exit Sub
    End If
    
    '----------------------------------------------------------
    'Otherwise we want to expand the tree one level
    '----------------------------------------------------------
    lTempLong = RegEnumKeys(lMainKey, sSubkey, iLevel, iListIdx + 1)
    
    '----------------------------------------------------------
    'Set Horizontal Scrollbar if necessary
    '----------------------------------------------------------
    iTempInt = SendMessage(List1.hWnd, LB_SETHORIZONTALEXTENT, iListWidth, 0&)
    For iX = 0 To List1.ListCount - 1
	lTempLong = GetTextExtent(hDC, List1.List(iX), Len(List1.List(iX)))
	iTempInt = WordLo(lTempLong)
	If iTempInt > iListWidth Then
	    iListWidth = iTempInt
	    iTempInt = SendMessage(List1.hWnd, LB_SETHORIZONTALEXTENT, iListWidth, 0&)
	End If
    Next iX


End Sub

Sub List2_DblClick ()

    '----------------------------------------------------------
    'Call EditRegValue to load the value into the editor.
    '----------------------------------------------------------
    EditRegValue lMainKey, sSubkey, CLng(List2.ListIndex)

End Sub

