#pragma once
#include "xx_epoll.h"
#include "xx_obj.h"

namespace xx::Epoll {

    // 带超时的回调
    template<typename Peer, typename Func_ = std::function<void(int32_t const& serial_, xx::ObjBase_s &&o)>>
    struct CB : Timer {
        using Func = Func_;
        using Container = std::unordered_map<int32_t, xx::Shared<CB<Peer, Func_>>>;

        Container* container;

        // 序号
        int32_t serial = 0;

        // 回调函数本体
        Func func;

        CB(Context* const& ec, Container *const &container, int32_t const &serial, Func &&cbFunc, double const &timeoutSeconds)
                : Timer(ec, -1), container(container), serial(serial), func(std::move(cbFunc)) {
            SetTimeoutSeconds(timeoutSeconds);
        }

        // 模拟超时 后 从容器移除
        void Timeout() override {
            // 模拟超时
            func(serial, nullptr);
            // 从容器移除
            container->erase(serial);
        }
    };

    // attach base send & recv funcs & ctx.
    template<typename T>
    struct OMExt {
        // 循环自增用于生成 serial
        int32_t autoIncSerial = 0;

        // 产生一个序号. 在正整数范围循环自增( 可能很多天之后会重复 )
        int32_t GenSerial() {
            autoIncSerial = (autoIncSerial + 1) & 0x7FFFFFFF;
            return autoIncSerial;
        }

        // 所有 带超时的回调. key: serial
        std::unordered_map<int32_t, xx::Shared<CB<T>>> callbacks;

        // 发推送
        template<typename PKG = xx::ObjBase, typename ... Args>
        void SendPush(Args const& ... args) {
            // 推送性质的包, serial == 0
            ((T*)this)->template SendResponse<PKG>(0, args...);
        }

        // 发请求（收到相应回应时会触发 cb 执行。超时或断开也会触发，o == nullptr）
        template<typename PKG = xx::ObjBase, typename ... Args>
        int SendRequest(typename CB<T>::Func &&cbfunc, double const &timeoutSeconds, Args const& ... args) {
            // 产生一个序号. 在正整数范围循环自增( 可能很多天之后会重复 )
            auto serial = GenSerial();
            // 创建一个 带超时的回调
            auto &&cb = xx::Make<CB<T>>(((T*)this)->ec, &callbacks, serial, std::move(cbfunc), timeoutSeconds);
            cb->Hold();
            // 以序列号建立cb的映射
            callbacks[serial] = std::move(cb);
            // 发包并返回( 请求性质的包, 序号为负数 )
            ((T*)this)->template SendResponse<PKG>(-serial, args...);
            return serial;
        }

        // 收到回应( 自动调用 发送请求时设置的回调函数 )
        void ReceiveResponse(int32_t const &serial, xx::ObjBase_s &&ob) {
            // 根据序号定位到 cb. 找不到可能是超时或发错?
            auto &&iter = callbacks.find(serial);
            if (iter == callbacks.end()) return;
            iter->second->func(iter->second->serial, std::move(ob));
            iter->second->SetTimeoutSeconds(0);
            callbacks.erase(iter);
        }

        // 触发所有已存在回调（ 模拟超时回调 ） & clear
        void ClearCallbacks() {
            for (auto &&iter : callbacks) {
                iter.second->func(iter.second->serial, nullptr);
                iter.second->SetTimeout(0);
            }
            callbacks.clear();
        }
    };

    struct TypeCounterExt {
        // when receive a package, let typeCounters[typeId]++
        // when handled, typeCounters[typeId]--
        std::array<int8_t, 65536> typeCounters{};
        // return count value
        template<typename T>
        [[nodiscard]] int8_t const& TypeCount() const {
            return typeCounters[xx::TypeId_v<T>];
        }
        // ++count
        template<typename T>
        void TypeCountInc() {
            assert(typeCounters[xx::TypeId_v<T>] >= 0);
            ++typeCounters[xx::TypeId_v<T>];
        }
        // --count
        template<typename T>
        void TypeCountDec() {
            assert(typeCounters[xx::TypeId_v<T>] > 0);
            --typeCounters[xx::TypeId_v<T>];
        }
    };
}
