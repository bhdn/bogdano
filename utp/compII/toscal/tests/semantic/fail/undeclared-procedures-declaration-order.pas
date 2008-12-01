program procedures_declared_out_of_order;
(* we don't expect to have procedures declared in any order to work, either
   we don't have functions prototype for now. *)

procedure a;
begin
	b(1, 2, 3);
end;

procedure b(k, l, m : integer) : integer;
begin
	k := l + m;
end;

begin
	a;
end.


