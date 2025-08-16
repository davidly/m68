{
  nqueens.pas

  A simple implementation of the N-Queens problem using backtracking.
  This program counts the number of solutions for placing N queens on an
  N x N chessboard such that no two queens threaten each other.
  Mirror and flip optimizations are not applied.

    n   fundamental     all
    1   1               1
    2   0               0
    3   0               0
    4   1               2
    5   2               10
    6   1               4
    7   6               40
    8   12              92
    9   46              352
    10  92              724
    11  341             2,680
    12  1,787           14,200
    13  9,233           73,712
    14  45,752          365,596
    15  285,053         2,279,184
    16  1,846,955       14,772,512
}

{ turn off range checking in svs pascal. it's off by default in mt pascal }
{$R-}
{ turn off exception checking for mt pascal. no impact for svs pascal}
{$X- }

program nqueens;

const 
    LIMIT = 8;
    LIMIT_MINUS_1 = 7;

type
    boardType = array[ 0..LIMIT_MINUS_1, 0..LIMIT_MINUS_1 ] of boolean;

var
    solutions, size, ir, ic : integer;
    board : boardType;

function isSafe( row : integer; col : integer; n : integer ) : boolean;
var
    c, r : integer;
    safe : boolean;
begin
    safe := true;
    c := col - 1;
    while ( c >= 0 ) and safe do
    begin
        if ( board[ row, c ] ) then safe := false;
        c := c - 1;
    end;

    if ( safe ) then
    begin
        r := row - 1;
        c := col - 1;
        while safe and ( r >= 0 ) and ( c >= 0 ) do
        begin
            if ( board[r][c] ) then safe := false;
            r := r - 1;
            c := c - 1;
        end;
        
        if ( safe ) then
        begin
            r := row + 1;
            c := col - 1;
            while safe and ( r < n ) and ( c >= 0 ) do
            begin
                if ( board[r][c] ) then safe := false;
                r := r + 1;
                c := c - 1;
            end;
        end;
    end;

    isSafe := safe;
end;

procedure solve( col : integer; n : integer );
var
    r : integer;
begin
    if ( col = n ) then
        solutions := solutions + 1
    else
    begin
        for r := 0 to n - 1 do
        begin
            if ( isSafe( r, col, n ) ) then
            begin
                board[r,col] := true;
                solve( col + 1, n );
                board[r,col] := false;
            end
        end;
    end;
end;

begin
    for ir := 0 to LIMIT_MINUS_1 do
        for ic := 0 to LIMIT_MINUS_1 do
            board[ ir, ic ] := false;

    writeln( 'size    solutions' );
    for size := 1 to LIMIT do
    begin
        solutions := 0;
        solve( 0, size );
        writeln( size:4, '   ', solutions:10 );
    end;
end.
