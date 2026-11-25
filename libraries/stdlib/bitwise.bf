' BasicForge standard library: bitwise helpers.
' Include inside a Module: Include "bitwise.bf"

Function BitValue(bit As Integer) As Integer
    Dim p As Integer
    p = 1
    Dim i As Integer
    For i = 1 To bit
        p = p * 2
    Next
    Return p
End Function

Function TestBit(v As Integer, bit As Integer) As Boolean
    Return (v \ BitValue(bit)) Mod 2 = 1
End Function

Function SetBit(v As Integer, bit As Integer) As Integer
    Dim p As Integer
    p = BitValue(bit)
    If (v \ p) Mod 2 = 0 Then
        Return v + p
    End If
    Return v
End Function

Function ClearBit(v As Integer, bit As Integer) As Integer
    Dim p As Integer
    p = BitValue(bit)
    If (v \ p) Mod 2 = 1 Then
        Return v - p
    End If
    Return v
End Function

Function ToggleBit(v As Integer, bit As Integer) As Integer
    if TestBit(v, bit) Then Return ClearBit(v, bit)
    Return SetBit(v, bit)
End Function

Function Shl(v As Integer, n As Integer) As Integer
    Dim i As Integer
    For i = 1 To n
        v = v * 2
    Next
    Return v
End Function

Function Shr(v As Integer, n As Integer) As Integer
    Dim i As Integer
    For i = 1 To n
        v = v \ 2
    Next
    Return v
End Function

Function IsPowerOfTwo(v As Integer) As Boolean
    If v <= 0 Then Return False
    While v Mod 2 = 0
        v = v \ 2
    End While
    Return v = 1
End Function