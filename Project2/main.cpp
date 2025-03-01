/**
* Author: Belinda Weng
* Assignment: Pong Clone
* Date due: 2025-3-1, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"
#include <cmath>
#include <iostream>

enum AppStatus { RUNNING, TERMINATED };
enum GameState { START, PLAYING, GAME_OVER };

// Our window dimensions
constexpr int WINDOW_WIDTH = 1280,
WINDOW_HEIGHT = 720;

// Background color components
constexpr float BG_RED = 0.0f,
BG_BLUE = 0.0f,
BG_GREEN = 0.0f,
BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Our shader filepaths; these are necessary for a number of things
// Not least, to actually draw our shapes 
// We'll have a whole lecture on these later
constexpr char V_SHADER_PATH[] = "shaders/vertex.glsl",
F_SHADER_PATH[] = "shaders/fragment.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

// Colors for our game objects
constexpr float PADDLE_RED = 1.0f,
PADDLE_GREEN = 1.0f,
PADDLE_BLUE = 1.0f,
PADDLE_OPACITY = 1.0f;

constexpr float BALL_RED = 1.0f,
BALL_GREEN = 1.0f,
BALL_BLUE = 1.0f,
BALL_OPACITY = 1.0f;

// Game parameters
constexpr float PADDLE_HEIGHT = 1.0f;
constexpr float PADDLE_WIDTH = 0.2f;
constexpr float BALL_SIZE = 0.2f;
constexpr float PADDLE_SPEED = 5.0f;
constexpr float BALL_SPEED = 3.0f;
constexpr float SCREEN_TOP = 3.75f;
constexpr float SCREEN_BOTTOM = -3.75f;
constexpr float SCREEN_LEFT = -5.0f;
constexpr float SCREEN_RIGHT = 5.0f;

AppStatus g_app_status = RUNNING;
GameState g_game_state = START;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
g_projection_matrix;  // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.

// Game objects
glm::vec3 g_left_paddle_position = glm::vec3(-4.5f, 0.0f, 0.0f);
glm::vec3 g_right_paddle_position = glm::vec3(4.5f, 0.0f, 0.0f);
glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_left_paddle_movement = glm::vec3(0.0f);
glm::vec3 g_right_paddle_movement = glm::vec3(0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f);

// Game state
bool g_two_player_mode = true;
int g_winner = 0; // 0 = no winner, 1 = left player, 2 = right player

float previous_ticks = 0.0f;

// Function declarations
void initialise();
void process_input();
void update();
void render();
void shutdown();
void reset_game();
void draw_paddle(const glm::vec3& position);
void draw_ball(const glm::vec3& position);
bool check_collision(const glm::vec3& paddle_pos, const glm::vec3& ball_pos);
void update_ai_paddle(float delta_time);

void initialise()
{
    // Initialize SDL and create window
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;
        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // Initialize OpenGL viewport
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    // Load shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    // Initialize matrices
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // Initialize ball movement with random direction
    reset_game();
}

void reset_game()
{
    // Reset positions
    g_left_paddle_position = glm::vec3(-4.5f, 0.0f, 0.0f);
    g_right_paddle_position = glm::vec3(4.5f, 0.0f, 0.0f);
    g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);

    // Reset movements
    g_left_paddle_movement = glm::vec3(0.0f);
    g_right_paddle_movement = glm::vec3(0.0f);

    // Set random ball direction
    float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
    g_ball_movement.x = cos(angle);
    g_ball_movement.y = sin(angle);

    // Make sure the ball is moving horizontally enough
    if (fabs(g_ball_movement.x) < 0.5f) {
        g_ball_movement.x = (g_ball_movement.x > 0) ? 0.5f : -0.5f;
    }

    // Normalize the movement vector
    g_ball_movement = glm::normalize(g_ball_movement);

    g_game_state = PLAYING;
    g_winner = 0;
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                g_app_status = TERMINATED;
                break;
            case SDLK_t:
                if (g_game_state == PLAYING) {
                    g_two_player_mode = !g_two_player_mode;
                }
                break;
            case SDLK_SPACE:
                if (g_game_state == START || g_game_state == GAME_OVER) {
                    reset_game();
                }
                break;
            }
            break;
        }
    }

    // Process keyboard input for paddle movement
    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // Reset paddle movements
    g_left_paddle_movement = glm::vec3(0.0f);
    g_right_paddle_movement = glm::vec3(0.0f);

    // Only process player 1 input if in two-player mode
    if (g_two_player_mode && g_game_state == PLAYING) {
        if (key_state[SDL_SCANCODE_W]) {
            g_left_paddle_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_S]) {
            g_left_paddle_movement.y = -1.0f;
        }
    }

    // Always process player 2 input
    if (g_game_state == PLAYING) {
        if (key_state[SDL_SCANCODE_UP]) {
            g_right_paddle_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN]) {
            g_right_paddle_movement.y = -1.0f;
        }
    }
}

void update_ai_paddle(float delta_time)
{
    // Simple AI: follow the ball
    if (g_ball_position.y > g_left_paddle_position.y + 0.1f) {
        g_left_paddle_movement.y = 1.0f;
    }
    else if (g_ball_position.y < g_left_paddle_position.y - 0.1f) {
        g_left_paddle_movement.y = -1.0f;
    }
    else {
        g_left_paddle_movement.y = 0.0f;
    }
}

bool check_collision(const glm::vec3& paddle_pos, const glm::vec3& ball_pos)
{
    // Check if ball collides with paddle
    float x_distance = fabs(paddle_pos.x - ball_pos.x);
    float y_distance = fabs(paddle_pos.y - ball_pos.y);

    return (x_distance < (PADDLE_WIDTH + BALL_SIZE) / 2.0f &&
        y_distance < (PADDLE_HEIGHT + BALL_SIZE) / 2.0f);
}

void update()
{
    // Calculate delta time
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;

    if (g_game_state != PLAYING) {
        return;
    }

    // Update AI paddle if in single-player mode
    if (!g_two_player_mode) {
        update_ai_paddle(delta_time);
    }

    // Update paddle positions
    g_left_paddle_position += g_left_paddle_movement * PADDLE_SPEED * delta_time;
    g_right_paddle_position += g_right_paddle_movement * PADDLE_SPEED * delta_time;

    // Constrain paddles to screen boundaries
    if (g_left_paddle_position.y + PADDLE_HEIGHT / 2.0f > SCREEN_TOP) {
        g_left_paddle_position.y = SCREEN_TOP - PADDLE_HEIGHT / 2.0f;
    }
    else if (g_left_paddle_position.y - PADDLE_HEIGHT / 2.0f < SCREEN_BOTTOM) {
        g_left_paddle_position.y = SCREEN_BOTTOM + PADDLE_HEIGHT / 2.0f;
    }

    if (g_right_paddle_position.y + PADDLE_HEIGHT / 2.0f > SCREEN_TOP) {
        g_right_paddle_position.y = SCREEN_TOP - PADDLE_HEIGHT / 2.0f;
    }
    else if (g_right_paddle_position.y - PADDLE_HEIGHT / 2.0f < SCREEN_BOTTOM) {
        g_right_paddle_position.y = SCREEN_BOTTOM + PADDLE_HEIGHT / 2.0f;
    }

    // Update ball position
    g_ball_position += g_ball_movement * BALL_SPEED * delta_time;

    // Check for collisions with top and bottom walls
    if (g_ball_position.y + BALL_SIZE / 2.0f > SCREEN_TOP) {
        g_ball_position.y = SCREEN_TOP - BALL_SIZE / 2.0f;
        g_ball_movement.y *= -1.0f;
    }
    else if (g_ball_position.y - BALL_SIZE / 2.0f < SCREEN_BOTTOM) {
        g_ball_position.y = SCREEN_BOTTOM + BALL_SIZE / 2.0f;
        g_ball_movement.y *= -1.0f;
    }

    // Check for collisions with paddles
    if (check_collision(g_left_paddle_position, g_ball_position)) {
        g_ball_position.x = g_left_paddle_position.x + (PADDLE_WIDTH + BALL_SIZE) / 2.0f;
        g_ball_movement.x = fabs(g_ball_movement.x); // Ensure ball moves right

        // Add some variation to the bounce based on where the ball hit the paddle
        float relative_intersect_y = (g_left_paddle_position.y - g_ball_position.y) / (PADDLE_HEIGHT / 2.0f);
        float bounce_angle = relative_intersect_y * 0.75f;
        g_ball_movement.y = -bounce_angle;
        g_ball_movement = glm::normalize(g_ball_movement);
    }
    else if (check_collision(g_right_paddle_position, g_ball_position)) {
        g_ball_position.x = g_right_paddle_position.x - (PADDLE_WIDTH + BALL_SIZE) / 2.0f;
        g_ball_movement.x = -fabs(g_ball_movement.x); // Ensure ball moves left

        // Add some variation to the bounce based on where the ball hit the paddle
        float relative_intersect_y = (g_right_paddle_position.y - g_ball_position.y) / (PADDLE_HEIGHT / 2.0f);
        float bounce_angle = relative_intersect_y * 0.75f;
        g_ball_movement.y = -bounce_angle;
        g_ball_movement = glm::normalize(g_ball_movement);
    }

    // Check for scoring (ball hits left or right wall)
    if (g_ball_position.x < SCREEN_LEFT) {
        // Right player scores
        g_winner = 2;
        g_game_state = GAME_OVER;
    }
    else if (g_ball_position.x > SCREEN_RIGHT) {
        // Left player scores
        g_winner = 1;
        g_game_state = GAME_OVER;
    }
}

void draw_paddle(const glm::vec3& position)
{
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    g_shader_program.set_model_matrix(model_matrix);

    g_shader_program.set_colour(PADDLE_RED, PADDLE_GREEN, PADDLE_BLUE, PADDLE_OPACITY);

    float vertices[] = {
        -PADDLE_WIDTH / 2, -PADDLE_HEIGHT / 2,  // Bottom left
         PADDLE_WIDTH / 2, -PADDLE_HEIGHT / 2,  // Bottom right
         PADDLE_WIDTH / 2,  PADDLE_HEIGHT / 2,  // Top right

        -PADDLE_WIDTH / 2, -PADDLE_HEIGHT / 2,  // Bottom left
         PADDLE_WIDTH / 2,  PADDLE_HEIGHT / 2,  // Top right
        -PADDLE_WIDTH / 2,  PADDLE_HEIGHT / 2   // Top left
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
}

void draw_ball(const glm::vec3& position)
{
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    g_shader_program.set_model_matrix(model_matrix);

    g_shader_program.set_colour(BALL_RED, BALL_GREEN, BALL_BLUE, BALL_OPACITY);

    // Create a simple square for the ball
    float vertices[] = {
        -BALL_SIZE / 2, -BALL_SIZE / 2,  // Bottom left
         BALL_SIZE / 2, -BALL_SIZE / 2,  // Bottom right
         BALL_SIZE / 2,  BALL_SIZE / 2,  // Top right

        -BALL_SIZE / 2, -BALL_SIZE / 2,  // Bottom left
         BALL_SIZE / 2,  BALL_SIZE / 2,  // Top right
        -BALL_SIZE / 2,  BALL_SIZE / 2   // Top left
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw paddles
    draw_paddle(g_left_paddle_position);
    draw_paddle(g_right_paddle_position);

    // Draw ball
    draw_ball(g_ball_position);



    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}