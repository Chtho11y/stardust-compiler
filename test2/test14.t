struct test {
    let a: int;
    let b: float;
};
func test_2_r14 () -> int{
    struct test t;
    t.a = 1;
    t.b = 20.0;
    t.c = t.a;
    0
}