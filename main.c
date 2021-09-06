#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

const int SCREEN_SIZE = 500;
const int GRID_SIZE = 20; // 0 - 256
const int MINE_ODDS = 6;

TTF_Font *FONT;

const uint8_t STATE_BLANK = 0;
const uint8_t STATE_MINE = 9;
const uint8_t STATE_EMPTY_NEIGHBOUR = 255;

bool gameOver = false;

struct Tile
{
  uint8_t x;
  uint8_t y;
  uint8_t state;
  bool revealed;
  bool flag;
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

  for (size_t i = 0; i < 8; i++)
  {
    neighbours[i].state = STATE_EMPTY_NEIGHBOUR;
  }

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
        if (neighbours[i].state != STATE_EMPTY_NEIGHBOUR && neighbours[i].state == STATE_MINE)
        {
          GRID[r][c].state++;
        }
      }
    }
  }
}

SDL_Texture *numTextures[8];

void setupMineNumbers(SDL_Renderer *renderer)
{
  SDL_Surface *surface;
  SDL_Color textColor = {0, 0, 255, 0};

  for (uint8_t i = 0; i < 8; i++)
  {
    char text[1];
    sprintf(text, "%u", i + 1);

    surface = TTF_RenderText_Solid(FONT, text, textColor);
    if (surface == NULL)
    {
      printf(" %s ", SDL_GetError());
    }

    numTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
    if (numTextures[i] == NULL)
    {
      printf(" %s ", SDL_GetError());
    }
  }

  SDL_FreeSurface(surface);
}

void revealNeighbours(int r, int c)
{
  struct Tile neighbours[8];
  getNeighbours(r, c, neighbours);

  for (size_t i = 0; i < 8; i++)
  {
    if (neighbours[i].state == STATE_EMPTY_NEIGHBOUR || neighbours[i].revealed)
      continue;

    GRID[neighbours[i].y][neighbours[i].x].revealed = true;

    if (neighbours[i].state == STATE_BLANK)
    {
      revealNeighbours(neighbours[i].y, neighbours[i].x);
    }
  }
}

void tileClicked(int r, int c)
{
  struct Tile *tile = &GRID[r][c];

  switch (tile->state)
  {
  case STATE_MINE:
    gameOver = true;
    tile->revealed = true;
    break;
  case STATE_BLANK:
    // recursively reveal all reachable blanks
    tile->revealed = true;
    revealNeighbours(r, c);
    break;
  default:
    tile->revealed = true;
    break;
  }
}

void placeFlag(int r, int c)
{
  if (!GRID[r][c].revealed)
  {
    GRID[r][c].flag = !GRID[r][c].flag;
  }
}

void drawGrid(SDL_Window *window, SDL_Renderer *renderer)
{
  SDL_SetRenderDrawColor(renderer, 0x99, 0x99, 0x99, 0xFF);
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

      if (GRID[r][c].revealed)
      {
        if (GRID[r][c].state == STATE_MINE)
        {
          SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
          SDL_RenderFillRect(renderer, &rect);
        }
        else if (GRID[r][c].state > STATE_BLANK && GRID[r][c].state < STATE_MINE)
        {
          SDL_SetRenderDrawColor(renderer, 0xCC, 0xCC, 0xCC, 0xFF);
          SDL_RenderFillRect(renderer, &rect);

          // render mines text
          SDL_Rect textRect;
          textRect.w = SCREEN_SIZE / GRID_SIZE;
          textRect.h = SCREEN_SIZE / GRID_SIZE;
          textRect.x = rect.x;
          textRect.y = rect.y;

          if (SDL_RenderCopy(renderer, numTextures[GRID[r][c].state - 1], NULL, &textRect) < 0)
            printf(" RENDERCOPY_ERROR: %s ", SDL_GetError());
        }
        else
        {
          SDL_SetRenderDrawColor(renderer, 0xCC, 0xCC, 0xCC, 0xFF);
          SDL_RenderFillRect(renderer, &rect);
        }
      }
      else if (GRID[r][c].flag)
      {
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

int main()
{
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;

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

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  // Create a window usable with OpenGL context
  window = SDL_CreateWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_SIZE, SCREEN_SIZE, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  // Select render driver
  // - A render driver that supports HW acceleration is used when available
  // - Otherwise a render driver supporting software fallback is selected
  SDL_RendererInfo renderDriverInfo;
  uint32_t rendererFlags = SDL_RENDERER_TARGETTEXTURE;
  int32_t nbRenderDrivers = SDL_GetNumRenderDrivers(), index = 0;
  if (nbRenderDrivers < 0)
  {
    exit(EXIT_FAILURE);
  }

  while (index < nbRenderDrivers)
  {
    if (SDL_GetRenderDriverInfo(index, &renderDriverInfo) == 0)
    {
      if (((renderDriverInfo.flags & rendererFlags) == rendererFlags) && ((renderDriverInfo.flags & SDL_RENDERER_ACCELERATED) == SDL_RENDERER_ACCELERATED))
      {
        // Using render driver with HW acceleration
        rendererFlags |= SDL_RENDERER_ACCELERATED;
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderDriverInfo.name);
        break;
      }
    }
    ++index;
  }

  if (index == nbRenderDrivers)
  {
    // Let SDL use the first render driver supporting software fallback
    rendererFlags |= SDL_RENDERER_SOFTWARE;
    index = -1;
  }

  // Create renderer
  renderer = SDL_CreateRenderer(window, index, rendererFlags);
  if (renderer == NULL)
  {
    printf("renderer SDL_Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  SDL_Event e;

  setupGrid();
  setupMineNumbers(renderer);

  bool quit = false;

  while (!quit && !gameOver)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      switch (e.type)
      {
      case SDL_QUIT:
      {

        quit = true;
        break;
      }
      case SDL_MOUSEBUTTONUP:
      {
        int x, y;
        SDL_GetMouseState(&x, &y);

        int r = y / (SCREEN_SIZE / GRID_SIZE);
        int c = x / (SCREEN_SIZE / GRID_SIZE);

        if (e.button.button == SDL_BUTTON_LEFT && !GRID[r][c].flag)
          tileClicked(r, c);
        else if (e.button.button == SDL_BUTTON_RIGHT)
          placeFlag(r, c);

        break;
      }
      }
    }

    drawGrid(window, renderer);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
