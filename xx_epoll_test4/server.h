#pragma once
#include "xx_epoll.h"
#include "xx_dict_mk.h"
#include "xx_obj.h"
namespace EP = xx::Epoll;

// 预声明
struct Listener;
struct DB;

//struct SPeer

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // 等待 server 接入的监听器
    xx::Shared<Listener> listener;

    // shared database handler
    xx::Shared<DB> db;

    // shared obj manager
    xx::ObjManager om;

    // 根据 config 进一步初始化各种成员
    int Init();

    /**************************************************************************************/
    // caches

    // package instance cache for Send Push / Response ( can't hold )
    std::array<xx::ObjBase_s, std::numeric_limits<uint16_t>::max()> objs{};

    // return cached package instance
    template<typename T>
    xx::Shared<T> const& FromCache() {
        assert((*(xx::Shared<T>*)&objs[xx::TypeId_v<T>]).useCount() == 1);
        return *(xx::Shared<T>*)&objs[xx::TypeId_v<T>];
    }
};
