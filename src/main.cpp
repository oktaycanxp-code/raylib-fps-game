#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace {
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr float PLAYER_RADIUS = 0.35f;
constexpr float PLAYER_HEIGHT = 1.8f;
constexpr float EYE_HEIGHT = 1.55f;
constexpr float GRAVITY = 22.0f;
constexpr float JUMP_SPEED = 8.5f;
constexpr float MOVE_SPEED = 5.0f;
constexpr float MOUSE_SENSITIVITY = 0.0025f;

struct Wall {
    BoundingBox box;
    Color color;
};

struct Target {
    Vector3 position;
    Vector3 basePosition;
    float phase;
    bool alive = true;
};

BoundingBox GetPlayerBox(const Vector3 position) {
    return {
        {position.x - PLAYER_RADIUS, position.y, position.z - PLAYER_RADIUS},
        {position.x + PLAYER_RADIUS, position.y + PLAYER_HEIGHT,
         position.z + PLAYER_RADIUS}
    };
}

bool IsPlayerColliding(const Vector3 position, const std::vector<Wall>& walls) {
    const BoundingBox playerBox = GetPlayerBox(position);
    for (const Wall& wall : walls) {
        if (CheckCollisionBoxes(playerBox, wall.box)) {
            return true;
        }
    }
    return false;
}

void MovePlayer(Vector3& position, const Vector3 delta,
                const std::vector<Wall>& walls) {
    // Resolve each horizontal axis independently so the player can slide along walls.
    Vector3 next = position;
    next.x += delta.x;
    if (!IsPlayerColliding(next, walls)) {
        position.x = next.x;
    }

    next = position;
    next.z += delta.z;
    if (!IsPlayerColliding(next, walls)) {
        position.z = next.z;
    }
}

Vector3 GetForward(const float yaw, const float pitch) {
    const float cosinePitch = std::cos(pitch);
    return Vector3Normalize({
        std::sin(yaw) * cosinePitch,
        std::sin(pitch),
        std::cos(yaw) * cosinePitch
    });
}

bool ShootNearestTarget(const Camera3D& camera, std::vector<Target>& targets) {
    const Ray ray = GetScreenToWorldRay(
        {SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f}, camera);

    int nearestIndex = -1;
    float nearestDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < static_cast<int>(targets.size()); ++i) {
        if (!targets[i].alive) {
            continue;
        }

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

void DrawMenu() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {12, 18, 30, 255});

    const char* title = "RAYLIB FPS";
    DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 64) / 2,
             145, 64, SKYBLUE);

    const char* subtitle = "3D TRAINING ARENA";
    DrawText(subtitle, SCREEN_WIDTH / 2 - MeasureText(subtitle, 24) / 2,
             225, 24, LIGHTGRAY);

    const Rectangle playButton = {
        SCREEN_WIDTH / 2.0f - 150.0f, 335.0f, 300.0f, 70.0f
    };
    DrawRectangleRec(playButton, BLUE);

    const char* playText = "PLAY";
    DrawText(playText, SCREEN_WIDTH / 2 - MeasureText(playText, 32) / 2,
             353, 32, RAYWHITE);

    DrawText("WASD move   |   Mouse look   |   Space jump",
             SCREEN_WIDTH / 2 - 255, 500, 20, GRAY);
    DrawText("Left click: shoot   |   ESC: menu", SCREEN_WIDTH / 2 - 160,
             530, 20, GRAY);
    DrawText("Press ENTER or click PLAY", SCREEN_WIDTH / 2 - 140,
             580, 20, LIGHTGRAY);
}

std::vector<Wall> CreateMap() {
    return {
        // Outer walls.
        {{{-12.0f, 0.0f, -12.0f}, {12.0f, 3.0f, -11.5f}}, GRAY},
        {{{-12.0f, 0.0f, 11.5f}, {12.0f, 3.0f, 12.0f}}, GRAY},
        {{{-12.0f, 0.0f, -12.0f}, {-11.5f, 3.0f, 12.0f}}, GRAY},
        {{{11.5f, 0.0f, -12.0f}, {12.0f, 3.0f, 12.0f}}, GRAY},
        // Cover and obstacles.
        {{{-3.0f, 0.0f, -2.0f}, {-2.2f, 2.2f, 3.0f}}, BROWN},
        {{{3.0f, 0.0f, 1.0f}, {7.0f, 1.4f, 2.0f}}, BROWN},
        {{{-8.0f, 0.0f, 6.0f}, {-4.0f, 1.2f, 7.0f}}, BROWN}
    };
}

std::vector<Target> CreateTargets() {
    return {
        {{-5.0f, 1.4f, -4.0f}, {-5.0f, 1.4f, -4.0f}, 0.0f},
        {{5.0f, 1.4f, -5.0f}, {5.0f, 1.4f, -5.0f}, 1.7f},
        {{6.0f, 1.4f, 7.0f}, {6.0f, 1.4f, 7.0f}, 3.0f},
        {{-7.0f, 1.4f, 3.0f}, {-7.0f, 1.4f, 3.0f}, 4.2f}
    };
}

void DrawWorld(const Camera3D& camera, const std::vector<Wall>& walls,
              const std::vector<Target>& targets) {
    BeginMode3D(camera);
    DrawPlane({0.0f, 0.0f, 0.0f}, {24.0f, 24.0f}, DARKGREEN);

    for (const Wall& wall : walls) {
        const Vector3 center = Vector3Scale(
            Vector3Add(wall.box.min, wall.box.max), 0.5f);
        const Vector3 size = Vector3Subtract(wall.box.max, wall.box.min);
        DrawCube(center, size.x, size.y, size.z, wall.color);
        DrawCubeWires(center, size.x, size.y, size.z, MAROON);
    }

    for (const Target& target : targets) {
        if (!target.alive) {
            continue;
        }
        DrawCube(target.position, 0.9f, 1.4f, 0.9f, RED);
        DrawCubeWires(target.position, 0.9f, 1.4f, 0.9f, YELLOW);
    }

    EndMode3D();
}
} // namespace

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib 6.0 - FPS Training Arena");
    SetTargetFPS(144);

    enum class GameState { Menu, Playing };
    GameState state = GameState::Menu;

    const std::vector<Wall> walls = CreateMap();
    std::vector<Target> targets = CreateTargets();

    Vector3 playerPosition = {0.0f, 0.05f, 8.0f};
    float verticalVelocity = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    int score = 0;

    while (!WindowShouldClose()) {
        const float deltaTime = std::min(GetFrameTime(), 0.05f);

        if (state == GameState::Menu) {
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state = GameState::Playing;
                playerPosition = {0.0f, 0.05f, 8.0f};
                verticalVelocity = 0.0f;
                yaw = 0.0f;
                pitch = 0.0f;
                score = 0;
                for (Target& target : targets) {
                    target.alive = true;
                    target.position = target.basePosition;
                }
                DisableCursor();
            }

            BeginDrawing();
            ClearBackground(BLACK);
            DrawMenu();
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = GameState::Menu;
            EnableCursor();
            continue;
        }

        const Vector2 mouseDelta = GetMouseDelta();
        yaw += mouseDelta.x * MOUSE_SENSITIVITY;
        pitch = std::clamp(pitch - mouseDelta.y * MOUSE_SENSITIVITY,
                           -1.45f, 1.45f);

        const Vector3 forward = GetForward(yaw, 0.0f);
        const Vector3 right = {std::cos(yaw), 0.0f, -std::sin(yaw)};
        Vector3 movement = {0.0f, 0.0f, 0.0f};

        if (IsKeyDown(KEY_W)) movement = Vector3Add(movement, forward);
        if (IsKeyDown(KEY_S)) movement = Vector3Subtract(movement, forward);
        if (IsKeyDown(KEY_D)) movement = Vector3Add(movement, right);
        if (IsKeyDown(KEY_A)) movement = Vector3Subtract(movement, right);

        if (Vector3Length(movement) > 0.0f) {
            movement = Vector3Scale(
                Vector3Normalize(movement), MOVE_SPEED * deltaTime);
            MovePlayer(playerPosition, movement, walls);
        }

        const bool grounded = playerPosition.y <= 0.051f;
        if (grounded && verticalVelocity < 0.0f) {
            verticalVelocity = 0.0f;
        }
        if (grounded && IsKeyPressed(KEY_SPACE)) {
            verticalVelocity = JUMP_SPEED;
        }

        verticalVelocity -= GRAVITY * deltaTime;
        playerPosition.y += verticalVelocity * deltaTime;
        if (playerPosition.y < 0.05f) {
            playerPosition.y = 0.05f;
            verticalVelocity = 0.0f;
        }

        for (Target& target : targets) {
            if (target.alive) {
                target.position.x = target.basePosition.x +
                    std::sin(static_cast<float>(GetTime()) * 1.2f + target.phase) * 2.0f;
            }
        }

        Camera3D camera = {};
        camera.position = {
            playerPosition.x,
            playerPosition.y + EYE_HEIGHT,
            playerPosition.z
        };
        camera.target = Vector3Add(camera.position, GetForward(yaw, pitch));
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 75.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            ShootNearestTarget(camera, targets)) {
            ++score;
        }

        BeginDrawing();
        ClearBackground(SKYBLUE);
        DrawWorld(camera, walls, targets);

        // Center crosshair.
        DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2.0f, WHITE);
        DrawLine(SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2,
                 SCREEN_WIDTH / 2 + 10, SCREEN_HEIGHT / 2, WHITE);
        DrawLine(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10,
                 SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 10, WHITE);

        DrawRectangle(15, 15, 235, 76, Fade(BLACK, 0.55f));
        DrawText(TextFormat("FPS: %d", GetFPS()), 28, 25, 20, LIME);
        DrawText(TextFormat("Targets: %d / %d", score,
                           static_cast<int>(targets.size())),
                 28, 53, 20, RAYWHITE);
        DrawText("ESC: menu", SCREEN_WIDTH - 120, 20, 18, RAYWHITE);
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}
