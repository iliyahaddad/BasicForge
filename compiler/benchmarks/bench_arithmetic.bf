Module Main
    Sub Main()
        Dim i As Integer
        Dim s As Integer
        s = 0
        For i = 1 To 10000000
            s = s + i
        Next
        Print(s)
    End Sub
End Module