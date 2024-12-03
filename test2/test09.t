func compare(int x, int y) -> int {
    if (x > y) {1}
    else if (y > x) {-1}
    else {0}
}
func test_2_r09 () -> int{
    let int a = 5;
    let int b = 9;
    let int c = 7;
    compare(a, b, c)
    compare(a, b)
    compare(a)
    compare()
}