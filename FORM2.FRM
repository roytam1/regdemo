VERSION 2.00
Begin Form Form2 
   BackColor       =   &H00C0C0C0&
   Caption         =   "Form2"
   ClientHeight    =   2784
   ClientLeft      =   4560
   ClientTop       =   3816
   ClientWidth     =   5520
   Height          =   3108
   Left            =   4512
   LinkTopic       =   "Form2"
   ScaleHeight     =   2784
   ScaleWidth      =   5520
   Top             =   3540
   Width           =   5616
   Begin TextBox Text4 
      Height          =   288
      Left            =   4080
      TabIndex        =   10
      Text            =   "Text4"
      Top             =   2460
      Visible         =   0   'False
      Width           =   492
   End
   Begin TextBox Text3 
      Height          =   288
      Left            =   3180
      TabIndex        =   9
      Text            =   "Text3"
      Top             =   2460
      Visible         =   0   'False
      Width           =   492
   End
   Begin Frame Frame1 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Base"
      Height          =   912
      Left            =   360
      TabIndex        =   7
      Top             =   1680
      Visible         =   0   'False
      Width           =   2232
      Begin OptionButton Option1 
         BackColor       =   &H00C0C0C0&
         Caption         =   "Decimal"
         Height          =   252
         Index           =   1
         Left            =   360
         TabIndex        =   0
         Top             =   540
         Width           =   1512
      End
      Begin OptionButton Option1 
         BackColor       =   &H00C0C0C0&
         Caption         =   "Hexadecimal"
         Height          =   252
         Index           =   0
         Left            =   360
         TabIndex        =   8
         Top             =   240
         Value           =   -1  'True
         Width           =   1512
      End
   End
   Begin CommandButton Command1 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   312
      Index           =   1
      Left            =   4140
      TabIndex        =   6
      Top             =   1920
      Width           =   912
   End
   Begin CommandButton Command1 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   312
      Index           =   0
      Left            =   2940
      TabIndex        =   5
      Top             =   1920
      Width           =   912
   End
   Begin TextBox Text2 
      Height          =   312
      Left            =   300
      TabIndex        =   4
      Top             =   1260
      Width           =   4812
   End
   Begin TextBox Text1 
      BackColor       =   &H00C0C0C0&
      Enabled         =   0   'False
      Height          =   312
      Left            =   300
      TabIndex        =   2
      Top             =   420
      Width           =   4812
   End
   Begin Label Label1 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Value Data:"
      Height          =   312
      Left            =   360
      TabIndex        =   3
      Top             =   840
      Width           =   1632
   End
   Begin Label Label2 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Value Name:"
      Height          =   252
      Left            =   360
      TabIndex        =   1
      Top             =   60
      Width           =   2172
   End
End
Option Explicit
Dim bPainted%

Sub Command1_Click (iIndex%)
    Dim nRtn&
    Dim hKey&
    Dim varTempVar
    Dim sSubKey$
    Dim sValueName$
    Dim sValueData$
    Select Case iIndex
        Case 0
            
            '-------------------------------------------------
            'OK was clicked
            'Values were loaded into visible and
            'hidden text boxes by the EditRegValue function.
            '-------------------------------------------------
            hKey = CLng(text3)
            sSubKey = text4
            sValueData = text2
            sValueName = Text1
            If sValueName = "(Default)" Then
                
                '---------------------------------------------
                'We are visually displaying (Default) when the
                'value name is null.
                '---------------------------------------------
                sValueName = ""
            End If

            '-------------------------------------------------
            'We handle DWORDs differently than strings
            '-------------------------------------------------
            If lCurrentType = REG_DWORD Then
                
                '---------------------------------------------
                'We do some tortuous maneuvers to make sure
                'that all values are hex longs, including decimal
                'values greater than 7FFFFFFF
                '---------------------------------------------
                varTempVar = text2
                If Option1(0) Then
                    varTempVar = "&H" & varTempVar
                End If
                If IsNumeric(varTempVar) Then
                    If Option1(0) Then
                        sValueData = text2
                    Else
                        fTempDbl = Val(text2 & "#")
                        If fTempDbl > &H7FFFFFFF Then
                            sValueData = Hex$(fTempDbl - 4294967296#)
                        Else
                            sValueData = Hex$(fTempDbl)
                        End If
                    End If
                Else
                    '-----------------------------------------
                    'We have a DWORD value, but the text box
                    'does not contain an number.
                    '-----------------------------------------
                    Exit Sub
                End If
            End If
            sTempString = sValueData
            
            '-------------------------------------------------
            'Write new value to the registry
            '-------------------------------------------------
            nRtn = RegSetValue(hKey, sSubKey, sValueName, sValueData)
            
            '-------------------------------------------------
            'If the value was successfully changed, update the list box
            '-------------------------------------------------
            If nRtn = 0 Then
                sValueData = sTempString
                sTempString = Text1    'gets us (Default) back if no value name
                If lCurrentType = REG_SZ Then
                    sValueData = """" & sValueData & """"
                End If
                If lCurrentType = REG_DWORD Then
                    
                    '-----------------------------------------
                    'Tortuous calculations again to display positive
                    'integer values greater than a VB long may contain.
                    '-----------------------------------------
                    If Len(sValueData) < 5 Then
                        sValueData = sValueData & "&"
                    End If
                    fTempDbl = Val("&H" & sValueData)
                    If fTempDbl < 0 Then
                        fTempDbl = fTempDbl + 4294967296#
                    End If
                    If Right$(sValueData, 1) = "&" Then
                        sValueData = Left$(sValueData, Len(sValueData) - 1)
                    End If
                    sValueData = "&H" & UCase(sValueData) & " (" & Format$(fTempDbl) & ")"
                End If

                '---------------------------------------------
                'The iList2Tab value comes from the RegEnumValues function,
                'and tells us how to format our output to match the other
                'values in the same list.
                '---------------------------------------------
                sTempString = sTempString & Space$(iList2Tab - Len(sValueName) + 1) & sValueData
                iTempInt = form1!List2.ListIndex

                '---------------------------------------------
                'Remove the old, add the new.
                '---------------------------------------------
                form1!List2.RemoveItem iTempInt
                form1!List2.AddItem sTempString, iTempInt
                form1!List2.ListIndex = iTempInt
            End If
            Unload form2
    
        Case 1
            
            '-------------------------------------------------
            'Cancel was clicked
            '-------------------------------------------------
            Unload form2
    End Select
End Sub

Sub Form_Load ()
    CenterForm Me
End Sub

Sub Form_Paint ()
    If Not bPainted Then
        bPainted = True
        text2.SelStart = 0
        text2.SelLength = Len(text2) + 1
    End If
End Sub

Sub Form_Unload (Cancel As Integer)
bPainted = False
End Sub

Sub Option1_Click (iIndex%)
Select Case iIndex
    Case 0
        
        '------------------------------------------
        'Convert from decimal to Hex
        '------------------------------------------
        sTempString = text2
        If Val(sTempString) < 0 Then
            
            '--------------------------------------
            'Force values that otherwise would be
            'given negative integer numbers to type
            'long.
            '--------------------------------------
            sTempString = sTempString & "&"
        End If
        fTempDbl = Val(text2 & "#")
            
            '--------------------------------------
            'Hex function will fail on values outside
            'the normal VB long range.  Convert such
            'to a negative value within the range
            '--------------------------------------
            If fTempDbl > &H7FFFFFFF Then
                text2 = Hex$(fTempDbl - 4294967296#)
            Else
                text2 = Hex$(fTempDbl)
            End If
    Case 1
        
        '------------------------------------------
        'Convert from Hex to decimal
        '------------------------------------------
        sTempString = text2
        If Len(sTempString) < 5 Then
            
            '--------------------------------------
            'Force values that otherwise would be
            'given negative integer numbers to type
            'long.
            '--------------------------------------
            sTempString = sTempString & "&"
        End If
        fTempDbl = Val("&H" & sTempString)
        If fTempDbl < 0 Then
            
            '--------------------------------------
            'Val function will return a negative long
            'for hex numbers greater than 7FFFFFFF.
            'Convert these to positive values for
            'display purposes.
            '--------------------------------------
            fTempDbl = fTempDbl + 4294967296#
        End If
        text2 = Format$(fTempDbl)
End Select
End Sub

