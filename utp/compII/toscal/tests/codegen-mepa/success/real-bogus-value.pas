program real_bogus_value;
(* This program shows the bizarre bug in the parser that makes an real
 * constant look "0" in the generated MEPA code. *)
const 
	THE_VALUE = 666.0;
var 
	x : real;
begin
	x := THE_VALUE * 10;
end.
