#pragma once

#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace nz::walkable {

constexpr int Width = 640, Height = 360;
constexpr float Scale = 26.f * .70710678f;
constexpr Vector2 Origin{320, 100};

inline Vector2 Project(float x, float y, float z = 0) {
    return {Origin.x + Scale * (y - x), Origin.y + Scale * .5f * (x + y) - 22.51666f * z};
}

struct Prop {
    Rectangle footprint;
    float height;
    Color color;
};

struct Room {
    const char* id;
    const char* title;
    const char* speaker;
    const char* line;
    Color floor;
    Vector2 npc;
    std::array<Prop, 2> props;
};

constexpr Room Rooms[] = {
    {"garage", "Garage", "Mechanic", "Keep the bay clear. We have a car coming in tonight.",
     {48, 61, 68, 255}, {2, 6}, {{{{1, 1, 3, 1}, 1, DARKBLUE}, {{5, 3, 2, 4}, 1.25f, MAROON}}}},
    {"dealer", "Dealer", "Salesperson", "Take a look around. Ask me about the display cars.",
     {60, 68, 84, 255}, {2, 6}, {{{{1, 1, 3, 1}, 1, DARKBLUE}, {{5, 3, 2, 4}, 1, DARKPURPLE}}}},
    {"pa", "Parking Area", "Rival", "I will be here after the next run if you want to talk.",
     {54, 58, 64, 255}, {2, 6}, {{{{1, 1, 3, 1}, .65f, DARKGRAY}, {{5, 3, 2, 4}, 1, DARKGREEN}}}},
};

inline bool CanStand(const Room& room, Vector2 feet) {
    constexpr float Radius = .23f;
    if (feet.x < Radius || feet.x > 10 - Radius || feet.y < Radius || feet.y > 10 - Radius)
        return false;
    for (const auto& prop : room.props)
        if (CheckCollisionCircleRec(feet, Radius, prop.footprint)) return false;
    const float dx = feet.x - room.npc.x, dy = feet.y - room.npc.y;
    return dx * dx + dy * dy >= .46f * .46f;
}

inline bool CanTalk(const Room& room, Vector2 feet) {
    const float dx = feet.x - room.npc.x, dy = feet.y - room.npc.y;
    return dx * dx + dy * dy <= 1.25f * 1.25f;
}

inline void Quad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color) {
    DrawTriangle(a, b, c, color);
    DrawTriangle(a, c, d, color);
}

inline void Block(Rectangle r, float height, Color color) {
    const float right = r.x + r.width, bottom = r.y + r.height;
    const Vector2 a = Project(r.x, r.y, height), b = Project(r.x, bottom, height);
    const Vector2 c = Project(right, bottom, height), d = Project(right, r.y, height);
    const Vector2 cb = Project(right, bottom);
    Quad(b, c, cb, Project(r.x, bottom), ColorBrightness(color, -.20f));
    Quad(c, d, Project(right, r.y), cb, ColorBrightness(color, -.34f));
    Quad(a, d, c, b, color);
}

inline void Person(Vector2 feet, Color coat) {
    const Vector2 p = Project(feet.x, feet.y);
    DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y), 6, 3, Fade(BLACK, .5f));
    DrawRectangle(static_cast<int>(p.x) - 4, static_cast<int>(p.y) - 12, 8, 12, DARKGRAY);
    DrawRectangle(static_cast<int>(p.x) - 6, static_cast<int>(p.y) - 28, 12, 19, coat);
    DrawCircleV({p.x, p.y - 32}, 5, {237, 188, 145, 255});
}

inline void LoadLayers(const Room& room, std::array<Texture2D, 3>& layers) {
    constexpr const char* Names[] = {"base", "prop_1", "prop_2"};
    for (size_t i = 0; i < layers.size(); ++i) {
        if (layers[i].id) UnloadTexture(layers[i]);
        const char* path = TextFormat("assets/scenes/%s/%s.png", room.id, Names[i]);
        layers[i] = FileExists(path) ? LoadTexture(path) : Texture2D{};
        if (layers[i].id && (layers[i].width != Width || layers[i].height != Height)) {
            TraceLog(LOG_WARNING, "Invalid scene layer size: %s", path);
            UnloadTexture(layers[i]);
            layers[i] = {};
        }
    }
}

inline void DrawRoom(const Room& room, const std::array<Texture2D, 3>& layers,
                     Vector2 player, bool talking) {
    ClearBackground({14, 22, 31, 255});
    if (layers[0].id) {
        DrawTexture(layers[0], 0, 0, WHITE);
    } else {
        Quad(Project(0, 0), Project(10, 0), Project(10, 10), Project(0, 10), room.floor);
        for (int i = 0; i <= 10; ++i) {
            const float line = static_cast<float>(i);
            DrawLineV(Project(line, 0), Project(line, 10), Fade(RAYWHITE, .18f));
            DrawLineV(Project(0, line), Project(10, line), Fade(RAYWHITE, .18f));
        }
    }

    const auto& a = room.props[0].footprint;
    const auto& b = room.props[1].footprint;
    struct DrawItem { float depth; int index; };
    std::array<DrawItem, 4> items{{
        {a.x + a.width + a.y + a.height, 0}, {b.x + b.width + b.y + b.height, 1},
        {room.npc.x + room.npc.y, 2}, {player.x + player.y, 3},
    }};
    std::stable_sort(items.begin(), items.end(),
                     [](DrawItem a, DrawItem b) { return a.depth < b.depth; });
    for (const auto item : items) {
        if (item.index == 2) Person(room.npc, GOLD);
        else if (item.index == 3) Person(player, SKYBLUE);
        else if (layers[item.index + 1].id) DrawTexture(layers[item.index + 1], 0, 0, WHITE);
        else {
            const auto& prop = room.props[item.index];
            Block(prop.footprint, prop.height, prop.color);
        }
    }

    DrawText(room.title, 16, 13, 22, RAYWHITE);
    DrawText("WASD / arrows   |   E talk   |   1 garage   2 dealer   3 PA", 16, 41, 12, LIGHTGRAY);
    if (talking) {
        DrawRectangle(12, 300, 616, 52, {20, 31, 46, 255});
        DrawText(room.speaker, 22, 306, 14, GOLD);
        DrawText(room.line, 22, 326, 14, RAYWHITE);
    } else if (CanTalk(room, player)) {
        DrawText(TextFormat("E  Talk to %s", room.speaker), 16, 326, 16, GOLD);
    }
}

inline int Run(bool smoke) {
    if (smoke) {
        for (const auto& room : Rooms)
            if (!CanStand(room, {8, 8}) || CanStand(room, {-.1f, 4}) ||
                CanStand(room, {5.5f, 4}) || CanStand(room, room.npc) ||
                !CanTalk(room, {2, 7}) || CanTalk(room, {8, 8})) return 1;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | (smoke ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(1280, 720, "Number Zero | Walkable scenes");
    SetWindowMinSize(640, 360);
    SetTargetFPS(60);
    const RenderTexture2D canvas = LoadRenderTexture(Width, Height);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    int roomIndex = 0;
    Vector2 player = smoke ? Vector2{2, 7} : Vector2{8, 8};
    bool talking = smoke;
    std::array<Texture2D, 3> layers{};
    LoadLayers(Rooms[roomIndex], layers);
    do {
        for (int i = 0; i < 3; ++i) {
            if (!IsKeyPressed(KEY_ONE + i) || i == roomIndex) continue;
            roomIndex = i;
            player = {8, 8};
            talking = false;
            LoadLayers(Rooms[roomIndex], layers);
        }
        const auto& room = Rooms[roomIndex];
        if (IsKeyPressed(KEY_E) && CanTalk(room, player)) talking = !talking;
        const int horizontal = (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
                               (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
        const int vertical = (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) -
                             (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
        if (!talking && (horizontal || vertical)) {
            const float dx = vertical - horizontal, dy = vertical + horizontal;
            const float step = 3.f * std::min(GetFrameTime(), .05f) / std::hypot(dx, dy);
            if (CanStand(room, {player.x + dx * step, player.y})) player.x += dx * step;
            if (CanStand(room, {player.x, player.y + dy * step})) player.y += dy * step;
        }

        BeginTextureMode(canvas);
        DrawRoom(room, layers, player, talking);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        const float scale = std::min(GetScreenWidth() / float(Width), GetScreenHeight() / float(Height));
        const float w = Width * scale, h = Height * scale;
        DrawTexturePro(canvas.texture, {0, 0, Width, -Height},
                       {(GetScreenWidth() - w) * .5f, (GetScreenHeight() - h) * .5f, w, h}, {}, 0, WHITE);
        EndDrawing();
    } while (!smoke && !WindowShouldClose());
    bool saved = true;
    if (smoke) {
        Image image = LoadImageFromTexture(canvas.texture);
        ImageFlipVertical(&image);
        saved = ExportImage(image, "build/walkable-scenes-smoke.png");
        UnloadImage(image);
    }
    for (const auto layer : layers)
        if (layer.id) UnloadTexture(layer);
    UnloadRenderTexture(canvas);
    CloseWindow();
    return saved ? 0 : 1;
}

} // namespace nz::walkable

