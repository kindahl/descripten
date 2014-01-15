function f()
{
    var a = 42;
    eval("if (a != 42) $ERROR('#1 expected: a == 42; actual: a == ' + a);");
}

f();
