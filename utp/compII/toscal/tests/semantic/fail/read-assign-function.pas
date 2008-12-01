program test_read_function;
var i, j, k : integer;

function f : integer;
begin
	read(f)
end;

function g : integer;
var t : integer;
begin
	read(g, f);
end;

begin
	i := f * -10;
	j := g * -1000;
end.
