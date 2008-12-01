program while_nested;
var i, j, k : integer;

begin
	k := 0;
	i := 0;
	while i < 5 do begin
		j := 0;
		while j < 5 do begin
			k := k + 1;
			j := j + 1;
		end;
		i := i + 1;
	end;
end.
