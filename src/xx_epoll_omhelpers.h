#pragma once
#include "xx_epoll.h"
#include "xx_obj.h"

namespace xx::Epoll {

    // 带超时的回调
    template<typename Peer, typename Func_ = std::function<void(xx::ObjBase_s &&o)>>
    struct CB : Timer {
        using Func = Func_;
        using Container = std::unordered_map<int32_t, xx::Shared<CB<Peer, Func_>>>;

        Container* container;

        // 序号( 移除的时候要用到 )
        int32_t serial = 0;

        // 回调函数本体
        Func func;

        CB(Context* const& ec, Container *const &container, int32_t const &serial, Func &&cbFunc, double const &timeoutSeconds)
                : Timer(ec, -1), container(container), serial(serial), func(std::move(cbFunc)) {
            SetTimeoutSeconds(timeoutSeconds);
        }

        // 执行 func(0,0) 后 从容器移除
        void Timeout() override {
            // 模拟超时
            func(nullptr);
            // 从容器移除
            container->erase(serial);
        }
    };

    // attach base send & recv funcs & ctx.
    template<typename T>
    struct OMExt {
        // 循环自增用于生成 serial
        int32_t autoIncSerial = 0;

        // 所有 带超时的回调. key: serial
        std::unordered_map<int32_t, xx::Shared<CB<T>>> callbacks;

        // 发推送
        int SendPush(xx::ObjBase_s const &ob) {
            // 推送性质的包, serial == 0
            return ((T*)this)->SendResponse(0, ob);
        }

        // 发请求（收到相应回应时会触发 cb 执行。超时或断开也会触发，o == nullptr）
        int SendRequest(xx::ObjBase_s const &o, typename CB<T>::Func &&cbfunc, double const &timeoutSeconds) {
            // 产生一个序号. 在正整数范围循环自增( 可能很多天之后会重复 )
            autoIncSerial = (autoIncSerial + 1) & 0x7FFFFFFF;
            // 创建一个 带超时的回调
            auto &&cb = xx::Make<CB<T>>(((T*)this)->ec, &callbacks, autoIncSerial, std::move(cbfunc), timeoutSeconds);
            cb->Hold();
            // 以序列号建立cb的映射
            callbacks[autoIncSerial] = std::move(cb);
            // 发包并返回( 请求性质的包, 序号为负数 )
            return ((T*)this)->SendResponse(-autoIncSerial, o);
        }

        // 收到回应( 自动调用 发送请求时设置的回调函数 )
        void ReceiveResponse(int32_t const &serial, xx::ObjBase_s &&ob) {
            // 根据序号定位到 cb. 找不到可能是超时或发错?
            auto &&iter = callbacks.find(serial);
            if (iter == callbacks.end()) return;
            iter->second->func(std::move(ob));
            callbacks.erase(iter);
        }

        // 触发所有已存在回调（ 模拟超时回调 ） & clear
        void ClearCallbacks() {
            for (auto &&iter : callbacks) {
                iter.second->func(nullptr);
            }
            callbacks.clear();
        }
    };

    struct TypeCounterExt {
        // when receive a package, let typeCounters[typeId]++
        // when handled, typeCounters[typeId]--
        std::array<int8_t, std::numeric_limits<uint16_t>::max()> typeCounters{};
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

/*
 * example:

namespace EP = xx::Epoll;
struct Peer : EP::TcpPeer, EP::OMExt<Peer>, EP::TypeCounterExt {
    using EP::TcpPeer::TcpPeer;

    // cleanup callbacks, DelayUnhold
    bool Close(int const &reason, std::string_view const &desc) override {
        // 防重入( 同时关闭 fd )
        if (!this->Item::Close(reason, desc)) return false;

        // 触发所有已存在回调（ 模拟超时回调 ）
        ClearCallbacks();

        // 延迟减持
        DelayUnhold();
        return true;
    }

    //// 收到数据. 切割后进一步调用 ReceiveXxxxxxx
    void DBPeer::Receive() {
        // 取出指针备用
        auto buf = recv.buf;
        auto end = recv.buf + recv.len;
        uint32_t dataLen = 0;
        uint32_t addr = 0;

        // 确保包头长度充足
        while (buf + sizeof(dataLen) <= end) {
            // 取长度
            dataLen = *(uint32_t *) buf;

            // 长度异常则断线退出( 不含地址? 超长? 256k 不够可以改长 )
            if (dataLen < sizeof(addr) || dataLen > 1024 * 256) {
                Close(__LINE__, "Peer Receive if (dataLen < sizeof(addr) || dataLen > 1024 * 256)");
                return;
            }

            // 数据未接收完 就 跳出
            if (buf + sizeof(dataLen) + dataLen > end) break;

            // 跳到数据区开始调用处理回调
            buf += sizeof(dataLen);
            {
                xx::Data_r dr(buf, dataLen);
                // 试读出序号
                int32_t serial = 0;
                if (int r = dr.Read(serial)) {
                    LOG_ERR("dr.Read(serial) r = ", r);
                }
                else {
                    // unpack
                    xx::ObjBase_s o;
                    if ((r = S->om.ReadFrom(dr, o))) {
                        LOG_ERR("S->om.ReadFrom(dr, o) r = ", r);
                        S->om.KillRecursive(o);
                    }
                    if (!o || o.typeId() == 0) {
                        LOG_ERR("o is nullptr or typeId == 0");
                    }
                    else {
                        // 根据序列号的情况分性质转发
                        if (serial == 0) {
                            ReceivePush(std::move(o));
                        } else if (serial > 0) {
                            ReceiveResponse(serial, std::move(o));
                        } else {
                            // -serial: 将 serial 转为正数
                            ReceiveRequest(-serial, std::move(o));
                        }
                    }
                }
                // 如果当前类实例 fd 已 close 则退出
                if (!Alive()) return;
            }
            // 跳到下一个包的开头
            buf += dataLen;
        }

        // 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
        recv.RemoveFront(buf - recv.buf);
    }

    // 发回应
    int SendResponse(int32_t const &serial, xx::ObjBase_s const &ob) {
        if (!Alive()) return __LINE__;
        // 准备发包填充容器
        xx::Data d(65536);
        // 跳过包头
        d.len = sizeof(uint32_t);
        // 写序号
        d.WriteVarInteger(serial);
        // 写数据
        S->om.WriteTo(d, ob);
        // 填包头
        *(uint32_t *) d.buf = (uint32_t) (d.len - sizeof(uint32_t));
        // 发包并返回
        Send(std::move(d));
        return 0;
    }

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(xx::ObjBase_s &&ob) {
        // logic here
    }

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int32_t const &serial, xx::ObjBase_s &&ob) {
        // logic here
    }
};
 */
