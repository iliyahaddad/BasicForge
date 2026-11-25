' BasicForge standard library: random helpers.
' Include inside a Module: Include "random.bf"

Function RandInt(lo As Integer, hi As Integer) As Integer
    Dim r As Double
    r = Rnd()
    If r >= 1.0 Then r = 0.999999
    Return lo + CInt(r * (hi - lo + 1))
End Function

Function Chance(percent As Integer) As Boolean
    Return RandInt(1, 100) <= percent
End Function

Function BoolRand() As Boolean
    Return RandInt(0, 1) = 1
End Function