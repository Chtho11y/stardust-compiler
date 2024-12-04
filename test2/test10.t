func test_2_r10 () -> int{
    let a: int[2][4][6][8];
    let b: int[4][6][8];
    let c: int[6][8];
    let d: int[8];
    a[0] = b;
    a[0][1] = c;
    a[0][1][3] = d;
    a[0][1][3][5] = e;
    a[e][e][e][e][e]
}