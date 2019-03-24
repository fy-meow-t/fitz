#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Exit status */
#define EXIT_ARG 1
#define EXIT_ACCESS_TILE 2
#define EXIT_TILE_CONTENTS 3
#define EXIT_PLAYER 4
#define EXIT_DIMS 5
#define EXIT_ACCESS_SAVE 6
#define EXIT_SAVE_CONTENTS 7
#define EXIT_END_INPUT 10

#define MIDDLE 2 // The middle (@) of the 2D array tile is at tile[2][2]
#define TILE_SIZE 5 // Tile size: each tile is described as a 5*5 grid.
#define MAX 70 // Initial size of a dynamic char array
#define AUTO_1 1
#define AUTO_2 2
#define HUMAN 3
#define FIRST_TYPE '*'
#define SECOND_TYPE '#'
#define BOARD_EMPTY '.'
#define TILE_EMPTY ','
#define TILE_EXIST '!'
#define FIRST_PLAYER 0
#define SECOND_PLAYER 1

/* A tile with its rotations */
typedef struct {
    char** rotate0;
    char** rotate90;
    char** rotate180;
    char** rotate270;
} Tile;

/* Collection of all tiles */
typedef struct {
    int size; // Total number of tiles
    int current; // Current tile (starting at 0)
    Tile* allTiles;
} AllTiles;

/* The board to place tiles on */
typedef struct {
    int height;
    int width;
    char** grid;
} Board;

/* Player h, 1, or 2*/
typedef struct {
    int rowStart;
    int colStart;
    int validDegree;
    int type; // AUTO_1 (1), AUTO_2 (2), or HUMAN (3)
    int order; // FIRST_PLAYER (0) or SECOND_PLAYER (1)
} Player;

/*
 * Rotate a tile clockwise in 90 degrees.
 * Param: original - the non-rotated tile
 * Return: the rotated tile
 */
char** rotate_once(char** original) {
    char** result = (char**) malloc(sizeof(char*) * TILE_SIZE);
    for (int row = 0; row < TILE_SIZE; row++) {
        result[row] = (char*) malloc(sizeof(char) * TILE_SIZE);
        for (int column = 0; column < TILE_SIZE; column++) {
            result[row][column] = original[TILE_SIZE - 1 - column][row];
        }
    }
    return result;
}

/*
 * Rotate the tile in all three degrees and save the rotations
 * Param: tile - the tile to rotate
 */
void set_rotate(Tile* tile) {
    tile->rotate90 = rotate_once(tile->rotate0);
    tile->rotate180 = rotate_once(tile->rotate90);
    tile->rotate270 = rotate_once(tile->rotate180);
}

/*
 * Called by read_file(). Print error message and exit at status 3.
 */
void invalid_file(void) {
    fprintf(stderr, "Invalid tile file contents\n");
    exit(EXIT_TILE_CONTENTS);
}

/**
 * Read the tile file.
 * Param: file - the tile file
 *        tiles - the collection to store all tiles
 * Error: call invalid_file() if the tile file is incorrect
 */
void read_file(FILE* file, AllTiles* tiles) {
    tiles->allTiles = (Tile*) malloc(sizeof(Tile) * 1);
    int row = 0, column = 0, next = 0;
    tiles->size = 0;
    Tile* tile;
    while (1) {
        tiles->allTiles[tiles->size].rotate0 =
                (char**) malloc(sizeof(char*) * TILE_SIZE);
        tile = &(tiles->allTiles[tiles->size]);
        while (row < TILE_SIZE) {
            tile->rotate0[row] = (char*) malloc(sizeof(char) * TILE_SIZE);
            for (column = 0; column < 6; column++) {
                next = fgetc(file);
                if (column == TILE_SIZE && next == '\n') {
                    row++;
                } else if (column < TILE_SIZE && (next == TILE_EMPTY ||
                        next == TILE_EXIST)) {
                    tile->rotate0[row][column] = (char) next;
                } else {
                    invalid_file();
                }
            }
        }
        set_rotate(tile);
        row = 0;
        next = fgetc(file);
        tiles->size++;
        if (next == EOF) {
            return;
        } else if (next == '\n') {
            tiles->allTiles = (Tile*) realloc(tiles->allTiles,
                    sizeof(Tile) * (tiles->size + 1));
        } else {
            invalid_file();
        }
    }
}

/*
 * Print a tile.
 * Param: tile - the tile to print
 *        all - if all is 1, print all rotations. Otherwise just the original
 */
void print_tile(Tile* tile, int all) {
    for (int row = 0; row < TILE_SIZE; row++) {
        printf("%s", tile->rotate0[row]);
        if (all) {
            printf(" %s", tile->rotate90[row]);
            printf(" %s", tile->rotate180[row]);
            printf(" %s", tile->rotate270[row]);
        }
        printf("\n");
    }
}

/*
 * Print every tile with its rotations
 * Param: tiles - all tiles to print
 */
void print_all_tiles(AllTiles* tiles) {
    for (int i = 0; i < tiles->size; i++) {
        print_tile(&(tiles->allTiles[i]), 1);
        if (i < tiles->size - 1) {
            printf("\n");
        }
    }
}

/*
 * Build a new empty board
 * Param: board - the board to build the empty grid in
 */
void new_board(Board* board) {
    board->grid = (char**) malloc(sizeof(char*) * board->height);
    for (int row = 0; row < board->height; row++) {
        board->grid[row] = (char*) malloc(sizeof(char) * board->width);
        for (int column = 0; column < board->width; column++) {
            board->grid[row][column] = BOARD_EMPTY;
        }
    }
}

/*
 * Print the board.
 * Param: board - the board to print its grid
 */
void print_board(Board* board) {
    for (int i = 0; i < board->height; i++) {
        for (int j = 0; j < board->width; j++) {
            printf("%c", board->grid[i][j]);
        }
        printf("\n");
    }
}

/*
 * Check whether the placement is valid
 * Param: tile - the tile to put
 *        board - the board to place the tile on
 *        row, column - where to place the tile on the board
 * Return: 1 if it is a valid placement; 0 if it is not.
 */
int valid_place(char** tile, Board* board, int row, int column) {
    int boardRow, boardColumn;
    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
            if (tile[i][j] == TILE_EXIST) {
                // The position of '!' on the board
                boardRow = row + i - MIDDLE;
                boardColumn = column + j - MIDDLE;
                if (boardRow < 0 || boardRow >= board->height ||
                        boardColumn < 0 || boardColumn >= board->width) {
                    // Off the board
                    return 0;
                } else if (board->grid[boardRow][boardColumn] != BOARD_EMPTY) {
                    // Overlap
                    return 0;
                }
            }
        }
    }
    return 1;
}

/*
 * Place the tile.
 * Param: tile - the tile to put
 *        board - the board to place the tile on
 *        row, column - where to put the tile on the board
 *        type - the pattern of the tile. FIRST_TYPE ('*') or SECOND_TYPE ('#')
 */
void put_tile(char** tile, Board* board, int row, int column, char type) {
    int boardRow, boardColumn;
    for (int i = 0; i < TILE_SIZE; i++) {
        for (int j = 0; j < TILE_SIZE; j++) {
            if (tile[i][j] == TILE_EXIST) {
                boardRow = row + i - MIDDLE;
                boardColumn = column + j - MIDDLE;
                board->grid[boardRow][boardColumn] = type;
            }
        }
    }
}

/*
 * Read a line from user input or a file
 * Param: file - pointer to a FILE object to read
 * Return: The line got from the stream
 * Error: Exit (10) if it reaches the end of file while waiting for user input
 */
char* read_line(FILE* file) {
    int max = MAX, n = 0, next = fgetc(file);
    char* buffer = (char*) malloc(sizeof(char) * max);
    while (next != '\n') {
        if (n >= max - 1) {
            buffer = (char*) realloc(buffer, sizeof(char) * (n + 2));
            max++;
        }
        if (next == EOF) {
            fprintf(stderr, "End of input\n");
            exit(EXIT_END_INPUT);
        }
        buffer[n] = (char) next;
        n++;
        next = getc(file);
    }
    buffer[n] = '\0';
    return buffer;
}

/*
 * Get the rotated tile
 * Param: degrees - 0, 90, 180, or 270 degrees to rotate in
 *        currentTile - the original tile to rotate
 * Return: the rotated tile
 */
char** rotate_tile(int degrees, Tile* currentTile) {
    switch (degrees) {
        case 0:
            return currentTile->rotate0;
        case 90:
            return currentTile->rotate90;
        case 180:
            return currentTile->rotate180;
    }
    return currentTile->rotate270;
}

/*
 * Check whether the current player (human or auto 1) can win in this turn
 * Param: currentTile - the tile to check possible placements of
 *        board - the board to play on
 *        currentPlayer - the player who will play in this turn if they can win
 *        anotherPlayer - the other player
 *        turn - the turn counts starting at 1
 *        human - the player is human or auto type 1
 * Return: 1 if no valid placements anymore i.e. the game ends.
 *         0 if there exists a possible valid placement and game continues
 */
int game_end1(Tile* currentTile, Board* board, Player* currentPlayer,
        Player* anotherPlayer, int turn, int human) {
    int degree = 0, row, col;
    if (turn == 1) {
        row = MIDDLE * (-1), col = MIDDLE * (-1);
    } else {
        // Most recent legal move by either player i.e. the other player
        row = anotherPlayer->rowStart, col = anotherPlayer->colStart;
    }
    while (degree <= 270) {
        char** tile = rotate_tile(degree, currentTile);
        do {
            if (valid_place(tile, board, row, col) == 1) {
                if (!human) {
                    currentPlayer->rowStart = row;
                    currentPlayer->colStart = col;
                    currentPlayer->validDegree = degree;
                }
                return 0; // Valid placement exists
            }
            col++;

            if (col > board->width + 1) {
                col = MIDDLE * (-1);
                row++; // Go to check the next row
            }
            if (row > board->height + 1) {
                row = MIDDLE * (-1); // Back to the top
            }
        } while (row != anotherPlayer->rowStart ||
                col != anotherPlayer->colStart);
        degree += 90;
    }
    return 1;
}

/*
 * Check whether the current player (auto 2) can win in this turn
 * Param: currentTile - the tile to check possible placements of
 *        board - the board to play on
 *        currentPlayer - the player who will play in this turn if they can win
 * Return: 1 if no valid placements anymore i.e. the game ends.
 *         0 if there exists a possible valid placement and game continues
 */
int game_end2(Tile* currentTile, Board* board, Player* player) {
    int row = player->rowStart, col = player->colStart, degree;
    do {
        degree = 0;
        while (degree <= 270) {
            char** tile = rotate_tile(degree, currentTile);
            if (valid_place(tile, board, row, col) == 1) {
                player->rowStart = row;
                player->colStart = col;
                player->validDegree = degree;
                return 0;
            }
            degree += 90;
        }
        if (player->order == 1) {
            // The second player search from right to left, bottom to top
            col--;
            if (col < MIDDLE * (-1)) {
                col = board->width + 1;
                row--;
            }
            if (row < MIDDLE * (-1)) {
                row = board->height + 1;
            }
        } else {
            col++;
            if (col > board->width + 1) {
                col = MIDDLE * (-1);
                row++;
            }
            if (row > board->height + 1) {
                row = MIDDLE * (-1);
            }
        }
    } while (row != player->rowStart || col != player->colStart);
    return 1;
}

/*
 * An automatic player is playing. Put a tile if they can win.
 * Param: currentTile - the tile to be placed
 *        board - the board to play on
 *        currentPlayer - who will play or lose in this turn
 *        anotherPlayer - the other player
 *        turn - the turn counts starting at 1
 * Return: 1 if the game will end; 0 if it will continue
 */
int auto_turn(Tile* currentTile, Board* board, Player* currentPlayer,
        Player* anotherPlayer, int turn) {
    char type = currentPlayer->order ? SECOND_TYPE : FIRST_TYPE;
    int end = 0;
    if (currentPlayer->type == AUTO_1) {
        end = game_end1(currentTile, board, currentPlayer, anotherPlayer, turn,
                0);
    } else {
        end = game_end2(currentTile, board, currentPlayer);
    }

    if (!end) {
        // The game continues. Put the valid tile
        printf("Player %c => %d %d rotated %d\n", type,
                currentPlayer->rowStart, currentPlayer->colStart,
                currentPlayer->validDegree);
        char** tile = rotate_tile(currentPlayer->validDegree, currentTile);
        put_tile(tile, board, currentPlayer->rowStart, currentPlayer->colStart,
                type);
    }
    return end;
}

/*
 * Save the game into a file.
 * Param: path - the path of the file to save to
 *        tile - number next tile to play (starting from 0)
 *        player - the next player to have their turn (0 or 1)
 *        board - the board to play on
 * Error: print message and prompt again if there is error opening save file
 */
void save(char* path, int tile, int player, Board* board) {
    FILE* outputFile = fopen(path, "w");
    if (outputFile == NULL) {
        fprintf(stderr, "Unable to save game\n");
    } else {
        fprintf(outputFile, "%d %d %d %d\n", tile, player, board->height,
                board->width);
        for (int i = 0; i < board->height; i++) {
            for (int j = 0; j < board->width; j++) {
                fprintf(outputFile, "%c", board->grid[i][j]);
            }
            fprintf(outputFile, "\n");
        }
        fflush(outputFile);
        fclose(outputFile);
    }
}

/*
 * Get the user input from the human player
 * Param: tiles - collection of all tiles
 *        board - the board to play on
 *        player - current human player
 */
void human_turn(AllTiles* tiles, Board* board, Player* player) {
    Tile* currentTile = &(tiles->allTiles[tiles->current]);
    int loop = 1;
    char type = player->order ? SECOND_TYPE : FIRST_TYPE;
    while (loop) {
        // Keep prompting until the input is valid
        printf("Player %c] ", type);
        char* buffer = read_line(stdin);
        int num[3] = {0};
        char next;
        char firstFour[5];
        if (buffer[0] != ' ') {
            strncpy(firstFour, buffer, 4);
            firstFour[4] = '\0';

            if (strcmp(firstFour, "save") == 0) {
                // Save games
                save(buffer + 4, tiles->current, player->order, board);
            } else {
                int status = sscanf(buffer, "%d%d%d%c", &num[0], &num[1],
                        &num[2], &next);
                free(buffer);
                if (status == 3) {
                    // The input is three space separated integers
                    int row = num[0], column = num[1], degree = num[2];

                    if (degree == 0 || degree == 90 || degree == 180 ||
                            degree == 270) {
                        // Valid degree
                        char** tile = rotate_tile(degree, currentTile);
                        if (valid_place(tile, board, row, column) == 1) {
                            // Valid placement
                            put_tile(tile, board, row, column, type);
                            player->rowStart = row;
                            player->colStart = column;
                            loop = 0; // Valid input. Stop prompting
                        }
                    }
                }
            }
        }
    }
}

/*
 * A new turn. Put a tile or wait for user input if the player can win.
 * Param: tiles - collection of all tiles
 *        currentPlayer - who will play or lose in this turn
 *        anotherPlayer - the other player
 *        board - the board to play on
 *        turn - the turn counts starting at 1
 * Return: 1 if the player will lose (i.e. game will end) in this turn
 *         0 otherwise. The player can win and start playing
 */
int new_turn(AllTiles* tiles, Player* currentPlayer, Player* anotherPlayer,
        Board* board, int turn) {
    Tile* currentTile = &(tiles->allTiles[tiles->current]);
    int end = 0;
    if (currentPlayer->type == HUMAN) {
        if (!game_end1(currentTile, board, currentPlayer, currentPlayer, turn,
                1)) {
            print_tile(currentTile, 0);
            human_turn(tiles, board, currentPlayer);
        } else {
            end = 1;
        }
    } else {
        end = auto_turn(currentTile, board, currentPlayer, anotherPlayer,
                turn);
    }
    return end;
}

/*
 * Set the player's initial legal play position
 * The second player who is auto type 2 starts at the bottom right corner
 *      (board rows + 2, board columns + 2);
 * Others (human player, auto type 1, the first auto 2) starts at the top left
 *      (-2, -2)
 */
void set_start(Board* board, Player* player) {
    if (player->type == AUTO_2 && player->order == 1) {
        player->rowStart = board->height + MIDDLE;
        player->colStart = board->width + MIDDLE;
    } else {
        player->rowStart = player->colStart = MIDDLE * (-1);
    }
}

/*
 * Start a new game
 * Param: board - the board to play on
 *        tiles - collection of all tiles
 *        player1 - the first player
 *        player2 - the second player
 *        order - the player (0 or 1) who starts to play.
 *                Read from a saved file or 0 for a new game
 */
void new_game(Board* board, AllTiles* tiles, Player* player1, Player* player2,
        int order) {
    print_board(board);
    set_start(board, player1);
    set_start(board, player2);
    int turn = 1, end = 0;
    while (1) {
        end = order ? new_turn(tiles, player2, player1, board, turn) :
                new_turn(tiles, player1, player2, board, turn);
        if (end) {
            char preType = order ? FIRST_TYPE : SECOND_TYPE;
            printf("Player %c wins\n", preType);
            return;
        } else {
            print_board(board);
            tiles->current++;
            turn++;
            if (tiles->current >= tiles->size) {
                // Run out of tiles. Start again at the beginning
                tiles->current = 0;
            }
            order = order ? FIRST_PLAYER : SECOND_PLAYER;
        }
    }
}

/*
 * Load the board from the save file
 * Param: board - the board to play on
 *        file - the save file to get the board from
 * Return: 1 if successfully loaded. Return 0 otherwise
 */
int load_board(Board* board, FILE* file) {
    int row = 0, column = 0, next = 0;
    next = fgetc(file);
    while (next != EOF) {
        column = 0;
        while (next != '\n') {
            if (row < board->height && column < board->width &&
                    (next == FIRST_TYPE || next == SECOND_TYPE ||
                    next == BOARD_EMPTY)) {
                board->grid[row][column] = (char) next;
                column++;
                next = fgetc(file);
            } else {
                return 0;
            }
        }
        if (column != board->width) {
            return 0;
        }
        row++;
        next = fgetc(file);
    }
    if (row != board->height) {
        return 0;
    }
    return 1;
}

/*
 * Load the game from the save file
 * Param: filename - the filename got from the command line
 *        tiles - collection of all tiles
 *        board - the board to play on
 *        player1 - the first player
 *        player2 - the second player
 * Error: exit at status 6 if the save file can't be read
 *        exit at status 7 if the file contents are invalid
 */
void load_game(char* filename, AllTiles* tiles, Board* board, Player* player1,
        Player* player2) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Can't access save file\n");
        exit(EXIT_ACCESS_SAVE);
    }
    char* firstLine = read_line(file);
    int num[4];
    char next;
    int status = sscanf(firstLine, "%d%d%d%d%c", &num[0], &num[1], &num[2],
            &num[3], &next);
    int success = 0;
    if (status == 4) {
        int current = num[0], order = num[1];
        board->height = num[2];
        board->width = num[3];
        new_board(board);
        if (current < tiles->size && (order == 0 || order == 1) &&
                load_board(board, file) == 1) {
            success = 1;
            tiles->current = current;
            new_game(board, tiles, player1, player2, order);
        }
    }
    if (!success) {
        fprintf(stderr, "Invalid save file contents\n");
        exit(EXIT_SAVE_CONTENTS);
    }
    fclose(file);
}

/*
 * Check and decide the player type
 * Param: type - the type got from the command line
 *        player - whose type needs to be defined
 * Error: Exit at status 4 if the type is invalid i.e. not 'h', '1', nor '2'.
 */
void check_player(char* type, Player* player) {
    if (strcmp(type, "h") == 0) {
        player->type = HUMAN;
    } else if (strcmp(type, "1") == 0) {
        player->type = AUTO_1;
    } else if (strcmp(type, "2") == 0) {
        player->type = AUTO_2;
    } else {
        fprintf(stderr, "Invalid player type\n");
        exit(EXIT_PLAYER);
    }
}

/*
 * Free the dynamic array built in Tile and AllTiles
 * Param: tiles - the collection of all tiles
 */
void free_tiles(AllTiles* tiles) {
    for (int size = 0; size < tiles->size; size++) {
        for (int row = 0; row < TILE_SIZE; row++) {
            free(tiles->allTiles[size].rotate0[row]);
            free(tiles->allTiles[size].rotate90[row]);
            free(tiles->allTiles[size].rotate180[row]);
            free(tiles->allTiles[size].rotate270[row]);
        }
        free(tiles->allTiles[size].rotate0);
        free(tiles->allTiles[size].rotate90);
        free(tiles->allTiles[size].rotate180);
        free(tiles->allTiles[size].rotate270);
    }
    free(tiles->allTiles);
}

/*
 * Free the dynamic array grid in Board
 * Param: board - whose grid to be freed
 */
void free_board(Board* board) {
    for (int i = 0; i < board->height; i++) {
        free(board->grid[i]);
    }
    free(board->grid);
}

int main(int argc, char** argv) {
    // Incorrect number of arguments. Exit at status 1
    if (argc != 6 && argc != 5 && argc != 2) {
        fprintf(stderr, "Usage: fitz tilefile [p1type p2type");
        fprintf(stderr, " [height width | filename]]\n");
        exit(EXIT_ARG);
    } else {
        FILE* file = fopen(argv[1], "r");
        if (file == NULL) {
            // The file can't be read. Exit at status 2
            fprintf(stderr, "Can't access tile file\n");
            exit(EXIT_ACCESS_TILE);
        }
        AllTiles tiles;
        tiles.current = 0;
        read_file(file, &tiles);
        fclose(file);
        if (argc == 2) {
            print_all_tiles(&tiles); // Output the tile file contents
        } else {
            Player player1, player2;
            check_player(argv[2], &player1);
            player1.order = FIRST_PLAYER;
            check_player(argv[3], &player2);
            player2.order = SECOND_PLAYER;
            Board board;
            if (argc == 6) {
                // Five args. Start a new game
                float height = atof(argv[4]);
                float width = atof(argv[5]);
                if (0 < width && width < 999 && 0 < height && height < 999 &&
                        (int) height == height && (int) width == width) {
                    board.height = (int) height;
                    board.width = (int) width;
                    new_board(&board);
                    new_game(&board, &tiles, &player1, &player2, FIRST_PLAYER);
                } else {
                    // Invalid height or width (not integers between 1 and 999)
                    fprintf(stderr, "Invalid dimensions\n");
                    exit(EXIT_DIMS); // status 5
                }
            } else {
                //Four arguments. Load game from a save file
                load_game(argv[4], &tiles, &board, &player1, &player2);
            }
            free_board(&board);
        }
        free_tiles(&tiles);
    }
    return 0;
}
