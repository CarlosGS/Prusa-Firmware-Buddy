#include "catch2/catch.hpp"

#include <deque>
#include <regex>
#include <string>

#include "e-stall_detector.h"

using namespace std;

[[nodiscard]] constexpr bool almost_equal(float l, float r) {
    return r == std::nextafter(l, r);
}

TEST_CASE("Marlin::MotorStallFilter setup check", "[Marlin][EStallDetector]") {
    MotorStallFilter msf;

    // check, that the filter's input buffer is clean
    for_each(msf.buffer.begin(), msf.buffer.end(), [](uint32_t v) {
        CHECK(v == 0);
    });
}

TEST_CASE("Marlin::MotorStallFilter filter", "[Marlin][EStallDetector]") {
    MotorStallFilter msf;
    // start feeding the filter with real data from a test log taken at 2023-06-07 13:52:37.31467Z
    bool detected = msf.ProcessSample(1997572);
    CHECK_FALSE(detected);
    detected = msf.ProcessSample(2054684);
    CHECK_FALSE(detected);
    detected = msf.ProcessSample(2096993);
    CHECK_FALSE(detected);
    detected = msf.ProcessSample(2125882);
    CHECK_FALSE(detected);
    detected = msf.ProcessSample(2072601);
    CHECK_FALSE(detected);
    // now this drop shall be almost detected
    // tweaked from 985962 to make the drop bigger to accomodate for much a higher threshold (500k -> 900k threshold)
    detected = msf.ProcessSample(585962);
    CHECK_FALSE(detected);
    detected = msf.ProcessSample(1976478);
    CHECK(detected); // the drop got into the sensitive part of the filter, "cliff" is being reported
}

// check performed on top of multiple data records
TEST_CASE("Marlin::EStallDetector", "[Marlin][EStallDetector]") {
    auto &emsd = EMotorStallDetector::Instance();
    emsd.SetEnabled();
    REQUIRE_FALSE(emsd.Blocked());
    REQUIRE_FALSE(emsd.DetectedOnce());
    REQUIRE(emsd.Enabled());

    // put some data through the detector - this data should work (from the previous test)
    emsd.ProcessSample(1997572);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2054684);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2096993);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2125882);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2072601);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(585962); // now this drop shall be almost detected
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(1976478);
    REQUIRE(emsd.DetectedOnce());
    REQUIRE_FALSE(emsd.Blocked());

    // put some more data through the filter - the FW may not respond to the flag immediately
    // the Detected flag should remain intact as well as the Blocked flag
    emsd.ProcessSample(1997572);
    REQUIRE(emsd.DetectedOnce());
    emsd.ProcessSample(2054684);
    REQUIRE(emsd.DetectedOnce());
    emsd.ProcessSample(2096993);
    REQUIRE(emsd.DetectedOnce());

    { // now simulate what the firmware would do - process an injected M600
        BlockEStallDetection besd;
        REQUIRE(emsd.Blocked());
        // perform M600
    }

    // returned from M600 - Detected and Blocked flags should be off for a new run
    REQUIRE_FALSE(emsd.DetectedOnce());
    REQUIRE_FALSE(emsd.Blocked());

    emsd.ProcessSample(1997572);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2054684);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2096993);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2125882);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(2072601);
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(585962); // now this drop shall be almost detected
    CHECK_FALSE(emsd.DetectedOnce());
    emsd.ProcessSample(1976478);
    CHECK(emsd.DetectedOnce());
    CHECK_FALSE(emsd.Blocked());
}

using TRawData = std::vector<int32_t>;

bool DetectStall(const TRawData &v, float threshold = 700'000.F) {
    MotorStallFilter msf;
    msf.SetDetectionThreshold(threshold);

    for (size_t i = 0; i < v.size(); ++i) {
        bool detected = msf.ProcessSample(v[i]);
        if (detected) {
            INFO(i);
            return true;
        }
    }
    return false;
}

TEST_CASE("Marlin::MotorStallFilter no-stall", "[Marlin][EStallDetector]") {
    // clang-format off
    TRawData raw = {
        84124, 84319, 85036, 85062, 84943, 83407, 84812, 84404, 84157, 84070, 84975, 85322, 85617, 85245, 85651, 84951, 84978, 85058, 85449, 85814, 85224, 85400, 85746, 85331, 85051, 85170, 86216, 85304, 87215, 84997, 86196, 85494, 85515, 85104, 85922, 86642, 85791, 86173, 86788, 84966, 86497, 85951, 85691, 85735, 86139, 85422, 85350, 86938, 86756, 86653, 85809, 85965, 85676, 85684, 86362, 85413, 86797, 86223, 86431, 85393, 86342, 85756, 86238, 86353, 86023, 85798, 85331, 85825, 85443, 85521, 84830, 86325, 86844, 86299, 86641, 86305, 85273, 85157, 87505, 86033, 85955, 86793, 86582, 85999, 86350, 86824, 86269, 86754, 87309, 87883, 87134, 86763, 87165, 86251, 86681, 86239, 86578, 87093, 87089, 86678, 87784, 87316, 86829, 87104, 86951, 87472, 87250, 88206, 87700, 86954, 88128, 88054, 88477, 88316, 88155, 87739, 87985, 87990, 87682, 87106, 86895, 88165, 87561, 87465, 87774, 87480, 87798, 86814, 87323, 89460, 87427, 88561, 88458, 88274, 88485, 88318, 88342, 88265, 87531, 87504, 88099, 89147, 88882, 89139, 88614, 89019, 89172, 88478, 89716, 89179, 88019, 89641, 88491, 88285, 88706, 89058, 89303, 89068, 89947, 89224, 89667, 88844, 88675, 88879, 89463, 88737, 89100, 88740, 90659, 88631, 90091, 90462, 89109, 89368, 89759, 90021, 89352, 89902, 90373, 90847, 90528, 90235, 90830, 90618, 89162, 89997, 90018, 90282, 89473, 90615, 89465, 91114, 91926, 91625, 91290, 90716, 90316, 91049, 91202, 91072, 91034, 90445, 90988, 91854, 91212, 91319, 91251, 91319, 91557, 89610, 90655, 91618, 91102, 91308, 91509, 91646, 91266, 91248, 90586, 91163, 91911, 90970, 91159, 91116, 91168, 91048, 91111, 91192, 90720, 91711, 90669, 89866, 91370, 91526, 91416, 91304, 91315, 91543, 92100, 91533, 90855, 91390, 91784, 90964, 91896, 91221, 91388, 91776, 90605, 91638, 90996, 91606, 90549, 91102, 91515, 91013, 91177, 91940, 91991, 92558, 91140, 92326, 93244, 92376, 92830, 91640, 92848, 92311, 92061, 92157, 92888, 91032, 92007, 92568, 92172, 92341, 92308, 92462, 91736, 93136, 93166, 92913, 92385, 90782, 92368, 92589, 92711, 92691, 92075, 93845, 92271, 92914, 93084, 94032, 92976, 92766, 92539, 92736, 92841, 91908, 94590, 94065, 93036, 94002, 93299, 92990, 92549, 54917, 34000, 47676, 45978, 45988, 47129, 46954, 47093, 45052, 46519, 44686, 46121, 46168, 45940, 45186, 48703, 46537, 47902, 47119, 47460, 49298, 46936, 49418, 44453, 45589, 51072, 45136, 44153, 45472, 46728, 45948, 48550, 45710, 45957, 47188, 46232, 45200, 47695, 46510, 40603, 46367, 46641, -114, -666, 1461, 1526, 82, 245, 2040, 1019, 1119, 1820, 1639, 209, 2537, 2207, 3318, 3228, 2585, 2786, 3081, 3101, 2735, 2506, 2699, 2957, 2264, 3953, 2630, 3327, 4386, 3282, 3548, 3451, 3862, 2810, 2871, 3991, 4000, 4395, 3953, 4164, 4714, 5376, 5135, 5246, 4187, 5895, 5298, 5333, 6009, 5493, 5632, -22672, 5952, 5610, 5989, 4868, 15, 385, -23594, -150, -403, 14351, 13809, -14790, -6427, -4811, -3590, -2612, -1732, -1665, -4053, -1654, -925, -88, -1, -1871, -3682, -1259, -283, -141, 63690, -65587, -10194, -9251, -9408, -10212, 1520, 2491, 3429, 606, 781, 5221, 9731, 8928, 10455, 5019, 4463, 16443, 11565, 9008, -339, -1987, -4654, 759, 1962, 1161, -1478, -4384, -4519, -6538, -9054, -16196, -18725, -17427, -5240, -9028, -12920, -8792, -15700, -1123, 1152, 1907, 3507, 1451, 1307, 2868, -915, -739, -5187, -7825, -21053, -20930, -31084, -27122, -37954, -47699, -43896, -48780, -47435, -48172, -48282, -49369, -45959, -43377, -42526, -52584, -55612, -93772, -204532, -176484, -120622, -92638, -119556, -187357, -122553, -40746, 2064, 5215, 5511, 5213, 5432, 4498, 7820, 7804, 8699, 9810, 6403, 8816, 10419, 8344, 6708, 24027, 46053, 47936, 48513, 64869, 42089, 31922, 33133, 32055, 26329, 13982, 15367, 22992, 24243, 19752, 22660, 15818, 22238, 25875, 16429, 25740, 19809, 27944, 18786, 18801, 18971, 13561, 18708, 16931, 26993, 358647, 827001, 465164, 524631, 522625, 516601, 502929, 499867, 501596, 498167, 498870, 495302, 503580, 504718, 508112, 518322, 510363, 518923, 523225, 524554, 525590, 539573, 540552, 542722, 536880, 533025, 555692, 564925, 552074, 561537, 578002, 592563, 590102, 596405, 591034, 593199, 599003, 598004, 603654, 605727, 609119, 422624, 265422, 194964, 152931, 126438, 105059, 88544, 78050, 69918, 76069, 1133426, 1228749, 619184, 363347, 262779, 204213, 165470, 136208, 107813, 87910, 77453, 68339, 58558, 52308, 50340, 46661, 46537, 47203, 47413, 48160, 48333, 47950, 48780, 49479, 48614, 48591, 48861, 49160, 48294, 48284, 48500, 48988, 48982, 48059, 47097, 47455, 48760, 47031, 47360, -8814, 608, 2073, 3897, 4230, 5892, 6043, 9660, 7879, 8954, -7392, -644, 10663, 11529, 11272, 15003, 219, 3136, 2190, 3004, 3408, 4120, 3935, 4516, 3988, 842, -6897, 4149, 7874, 7681, 7873, 7774, -278, 1209, 563, 196, 1285, 1518, 1781, 3061, 2736, -9055, 2556, 4311, 3316, 3254, 15134, 1619, 272, -108, 737, 968, 521, 1764, 4091, 1155, -7403, 1736, 3935, 3074, 3015, 3869, 3402, 3368, 3464, 3866, 6184, 5398, 5254, 3791, 5654, 3736, 4409, 3443, 4519, 1550, 2639, 3627, 5120, 5592, 5810, 5740, 9226, 8973, 8262, 8045, 7719, 10342, 10217, 9569, 8427, 5371, 7827, 11057, 8003, 7352, 9200, 9517, 465, 167, 538, 604, 1408, 1346, 2048, 2190, 1728, 1346, 1987, 3028, 2719, 2985, 2831, 3161, 3279, 3337, 2996, 3605, 3018, 3655, 2786, 1974, -7953, 3256, 3640, 4895, 8082, 566, -2351, -10363, -3318, -1226, -517, -315, -517, -1793, -8850, 792, 12, -1009, 227, 743, -1384, 256, -10715, -415, 541, -127, -2216, 1034, 2302, 1673, -9432, 1756, 1994, 1473, 1871, 9193, 206, -512, -9509, 710, -9, 673, -1348, 92, 598, -7574, -374, -417, 1479, 707, 572, 1007, -1269, -5971, 1411, 617, 152, 3605, -8342, -7819, -3195, 1177, 1769, 1301, 1012, 680, 2436, 810, 1739, 1819, 1756, 1157, 418, 182, -158, -964, 1176, 360, -353, -126, 1162, 1380, 504, 951, 1334, 1191, 667, 1770, 2597, 2093, 1255, 1976, 1480, 1274, -7144, -1050, 1087, 690, 2624, 5996, -7294, -2260, -7428, -2320, -3550, -4005, -1971, -1977, 975, -7808, -3847, 1283, 1654, 1053, 140, 647, 893, 1558, 1829, 1665, -550, 1070, 1701, 1734, 3155, 1745, 1800, 2252, 3541, 3354, 3987, 4025, 4679, 5328, 4914, 4969, 4745, 5465, 5391, 6400, 5583, 5731, 6155, 6391, 6447, 6441, 6028, 6675, 6589, 4720, 7466, 6593, 6995, 7490, 7587, 7549, 6974, 7007, 6971, 6780, 7232, 7607, 7829, 7747, 8056, 7058, 6713, 8699, 8813, 7025, 7293, 7617, 8163, 8340, 8383, 8912, 8623, 8693, 8134, 7844, 7962, 8567, 8634, 7155, 8188, 8915, 6463, 8399, 7147, 6944, 7811, 7350, 6743, 6675, 6101, 6338, 6676, 5364, 4477, 4105, 4819, 4645, 4578, 4817, 4844, 3072, 3263, 3197, 3839, 3133, 2236, 3795, 3992, 3151, 3160, 2526, 2540, 3525, 2868, 3896, 1970, 3170, 3119, 3176, 2066, 3090, 3672, 3805, 4347, 4429, 2799, 3425, 3373, 4421, 4081, 3941, 4308, 3138, 4792, 5805, 4714, 4722, 4235, 4782, 5053, 4322, 3803, 5318, 4243, 3547, 4337, 5150, 4513, 5318, 4668, 4668, 5004, 4510, 5712, 5142, 4099, 4937, 5166, 4723, 4458, 4525, 4206, 3945, 3734, 3606, 3408, 3153, 2665, 2938, 2531, 3737, 2770, 3446, 2241, 2768, 2989, 2495, 2292, 1983, 2522, 2192, 1879, 1648, 387, 796, 3083, 1524, 1947, 1252, 1755, 1278, 1328, 1027, 960, 0, -243, -1186, 1043, 283832, 202866, 151465, 162519, 149596, 132430, 152272, 159411, 135353, 147554, 154306, 149678, 163154, 281776, 293676, 287615, 296523, 291432, 309803, 348017, 344118, 352619, 346950, 373688, 402309, 399416, 416567, 251437, -63274, -46105, -42671, -40474, -39133, -38416, -66420, -59582, -58736, -56051, -56983, -51766, -51096, -52608, -52619, -51190, -52507, -52437, -49583, 638489, -96935, -52075, -51526, -51067, -49664, -51795, -48842, -49541, -58800, -50438, -51325, -49804, -53878, -54614, -54005, -56516, -55925, -51246, -49149, -50879, -52493, -50099, -49514, -51994, -53394, -54602, -50563, -51665, -53052, -48736, -47362, -52890, -51299, -52479, -54702, -55396, -58988, -77226, -58618, -41096, -46988, -45932, -46827, -46639, -49893, -54454, -46642, -51637, -47735, -45339, -44009, -42303, -42279, -41470, -42091, -41813, -41435, -41313, -41072, -41249, -40878, -40735, -40465, -41110, -40219, -39807, -39794, -44353, -39819, -54527, -52340, -63771, -89949, -76596, -169425, -274257, -76486, -42251, -41115, -39107, -39411, -42961, -42137, -42058, -40750, -39862, -41657, -39373, -40129, -37053, -17855, -24487, -29713, 17142, 62561, 67635, 61448, 38683, 18929, 39000, 36017, 55412, 52288, 55038, 46554, 44594, 27539, 15753, 2877, 14574, 2639, 12639, 31232, 9952, 22998, 9751, 13862, -8329, -4978, 22072, 23760, 46523, 141812, 26030, 24645, 21171, 12249, 117875, 137244, 146079, 143330, 137250, 141687, 140399, 139679, 148391, 147774, 149767, 150259, 153974, 155107, 153797, 156491, 155034, 161259, 168126, 167394, 171633, 162886, 165397, 170333, 173202, 173488, 180328, 182163, 188886, 183407, 183557, 188482, 191321, 190490, 195164, 195309, 72792, 27342, 14062, 6212, 2189, -1571, -2101, 210301, 437068, 434567, 80364, 31143, 16244, 9576, 7012, 4999, 3222, 452, -2599, -4708, -4862, -4585, -2529, -2273, -1981, -2018, -2618, -1264, 105, -1256, -179, -1117, -2154, -1314, -1198, -1266, -1416, -3174, -1994, -2448, -3238, -1355, 221695, 164819, 143870, 136232, 133281, 123644, 127787, 128824, 125693, 117224, 119514, 113164, 112302, 103919, 99705, 93493, 95709, 90344, 94593, 94947, 93538, 91742, 89142, 90317, 90607, 89128, 87176, 85168, 86196, 67860, 87130, 85613, 78830, 86505, 78632, 83564, 77454, 89173, 83383, 85289, 77053, 83304, 81548, 73670, 71120, 74684, 78927, 79326, 79384, 86336, 87583, 90609, 88823, 89721, 91884, 92688, 92650, 93564, 95324, -93285, -57447, -56472, 111053, 98961, 99815, 92486, 98091, 90551, 103960, 99472, 109831, 102530, 109979, 102976, 107817, 99227, 100913, 97348, 92402, 45314, -58734, -57915, -56553, -55183, -54991, -53242, -52986, -53462, -51501, -52242, -52827, -52018, -53654, -48974, 634937, -104739, -54558, -58767, -57248, -54996, -52409, -51023, -51283, -50420, -47807, -48928, -50467, -52765, -52177, -50350, -48018, -50968, -51552, -52964, -50591, -50888, -52290, -53605, -53579, -48618, -45679, -46142, -46039
    };
    // clang-format on
    CHECK_FALSE(DetectStall(raw)); // in this sample, no stall detection shall occur
                                   // - at least the python filter doesn't detect it (which is correct), but the firmware emitted multiple bogus M600's
}

TEST_CASE("Marlin::MotorStallFilter no-stall2", "[Marlin][EStallDetector]") {
    // clang-format off
    TRawData raw = {
        -1225, -1470, -2004, -904, -1742, -2116, -2073, -909, 5041, 14050, 25269, 193812, 317871, 436089, 510097, 710793, 745859, 796605, 843023, 852420, 944511, 969374, 983048, 994434, 1026007, 1031262, 1037433, 1039436, 1058834, 1058264, 1063326, 1058177, 1060067, 1068617, 1068175, 1068741, 1066414, 1066436, 1068125, 1069134, 1061472, 1058877, 817792, 795918, 722114, 673828, 634656, 531668, 505217, 476352, 450455, 398420, 394146, 377336, 362474, 354231, 322626, 312545, 301133, 293208, 271418, 268576, 259810, 251608, 249986, 233789, 231928, 227217, 221067, 218917, 206460, 202367, 198396, 193672, 182528, 181473, 179567, 179188, 159010, 157374, 150174, 148110, 162633, 151074, 154290, 138102, 137025, 139425, 136622, 135418, 129267, 127934, 127743, 124886, 123735, 119778, 120983, 118180, 118109, 114619, 114753, 107652, 110436, 104417, 105350, 101882, 102765, 103726, 99837, 100917, 96629, 96417
    };
    // clang-format on
    CHECK_FALSE(DetectStall(raw));
}

TEST_CASE("Marlin::MotorStallFilter crash-detection->no-stall", "[Marlin][EStallDetector][.]") {
    // One occasion when a crash into an overhang caused false E-stall detection
    // Since the peak looks very similar to a E-stall and further tweaking of the filter would not bring the desired effect:
    // - this unit test has been intentionally disabled
    // - we only consider results of the stall filter when the E-motor is performing some moves - that way we can mitigate false triggers caused by crashes while travelling
    // clang-format off
    TRawData raw = {
        391905, 404371, 405854, 402122, 400523, 392732, 390482, 383685, 375939, 371534, 373195, 383001, 389941, 392299, 409750, 411172, 413329, 398768, 354193, -125842, -86548, -73023, -70571, -60750, -53781, -48922, -48932, -50103, -50580, -47325, -47930, -48959, -46756, -47501, -47244, -46907, -46122, -47183, -46598, -45348, -46271, -45987, -46766, -43987, -46015, -51967, -78746, -74678, -74425, -82025, -82999, 105827, 228888, 401001, 414139, 415429, 431008, 467807, 486993, 531888, 545483, 544382, 562018, 564147, 648376, 658576, 672646, 685942, 687316, 724116, 743281, 737573, 694063, 563891, 612363, 656767, 661416, 674395, 779188, 797234, 840227, 873711, 881713, 980368, 1019104, 1058557, 1078528, 1151262, 1186138, 1214174, 1246571, 1247441, 1216363, 1126591, 947548, 898018, 17194, -26365, -83369, -84023, -91993, -77246, -64873, -56761, -56145, -83734, -81783, -90622, -89850, -116616, -103353, -110324, -115837, -115955, -123351, -129730, -133345, -117380, -143207, 44749, 229528, 581889, 807905, 815321, 763994, 803633, 778222, 802330, 818249, 824280, 790275, 795275, 756400, 744508, 729941, 734947, 720923, 709442, 700795, 698399, 688762, 690040, 686188, 680697, 681209, 671534, 670082, 668078, 669128, 664446, 628915, 611762, 604558, 597925, 526342, 499253, 424411, 338257, 223791, 221090, 223078, 222283, 221705, 220305, 219018, 118125, -91547, -76577, -72494, -67712, -66399, -60848, -59944, -57689, -57928, -57246, -57029, -56656, -58339, -55276, -56303, -56194, -55695, -56449, -56995, -54408, -54882, -54503, -52718
    };
    // clang-format on
    CHECK_FALSE(DetectStall(raw));
}

TEST_CASE("Marlin::MotorStallFilter stall-180Hz", "[Marlin][EStallDetector]") {
    // 2023-11-02 after change of loadcell sampling rate
    // clang-format off
    TRawData raw = {
        98942, 98385, 98007, 98034, 98399, 98490, 98542, 146151, 167215, 313676, 375168, 402157, 879225, 1111218, -46991, -32414, 594033, 430360, -81750, 557247, 462376, 38131, -76215, 597621, 485758, 1164070, -138657, 271674, -67722, 630513, 498364, 417608, 288181, 209869, 631961, 260319, -17518, 76786, -43293, -41805, 796917, 13390, -37097, -33791, -33180, -30885, -27613, -38214, -185926, -166472, -110776, -74534, -31256, 750, 34409, 29547, 27996, 25893, 23528, 22581,
    };
    // clang-format on
    CHECK(DetectStall(raw));
}

TEST_CASE("Marlin::MotorStallFilter stall2-180Hz", "[Marlin][EStallDetector]") {
    // 2023-11-02 after change of loadcell sampling rate
    // clang-format off
    TRawData raw = {
        75134, 101217, 144760, 199883, 214969, 348655, 372020, 386092, 425744, 768085, 964659, 1128557, 1173803, 781900, 889094, 90218, 964957, 607426, 121618, 1037210, 1017893, -89579, -13282, 63068, 71035,
    };
    // clang-format on
    CHECK(DetectStall(raw));
}

TEST_CASE("Marlin::MotorStallFilter stall3-300Hz", "[Marlin][EStallDetector]") {
    // 2023-11-10 this sequence didn't trigger while loading filament, but triggers in the unit tests... the problem seems to be somewhere else...
    // clang-format off
    TRawData raw = {
        38747, 42551, 39195, 107739, 1211501, 57990, 97656, 1422943, 53403, 614073, 1017873, 38285, 1020699, 243842,
        76497, 1401329, 54463, 240977, 1030710, 38553, 577184, 232539, 72789, 1403437, 54892, 232508, 931817, 53089,
        571090, 295593, 74055, 986414, 70924, 46978, 1159754, 202787, 39675, 851286, 558148, 36953, 570048, 299672,
        49188, 329202, 866133, 52526, 154011, 1381584, 53208, 69612, 1435173, 51116, 47776, 1173704, 50824, 56547,
        864145, 82570, 43722, 576822, 259101, 78167, 1030683, 56993, 862171, 53533, 52191, 1313283, 53193, 451648,
        1289335, 38032, 873520, 360940, 52660, 1326319, 54926, 162392, 1152997, 41251, 882594, 369808, 54100, 1306731,
        57248, 164403, 1426799, 49505, 77176, 1396125, 48751, 49527, 1199737, 53435, 60746, 894431, 56632, 171987, 1094496,
        49064, 1178311, 49221, 114579, 1362563, 43615, 355588, 418509, 257543, 671350, 59786, 1342138, 51288, 43551, 1055247,
        58694, 904363, 501015, 41889, 628590, 492120, 38276, 348792, 868909, 925140, 916557, 912337,
        903496, 901024, 899172, 899072, 54361, 40506, 40273
    };
    // clang-format on
    CHECK(DetectStall(raw, 500'000.F));

    {
        auto &emsd = EMotorStallDetector::Instance();
        std::fill(emsd.emf.buffer.begin(), emsd.emf.buffer.end(), 0.F);
        // run the same sequence through the whole detector stack
        EStallDetectionStateLatch esdsl;

        // activate the detector
        emsd.SetEnabled();
        emsd.SetBlocked(false);
        // lower the detection threshold to overcome the sampling rate limitation - see explanation above
        // @@TODO we might process all of the recorded data to see the occasions when the filter would have trigger even though the original try-load would have succeeded
        emsd.SetDetectionThreshold(500'000.F);

        for (size_t i = 0; i < raw.size(); ++i) {
            emsd.ProcessSample(raw[i]);
            if (emsd.Evaluate(true, true)) {
                INFO(i);
                break;
            }
        }
        // check repeated eval - and that has been the problem - Marlin eval call vs. another call from mk4mmu
        // Eval shall return false, because it has already detected a trigger + it is now blocked.
        // The only way of checking the detected flag is though a new function DetectedRaw
        CHECK_FALSE(emsd.Evaluate(true, true));
        CHECK_FALSE(emsd.DetectedOnce());
        CHECK(emsd.DetectedRaw());
    }
}
