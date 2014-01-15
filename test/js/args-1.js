function foo(x, y)
{
    arguments[0] = 232;
    if (arguments[0] != 232)
        $ERROR('#1 expected: arguments[0] == 232; actual: arguments[0] == ' + arguments[0]);
    if (x != 232)
        $ERROR('#2 expected: x == 232; actual: x == ' + x);
    return arguments;
}

if (foo(42)[0] != 232)
    $ERROR('#3 expected: foo(42)[0] == 232; actual: foo(42)[0] == ' + foo(42)[0]);
