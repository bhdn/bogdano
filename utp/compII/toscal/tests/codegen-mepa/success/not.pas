program test_not;
var x, y : integer;

begin
	y := 10;
	x := not (y > 5);
	x := not 5;
	x := not 0;
	x := not 1;
	x := not not not 0;
	write(30, x);
	x := not not not 1;
	write(31, x);
	x := not not 1;
	write(21, x);
	x := not not 0;
	write(20, x);
end.
