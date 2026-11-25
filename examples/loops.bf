Module LoopsDemo

    Sub Main()
        Dim i As Integer
        
        Print("For loop 1 to 5:")
        For i = 1 To 5
            Print("i = " & i)
        Next
        
        Print("For loop with step 2:")
        For i = 2 To 10 Step 2
            Print("i = " & i)
        Next
        
        Dim count As Integer = 0
        While count < 3
            Print("While iteration: " & count)
            count = count + 1
        End While
    End Sub

End Module
