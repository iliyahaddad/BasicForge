' BasicForge standard library: string helpers.
' Include inside a Module: Include "string.bf"

Function ToUpper(s As String) As String
    Return UCase(s)
End Function

Function ToLower(s As String) As String
    Return LCase(s)
End Function

Function Reverse(s As String) As String
    Dim i As Integer
    Dim r As String
    For i = Len(s) To 1 Step -1
        r = r & Mid(s, i, 1)
    Next
    Return r
End Function

Function Contains(s As String, part As String) As Boolean
    Return InStr(s, part) > 0
End Function

Function StartsWith(s As String, prefix As String) As Boolean
    If Len(s) < Len(prefix) Then Return False
    Return Left(s, Len(prefix)) = prefix
End Function

Function EndsWith(s As String, suffix As String) As Boolean
    If Len(s) < Len(suffix) Then Return False
    Return Right(s, Len(suffix)) = suffix
End Function

Function PadLeft(s As String, width As Integer, ch As String) As String
    While Len(s) < width
        s = ch & s
    End While
    Return s
End Function

Function PadRight(s As String, width As Integer, ch As String) As String
    While Len(s) < width
        s = s & ch
    End While
    Return s
End Function

Function Repeat(s As String, count As Integer) As String
    Dim i As Integer
    Dim r As String
    For i = 1 To count
        r = r & s
    Next
    Return r
End Function

Function IsBlank(s As String) As Boolean
    Return Len(Trim(s)) = 0
End Function

Function Truncate(s As String, maxLen As Integer) As String
    If Len(s) <= maxLen Then Return s
    Return Left(s, maxLen)
End Function