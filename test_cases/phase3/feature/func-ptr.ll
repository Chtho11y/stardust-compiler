; ModuleID = 'test.ll'
source_filename = "test.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private constant [20 x i8] c"invalid op, exited.\00", align 1

declare void @write(i32)

declare void @exit(i32)

declare i32 @read()

declare i32 @puts(i8*)

declare i32 @putchar(i32)

declare i32 @getchar()

declare i8* @malloc(i64)

declare i32 @free(i8*)

declare i64 @clock()

declare i32 @printf(i8*, ...)

declare i32 @scanf(i8*, ...)

define i32 @add(i32 %0, i32 %1) {
add:
  %x = alloca i32
  store i32 %0, i32* %x
  %y = alloca i32
  store i32 %1, i32* %y
  %2 = load i32, i32* %x
  %3 = load i32, i32* %y
  %4 = add i32 %2, %3
  ret i32 %4
}

define i32 @sub(i32 %0, i32 %1) {
sub:
  %x = alloca i32
  store i32 %0, i32* %x
  %y = alloca i32
  store i32 %1, i32* %y
  %2 = load i32, i32* %x
  %3 = load i32, i32* %y
  %4 = sub i32 %2, %3
  ret i32 %4
}

define i32 @div(i32 %0, i32 %1) {
div:
  %x = alloca i32
  store i32 %0, i32* %x
  %y = alloca i32
  store i32 %1, i32* %y
  %2 = load i32, i32* %x
  %3 = load i32, i32* %y
  %4 = sdiv i32 %2, %3
  ret i32 %4
}

define i32 @mul(i32 %0, i32 %1) {
mul:
  %x = alloca i32
  store i32 %0, i32* %x
  %y = alloca i32
  store i32 %1, i32* %y
  %2 = load i32, i32* %x
  %3 = load i32, i32* %y
  %4 = mul i32 %2, %3
  ret i32 %4
}

define i32 @mod(i32 %0, i32 %1) {
mod:
  %x = alloca i32
  store i32 %0, i32* %x
  %y = alloca i32
  store i32 %1, i32* %y
  %2 = load i32, i32* %x
  %3 = load i32, i32* %y
  %4 = srem i32 %2, %3
  ret i32 %4
}

define i32 @err(i32 %0, i32 %1) {
err:
  %_x = alloca i32
  store i32 %0, i32* %_x
  %_y = alloca i32
  store i32 %1, i32* %_y
  %ret = call i32 @puts(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str, i32 0, i32 0))
  call void @exit(i32 0)
  ret i32 0
}

define i32 @main() {
main:
  %b = alloca i32
  %ch = alloca i32
  %a = alloca i32
  %i = alloca i32
  %op = alloca [128 x i32 (i32, i32)*]
  store i32 0, i32* %i
  br label %for.cond

for.begin:                                        ; preds = %for.cond
  %0 = load i32, i32* %i
  %sext = sext i32 %0 to i64
  %1 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 %sext
  store i32 (i32, i32)* @err, i32 (i32, i32)** %1
  br label %for.nxt

for.nxt:                                          ; preds = %for.begin
  %2 = load i32, i32* %i
  %3 = add i32 %2, 1
  store i32 %3, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.nxt, %main
  %4 = load i32, i32* %i
  %5 = icmp slt i32 %4, 128
  br i1 %5, label %for.begin, label %for.end

for.end:                                          ; preds = %for.cond
  %6 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 43
  store i32 (i32, i32)* @add, i32 (i32, i32)** %6
  %7 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 45
  store i32 (i32, i32)* @sub, i32 (i32, i32)** %7
  %8 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 42
  store i32 (i32, i32)* @mul, i32 (i32, i32)** %8
  %9 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 47
  store i32 (i32, i32)* @div, i32 (i32, i32)** %9
  %10 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 37
  store i32 (i32, i32)* @mod, i32 (i32, i32)** %10
  br label %while.cond7

while.begin:                                      ; preds = %while.cond7
  %ret = call i32 @read()
  store i32 %ret, i32* %a
  %ret1 = call i32 @getchar()
  store i32 %ret1, i32* %ch
  br label %while.cond

while.begin2:                                     ; preds = %while.cond
  %ret3 = call i32 @getchar()
  store i32 %ret3, i32* %ch
  br label %while.cond

while.cond:                                       ; preds = %while.begin2, %while.begin
  %11 = load i32, i32* %ch
  %12 = icmp eq i32 %11, 32
  %13 = load i32, i32* %ch
  %14 = icmp eq i32 %13, 10
  %15 = select i1 %12, i1 true, i1 %14
  br i1 %15, label %while.begin2, label %while.end

while.end:                                        ; preds = %while.cond
  %ret4 = call i32 @read()
  store i32 %ret4, i32* %b
  %16 = load i32, i32* %ch
  %sext5 = sext i32 %16 to i64
  %17 = getelementptr [128 x i32 (i32, i32)*], [128 x i32 (i32, i32)*]* %op, i64 0, i64 %sext5
  %18 = load i32 (i32, i32)*, i32 (i32, i32)** %17
  %19 = load i32, i32* %a
  %20 = load i32, i32* %b
  %ret6 = call i32 %18(i32 %19, i32 %20)
  call void @write(i32 %ret6)
  br label %while.cond7

while.cond7:                                      ; preds = %while.end, %for.end
  br i1 true, label %while.begin, label %while.end8

while.end8:                                       ; preds = %while.cond7
  ret i32 0
}
