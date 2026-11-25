; BasicForge-generated LLVM IR (from BasicForge IR)
target datalayout = "e-m:w-p:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@str.0 = private constant [12 x i8] c"Hello World\00"

declare void @bf_print_string(i8* nocapture)
declare void @bf_print_int(i64)
declare void @bf_print_double(double)
declare void @bf_print_bool(i1)
declare i8* @bf_str_concat(i8*, i8*)

define void @Main() {
  call void @bf_print_string(i8* @str.0)
  ret void
}

