#include "raylib.h"
#include <iostream>

#define cellWidth 25
#define maxTail cellWidth * cellWidth

namespace colours{
    Color backgroundGrid = {194, 194, 194, 255};
    Color apple = {255, 77, 77, 255};
    Color snakeHead = {47, 163, 8, 255};
    Color grid1 = {165, 199, 181, 150};
    Color grid2 = {138, 166, 151, 150};
    Color gold = {211, 214, 122, 255};
    Color greenText = {15, 97, 0, 255};
};

namespace assets{
    Sound pointSound;
    Sound lossSound;
};

const static int screenWidth = 1025;
const static int screenHeight = 675;

static void setup();
static void loadAssets();
static void spawnApple();
static void spawnSnake();
static void checkCollisions();
static void render();
static void update();
static void updateSnakePosition();
static void keyboardInputs();
static void unloadAssets();
static void tailUpdate();
static void crashed();
static void reset();

enum cellState {NONE, APPLE, OBSTACLE};

cellState grid[screenWidth][screenHeight];

Vector2 tail[maxTail];

class Snake{
public:
    Vector2 position;
    Vector2 direction;
    int length = 1;
};

Snake snake;

int score = 0;
int highScore;
bool isPaused = false;
bool hasStarted = false;

float timer = 0;
bool resetTiming;

int main()
{
    InitWindow(screenWidth, screenHeight, "Snake");
   
    InitAudioDevice();
    
    SetTargetFPS(12); 
    
    loadAssets();
    setup();

    while (!WindowShouldClose())  
    {
        BeginDrawing();

            ClearBackground(RAYWHITE);

            update();
            render();

        EndDrawing();
    }
    
    unloadAssets();

    CloseWindow(); 
    CloseAudioDevice();
    SaveStorageValue(0, highScore);

    return 0;
}

static void setup(){
    score = 0;
    snake.length = 0;
    HideCursor();
    spawnSnake();
    
    spawnApple();
    spawnApple();
}

static void reset(){
    score = 0;
    snake.length = 0;
    hasStarted = false;
    
    for (int x = 0; x < screenWidth; x += cellWidth){
        for (int y = 0; y < screenHeight; y += cellWidth){
            grid[x][y] = NONE;
        }
    }
    
    spawnSnake();
    isPaused = false;
    snake.direction = {0, 0};
    setup();
}

static void loadAssets(){
    highScore = LoadStorageValue(0);
    assets::pointSound = LoadSound("res/point.wav");
    assets::lossSound  = LoadSound("res/explosion.wav");
}

static void spawnSnake(){
    Vector2 pos = {screenWidth / 2, screenHeight / 2};
    
    pos.x = int(pos.x / cellWidth) * cellWidth;
    pos.y = int(pos.y / cellWidth) * cellWidth;
      
    snake.position = pos;
}

static void spawnApple(){
    respawn:
        Vector2 randPosition;
         
        randPosition.x = GetRandomValue(0, screenWidth - cellWidth);
        randPosition.y = GetRandomValue(0, screenHeight - cellWidth);
        
        randPosition.x = int(randPosition.x / cellWidth) * cellWidth;
        randPosition.y = int(randPosition.y / cellWidth) * cellWidth;
        
        if (grid[int(randPosition.x)][int(randPosition.y)] == OBSTACLE || grid[int(randPosition.x)][int(randPosition.y)] == APPLE){
            goto respawn;
        }
        grid[int(randPosition.x)][int(randPosition.y)] = APPLE;
}

static void render(){
    int gridAlternator = 1;
    for (int x = 0; x < screenWidth; x += cellWidth){
        for (int y = 0; y < screenHeight; y += cellWidth){
            if (gridAlternator == 1){
                DrawRectangle(x, y, cellWidth, cellWidth, colours::grid1);
                gridAlternator = 2;
            }
            else{
                DrawRectangle(x, y, cellWidth, cellWidth, colours::grid2);
                gridAlternator = 1;
            }
            
            if (grid[x][y] == APPLE){
                DrawRectangle(x, y, cellWidth, cellWidth, colours::apple);
            }
            if (grid[x][y] == OBSTACLE){
                DrawRectangle(x, y, cellWidth, cellWidth, BLACK);
            }
        }
    }
    
    DrawRectangle(snake.position.x, snake.position.y, cellWidth, cellWidth, colours::snakeHead);
    DrawText(TextFormat("Score: %01i", score), 0, 0, 30, YELLOW);
    
    if (!hasStarted){
        DrawText("Move to begin", screenWidth / 2 - 145, screenHeight / 2 - 300, 40, colours::greenText);
        DrawText(TextFormat("Hi-score: %01i", highScore), screenWidth / 2 - 90, screenHeight / 2 + 200, 30, YELLOW);
    }
}

static void update(){
    if (snake.direction.x != 0 || snake.direction.y != 0){
        hasStarted = true;
    }
    if (score > highScore){
        highScore = score;
    }
    
    tailUpdate();
    
    if (timer > 0){
        timer -= 1 * GetFrameTime();
    }
    if (resetTiming){
        if (timer <= 0){
            resetTiming = false;
            reset();
        }
    }
    
    if (isPaused) {return;}
    keyboardInputs();
    updateSnakePosition();
}

static void updateSnakePosition(){
    Vector2 newPosition = snake.position;
    
    if (newPosition.x > screenWidth - cellWidth || newPosition.x < 0 || newPosition.y > screenHeight - cellWidth || newPosition.y < 0) {crashed(); return;}
    
    checkCollisions();
    
    newPosition.x += snake.direction.x * cellWidth;
    newPosition.y += snake.direction.y * cellWidth;
    
    snake.position = newPosition;
}

static void keyboardInputs(){
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)){
        if (snake.direction.y == 1) {return;}
        snake.direction = {0, -1};
    }
    else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)){
        if (snake.direction.y == -1) {return;}
        snake.direction = {0, 1};
    }
    else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)){
        if (snake.direction.x == 1) {return;}
        snake.direction = {-1, 0};
    }
    else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)){
        if (snake.direction.x == -1) {return;}
        snake.direction = {1, 0};
    }
}

static void checkCollisions(){
    Vector2 playerPos;
    playerPos = snake.position;
    
    playerPos.x = int(playerPos.x / cellWidth) * cellWidth;
    playerPos.y = int(playerPos.y / cellWidth) * cellWidth;
    
    if (grid[int(playerPos.x)][int(playerPos.y)] == APPLE){ //Collected an apple.
        snake.length += 1;
        score += 1;
        spawnApple();
        PlaySound(assets::pointSound);
        grid[int(playerPos.x)][int(playerPos.y)] = OBSTACLE;
    }
    else if (grid[int(playerPos.x)][int(playerPos.y)] == OBSTACLE){
        crashed();
    }
    
    for (int i = 1; i < snake.length; i++){
        if (tail[i].x == snake.position.x && tail[i].y == snake.position.y){
            crashed();
        }
    }
}

static void unloadAssets(){
    UnloadSound(assets::pointSound);
    UnloadSound(assets::lossSound);
}

static void tailUpdate(){    
    if (!isPaused){
        for (int i = 0; i < snake.length - 1; i++){
            Vector2 temp = tail[i];
            tail[i] = tail[i + 1];
            tail[i + 1] = temp;
        }
        
        tail[snake.length + 1] = snake.position;
        
        tail[0] = snake.position;        
    }

    for (int i = 0; i < snake.length; i++){
       DrawRectangle(tail[i].x, tail[i].y, cellWidth, cellWidth, GREEN); 
    }
}

static void crashed(){
    isPaused = true;
    PlaySound(assets::lossSound);
    resetTiming = true;
    timer = 2;
}