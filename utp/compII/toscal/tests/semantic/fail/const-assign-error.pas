program const_assign_error;
const 
	Bla = 666;
	Vla = 666.0;
	Cla = '?';
var x : integer;
begin
	x := Bla + 1;
	Bla := 777;
end.
