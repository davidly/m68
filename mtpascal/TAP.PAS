program tap;

var
    rsf, greatest, a, b, c : integer;
    loops, prev, i, total : integer;
    v, ri, rtotal : real;
    randSeed : integer;

function gcd( m : integer; n : integer ) : integer;
var
    a, b, r : integer;
begin { gcd }
     a := 0;
     if ( m > n ) then begin
         b := m;
         r := n;
     end
     else begin
         b := n;
         r := m;
     end;

    while ( 0 <> r ) do begin
        a := b;
        b := r;
        r := a MOD b;
    end;

    gcd := b;
end; { gcd }

procedure first_implementation;
var
    total, i, prev : integer;
    sofar, ri, iq : real;
begin
    total := 10000;
    sofar := 0.0;
    prev := 1;

    for i := 1 to total do begin
        ri := i;
        iq := ri * ri * ri;
        sofar := sofar + ( 1.0 / iq );
        if ( i = ( prev * 10 ) ) then begin
            prev := i;
            write( '  at ', i );
            writeln( ' iterations: ', sofar );
        end;
    end;
end;

function Random : integer;
begin
    randSeed := SHR( randSeed, 2 );
    randSeed := randSeed * 3079 + 73;
    Random := ABS( randSeed );
end;

begin { tap }
    writeln( 'tap starting, should tend towards 1.2020569031595942854...' );

    writeln( 'first implementation...' );
    first_implementation;

    {
      this implementation doesn't work well because Random() isn't very good and because
      it uses integer, not longint for the values. I can't figure out how to convert
      from longint to real. And MOD doesn't work correctly with longint in gcd().
    }

    writeln( 'second implementation...' );
    randSeed := 13;
    loops := 10000;
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
            rtotal := total;
            ri := i;
            v := ri / rtotal;
            writeln( '  at ', i, ' iterations: ', v );
        end;
    end;

    writeln( 'tap completed with great success' );
end. { tap }

