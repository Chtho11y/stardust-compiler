struct Apple {
    let weight: int;
    let round: float;
    let price: float;
};

struct Apple {
    let round: int;
    let weight: float;
    let price: float;
};

func test_2_r15 () -> int{
    struct Apple aa;
    aa.price = 2;
    0
}