program test_labels_loop;
var res : integer;

function foo(a, b : integer) : integer;

	function subfoo(x : integer) : integer;
	var aux : integer;

		function subsubfoo(y : integer) : integer;
		var l : integer;

			function subsubsubfoo(j : integer) : integer;
			var essavaivaler2 : integer;
			begin
				essavaivaler2 := -j;
				subsubsubfoo := j;
			end;

		begin
			l := -subsubsubfoo(y);
			write(l);
			subsubfoo := -l;
		end;

	begin
		aux := subsubfoo(1);
		aux := x * 100;
		subfoo := aux + 1
	end;
begin
	foo := a + subfoo(b)
end;

function bar(x : integer) : integer;
begin
	bar := -foo(100, x)
end;

begin
	res := bar(5);
	write(res);
end.
