' BasicForge standard library: Win32 SDK wrappers.
' Include inside a Module: Include "win32.bf"

Sub Message(text As String, caption As String)
    MessageBox(text, caption)
End Sub

Sub OpenPath(path As String)
    ShellExecute(path)
End Sub

Sub WaitMs(ms As Integer)
    Sleep(ms)
End Sub

Sub Sound(freq As Integer, ms As Integer)
    Beep(freq, ms)
End Sub

Sub SetClipText(text As String)
    SetClipboard(text)
End Sub

Function GetClipText() As String
    Return GetClipboard()
End Function

Sub SetWindowTitle(title As String)
    SetTitle(title)
End Sub