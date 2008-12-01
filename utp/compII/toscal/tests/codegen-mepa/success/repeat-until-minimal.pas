program test_repeat_until_minimal;
var i, j, k, z : integer;

begin
	i := 0;
	j := 0;
	k := 0;

	(* single statement *)
	repeat
		i := i + 1
	until i = 9;

	(* grouped statements, ends with semicolon *)
	repeat
		j := j + 1;
		k := k + 1;
	until k = 9;

	k := 0;
	j := 0;
	z := 0;

	(* grouped statements, without ending semicolon *)
	repeat
		j := j + 1;
		k := k + 1;
		z := z + 10
	until k = 9
end.
