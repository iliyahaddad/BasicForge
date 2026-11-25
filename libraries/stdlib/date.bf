' BasicForge standard library: date helpers.
' Include inside a Module: Include "date.bf"

Function IsLeapYear(y As Integer) As Boolean
    If y Mod 400 = 0 Then Return True
    If y Mod 100 = 0 Then Return False
    Return (y Mod 4 = 0)
End Function

Function DaysInMonth(y As Integer, m As Integer) As Integer
    If m = 2 Then
        If IsLeapYear(y) Then Return 29
        Return 28
    End If
    If m = 4 Or m = 6 Or m = 9 Or m = 11 Then Return 30
    Return 31
End Function

Function IsValidDate(y As Integer, m As Integer, d As Integer) As Boolean
    If m < 1 Or m > 12 Then Return False
    If d < 1 Then Return False
    Return d <= DaysInMonth(y, m)
End Function

Function NextYear(y As Integer) As Integer
    Return y + 1
End Function

Function IsWeekend(dayOfWeek As Integer) As Boolean
    Return dayOfWeek = 0 Or dayOfWeek = 6
End Function