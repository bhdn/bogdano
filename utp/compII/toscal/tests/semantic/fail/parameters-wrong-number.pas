program check_the_number_of_parameters;

procedure p(a, b, c : integer);
begin
	a := 10;
	b := 20;
	c := 30;
end;

function f(a, b, c : char) : integer;
begin
	a := 'a';
	b := 'b';
	c := 'c';
	f := 10
end;

begin
	p(1,2,3);
	f('z', 'k');
end.
