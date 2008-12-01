program Foo;
var k, z : char;

function foo(var t, p, j : char) : integer ;
var x, blabla, L: integer;
begin
	(*L := 10*)
	if x or z then
		blabla := 'a' and 'b'
	else
		z := 'a';
	p := 'b';

	foo := 10
end;

procedure bar;
var x : integer;
begin
	foo(1.2, 'b', foo(1,2,3))
end;

begin
	foo(1.2, 'b', 'd') (* crvl #fooaddr *)
end.
