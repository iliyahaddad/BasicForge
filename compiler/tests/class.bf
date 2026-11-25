Module Main
    Class Counter
        Public Value As Integer
        Public Sub Add(ByVal n As Integer)
            Me.Value = Me.Value + n
        End Sub
        Public Function Get() As Integer
            Return Me.Value
        End Function
    End Class
    Sub Main()
        Dim c As New Counter()
        c.Add(7)
        c.Add(5)
        Print(c.Get())
    End Sub
End Module