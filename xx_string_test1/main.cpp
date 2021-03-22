#include "xx_string.h"

int main() {
	xx::Data d;
	d.Fill({ 3,4,5,6 });
	xx::Cout(1, 2, "asdf", 3, d);

	xx::Cout("end");
	return 0;
}
