' BasicForge standard library: file helpers.
' Include inside a Module: Include "file.bf"

Function ReadTextFile(path As String) As String
    Return ReadAllText(path)
End Function

Sub WriteTextFile(path As String, text As String)
    WriteAllText(path, text)
End Sub

Sub AppendTextFile(path As String, text As String)
    AppendAllText(path, text)
End Sub

Function FileExistsP(path As String) As Boolean
    Return FileExists(path)
End Function

Function FileSize(path As String) As Integer
    Dim s As String
    s = ReadAllText(path)
    Return Len(s)
End Function