program check_cond_expr_type;
var x, y, z : integer;
    k, j, l: char;
begin
	x := 1;
	y := 2;
	z := 3;
	k := 'a';
	j := 'b';
	l := 'c';

	if (x < y) and (z > x) then
		y := z + x
	else
		y := 0;

	if k then
		l :=  '?';
end.
