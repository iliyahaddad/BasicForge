Module Main
    ' Native Win32 form. Run it and click the button.
    Form MainForm
        Width = 640
        Height = 480
        Text = "BasicForge Forms"

        Button btnOK
            Text = "Click me"
            Left = 20
            Top = 20
            Width = 120
            Height = 34
            OnClick = btnOK_Click
        End Button

        TextBox txt
            Text = "Hello from BasicForge"
            Left = 20
            Top = 70
            Width = 260
            Height = 26
        End TextBox
    End Form

    Sub Main()
        ' Forms are created automatically; Main runs before the message loop.
        Print("Starting form...")
    End Sub

    Sub btnOK_Click()
        Print("Button clicked!")
    End Sub
End Module