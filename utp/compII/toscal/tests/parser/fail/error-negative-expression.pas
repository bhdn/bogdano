program test_negative_numbers;
var x, y : integer;
begin
	x := -1;
	y := 5 * -x;
	y := 5 * -not x;
	y := x * 5 * -10 + x div -8;
end.
