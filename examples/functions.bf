Module FunctionsDemo

    Function Add(a As Integer, b As Integer) As Integer
        Return a + b
    End Function

    Function Multiply(a As Integer, b As Integer) As Integer
        Return a * b
    End Function

    Sub Main()
        Dim x As Integer = Add(10, 20)
        Dim y As Integer = Multiply(x, 3)
        
        Print("Add(10, 20) = " & x)
        Print("Multiply(" & x & ", 3) = " & y)
    End Sub

End Module
