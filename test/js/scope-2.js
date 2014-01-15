print("NOTE: test should fail with a reference error.");

function f()
{
    var e = eval;
    var a = 42;
    e("print('a: ' + a);");
}

f();
