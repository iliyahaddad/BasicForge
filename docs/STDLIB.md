# BasicForge Standard Library

Include modules inside a `Module` (they contain only `Sub`/`Function`):

```vb
Module Main
    Include "math.bf"
    Include "string.bf"
    Sub Main()
        Print(Clamp(15, 0, 10))
    End Sub
End Module
```

Compile with `-I libraries/stdlib` (or rely on the automatic search next to
`bfc.exe`).

## math.bf

| Function | Description |
|---|---|
| `Clamp(v, lo, hi)` | Clamp to range |
| `MaxI(a, b)` / `MinI(a, b)` | Maximum/minimum |
| `AbsI(v)` | Absolute value |
| `IsPrime(n)` | Primality test |
| `Gcd(a, b)` / `Lcm(a, b)` | Greatest common divisor / least common multiple |
| `Factorial(n)` | Factorial |
| `DegToRad(deg)` / `RadToDeg(rad)` | Angle conversion |
| `Sign(v)` | -1 / 0 / 1 |

## string.bf

| Function | Description |
|---|---|
| `ToUpper(s)` / `ToLower(s)` | Case conversion |
| `Reverse(s)` | Reverse text |
| `Contains(s, part)` | Substring check |
| `StartsWith(s, prefix)` / `EndsWith(s, suffix)` | Prefix/suffix |
| `PadLeft(s, w, ch)` / `PadRight(s, w, ch)` | Padding |
| `Repeat(s, n)` | Repeat text |
| `IsBlank(s)` | Whitespace check |
| `Truncate(s, n)` | Truncate |

## array.bf

| Function | Description |
|---|---|
| `SwapInt(arr, i, j)` | Swap two elements |
| `SortAsc(arr, n)` / `SortDesc(arr, n)` | Bubble sort |
| `SumArr(arr, n)` | Sum |
| `AverageArr(arr, n)` | Average |
| `MinArr(arr, n)` / `MaxArr(arr, n)` | Min/max |
| `ReverseArr(arr, n)` | Reverse |
| `IndexOfInt(arr, n, v)` | Index of value |

## file.bf

| Function | Description |
|---|---|
| `ReadTextFile(path)` | Read all text |
| `WriteTextFile(path, text)` | Write text |
| `AppendTextFile(path, text)` | Append text |
| `FileExistsP(path)` | Existence check |
| `FileSize(path)` | Size in bytes (read-based) |

## date.bf

| Function | Description |
|---|---|
| `IsLeapYear(y)` | Leap year |
| `DaysInMonth(y, m)` | Days in month |
| `IsValidDate(y, m, d)` | Validate date |
| `NextYear(y)` | Next year |
| `IsWeekend(dow)` | Weekend check |

## console.bf

| Function | Description |
|---|---|
| `ClearScreen()` | Clear console |
| `SetWindowTitle(t)` | Set title |
| `PrintLine(t)` | Print line |
| `ReadInput()` | Read line |
| `WaitMs(ms)` | Sleep |
| `Sound(f, ms)` | Beep |

## json.bf

| Function | Description |
|---|---|
| `JsonEscape(s)` | Escape text |
| `JsonString(s)` | Quote/escape |
| `JsonNumber(n)` | Number to text |
| `JsonBool(b)` | true/false |
| `JsonObject(k, v)` | `"k": "v"` |

## csv.bf

| Function | Description |
|---|---|
| `CsvEscape(s)` | Quote field |
| `CsvRow(a, b)` | Two columns |
| `CsvRow3(a, b, c)` | Three columns |

## validation.bf

| Function | Description |
|---|---|
| `IsNumber(s)` | Digits only |
| `IsIntegerText(s)` | Alias of `IsNumber` |
| `IsEmpty(s)` | Blank check |
| `IsEmail(s)` | Contains `@` and `.` |
| `IsUrl(s)` | Starts with http(s):// |

## random.bf

| Function | Description |
|---|---|
| `RandInt(lo, hi)` | Random integer |
| `Chance(p)` | Probability |
| `BoolRand()` | Random boolean |

## bitwise.bf

| Function | Description |
|---|---|
| `BitValue(bit)` | `2^bit` |
| `TestBit(v, bit)` | Test a bit |
| `SetBit(v, bit)` | Set a bit |
| `ClearBit(v, bit)` | Clear a bit |
| `ToggleBit(v, bit)` | Toggle a bit |
| `Shl(v, n)` / `Shr(v, n)` | Shift left/right |
| `IsPowerOfTwo(v)` | Power-of-two check |

## math_ex.bf

| Function | Description |
|---|---|
| `Round(v, digits)` | Round to N digits |
| `Floor(v)` / `Ceil(v)` | Floor/ceiling |
| `ClampD(v, lo, hi)` | Clamp a Double |
| `Lerp(a, b, t)` | Linear interpolation |
| `MaxD(a, b)` / `MinD(a, b)` | Double min/max |

## strings_ex.bf

| Function | Description |
|---|---|
| `CountOccurrences(s, part)` | Count substring occurrences |
| `Insert(s, at, part)` | Insert text |
| `RemoveAt(s, at, count)` | Remove text |
| `ReplaceAll(s, old, new)` | Replace all |
| `SplitCount(s, sep)` | Number of fields |
| `Join(a, b, sep)` | Join two strings |

## SDKs

`sdks/win32/win32.bf` wraps `MessageBox`, `ShellExecute`, `Sleep`, `Beep`,
clipboard and window title. `sdks/win32/environment.bf` wraps environment
variables. `sdks/json/json.bf` and `sdks/csv/csv.bf` re-export the stdlib
modules.