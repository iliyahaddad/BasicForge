' BasicForge standard library: extended string helpers.
' Include inside a Module: Include "strings_ex.bf"
Include "string.bf"

Function CountOccurrences(s As String, part As String) As Integer
    Dim count As Integer
    Dim start As Integer
    Dim pos As Integer
    count = 0
    start = 1
    While start <= Len(s)
        pos = InStr(Mid(s, start, Len(s)), part)
        If pos = 0 Then Return count
        count = count + 1
        start = start + pos + Len(part) - 1
    End While
    Return count
End Function

Function Insert(s As String, at As Integer, part As String) As String
    Return Left(s, at - 1) & part & Mid(s, at, Len(s))
End Function

Function RemoveAt(s As String, at As Integer, count As Integer) As String
    Return Left(s, at - 1) & Mid(s, at + count, Len(s))
End Function

Function ReplaceAll(s As String, old As String, replacement As String) As String
    Return Replace(s, old, replacement)
End Function

Function SplitCount(s As String, sep As String) As Integer
    Return CountOccurrences(s, sep) + 1
End Function

Function Join(a As String, b As String, sep As String) As String
    Return a & sep & b
End Function