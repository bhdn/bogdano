program test_cond_nested;
var a, b, c : integer;
begin
	a := 10;
	b := 11;
	c := 12;

	if b < a then
		if c < b then
			if a < c then
				a := 1
			else
				a := 0
		else
			a := -1
	else
		a := -2;
end.
