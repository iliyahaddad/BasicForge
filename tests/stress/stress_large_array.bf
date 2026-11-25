Module Main
    Sub Main()
        Dim a(1000000) As Integer
        Dim i As Integer
        For i = 0 To 999999
            a(i) = i
        Next
        Print(a(999999))
    End Sub
End Module