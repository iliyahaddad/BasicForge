Module Benchmark

Sub Main()
    Dim sum As Integer = 0
    Dim i As Integer
    For i = 1 To 1000000
        sum = sum + i
    Next
    Print(sum)
    
    Dim product As Integer = 1
    For i = 1 To 12
        product = product * i
    Next
    Print(product)
    
    Dim x As Integer = 50
    Dim y As Integer = 25
    Print(x + y)
    Print(x - y)
    Print(x * y)
    Print(x / y)
End Sub

End Module