## TODO

- 结构体访问
- And/Or 的短路
- For 循环
- 数组/结构体初始化

## Usage

+ 下载 llvm （llvm-10及以上）,同时确保安装 clang

  ```
  apt install llvm
  ```

+ 使用 --ir=llvm 生成 IR （如果启用了打印ast等选项，记得删除多余信息）

  ```
  stardust --ir=llvm -o res.ll
  ```

+ 使用 clang 编译 llvm ir 并链接库函数

  ```
  clang sdlib/lib.o res.ll -lc -o out
  ```

  即可得到可执行文件 out.

或者直接使用 lli 执行 llvm ir，此时不支持 read/write 函数。

```
lli res.ll
```

