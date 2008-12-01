program test_labels_minimal;
label 100, 200;
var x : integer;

begin
	x := 10;
	if x = 10 then
		goto 100
	else 
		goto 200;
200:
	write(1);
100:
	write(2);
end.
