program test_labels_loop;
label 100, 200;
var x : integer;

begin
	x := 0;
100:
	if not (x < 10)	then
		goto 200;
	write(x);
	x := x + 1;
	goto 100;
200:
	write(-1)
end.
