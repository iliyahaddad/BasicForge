Module ConditionsDemo

    Sub Main()
        Dim age As Integer = 25
        
        If age >= 18 Then
            Print("Adult")
        Else
            Print("Minor")
        End If
        
        Dim score As Integer = 85
        
        Select Case score
            Case 90 To 100
                Print("Grade: A")
            Case 80 To 89
                Print("Grade: B")
            Case 70 To 79
                Print("Grade: C")
            Case Else
                Print("Grade: F")
        End Select
    End Sub

End Module
