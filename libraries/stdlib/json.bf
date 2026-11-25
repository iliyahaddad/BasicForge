' BasicForge standard library: JSON helpers.
' Include inside a Module: Include "json.bf"

Function JsonEscape(s As String) As String
    Dim i As Integer
    Dim c As String
    Dim r As String
    For i = 1 To Len(s)
        c = Mid(s, i, 1)
        If c = "\"" Then
            r = r & "\\\""
        ElseIf c = "\\" Then
            r = r & "\\\\"
        ElseIf c = Chr(10) Then
            r = r & "\\n"
        ElseIf c = Chr(13) Then
            r = r & "\\r"
        ElseIf c = Chr(9) Then
            r = r & "\\t"
        Else
            r = r & c
        End If
    Next
    Return r
End Function

Function JsonString(s As String) As String
    Return "\"" & JsonEscape(s) & "\""
End Function

Function JsonNumber(n As Integer) As String
    Return CStr(n)
End Function

Function JsonBool(b As Boolean) As String
    If b Then Return "true"
    Return "false"
End Function

Function JsonObject(key As String, value As String) As String
    Return JsonString(key) & ": " & JsonString(value)
End Function