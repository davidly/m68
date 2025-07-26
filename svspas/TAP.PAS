program tap;

var
    loops, i, prev, total, greatest, a, b, c : longint;
    randSeed : longint;
    v, dtotal : double;

function gcd( m : longint; n : longint ) : longint;
var
    x, y, r : longint;
begin { gcd }
     x := 0;
     if ( m > n ) then begin
         y := m;
         r := n;
     end
     else begin
         y := n;
         r := m;
     end;

    while ( 0 <> r ) do begin
        x := y;
        y := r;
        r := x MOD y;
    end;

    gcd := y;
end; { gcd }

procedure first_implementation;
var
    limit, j, prev : longint;
    sofar, dj : double;
begin
    limit := 100000;
    sofar := 0.0;
    prev := 1;

    for j := 1 to limit do begin
        dj := j;
        sofar := sofar + ( 1.0 / ( dj * dj * dj ) );
        if ( j = ( prev * 10 ) ) then begin
            prev := j;
            writeln( '  at ', j, ' iterations: ', sofar );
        end;
    end;
end;

{ svs Pascal has to Random(), so use this hack }

function Random : longint;
begin
    randSeed := randSeed div 2;
    randSeed := randSeed * 3079 + 73;
    Random := ABS( randSeed );
end;

begin { tap }
    writeln( 'tap starting, should tend towards 1.2020569031595942854...' );

    writeln( 'first implementation...' );
    first_implementation;

    writeln( 'second implementation...' );
    randSeed := 1000000007; { arbitrary }
    loops := 100000;
    total := 0;
    prev := 1;

    for i := 1 to loops do begin
        a := Random;
        b := Random;
        c := Random;

        greatest := gcd( a, gcd( b, c ) );
        if ( 1 = greatest ) then total := total + 1;
        if ( i = ( prev * 10 ) ) then begin
            prev := i;
            dtotal := total;
            v := i / dtotal;
            writeln( '  at ', i, ' iterations: ', v );
        end;
    end;

    writeln( 'tap completed with great success' );
end. { tap }

