Module TryTest

Sub Main()
    Dim x As Integer = 10
    Try
        Print("In Try")
        If x > 5 Then
            Throw "Too big"
        End If
    Catch ex
        Print("Caught: " & ex)
    End Try
    Print("Done")
End Sub

End Module