#include "xx_dict.h"
#include <iostream>

namespace xx {
    template<typename V, typename ... KS> struct DictMK {
        using KeyTypes = std::tuple<KS...>;
        Dict<std::tuple_element<0, KeyTypes>, V> dict;      // 主体
        std::tuple<Dict<KS, int>...> dicts;                 // value 指向主体 节点下标

    protected:
        template<size_t i = 0>	std::enable_if_t<i == sizeof...(KS)> Reserve_(int const& cap) {}
        template<size_t i = 0>	std::enable_if_t<i < sizeof...(KS)> Reserve_(int const& cap) {
            std::get<i>(dicts).Reserve(cap);
            Reserve_<i + 1>(cap);
        }
    public:
        void Reserve(int const& cap) {
            dict.Reserve(cap);
            Reserve_(cap);
        }

        explicit DictMK(int const& cap = 16) {
            Reserve(cap);
        }

        template<int keyIndex>
        bool Exists(std::tuple_element<keyIndex, KeyTypes> const& key) const noexcept {
            if constexpr (keyIndex == 0) {
                return dict.Find(key) != -1;
            }
            else {
                return std::get<keyIndex>(dicts).Find(key) != -1;
            }
        }

        template<int keyIndex>
        bool TryGetValue(KeyType_t<keyIndex, V, KS...> const& key, V& value) const noexcept {
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

        // todo: Add
    };
}

int main() {
    using T = std::tuple<int, std::string>;
	xx::DictMK<T, int, std::string> d;
	//d.Add(1, "asdf");
	//std::cout << d[1] << std::endl;

	return 0;
}
