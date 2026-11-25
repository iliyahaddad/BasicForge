' BasicForge standard library: validation helpers.
' Include inside a Module: Include "validation.bf"
Include "string.bf"

Function IsNumber(s As String) As Boolean
    Dim i As Integer
    Dim c As String
    Dim n As Integer
    If Len(s) = 0 Then Return False
    For i = 1 To Len(s)
        c = Mid(s, i, 1)
        n = Asc(c)
        If n < 48 Or n > 57 Then Return False
    Next
    Return True
End Function

Function IsIntegerText(s As String) As Boolean
    Return IsNumber(s)
End Function

Function IsEmpty(s As String) As Boolean
    Return Len(Trim(s)) = 0
End Function

Function IsEmail(s As String) As Boolean
    Return InStr(s, "@") > 0 And InStr(s, ".") > 0
End Function

Function IsUrl(s As String) As Boolean
    Return StartsWith(s, "http://") Or StartsWith(s, "https://")
End Function