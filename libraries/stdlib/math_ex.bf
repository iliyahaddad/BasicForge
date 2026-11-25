' BasicForge standard library: extended math helpers.
' Include inside a Module: Include "math_ex.bf"

Function Round(v As Double, digits As Integer) As Double
    Dim p As Double
    p = 1.0
    Dim i As Integer
    For i = 1 To digits
        p = p * 10.0
    Next
    Return Int(v * p + 0.5) / p
End Function

Function Floor(v As Double) As Double
    If v >= 0.0 Then Return Int(v)
    If v = Int(v) Then Return v
    Return Int(v) - 1.0
End Function

Function Ceil(v As Double) As Double
    If v = Int(v) Then Return v
    Return Int(v) + 1.0
End Function

Function ClampD(v As Double, lo As Double, hi As Double) As Double
    If v < lo Then Return lo
    If v > hi Then Return hi
    Return v
End Function

Function Lerp(a As Double, b As Double, t As Double) As Double
    Return a + (b - a) * t
End Function

Function MaxD(a As Double, b As Double) As Double
    If a > b Then Return a
    Return b
End Function

Function MinD(a As Double, b As Double) As Double
    If a < b Then Return a
    Return b
End Function