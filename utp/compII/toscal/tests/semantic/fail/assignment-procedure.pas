program test_labels_loop;
var res : integer;

procedure bar(x : integer);
var k : integer;
begin
	k := x * -1000;
end;

begin
	res := bar(5);
	write(res);
end.
