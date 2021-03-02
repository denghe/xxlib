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
    d.WriteFixed(0x04030201u);
    assert(d.len == 8);
    assert(d[0] == d[4]);
    assert(d[1] == d[5]);
    assert(d[2] == d[6]);
    assert(d[3] == d[7]);
    d.WriteFixedBE(0x01020304u);
    assert(d.len == 12);
    assert(d[0] == d[8]);
    assert(d[1] == d[9]);
    assert(d[2] == d[10]);
    assert(d[3] == d[11]);

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

    {
        xx::Data_v dv(d.buf, d.len);
        assert(dv.buf == d.buf);
        assert(dv.len == d.len);
        assert(dv.offset == 0);

        uint32_t v = 0;
        int r = dv.ReadFixedBE(v);
        assert(r == 0);
        assert(v == 0x01020304u);
    }

    return 0;
}
