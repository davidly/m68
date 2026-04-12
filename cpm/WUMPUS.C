/*
 * Hunt the Wumpus
 * K&R-style version for older MS-DOS C compilers.
 *
 * Usage:
 *   WUMPUS
 *   WUMPUS -c
 */

#include <stdio.h>
#include <ctype.h>

#define ROOM_COUNT        20
#define TUNNELS_PER_ROOM   3
#define LOCATION_COUNT     6
#define MAX_ARROW_STEPS    5
#define INITIAL_ARROWS     5

#define PLAYER 0
#define WUMPUS 1
#define PIT_1  2
#define PIT_2  3
#define BAT_1  4
#define BAT_2  5

#define TURN_MOVE   0
#define TURN_SHOOT  1

#define OC_CONTINUE 0
#define OC_WIN      1
#define OC_LOSE     2

typedef struct GameState {
    int locations[LOCATION_COUNT];
    int initial_locations[LOCATION_COUNT];
    int arrows;
    int cp_player;
} GameState;

static int cave[ROOM_COUNT + 1][TUNNELS_PER_ROOM] = {
    {0, 0, 0},
    {2, 5, 8},
    {1, 3, 10},
    {2, 4, 12},
    {3, 5, 14},
    {1, 4, 6},
    {5, 7, 15},
    {6, 8, 17},
    {1, 7, 9},
    {8, 10, 18},
    {2, 9, 11},
    {10, 12, 19},
    {3, 11, 13},
    {12, 14, 20},
    {4, 13, 15},
    {6, 14, 16},
    {15, 17, 20},
    {7, 16, 18},
    {9, 17, 19},
    {11, 18, 20},
    {13, 16, 19}
};

static int r_room();
static int r_index();
static void read_line();
static int read_nonspace_char();
static int read_int();
static int rooms_are_adjacent();
static int any_duplicate_locations();
static int rhas_pit();
static int rhas_bat();
static int rhas_wumpus();
static int room_is_safe_now();
static void print_instructions();
static void initialize_game();
static void restore_same_setup();
static void print_warnings();
static void print_status();
static int wake_wumpus();
static int resolve_player_room();
static int pr_turn_action();
static int dom_to();
static int dom();
static int pr_ar_length();
static void pr_ar_path();
static int dosh_path();
static int dosh();
static int pr_instructions();
static int pr_same_setup();
static int ch_r_adjacent_room();
static int ch_safe_adjacent_room();
static int find_adjacent_wumpus();
static int cp_ch_action();
static int cp_take_turn();
static int parse_args();
static void prompt();
static void flush_out();

char * x_strchr( str, c ) char * str; char c;
{
    while ( *str )
    {
        if ( *str == c )
            return str; 
        str++;
    }

    return 0;
}

char * x_memcpy( a, b, c ) char * a; char * b; int c;
{
    char * dst = a;
    while ( c > 0 )
    {
        *a++ = *b++;
        c--;
    }
    return dst;
}

static void flush_out()
{
    fflush(stdout);
}

static void prompt(s)
char *s;
{
    printf("%s", s);
    fflush(stdout);
}

static int r_room()
{
    return (rand() % ROOM_COUNT) + 1;
}

static int r_index(count)
int count;
{
    return rand() % count;
}

static void read_line(buffer, size)
char *buffer;
int size;
{
    char *p;
    int len;

    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    p = x_strchr(buffer, '\r');
    if (p != NULL) {
        *p = '\0';
    }
}

static int read_nonspace_char()
{
    char buffer[128];
    unsigned char ch;
    int i;

    read_line(buffer, sizeof(buffer));

    for (i = 0; buffer[i] != '\0'; ++i) {
        ch = (unsigned char)buffer[i];
        if (!isspace(ch)) {
            return toupper(ch);
        }
    }

    return '\0';
}

static int read_int()
{
    char buffer[128];
    char *end;
    int value;

    read_line(buffer, sizeof(buffer));
    sscanf( buffer, "%d", &value);
    return value;
}

static int rooms_are_adjacent(from, to)
int from;
int to;
{
    int i;

    for (i = 0; i < TUNNELS_PER_ROOM; ++i) {
        if (cave[from][i] == to) {
            return TRUE;
        }
    }

    return FALSE;
}

static int any_duplicate_locations(locations, count)
int *locations;
int count;
{
    int i;
    int j;

    for (i = 0; i < count; ++i) {
        for (j = i + 1; j < count; ++j) {
            if (locations[i] == locations[j]) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static int rhas_pit(game, room)
GameState *game;
int room;
{
    return room == game->locations[PIT_1] || room == game->locations[PIT_2];
}

static int rhas_bat(game, room)
GameState *game;
int room;
{
    return room == game->locations[BAT_1] || room == game->locations[BAT_2];
}

static int rhas_wumpus(game, room)
GameState *game;
int room;
{
    return room == game->locations[WUMPUS];
}

static int room_is_safe_now(game, room)
GameState *game;
int room;
{
    return !rhas_pit(game, room) &&
           !rhas_bat(game, room) &&
           !rhas_wumpus(game, room);
}

static void print_instructions()
{
    printf("WELCOME TO 'HUNT THE WUMPUS'\n");
    printf("THE WUMPUS LIVES IN A CAVE OF 20 ROOMS. EACH ROOM\n");
    printf("HAS 3 TUNNELS LEADING TO OTHER ROOMS.\n");
    printf("\n");
    printf("HAZARDS:\n");
    printf("  BOTTOMLESS PITS - TWO ROOMS HAVE BOTTOMLESS PITS IN THEM.\n");
    printf("      IF YOU GO THERE, YOU FALL IN AND LOSE.\n");
    printf("  SUPER BATS - TWO OTHER ROOMS HAVE SUPER BATS. IF YOU\n");
    printf("      GO THERE, A BAT GRABS YOU AND TAKES YOU TO A RANDOM ROOM.\n");
    printf("\n");
    printf("WUMPUS:\n");
    printf("  THE WUMPUS IS NOT BOTHERED BY HAZARDS.\n");
    printf("  USUALLY HE IS ASLEEP.\n");
    printf("  TWO THINGS WAKE HIM UP: ENTERING HIS ROOM OR SHOOTING AN ARROW.\n");
    printf("  WHEN HE WAKES, HE MAY MOVE TO AN ADJACENT ROOM OR STAY PUT.\n");
    printf("  IF HE ENDS UP IN YOUR ROOM, YOU LOSE.\n");
    printf("\n");
    printf("YOU:\n");
    printf("  EACH TURN YOU MAY MOVE OR SHOOT A CROOKED ARROW.\n");
    printf("  YOU HAVE 5 ARROWS.\n");
    printf("  EACH ARROW CAN TRAVEL THROUGH 1 TO 5 ROOMS.\n");
    printf("  IF THE ARROW CAN'T GO THE WAY YOU AIMED, IT TAKES A RANDOM TUNNEL.\n");
    printf("  IF THE ARROW HITS THE WUMPUS, YOU WIN.\n");
    printf("  IF THE ARROW HITS YOU, YOU LOSE.\n");
    printf("\n");
    printf("WARNINGS:\n");
    printf("  WUMPUS - \"I SMELL A WUMPUS\"\n");
    printf("  BATS   - \"BATS NEARBY\"\n");
    printf("  PIT    - \"I FEEL A DRAFT\"\n");
    printf("\n");
    flush_out();
}

static void initialize_game(game)
GameState *game;
{
    int i;

    do {
        for (i = 0; i < LOCATION_COUNT; ++i) {
            game->locations[i] = r_room();
        }
    } while (any_duplicate_locations(game->locations, LOCATION_COUNT));

    x_memcpy(game->initial_locations, game->locations, sizeof(game->locations));
    game->arrows = INITIAL_ARROWS;
}

static void restore_same_setup(game)
GameState *game;
{
    x_memcpy(game->locations, game->initial_locations, sizeof(game->locations));
    game->arrows = INITIAL_ARROWS;
}

static void print_warnings(game)
GameState *game;
{
    int player_room;
    int i;
    int adjacent;

    player_room = game->locations[PLAYER];

    for (i = 0; i < TUNNELS_PER_ROOM; ++i) {
        adjacent = cave[player_room][i];

        if (adjacent == game->locations[WUMPUS]) {
            printf("I SMELL A WUMPUS!\n");
        }
        if (adjacent == game->locations[PIT_1] ||
            adjacent == game->locations[PIT_2]) {
            printf("I FEEL A DRAFT!\n");
        }
        if (adjacent == game->locations[BAT_1] ||
            adjacent == game->locations[BAT_2]) {
            printf("BATS NEARBY!\n");
        }
    }
}

static void print_status(game)
GameState *game;
{
    int player_room;

    player_room = game->locations[PLAYER];

    printf("\n");
    print_warnings(game);
    printf("YOU ARE IN ROOM %d\n", player_room);
    printf("TUNNELS LEAD TO %d, %d, AND %d\n",
           cave[player_room][0],
           cave[player_room][1],
           cave[player_room][2]);
    printf("ARROWS LEFT: %d\n", game->arrows);
    printf("\n");
    flush_out();
}

static int wake_wumpus(game)
GameState *game;
{
    int move;
    int wumpus_room;

    move = rand() % 4;

    if (move < 3) {
        wumpus_room = game->locations[WUMPUS];
        game->locations[WUMPUS] = cave[wumpus_room][move];
    }

    if (game->locations[WUMPUS] == game->locations[PLAYER]) {
        printf("TSK TSK TSK - WUMPUS GOT YOU!\n");
        flush_out();
        return OC_LOSE;
    }

    return OC_CONTINUE;
}

static int resolve_player_room(game)
GameState *game;
{
    int player_room;

    for (;;) {
        player_room = game->locations[PLAYER];

        if (player_room == game->locations[WUMPUS]) {
            printf("... OOPS! BUMPED A WUMPUS!\n");
            flush_out();
            return wake_wumpus(game);
        }

        if (player_room == game->locations[PIT_1] ||
            player_room == game->locations[PIT_2]) {
            printf("YYYIIIIEEEE . . . FELL IN PIT\n");
            flush_out();
            return OC_LOSE;
        }

        if (player_room == game->locations[BAT_1] ||
            player_room == game->locations[BAT_2]) {
            printf("ZAP--SUPER BAT SNATCH! ELSEWHEREVILLE FOR YOU!\n");
            flush_out();
            game->locations[PLAYER] = r_room();
            continue;
        }

        return OC_CONTINUE;
    }
}

static int pr_turn_action()
{
    int ch;

    for (;;) {
        prompt("SHOOT OR MOVE (S-M) ");
        ch = read_nonspace_char();
        if (ch == 'S') {
            return TURN_SHOOT;
        }
        if (ch == 'M') {
            return TURN_MOVE;
        }
    }
}

static int dom_to(game, destination)
GameState *game;
int destination;
{
    game->locations[PLAYER] = destination;
    return resolve_player_room(game);
}

static int dom(game)
GameState *game;
{
    int destination;

    for (;;) {
        prompt("WHERE TO ");
        destination = read_int();

        if (destination < 1 || destination > ROOM_COUNT) {
            printf("NOT POSSIBLE -\n");
            flush_out();
            continue;
        }

        if (!rooms_are_adjacent(game->locations[PLAYER], destination)) {
            printf("NOT POSSIBLE -\n");
            flush_out();
            continue;
        }

        break;
    }

    return dom_to(game, destination);
}

static int pr_ar_length()
{
    int length;

    for (;;) {
        prompt("NO. OF ROOMS (1-5) ");
        length = read_int();
        if (length >= 1 && length <= MAX_ARROW_STEPS) {
            return length;
        }
    }
}

static void pr_ar_path(path, length)
int *path;
int length;
{
    int i;

    for (i = 0; i < length; ++i) {
        for (;;) {
            printf("ROOM #%d ", i + 1);
            flush_out();
            path[i] = read_int();

            if (path[i] < 1 || path[i] > ROOM_COUNT) {
                continue;
            }

            if (i >= 2 && path[i] == path[i - 2]) {
                printf("ARROWS AREN'T THAT CROOKED - TRY ANOTHER ROOM\n");
                flush_out();
                continue;
            }

            break;
        }
    }
}

static int dosh_path(game, path, path_length)
GameState *game;
int *path;
int path_length;
{
    int arrow_room;
    int i;

    arrow_room = game->locations[PLAYER];

    for (i = 0; i < path_length; ++i) {
        if (rooms_are_adjacent(arrow_room, path[i])) {
            arrow_room = path[i];
        } else {
            arrow_room = cave[arrow_room][r_index(TUNNELS_PER_ROOM)];
        }

        if (arrow_room == game->locations[WUMPUS]) {
            printf("AHA! YOU GOT THE WUMPUS!\n");
            flush_out();
            return OC_WIN;
        }

        if (arrow_room == game->locations[PLAYER]) {
            printf("OUCH! ARROW GOT YOU!\n");
            game->arrows--;
            flush_out();
            return OC_LOSE;
        }
    }

    game->arrows--;
    printf("MISSED\n");
    flush_out();

    if (game->arrows <= 0) {
        printf("YOU RAN OUT OF ARROWS!\n");
        flush_out();
        return OC_LOSE;
    }

    return wake_wumpus(game);
}

static int dosh(game)
GameState *game;
{
    int path[MAX_ARROW_STEPS];
    int path_length;

    path_length = pr_ar_length();
    pr_ar_path(path, path_length);
    return dosh_path(game, path, path_length);
}

static int pr_instructions()
{
    prompt("INSTRUCTIONS (Y-N) ");
    return read_nonspace_char() != 'N';
}

static int pr_same_setup()
{
    prompt("SAME SET-UP (Y-N) ");
    return read_nonspace_char() == 'Y';
}

static int ch_r_adjacent_room(game)
GameState *game;
{
    int player_room;

    player_room = game->locations[PLAYER];
    return cave[player_room][r_index(TUNNELS_PER_ROOM)];
}

static int ch_safe_adjacent_room(game)
GameState *game;
{
    int player_room;
    int safe_rooms[TUNNELS_PER_ROOM];
    int safe_count;
    int i;
    int room;

    player_room = game->locations[PLAYER];
    safe_count = 0;

    for (i = 0; i < TUNNELS_PER_ROOM; ++i) {
        room = cave[player_room][i];
        if (room_is_safe_now(game, room)) {
            safe_rooms[safe_count++] = room;
        }
    }

    if (safe_count == 0) {
        return 0;
    }

    return safe_rooms[r_index(safe_count)];
}

static int find_adjacent_wumpus(game, room_out)
GameState *game;
int *room_out;
{
    int player_room;
    int i;
    int room;

    player_room = game->locations[PLAYER];

    for (i = 0; i < TUNNELS_PER_ROOM; ++i) {
        room = cave[player_room][i];
        if (room == game->locations[WUMPUS]) {
            *room_out = room;
            return TRUE;
        }
    }

    return FALSE;
}

static int cp_ch_action(game)
GameState *game;
{
    int wumpus_room;

    if (game->arrows > 0 && find_adjacent_wumpus(game, &wumpus_room)) {
        return TURN_SHOOT;
    }

    return TURN_MOVE;
}

static int cp_take_turn(game)
GameState *game;
{
    int action;
    int target_room;
    int path[1];
    int destination;

    action = cp_ch_action(game);

    if (action == TURN_SHOOT) {
        target_room = 0;
        find_adjacent_wumpus(game, &target_room);
        path[0] = target_room;

        printf("COMPUTER CHOOSES: SHOOT\n");
        printf("COMPUTER SHOOTS THROUGH ROOM %d\n", target_room);
        flush_out();

        return dosh_path(game, path, 1);
    } else {
        destination = ch_safe_adjacent_room(game);
        if (destination == 0) {
            destination = ch_r_adjacent_room(game);
        }

        printf("COMPUTER CHOOSES: MOVE\n");
        printf("COMPUTER MOVES TO ROOM %d\n", destination);
        flush_out();

        return dom_to(game, destination);
    }
}

static int parse_args(argc, argv, cp_player)
int argc;
char **argv;
int *cp_player;
{
    int i;

    *cp_player = FALSE;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            *cp_player = TRUE;
        } else {
            printf("usage: %s [-c]\n", argv[0]);
            flush_out();
            return FALSE;
        }
    }

    return TRUE;
}

int main(argc, argv)
int argc;
char **argv;
{
    GameState game;
    int outcome;
    int action;

    setbuf(stdout, (char *)0);

    if (!parse_args(argc, argv, &game.cp_player)) {
        return 1;
    }

//    srand((unsigned)time((time_t *)0));

    printf("            WUMPUS\n");
    printf(" CREATIVE COMPUTING MORRISTOWN, NJ\n");
    printf("\n");
    flush_out();

    if (!game.cp_player && pr_instructions()) {
        print_instructions();
    }

    initialize_game(&game);

    for (;;) {
        outcome = OC_CONTINUE;

        printf("HUNT THE WUMPUS\n");
        flush_out();

        while (outcome == OC_CONTINUE) {
            print_status(&game);

            if (game.cp_player) {
                outcome = cp_take_turn(&game);
            } else {
                action = pr_turn_action();
                if (action == TURN_MOVE) {
                    outcome = dom(&game);
                } else {
                    outcome = dosh(&game);
                }
            }
        }

        if (outcome == OC_WIN) {
            printf("HEE HEE HEE - THE WUMPUS'LL GETCHA NEXT TIME!!\n");
        } else {
            printf("HA HA HA - YOU LOSE!\n");
        }
        flush_out();

        if (game.cp_player) {
            break;
        }

        if (pr_same_setup()) {
            restore_same_setup(&game);
        } else {
            initialize_game(&game);
        }
    }

    return 0;
}
