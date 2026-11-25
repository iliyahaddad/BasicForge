' BasicForge standard library: CSV helpers.
' Include inside a Module: Include "csv.bf"

Function CsvEscape(s As String) As String
    If InStr(s, ",") > 0 Or InStr(s, "\"") > 0 Or InStr(s, Chr(10)) > 0 Then
        Return "\"" & Replace(s, "\"", "\"\"") & "\""
    End If
    Return s
End Function

Function CsvRow(a As String, b As String) As String
    Return CsvEscape(a) & "," & CsvEscape(b)
End Function

Function CsvRow3(a As String, b As String, c As String) As String
    Return CsvEscape(a) & "," & CsvEscape(b) & "," & CsvEscape(c)
End Function