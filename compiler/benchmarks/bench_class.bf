Module Main
    Class Counter
        Public X As Integer
        Public Sub Add(ByVal n As Integer)
            X = X + n
        End Sub
    End Class
    Sub Main()
        Dim c As New Counter()
        Dim i As Integer
        For i = 1 To 1000000
            c.Add(1)
        Next
        Print(c.X)
    End Sub
End Module