program complain_about_redeclared_procedures;
var a, b : integer;

procedure l;
var b, c : char;
	b := 10;
	c := 10;
end;

begin
	l
end;
