#pragma once

#include <cassert>
#include <cstring>

namespace xx {

    template<typename T, int firstCap = 16, bool reserve = false>
    struct NodePool {
        static_assert(firstCap > 0);
        struct Node {
            int prev, next;
            T value;
        };
        int freeList = -1;
        int freeCount = 0;
        int count = 0;
        int cap = 0;
        Node *nodes = nullptr;

        NodePool(NodePool const &) = delete;

        NodePool &operator=(NodePool const &) = delete;

        NodePool(NodePool &&o) noexcept {
            operator=(std::move(o));
        }

        NodePool &operator=(NodePool &&o) noexcept {
            std::swap(this->freeList, o.freeList);
            std::swap(this->freeCount, o.freeCount);
            std::swap(this->count, o.count);
            std::swap(this->cap, o.cap);
            std::swap(this->nodes, o.nodes);
            return *this;
        }

        NodePool() {
            if constexpr (reserve) {
                cap = firstCap;
                nodes = (Node *) malloc(nodes, cap);
            }
        }

        ~NodePool() {
            Clear();
            free(nodes);
        }

        void Clear() {
            // foreach example
            for (int i = 0; i < count; ++i) {
                if (nodes[i].prev == -2) continue;
                nodes[i].value.~T();
            }
            freeList = -1;
            freeCount = 0;
            count = 0;
        }

        // warning: prev, next need init
        template<typename...Args>
        int Alloc(Args&&...args) {
            int idx;
            if (freeCount > 0) {
                idx = freeList;
                freeList = nodes[idx].next;
                freeCount--;
            } else {
                if (count == cap) {
                    cap *= 2;
                    if (!cap) {
                        cap = firstCap;
                    }
                    nodes = (Node *) realloc(nodes, cap);
                }
                idx = count;
                count++;
            }
            new(&nodes[idx].value) T(std::forward<Args>(args)...);
            return idx;
        }

        void Free(int const &idx) {
            assert(idx >= 0 && idx < count && nodes[idx].prev != -2);
            nodes[idx].value.~T();
            nodes[idx].next = freeList;
            freeList = idx;
            freeCount++;
            nodes[idx].prev = -2;           // -2: foreach 时的无效标志
        }

        Node &operator[](int const &idx) {
            assert(idx >= 0 && idx < count && nodes[idx].prev != -2);
            return nodes[idx];
        }

        Node const &operator[](int const &idx) const {
            assert(idx >= 0 && idx < count && nodes[idx].prev != -2);
            return nodes[idx];
        }

        [[nodiscard]] int Count() const {
            return count - freeCount;
        }

        [[nodiscard]] bool Empty() const {
            return Count() == 0;
        }
    };
}
