#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// Game Constants
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr float PLAYER_RADIUS = 0.35f;
constexpr float PLAYER_HEIGHT = 1.8f;
constexpr float EYE_HEIGHT = 1.55f;
constexpr float GRAVITY = 22.0f;
constexpr float JUMP_SPEED = 8.5f;
constexpr float MOVE_SPEED = 5.0f;
constexpr float MOUSE_SENSITIVITY = 0.0025f;
constexpr float TARGET_FOV = 75.0f;

// Game Structures
struct Wall {
    BoundingBox box;
    Color color;
};

struct Target {
    Vector3 position;
    Vector3 basePosition;
    float phase;
    bool alive;
};

struct Player {
    Vector3 position;
    float verticalVelocity;
    float yaw;
    float pitch;
    bool grounded;
};

// Function Declarations
BoundingBox GetPlayerBoundingBox(const Vector3 position);
bool IsColliding(const Vector3 position, const std::vector<Wall>& walls);
void MovePlayerXZ(Vector3& position, const Vector3 delta, const std::vector<Wall>& walls);
Vector3 GetForwardVector(float yaw, float pitch);
Vector3 GetRightVector(float yaw);
bool ShootTarget(const Camera3D& camera, std::vector<Target>& targets);
void DrawGameMenu();
void DrawGame3D(const Camera3D& camera, const std::vector<Wall>& walls, const std::vector<Target>& targets);
void DrawGameUI(int fps, int score, int totalTargets);
std::vector<Wall> InitializeWalls();
std::vector<Target> InitializeTargets();
void ResetGame(Player& player, std::vector<Target>& targets, int& score);

// Function Implementations
BoundingBox GetPlayerBoundingBox(const Vector3 position) {
    return {
        {position.x - PLAYER_RADIUS, position.y, position.z - PLAYER_RADIUS},
        {position.x + PLAYER_RADIUS, position.y + PLAYER_HEIGHT, position.z + PLAYER_RADIUS}
    };
}

bool IsColliding(const Vector3 position, const std::vector<Wall>& walls) {
    const BoundingBox playerBox = GetPlayerBoundingBox(position);
    for (const Wall& wall : walls) {
        if (CheckCollisionBoxes(playerBox, wall.box)) {
            return true;
        }
    }
    return false;
}

void MovePlayerXZ(Vector3& position, const Vector3 delta, const std::vector<Wall>& walls) {
    Vector3 next = position;
    next.x += delta.x;
    if (!IsColliding(next, walls)) {
        position.x = next.x;
    }

    next = position;
    next.z += delta.z;
    if (!IsColliding(next, walls)) {
        position.z = next.z;
    }
}

Vector3 GetForwardVector(float yaw, float pitch) {
    const float cosPitch = std::cos(pitch);
    return Vector3Normalize({
        std::sin(yaw) * cosPitch,
        std::sin(pitch),
        std::cos(yaw) * cosPitch
    });
}

Vector3 GetRightVector(float yaw) {
    return {std::cos(yaw), 0.0f, -std::sin(yaw)};
}

bool ShootTarget(const Camera3D& camera, std::vector<Target>& targets) {
    const Ray ray = GetScreenToWorldRay({SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f}, camera);
    
    int nearestIndex = -1;
    float nearestDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < static_cast<int>(targets.size()); ++i) {
        if (!targets[i].alive) continue;

        const Vector3 p = targets[i].position;
        const BoundingBox targetBox = {
            {p.x - 0.45f, p.y - 0.7f, p.z - 0.45f},
            {p.x + 0.45f, p.y + 0.7f, p.z + 0.45f}
        };
        
        const RayCollision collision = GetRayCollisionBox(ray, targetBox);
        
        if (collision.hit && collision.distance < nearestDistance) {
            nearestDistance = collision.distance;
            nearestIndex = i;
        }
    }

    if (nearestIndex < 0) {
        return false;
    }

    targets[nearestIndex].alive = false;
    return true;
}

void DrawGameMenu() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {12, 18, 30, 255});

    const char* title = "RAYLIB FPS";
    DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 64) / 2, 145, 64, SKYBLUE);

    const char* subtitle = "3D TRAINING ARENA";
    DrawText(subtitle, SCREEN_WIDTH / 2 - MeasureText(subtitle, 24) / 2, 225, 24, LIGHTGRAY);

    DrawRectangle(SCREEN_WIDTH / 2 - 150, 335, 300, 70, BLUE);
    const char* playText = "PLAY";
    DrawText(playText, SCREEN_WIDTH / 2 - MeasureText(playText, 32) / 2, 353, 32, RAYWHITE);

    DrawText("WASD move   |   Mouse look   |   Space jump", SCREEN_WIDTH / 2 - 255, 500, 20, GRAY);
    DrawText("Left click: shoot   |   ESC: menu", SCREEN_WIDTH / 2 - 160, 530, 20, GRAY);
    DrawText("Press ENTER or click PLAY", SCREEN_WIDTH / 2 - 140, 580, 20, LIGHTGRAY);
}

void DrawGame3D(const Camera3D& camera, const std::vector<Wall>& walls, const std::vector<Target>& targets) {
    BeginMode3D(camera);
    
    // Draw ground plane
    DrawPlane({0.0f, 0.0f, 0.0f}, {24.0f, 24.0f}, DARKGREEN);
    
    // Draw walls
    for (const Wall& wall : walls) {
        const Vector3 center = Vector3Scale(Vector3Add(wall.box.min, wall.box.max), 0.5f);
        const Vector3 size = Vector3Subtract(wall.box.max, wall.box.min);
        DrawCube(center, size.x, size.y, size.z, wall.color);
        DrawCubeWires(center, size.x, size.y, size.z, MAROON);
    }
    
    // Draw targets
    for (const Target& target : targets) {
        if (!target.alive) continue;
        DrawCube(target.position, 0.9f, 1.4f, 0.9f, RED);
        DrawCubeWires(target.position, 0.9f, 1.4f, 0.9f, YELLOW);
    }
    
    EndMode3D();
}

void DrawGameUI(int fps, int score, int totalTargets) {
    // Draw crosshair
    DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2.0f, WHITE);
    DrawLine(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2 + 10, SCREEN_HEIGHT / 2, WHITE);
    DrawLine(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 10, WHITE);
    
    // Draw HUD background
    DrawRectangle(15, 15, 235, 76, Fade(BLACK, 0.55f));
    
    // Draw FPS and score
    DrawText(TextFormat("FPS: %d", fps), 28, 25, 20, LIME);
    DrawText(TextFormat("Targets: %d / %d", score, totalTargets), 28, 53, 20, RAYWHITE);
    
    // Draw menu hint
    DrawText("ESC: menu", SCREEN_WIDTH - 120, 20, 18, RAYWHITE);
}

std::vector<Wall> InitializeWalls() {
    return {
        // Outer walls
        {{{-12.0f, 0.0f, -12.0f}, {12.0f, 3.0f, -11.5f}}, GRAY},
        {{{-12.0f, 0.0f, 11.5f}, {12.0f, 3.0f, 12.0f}}, GRAY},
        {{{-12.0f, 0.0f, -12.0f}, {-11.5f, 3.0f, 12.0f}}, GRAY},
        {{{11.5f, 0.0f, -12.0f}, {12.0f, 3.0f, 12.0f}}, GRAY},
        
        // Interior cover
        {{{-3.0f, 0.0f, -2.0f}, {-2.2f, 2.2f, 3.0f}}, BROWN},
        {{{3.0f, 0.0f, 1.0f}, {7.0f, 1.4f, 2.0f}}, BROWN},
        {{{-8.0f, 0.0f, 6.0f}, {-4.0f, 1.2f, 7.0f}}, BROWN}
    };
}

std::vector<Target> InitializeTargets() {
    return {
        {{-5.0f, 1.4f, -4.0f}, {-5.0f, 1.4f, -4.0f}, 0.0f, true},
        {{5.0f, 1.4f, -5.0f}, {5.0f, 1.4f, -5.0f}, 1.7f, true},
        {{6.0f, 1.4f, 7.0f}, {6.0f, 1.4f, 7.0f}, 3.0f, true},
        {{-7.0f, 1.4f, 3.0f}, {-7.0f, 1.4f, 3.0f}, 4.2f, true}
    };
}

void ResetGame(Player& player, std::vector<Target>& targets, int& score) {
    player.position = {0.0f, 0.05f, 8.0f};
    player.verticalVelocity = 0.0f;
    player.yaw = 0.0f;
    player.pitch = 0.0f;
    player.grounded = false;
    
    score = 0;
    
    for (Target& target : targets) {
        target.alive = true;
        target.position = target.basePosition;
    }
}

// Main Game Loop
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib 6.0 - FPS Training Arena");
    SetTargetFPS(144);

    enum class GameState { Menu, Playing };
    GameState state = GameState::Menu;

    const std::vector<Wall> walls = InitializeWalls();
    std::vector<Target> targets = InitializeTargets();

    Player player = {
        {0.0f, 0.05f, 8.0f},  // position
        0.0f,                   // verticalVelocity
        0.0f,                   // yaw
        0.0f,                   // pitch
        false                   // grounded
    };

    int score = 0;

    while (!WindowShouldClose()) {
        const float deltaTime = std::min(GetFrameTime(), 0.05f);

        if (state == GameState::Menu) {
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state = GameState::Playing;
                ResetGame(player, targets, score);
                DisableCursor();
            }

            BeginDrawing();
            ClearBackground(BLACK);
            DrawGameMenu();
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = GameState::Menu;
            EnableCursor();
            continue;
        }

        // Handle mouse input
        const Vector2 mouseDelta = GetMouseDelta();
        player.yaw += mouseDelta.x * MOUSE_SENSITIVITY;
        player.pitch = std::clamp(player.pitch - mouseDelta.y * MOUSE_SENSITIVITY, -1.45f, 1.45f);

        // Handle movement input
        const Vector3 forward = GetForwardVector(player.yaw, 0.0f);
        const Vector3 right = GetRightVector(player.yaw);
        Vector3 movement = {0.0f, 0.0f, 0.0f};

        if (IsKeyDown(KEY_W)) movement = Vector3Add(movement, forward);
        if (IsKeyDown(KEY_S)) movement = Vector3Subtract(movement, forward);
        if (IsKeyDown(KEY_D)) movement = Vector3Add(movement, right);
        if (IsKeyDown(KEY_A)) movement = Vector3Subtract(movement, right);

        if (Vector3Length(movement) > 0.0f) {
            movement = Vector3Scale(Vector3Normalize(movement), MOVE_SPEED * deltaTime);
            MovePlayerXZ(player.position, movement, walls);
        }

        // Handle jumping
        player.grounded = player.position.y <= 0.051f;
        if (player.grounded && player.verticalVelocity < 0.0f) {
            player.verticalVelocity = 0.0f;
        }
        if (player.grounded && IsKeyPressed(KEY_SPACE)) {
            player.verticalVelocity = JUMP_SPEED;
        }

        // Apply gravity
        player.verticalVelocity -= GRAVITY * deltaTime;
        player.position.y += player.verticalVelocity * deltaTime;
        if (player.position.y < 0.05f) {
            player.position.y = 0.05f;
            player.verticalVelocity = 0.0f;
        }

        // Update target positions (patrol pattern)
        const float currentTime = static_cast<float>(GetTime());
        for (Target& target : targets) {
            if (target.alive) {
                target.position.x = target.basePosition.x + 
                    std::sin(currentTime * 1.2f + target.phase) * 2.0f;
            }
        }

        // Setup camera
        Camera3D camera = {};
        camera.position = {
            player.position.x,
            player.position.y + EYE_HEIGHT,
            player.position.z
        };
        camera.target = Vector3Add(camera.position, GetForwardVector(player.yaw, player.pitch));
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = TARGET_FOV;
        camera.projection = CAMERA_PERSPECTIVE;

        // Handle shooting
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (ShootTarget(camera, targets)) {
                ++score;
            }
        }

        // Render
        BeginDrawing();
        ClearBackground(SKYBLUE);
        
        DrawGame3D(camera, walls, targets);
        DrawGameUI(GetFPS(), score, static_cast<int>(targets.size()));
        
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}
