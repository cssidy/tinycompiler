// absolute value - exercises if without else, subtraction, < comparison
function abs(x) {
    if (x < 0) {
        return 0 - x;
    }
    return x;
}

// max of two values - exercises if/else, > comparison
function max(a, b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

// check even - exercises %, == comparison, boolean literals
function isEven(n) {
    if (n % 2 == 0) {
        return true;
    }
    return false;
}

// gcd - exercises while, !=, %, var with initializer, assignment
function gcd(a, b) {
    while (b != 0) {
        var t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// sum 1..n - exercises while, +, var without initializer, <=... using !=
function sumTo(n) {
    var sum = 0;
    var i = 1;
    while (i != n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

// compute - exercises calls as expressions, *, /, parenthesized expressions,
//           multiple var decls, nested arithmetic
function compute(a, b) {
    var g = gcd(a, b);
    var s = sumTo(10);
    var m = max(abs(a), abs(b));
    var result = (s + g) * m / 2;
    return result;
}
