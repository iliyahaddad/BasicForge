' BasicForge standard library: array helpers.
' Include inside a Module: Include "array.bf"

Sub SwapInt(arr() As Integer, i As Integer, j As Integer)
    Dim t As Integer
    t = arr(i)
    arr(i) = arr(j)
    arr(j) = t
End Sub

Sub SortAsc(arr() As Integer, n As Integer)
    Dim i As Integer
    Dim j As Integer
    For i = 0 To n - 2
        For j = 0 To n - 2 - i
            If arr(j) > arr(j + 1) Then
                SwapInt(arr, j, j + 1)
            End If
        Next
    Next
End Sub

Sub SortDesc(arr() As Integer, n As Integer)
    Dim i As Integer
    Dim j As Integer
    For i = 0 To n - 2
        For j = 0 To n - 2 - i
            If arr(j) < arr(j + 1) Then
                SwapInt(arr, j, j + 1)
            End If
        Next
    Next
End Sub

Function SumArr(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim s As Integer
    For i = 0 To n - 1
        s = s + arr(i)
    Next
    Return s
End Function

Function AverageArr(arr() As Integer, n As Integer) As Double
    If n <= 0 Then Return 0.0
    Return SumArr(arr, n) / n
End Function

Function MinArr(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim m As Integer
    If n <= 0 Then Return 0
    m = arr(0)
    For i = 1 To n - 1
        If arr(i) < m Then m = arr(i)
    Next
    Return m
End Function

Function MaxArr(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim m As Integer
    If n <= 0 Then Return 0
    m = arr(0)
    For i = 1 To n - 1
        If arr(i) > m Then m = arr(i)
    Next
    Return m
End Function

Sub ReverseArr(arr() As Integer, n As Integer)
    Dim i As Integer
    Dim j As Integer
    i = 0
    j = n - 1
    While i < j
        SwapInt(arr, i, j)
        i = i + 1
        j = j - 1
    End While
End Sub

Function IndexOfInt(arr() As Integer, n As Integer, v As Integer) As Integer
    Dim i As Integer
    For i = 0 To n - 1
        if arr(i) = v Then Return i
    Next
    Return -1
End Function

Sub FillArray(arr() As Integer, n As Integer, v As Integer)
    Dim i As Integer
    For i = 0 To n - 1
        arr(i) = v
    Next
End Sub

Sub RangeArray(arr() As Integer, n As Integer)
    Dim i As Integer
    For i = 0 To n - 1
        arr(i) = i + 1
    Next
End Sub

Sub IotaArray(arr() As Integer, n As Integer, startVal As Integer, stepVal As Integer)
    Dim i As Integer
    For i = 0 To n - 1
        arr(i) = startVal + i * stepVal
    Next
End Sub

Function ReduceSum(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim s As Integer
    For i = 0 To n - 1
        s = s + arr(i)
    Next
    Return s
End Function

Function ReduceProduct(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim p As Integer
    p = 1
    For i = 0 To n - 1
        p = p * arr(i)
    Next
    Return p
End Function

Function ReduceMax(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim m As Integer
    If n <= 0 Then Return 0
    m = arr(0)
    For i = 1 To n - 1
        If arr(i) > m Then m = arr(i)
    Next
    Return m
End Function

Function ReduceMin(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim m As Integer
    If n <= 0 Then Return 0
    m = arr(0)
    For i = 1 To n - 1
        If arr(i) < m Then m = arr(i)
    Next
    Return m
End Function

Sub ScanSum(arr() As Integer, n As Integer)
    Dim i As Integer
    Dim s As Integer
    For i = 0 To n - 1
        s = s + arr(i)
        arr(i) = s
    Next
End Sub

Function FilterEven(arr() As Integer, n As Integer) As Integer
    Dim i As Integer
    Dim count As Integer
    count = 0
    For i = 0 To n - 1
        If arr(i) Mod 2 = 0 Then
            arr(count) = arr(i)
            count = count + 1
        End If
    Next
    Return count
End Function

Function SelectGreater(arr() As Integer, n As Integer, threshold As Integer) As Integer
    Dim i As Integer
    Dim count As Integer
    count = 0
    For i = 0 To n - 1
        If arr(i) > threshold Then
            arr(count) = arr(i)
            count = count + 1
        End If
    Next
    Return count
End Function