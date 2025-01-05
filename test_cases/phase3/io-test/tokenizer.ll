; ModuleID = 'test.ll'
source_filename = "test.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%Token.43 = type { [20 x i8], i32, i32, i1 }
%trait.proto = type { i8*, i8* }

@token_name = global [10 x i8*] undef
@.str = private constant [7 x i8] c"NUMBER\00", align 1
@.str.1 = private constant [7 x i8] c"STRING\00", align 1
@.str.2 = private constant [5 x i8] c"CHAR\00", align 1
@.str.3 = private constant [8 x i8] c"KEYWORD\00", align 1
@.str.4 = private constant [3 x i8] c"ID\00", align 1
@.str.5 = private constant [7 x i8] c"SYMBOL\00", align 1
@.str.6 = private constant [8 x i8] c"COMMENT\00", align 1
@.str.7 = private constant [1 x i8] zeroinitializer, align 1
@symbols = global [100 x i8*] undef
@.str.8 = private constant [2 x i8] c"+\00", align 1
@.str.9 = private constant [2 x i8] c"-\00", align 1
@.str.10 = private constant [2 x i8] c"*\00", align 1
@.str.11 = private constant [2 x i8] c"/\00", align 1
@.str.12 = private constant [3 x i8] c"+=\00", align 1
@.str.13 = private constant [3 x i8] c"-=\00", align 1
@.str.14 = private constant [3 x i8] c"*=\00", align 1
@.str.15 = private constant [3 x i8] c"/=\00", align 1
@.str.16 = private constant [2 x i8] c"=\00", align 1
@.str.17 = private constant [2 x i8] c"&\00", align 1
@.str.18 = private constant [2 x i8] c"[\00", align 1
@.str.19 = private constant [2 x i8] c"]\00", align 1
@.str.20 = private constant [2 x i8] c"(\00", align 1
@.str.21 = private constant [2 x i8] c")\00", align 1
@.str.22 = private constant [2 x i8] c"{\00", align 1
@.str.23 = private constant [2 x i8] c"}\00", align 1
@.str.24 = private constant [3 x i8] c"->\00", align 1
@.str.25 = private constant [3 x i8] c"==\00", align 1
@.str.26 = private constant [3 x i8] c"!=\00", align 1
@.str.27 = private constant [3 x i8] c"<=\00", align 1
@.str.28 = private constant [3 x i8] c">=\00", align 1
@.str.29 = private constant [2 x i8] c"<\00", align 1
@.str.30 = private constant [2 x i8] c">\00", align 1
@.str.31 = private constant [2 x i8] c":\00", align 1
@.str.32 = private constant [2 x i8] c",\00", align 1
@.str.33 = private constant [2 x i8] c";\00", align 1
@.str.34 = private constant [2 x i8] c"^\00", align 1
@.str.35 = private constant [2 x i8] c"|\00", align 1
@.str.36 = private constant [3 x i8] c"||\00", align 1
@.str.37 = private constant [3 x i8] c"&&\00", align 1
@.str.38 = private constant [2 x i8] c".\00", align 1
@.str.39 = private constant [2 x i8] c"!\00", align 1
@.str.40 = private constant [1 x i8] zeroinitializer, align 1
@keywords = global [20 x i8*] undef
@.str.41 = private constant [5 x i8] c"func\00", align 1
@.str.42 = private constant [4 x i8] c"let\00", align 1
@.str.43 = private constant [7 x i8] c"struct\00", align 1
@.str.44 = private constant [4 x i8] c"for\00", align 1
@.str.45 = private constant [6 x i8] c"while\00", align 1
@.str.46 = private constant [3 x i8] c"if\00", align 1
@.str.47 = private constant [5 x i8] c"else\00", align 1
@.str.48 = private constant [7 x i8] c"return\00", align 1
@.str.49 = private constant [5 x i8] c"impl\00", align 1
@.str.50 = private constant [1 x i8] zeroinitializer, align 1
@NUMBER = global i32 undef
@STRING = global i32 undef
@CHAR = global i32 undef
@KEYWORD = global i32 undef
@ID = global i32 undef
@SYMBOL = global i32 undef
@COMMENT = global i32 undef
@.str.51 = private constant [6 x i8] c"[%s] \00", align 1
@.str.52 = private constant [1 x i8] zeroinitializer, align 1
@.str.53 = private constant [3 x i8] c"//\00", align 1

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

define internal [10 x i8*] @token_name.init() {
entry:
  %0 = alloca [10 x i8*]
  %1 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 0
  store i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str, i32 0, i32 0), i8** %1
  %2 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 1
  store i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.1, i32 0, i32 0), i8** %2
  %3 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 2
  store i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.2, i32 0, i32 0), i8** %3
  %4 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 3
  store i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.3, i32 0, i32 0), i8** %4
  %5 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 4
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.4, i32 0, i32 0), i8** %5
  %6 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 5
  store i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.5, i32 0, i32 0), i8** %6
  %7 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 6
  store i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.6, i32 0, i32 0), i8** %7
  %8 = getelementptr [10 x i8*], [10 x i8*]* %0, i32 0, i32 7
  store i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.7, i32 0, i32 0), i8** %8
  %9 = load [10 x i8*], [10 x i8*]* %0
  ret [10 x i8*] %9
}

define void @global.var.init() {
entry:
  %0 = call [10 x i8*] @token_name.init()
  store [10 x i8*] %0, [10 x i8*]* @token_name
  %1 = call [100 x i8*] @symbols.init()
  store [100 x i8*] %1, [100 x i8*]* @symbols
  %2 = call [20 x i8*] @keywords.init()
  store [20 x i8*] %2, [20 x i8*]* @keywords
  %3 = call i32 @NUMBER.init()
  store i32 %3, i32* @NUMBER
  %4 = call i32 @STRING.init()
  store i32 %4, i32* @STRING
  %5 = call i32 @CHAR.init()
  store i32 %5, i32* @CHAR
  %6 = call i32 @KEYWORD.init()
  store i32 %6, i32* @KEYWORD
  %7 = call i32 @ID.init()
  store i32 %7, i32* @ID
  %8 = call i32 @SYMBOL.init()
  store i32 %8, i32* @SYMBOL
  %9 = call i32 @COMMENT.init()
  store i32 %9, i32* @COMMENT
  ret void
}

define internal [100 x i8*] @symbols.init() {
entry:
  %0 = alloca [100 x i8*]
  %1 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 0
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.8, i32 0, i32 0), i8** %1
  %2 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 1
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.9, i32 0, i32 0), i8** %2
  %3 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 2
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.10, i32 0, i32 0), i8** %3
  %4 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 3
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.11, i32 0, i32 0), i8** %4
  %5 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 4
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.12, i32 0, i32 0), i8** %5
  %6 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 5
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.13, i32 0, i32 0), i8** %6
  %7 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 6
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.14, i32 0, i32 0), i8** %7
  %8 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 7
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.15, i32 0, i32 0), i8** %8
  %9 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 8
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.16, i32 0, i32 0), i8** %9
  %10 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 9
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.17, i32 0, i32 0), i8** %10
  %11 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 10
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.18, i32 0, i32 0), i8** %11
  %12 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 11
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.19, i32 0, i32 0), i8** %12
  %13 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 12
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.20, i32 0, i32 0), i8** %13
  %14 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 13
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.21, i32 0, i32 0), i8** %14
  %15 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 14
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.22, i32 0, i32 0), i8** %15
  %16 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 15
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.23, i32 0, i32 0), i8** %16
  %17 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 16
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.24, i32 0, i32 0), i8** %17
  %18 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 17
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.25, i32 0, i32 0), i8** %18
  %19 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 18
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.26, i32 0, i32 0), i8** %19
  %20 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 19
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.27, i32 0, i32 0), i8** %20
  %21 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 20
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.28, i32 0, i32 0), i8** %21
  %22 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 21
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.29, i32 0, i32 0), i8** %22
  %23 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 22
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.30, i32 0, i32 0), i8** %23
  %24 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 23
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.31, i32 0, i32 0), i8** %24
  %25 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 24
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.32, i32 0, i32 0), i8** %25
  %26 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 25
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.33, i32 0, i32 0), i8** %26
  %27 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 26
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.34, i32 0, i32 0), i8** %27
  %28 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 27
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.35, i32 0, i32 0), i8** %28
  %29 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 28
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.36, i32 0, i32 0), i8** %29
  %30 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 29
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.37, i32 0, i32 0), i8** %30
  %31 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 30
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.38, i32 0, i32 0), i8** %31
  %32 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 31
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.39, i32 0, i32 0), i8** %32
  %33 = getelementptr [100 x i8*], [100 x i8*]* %0, i32 0, i32 32
  store i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.40, i32 0, i32 0), i8** %33
  %34 = load [100 x i8*], [100 x i8*]* %0
  ret [100 x i8*] %34
}

define internal [20 x i8*] @keywords.init() {
entry:
  %0 = alloca [20 x i8*]
  %1 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 0
  store i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.41, i32 0, i32 0), i8** %1
  %2 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 1
  store i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.42, i32 0, i32 0), i8** %2
  %3 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 2
  store i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.43, i32 0, i32 0), i8** %3
  %4 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 3
  store i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.44, i32 0, i32 0), i8** %4
  %5 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 4
  store i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str.45, i32 0, i32 0), i8** %5
  %6 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 5
  store i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.46, i32 0, i32 0), i8** %6
  %7 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 6
  store i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.47, i32 0, i32 0), i8** %7
  %8 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 7
  store i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.48, i32 0, i32 0), i8** %8
  %9 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 8
  store i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.49, i32 0, i32 0), i8** %9
  %10 = getelementptr [20 x i8*], [20 x i8*]* %0, i32 0, i32 9
  store i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.50, i32 0, i32 0), i8** %10
  %11 = load [20 x i8*], [20 x i8*]* %0
  ret [20 x i8*] %11
}

define internal i32 @NUMBER.init() {
entry:
  ret i32 0
}

define internal i32 @STRING.init() {
entry:
  ret i32 1
}

define internal i32 @CHAR.init() {
entry:
  ret i32 2
}

define internal i32 @KEYWORD.init() {
entry:
  ret i32 3
}

define internal i32 @ID.init() {
entry:
  ret i32 4
}

define internal i32 @SYMBOL.init() {
entry:
  ret i32 5
}

define internal i32 @COMMENT.init() {
entry:
  ret i32 6
}

define void @"type.43$clear"(i8* %0) {
clear:
  %1 = bitcast i8* %0 to %Token.43*
  %self = getelementptr %Token.43, %Token.43* %1, i64 0
  %2 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  store i32 0, i32* %2
  %3 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 0
  %4 = alloca [20 x i8]
  %5 = load [20 x i8], [20 x i8]* %4
  store [20 x i8] %5, [20 x i8]* %3
  %6 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 2
  store i32 0, i32* %6
  %7 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 3
  store i1 false, i1* %7
  ret void
}

define void @"type.43$push"(i8 %0, i8* %1) {
push:
  %c = alloca i8
  store i8 %0, i8* %c
  %2 = bitcast i8* %1 to %Token.43*
  %self = getelementptr %Token.43, %Token.43* %2, i64 0
  %3 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 0
  %4 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  %5 = load i32, i32* %4
  %sext = sext i32 %5 to i64
  %6 = getelementptr [20 x i8], [20 x i8]* %3, i64 0, i64 %sext
  %7 = load i8, i8* %c
  store i8 %7, i8* %6
  %8 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  %9 = load i32, i32* %8
  %10 = add i32 %9, 1
  store i32 %10, i32* %8
  ret void
}

define void @"type.43$pop"(i8* %0) {
pop:
  %1 = bitcast i8* %0 to %Token.43*
  %self = getelementptr %Token.43, %Token.43* %1, i64 0
  %2 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  %3 = load i32, i32* %2
  %4 = sub i32 %3, 1
  store i32 %4, i32* %2
  %5 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 0
  %6 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  %7 = load i32, i32* %6
  %sext = sext i32 %7 to i64
  %8 = getelementptr [20 x i8], [20 x i8]* %5, i64 0, i64 %sext
  store i8 0, i8* %8
  ret void
}

define void @"type.43$print"(i8* %0) {
print:
  %1 = bitcast i8* %0 to %Token.43*
  %self = getelementptr %Token.43, %Token.43* %1, i64 0
  %2 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  %3 = load i32, i32* %2
  %4 = icmp eq i32 %3, 0
  br i1 %4, label %if.then, label %if.end

if.then:                                          ; preds = %print
  ret void

never:                                            ; No predecessors!
  br label %if.end

if.end:                                           ; preds = %never, %print
  %5 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 0
  %6 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 1
  %7 = load i32, i32* %6
  %sext = sext i32 %7 to i64
  %8 = getelementptr [20 x i8], [20 x i8]* %5, i64 0, i64 %sext
  store i8 0, i8* %8
  %9 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 2
  %10 = load i32, i32* %9
  %sext1 = sext i32 %10 to i64
  %11 = getelementptr [10 x i8*], [10 x i8*]* @token_name, i64 0, i64 %sext1
  %12 = load i8*, i8** %11
  %ret = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str.51, i32 0, i32 0), i8* %12)
  %13 = getelementptr %Token.43, %Token.43* %self, i64 0, i32 0
  %14 = getelementptr [20 x i8], [20 x i8]* %13, i32 0, i32 0
  %ret2 = call i32 @puts(i8* %14)
  %15 = alloca %trait.proto
  %16 = bitcast %Token.43* %self to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %15, i64 0, i32 0
  store i8* %16, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %15, i64 0, i32 1
  store i8* bitcast (void (i8*)* @"type.43$clear" to i8*), i8** %fn
  %17 = getelementptr %trait.proto, %trait.proto* %15, i64 0, i32 0
  %self3 = load i8*, i8** %17
  %18 = getelementptr %trait.proto, %trait.proto* %15, i64 0, i32 1
  %19 = load i8*, i8** %18
  %20 = getelementptr i8, i8* %19, i64 0
  %21 = bitcast i8* %20 to void (i8*)*
  call void %21(i8* %self3)
  ret void
}

define i1 @"type.13$eq"(i8* %0, i8* %1) {
eq:
  %a = alloca i8*
  %b = alloca i8*
  store i8* %0, i8** %b
  %2 = bitcast i8* %1 to i8**
  %self = getelementptr i8*, i8** %2, i64 0
  %3 = load i8*, i8** %self
  store i8* %3, i8** %a
  br label %while.cond

while.begin:                                      ; preds = %while.cond
  %4 = load i8*, i8** %a
  %5 = load i8, i8* %4
  %6 = load i8*, i8** %b
  %7 = load i8, i8* %6
  %8 = icmp ne i8 %5, %7
  br i1 %8, label %if.then, label %if.end

if.then:                                          ; preds = %while.begin
  ret i1 false

never:                                            ; No predecessors!
  br label %if.end

if.end:                                           ; preds = %never, %while.begin
  %9 = load i8*, i8** %a
  %10 = getelementptr i8, i8* %9, i32 1
  store i8* %10, i8** %a
  %11 = load i8*, i8** %b
  %12 = getelementptr i8, i8* %11, i32 1
  store i8* %12, i8** %b
  br label %while.cond

while.cond:                                       ; preds = %if.end, %eq
  %13 = load i8*, i8** %a
  %14 = load i8, i8* %13
  %15 = icmp ne i8 %14, 0
  %16 = load i8*, i8** %b
  %17 = load i8, i8* %16
  %18 = icmp ne i8 %17, 0
  %19 = select i1 %15, i1 %18, i1 false
  br i1 %19, label %while.begin, label %while.end

while.end:                                        ; preds = %while.cond
  %20 = load i8*, i8** %a
  %21 = load i8, i8* %20
  %22 = load i8*, i8** %b
  %23 = load i8, i8* %22
  %24 = icmp eq i8 %21, %23
  ret i1 %24

never1:                                           ; No predecessors!
  ret i1 false
}

define i1 @"type.11$is_alpha"(i8* %0) {
is_alpha:
  %self = getelementptr i8, i8* %0, i64 0
  %1 = load i8, i8* %self
  %2 = icmp sle i8 97, %1
  %3 = load i8, i8* %self
  %4 = icmp sle i8 %3, 122
  %5 = select i1 %2, i1 %4, i1 false
  %6 = load i8, i8* %self
  %7 = icmp sle i8 65, %6
  %8 = load i8, i8* %self
  %9 = icmp sle i8 %8, 90
  %10 = select i1 %7, i1 %9, i1 false
  %11 = select i1 %5, i1 true, i1 %10
  ret i1 %11
}

define i1 @"type.11$is_digit"(i8* %0) {
is_digit:
  %self = getelementptr i8, i8* %0, i64 0
  %1 = load i8, i8* %self
  %2 = icmp sle i8 48, %1
  %3 = load i8, i8* %self
  %4 = icmp sle i8 %3, 57
  %5 = select i1 %2, i1 %4, i1 false
  ret i1 %5
}

define i1 @"type.11$is_alnum"(i8* %0) {
is_alnum:
  %self = getelementptr i8, i8* %0, i64 0
  %1 = alloca %trait.proto
  %obj = getelementptr %trait.proto, %trait.proto* %1, i64 0, i32 0
  store i8* %self, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %1, i64 0, i32 1
  store i8* bitcast (i1 (i8*)* @"type.11$is_alpha" to i8*), i8** %fn
  %2 = getelementptr %trait.proto, %trait.proto* %1, i64 0, i32 0
  %self1 = load i8*, i8** %2
  %3 = getelementptr %trait.proto, %trait.proto* %1, i64 0, i32 1
  %4 = load i8*, i8** %3
  %5 = getelementptr i8, i8* %4, i64 0
  %6 = bitcast i8* %5 to i1 (i8*)*
  %ret = call i1 %6(i8* %self1)
  %7 = alloca %trait.proto
  %obj2 = getelementptr %trait.proto, %trait.proto* %7, i64 0, i32 0
  store i8* %self, i8** %obj2
  %fn3 = getelementptr %trait.proto, %trait.proto* %7, i64 0, i32 1
  store i8* bitcast (i1 (i8*)* @"type.11$is_digit" to i8*), i8** %fn3
  %8 = getelementptr %trait.proto, %trait.proto* %7, i64 0, i32 0
  %self4 = load i8*, i8** %8
  %9 = getelementptr %trait.proto, %trait.proto* %7, i64 0, i32 1
  %10 = load i8*, i8** %9
  %11 = getelementptr i8, i8* %10, i64 0
  %12 = bitcast i8* %11 to i1 (i8*)*
  %ret5 = call i1 %12(i8* %self4)
  %13 = select i1 %ret, i1 true, i1 %ret5
  ret i1 %13
}

define i1 @contain(i8** %0, i8* %1) {
contain:
  %item = alloca i8**
  %table = alloca i8**
  store i8** %0, i8*** %table
  %str = alloca i8*
  store i8* %1, i8** %str
  %2 = load i8**, i8*** %table
  store i8** %2, i8*** %item
  br label %for.cond

for.begin:                                        ; preds = %for.cond
  %3 = alloca %trait.proto
  %4 = load i8**, i8*** %item
  %5 = bitcast i8** %4 to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %3, i64 0, i32 0
  store i8* %5, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %3, i64 0, i32 1
  store i8* bitcast (i1 (i8*, i8*)* @"type.13$eq" to i8*), i8** %fn
  %6 = getelementptr %trait.proto, %trait.proto* %3, i64 0, i32 0
  %self = load i8*, i8** %6
  %7 = getelementptr %trait.proto, %trait.proto* %3, i64 0, i32 1
  %8 = load i8*, i8** %7
  %9 = getelementptr i8, i8* %8, i64 0
  %10 = bitcast i8* %9 to i1 (i8*, i8*)*
  %11 = load i8*, i8** %str
  %ret = call i1 %10(i8* %11, i8* %self)
  br i1 %ret, label %if.then, label %if.end

if.then:                                          ; preds = %for.begin
  ret i1 true

never:                                            ; No predecessors!
  br label %if.end

if.end:                                           ; preds = %never, %for.begin
  br label %for.nxt

for.nxt:                                          ; preds = %if.end
  %12 = load i8**, i8*** %item
  %13 = getelementptr i8*, i8** %12, i32 1
  store i8** %13, i8*** %item
  br label %for.cond

for.cond:                                         ; preds = %for.nxt, %contain
  %14 = alloca %trait.proto
  %15 = load i8**, i8*** %item
  %16 = bitcast i8** %15 to i8*
  %obj1 = getelementptr %trait.proto, %trait.proto* %14, i64 0, i32 0
  store i8* %16, i8** %obj1
  %fn2 = getelementptr %trait.proto, %trait.proto* %14, i64 0, i32 1
  store i8* bitcast (i1 (i8*, i8*)* @"type.13$eq" to i8*), i8** %fn2
  %17 = getelementptr %trait.proto, %trait.proto* %14, i64 0, i32 0
  %self3 = load i8*, i8** %17
  %18 = getelementptr %trait.proto, %trait.proto* %14, i64 0, i32 1
  %19 = load i8*, i8** %18
  %20 = getelementptr i8, i8* %19, i64 0
  %21 = bitcast i8* %20 to i1 (i8*, i8*)*
  %ret4 = call i1 %21(i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.52, i32 0, i32 0), i8* %self3)
  %22 = xor i1 %ret4, true
  br i1 %22, label %for.begin, label %for.end

for.end:                                          ; preds = %for.cond
  ret i1 false

never5:                                           ; No predecessors!
  ret i1 false
}

define i8* @state_refuse(%Token.43* %0, i8 %1) {
state_refuse:
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  ret i8* null
}

define i8* @state1(%Token.43* %0, i8 %1) {
state1:
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = alloca %trait.proto
  %obj = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  store i8* %c, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  store i8* bitcast (i1 (i8*)* @"type.11$is_alnum" to i8*), i8** %fn
  %3 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  %self = load i8*, i8** %3
  %4 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  %5 = load i8*, i8** %4
  %6 = getelementptr i8, i8* %5, i64 0
  %7 = bitcast i8* %6 to i1 (i8*)*
  %ret = call i1 %7(i8* %self)
  %8 = load i8, i8* %c
  %9 = icmp eq i8 %8, 95
  %10 = select i1 %ret, i1 true, i1 %9
  br i1 %10, label %if.then, label %if.else

if.then:                                          ; preds = %state1
  %11 = alloca %trait.proto
  %12 = bitcast %Token.43* %token to i8*
  %obj1 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  store i8* %12, i8** %obj1
  %fn2 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn2
  %13 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  %self3 = load i8*, i8** %13
  %14 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  %15 = load i8*, i8** %14
  %16 = getelementptr i8, i8* %15, i64 0
  %17 = bitcast i8* %16 to void (i8, i8*)*
  %18 = load i8, i8* %c
  call void %17(i8 %18, i8* %self3)
  br label %if.end6

if.else:                                          ; preds = %state1
  %19 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 0
  %20 = getelementptr [20 x i8], [20 x i8]* %19, i32 0, i32 0
  %ret4 = call i1 @contain(i8** getelementptr inbounds ([20 x i8*], [20 x i8*]* @keywords, i32 0, i32 0), i8* %20)
  br i1 %ret4, label %if.then5, label %if.end

if.then5:                                         ; preds = %if.else
  %21 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %22 = load i32, i32* @KEYWORD
  store i32 %22, i32* %21
  br label %if.end

if.end:                                           ; preds = %if.then5, %if.else
  br label %if.end6

if.end6:                                          ; preds = %if.end, %if.then
  %if.res = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state1 to i8*), %if.then ], [ null, %if.end ]
  ret i8* %if.res
}

define i8* @state2(%Token.43* %0, i8 %1) {
state2:
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = alloca %trait.proto
  %obj = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  store i8* %c, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  store i8* bitcast (i1 (i8*)* @"type.11$is_digit" to i8*), i8** %fn
  %3 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  %self = load i8*, i8** %3
  %4 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  %5 = load i8*, i8** %4
  %6 = getelementptr i8, i8* %5, i64 0
  %7 = bitcast i8* %6 to i1 (i8*)*
  %ret = call i1 %7(i8* %self)
  %8 = load i8, i8* %c
  %9 = icmp eq i8 %8, 46
  %10 = select i1 %ret, i1 true, i1 %9
  br i1 %10, label %if.then, label %if.else

if.then:                                          ; preds = %state2
  %11 = alloca %trait.proto
  %12 = bitcast %Token.43* %token to i8*
  %obj1 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  store i8* %12, i8** %obj1
  %fn2 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn2
  %13 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  %self3 = load i8*, i8** %13
  %14 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  %15 = load i8*, i8** %14
  %16 = getelementptr i8, i8* %15, i64 0
  %17 = bitcast i8* %16 to void (i8, i8*)*
  %18 = load i8, i8* %c
  call void %17(i8 %18, i8* %self3)
  br label %if.end

if.else:                                          ; preds = %state2
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %if.res = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state2 to i8*), %if.then ], [ null, %if.else ]
  ret i8* %if.res
}

define i8* @state3(%Token.43* %0, i8 %1) {
state3:
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = load i8, i8* %c
  %3 = icmp eq i8 %2, 39
  br i1 %3, label %if.then, label %if.else2

if.then:                                          ; preds = %state3
  %4 = alloca %trait.proto
  %5 = bitcast %Token.43* %token to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 0
  store i8* %5, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn
  %6 = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 0
  %self = load i8*, i8** %6
  %7 = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 1
  %8 = load i8*, i8** %7
  %9 = getelementptr i8, i8* %8, i64 0
  %10 = bitcast i8* %9 to void (i8, i8*)*
  %11 = load i8, i8* %c
  call void %10(i8 %11, i8* %self)
  %12 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  %13 = load i1, i1* %12
  br i1 %13, label %if.then1, label %if.else

if.then1:                                         ; preds = %if.then
  %14 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  store i1 false, i1* %14
  br label %if.end

if.else:                                          ; preds = %if.then
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then1
  %if.res = phi i8* (%Token.43*, i8)* [ @state3, %if.then1 ], [ @state_refuse, %if.else ]
  br label %if.end13

if.else2:                                         ; preds = %state3
  %15 = load i8, i8* %c
  %16 = icmp eq i8 %15, 92
  br i1 %16, label %if.then3, label %if.else7

if.then3:                                         ; preds = %if.else2
  %17 = alloca %trait.proto
  %18 = bitcast %Token.43* %token to i8*
  %obj4 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 0
  store i8* %18, i8** %obj4
  %fn5 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn5
  %19 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 0
  %self6 = load i8*, i8** %19
  %20 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 1
  %21 = load i8*, i8** %20
  %22 = getelementptr i8, i8* %21, i64 0
  %23 = bitcast i8* %22 to void (i8, i8*)*
  %24 = load i8, i8* %c
  call void %23(i8 %24, i8* %self6)
  %25 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  %26 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  %27 = load i1, i1* %26
  %zext = zext i1 %27 to i32
  %28 = xor i32 %zext, 1
  %tobool = icmp ne i32 %28, 0
  store i1 %tobool, i1* %25
  br label %if.end11

if.else7:                                         ; preds = %if.else2
  %29 = alloca %trait.proto
  %30 = bitcast %Token.43* %token to i8*
  %obj8 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 0
  store i8* %30, i8** %obj8
  %fn9 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn9
  %31 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 0
  %self10 = load i8*, i8** %31
  %32 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 1
  %33 = load i8*, i8** %32
  %34 = getelementptr i8, i8* %33, i64 0
  %35 = bitcast i8* %34 to void (i8, i8*)*
  %36 = load i8, i8* %c
  call void %35(i8 %36, i8* %self10)
  %37 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  store i1 false, i1* %37
  br label %if.end11

if.end11:                                         ; preds = %if.else7, %if.then3
  %if.res12 = phi i8* (%Token.43*, i8)* [ @state3, %if.then3 ], [ @state3, %if.else7 ]
  br label %if.end13

if.end13:                                         ; preds = %if.end11, %if.end
  %if.res14 = phi i8* (%Token.43*, i8)* [ %if.res, %if.end ], [ %if.res12, %if.end11 ]
  %ptr = bitcast i8* (%Token.43*, i8)* %if.res14 to i8*
  ret i8* %ptr
}

define i8* @state4(%Token.43* %0, i8 %1) {
state4:
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = load i8, i8* %c
  %3 = icmp eq i8 %2, 34
  br i1 %3, label %if.then, label %if.else2

if.then:                                          ; preds = %state4
  %4 = alloca %trait.proto
  %5 = bitcast %Token.43* %token to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 0
  store i8* %5, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn
  %6 = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 0
  %self = load i8*, i8** %6
  %7 = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 1
  %8 = load i8*, i8** %7
  %9 = getelementptr i8, i8* %8, i64 0
  %10 = bitcast i8* %9 to void (i8, i8*)*
  %11 = load i8, i8* %c
  call void %10(i8 %11, i8* %self)
  %12 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  %13 = load i1, i1* %12
  br i1 %13, label %if.then1, label %if.else

if.then1:                                         ; preds = %if.then
  %14 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  store i1 false, i1* %14
  br label %if.end

if.else:                                          ; preds = %if.then
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then1
  %if.res = phi i8* (%Token.43*, i8)* [ @state4, %if.then1 ], [ @state_refuse, %if.else ]
  br label %if.end13

if.else2:                                         ; preds = %state4
  %15 = load i8, i8* %c
  %16 = icmp eq i8 %15, 92
  br i1 %16, label %if.then3, label %if.else7

if.then3:                                         ; preds = %if.else2
  %17 = alloca %trait.proto
  %18 = bitcast %Token.43* %token to i8*
  %obj4 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 0
  store i8* %18, i8** %obj4
  %fn5 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn5
  %19 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 0
  %self6 = load i8*, i8** %19
  %20 = getelementptr %trait.proto, %trait.proto* %17, i64 0, i32 1
  %21 = load i8*, i8** %20
  %22 = getelementptr i8, i8* %21, i64 0
  %23 = bitcast i8* %22 to void (i8, i8*)*
  %24 = load i8, i8* %c
  call void %23(i8 %24, i8* %self6)
  %25 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  %26 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  %27 = load i1, i1* %26
  %zext = zext i1 %27 to i32
  %28 = xor i32 %zext, 1
  %tobool = icmp ne i32 %28, 0
  store i1 %tobool, i1* %25
  br label %if.end11

if.else7:                                         ; preds = %if.else2
  %29 = alloca %trait.proto
  %30 = bitcast %Token.43* %token to i8*
  %obj8 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 0
  store i8* %30, i8** %obj8
  %fn9 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn9
  %31 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 0
  %self10 = load i8*, i8** %31
  %32 = getelementptr %trait.proto, %trait.proto* %29, i64 0, i32 1
  %33 = load i8*, i8** %32
  %34 = getelementptr i8, i8* %33, i64 0
  %35 = bitcast i8* %34 to void (i8, i8*)*
  %36 = load i8, i8* %c
  call void %35(i8 %36, i8* %self10)
  %37 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 3
  store i1 false, i1* %37
  br label %if.end11

if.end11:                                         ; preds = %if.else7, %if.then3
  %if.res12 = phi i8* (%Token.43*, i8)* [ @state4, %if.then3 ], [ @state4, %if.else7 ]
  br label %if.end13

if.end13:                                         ; preds = %if.end11, %if.end
  %if.res14 = phi i8* (%Token.43*, i8)* [ %if.res, %if.end ], [ %if.res12, %if.end11 ]
  %ptr = bitcast i8* (%Token.43*, i8)* %if.res14 to i8*
  ret i8* %ptr
}

define i8* @state6(%Token.43* %0, i8 %1) {
state6:
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = load i8, i8* %c
  %3 = icmp eq i8 %2, 10
  br i1 %3, label %if.then, label %if.else

if.then:                                          ; preds = %state6
  br label %if.end

if.else:                                          ; preds = %state6
  %4 = alloca %trait.proto
  %5 = bitcast %Token.43* %token to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 0
  store i8* %5, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn
  %6 = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 0
  %self = load i8*, i8** %6
  %7 = getelementptr %trait.proto, %trait.proto* %4, i64 0, i32 1
  %8 = load i8*, i8** %7
  %9 = getelementptr i8, i8* %8, i64 0
  %10 = bitcast i8* %9 to void (i8, i8*)*
  %11 = load i8, i8* %c
  call void %10(i8 %11, i8* %self)
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %if.res = phi i8* [ null, %if.then ], [ bitcast (i8* (%Token.43*, i8)* @state6 to i8*), %if.else ]
  ret i8* %if.res
}

define i8* @state5(%Token.43* %0, i8 %1) {
state5:
  %str = alloca i8*
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = alloca %trait.proto
  %3 = bitcast %Token.43* %token to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  store i8* %3, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn
  %4 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  %self = load i8*, i8** %4
  %5 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  %6 = load i8*, i8** %5
  %7 = getelementptr i8, i8* %6, i64 0
  %8 = bitcast i8* %7 to void (i8, i8*)*
  %9 = load i8, i8* %c
  call void %8(i8 %9, i8* %self)
  %10 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 0
  %11 = getelementptr [20 x i8], [20 x i8]* %10, i32 0, i32 0
  store i8* %11, i8** %str
  %12 = alloca %trait.proto
  %13 = bitcast i8** %str to i8*
  %obj1 = getelementptr %trait.proto, %trait.proto* %12, i64 0, i32 0
  store i8* %13, i8** %obj1
  %fn2 = getelementptr %trait.proto, %trait.proto* %12, i64 0, i32 1
  store i8* bitcast (i1 (i8*, i8*)* @"type.13$eq" to i8*), i8** %fn2
  %14 = getelementptr %trait.proto, %trait.proto* %12, i64 0, i32 0
  %self3 = load i8*, i8** %14
  %15 = getelementptr %trait.proto, %trait.proto* %12, i64 0, i32 1
  %16 = load i8*, i8** %15
  %17 = getelementptr i8, i8* %16, i64 0
  %18 = bitcast i8* %17 to i1 (i8*, i8*)*
  %ret = call i1 %18(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.53, i32 0, i32 0), i8* %self3)
  br i1 %ret, label %if.then, label %if.else

if.then:                                          ; preds = %state5
  %19 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %20 = load i32, i32* @COMMENT
  store i32 %20, i32* %19
  br label %if.end10

if.else:                                          ; preds = %state5
  %21 = load i8*, i8** %str
  %ret4 = call i1 @contain(i8** getelementptr inbounds ([100 x i8*], [100 x i8*]* @symbols, i32 0, i32 0), i8* %21)
  br i1 %ret4, label %if.then5, label %if.else6

if.then5:                                         ; preds = %if.else
  br label %if.end

if.else6:                                         ; preds = %if.else
  %22 = alloca %trait.proto
  %23 = bitcast %Token.43* %token to i8*
  %obj7 = getelementptr %trait.proto, %trait.proto* %22, i64 0, i32 0
  store i8* %23, i8** %obj7
  %fn8 = getelementptr %trait.proto, %trait.proto* %22, i64 0, i32 1
  store i8* bitcast (void (i8*)* @"type.43$pop" to i8*), i8** %fn8
  %24 = getelementptr %trait.proto, %trait.proto* %22, i64 0, i32 0
  %self9 = load i8*, i8** %24
  %25 = getelementptr %trait.proto, %trait.proto* %22, i64 0, i32 1
  %26 = load i8*, i8** %25
  %27 = getelementptr i8, i8* %26, i64 0
  %28 = bitcast i8* %27 to void (i8*)*
  call void %28(i8* %self9)
  br label %if.end

if.end:                                           ; preds = %if.else6, %if.then5
  %if.res = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state5 to i8*), %if.then5 ], [ null, %if.else6 ]
  br label %if.end10

if.end10:                                         ; preds = %if.end, %if.then
  %if.res11 = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state6 to i8*), %if.then ], [ %if.res, %if.end ]
  ret i8* %if.res11
}

define i8* @state0(%Token.43* %0, i8 %1) {
state0:
  %s = alloca [2 x i8]
  %token = getelementptr %Token.43, %Token.43* %0, i64 0
  %c = alloca i8
  store i8 %1, i8* %c
  %2 = alloca %trait.proto
  %obj = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  store i8* %c, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  store i8* bitcast (i1 (i8*)* @"type.11$is_alpha" to i8*), i8** %fn
  %3 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 0
  %self = load i8*, i8** %3
  %4 = getelementptr %trait.proto, %trait.proto* %2, i64 0, i32 1
  %5 = load i8*, i8** %4
  %6 = getelementptr i8, i8* %5, i64 0
  %7 = bitcast i8* %6 to i1 (i8*)*
  %ret = call i1 %7(i8* %self)
  %8 = load i8, i8* %c
  %9 = icmp eq i8 %8, 95
  %10 = select i1 %ret, i1 true, i1 %9
  br i1 %10, label %if.then, label %if.else

if.then:                                          ; preds = %state0
  %11 = alloca %trait.proto
  %12 = bitcast %Token.43* %token to i8*
  %obj1 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  store i8* %12, i8** %obj1
  %fn2 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn2
  %13 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  %self3 = load i8*, i8** %13
  %14 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  %15 = load i8*, i8** %14
  %16 = getelementptr i8, i8* %15, i64 0
  %17 = bitcast i8* %16 to void (i8, i8*)*
  %18 = load i8, i8* %c
  call void %17(i8 %18, i8* %self3)
  %19 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %20 = load i32, i32* @ID
  store i32 %20, i32* %19
  br label %if.end39

if.else:                                          ; preds = %state0
  %21 = alloca %trait.proto
  %obj4 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 0
  store i8* %c, i8** %obj4
  %fn5 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 1
  store i8* bitcast (i1 (i8*)* @"type.11$is_digit" to i8*), i8** %fn5
  %22 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 0
  %self6 = load i8*, i8** %22
  %23 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 1
  %24 = load i8*, i8** %23
  %25 = getelementptr i8, i8* %24, i64 0
  %26 = bitcast i8* %25 to i1 (i8*)*
  %ret7 = call i1 %26(i8* %self6)
  br i1 %ret7, label %if.then8, label %if.else12

if.then8:                                         ; preds = %if.else
  %27 = alloca %trait.proto
  %28 = bitcast %Token.43* %token to i8*
  %obj9 = getelementptr %trait.proto, %trait.proto* %27, i64 0, i32 0
  store i8* %28, i8** %obj9
  %fn10 = getelementptr %trait.proto, %trait.proto* %27, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn10
  %29 = getelementptr %trait.proto, %trait.proto* %27, i64 0, i32 0
  %self11 = load i8*, i8** %29
  %30 = getelementptr %trait.proto, %trait.proto* %27, i64 0, i32 1
  %31 = load i8*, i8** %30
  %32 = getelementptr i8, i8* %31, i64 0
  %33 = bitcast i8* %32 to void (i8, i8*)*
  %34 = load i8, i8* %c
  call void %33(i8 %34, i8* %self11)
  %35 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %36 = load i32, i32* @NUMBER
  store i32 %36, i32* %35
  br label %if.end37

if.else12:                                        ; preds = %if.else
  %37 = load i8, i8* %c
  %38 = icmp eq i8 %37, 39
  br i1 %38, label %if.then13, label %if.else17

if.then13:                                        ; preds = %if.else12
  %39 = alloca %trait.proto
  %40 = bitcast %Token.43* %token to i8*
  %obj14 = getelementptr %trait.proto, %trait.proto* %39, i64 0, i32 0
  store i8* %40, i8** %obj14
  %fn15 = getelementptr %trait.proto, %trait.proto* %39, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn15
  %41 = getelementptr %trait.proto, %trait.proto* %39, i64 0, i32 0
  %self16 = load i8*, i8** %41
  %42 = getelementptr %trait.proto, %trait.proto* %39, i64 0, i32 1
  %43 = load i8*, i8** %42
  %44 = getelementptr i8, i8* %43, i64 0
  %45 = bitcast i8* %44 to void (i8, i8*)*
  %46 = load i8, i8* %c
  call void %45(i8 %46, i8* %self16)
  %47 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %48 = load i32, i32* @CHAR
  store i32 %48, i32* %47
  br label %if.end35

if.else17:                                        ; preds = %if.else12
  %49 = load i8, i8* %c
  %50 = icmp eq i8 %49, 34
  br i1 %50, label %if.then18, label %if.else22

if.then18:                                        ; preds = %if.else17
  %51 = alloca %trait.proto
  %52 = bitcast %Token.43* %token to i8*
  %obj19 = getelementptr %trait.proto, %trait.proto* %51, i64 0, i32 0
  store i8* %52, i8** %obj19
  %fn20 = getelementptr %trait.proto, %trait.proto* %51, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn20
  %53 = getelementptr %trait.proto, %trait.proto* %51, i64 0, i32 0
  %self21 = load i8*, i8** %53
  %54 = getelementptr %trait.proto, %trait.proto* %51, i64 0, i32 1
  %55 = load i8*, i8** %54
  %56 = getelementptr i8, i8* %55, i64 0
  %57 = bitcast i8* %56 to void (i8, i8*)*
  %58 = load i8, i8* %c
  call void %57(i8 %58, i8* %self21)
  %59 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %60 = load i32, i32* @STRING
  store i32 %60, i32* %59
  br label %if.end33

if.else22:                                        ; preds = %if.else17
  %61 = load i8, i8* %c
  %62 = icmp eq i8 %61, 9
  %63 = load i8, i8* %c
  %64 = icmp eq i8 %63, 10
  %65 = select i1 %62, i1 true, i1 %64
  %66 = load i8, i8* %c
  %67 = icmp eq i8 %66, 32
  %68 = select i1 %65, i1 true, i1 %67
  br i1 %68, label %if.then23, label %if.else24

if.then23:                                        ; preds = %if.else22
  br label %if.end31

if.else24:                                        ; preds = %if.else22
  %69 = alloca [2 x i8]
  %70 = load i8, i8* %c
  %71 = getelementptr [2 x i8], [2 x i8]* %69, i32 0, i32 0
  store i8 %70, i8* %71
  %72 = getelementptr [2 x i8], [2 x i8]* %69, i32 0, i32 1
  store i8 0, i8* %72
  %73 = load [2 x i8], [2 x i8]* %69
  store [2 x i8] %73, [2 x i8]* %s
  %74 = getelementptr [2 x i8], [2 x i8]* %s, i32 0, i32 0
  %ret25 = call i1 @contain(i8** getelementptr inbounds ([100 x i8*], [100 x i8*]* @symbols, i32 0, i32 0), i8* %74)
  br i1 %ret25, label %if.then26, label %if.else30

if.then26:                                        ; preds = %if.else24
  %75 = getelementptr %Token.43, %Token.43* %token, i64 0, i32 2
  %76 = load i32, i32* @SYMBOL
  store i32 %76, i32* %75
  %77 = alloca %trait.proto
  %78 = bitcast %Token.43* %token to i8*
  %obj27 = getelementptr %trait.proto, %trait.proto* %77, i64 0, i32 0
  store i8* %78, i8** %obj27
  %fn28 = getelementptr %trait.proto, %trait.proto* %77, i64 0, i32 1
  store i8* bitcast (void (i8, i8*)* @"type.43$push" to i8*), i8** %fn28
  %79 = getelementptr %trait.proto, %trait.proto* %77, i64 0, i32 0
  %self29 = load i8*, i8** %79
  %80 = getelementptr %trait.proto, %trait.proto* %77, i64 0, i32 1
  %81 = load i8*, i8** %80
  %82 = getelementptr i8, i8* %81, i64 0
  %83 = bitcast i8* %82 to void (i8, i8*)*
  %84 = load i8, i8* %c
  call void %83(i8 %84, i8* %self29)
  br label %if.end

if.else30:                                        ; preds = %if.else24
  br label %if.end

if.end:                                           ; preds = %if.else30, %if.then26
  %if.res = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state5 to i8*), %if.then26 ], [ null, %if.else30 ]
  br label %if.end31

if.end31:                                         ; preds = %if.end, %if.then23
  %if.res32 = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state0 to i8*), %if.then23 ], [ %if.res, %if.end ]
  br label %if.end33

if.end33:                                         ; preds = %if.end31, %if.then18
  %if.res34 = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state4 to i8*), %if.then18 ], [ %if.res32, %if.end31 ]
  br label %if.end35

if.end35:                                         ; preds = %if.end33, %if.then13
  %if.res36 = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state3 to i8*), %if.then13 ], [ %if.res34, %if.end33 ]
  br label %if.end37

if.end37:                                         ; preds = %if.end35, %if.then8
  %if.res38 = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state2 to i8*), %if.then8 ], [ %if.res36, %if.end35 ]
  br label %if.end39

if.end39:                                         ; preds = %if.end37, %if.then
  %if.res40 = phi i8* [ bitcast (i8* (%Token.43*, i8)* @state1 to i8*), %if.then ], [ %if.res38, %if.end37 ]
  ret i8* %if.res40
}

define i32 @main() {
main:
  call void @global.var.init()
  %n_state = alloca i8* (%Token.43*, i8)*
  %token = alloca %Token.43
  %state = alloca i8* (%Token.43*, i8)*
  %ch = alloca i8
  %ret = call i32 @getchar()
  %trunc = trunc i32 %ret to i8
  store i8 %trunc, i8* %ch
  store i8* (%Token.43*, i8)* @state0, i8* (%Token.43*, i8)** %state
  %0 = alloca %trait.proto
  %1 = bitcast %Token.43* %token to i8*
  %obj = getelementptr %trait.proto, %trait.proto* %0, i64 0, i32 0
  store i8* %1, i8** %obj
  %fn = getelementptr %trait.proto, %trait.proto* %0, i64 0, i32 1
  store i8* bitcast (void (i8*)* @"type.43$clear" to i8*), i8** %fn
  %2 = getelementptr %trait.proto, %trait.proto* %0, i64 0, i32 0
  %self = load i8*, i8** %2
  %3 = getelementptr %trait.proto, %trait.proto* %0, i64 0, i32 1
  %4 = load i8*, i8** %3
  %5 = getelementptr i8, i8* %4, i64 0
  %6 = bitcast i8* %5 to void (i8*)*
  call void %6(i8* %self)
  br label %while.cond

while.begin:                                      ; preds = %while.cond
  %7 = load i8* (%Token.43*, i8)*, i8* (%Token.43*, i8)** %state
  %8 = load i8, i8* %ch
  %ret1 = call i8* %7(%Token.43* %token, i8 %8)
  %ptr = bitcast i8* %ret1 to i8* (%Token.43*, i8)*
  store i8* (%Token.43*, i8)* %ptr, i8* (%Token.43*, i8)** %n_state
  %9 = load i8* (%Token.43*, i8)*, i8* (%Token.43*, i8)** %n_state
  %ptr2 = bitcast i8* (%Token.43*, i8)* %9 to i8*
  %10 = icmp eq i8* %ptr2, null
  br i1 %10, label %if.then, label %if.else

if.then:                                          ; preds = %while.begin
  %11 = alloca %trait.proto
  %12 = bitcast %Token.43* %token to i8*
  %obj3 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  store i8* %12, i8** %obj3
  %fn4 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  store i8* bitcast (void (i8*)* @"type.43$print" to i8*), i8** %fn4
  %13 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 0
  %self5 = load i8*, i8** %13
  %14 = getelementptr %trait.proto, %trait.proto* %11, i64 0, i32 1
  %15 = load i8*, i8** %14
  %16 = getelementptr i8, i8* %15, i64 0
  %17 = bitcast i8* %16 to void (i8*)*
  call void %17(i8* %self5)
  br label %if.end

if.else:                                          ; preds = %while.begin
  %ret6 = call i32 @getchar()
  %trunc7 = trunc i32 %ret6 to i8
  store i8 %trunc7, i8* %ch
  %18 = load i8* (%Token.43*, i8)*, i8* (%Token.43*, i8)** %n_state
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %if.res = phi i8* (%Token.43*, i8)* [ @state0, %if.then ], [ %18, %if.else ]
  store i8* (%Token.43*, i8)* %if.res, i8* (%Token.43*, i8)** %state
  br label %while.cond

while.cond:                                       ; preds = %if.end, %main
  %19 = load i8, i8* %ch
  %sext = sext i8 %19 to i32
  %20 = icmp ne i32 %sext, -1
  br i1 %20, label %while.begin, label %while.end

while.end:                                        ; preds = %while.cond
  %21 = alloca %trait.proto
  %22 = bitcast %Token.43* %token to i8*
  %obj8 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 0
  store i8* %22, i8** %obj8
  %fn9 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 1
  store i8* bitcast (void (i8*)* @"type.43$print" to i8*), i8** %fn9
  %23 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 0
  %self10 = load i8*, i8** %23
  %24 = getelementptr %trait.proto, %trait.proto* %21, i64 0, i32 1
  %25 = load i8*, i8** %24
  %26 = getelementptr i8, i8* %25, i64 0
  %27 = bitcast i8* %26 to void (i8*)*
  call void %27(i8* %self10)
  ret i32 0
}
