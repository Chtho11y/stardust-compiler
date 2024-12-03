struct Child {
    let name: int;
    let age: int;
};
struct Parent
{
  struct Child c;
  let cc: int;
};
func test_2_r07 () -> int{
  struct Parent father;
  struct Parent mother;
  father.c.name = 1;
  father.c.age = 1;
  father.cc = 30;
  mother.c.name = father.c.name;
  mother.cc.age = father.cc.age;
  mother.cc = father.cc;
  0
}