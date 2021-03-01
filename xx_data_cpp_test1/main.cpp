#include "xx_data.h"
#include "xx_data_v.h"

#include <iostream>

int main() {
    xx::Data d;
    assert(d.buf == nullptr);
    assert(d.cap == 0);
    assert(d.len == 0);
    assert(d.offset == 0);

    d.Fill({1, 2, 3, 4});
    assert(d.buf != nullptr);
    assert(d.cap > 3);
    assert(d.len == 4);
    assert(d.offset == 0);

    xx::Data_v dv;
    dv.Reset(d.buf, d.len);
    assert(dv.buf == d.buf);
    assert(dv.len == d.len);
    assert(dv.offset == 0);

    uint32_t v = 0;
    if (int r = dv.ReadFixed(v)) {
        std::cout << r << std::endl;
    }
    else {
        std::cout << std::hex << v << std::endl;
    }

    return 0;
}
