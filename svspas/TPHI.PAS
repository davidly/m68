{$R-}
program tphi( output );

var
    prev2, prev1, next : longint;
    i : longint;
    v : double;

begin
    writeln( 'should tend towards 1.618033988749...' );
    prev1 := 1;
    prev2 := 1;

    for i := 1 to 14 do begin { longint overflow beyond 21. can't use longint for division }
        next := prev1 + prev2;
        prev2 := prev1;
        prev1 := next;
        v := prev1 / prev2;
        writeln( '  at ', i, ' iterations: ', v );
    end;
end.
