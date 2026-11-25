Module TypesDemo

    Sub Main()
        Dim age As Integer = 25
        Dim big As Long = 1000000
        Dim rate As Single = 3.14
        Dim precise As Double = 3.1415926535
        Dim flag As Boolean = True
        Dim text As String = "Hello"
        
        Print("age = " & age)
        Print("big = " & big)
        Print("rate = " & rate)
        Print("precise = " & precise)
        Print("flag = " & flag)
        Print("text = " & text)
        
        ' Type conversion
        Print("CInt(3.7) = " & CInt(3.7))
        Print("CStr(42) = " & CStr(42))
        Print("CBool(0) = " & CBool(0))
        Print("CBool(1) = " & CBool(1))
    End Sub

End Module
