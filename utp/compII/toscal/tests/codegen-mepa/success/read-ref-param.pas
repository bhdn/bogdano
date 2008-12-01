program test_by_ref;
var p, q : integer;

function mul(x, y : integer) : integer;
begin
	mul := x * y
end;

procedure myread(var v : integer);
begin
	read(v);
end;

begin
	p := 2;
	myread(p);
	q := 3;
	myread(q);
	write(mul(p, q));
end.
