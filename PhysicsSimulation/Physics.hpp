#pragma once

#include <glm/glm.hpp>
#include <shared_mutex>

#include "ThreadPool.h"
#include "Grid.hpp"
#include "PhysicalObject.hpp"

class PhysicsSolver
{
public:
    //std::shared_mutex mtx; // 如果需要无锁，那么物理计算线程与渲染线程应当一致

    std::vector<SimplePhysicalObject> objects;
    //std::vector<PhysicalObject> objects;

    SafeSimpleThreadPool& threadPool;
    //FastThreadPool& threadPool;
    Grid grid;
    glm::vec2 world_size;
    glm::vec2 gravity = { 0.0f, 5.0f };

    std::vector<std::future<void>> rets;

    size_t sub_steps;

    PhysicsSolver(glm::vec2 size, SafeSimpleThreadPool& threadPool) :
        grid(static_cast<int32_t>(size.x), static_cast<int32_t>(size.y)),
        world_size(size.x, size.y),
        sub_steps(8),
        threadPool(threadPool) {
        grid.clear();
        rets.reserve(threadPool.getThreadCount());
    }

    void solveContact(size_t atom1_id, size_t atom2_id) {
        constexpr float response_coef = 1.0f;
        constexpr float eps = 0.0001f;
        SimplePhysicalObject* obj1, * obj2;
        glm::vec2 o2_to_o1;
        float dist2;
        {
            //std::shared_lock<std::shared_mutex> lk(mtx);
            obj1 = &objects[atom1_id];
            obj2 = &objects[atom2_id];
            o2_to_o1 = obj1->position - obj2->position;
            dist2 = o2_to_o1.x * o2_to_o1.x + o2_to_o1.y * o2_to_o1.y;
        }
        if (dist2 < 1.0f && dist2 > eps) {
            //std::unique_lock<std::shared_mutex> lk(mtx);
            const float dist = sqrt(dist2);
            const float delta = response_coef * 0.5f * (1.0f - dist); // 长度是一半
            const glm::vec2 col_vec = (o2_to_o1 / dist) * delta; // 计算退行向量
            obj1->position += col_vec;
            obj2->position -= col_vec;
        }
    }
    
    void checkAtomCellCollisions(size_t atom_id, const CollisionCell& c) {
        for (size_t i = 0; i < c.objects_count; ++i) {
            solveContact(atom_id, c.objects[i]);
        }
    }

    void checkCellCollision(const CollisionCell& c, size_t index) {
        for (size_t i = 0; i < c.objects_count; ++i) {
            const size_t atom_idx = c.objects[i];
            // 当前grid内以及周边8个grid的碰撞情况
            checkAtomCellCollisions(atom_idx, grid.data[index - 1]); // 左
            checkAtomCellCollisions(atom_idx, grid.data[index]);
            checkAtomCellCollisions(atom_idx, grid.data[index + 1]); // 右
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height - 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height]); // 下
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height + 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height - 1]);
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height]); // 上
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height + 1]);
        }
    }

    void solveCollisionSingleThread(size_t i, size_t slice_size) {
        const size_t start = i * slice_size;
        const size_t end = (i + 1) * slice_size;
        for (size_t idx = start; idx < end; ++idx) {
            checkCellCollision(grid.data[idx], idx);
        }
    }

    void solveCollisions() {
        const size_t thread_count = static_cast<size_t>(threadPool.getThreadCount());
        const size_t slice_count = thread_count * 2;
        const size_t slice_size = (grid.width / slice_count) * grid.height;
        //std::vector<std::future<void>> rets;
        //rets.reserve(thread_count);
        rets.clear();
        for (size_t i = 0; i < thread_count; ++i) {
            rets.emplace_back(
                threadPool.enqueue(
                    [this, i, slice_size]() {
                        solveCollisionSingleThread(2 * i, slice_size);
                    }
                )
            );
        }
        //threadPool.waitForCompletion();
        for (auto& ret : rets)
            ret.wait();
        rets.clear();
        for (size_t i = 0; i < thread_count; ++i) {
            rets.emplace_back(
                threadPool.enqueue(
                    [this, i, slice_size]() {
                        solveCollisionSingleThread(2 * i + 1, slice_size);
                    }
                )
            );
        }
        //threadPool.waitForCompletion();
        for (auto& ret : rets)
            ret.wait();
    }

    size_t add(const SimplePhysicalObject& obj) {
        //std::unique_lock<std::shared_mutex> lk(mtx);
        objects.push_back(obj);
        return objects.size() - 1;
    }

    size_t create(glm::vec2 pos) {
        //std::unique_lock<std::shared_mutex> lk(mtx);
        objects.emplace_back(pos);
        return objects.size() - 1;
    }

    void update(float deltaTime) {
        const float sub_dt = deltaTime / static_cast<float>(sub_steps);
        using namespace std::chrono;
        for (size_t i = sub_steps; i > 0; --i) {
            addObjectsToGrid();
            solveCollisions();
            updateObjects_multi(sub_dt);
        }
    }

    void addObjectsToGrid() {
        grid.clear();
        size_t i = 0;
        for (const auto& obj : objects) {
            if (obj.position.x > 1.0f && obj.position.x < world_size.x - 1.0f
                && obj.position.y > 1.0f && obj.position.y < world_size.y - 1.0f) {
                grid.add(static_cast<size_t>(obj.position.x), static_cast<size_t>(obj.position.y), i);
            }
            i++;
        }
    }

    void updateObjects_multi(float deltaTime) {
        /*
        static std::function<void(size_t, size_t)> func = [this, deltaTime](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                SimplePhysicalObject& obj = objects[i];
                obj.acceleration += gravity;
                obj.Update(deltaTime);
                static constexpr float margin = 2.0f;
                if (obj.position.x > world_size.x - margin) {
                    obj.position.x = world_size.x - margin;
                }
                else if (obj.position.x < margin) {
                    obj.position.x = margin;
                }
                if (obj.position.y > world_size.y - margin) {
                    obj.position.y = world_size.y - margin;
                }
                else if (obj.position.y < margin) {
                    obj.position.y = margin;
                }
            }
            };

        size_t count = static_cast<size_t>(objects.size());
        size_t batch_size = count / threadPool.getThreadCount();
        //std::vector<std::future<void>> rets;
        rets.clear();
        for (size_t i = 0; i < threadPool.getThreadCount(); ++i) {
            const size_t start = batch_size * i;
            const size_t end = start + batch_size;
            rets.emplace_back(
                threadPool.enqueue(
                std::bind(func, start, end)
            )
            );
        }
        if (batch_size * threadPool.getThreadCount() < count) {
            const size_t start = batch_size * threadPool.getThreadCount();
            func(start, count);
        }

        for (auto& ret : rets)
            ret.wait();
        */

        
        threadPool.dispatch
        (
            objects.size(),
            [&](size_t start, size_t end) {
                for (size_t i = start; i < end; ++i) {
                    SimplePhysicalObject& obj = objects[i];
                    //PhysicalObject& obj = objects[i];
                    obj.acceleration += gravity;
                    obj.Update(deltaTime);
                    const float margin = 2.0f;
                    if (obj.position.x > world_size.x - margin) {
                        obj.position.x = world_size.x - margin;
                    }
                    else if (obj.position.x < margin) {
                        obj.position.x = margin;
                    }
                    if (obj.position.y > world_size.y - margin) {
                        obj.position.y = world_size.y - margin;
                    }
                    else if (obj.position.y < margin) {
                        obj.position.y = margin;
                    }
                }
            }
        );
        
    }
};

class NewPhysicsSolver
{
public:
    //std::shared_mutex mtx; // 如果需要无锁，那么物理计算线程与渲染线程应当一致

    //std::vector<SimplePhysicalObject> objects;
    std::vector<PhysicalObject> objects;

    SafeSimpleThreadPool& threadPool;
    //FastThreadPool& threadPool;
    Grid grid;
    glm::vec2 world_size;
    glm::vec2 gravity = { 0.0f, 9.8f };

    std::vector<std::future<void>> rets;

    static constexpr float energyLossRate = 0.1f;

    size_t sub_steps;

    NewPhysicsSolver(glm::vec2 size, SafeSimpleThreadPool& threadPool) :
        grid(static_cast<int32_t>(size.x), static_cast<int32_t>(size.y)),
        world_size(size.x, size.y),
        sub_steps(8),
        threadPool(threadPool) {
        grid.clear();
        rets.reserve(threadPool.getThreadCount());
    }

    void solveContact(size_t atom1_id, size_t atom2_id, const float& deltaTime) {
        constexpr float response_coef = 1.0f;
        constexpr float eps = 0.0001f;

        //if (atom1_id == atom2_id)return;

        PhysicalObject* obj1, * obj2;
        glm::vec2 o2_to_o1;
        float dist2;
        {
            //std::shared_lock<std::shared_mutex> lk(mtx);
            obj1 = &objects[atom1_id];
            obj2 = &objects[atom2_id];
            o2_to_o1 = obj1->position - obj2->position;
            dist2 = o2_to_o1.x * o2_to_o1.x + o2_to_o1.y * o2_to_o1.y;
        }
        if (dist2 < 1.0f && dist2 > eps) {
            //std::unique_lock<std::shared_mutex> lk(mtx);
            const float dist = sqrt(dist2);
            const float delta = response_coef * 0.5f * (1.0f - dist); // 长度是一半
            const glm::vec2 col_vec = (o2_to_o1 / dist) * delta; // 计算退行向量
            obj1->position += col_vec;
            obj2->position -= col_vec;

            // 动量守恒
            const float& m1 = obj1->mass, & m2 = obj2->mass;
            const glm::vec2&& v1 = obj1->GetVelocity(deltaTime), && v2 = obj2->GetVelocity(deltaTime);
            if (glm::length(v1) > 1.0e-2f && glm::length(v2) > 1.0e-2f) {
                const glm::vec2&& newVel1 = (m1 - m2) / (m1 + m2) * v1 + 2.0f * m2 / (m1 + m2) * v2;
                const glm::vec2&& newVel2 = 2.0f * m1 / (m1 + m2) * v1 + (m2 - m1) / (m1 + m2) * v2;

                const glm::vec2 I1 = m1 * (newVel1 - v1), I2 = m2 * (newVel2 - v2);
                obj1->OnForce(I1 / deltaTime * (1.0f - energyLossRate), deltaTime);
                obj2->OnForce(I2 / deltaTime * (1.0f - energyLossRate), deltaTime);
            }
        }
    }

    template<typename F>
    void checkAtomCellCollisions(size_t atom_id, const CollisionCell& c, F&& f) {
        for (size_t i = 0; i < c.objects_count; ++i) {
            f(atom_id, c.objects[i]);
        }
    }

    template<typename F>
    void checkCellCollision(const CollisionCell& c, size_t index, F&& f) {
        for (size_t i = 0; i < c.objects_count; ++i) {
            const size_t atom_idx = c.objects[i];
            // 当前grid内以及周边8个grid的碰撞情况
            checkAtomCellCollisions(atom_idx, grid.data[index - 1], std::forward<F>(f)); // 左
            checkAtomCellCollisions(atom_idx, grid.data[index], std::forward<F>(f));
            checkAtomCellCollisions(atom_idx, grid.data[index + 1], std::forward<F>(f)); // 右
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height - 1], std::forward<F>(f));
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height], std::forward<F>(f)); // 下
            checkAtomCellCollisions(atom_idx, grid.data[index + grid.height + 1], std::forward<F>(f));
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height - 1], std::forward<F>(f));
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height], std::forward<F>(f)); // 上
            checkAtomCellCollisions(atom_idx, grid.data[index - grid.height + 1], std::forward<F>(f));
        }
    }

    template<typename F>
    void solveCollisionSingleThread(size_t i, size_t slice_size, F&& f) {
        const size_t start = i * slice_size;
        const size_t end = (i + 1) * slice_size;
        for (size_t idx = start; idx < end; ++idx) {
            checkCellCollision(grid.data[idx], idx, std::forward<F>(f));
        }
    }

    template<typename F>
    void solveCollisions(F&& collisionCheck) {
        const size_t thread_count = static_cast<size_t>(threadPool.getThreadCount());
        const size_t slice_count = thread_count * 2;
        const size_t slice_size = (grid.width / slice_count) * grid.height;
        //std::vector<std::future<void>> rets;
        //rets.reserve(thread_count);
        rets.clear();
        for (size_t i = 0; i < thread_count; ++i) {
            rets.emplace_back(
                threadPool.enqueue(
                    [this, i, slice_size, &collisionCheck]() {
                        solveCollisionSingleThread(2 * i, slice_size, std::forward<F>(collisionCheck));
                    }
                )
            );
        }
        //threadPool.waitForCompletion();
        for (auto& ret : rets)
            ret.wait();
        rets.clear();
        for (size_t i = 0; i < thread_count; ++i) {
            rets.emplace_back(
                threadPool.enqueue(
                    [this, i, slice_size, &collisionCheck]() {
                        solveCollisionSingleThread(2 * i + 1, slice_size, std::forward<F>(collisionCheck));
                    }
                )
            );
        }
        //threadPool.waitForCompletion();
        for (auto& ret : rets)
            ret.wait();
    }

    size_t add(const PhysicalObject& obj) {
        objects.push_back(obj);
        return objects.size() - 1;
    }

    size_t create(glm::vec2 pos) {
        //std::unique_lock<std::shared_mutex> lk(mtx);
        static float weight = 1.0f;
        objects.emplace_back(pos, weight, gravity);
        weight += 0.01f;
        return objects.size() - 1;
    }

    void update(float deltaTime) {
        const float sub_dt = deltaTime / static_cast<float>(sub_steps);
        using namespace std::chrono;
        for (size_t i = sub_steps; i > 0; --i) {
            addObjectsToGrid();
            solveCollisions([this, deltaTime](size_t atom1_id, size_t atom2_id) {
                solveContact(atom1_id, atom2_id, deltaTime);
                            });
            updateObjects_multi(sub_dt);
        }
    }

    void addObjectsToGrid() {
        grid.clear();
        size_t i = 0;
        for (const auto& obj : objects) {
            if (obj.position.x > 1.0f && obj.position.x < world_size.x - 1.0f
                && obj.position.y > 1.0f && obj.position.y < world_size.y - 1.0f) {
                grid.add(static_cast<size_t>(obj.position.x), static_cast<size_t>(obj.position.y), i);
            }
            i++;
        }
    }

    void updateObjects_multi(float deltaTime) {
        /*
        static std::function<void(size_t, size_t)> func = [this, deltaTime](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                SimplePhysicalObject& obj = objects[i];
                obj.acceleration += gravity;
                obj.Update(deltaTime);
                static constexpr float margin = 2.0f;
                if (obj.position.x > world_size.x - margin) {
                    obj.position.x = world_size.x - margin;
                }
                else if (obj.position.x < margin) {
                    obj.position.x = margin;
                }
                if (obj.position.y > world_size.y - margin) {
                    obj.position.y = world_size.y - margin;
                }
                else if (obj.position.y < margin) {
                    obj.position.y = margin;
                }
            }
            };

        size_t count = static_cast<size_t>(objects.size());
        size_t batch_size = count / threadPool.getThreadCount();
        //std::vector<std::future<void>> rets;
        rets.clear();
        for (size_t i = 0; i < threadPool.getThreadCount(); ++i) {
            const size_t start = batch_size * i;
            const size_t end = start + batch_size;
            rets.emplace_back(
                threadPool.enqueue(
                std::bind(func, start, end)
            )
            );
        }
        if (batch_size * threadPool.getThreadCount() < count) {
            const size_t start = batch_size * threadPool.getThreadCount();
            func(start, count);
        }

        for (auto& ret : rets)
            ret.wait();
        */


        threadPool.dispatch
        (
            objects.size(),
            [&](size_t start, size_t end) {
                for (size_t i = start; i < end; ++i) {
                    PhysicalObject& obj = objects[i];
                    obj.acceleration += gravity;
                    obj.Update(deltaTime);
                    const float margin = 2.0f;
                    if (obj.position.x > world_size.x - margin) {
                        obj.position.x = world_size.x - margin;
                        const glm::vec2&& velocity = obj.GetVelocity(deltaTime);
                        const glm::vec2 newVel = { -velocity.x, velocity.y };
                        obj.OnForce((newVel - velocity) * obj.mass / deltaTime * (1.0f - energyLossRate), deltaTime);
                    }
                    else if (obj.position.x < margin) {
                        obj.position.x = margin;
                        const glm::vec2&& velocity = obj.GetVelocity(deltaTime);
                        const glm::vec2 newVel = { -velocity.x, velocity.y };
                        obj.OnForce((newVel - velocity) * obj.mass / deltaTime * (1.0f - energyLossRate), deltaTime);
                    }
                    if (obj.position.y > world_size.y - margin) {
                        obj.position.y = world_size.y - margin;
                        const glm::vec2&& velocity = obj.GetVelocity(deltaTime);
                        const glm::vec2 newVel = { velocity.x, -velocity.y };
                        obj.OnForce((newVel - velocity) * obj.mass / deltaTime * (1.0f - energyLossRate), deltaTime);
                    }
                    else if (obj.position.y < margin) {
                        obj.position.y = margin;
                        const glm::vec2&& velocity = obj.GetVelocity(deltaTime);
                        const glm::vec2 newVel = { velocity.x, -velocity.y };
                        obj.OnForce((newVel - velocity) * obj.mass / deltaTime * (1.0f - energyLossRate), deltaTime);
                    }
                }
            }
        );

    }
};