program while_nested;
var i, j, k : integer;

function foo(x, y : integer) : integer;
var p, z : integer;
begin
	x := y + k;
	foo := x + p + z;
end;

begin
	i := 0;
	k := 10;
	while i < 5 do begin
		j := 0;
		while j < 5 do begin
			k := k + 1;
			j := j + 1;
		end;
		i := i + 1;
	end;
end.
