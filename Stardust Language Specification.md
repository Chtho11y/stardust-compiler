# Stardust 说明

## 编译器设计和实现

### 环境

| Name  | Value                                       |
| ----- | ------------------------------------------- |
| Flex  | flex 2.6.4                                  |
| Bison | bison (GNU Bison) 3.8.2                     |
| gcc   | gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0   |
| Make  | GNU Make 4.3. Built for x86_64-pc-linux-gnu |

### 项目结构

```
Project4
├-- inc
|	├--ast.h
|	├--context.h
|	├--error.h
|	├--parse.h
|	├--util.h
|	├--var_type.h
|	├--arg_parse.h //命令行参数包装
|	└─-clipp.h 	//命令行工具 (https://github.com/muellan/clipp)
└─- src
	├-- lex
	|	├-- lex.l
	|	└─- syntax.y
	├-- ast.cpp //语法树实现
	├-- context.cpp //parsing上下文、locator等信息
	├-- error.cpp //报错系统
	├-- main.cpp //编译器入口
	├-- operator.cpp //操作符实现
	├-- type.cpp //类型系统
	└─- util.cpp
	
```

### 编译流程

#### Phase 1：

构建语法树：

- 边 parse 边构建初步符号表，确定 token 是否为类型名。
- 跟踪每个 token 的位置，进行具体的语义错误定位。

#### Phase 2：

构建符号表：

- 对每个符号和自定义类型，向上找到最近的作用域，将名称和类型信息加入其中，然后计算每个AST节点的返回值（同时为每个变量名和自建类型名分配唯一编号）。

## 基本语法

### 外层声明

#### 变量声明

```
let a : int;
let a : int = 10;
let a : float64 = 1.1;

let a : int[10]; //数组
let a : int[10][20]; //10 x 20 
let a : (int[10])[20]; //20 x 10
let a : int*[10]*[20] //20 x (10 x (int*))*
let c = a.length;

let a : int* = (int*)10;
let c = a[0];
let b : int = *a;
```

#### 结构体声明

```
struct a {
	a1 : int,
	a2 : float,
	a3 : b
};
let aa: a = struct{ a1: 10, a2: 10.0, a3: struct{b1:10.0} }; //结构体实例化
```

#### 函数声明

不指明返回值默认无返回值

```
func f(a : int, b : float) {
	...
}
func f(a : int, b : float) -> int {
	...
	return 1; // 1 也可以，作为执行块的返回值
}
```

### 执行块

花括号包裹的由分号分隔的若干表达式，是否有返回值由末尾是否有分号决定。

#### 无返回值

```
{
	let a : int = 1;
	{a = a + 1;}
	a = a + 1;
}
```

#### 有返回值

视为表达式，单独作为语句时必须由分号分隔。

```
{
	let a : int = 1;
	let b : int = {a += 1};
	{b += 1}; 
	a, b
}
```

### 控制语句

只允许 if-else 有返回值，返回值为，语法上要求单独的 if / while / for 无返回值 。

`break;`用于跳出循环；`return; / return value;`作用于函数，用于函数返回。

```
while a > b { 
    if a > 1 {
        break;
    }else if a > 0 {
        a = a + 2;
    }else {
        a = a + 1;
    }
}

for let i: int = 0; i < 5; i+=1 {
	a = a + 1;
}
```

### 项

操作的基本单元

左值：标识符、数组实例、数组成员、结构体实例、结构体成员、函数、括号包裹的左值表达式。

右值：字面量、有返回值的执行块、有返回值的 if-else 语句、括号包裹的右值表达式。

### 表达式

项或求值语句，支持的基本的四则运算、布尔运算及位运算。此外，还支持以下的特殊运算符：

- 一元运算符：

  `*` 解引用，`&` 取地址。

- 二元运算符：

  `+= -=` 等运算赋值，`,`逗号表达式，值为第二个操作数的值。

## 特殊功能

```
let a = 10; //声明时可以进行自动类型推导

//函数内结构体
func test_struct_in_function() -> int {
	struct Student {
		weight : int
	}
    struct Point {
        x: int,
        y: int,
        aa: Student
    }
    let p: Point = struct{ x: 10, y: 20, aa:struct{weight:10} }; 
    p.x + p.y
}

//函数内函数
func outer_func() -> int {
    func inner_func() -> int {
        10
    }
    let result = inner_func(); 
    result * 2        
}

```

## 类型系统

```
bool < char < int < float
int8 < uint8 < int16 < ... < uint64
float32 ＜ float64
void = ()
```

- 允许隐式向上转换，向下转换必须使用强制转换
- 判断语句只支持 bool，但允许隐式转换

## 测试用例说明

18个测试用例由三部分组成

- test_xx.sd 是由github上提供的spl样例转换得到的测试用例（10个）
- test_nfeaturex.sd 是展示我们设计语言新特性的正确测试用例（7个）
- semantic_error.sd 是所有实现的语义错误检查的汇总（1个）

### 1. 变量相关

- 重复定义 (test04.sd, semantic_error.sd:C01)
- 未定义使用 (test01.sd, semantic_error.sd:C02)

### 2. 函数相关
- 重复定义 (test02.sd)
- 参数不匹配 (test09.sd, semantic_error.sd:C13)
- 返回值类型错误 (test03.sd)
- 未定义调用 (test01.sd)

### 3. 类型相关

- 类型不匹配 (semantic_error.sd:C04)
- 非法类型转换 (semantic_error.sd:C07,C08)
- 非法赋值 (semantic_error.sd:C05)

### 4.  结构体相关

- 定义错误 (test04.sd, test05.sd)
- 成员访问 (test05.sd, test06.sd)
- 嵌套结构体 (test06.sd)

### 5.  数组相关

- 维度错误 (test10.sd)
- 索引错误 (test10.sd, test12.sd)
- 类型不匹配 (test12.sd)

### 6. 其他特性

- 作用域和遮蔽 (test_nfeature5.sd)

- 解引用错误 (semantic_error.sd:C06)
- 地址操作错误 (test_nfeature3.sd)

- 类型推导 (test_nfeature1.sd / test_nfeature9.sd)
- 类型转换 (test_nfeature2.sd)
- 指针操作 (test_nfeature3.sd / test_nfeature8.sd)

- 内部函数 (test_nfeature6.sd)
- 函数内结构体 (test_nfeature7.sd)
- 代码块返回值 (test_nfeature5.sd)
- for循环 (test_nfeature5.sd)
