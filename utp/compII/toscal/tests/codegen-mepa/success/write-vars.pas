program test_write_function;
var x : integer;

function f : integer;
begin
	f := 22222;
end;

function g(p : integer) : integer;
var t : integer;
begin
	t := p * -100;
	write(t, f * -1000);
end;

begin
	x := g(444)
end.
