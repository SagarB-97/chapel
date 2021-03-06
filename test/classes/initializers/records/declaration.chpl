// Tests behavior when no zero-argument or all default value initializer
// declared for the type
record Foo {
  var x: int;

  proc init(xVal) {
    x = xVal;
    super.init();
  }
}

var f: Foo;
writeln(f);
