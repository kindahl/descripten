print("NOTE: test should fail with a reference error.");

function f()
{
    var a = 42;
    var foo = new Function("print('a: ' + a);");
    foo();
}

f();
