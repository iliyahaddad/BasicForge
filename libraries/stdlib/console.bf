' BasicForge standard library: console helpers.
' Include inside a Module: Include "console.bf"

Sub ClearScreen()
    Clear
End Sub

Sub SetWindowTitle(title As String)
    SetTitle(title)
End Sub

Sub PrintLine(text As String)
    Print(text)
End Sub

Function ReadInput() As String
    Return ReadLine()
End Function

Sub WaitMs(ms As Integer)
    Sleep(ms)
End Sub

Sub Sound(freq As Integer, ms As Integer)
    Beep(freq, ms)
End Sub