' BasicForge standard library: environment helpers.
' Include inside a Module: Include "environment.bf"

Function GetEnvVar(name As String) As String
    Return GetEnv(name)
End Function

Sub SetEnvVar(name As String, value As String)
    SetEnv(name, value)
End Sub

Function HasEnv(name As String) As Boolean
    Return Len(GetEnv(name)) > 0
End Function