; ModuleID = 'test.ll'
source_filename = "test.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private constant [4 x i8] c"%d \00", align 1
@.str.1 = private constant [20 x i8] c"bubble_sort result:\00", align 1
@.str.2 = private constant [18 x i8] c"time used: %dms.\0A\00", align 1
@.str.3 = private constant [19 x i8] c"quick_sort result:\00", align 1
@.str.4 = private constant [18 x i8] c"time used: %dms.\0A\00", align 1
@.str.5 = private constant [18 x i8] c"heap_sort result:\00", align 1
@.str.6 = private constant [18 x i8] c"time used: %dms.\0A\00", align 1

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

define void @print_array(i32* %0, i32* %1) {
print_array:
  %begin = alloca i32*
  store i32* %0, i32** %begin
  %end = alloca i32*
  store i32* %1, i32** %end
  br label %for.cond

for.begin:                                        ; preds = %for.cond
  %2 = load i32*, i32** %begin
  %3 = load i32, i32* %2
  %ret = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32 %3)
  br label %for.nxt

for.nxt:                                          ; preds = %for.begin
  %4 = load i32*, i32** %begin
  %5 = getelementptr i32, i32* %4, i32 1
  store i32* %5, i32** %begin
  br label %for.cond

for.cond:                                         ; preds = %for.nxt, %print_array
  %6 = load i32*, i32** %begin
  %7 = load i32*, i32** %end
  %8 = icmp ne i32* %6, %7
  br i1 %8, label %for.begin, label %for.end

for.end:                                          ; preds = %for.cond
  %ret1 = call i32 @putchar(i32 10)
  ret void
}

define void @swap(i32* %0, i32* %1) {
swap:
  %tmp = alloca i32
  %a = getelementptr i32, i32* %0, i64 0
  %b = getelementptr i32, i32* %1, i64 0
  %2 = load i32, i32* %a
  store i32 %2, i32* %tmp
  %3 = load i32, i32* %b
  store i32 %3, i32* %a
  %4 = load i32, i32* %tmp
  store i32 %4, i32* %b
  ret void
}

define void @qsort(i32* %0, i32* %1) {
qsort:
  %pivot = alloca i32
  %p = alloca i32*
  %gt = alloca i32*
  %lt = alloca i32*
  %begin = alloca i32*
  store i32* %0, i32** %begin
  %end = alloca i32*
  store i32* %1, i32** %end
  %2 = load i32*, i32** %end
  %3 = load i32*, i32** %begin
  %4 = ptrtoint i32* %2 to i64
  %5 = ptrtoint i32* %3 to i64
  %6 = sub i64 %4, %5
  %7 = sdiv exact i64 %6, ptrtoint (i32* getelementptr (i32, i32* null, i32 1) to i64)
  %8 = icmp sle i64 %7, 1
  br i1 %8, label %if.then, label %if.end

if.then:                                          ; preds = %qsort
  ret void

never:                                            ; No predecessors!
  br label %if.end

if.end:                                           ; preds = %never, %qsort
  %9 = load i32*, i32** %begin
  store i32* %9, i32** %lt
  %10 = load i32*, i32** %end
  store i32* %10, i32** %gt
  %11 = load i32*, i32** %begin
  store i32* %11, i32** %p
  %12 = load i32*, i32** %begin
  %13 = load i32, i32* %12
  store i32 %13, i32* %pivot
  br label %while.cond

while.begin:                                      ; preds = %while.cond
  %14 = load i32*, i32** %p
  %15 = load i32, i32* %14
  %16 = load i32, i32* %pivot
  %17 = icmp eq i32 %15, %16
  br i1 %17, label %if.then1, label %if.end3

if.then1:                                         ; preds = %while.begin
  %18 = load i32*, i32** %p
  %19 = getelementptr i32, i32* %18, i32 1
  store i32* %19, i32** %p
  br label %while.cond

never2:                                           ; No predecessors!
  br label %if.end3

if.end3:                                          ; preds = %never2, %while.begin
  %20 = load i32*, i32** %p
  %21 = load i32, i32* %20
  %22 = load i32, i32* %pivot
  %23 = icmp slt i32 %21, %22
  br i1 %23, label %if.then4, label %if.else

if.then4:                                         ; preds = %if.end3
  %24 = load i32*, i32** %lt
  %25 = load i32*, i32** %p
  call void @swap(i32* %24, i32* %25)
  %26 = load i32*, i32** %lt
  %27 = getelementptr i32, i32* %26, i32 1
  store i32* %27, i32** %lt
  %28 = load i32*, i32** %p
  %29 = getelementptr i32, i32* %28, i32 1
  store i32* %29, i32** %p
  br label %if.end5

if.else:                                          ; preds = %if.end3
  %30 = load i32*, i32** %gt
  %31 = getelementptr i32, i32* %30, i32 -1
  store i32* %31, i32** %gt
  %32 = load i32*, i32** %gt
  %33 = load i32*, i32** %p
  call void @swap(i32* %32, i32* %33)
  br label %if.end5

if.end5:                                          ; preds = %if.else, %if.then4
  br label %while.cond

while.cond:                                       ; preds = %if.end5, %if.then1, %if.end
  %34 = load i32*, i32** %p
  %35 = load i32*, i32** %gt
  %36 = icmp ult i32* %34, %35
  br i1 %36, label %while.begin, label %while.end

while.end:                                        ; preds = %while.cond
  %37 = load i32*, i32** %begin
  %38 = load i32*, i32** %lt
  call void @qsort(i32* %37, i32* %38)
  %39 = load i32*, i32** %gt
  %40 = load i32*, i32** %end
  call void @qsort(i32* %39, i32* %40)
  ret void
}

define void @heapify(i32* %0, i32 %1, i32 %2) {
heapify:
  %largest = alloca i32
  %right = alloca i32
  %left = alloca i32
  %arr = alloca i32*
  store i32* %0, i32** %arr
  %n = alloca i32
  store i32 %1, i32* %n
  %i = alloca i32
  store i32 %2, i32* %i
  %3 = load i32, i32* %i
  %4 = mul i32 2, %3
  store i32 %4, i32* %left
  %5 = load i32, i32* %i
  %6 = mul i32 2, %5
  %7 = add i32 %6, 1
  store i32 %7, i32* %right
  %8 = load i32, i32* %i
  store i32 %8, i32* %largest
  %9 = load i32, i32* %left
  %10 = load i32, i32* %n
  %11 = icmp sle i32 %9, %10
  %12 = load i32*, i32** %arr
  %13 = load i32, i32* %left
  %sext = sext i32 %13 to i64
  %14 = getelementptr i32, i32* %12, i64 %sext
  %15 = load i32, i32* %14
  %16 = load i32*, i32** %arr
  %17 = load i32, i32* %largest
  %sext1 = sext i32 %17 to i64
  %18 = getelementptr i32, i32* %16, i64 %sext1
  %19 = load i32, i32* %18
  %20 = icmp sgt i32 %15, %19
  %21 = select i1 %11, i1 %20, i1 false
  br i1 %21, label %if.then, label %if.end

if.then:                                          ; preds = %heapify
  %22 = load i32, i32* %left
  store i32 %22, i32* %largest
  br label %if.end

if.end:                                           ; preds = %if.then, %heapify
  %23 = load i32, i32* %right
  %24 = load i32, i32* %n
  %25 = icmp sle i32 %23, %24
  %26 = load i32*, i32** %arr
  %27 = load i32, i32* %right
  %sext2 = sext i32 %27 to i64
  %28 = getelementptr i32, i32* %26, i64 %sext2
  %29 = load i32, i32* %28
  %30 = load i32*, i32** %arr
  %31 = load i32, i32* %largest
  %sext3 = sext i32 %31 to i64
  %32 = getelementptr i32, i32* %30, i64 %sext3
  %33 = load i32, i32* %32
  %34 = icmp sgt i32 %29, %33
  %35 = select i1 %25, i1 %34, i1 false
  br i1 %35, label %if.then4, label %if.end5

if.then4:                                         ; preds = %if.end
  %36 = load i32, i32* %right
  store i32 %36, i32* %largest
  br label %if.end5

if.end5:                                          ; preds = %if.then4, %if.end
  %37 = load i32, i32* %largest
  %38 = load i32, i32* %i
  %39 = icmp ne i32 %37, %38
  br i1 %39, label %if.then6, label %if.end9

if.then6:                                         ; preds = %if.end5
  %40 = load i32*, i32** %arr
  %41 = load i32, i32* %i
  %sext7 = sext i32 %41 to i64
  %42 = getelementptr i32, i32* %40, i64 %sext7
  %43 = load i32*, i32** %arr
  %44 = load i32, i32* %largest
  %sext8 = sext i32 %44 to i64
  %45 = getelementptr i32, i32* %43, i64 %sext8
  call void @swap(i32* %42, i32* %45)
  %46 = load i32*, i32** %arr
  %47 = load i32, i32* %n
  %48 = load i32, i32* %largest
  call void @heapify(i32* %46, i32 %47, i32 %48)
  br label %if.end9

if.end9:                                          ; preds = %if.then6, %if.end5
  ret void
}

define void @heap_sort(i32* %0, i32* %1) {
heap_sort:
  %i2 = alloca i32
  %i = alloca i32
  %base = alloca i32*
  %len = alloca i32
  %begin = alloca i32*
  store i32* %0, i32** %begin
  %end = alloca i32*
  store i32* %1, i32** %end
  %2 = load i32*, i32** %end
  %3 = load i32*, i32** %begin
  %4 = ptrtoint i32* %2 to i64
  %5 = ptrtoint i32* %3 to i64
  %6 = sub i64 %4, %5
  %7 = sdiv exact i64 %6, ptrtoint (i32* getelementptr (i32, i32* null, i32 1) to i64)
  %trunc = trunc i64 %7 to i32
  store i32 %trunc, i32* %len
  %8 = load i32*, i32** %begin
  %9 = getelementptr i32, i32* %8, i32 -1
  store i32* %9, i32** %base
  %10 = load i32, i32* %len
  %11 = add i32 %10, 1
  %12 = sdiv i32 %11, 2
  store i32 %12, i32* %i
  br label %for.cond

for.begin:                                        ; preds = %for.cond
  %13 = load i32*, i32** %base
  %14 = load i32, i32* %len
  %15 = load i32, i32* %i
  call void @heapify(i32* %13, i32 %14, i32 %15)
  br label %for.nxt

for.nxt:                                          ; preds = %for.begin
  %16 = load i32, i32* %i
  %17 = sub i32 %16, 1
  store i32 %17, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.nxt, %heap_sort
  %18 = load i32, i32* %i
  %19 = icmp sge i32 %18, 1
  br i1 %19, label %for.begin, label %for.end

for.end:                                          ; preds = %for.cond
  %20 = load i32, i32* %len
  store i32 %20, i32* %i2
  br label %for.cond4

for.begin1:                                       ; preds = %for.cond4
  %21 = load i32*, i32** %base
  %22 = getelementptr i32, i32* %21, i64 1
  %23 = load i32*, i32** %base
  %24 = load i32, i32* %i2
  %sext = sext i32 %24 to i64
  %25 = getelementptr i32, i32* %23, i64 %sext
  call void @swap(i32* %22, i32* %25)
  %26 = load i32*, i32** %base
  %27 = load i32, i32* %i2
  %28 = sub i32 %27, 1
  call void @heapify(i32* %26, i32 %28, i32 1)
  br label %for.nxt3

for.nxt3:                                         ; preds = %for.begin1
  %29 = load i32, i32* %i2
  %30 = sub i32 %29, 1
  store i32 %30, i32* %i2
  br label %for.cond4

for.cond4:                                        ; preds = %for.nxt3, %for.end
  %31 = load i32, i32* %i2
  %32 = icmp sge i32 %31, 1
  br i1 %32, label %for.begin1, label %for.end5

for.end5:                                         ; preds = %for.cond4
  ret void
}

define void @bubble_sort(i32* %0, i32* %1) {
bubble_sort:
  %j = alloca i32
  %i = alloca i32
  %len = alloca i64
  %begin = alloca i32*
  store i32* %0, i32** %begin
  %end = alloca i32*
  store i32* %1, i32** %end
  %2 = load i32*, i32** %end
  %3 = load i32*, i32** %begin
  %4 = ptrtoint i32* %2 to i64
  %5 = ptrtoint i32* %3 to i64
  %6 = sub i64 %4, %5
  %7 = sdiv exact i64 %6, ptrtoint (i32* getelementptr (i32, i32* null, i32 1) to i64)
  store i64 %7, i64* %len
  store i32 0, i32* %i
  br label %for.cond7

for.begin:                                        ; preds = %for.cond7
  store i32 1, i32* %j
  br label %for.cond

for.begin1:                                       ; preds = %for.cond
  %8 = load i32*, i32** %begin
  %9 = load i32, i32* %j
  %sext = sext i32 %9 to i64
  %10 = getelementptr i32, i32* %8, i64 %sext
  %11 = load i32, i32* %10
  %12 = load i32*, i32** %begin
  %13 = load i32, i32* %j
  %14 = sub i32 %13, 1
  %sext2 = sext i32 %14 to i64
  %15 = getelementptr i32, i32* %12, i64 %sext2
  %16 = load i32, i32* %15
  %17 = icmp slt i32 %11, %16
  br i1 %17, label %if.then, label %if.end

if.then:                                          ; preds = %for.begin1
  %18 = load i32*, i32** %begin
  %19 = load i32, i32* %j
  %sext3 = sext i32 %19 to i64
  %20 = getelementptr i32, i32* %18, i64 %sext3
  %21 = load i32*, i32** %begin
  %22 = load i32, i32* %j
  %23 = sub i32 %22, 1
  %sext4 = sext i32 %23 to i64
  %24 = getelementptr i32, i32* %21, i64 %sext4
  call void @swap(i32* %20, i32* %24)
  br label %if.end

if.end:                                           ; preds = %if.then, %for.begin1
  br label %for.nxt

for.nxt:                                          ; preds = %if.end
  %25 = load i32, i32* %j
  %26 = add i32 %25, 1
  store i32 %26, i32* %j
  br label %for.cond

for.cond:                                         ; preds = %for.nxt, %for.begin
  %27 = load i32, i32* %j
  %28 = load i32, i32* %i
  %29 = add i32 %27, %28
  %sext5 = sext i32 %29 to i64
  %30 = load i64, i64* %len
  %31 = icmp slt i64 %sext5, %30
  br i1 %31, label %for.begin1, label %for.end

for.end:                                          ; preds = %for.cond
  br label %for.nxt6

for.nxt6:                                         ; preds = %for.end
  %32 = load i32, i32* %i
  %33 = add i32 %32, 1
  store i32 %33, i32* %i
  br label %for.cond7

for.cond7:                                        ; preds = %for.nxt6, %bubble_sort
  %34 = load i32, i32* %i
  %sext8 = sext i32 %34 to i64
  %35 = load i64, i64* %len
  %36 = icmp slt i64 %sext8, %35
  br i1 %36, label %for.begin, label %for.end9

for.end9:                                         ; preds = %for.cond7
  ret void
}

define i32 @main() {
main:
  %i24 = alloca i32
  %i13 = alloca i32
  %ed = alloca i64
  %st = alloca i64
  %i = alloca i32
  %b = alloca i32*
  %a = alloca i32*
  %n = alloca i32
  %ret = call i32 @read()
  store i32 %ret, i32* %n
  %0 = load i32, i32* %n
  %1 = mul i32 4, %0
  %sext = sext i32 %1 to i64
  %ret1 = call i8* @malloc(i64 %sext)
  %ptr = bitcast i8* %ret1 to i32*
  store i32* %ptr, i32** %a
  %2 = load i32, i32* %n
  %3 = mul i32 4, %2
  %sext2 = sext i32 %3 to i64
  %ret3 = call i8* @malloc(i64 %sext2)
  %ptr4 = bitcast i8* %ret3 to i32*
  store i32* %ptr4, i32** %b
  store i32 0, i32* %i
  br label %for.cond

for.begin:                                        ; preds = %for.cond
  %4 = load i32*, i32** %b
  %5 = load i32, i32* %i
  %sext5 = sext i32 %5 to i64
  %6 = getelementptr i32, i32* %4, i64 %sext5
  %7 = load i32*, i32** %a
  %8 = load i32, i32* %i
  %sext6 = sext i32 %8 to i64
  %9 = getelementptr i32, i32* %7, i64 %sext6
  %ret7 = call i32 @read()
  store i32 %ret7, i32* %9
  %10 = load i32, i32* %9
  store i32 %10, i32* %6
  br label %for.nxt

for.nxt:                                          ; preds = %for.begin
  %11 = load i32, i32* %i
  %12 = add i32 %11, 1
  store i32 %12, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.nxt, %main
  %13 = load i32, i32* %i
  %14 = load i32, i32* %n
  %15 = icmp slt i32 %13, %14
  br i1 %15, label %for.begin, label %for.end

for.end:                                          ; preds = %for.cond
  %ret8 = call i64 @clock()
  store i64 %ret8, i64* %st
  %16 = load i32*, i32** %b
  %17 = load i32*, i32** %b
  %18 = load i32, i32* %n
  %19 = getelementptr i32, i32* %17, i32 %18
  call void @bubble_sort(i32* %16, i32* %19)
  %ret9 = call i64 @clock()
  store i64 %ret9, i64* %ed
  %ret10 = call i32 @puts(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i32 0, i32 0))
  %20 = load i32*, i32** %b
  %21 = load i32*, i32** %b
  %22 = load i32, i32* %n
  %23 = getelementptr i32, i32* %21, i32 %22
  call void @print_array(i32* %20, i32* %23)
  %24 = load i64, i64* %ed
  %25 = load i64, i64* %st
  %26 = sub i64 %24, %25
  %ret11 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @.str.2, i32 0, i32 0), i64 %26)
  store i32 0, i32* %i13
  br label %for.cond17

for.begin12:                                      ; preds = %for.cond17
  %27 = load i32*, i32** %b
  %28 = load i32, i32* %i13
  %sext14 = sext i32 %28 to i64
  %29 = getelementptr i32, i32* %27, i64 %sext14
  %30 = load i32*, i32** %a
  %31 = load i32, i32* %i13
  %sext15 = sext i32 %31 to i64
  %32 = getelementptr i32, i32* %30, i64 %sext15
  %33 = load i32, i32* %32
  store i32 %33, i32* %29
  br label %for.nxt16

for.nxt16:                                        ; preds = %for.begin12
  %34 = load i32, i32* %i13
  %35 = add i32 %34, 1
  store i32 %35, i32* %i13
  br label %for.cond17

for.cond17:                                       ; preds = %for.nxt16, %for.end
  %36 = load i32, i32* %i13
  %37 = load i32, i32* %n
  %38 = icmp slt i32 %36, %37
  br i1 %38, label %for.begin12, label %for.end18

for.end18:                                        ; preds = %for.cond17
  %ret19 = call i64 @clock()
  store i64 %ret19, i64* %st
  %39 = load i32*, i32** %b
  %40 = load i32*, i32** %b
  %41 = load i32, i32* %n
  %42 = getelementptr i32, i32* %40, i32 %41
  call void @qsort(i32* %39, i32* %42)
  %ret20 = call i64 @clock()
  store i64 %ret20, i64* %ed
  %ret21 = call i32 @puts(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @.str.3, i32 0, i32 0))
  %43 = load i32*, i32** %b
  %44 = load i32*, i32** %b
  %45 = load i32, i32* %n
  %46 = getelementptr i32, i32* %44, i32 %45
  call void @print_array(i32* %43, i32* %46)
  %47 = load i64, i64* %ed
  %48 = load i64, i64* %st
  %49 = sub i64 %47, %48
  %ret22 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @.str.4, i32 0, i32 0), i64 %49)
  store i32 0, i32* %i24
  br label %for.cond28

for.begin23:                                      ; preds = %for.cond28
  %50 = load i32*, i32** %b
  %51 = load i32, i32* %i24
  %sext25 = sext i32 %51 to i64
  %52 = getelementptr i32, i32* %50, i64 %sext25
  %53 = load i32*, i32** %a
  %54 = load i32, i32* %i24
  %sext26 = sext i32 %54 to i64
  %55 = getelementptr i32, i32* %53, i64 %sext26
  %56 = load i32, i32* %55
  store i32 %56, i32* %52
  br label %for.nxt27

for.nxt27:                                        ; preds = %for.begin23
  %57 = load i32, i32* %i24
  %58 = add i32 %57, 1
  store i32 %58, i32* %i24
  br label %for.cond28

for.cond28:                                       ; preds = %for.nxt27, %for.end18
  %59 = load i32, i32* %i24
  %60 = load i32, i32* %n
  %61 = icmp slt i32 %59, %60
  br i1 %61, label %for.begin23, label %for.end29

for.end29:                                        ; preds = %for.cond28
  %ret30 = call i64 @clock()
  store i64 %ret30, i64* %st
  %62 = load i32*, i32** %b
  %63 = load i32*, i32** %b
  %64 = load i32, i32* %n
  %65 = getelementptr i32, i32* %63, i32 %64
  call void @heap_sort(i32* %62, i32* %65)
  %ret31 = call i64 @clock()
  store i64 %ret31, i64* %ed
  %ret32 = call i32 @puts(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @.str.5, i32 0, i32 0))
  %66 = load i32*, i32** %b
  %67 = load i32*, i32** %b
  %68 = load i32, i32* %n
  %69 = getelementptr i32, i32* %67, i32 %68
  call void @print_array(i32* %66, i32* %69)
  %70 = load i64, i64* %ed
  %71 = load i64, i64* %st
  %72 = sub i64 %70, %71
  %ret33 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @.str.6, i32 0, i32 0), i64 %72)
  %73 = load i32*, i32** %a
  %ptr34 = bitcast i32* %73 to i8*
  %ret35 = call i32 @free(i8* %ptr34)
  %74 = load i32*, i32** %b
  %ptr36 = bitcast i32* %74 to i8*
  %ret37 = call i32 @free(i8* %ptr36)
  ret i32 0
}
