﻿#pragma once
#include "xx_helpers.h"

namespace xx {

	/************************************************************************************/
	// 质数相关

	// < 2G, 8 - 512 dataSize
	constexpr static const int32_t primes[] = { 7, 11, 13, 17, 19, 23, 31, 37, 43, 47, 53, 59, 67, 71, 73, 79, 83, 89, 97, 103, 107, 109, 113, 131, 139, 151, 157, 163, 167, 173, 179, 181, 191, 193, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 271, 277, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 463, 467, 479, 487, 491, 499, 503, 509, 523, 541, 547, 557, 563, 571, 577, 587, 599, 607, 613, 619, 631, 647, 653, 661, 677, 683, 691, 701, 709, 719, 727, 733, 743, 751, 757, 761, 773, 787, 797, 811, 823, 829, 839, 853, 863, 877, 887, 911, 919, 929, 941, 947, 953, 967, 971, 983, 991, 997, 1013, 1039, 1051, 1069, 1087, 1103, 1117, 1129, 1151, 1163, 1181, 1193, 1213, 1231, 1237, 1259, 1279, 1291, 1307, 1327, 1373, 1381, 1399, 1423, 1439, 1453, 1471, 1487, 1499, 1511, 1531, 1549, 1567, 1583, 1597, 1613, 1627, 1637, 1663, 1669, 1693, 1709, 1723, 1741, 1759, 1789, 1801, 1823, 1831, 1847, 1871, 1879, 1901, 1913, 1933, 1951, 1979, 1999, 2011, 2029, 2069, 2111, 2143, 2161, 2207, 2239, 2269, 2297, 2333, 2357, 2399, 2423, 2459, 2477, 2521, 2557, 2591, 2621, 2647, 2687, 2719, 2749, 2777, 2803, 2843, 2879, 2909, 2939, 2971, 3001, 3037, 3067, 3089, 3121, 3167, 3191, 3229, 3259, 3271, 3323, 3359, 3391, 3413, 3449, 3469, 3517, 3547, 3583, 3613, 3643, 3677, 3709, 3739, 3769, 3803, 3833, 3863, 3889, 3931, 3967, 3989, 4027, 4057, 4159, 4219, 4283, 4349, 4409, 4463, 4523, 4603, 4663, 4733, 4799, 4861, 4919, 4987, 5051, 5119, 5179, 5237, 5309, 5351, 5437, 5503, 5563, 5623, 5693, 5749, 5821, 5881, 5939, 6011, 6079, 6143, 6203, 6271, 6329, 6397, 6451, 6521, 6581,
		6653, 6719, 6781, 6841, 6911, 6971, 7039, 7103, 7159, 7229, 7283, 7351, 7417, 7487, 7549, 7607, 7673, 7741, 7793, 7867, 7933, 7993, 8059, 8123, 8317, 8447, 8573, 8699, 8831, 8951, 9067, 9209, 9343, 9467, 9587, 9721, 9851, 9973, 10111, 10223, 10357, 10487, 10613, 10739, 10867, 11003, 11131, 11261, 11383, 11519, 11633, 11743, 11903, 12011, 12157, 12281, 12413, 12541, 12671, 12799, 12923, 13049, 13183, 13309, 13421, 13567, 13693, 13807, 13933, 14071, 14207, 14327, 14461, 14591, 14717, 14843, 14969, 15101, 15227, 15359, 15473, 15607, 15739, 15859, 15991, 16127, 16253, 16633, 16889, 17137, 17401, 17659, 17911, 18169, 18427, 18679, 18919, 19183, 19447, 19709, 19963, 20219, 20479, 20731, 20983, 21247, 21503, 21757, 22013, 22271, 22511, 22783, 23039, 23293, 23549, 23801, 24061, 24317, 24571, 24821, 25087, 25343, 25589, 25849, 26111, 26357, 26597, 26879, 27127, 27367, 27647, 27901, 28151, 28411, 28669, 28927, 29179, 29437, 29683, 29947, 30203, 30449, 30713, 30971, 31231, 31481, 31741, 31991, 32251, 32507, 33247, 33791, 34303, 34807, 35327, 35839, 36343, 36857, 37369, 37879, 38393, 38903, 39419, 39929, 40433, 40949, 41467, 41983, 42491, 43003, 43517, 44029, 44543, 45053, 45557, 46073, 46591, 47093, 47609, 48121, 48623, 49139, 49663, 50159, 50683, 51199, 51691, 52223, 52733, 53239, 53759, 54269, 54779, 55291, 55807, 56311, 56827, 57331, 57853, 58367, 58831, 59387, 59887, 60413, 60923, 61417, 61949, 62459, 62971, 63487, 63997, 64499, 65011, 66553, 67579, 68597, 69623, 70639, 71671, 72701, 73727, 74747, 75773, 76781, 77813, 78839, 79867, 80863, 81919, 82939, 83939, 84991, 86011, 87037, 88037, 89087, 90107, 91129,
		92153, 93179, 94207, 95231, 96233, 97259, 98299, 99317, 100343, 101363, 102397, 103423, 104417, 105467, 106487, 107509, 108541, 109567, 110587, 111611, 112621, 113657, 114679, 115693, 116731, 117757, 118757, 119797, 120829, 121853, 122869, 123887, 124919, 125941, 126967, 127997, 129023, 130043, 133117, 135151, 137209, 139241, 141311, 143357, 145399, 147451, 149503, 151549, 153589, 155627, 157679, 159739, 161783, 163819, 165887, 167917, 169957, 172031, 174079, 176123, 178169, 180221, 182261, 184309, 186343, 188407, 190409, 192499, 194543, 196597, 198647, 200699, 202751, 204797, 206827, 208891, 210943, 212987, 214993, 217081, 219133, 221173, 223229, 225263, 227303, 229373, 231419, 233437, 235519, 237563, 239611, 241663, 243709, 245759, 247799, 249853, 251903, 253951, 255989, 258031, 260089, 266239, 270329, 274423, 278503, 282617, 286711, 290803, 294911, 298999, 303097, 307189, 311293, 315389, 319483, 323581, 327673, 331769, 335857, 339959, 344053, 348149, 352249, 356351, 360439, 364543, 368633, 372733, 376823, 380917, 385013, 389117, 393209, 397303, 401407, 405499, 409597, 413689, 417773, 421847, 425977, 430061, 434167, 438271, 442367, 446461, 450557, 454637, 458747, 462841, 466919, 471007, 475109, 479231, 483323, 487423, 491503, 495613, 499711, 503803, 507901, 511997, 516091, 520151, 532453, 540629, 548861, 557041, 565247, 573437, 581617, 589811, 598007, 606181, 614387, 622577, 630737, 638971, 647161, 655357, 663547,
		671743, 679933, 688111, 696317, 704507, 712697, 720887, 729073, 737279, 745471, 753659, 761833, 770047, 778237, 786431, 794593, 802811, 810989, 819187, 827389, 835559, 843763, 851957, 860143, 868349, 876529, 884717, 892919, 901111, 909301, 917503, 925679, 933883, 942079, 950269, 958459, 966653, 974837, 982981, 991229, 999389, 1007609, 1015769, 1023991, 1032191, 1040381, 1064957, 1081337, 1097717, 1114111, 1130471, 1146877, 1163263, 1179641, 1196029, 1212401, 1228789, 1245169, 1261567, 1277911, 1294309, 1310719, 1327099, 1343479, 1359871, 1376237, 1392631, 1409017, 1425371, 1441771, 1458169, 1474559, 1490941, 1507321, 1523707, 1540087, 1556473, 1572853, 1589239, 1605631, 1622009, 1638353, 1654739, 1671161, 1687549, 1703903, 1720307, 1736701, 1753069, 1769441, 1785853, 1802239, 1818617, 1835003, 1851391, 1867771, 1884133, 1900543, 1916921, 1933301, 1949657, 1966079, 1982447, 1998839, 2015213, 2031611, 2047993, 2064379, 2080763, 2129903, 2162681, 2195443, 2228221, 2260967, 2293757, 2326517, 2359267, 2392057, 2424827, 2457569, 2490337, 2523133, 2555897, 2588671, 2621431, 2654161, 2686973, 2719741, 2752499, 2785273, 2818043, 2850811, 2883577, 2916343, 2949119, 2981887, 3014653, 3047423, 3080167, 3112943, 3145721, 3178489, 3211213, 3244013, 3276799, 3309563, 3342331, 3375083, 3407857, 3440627, 3473399, 3506171, 3538933, 3571699, 3604451, 3637223, 3670013, 3702757, 3735547, 3768311, 3801073, 3833833, 3866623, 3899383, 3932153, 3964913, 3997673, 4030463, 4063217, 4095991, 4128767, 4161527, 4259837, 4325359, 4390909,
		4456433, 4521977, 4587503, 4653041, 4718579, 4784107, 4849651, 4915171, 4980727, 5046259, 5111791, 5177339, 5242877, 5308379, 5373931, 5439479, 5505023, 5570533, 5636077, 5701627, 5767129, 5832679, 5898209, 5963773, 6029299, 6094807, 6160381, 6225917, 6291449, 6356983, 6422519, 6488023, 6553577, 6619111, 6684659, 6750203, 6815741, 6881269, 6946813, 7012337, 7077883, 7143421, 7208951, 7274489, 7340009, 7405547, 7471099, 7536637, 7602151, 7667711, 7733233, 7798783, 7864301, 7929833, 7995391, 8060891, 8126453, 8191991, 8257531, 8323069, 8519647, 8650727, 8781797, 8912887, 9043967, 9175037, 9306097, 9437179, 9568219, 9699323, 9830393, 9961463, 10092539, 10223593, 10354667, 10485751, 10616831, 10747903, 10878961, 11010037, 11141113, 11272181, 11403247, 11534329, 11665403, 11796469, 11927551, 12058621, 12189677, 12320753, 12451807, 12582893, 12713959, 12845033, 12976121, 13107197, 13238263, 13369333, 13500373, 13631477, 13762549, 13893613, 14024671, 14155763, 14286809, 14417881, 14548979, 14680063, 14811133, 14942197, 15073277, 15204349, 15335407, 15466463, 15597559, 15728611, 15859687, 15990781, 16121849, 16252919, 16383977, 16515067, 16646099, 17039339, 17301463, 17563633, 17825791, 18087899, 18350063, 18612211, 18874367, 19136503, 19398647, 19660799, 19922923, 20185051, 20447191, 20709347, 20971507, 21233651, 21495797, 21757951, 22020091, 22282199, 22544351, 22806521, 23068667, 23330773, 23592937, 23855101, 24117217, 24379391, 24641479, 24903667, 25165813, 25427957, 25690097, 25952243, 26214379, 26476543, 26738687,
		27000817, 27262931, 27525109, 27787213, 28049407, 28311541, 28573673, 28835819, 29097977, 29360087, 29622269, 29884411, 30146531, 30408701, 30670847, 30932987, 31195117, 31457269, 31719409, 31981567, 32243707, 32505829, 32767997, 33030121, 33292283, 34078699, 34602991, 35127263, 35651579, 36175871, 36700159, 37224437, 37748717, 38273023, 38797303, 39321599, 39845887, 40370173, 40894457, 41418739, 41943023, 42467317, 42991609, 43515881, 44040187, 44564461, 45088739, 45613039, 46137319, 46661627, 47185907, 47710207, 48234451, 48758783, 49283063, 49807327, 50331599, 50855899, 51380179, 51904511, 52428767, 52953077, 53477357, 54001663, 54525917, 55050217, 55574507, 56098813, 56623093, 57147379, 57671671, 58195939, 58720253, 59244539, 59768831, 60293119, 60817397, 61341659, 61865971, 62390261, 62914549, 63438839, 63963131, 64487417, 65011703, 65535989, 66060277, 66584561, 68157433, 69205987, 70254563, 71303153, 72351733, 73400311, 74448877, 75497467, 76546039, 77594599, 78643199, 79691761, 80740339, 81788923, 82837501, 83886053, 84934621, 85983217, 87031759, 88080359, 89128939, 90177533, 91226101, 92274671, 93323249, 94371833, 95420401, 96468979, 97517543, 98566121, 99614689, 100663291, 101711839, 102760387, 103809011, 104857589, 105906167, 106954747, 108003323, 109051903, 110100409, 111148963, 112197629, 113246183, 114294721, 115343341, 116391917, 117440509, 118489081, 119537653, 120586231, 121634801, 122683391,
		123731963, 124780531, 125829103, 126877693, 127926263, 128974841, 130023407, 131071987, 132120557, 133169137, 136314869, 138412031, 140509183, 142606333, 144703477, 146800637, 148897789, 150994939, 153092087, 155189239, 157286383, 159383467, 161480689, 163577833, 165674981, 167772107, 169869311, 171966439, 174063611, 176160739, 178257917, 180355069, 182452199, 184549373, 186646517, 188743679, 190840817, 192937933, 195035129, 197132267, 199229419, 201326557, 203423729, 205520881, 207618031, 209715199, 211812331, 213909503, 216006599, 218103799, 220200947, 222298093, 224395253, 226492393, 228589561, 230686697, 232783871, 234881011, 236978171, 239075327, 241172479, 243269627, 245366777, 247463933, 249561083, 251658227, 253755391, 255852511, 257949691, 260046847, 262143977, 264241147, 266338297, 272629759, 276824033, 281018263, 285212659, 289406951, 293601263, 297795541, 301989881, 306184189, 310378493, 314572799, 318767093, 322961399, 327155693, 331349989, 335544301, 339738617, 343932923, 348127231, 352321531, 356515813, 360710137, 364904447, 369098707, 373293049, 377487343, 381681661, 385875929, 390070267, 394264567, 398458859, 402653171, 406847479, 411041767, 415236083, 419430383, 423624673, 427819001, 432013291, 436207613, 440401889, 444596209, 448790519, 452984827, 457179131, 461373431, 465567743, 469762043, 473956339, 478150643, 482344957, 486539257, 490733567, 494927857, 499122163, 503316469, 507510719, 511705069, 515899379,
		520093667, 524287997, 528482263, 532676593, 545259493, 553648103, 562036693, 570425299, 578813951, 587202547, 595591153, 603979769, 612368377, 620756953, 629145593, 637534199, 645922787, 654311423, 662700019, 671088637, 679477243, 687865847, 696254441, 704643067, 713031659, 721420261, 729808889, 738197503, 746586109, 754974691, 763363327, 771751927, 780140521, 788529121, 796917757, 805306357, 813694951, 822083551, 830472127, 838860791, 847249399, 855637957, 864026567, 872415211, 880803809, 889192441, 897581029, 905969653, 914358253, 922746863, 931135487, 939524087, 947912701, 956301169, 964689917, 973078511, 981467119, 989855707, 998244341, 1006632947, 1015021549, 1023410159, 1031798783, 1040187377, 1048575959, 1056964501, 1065353209, 1090519013, 1107296251, 1124073463, 1140850681, 1157627879, 1174405103, 1191182309, 1207959503, 1224736717, 1241513981, 1258291187, 1275068407, 1291845593, 1308622837, 1325400059, 1342177237, 1358954453, 1375731701, 1392508927, 1409286139, 1426063351, 1442840569, 1459617779, 1476394991, 1493172223, 1509949421, 1526726647, 1543503851, 1560281087, 1577058271, 1593835489, 1610612711, 1627389901, 1644167159, 1660944367, 1677721597, 1694498801, 1711276019, 1728053237, 1744830457, 1761607657, 1778384887, 1795162111, 1811939317, 1828716523, 1845493753, 1862270957, 1879048183, 1895825387, 1912602623, 1929379813, 1946157053, 1962934271, 1979711483, 1996488677, 2013265907, 2030043131, 2046820351, 2063597567, 2080374781, 2097151967, 2113929203, 2130706381 };

	//  < 2G 2^N
	constexpr static const int32_t primes2n[] = {
		1, 2, 3, 7, 13, 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071, 262139, 524287,
		1048573, 2097143, 4194301, 8388593, 16777213, 33554393, 67108859, 134217689, 268435399, 536870909, 1073741789
	};

	inline bool IsPrime(size_t const& candidate) noexcept {
		if ((candidate & 1) != 0) {
			size_t limit = size_t(sqrt((double)candidate));
			for (size_t divisor = 3; divisor <= limit; divisor += 2) {
				if ((candidate % divisor) == 0) return false;
			}
			return true;
		}
		return (candidate == 2);
	}

	inline int32_t GetPrime(int32_t const& capacity, int32_t const& dataSize) noexcept {
		auto memUsage = Round2n((size_t)capacity * (size_t)dataSize);
		auto maxCapacity = memUsage / dataSize;
		if (maxCapacity == (size_t)capacity) {
			return primes2n[Calc2n(capacity)];
		}
		if (dataSize >= 8 && dataSize <= 512) {                     // 数据长在 查表 范围内的
			return *std::upper_bound(std::begin(primes), std::end(primes), (int32_t)maxCapacity);
		}
		for (size_t i = maxCapacity + 1; i <= 0x7fffffff; i += 2) { // maxCapacity 是双数. +1 即为单数
			if (IsPrime(i)) return (int32_t)i;
		}
		assert(false);
		return -1;
	}


	// Dict.Add 的操作结果
	struct DictAddResult {
		bool success;
		int index;
		operator bool() const { return success; }
	};

	// 功能类似 std::unordered_map。抄自 .net 的 Dictionary
	// 主要特点：iter 是个固定数值下标。故可持有加速 Update & Remove 操作, 遍历时删除当前 iter 指向的数据安全。
	template <typename TK, typename TV>
	class Dict {
	public:
		typedef TK KeyType;
		typedef TV ValueType;

		struct Node {
			size_t			hashCode;
			int             next;
		};
		struct Data {
			TK              key;
			TV              value;
			int             prev;
		};

	private:
		int                 freeList;               // 自由空间链表头( next 指向下一个未使用单元 )
		int                 freeCount;              // 自由空间链长
		int                 count;                  // 已使用空间数
		int                 bucketsLen;             // 桶数组长( 质数 )
		int                *buckets;                // 桶数组
		Node               *nodes;                  // 节点数组
		Data               *items;                  // 数据数组( 与节点数组同步下标 )

	public:
		explicit Dict(int const& capacity = 16);
		~Dict();
		Dict(Dict const& o) = delete;
		Dict& operator=(Dict const& o) = delete;


		// 只支持没数据时扩容或空间用尽扩容( 如果不这样限制, 扩容时的 遍历损耗 略大 )
		void Reserve(int capacity = 0) noexcept;

		// 根据 key 返回下标. -1 表示没找到.
		template<typename K>
		int Find(K const& key) const noexcept;

		// 根据 key 移除一条数据
		template<typename K>
		void Remove(K const& key) noexcept;

		// 根据 下标 移除一条数据( unsafe )
		void RemoveAt(int const& idx) noexcept;

		// 规则同 std. 如果 key 不存在, 将创建 key, TV默认值 的元素出来. C++ 不方便像 C# 那样直接返回 default(TV). 无法返回临时变量的引用
		template<typename K>
		TV& operator[](K&& key) noexcept;

		// 清除所有数据
		void Clear() noexcept;

		// 放入数据. 如果放入失败, 将返回 false 以及已存在的数据的下标
		template<typename K, typename V>
		DictAddResult Add(K&& k, V&& v, bool const& override = false) noexcept;

        // 修改 key. 返回 负数 表示修改失败( -1: 旧 key 未找到. -2: 新 key 已存在 ), 0或正数 成功( index )
        template<typename K>
        int Update(TK const& oldKey, K&& newKey) noexcept;

        // 取数据记录数
		uint32_t Count() const noexcept;

		// 是否没有数据
		bool Empty() noexcept;

		// 试着填充数据到 outV. 如果不存在, 就返回 false
		bool TryGetValue(TK const& key, TV& outV) noexcept;

		// 同 operator[]
		template<typename K>
		TV& At(K&& key) noexcept;


		// 下标直肏系列( unsafe )

		// 读下标所在 key
		TK const& KeyAt(int const& idx) const noexcept;

		// 读下标所在 value
		TV& ValueAt(int const& idx) noexcept;
		TV const& ValueAt(int const& idx) const noexcept;

        // 修改 指定下标的 key. 返回 false 表示修改失败( 新 key 已存在 ). idx 不会改变
		template<typename K>
        bool UpdateAt(int const& idx, K&& newKey) noexcept;

        // 简单 check 某下标是否有效( for debug )
        bool IndexExists(int const& idx) const noexcept;

        // for (auto&& data : dict) {
		// for (auto&& iter = dict.begin(); iter != dict.end(); ++iter) {	if (iter->value...... iter.Remove()
		struct Iter {
			Dict& hs;
			int i;
			bool operator!=(Iter const& other) noexcept { return i != other.i; }
			Iter& operator++() noexcept {
				while (++i < hs.count) {
					if (hs.items[i].prev != -2) break;
				}
				return *this;
			}
			Data& operator*() { return hs.items[i]; }
			Data* operator->() { return &hs.items[i]; }
			operator int() const {
				return i;
			}
			void Remove() { hs.RemoveAt(i); }
		};
		Iter begin() noexcept {
			if (Empty()) return end();
			for (int i = 0; i < count; ++i) {
				if (items[i].prev != -2) return Iter{ *this, i };
			}
			return end();
		}
		Iter end() noexcept { return Iter{ *this, count }; }

	protected:
		// 用于 析构, Clear
		void DeleteKVs() noexcept;
	};





	template <typename TK, typename TV>
	Dict<TK, TV>::Dict(int const& capacity) {
		freeList = -1;
		freeCount = 0;
		count = 0;
		bucketsLen = (int)GetPrime(capacity, sizeof(Data));
		buckets = (int*)malloc(bucketsLen * sizeof(int));
		memset(buckets, -1, bucketsLen * sizeof(int));  // -1 代表 "空"
		nodes = (Node*)malloc(bucketsLen * sizeof(Node));
		items = (Data*)malloc(bucketsLen * sizeof(Data));
	}

	template <typename TK, typename TV>
	Dict<TK, TV>::~Dict() {
		DeleteKVs();
		free(buckets);
		free(nodes);
		free(items);
	}

	template <typename TK, typename TV>
	template<typename K, typename V>
	DictAddResult Dict<TK, TV>::Add(K&& k, V&& v, bool const& override) noexcept {
		assert(bucketsLen);
		// hash 按桶数取模 定位到具体 链表, 扫找
		auto hashCode = Hash<std::decay_t<K>>{}(k);
		auto targetBucket = hashCode % bucketsLen;
		for (int i = buckets[targetBucket]; i >= 0; i = nodes[i].next) {
			if (nodes[i].hashCode == hashCode && items[i].key == k) {
				if (override) {                      // 允许覆盖 value
					if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
						items[i].value.~TV();
					}
					new (&items[i].value) TV(std::forward<V>(v));
					return DictAddResult{ true, i };
				}
				return DictAddResult{ false, i };
			}
		}
		// 没找到则新增
		int index;									// 如果 自由节点链表 不空, 取一个来当容器
		if (freeCount > 0) {                        // 这些节点来自 Remove 操作. next 指向下一个
			index = freeList;
			freeList = nodes[index].next;
			freeCount--;
		}
		else {
			if (count == bucketsLen) {              // 所有空节点都用光了, Resize
				Reserve();
				targetBucket = hashCode % bucketsLen;
			}
			index = count;                         // 指向 Resize 后面的空间起点
			count++;
		}

		// 执行到此处时, freeList 要么来自 自由节点链表, 要么为 Reserve 后新增的空间起点.
		nodes[index].hashCode = hashCode;
		nodes[index].next = buckets[targetBucket];

		// 如果当前 bucket 中有 index, 则令其 prev 等于即将添加的 index
		if (buckets[targetBucket] >= 0) {
			items[buckets[targetBucket]].prev = index;
		}
		buckets[targetBucket] = index;

		// 移动复制构造写 key, value
		new (&items[index].key) TK(std::forward<K>(k));
		new (&items[index].value) TV(std::forward<V>(v));
		items[index].prev = -1;

		return DictAddResult{ true, index };
	}

	// 只支持没数据时扩容或空间用尽扩容( 如果不这样限制, 扩容时的 遍历损耗 略大 )
	template <typename TK, typename TV>
	void Dict<TK, TV>::Reserve(int capacity) noexcept {
		assert(buckets);
		assert(count == 0 || count == bucketsLen);          // 确保扩容函数使用情型

		// 得到空间利用率最高的扩容长度并直接修改 bucketsLen( count 为当前数据长 )
		if (capacity == 0) {
			capacity = count * 2;                           // 2倍扩容
		}
		if (capacity <= bucketsLen) return;
		bucketsLen = (int)GetPrime(capacity, sizeof(Data));

		// 桶扩容并全部初始化( 后面会重新映射一次 )
		free(buckets);
		buckets = (int*)malloc(bucketsLen * sizeof(int));
		memset(buckets, -1, bucketsLen * sizeof(int));

		// 节点数组扩容( 保留老数据 )
		nodes = (Node*)realloc(nodes, bucketsLen * sizeof(Node));

		// item 数组扩容
		if constexpr (std::is_trivial_v<TK> && std::is_trivial_v<TV>) {
			items = (Data*)realloc((void*)items, bucketsLen * sizeof(Data));
		}
		else {
			auto newItems = (Data*)malloc(bucketsLen * sizeof(Data));
			for (int i = 0; i < count; ++i) {
				new (&newItems[i].key) TK((TK&&)items[i].key);
				if constexpr (!(std::is_standard_layout_v<TK> && std::is_trivial_v<TK>)) {
					items[i].key.TK::~TK();
				}
				new (&newItems[i].value) TV((TV&&)items[i].value);
				if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
					items[i].value.TV::~TV();
				}
			}
			free(items);
			items = newItems;
		}

		// 遍历所有节点, 重构桶及链表( 扩容情况下没有节点空洞 )
		for (int i = 0; i < count; i++) {
			auto index = nodes[i].hashCode % bucketsLen;
			if (buckets[index] >= 0) {
				items[buckets[index]].prev = i;
			}
			items[i].prev = -1;
			nodes[i].next = buckets[index];
			buckets[index] = i;
		}
	}

	template <typename TK, typename TV>
	template<typename K>
	int Dict<TK, TV>::Find(K const& k) const noexcept {
		assert(buckets);
		auto hashCode = Hash<std::decay_t<K>>{}(k);
		for (int i = buckets[hashCode % bucketsLen]; i >= 0; i = nodes[i].next) {
			if (nodes[i].hashCode == hashCode && items[i].key == k) return i;
		}
		return -1;
	}

	template <typename TK, typename TV>
	template<typename K>
	void Dict<TK, TV>::Remove(K const& k) noexcept {
		assert(buckets);
		auto idx = Find(k);
		if (idx != -1) {
			RemoveAt(idx);
		}
	}

	template <typename TK, typename TV>
	void Dict<TK, TV>::RemoveAt(int const& idx) noexcept {
		assert(buckets);
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		if (items[idx].prev < 0) {
			buckets[nodes[idx].hashCode % bucketsLen] = nodes[idx].next;
		}
		else {
			nodes[items[idx].prev].next = nodes[idx].next;
		}
		if (nodes[idx].next >= 0) {      // 如果存在当前节点的下一个节点, 令其 prev 指向 上一个节点
			items[nodes[idx].next].prev = items[idx].prev;
		}

		nodes[idx].next = freeList;     // 当前节点已被移出链表, 令其 next 指向  自由节点链表头( next 有两种功用 )
		freeList = idx;
		freeCount++;

		if constexpr (!(std::is_standard_layout_v<TK> && std::is_trivial_v<TK>)) {
			items[idx].key.~TK();
		}
		if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
			items[idx].value.~TV();
		}
		items[idx].prev = -2;           // foreach 时的无效标志
	}

	template <typename TK, typename TV>
	void Dict<TK, TV>::Clear() noexcept {
		assert(buckets);
		if (!count) return;
		DeleteKVs();
		memset(buckets, -1, bucketsLen * sizeof(int));
		freeList = -1;
		freeCount = 0;
		count = 0;
	}

	template <typename TK, typename TV>
	template<typename K>
	TV& Dict<TK, TV>::operator[](K &&k) noexcept {
		assert(buckets);
		auto&& r = Add(std::forward<K>(k), TV(), false);
		return items[r.index].value;
		// return items[Add(std::forward<K>(k), TV(), false).index].value;		// 这样写会导致 gcc 先记录下 items 的指针，Add 如果 renew 了 items 就 crash
	}

	template <typename TK, typename TV>
	void Dict<TK, TV>::DeleteKVs() noexcept {
		assert(buckets);
		for (int i = 0; i < count; ++i) {
			if (items[i].prev != -2) {
				if constexpr (!(std::is_standard_layout_v<TK> && std::is_trivial_v<TK>)) {
					items[i].key.~TK();
				}
				if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
					items[i].value.~TV();
				}
				items[i].prev = -2;
			}
		}
	}

	template <typename TK, typename TV>
	uint32_t Dict<TK, TV>::Count() const noexcept {
		assert(buckets);
		return uint32_t(count - freeCount);
	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::Empty() noexcept {
		assert(buckets);
		return Count() == 0;
	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::TryGetValue(TK const& key, TV& outV) noexcept {
		int idx = Find(key);
		if (idx >= 0) {
			outV = items[idx].value;
			return true;
		}
		return false;
	}

	template <typename TK, typename TV>
	template<typename K>
	TV& Dict<TK, TV>::At(K&& key) noexcept {
		return operator[](std::forward<K>(key));
	}

	template <typename TK, typename TV>
	TV& Dict<TK, TV>::ValueAt(int const& idx) noexcept {
		assert(buckets);
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		return items[idx].value;
	}

	template <typename TK, typename TV>
	TK const& Dict<TK, TV>::KeyAt(int const& idx) const noexcept {
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		return items[idx].key;
	}

	template <typename TK, typename TV>
	TV const& Dict<TK, TV>::ValueAt(int const& idx) const noexcept {
		return const_cast<Dict*>(this)->ValueAt(idx);
	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::IndexExists(int const& idx) const noexcept {
		return idx >= 0 && idx < count && items[idx].prev != -2;
	}

	template <typename TK, typename TV>
	template<typename K>
	int Dict<TK, TV>::Update(TK const& oldKey, K&& newKey) noexcept {
		int idx = Find(oldKey);
		if (idx == -1) return -1;
		return UpdateAt(idx, std::forward<K>(newKey)) ? idx : -2;
	}

	template <typename TK, typename TV>
	template<typename K>
    bool Dict<TK, TV>::UpdateAt(int const& idx, K&& newKey) noexcept {
		assert(idx >= 0 && idx < count && items[idx].prev != -2);

		// 算 newKey hash, 定位到桶
		auto newHashCode = Hash<TK>{}(newKey);
        auto newBucket = newHashCode % (uint32_t)bucketsLen;

        // 检查 newKey 是否已存在
        for (int i = buckets[newBucket]; i >= 0; i = nodes[i].next) {
            if (nodes[i].hashCode == newHashCode && items[i].key == newKey) return false;
        }

        auto& node = nodes[idx];
        auto& item = items[idx];

        // 如果 hash 相等可以直接改 key 并退出
		if (node.hashCode == newHashCode) {
			item.key = std::forward<K>(newKey);
			return true;
		}

		// 定位到旧桶
        auto oldBucket = node.hashCode % (uint32_t)bucketsLen;

		// 位于相同 bucket, 直接改 hash & key 并退出
		if (oldBucket == newBucket) {
			node.hashCode = newHashCode;
			item.key = std::forward<K>(newKey);
			return true;
		}

		// 简化的 RemoveAt
		if (item.prev < 0) {
			buckets[oldBucket] = node.next;
		}
		else {
			nodes[item.prev].next = node.next;
		}
		if (node.next >= 0) {
			items[node.next].prev = item.prev;
		}

		// 简化的 Add 后半段
		node.hashCode = newHashCode;
		node.next = buckets[newBucket];

		if (buckets[newBucket] >= 0) {
			items[buckets[newBucket]].prev = idx;
		}
		buckets[newBucket] = idx;

		item.key = std::forward<K>(newKey);
		item.prev = -1;

		return true;
	}
}
