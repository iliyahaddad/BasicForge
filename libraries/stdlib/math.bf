' BasicForge standard library: math helpers.
' Include inside a Module: Include "math.bf"

Function Clamp(v As Integer, lo As Integer, hi As Integer) As Integer
    If v < lo Then Return lo
    If v > hi Then Return hi
    Return v
End Function

Function MaxI(a As Integer, b As Integer) As Integer
    If a > b Then Return a
    Return b
End Function

Function MinI(a As Integer, b As Integer) As Integer
    If a < b Then Return a
    Return b
End Function

Function AbsI(v As Integer) As Integer
    If v < 0 Then Return -v
    Return v
End Function

Function IsPrime(n As Integer) As Boolean
    If n < 2 Then Return False
    Dim i As Integer
    i = 2
    While i * i <= n
        If n Mod i = 0 Then Return False
        i = i + 1
    End While
    Return True
End Function

Function Gcd(a As Integer, b As Integer) As Integer
    Dim t As Integer
    While b <> 0
        t = b
        b = a Mod b
        a = t
    End While
    Return AbsI(a)
End Function

Function Lcm(a As Integer, b As Integer) As Integer
    If a = 0 Or b = 0 Then Return 0
    Dim r As Integer
    r = CInt((a / Gcd(a, b)) * b)
    Return AbsI(r)
End Function

Function Factorial(n As Integer) As Integer
    Dim r As Integer
    Dim i As Integer
    r = 1
    i = 2
    While i <= n
        r = r * i
        i = i + 1
    End While
    Return r
End Function

Function DegToRad(deg As Double) As Double
    Return deg * 3.14159265358979 / 180.0
End Function

Function RadToDeg(rad As Double) As Double
    Return rad * 180.0 / 3.14159265358979
End Function

Function Sign(v As Integer) As Integer
    If v > 0 Then Return 1
    If v < 0 Then Return -1
    Return 0
End Function