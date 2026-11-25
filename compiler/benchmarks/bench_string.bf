Module Main
    Sub Main()
        Dim i As Integer
        Dim s As String
        s = ""
        For i = 1 To 10000
            s = s & "x"
        Next
        Print(Len(s))
    End Sub
End Module