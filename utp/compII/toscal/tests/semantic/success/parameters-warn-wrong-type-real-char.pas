program checks_the_type_of_parameters_called_in_a_function;

procedure p(a, b, c : char);
var z : char;
begin
	z := a;
	z := b;
	z := c;
end;

begin
	p(1.1, 2.2, 3.3);
end.
