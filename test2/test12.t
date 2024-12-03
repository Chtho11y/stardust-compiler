func test_2_r12 () -> int{
    let a: int[3];
    let b: float[4];
    a[0] = a[a[2]];
    b[0] = b[a[1]];
    a[0] = a[b[0]];
    0
}