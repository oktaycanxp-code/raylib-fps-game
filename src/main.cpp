#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr int ScreenWidth = 1280;
constexpr int ScreenHeight = 720;
constexpr float PlayerRadius = 0.35f;
constexpr float PlayerHeight = 1.8f;
constexpr float EyeHeight = 1.55f;
constexpr float Gravity = 22.0f;
constexpr float JumpSpeed = 8.5f;

struct Wall {
    BoundingBox box;
    Color color;
};

struct Target {
    Vector3 position;
    float phase;
    bool alive = true;
};

BoundingBox PlayerBox(Vector3 position) {
    return {{position.x - PlayerRadius, position.y, position.z - PlayerRadius},
            {position.x + PlayerRadius, position.y + PlayerHeight,
             position.z + PlayerRadius}};
}

bool Collides(Vector3 position, const std::vector<Wall>& walls) {
    const BoundingBox player = PlayerBox(position);
    for (const Wall& wall : walls) {
        if (CheckCollisionBoxes(player, wall.box)) return true;
    }
    return false;
}

void MovePlayer(Vector3& position, Vector3 delta, const std::vector<Wall>& walls) {
    Vector3 next = position;
    next.x += delta.x;
    if (!Collides(next, walls)) position.x = next.x;

    next = position;
    next.z += delta.z;
    if (!Collides(next, walls)) position.z = next.z;
}

Vector3 Forward(float yaw, float pitch) {
    const float cp = cosf(pitch);
    return Vector3Normalize({sinf(yaw) * cp, sinf(pitch), cosf(yaw) * cp});
}

bool ShootTarget(const Camera3D& camera, std::vector<Target>& targets) {
    Ray ray = GetScreenToWorldRay({ScreenWidth / 2.0f, ScreenHeight / 2.0f}, camera);
    bool hit = false;
    for (Target& target : targets) {
        if (!target.alive) continue;
        BoundingBox box = {{target.position.x - 0.45f, target.position.y - 0.7f,
                           target.position.z - 0.45f},
                          {target.position.x + 0.45f, target.position.y + 0.7f,
                           target.position.z + 0.45f}};
        RayCollision collision = GetRayCollisionBox(ray, box);
        if (collision.hit && (!hit || collision.distance < 10000.0f)) {
            target.alive = false;
            hit = true;
        }
    }
    return hit;
}

void DrawMenu() {
    DrawRectangle(0, 0, ScreenWidth, ScreenHeight, Color{12, 18, 30, 255});
    DrawText("RAYLIB FPS", ScreenWidth / 2 - MeasureText("RAYLIB FPS", 64) / 2,
             145, 64, SKYBLUE);
    DrawText("3D TRAINING ARENA", ScreenWidth / 2 - MeasureText("3D TRAINING ARENA", 24) / 2,
             225, 24, LIGHTGRAY);
    DrawRectangle(ScreenWidth / 2 - 150, 335, 300, 70, BLUE);
    DrawText("PLAY", ScreenWidth / 2 - MeasureText("PLAY", 32) / 2, 353, 32, RAYWHITE);
    DrawText("WASD move   |   Mouse look   |   Space jump", ScreenWidth / 2 - 255,
             500, 20, GRAY);
    DrawText("Press ENTER or click PLAY", ScreenWidth / 2 - 140, 555, 20, LIGHTGRAY);
}
} // namespace

int main() {
    InitWindow(ScreenWidth, ScreenHeight, "Raylib 6.0 - FPS Training Arena");
    SetTargetFPS(144);

    enum class GameState { Menu, Playing };
    GameState state = GameState::Menu;

    std::vector<Wall> walls = {
        {{{-12, -0.1f, -12}, {12, 0.0f, 12}}, DARKGRAY}, // floor slab
        {{{-12, 0, -12}, {12, 3, -11.5f}}, GRAY},
        {{{-12, 0, 11.5f}, {12, 3, 12}}, GRAY},
        {{{-12, 0, -12}, {-11.5f, 3, 12}}, GRAY},
        {{{11.5f, 0, -12}, {12, 3, 12}}, GRAY},
        {{{-3, 0, -2}, {-2.2f, 2.2f, 3}}, BROWN},
        {{{3, 0, 1}, {7, 1.4f, 2}}, BROWN},
        {{{-8, 0, 6}, {-4, 1.2f, 7}}, BROWN},
    };
    std::vector<Target> targets = {{{-5, 1.4f, -4}, 0.0f}, {{5, 1.4f, -5}, 1.7f},
                                   {{6, 1.4f, 7}, 3.0f}, {{-7, 1.4f, 3}, 4.2f}};

    Vector3 player = {0, 0.05f, 8};
    float verticalVelocity = 0;
    float yaw = 0;
    float pitch = 0;
    bool grounded = false;
    int score = 0;

    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.05f);

        if (state == GameState::Menu) {
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state = GameState::Playing;
                player = {0, 0.05f, 8};
                verticalVelocity = 0;
                score = 0;
                for (Target& target : targets) target.alive = true;
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
        }

        Vector2 mouse = GetMouseDelta();
        yaw += mouse.x * 0.0025f;
        pitch = std::clamp(pitch - mouse.y * 0.0025f, -1.45f, 1.45f);

        Vector3 forward = Forward(yaw, 0);
        Vector3 right = {cosf(yaw), 0, -sinf(yaw)};
        Vector3 move = {0, 0, 0};
        if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
        if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
        if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
        if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
        if (Vector3Length(move) > 0) move = Vector3Scale(Vector3Normalize(move), 5.0f * dt);
        MovePlayer(player, move, walls);

        grounded = player.y <= 0.051f;
        if (grounded && verticalVelocity < 0) verticalVelocity = 0;
        if (grounded && IsKeyPressed(KEY_SPACE)) verticalVelocity = JumpSpeed;
        verticalVelocity -= Gravity * dt;
        float oldY = player.y;
        player.y += verticalVelocity * dt;
        if (player.y < 0.05f) {
            player.y = 0.05f;
            verticalVelocity = 0;
        }
        (void)oldY;

        for (Target& target : targets) {
            if (target.alive) target.position.x = sinf(GetTime() * 1.2f + target.phase) * 2.0f +
                                                    (target.phase < 2 ? -5.0f : 5.0f);
        }

        Camera3D camera = {};
        camera.position = {player.x, player.y + EyeHeight, player.z};
        camera.target = Vector3Add(camera.position, Forward(yaw, pitch));
        camera.up = {0, 1, 0};
        camera.fovy = 75;
        camera.projection = CAMERA_PERSPECTIVE;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ShootTarget(camera, targets)) ++score;

        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);
        DrawPlane({0, 0, 0}, {24, 24}, DARKGREEN);
        for (const Wall& wall : walls) {
            if (wall.box.min.y < 0.0f) continue;
            Vector3 center = Vector3Scale(Vector3Add(wall.box.min, wall.box.max), 0.5f);
            Vector3 size = Vector3Subtract(wall.box.max, wall.box.min);
            DrawCube(center, size.x, size.y, size.z, wall.color);
            DrawCubeWires(center, size.x, size.y, size.z, MAROON);
        }
        for (const Target& target : targets) {
            if (!target.alive) continue;
            DrawCube(target.position, 0.9f, 1.4f, 0.9f, RED);
            DrawCubeWires(target.position, 0.9f, 1.4f, 0.9f, YELLOW);
        }
        EndMode3D();

        DrawCircle(ScreenWidth / 2, ScreenHeight / 2, 2, WHITE);
        DrawLine(ScreenWidth / 2 - 10, ScreenHeight / 2, ScreenWidth / 2 + 10,
                 ScreenHeight / 2, WHITE);
        DrawLine(ScreenWidth / 2, ScreenHeight / 2 - 10, ScreenWidth / 2,
                 ScreenHeight / 2 + 10, WHITE);
        DrawRectangle(15, 15, 220, 74, Fade(BLACK, 0.55f));
        DrawText(TextFormat("FPS: %d", GetFPS()), 28, 25, 20, LIME);
        DrawText(TextFormat("Targets: %d / %d", score, (int)targets.size()), 28, 53, 20, RAYWHITE);
        DrawText("ESC: menu", ScreenWidth - 120, 20, 18, RAYWHITE);
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}
