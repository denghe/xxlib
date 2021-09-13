#include <iostream>
#include "xx_lua_bind.h"
#include "xx_lua_data.h"
#include "xx_string.h"


// Test result: native performance is pretty good.
// There is no performance improvement in the memory pool written by myself,
// and the direct link to mimalloc has greatly improved

namespace xx {
/*
    lua sample:

    xx::MemPool lmp;
    ...
    auto L = lua_newstate([](void *ud, void *ptr, size_t osize, size_t nsize) {
        return ((xx::MemPool*)ud)->Realloc(ptr, nsize, osize);
    }, &lmp);
*/
    struct MemPool {
        static_assert(sizeof(size_t) == sizeof(void *));
        std::array<void *, sizeof(void *) * 8> blocks{};


        ~MemPool() {
            for (auto b: blocks) {
                while (b) {
                    auto next = *(void **) b;
                    free(b);
                    b = next;
                }
            }
        }

        void *Alloc(size_t siz) {
            assert(siz);
            // reserve header space
            siz += sizeof(void *);

            // align with 2^n
            auto idx = Calc2n(siz);
            if (siz > (size_t(1) << idx)) {
                siz = size_t(1) << ++idx;
            }

            // try get pointer from chain
            auto p = blocks[idx];
            if (p) {
                // store next pointer at header space
                blocks[idx] = *(void **) p;
            } else {
                p = malloc(siz);
            }

            // error: no more free memory?
            if (!p) return nullptr;

            // store idx at header space
            *(size_t *) p = idx;
            return (void **) p + 1;
        }

        void Free(void *const &p) {
            if (!p) return;

            // calc header
            auto h = (void **) p - 1;

            // calc blocks[ index ]
            auto idx = *(size_t *) h;

            // store next pointer at header space
            *(void **) h = blocks[idx];

            // header link
            blocks[idx] = h;
        }

        void *Realloc(void *p, size_t const &newSize, size_t const &dataLen = -1) {
            // free only
            if (!newSize) {
                Free(p);
                return nullptr;
            }
            // alloc only
            if (!p) return Alloc(newSize);

            // begin realloc

            // free part1
            auto h = (void **) p - 1;
            auto idx = *(size_t *) h;

            // calc original cap
            auto cap = (size_t) ((size_t(1) << idx) - sizeof(void *));
            if (cap >= newSize) return p;

            // new & copy data
            auto np = Alloc(newSize);
            memcpy(np, p, std::min(cap, dataLen));

            // free part2
            *(void **) h = blocks[idx];
            blocks[idx] = h;

            return np;
        }
    };


    struct MemPool2 {
        static_assert(sizeof(size_t) == sizeof(void *));
        std::array<std::vector<void *>, sizeof(void *) * 8> blocks{};

        ~MemPool2() {
            for (auto &bs: blocks) {
                for (auto &b: bs) {
                    free((size_t*)b - 1);
                }
            }
        }

        void *Alloc(size_t siz) {
            assert(siz);
            // reserve header space
            siz += sizeof(size_t);

            // align with 2^n
            auto idx = Calc2n(siz);
            if (siz > (size_t(1) << idx)) {
                siz = size_t(1) << ++idx;
            }

            // try get or make
            auto &bs = blocks[idx];
            if (!bs.empty()) {
                auto r = bs.back();
                bs.pop_back();
                return r;
            } else {
                auto p = (size_t *)malloc(siz);
                if (!p) return nullptr;
                *p = idx;
                return p + 1;
            }
        }

        void Free(void *const &p) {
            if (!p) return;
            auto idx = *((size_t*)p - 1);
            blocks[idx].push_back(p);
        }

        void *Realloc(void *p, size_t const &newSize, size_t const &dataLen = -1) {
            // free only
            if (!newSize) {
                Free(p);
                return nullptr;
            }
            // alloc only
            if (!p) return Alloc(newSize);

            // begin realloc

            // free part1
            auto idx = *((size_t*)p - 1);

            // calc original cap
            auto cap = (size_t) ((size_t(1) << idx) - sizeof(void *));
            if (cap >= newSize)
                return p;

            // new & copy data
            auto np = Alloc(newSize);
            memcpy(np, p, std::min(cap, dataLen));

            // free part2
            blocks[idx].push_back(p);

            return np;
        }
    };
}

int main() {


//    xx::MemPool2 lmp;
//    auto L = lua_newstate([](void *ud, void *ptr, size_t osize, size_t nsize) {
//        return ((xx::MemPool2 *) ud)->Realloc(ptr, nsize, osize);
//    }, &lmp);
//    luaL_openlibs(L);

    xx::Lua::State L;

    luaL_dostring(L, R"(
local bt
local t = {}

for j=1,3 do
    bt = os.clock()
    for i=1,1000000 do
        t[#t+1] = {
            {
                a=1,b=2,c=3
            }
        }
    end
    print("insert cost", os.clock() - bt)

    bt = os.clock()
    for i=1,1000000 do
        t[#t+1] = nil
    end
    print("delete cost", os.clock() - bt)

    bt = os.clock()
    collectgarbage()
    print("gc cost", os.clock() - bt)

end

)");

    std::cout << "end." << std::endl;
    return 0;
}

