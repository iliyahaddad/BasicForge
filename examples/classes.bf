Module ClassesDemo

    Class Person
        Public Property Name As String
        Public Property Age As Integer
        
        Public Sub SayHello()
            Print("Hello, my name is " & Name & " and I am " & Age & " years old.")
        End Sub
    End Class

    Sub Main()
        Dim p As New Person()
        p.Name = "Alice"
        p.Age = 30
        p.SayHello()
    End Sub

End Module
