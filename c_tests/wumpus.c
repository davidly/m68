/*
 * Hunt the Wumpus
 * Idiomatic modern C version based on the Apple 1 BASIC program.
 *
 * Build:
 *   cc -O2 -Wall -Wextra -pedantic -std=c11 wumpus.c -o wumpus
 *
 * Usage:
 *   ./wumpus
 *   ./wumpus -c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

enum {
    ROOM_COUNT = 20,
    TUNNELS_PER_ROOM = 3,
    LOCATION_COUNT = 6,
    MAX_ARROW_STEPS = 5,
    INITIAL_ARROWS = 5
};

typedef enum {
    PLAYER = 0,
    WUMPUS = 1,
    PIT_1 = 2,
    PIT_2 = 3,
    BAT_1 = 4,
    BAT_2 = 5
} Entity;

typedef enum {
    TURN_MOVE,
    TURN_SHOOT
} TurnAction;

typedef enum {
    OUTCOME_CONTINUE,
    OUTCOME_WIN,
    OUTCOME_LOSE
} Outcome;

typedef struct {
    int locations[LOCATION_COUNT];
    int initial_locations[LOCATION_COUNT];
    int arrows;
    bool computer_player;
} GameState;

/* Room graph: rooms are numbered 1..20 */
static const int cave[ROOM_COUNT + 1][TUNNELS_PER_ROOM] = {
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

static int random_room(void)
{
    return (rand() % ROOM_COUNT) + 1;
}

static int random_index(int count)
{
    return rand() % count;
}

static void read_line(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
}

static char read_nonspace_char(void)
{
    char buffer[128];
    read_line(buffer, sizeof(buffer));

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        unsigned char ch = (unsigned char)buffer[i];
        if (!isspace(ch)) {
            return (char)toupper(ch);
        }
    }
    return '\0';
}

static int read_int(void)
{
    char buffer[128];
    char *end;
    long value;

    for (;;) {
        read_line(buffer, sizeof(buffer));
        value = strtol(buffer, &end, 10);
        if (end != buffer) {
            return (int)value;
        }
        printf("? ");
    }
}

static bool rooms_are_adjacent(int from, int to)
{
    for (int i = 0; i < TUNNELS_PER_ROOM; ++i) {
        if (cave[from][i] == to) {
            return true;
        }
    }
    return false;
}

static bool any_duplicate_locations(const int *locations, int count)
{
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (locations[i] == locations[j]) {
                return true;
            }
        }
    }
    return false;
}

static bool room_has_pit(const GameState *game, int room)
{
    return room == game->locations[PIT_1] || room == game->locations[PIT_2];
}

static bool room_has_bat(const GameState *game, int room)
{
    return room == game->locations[BAT_1] || room == game->locations[BAT_2];
}

static bool room_has_wumpus(const GameState *game, int room)
{
    return room == game->locations[WUMPUS];
}

static bool room_is_safe_now(const GameState *game, int room)
{
    return !room_has_pit(game, room) &&
           !room_has_bat(game, room) &&
           !room_has_wumpus(game, room);
}

static void print_instructions(void)
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
}

static void initialize_game(GameState *game)
{
    do {
        for (int i = 0; i < LOCATION_COUNT; ++i) {
            game->locations[i] = random_room();
        }
    } while (any_duplicate_locations(game->locations, LOCATION_COUNT));

    memcpy(game->initial_locations, game->locations, sizeof(game->locations));
    game->arrows = INITIAL_ARROWS;
}

static void restore_same_setup(GameState *game)
{
    memcpy(game->locations, game->initial_locations, sizeof(game->locations));
    game->arrows = INITIAL_ARROWS;
}

static void print_warnings(const GameState *game)
{
    int player_room = game->locations[PLAYER];

    for (int i = 0; i < TUNNELS_PER_ROOM; ++i) {
        int adjacent = cave[player_room][i];

        if (adjacent == game->locations[WUMPUS]) {
            printf("I SMELL A WUMPUS!\n");
        }
        if (adjacent == game->locations[PIT_1] || adjacent == game->locations[PIT_2]) {
            printf("I FEEL A DRAFT!\n");
        }
        if (adjacent == game->locations[BAT_1] || adjacent == game->locations[BAT_2]) {
            printf("BATS NEARBY!\n");
        }
    }
}

static void print_status(const GameState *game)
{
    int player_room = game->locations[PLAYER];

    printf("\n");
    print_warnings(game);
    printf("YOU ARE IN ROOM %d\n", player_room);
    printf("TUNNELS LEAD TO %d, %d, AND %d\n",
           cave[player_room][0],
           cave[player_room][1],
           cave[player_room][2]);
    printf("ARROWS LEFT: %d\n", game->arrows);
    printf("\n");
}

static Outcome wake_wumpus(GameState *game)
{
    int move = rand() % 4; /* 0,1,2 => move through that tunnel; 3 => stay */

    if (move < 3) {
        int wumpus_room = game->locations[WUMPUS];
        game->locations[WUMPUS] = cave[wumpus_room][move];
    }

    if (game->locations[WUMPUS] == game->locations[PLAYER]) {
        printf("TSK TSK TSK - WUMPUS GOT YOU!\n");
        return OUTCOME_LOSE;
    }

    return OUTCOME_CONTINUE;
}

static Outcome resolve_player_room(GameState *game)
{
    for (;;) {
        int player_room = game->locations[PLAYER];

        if (player_room == game->locations[WUMPUS]) {
            printf("... OOPS! BUMPED A WUMPUS!\n");
            return wake_wumpus(game);
        }

        if (player_room == game->locations[PIT_1] ||
            player_room == game->locations[PIT_2]) {
            printf("YYYIIIIEEEE . . . FELL IN PIT\n");
            return OUTCOME_LOSE;
        }

        if (player_room == game->locations[BAT_1] ||
            player_room == game->locations[BAT_2]) {
            printf("ZAP--SUPER BAT SNATCH! ELSEWHEREVILLE FOR YOU!\n");
            game->locations[PLAYER] = random_room();
            continue;
        }

        return OUTCOME_CONTINUE;
    }
}

static TurnAction prompt_turn_action(void)
{
    for (;;) {
        printf("SHOOT OR MOVE (S-M) ");
        char ch = read_nonspace_char();
        if (ch == 'S') {
            return TURN_SHOOT;
        }
        if (ch == 'M') {
            return TURN_MOVE;
        }
    }
}

static Outcome do_move_to(GameState *game, int destination)
{
    game->locations[PLAYER] = destination;
    return resolve_player_room(game);
}

static Outcome do_move(GameState *game)
{
    int destination;

    for (;;) {
        printf("WHERE TO ");
        destination = read_int();

        if (destination < 1 || destination > ROOM_COUNT) {
            printf("NOT POSSIBLE -\n");
            continue;
        }

        if (!rooms_are_adjacent(game->locations[PLAYER], destination)) {
            printf("NOT POSSIBLE -\n");
            continue;
        }

        break;
    }

    return do_move_to(game, destination);
}

static int prompt_arrow_length(void)
{
    for (;;) {
        printf("NO. OF ROOMS (1-5) ");
        int length = read_int();
        if (length >= 1 && length <= MAX_ARROW_STEPS) {
            return length;
        }
    }
}

static void prompt_arrow_path(int *path, int length)
{
    for (int i = 0; i < length; ++i) {
        for (;;) {
            printf("ROOM #%d ", i + 1);
            path[i] = read_int();

            if (path[i] < 1 || path[i] > ROOM_COUNT) {
                continue;
            }

            if (i >= 2 && path[i] == path[i - 2]) {
                printf("ARROWS AREN'T THAT CROOKED - TRY ANOTHER ROOM\n");
                continue;
            }

            break;
        }
    }
}

static Outcome do_shoot_path(GameState *game, const int *path, int path_length)
{
    int arrow_room = game->locations[PLAYER];

    for (int i = 0; i < path_length; ++i) {
        if (rooms_are_adjacent(arrow_room, path[i])) {
            arrow_room = path[i];
        } else {
            arrow_room = cave[arrow_room][random_index(TUNNELS_PER_ROOM)];
        }

        if (arrow_room == game->locations[WUMPUS]) {
            printf("AHA! YOU GOT THE WUMPUS!\n");
            return OUTCOME_WIN;
        }

        if (arrow_room == game->locations[PLAYER]) {
            printf("OUCH! ARROW GOT YOU!\n");
            game->arrows--;
            return OUTCOME_LOSE;
        }
    }

    game->arrows--;
    printf("MISSED\n");

    if (game->arrows <= 0) {
        printf("YOU RAN OUT OF ARROWS!\n");
        return OUTCOME_LOSE;
    }

    return wake_wumpus(game);
}

static Outcome do_shoot(GameState *game)
{
    int path[MAX_ARROW_STEPS];
    int path_length = prompt_arrow_length();

    prompt_arrow_path(path, path_length);
    return do_shoot_path(game, path, path_length);
}

static bool prompt_instructions(void)
{
    printf("INSTRUCTIONS (Y-N) ");
    return read_nonspace_char() != 'N';
}

static bool prompt_same_setup(void)
{
    printf("SAME SET-UP (Y-N) ");
    return read_nonspace_char() == 'Y';
}

static int choose_random_adjacent_room(const GameState *game)
{
    int player_room = game->locations[PLAYER];
    return cave[player_room][random_index(TUNNELS_PER_ROOM)];
}

static int choose_safe_adjacent_room(const GameState *game)
{
    int player_room = game->locations[PLAYER];
    int safe_rooms[TUNNELS_PER_ROOM];
    int safe_count = 0;

    for (int i = 0; i < TUNNELS_PER_ROOM; ++i) {
        int room = cave[player_room][i];
        if (room_is_safe_now(game, room)) {
            safe_rooms[safe_count++] = room;
        }
    }

    if (safe_count == 0) {
        return 0;
    }

    return safe_rooms[random_index(safe_count)];
}

static bool find_adjacent_wumpus(const GameState *game, int *room_out)
{
    int player_room = game->locations[PLAYER];

    for (int i = 0; i < TUNNELS_PER_ROOM; ++i) {
        int room = cave[player_room][i];
        if (room == game->locations[WUMPUS]) {
            *room_out = room;
            return true;
        }
    }

    return false;
}

static TurnAction computer_choose_action(const GameState *game)
{
    int wumpus_room;
    if (game->arrows > 0 && find_adjacent_wumpus(game, &wumpus_room)) {
        (void)wumpus_room;
        return TURN_SHOOT;
    }
    return TURN_MOVE;
}

static Outcome computer_take_turn(GameState *game)
{
    int player_room = game->locations[PLAYER];
    TurnAction action = computer_choose_action(game);

    if (action == TURN_SHOOT) {
        int target_room = 0;
        int path[1];

        (void)player_room;
        find_adjacent_wumpus(game, &target_room);
        path[0] = target_room;

        printf("COMPUTER CHOOSES: SHOOT\n");
        printf("COMPUTER SHOOTS THROUGH ROOM %d\n", target_room);

        return do_shoot_path(game, path, 1);
    } else {
        int destination = choose_safe_adjacent_room(game);
        if (destination == 0) {
            destination = choose_random_adjacent_room(game);
        }

        printf("COMPUTER CHOOSES: MOVE\n");
        printf("COMPUTER MOVES TO ROOM %d\n", destination);

        return do_move_to(game, destination);
    }
}

static bool parse_args(int argc, char **argv, bool *computer_player)
{
    *computer_player = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            *computer_player = true;
        } else {
            fprintf(stderr, "usage: %s [-c]\n", argv[0]);
            return false;
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    GameState game;

    if (!parse_args(argc, argv, &game.computer_player)) {
        return 1;
    }

    srand((unsigned)time(NULL));

    printf("            WUMPUS\n");
    printf(" CREATIVE COMPUTING MORRISTOWN, NJ\n");
    printf("\n");

    if (!game.computer_player && prompt_instructions()) {
        print_instructions();
    }

    initialize_game(&game);

    for (;;) {
        Outcome outcome = OUTCOME_CONTINUE;

        printf("HUNT THE WUMPUS\n");

        while (outcome == OUTCOME_CONTINUE) {
            print_status(&game);

            if (game.computer_player) {
                outcome = computer_take_turn(&game);
            } else {
                TurnAction action = prompt_turn_action();
                if (action == TURN_MOVE) {
                    outcome = do_move(&game);
                } else {
                    outcome = do_shoot(&game);
                }
            }
        }

        if (outcome == OUTCOME_WIN) {
            printf("HEE HEE HEE - THE WUMPUS'LL GETCHA NEXT TIME!!\n");
        } else {
            printf("HA HA HA - YOU LOSE!\n");
        }

        if (game.computer_player) {
            break;
        }

        if (prompt_same_setup()) {
            restore_same_setup(&game);
        } else {
            initialize_game(&game);
        }
    }

    return 0;
}
