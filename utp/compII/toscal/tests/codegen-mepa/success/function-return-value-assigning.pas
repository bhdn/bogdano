program test_sum;
var res : integer;

function sum_two_values(x, y : integer) : integer;
begin
	sum_two_values := x + y;
end;

begin
	res := sum_two_values(2, 1000);
end.
