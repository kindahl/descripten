function f()
{
    var a = new Object();
    a.foo = 42;
    with (a)
    {
        foo = 32;
    }

    if (a.foo != 32)
        $ERROR("#1 expected: a.foo == 32; actual: a.foo == " + a.foo);
}

f();
