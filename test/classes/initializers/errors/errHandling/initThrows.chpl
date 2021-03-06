// Verifies we currently don't allow initializers to be declared as throws
// We hope to eventually allow it.
class Foo {
  var x: int;

  proc init() throws {
    x = 10;
    super.init();
  }
}

var foo = new Foo();
writeln(foo);
delete foo;
