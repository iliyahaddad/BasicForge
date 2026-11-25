Module TestBench

Sub Main()
    Dim sum As Integer = 0
    Dim i As Integer
    For i = 1 To 100000
        sum = sum + i
    Next
    Print(sum)
End Sub

End Module
