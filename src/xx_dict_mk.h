#pragma once
#include "xx_dict.h"

namespace xx {
    // 多 key 字典
    template<typename V, typename ... KS> struct DictMK {
        using KeyTypes = std::tuple<KS...>;

        // dict, dicts 的下标应该是一致的
        Dict<std::tuple_element_t<0, KeyTypes>, V> dict;    // 主体
        std::tuple<Dict<KS, int>...> dicts;                 // value 指向主体 节点下标

        void Reserve(int const& cap) {
            dict.Reserve(cap);
            Reserve_<1>(cap);
        }
    protected:
        template<size_t i> std::enable_if_t<i == sizeof...(KS)> Reserve_(int const& cap) {}
        template<size_t i> std::enable_if_t<i  < sizeof...(KS)> Reserve_(int const& cap) {
            std::get<i>(dicts).Reserve(cap);
            Reserve_<i + 1>(cap);
        }

    public:

        explicit DictMK(int const& cap = 16) {
            Reserve(cap);
        }

        template<int keyIndex = 0>
        int Find(std::tuple_element_t<keyIndex, KeyTypes> const& key) const noexcept {
            if constexpr (keyIndex == 0) {
                return dict.Find(key);
            }
            else {
                return std::get<keyIndex>(dicts).Find(key);
            }
        }
        template<int keyIndex = 0>
        bool Exists(std::tuple_element_t<keyIndex, KeyTypes> const& key) const noexcept {
            return Find<keyIndex>(key) != -1;
        }
        template<int keyIndex = 0>
        bool ContainsKey(std::tuple_element_t<keyIndex, KeyTypes> const& key) const noexcept {
            return Find<keyIndex>(key) != -1;
        }

        template<int keyIndex = 0>
        bool TryGetValue(std::tuple_element_t<keyIndex, KeyTypes> const& key, V& value) const noexcept {
            if constexpr (keyIndex == 0) {
                return dict.TryGetValue(key, value);
            }
            else {
                auto& d =  std::get<keyIndex>(dicts);
                auto index = d.Find(key);
                if (index == -1) return false;
                value = dict.ValueAt(index);
                return true;
            }
        }

        template<typename TV, typename...TKS>
        DictAddResult Add(TV&& value, TKS&&...keys) noexcept {
            static_assert(sizeof...(keys) == sizeof...(KS));
            return Add0(std::forward<TV>(value), std::forward<TKS>(keys)...);
        }

    protected:
        template<typename TV, typename TK, typename...TKS>
        DictAddResult Add0(TV&& value, TK&& key, TKS&&...keys) noexcept {
            auto r = dict.Add(std::forward<TK>(key), std::forward<TV>(value), false);
            if (!r.success) return r;
            r = Add_<1, TKS...>(r.index, std::forward<TKS>(keys)...);
            if (!r.success) {
                dict.RemoveAt(r.index);
            }
            return r;
        }
        template<int keyIndex, typename TK, typename...TKS>
        DictAddResult Add_(int const& index, TK&& key, TKS&&...keys) noexcept {
            auto r = std::get<keyIndex>(dicts).Add(std::forward<TK>(key), index);
            if (r.success) {
                if constexpr( sizeof...(TKS) > 0) {
                    r = Add_<keyIndex + 1, TKS...>(index, std::forward<TKS>(keys)...);
                }
                else return r;
            }
            if (!r.success) {
                std::get<keyIndex>(dicts).RemoveAt(index);
            }
            return r;
        }

    public:
        void RemoveAt(int const& index) noexcept {
            dict.RemoveAt(index);
            RemoveAt_<1>(index);
        }
    protected:
        template<size_t i> std::enable_if_t<i == sizeof...(KS)> RemoveAt_(int const& index) {}
        template<size_t i> std::enable_if_t<i  < sizeof...(KS)> RemoveAt_(int const& index) {
            std::get<i>(dicts).RemoveAt(index);
            RemoveAt_<i + 1>(index);
        }

    public:
        template<int keyIndex = 0>
        bool Remove(std::tuple_element_t<keyIndex, KeyTypes> const& key) noexcept {
            auto index = Find<keyIndex>(key);
            if (index == -1) return false;
            RemoveAt(index);
            return true;
        }

        template<int keyIndex = 0, typename TK>
        bool UpdateAt(int const& index, TK&& newKey) noexcept {
            if constexpr(keyIndex == 0) {
                return dict.UpdateAt(index, std::forward<TK>(newKey));
            }
            else {
                return std::get<keyIndex>(dicts).UpdateAt(index, std::forward<TK>(newKey));
            }
        }

        template<int keyIndex = 0, typename TK>
        int Update(std::tuple_element_t<keyIndex, KeyTypes> const& oldKey, TK&& newKey) noexcept {
            if constexpr(keyIndex == 0) {
                return dict.Update(oldKey, std::forward<TK>(newKey));
            }
            else {
                return std::get<keyIndex>(dicts).Update(oldKey, std::forward<TK>(newKey));
            }
        }

        template<int keyIndex = 0>
        std::tuple_element_t<keyIndex, KeyTypes> const& KeyAt(int const& index) const noexcept {
            if constexpr(keyIndex == 0) {
                return dict.KeyAt(index);
            }
            else {
                return std::get<keyIndex>(dicts).KeyAt(index);
            }
        }

        V& ValueAt(int const& index) noexcept {
            return dict.ValueAt(index);
        }
        [[nodiscard]] V const& ValueAt(int const& index) const noexcept {
            return dict.ValueAt(index);
        }

        void Clear() noexcept {
            dict.Clear();
            Clear_<1>();
        }
    protected:
        template<size_t i> std::enable_if_t<i == sizeof...(KS)> Clear_() {}
        template<size_t i> std::enable_if_t<i  < sizeof...(KS)> Clear_() {
            std::get<i>(dicts).Clear();
            Clear_<i + 1>();
        }

    public:
        [[nodiscard]] uint32_t Count() const noexcept {
            return dict.Count();
        }
    };
}
