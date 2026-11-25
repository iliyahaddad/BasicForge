Module Main
    Include "array.bf"
    Sub Main()
        Dim a(100000) As Integer
        Dim i As Integer
        For i = 0 To 99999
            a(i) = i
        Next
        Print(SumArr(a, 100000))
    End Sub
End Module