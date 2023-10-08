#pragma once
#include <vector>

class CollisionCell
{
public:
    static constexpr uint8_t cell_capacity = 4;
    static constexpr uint8_t max_cell_id = cell_capacity - 1;

    uint32_t objects_count = 0;
    uint32_t objects[cell_capacity] = {};

    CollisionCell() = default;

    void add(uint32_t id) {
        objects[objects_count] = id;
        objects_count += objects_count < max_cell_id;
    }

    void clear() {
        objects_count = 0;
    }

    void remove(uint32_t id) {
        for (uint32_t i = 0; i < objects_count; ++i) {
            if (objects[i] == id) {
                objects[i] = objects[objects_count - 1];
                --objects_count;
                return;
            }
        }
    }
};

class Grid
{
public:
    int32_t width, height;
    std::vector<CollisionCell> data;

    Grid() :width(0), height(0) {}
    Grid(int32_t w, int32_t h) :width(w), height(h) {
        data.resize(width * height);
    }

    void add(size_t x, size_t y, uint32_t id) {
        data[x * height + y].add(id);
    }

    void clear() {
        for (auto& c : data)
            c.clear();
    }
};