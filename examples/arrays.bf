Module ArraysDemo

    Sub Main()
        Dim numbers(4) As Integer
        
        numbers(0) = 10
        numbers(1) = 20
        numbers(2) = 30
        numbers(3) = 40
        numbers(4) = 50
        
        Dim i As Integer
        For i = 0 To 4
            Print("numbers(" & i & ") = " & numbers(i))
        Next
        
        Dim sum As Integer = 0
        For i = 0 To 4
            sum = sum + numbers(i)
        Next
        Print("Sum: " & sum)
    End Sub

End Module
