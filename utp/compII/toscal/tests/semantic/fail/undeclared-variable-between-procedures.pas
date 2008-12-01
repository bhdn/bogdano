program undeclared_variables_between_procedures;

procedure a(z, l, m : integer);
begin
	z := l + m;
end;

function b : integer;
begin
	b := l + m;
end;

begin
	b;
end.
