#include "xx_data.h"
#include "xx_data_v.h"

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

    d.WriteBuf("x1x2x3x4", 4);
    d.WriteFixed(0x04030201u);

    {
        xx::Data_v dv;
        dv.Reset(d.buf, d.len);
        assert(dv.buf == d.buf);
        assert(dv.len == d.len);
        assert(dv.offset == 0);

        uint32_t v = 0;
        int r = dv.ReadFixed(v);
        assert(r == 0);
        assert(v == 0x04030201u);
    }

    d.Clear();


    return 0;
}
