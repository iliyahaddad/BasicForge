# BasicForge Standard Library

Pure BasicForge modules. Include them inside a `Module` (they contain only
`Sub`/`Function` declarations, not their own `Module`):

```vb
Module Main
    Include "math.bf"
    Include "string.bf"
    Include "array.bf"
    Include "file.bf"
    Include "date.bf"
    Include "console.bf"
    Include "json.bf"
    Include "csv.bf"
    Include "validation.bf"
    Include "random.bf"

    Sub Main()
        Print(Clamp(15, 0, 10))   ' 10
        Print(IsPrime(97))        ' True
        Print(ToUpper("hello"))   ' HELLO
    End Sub
End Module
```

Compile with the library search path:

```powershell
bfc.exe -I libraries/stdlib examples\stdlib_demo.bf -o demo.exe
```

`bfc.exe` also searches `libraries/` and `libraries/stdlib/` relative to the
executable automatically.

## Modules

| Module | Contents |
|---|---|
| `math.bf` | Clamp, MinI, MaxI, AbsI, IsPrime, Gcd, Lcm, Factorial, DegToRad, RadToDeg, Sign |
| `math_ex.bf` | Round, Floor, Ceil, ClampD, Lerp, MaxD, MinD |
| `string.bf` | ToUpper, ToLower, Reverse, Contains, StartsWith, EndsWith, PadLeft, PadRight, Repeat, IsBlank, Truncate |
| `strings_ex.bf` | CountOccurrences, Insert, RemoveAt, ReplaceAll, SplitCount, Join |
| `array.bf` | SwapInt, SortAsc, SortDesc, SumArr, AverageArr, MinArr, MaxArr, ReverseArr, IndexOfInt, FillArray, RangeArray |
| `bitwise.bf` | TestBit, SetBit, ClearBit, ToggleBit, Shl, Shr, IsPowerOfTwo |
| `file.bf` | ReadTextFile, WriteTextFile, AppendTextFile, FileExistsP, FileSize |
| `date.bf` | IsLeapYear, DaysInMonth, IsValidDate, NextYear, IsWeekend |
| `console.bf` | ClearScreen, SetWindowTitle, PrintLine, ReadInput, WaitMs, Sound |
| `json.bf` | JsonEscape, JsonString, JsonNumber, JsonBool, JsonObject |
| `csv.bf` | CsvEscape, CsvRow, CsvRow3 |
| `validation.bf` | IsNumber, IsIntegerText, IsEmpty, IsEmail, IsUrl |
| `random.bf` | RandInt, Chance, BoolRand |