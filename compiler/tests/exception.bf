Module ExceptionTest

Sub Main()
    Dim x As Integer = 10
    Try
        Print("In Try block")
        If x > 5 Then
            Throw "Error: value too large"
        End If
        Print("After If")
    Catch ex
        Print("Caught: " & ex)
    Finally
        Print("In Finally")
    End Try
    Print("After Try/Catch/Finally")
End Sub

End Module