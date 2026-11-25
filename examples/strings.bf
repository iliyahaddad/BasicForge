Module StringDemo

    Sub Main()
        Dim name As String = "BasicForge"
        Dim greeting As String = "Hello"
        
        Print(greeting & " " & name)
        Print("Length: " & Len(name))
        Print("Left 3: " & Left(name, 3))
        Print("Right 3: " & Right(name, 3))
        Print("Trim: '" & Trim("  " & name & "  ") & "'")
        Print("Replace: " & Replace(name, "Forge", "World"))
        Print("InStr: " & InStr(name, "For"))
    End Sub

End Module
