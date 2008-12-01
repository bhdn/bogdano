program Test_and;
var
	a : integer;
begin
	a := 1 and 0;
	a := a and 0;
	a := 0 and a;
	a := 0 and 0;
	if (a = 1) and (a <> 2) then
	    a := 0 and 1;
end.
