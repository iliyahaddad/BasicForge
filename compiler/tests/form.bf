Module Main
    Form TestForm
        Width = 400
        Height = 300
        Text = "Test"
        Button btnOK
            Text = "OK"
            Left = 20
            Top = 20
            Width = 100
            Height = 32
            OnClick = btnOK_Click
        End Button
        TextBox txt
            Text = "hello"
            Left = 20
            Top = 70
            Width = 200
            Height = 24
        End TextBox
    End Form
    Sub Main()
        bf_form_test_click(0)
        Print(GetText("txt"))
        bf_form_close_first()
    End Sub
    Sub btnOK_Click()
        Print("clicked")
    End Sub
End Module