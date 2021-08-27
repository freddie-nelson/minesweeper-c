#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

const int SCREEN_SIZE = 500;
const int GRID_SIZE = 25; // 0 - 256
const int MINE_ODDS = 12;

TTF_Font *FONT;

const uint8_t STATE_BLANK = 0;
const uint8_t STATE_MINE = 9;
const uint8_t STATE_FLAG = 10;

struct Tile
{
  uint8_t x;
  uint8_t y;
  uint8_t state;
  bool revealed;
};

struct Tile GRID[GRID_SIZE][GRID_SIZE];

bool isInGrid(uint8_t r, uint8_t c)
{
  return r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE;
}

void getNeighbours(uint8_t r, uint8_t c, struct Tile neighbours[8])
{
  // neighbours array layout
  // tl = 0, t = 1, tr = 2, l = 3, r = 4, bl = 5, b = 6, br = 7

  // right
  bool right = false;
  if (isInGrid(r, c + 1))
  {
    right = true;
    neighbours[4] = GRID[r][c + 1];
  }

  // left
  bool left = false;
  if (isInGrid(r, c - 1))
  {
    left = true;
    neighbours[3] = GRID[r][c - 1];
  }

  // top
  if (isInGrid(r - 1, c))
  {
    neighbours[1] = GRID[r - 1][c];

    if (right)
    {
      neighbours[2] = GRID[r - 1][c + 1];
    }

    if (left)
    {
      neighbours[0] = GRID[r - 1][c - 1];
    }
  }

  // bottom
  if (isInGrid(r + 1, c))
  {
    neighbours[6] = GRID[r + 1][c];

    if (right)
    {
      neighbours[7] = GRID[r + 1][c + 1];
    }

    if (left)
    {
      neighbours[5] = GRID[r + 1][c - 1];
    }
  }
}

void setupGrid()
{
  // initialize random number generator
  srand(time(NULL));

  // place mines
  for (size_t r = 0; r < GRID_SIZE; r++)
  {
    for (size_t c = 0; c < GRID_SIZE; c++)
    {
      GRID[r][c].x = c;
      GRID[r][c].y = r;

      uint8_t state = STATE_BLANK;
      if (rand() % MINE_ODDS == 0)
      {
        state = STATE_MINE;
      }
      GRID[r][c].state = state;
    }
  }

  // calculate state values
  for (size_t r = 0; r < GRID_SIZE; r++)
  {
    for (size_t c = 0; c < GRID_SIZE; c++)
    {
      if (GRID[r][c].state == STATE_MINE)
        continue;

      struct Tile neighbours[8];
      getNeighbours(r, c, neighbours);

      for (size_t i = 0; i < sizeof(neighbours); i++)
      {
        if (sizeof(neighbours[i]) > 0 && neighbours[i].state == STATE_MINE)
        {
          GRID[r][c].state++;
        }
      }
    }
  }
}

/*
- x, y: upper left corner.
- texture, rect: outputs.
*/
void getTextAndRect(SDL_Renderer *renderer, int x, int y, char *text, SDL_Texture **texture, SDL_Rect *rect)
{
  int text_width;
  int text_height;
  SDL_Surface *surface;
  SDL_Color textColor = {0, 0, 255, 0};

  surface = TTF_RenderText_Solid(FONT, text, textColor);
  *texture = SDL_CreateTextureFromSurface(renderer, surface);
  text_width = surface->w;
  text_height = surface->h;
  SDL_FreeSurface(surface);
  rect->x = x + (SCREEN_SIZE / GRID_SIZE - text_width) / 2;
  rect->y = y + (SCREEN_SIZE / GRID_SIZE - text_height) / 2;
  rect->w = text_width;
  rect->h = text_height;
}

void drawGrid(SDL_Window *window, SDL_Renderer *renderer)
{
  SDL_SetRenderDrawColor(renderer, 0xCC, 0xCC, 0xCC, 0xFF);
  SDL_RenderClear(renderer);

  for (size_t r = 0; r < GRID_SIZE; r++)
  {
    for (size_t c = 0; c < GRID_SIZE; c++)
    {
      SDL_Rect rect;
      rect.w = SCREEN_SIZE / GRID_SIZE;
      rect.h = SCREEN_SIZE / GRID_SIZE;
      rect.x = c * rect.w;
      rect.y = r * rect.h;

      if (GRID[r][c].state == STATE_MINE)
      {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderFillRect(renderer, &rect);
      }
      else if (GRID[r][c].state > STATE_BLANK && GRID[r][c].state < STATE_MINE)
      {
        SDL_SetRenderDrawColor(renderer, 0xCC, 0xCC, 0xCC, 0xFF);
        SDL_RenderFillRect(renderer, &rect);

        // create mines text
        char *mines = "1";
        switch (GRID[r][c].state)
        {
        case 2:
          mines = "2";
          break;
        case 3:
          mines = "3";
          break;
        case 4:
          mines = "4";
          break;
        case 5:
          mines = "5";
          break;
        case 6:
          mines = "6";
          break;
        case 7:
          mines = "7";
          break;
        case 8:
          mines = "8";
          break;
        default:
          break;
        }

        SDL_Rect textRect;
        SDL_Texture *texture;
        getTextAndRect(renderer, rect.x, rect.y, mines, &texture, &textRect);

        // render text
        if (SDL_RenderCopy(renderer, texture, NULL, &textRect) < 0)
          printf(" %s ", SDL_GetError());
      }
    }
  }

  SDL_RenderPresent(renderer);
}

int main()
{
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Surface *screenSurface = NULL;

  // init png loading
  IMG_Init(IMG_INIT_PNG);

  // load font
  TTF_Init();
  FONT = TTF_OpenFont("font.ttf", 16);
  if (FONT == NULL)
  {
    printf(" could not find font %s ", stderr);
    exit(EXIT_FAILURE);
  }

  bool quit = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
  }
  else
  {
    // create window
    window = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_SIZE, SCREEN_SIZE, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
      printf("Window could not be created! SDL_ERROR: %s\n", SDL_GetError());
    }
    else
    {
      // get window surface
      screenSurface = SDL_GetWindowSurface(window);
      renderer = SDL_GetRenderer(window);

      quit = false;
    }
  }

  SDL_Event e;

  setupGrid();

  while (!quit)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      if (e.type == SDL_QUIT)
        quit = true;
    }

    drawGrid(window, renderer);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
