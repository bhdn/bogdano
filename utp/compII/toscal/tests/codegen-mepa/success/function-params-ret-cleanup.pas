program test_funcall;
var x : integer;

function foo(a, b, c : integer) : integer;
begin
	c := a + b;

	foo := c + 100000;
end;

begin
	foo(100, 200, 300);
end.


