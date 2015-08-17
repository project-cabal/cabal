/* Cabal - Legacy Game Implementations
 *
 * Cabal is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Based on the ScummVM (GPLv2+) file of the same name

// Based on the Xentax Wiki documentation:
// http://wiki.xentax.com/index.php/The_Last_Express_SND

#include "lastexpress/data/snd.h"

#include "lastexpress/debug.h"

#include "audio/decoders/adpcm_intern.h"
#include "common/debug.h"
#include "common/memstream.h"
#include "common/substream.h"
#include "common/system.h"

namespace LastExpress {

#pragma region Sound filters tables

static const int stepTable[1424] = {
	0, 0, 0, 0, 128, 256, 384, 512, 0, 0, 0, 0, 128, 256,
	384, 512, 0, 0, 0, 0, 192, 320, 448, 576, 0, 0, 0, 0,
	192, 320, 448, 576, 64, 64, 64, 64, 256, 384, 512, 640,
	64, 64, 64, 64, 256, 384, 512, 640, 128, 128, 128, 128,
	320, 448, 576, 704, 128, 128, 128, 128, 320, 448, 576,
	704, 192, 192, 192, 192, 384, 512, 640, 768, 192, 192,
	192, 192, 384, 512, 640, 768, 256, 256, 256, 256, 448,
	576, 704, 832, 256, 256, 256, 256, 448, 576, 704, 832,
	320, 320, 320, 320, 512, 640, 768, 896, 320, 320, 320,
	320, 512, 640, 768, 896, 384, 384, 384, 384, 576, 704,
	832, 960, 384, 384, 384, 384, 576, 704, 832, 960, 448,
	448, 448, 448, 640, 768, 896, 1024, 448, 448, 448, 448,
	640, 768, 896, 1024, 512, 512, 512, 512, 704, 832, 960,
	1088, 512, 512, 512, 512, 704, 832, 960, 1088, 576,
	576, 576, 576, 768, 896, 1024, 1152, 576, 576, 576,
	576, 768, 896, 1024, 1152, 640, 640, 640, 640, 832,
	960, 1088, 1216, 640, 640, 640, 640, 832, 960, 1088,
	1216, 704, 704, 704, 704, 896, 1024, 1152, 1280, 704,
	704, 704, 704, 896, 1024, 1152, 1280, 768, 768, 768,
	768, 960, 1088, 1216, 1344, 768, 768, 768, 768, 960,
	1088, 1216, 1344, 832, 832, 832, 832, 1024, 1152, 1280,
	1408, 832, 832, 832, 832, 1024, 1152, 1280, 1408, 896,
	896, 896, 896, 1088, 1216, 1344, 1472, 896, 896, 896,
	896, 1088, 1216, 1344, 1472, 960, 960, 960, 960, 1152,
	1280, 1408, 1536, 960, 960, 960, 960, 1152, 1280, 1408,
	1536, 1024, 1024, 1024, 1024, 1216, 1344, 1472, 1600,
	1024, 1024, 1024, 1024, 1216, 1344, 1472, 1600, 1088,
	1088, 1088, 1088, 1280, 1408, 1536, 1664, 1088, 1088,
	1088, 1088, 1280, 1408, 1536, 1664, 1152, 1152, 1152,
	1152, 1344, 1472, 1600, 1728, 1152, 1152, 1152, 1152,
	1344, 1472, 1600, 1728, 1216, 1216, 1216, 1216, 1408,
	1536, 1664, 1792, 1216, 1216, 1216, 1216, 1408, 1536,
	1664, 1792, 1280, 1280, 1280, 1280, 1472, 1600, 1728,
	1856, 1280, 1280, 1280, 1280, 1472, 1600, 1728, 1856,
	1344, 1344, 1344, 1344, 1536, 1664, 1792, 1920, 1344,
	1344, 1344, 1344, 1536, 1664, 1792, 1920, 1408, 1408,
	1408, 1408, 1600, 1728, 1856, 1984, 1408, 1408, 1408,
	1408, 1600, 1728, 1856, 1984, 1472, 1472, 1472, 1472,
	1664, 1792, 1920, 2048, 1472, 1472, 1472, 1472, 1664,
	1792, 1920, 2048, 1536, 1536, 1536, 1536, 1728, 1856,
	1984, 2112, 1536, 1536, 1536, 1536, 1728, 1856, 1984,
	2112, 1600, 1600, 1600, 1600, 1792, 1920, 2048, 2176,
	1600, 1600, 1600, 1600, 1792, 1920, 2048, 2176, 1664,
	1664, 1664, 1664, 1856, 1984, 2112, 2240, 1664, 1664,
	1664, 1664, 1856, 1984, 2112, 2240, 1728, 1728, 1728,
	1728, 1920, 2048, 2176, 2304, 1728, 1728, 1728, 1728,
	1920, 2048, 2176, 2304, 1792, 1792, 1792, 1792, 1984,
	2112, 2240, 2368, 1792, 1792, 1792, 1792, 1984, 2112,
	2240, 2368, 1856, 1856, 1856, 1856, 2048, 2176, 2304,
	2432, 1856, 1856, 1856, 1856, 2048, 2176, 2304, 2432,
	1920, 1920, 1920, 1920, 2112, 2240, 2368, 2496, 1920,
	1920, 1920, 1920, 2112, 2240, 2368, 2496, 1984, 1984,
	1984, 1984, 2176, 2304, 2432, 2560, 1984, 1984, 1984,
	1984, 2176, 2304, 2432, 2560, 2048, 2048, 2048, 2048,
	2240, 2368, 2496, 2624, 2048, 2048, 2048, 2048, 2240,
	2368, 2496, 2624, 2112, 2112, 2112, 2112, 2304, 2432,
	2560, 2688, 2112, 2112, 2112, 2112, 2304, 2432, 2560,
	2688, 2176, 2176, 2176, 2176, 2368, 2496, 2624, 2752,
	2176, 2176, 2176, 2176, 2368, 2496, 2624, 2752, 2240,
	2240, 2240, 2240, 2432, 2560, 2688, 2816, 2240, 2240,
	2240, 2240, 2432, 2560, 2688, 2816, 2304, 2304, 2304,
	2304, 2496, 2624, 2752, 2880, 2304, 2304, 2304, 2304,
	2496, 2624, 2752, 2880, 2368, 2368, 2368, 2368, 2560,
	2688, 2816, 2944, 2368, 2368, 2368, 2368, 2560, 2688,
	2816, 2944, 2432, 2432, 2432, 2432, 2624, 2752, 2880,
	3008, 2432, 2432, 2432, 2432, 2624, 2752, 2880, 3008,
	2496, 2496, 2496, 2496, 2688, 2816, 2944, 3072, 2496,
	2496, 2496, 2496, 2688, 2816, 2944, 3072, 2560, 2560,
	2560, 2560, 2752, 2880, 3008, 3136, 2560, 2560, 2560,
	2560, 2752, 2880, 3008, 3136, 2624, 2624, 2624, 2624,
	2816, 2944, 3072, 3200, 2624, 2624, 2624, 2624, 2816,
	2944, 3072, 3200, 2688, 2688, 2688, 2688, 2880, 3008,
	3136, 3264, 2688, 2688, 2688, 2688, 2880, 3008, 3136,
	3264, 2752, 2752, 2752, 2752, 2944, 3072, 3200, 3328,
	2752, 2752, 2752, 2752, 2944, 3072, 3200, 3328, 2816,
	2816, 2816, 2816, 3008, 3136, 3264, 3392, 2816, 2816,
	2816, 2816, 3008, 3136, 3264, 3392, 2880, 2880, 2880,
	2880, 3072, 3200, 3328, 3456, 2880, 2880, 2880, 2880,
	3072, 3200, 3328, 3456, 2944, 2944, 2944, 2944, 3136,
	3264, 3392, 3520, 2944, 2944, 2944, 2944, 3136, 3264,
	3392, 3520, 3008, 3008, 3008, 3008, 3200, 3328, 3456,
	3584, 3008, 3008, 3008, 3008, 3200, 3328, 3456, 3584,
	3072, 3072, 3072, 3072, 3264, 3392, 3520, 3648, 3072,
	3072, 3072, 3072, 3264, 3392, 3520, 3648, 3136, 3136,
	3136, 3136, 3328, 3456, 3584, 3712, 3136, 3136, 3136,
	3136, 3328, 3456, 3584, 3712, 3200, 3200, 3200, 3200,
	3392, 3520, 3648, 3776, 3200, 3200, 3200, 3200, 3392,
	3520, 3648, 3776, 3264, 3264, 3264, 3264, 3456, 3584,
	3712, 3840, 3264, 3264, 3264, 3264, 3456, 3584, 3712,
	3840, 3328, 3328, 3328, 3328, 3520, 3648, 3776, 3904,
	3328, 3328, 3328, 3328, 3520, 3648, 3776, 3904, 3392,
	3392, 3392, 3392, 3584, 3712, 3840, 3968, 3392, 3392,
	3392, 3392, 3584, 3712, 3840, 3968, 3456, 3456, 3456,
	3456, 3648, 3776, 3904, 4032, 3456, 3456, 3456, 3456,
	3648, 3776, 3904, 4032, 3520, 3520, 3520, 3520, 3712,
	3840, 3968, 4096, 3520, 3520, 3520, 3520, 3712, 3840,
	3968, 4096, 3584, 3584, 3584, 3584, 3776, 3904, 4032,
	4160, 3584, 3584, 3584, 3584, 3776, 3904, 4032, 4160,
	3648, 3648, 3648, 3648, 3840, 3968, 4096, 4224, 3648,
	3648, 3648, 3648, 3840, 3968, 4096, 4224, 3712, 3712,
	3712, 3712, 3904, 4032, 4160, 4288, 3712, 3712, 3712,
	3712, 3904, 4032, 4160, 4288, 3776, 3776, 3776, 3776,
	3968, 4096, 4224, 4352, 3776, 3776, 3776, 3776, 3968,
	4096, 4224, 4352, 3840, 3840, 3840, 3840, 4032, 4160,
	4288, 4416, 3840, 3840, 3840, 3840, 4032, 4160, 4288,
	4416, 3904, 3904, 3904, 3904, 4096, 4224, 4352, 4480,
	3904, 3904, 3904, 3904, 4096, 4224, 4352, 4480, 3968,
	3968, 3968, 3968, 4160, 4288, 4416, 4544, 3968, 3968,
	3968, 3968, 4160, 4288, 4416, 4544, 4032, 4032, 4032,
	4032, 4224, 4352, 4480, 4608, 4032, 4032, 4032, 4032,
	4224, 4352, 4480, 4608, 4096, 4096, 4096, 4096, 4288,
	4416, 4544, 4672, 4096, 4096, 4096, 4096, 4288, 4416,
	4544, 4672, 4160, 4160, 4160, 4160, 4352, 4480, 4608,
	4736, 4160, 4160, 4160, 4160, 4352, 4480, 4608, 4736,
	4224, 4224, 4224, 4224, 4416, 4544, 4672, 4800, 4224,
	4224, 4224, 4224, 4416, 4544, 4672, 4800, 4288, 4288,
	4288, 4288, 4480, 4608, 4736, 4864, 4288, 4288, 4288,
	4288, 4480, 4608, 4736, 4864, 4352, 4352, 4352, 4352,
	4544, 4672, 4800, 4928, 4352, 4352, 4352, 4352, 4544,
	4672, 4800, 4928, 4416, 4416, 4416, 4416, 4608, 4736,
	4864, 4992, 4416, 4416, 4416, 4416, 4608, 4736, 4864,
	4992, 4480, 4480, 4480, 4480, 4672, 4800, 4928, 5056,
	4480, 4480, 4480, 4480, 4672, 4800, 4928, 5056, 4544,
	4544, 4544, 4544, 4736, 4864, 4992, 5120, 4544, 4544,
	4544, 4544, 4736, 4864, 4992, 5120, 4608, 4608, 4608,
	4608, 4800, 4928, 5056, 5184, 4608, 4608, 4608, 4608,
	4800, 4928, 5056, 5184, 4672, 4672, 4672, 4672, 4864,
	4992, 5120, 5248, 4672, 4672, 4672, 4672, 4864, 4992,
	5120, 5248, 4736, 4736, 4736, 4736, 4928, 5056, 5184,
	5312, 4736, 4736, 4736, 4736, 4928, 5056, 5184, 5312,
	4800, 4800, 4800, 4800, 4992, 5120, 5248, 5376, 4800,
	4800, 4800, 4800, 4992, 5120, 5248, 5376, 4864, 4864,
	4864, 4864, 5056, 5184, 5312, 5440, 4864, 4864, 4864,
	4864, 5056, 5184, 5312, 5440, 4928, 4928, 4928, 4928,
	5120, 5248, 5376, 5504, 4928, 4928, 4928, 4928, 5120,
	5248, 5376, 5504, 4992, 4992, 4992, 4992, 5184, 5312,
	5440, 5568, 4992, 4992, 4992, 4992, 5184, 5312, 5440,
	5568, 5056, 5056, 5056, 5056, 5248, 5376, 5504, 5632,
	5056, 5056, 5056, 5056, 5248, 5376, 5504, 5632, 5120,
	5120, 5120, 5120, 5312, 5440, 5568, 5632, 5120, 5120,
	5120, 5120, 5312, 5440, 5568, 5632, 5184, 5184, 5184,
	5184, 5376, 5504, 5632, 5632, 5184, 5184, 5184, 5184,
	5376, 5504, 5632, 5632, 5248, 5248, 5248, 5248, 5440,
	5568, 5632, 5632, 5248, 5248, 5248, 5248, 5440, 5568,
	5632, 5632, 5312, 5312, 5312, 5312, 5504, 5632, 5632,
	5632, 5312, 5312, 5312, 5312, 5504, 5632, 5632, 5632,
	5376, 5376, 5376, 5376, 5568, 5632, 5632, 5632, 5376,
	5376, 5376, 5376, 5568, 5632, 5632, 5632, 5440, 5440,
	5440, 5440, 5632, 5632, 5632, 5632, 5440, 5440, 5440,
	5440, 5632, 5632, 5632, 5632, 5504, 5504, 5504, 5504,
	5632, 5632, 5632, 5632, 5504, 5504, 5504, 5504, 5632,
	5632, 5632, 5632, 5568, 5568, 5568, 5568, 5632, 5632,
	5632, 5632, 5568, 5568, 5568, 5568, 5632, 5632, 5632,
	5632
};

static const int imaTable[1424] = {
	0, 2, 4, 6, 7, 9, 11, 13, 0, -2, -4, -6, -7, -9, -11,
	-13, 1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9,
	-11, -13, -15, 1, 3, 5, 7, 10, 12, 14, 16, -1, -3, -5,
	-7, -10, -12, -14, -16, 1, 3, 6, 8, 11, 13, 16, 18,
	-1, -3, -6, -8, -11, -13, -16, -18, 1, 4, 6, 9, 12,
	15, 17, 20, -1, -4, -6, -9, -12, -15, -17, -20, 1, 4,
	7, 10, 13, 16, 19, 22, -1, -4, -7, -10, -13, -16, -19,
	-22, 1, 4, 8, 11, 14, 17, 21, 24, -1, -4, -8, -11, -14,
	-17, -21, -24, 1, 5, 8, 12, 15, 19, 22, 26, -1, -5,
	-8, -12, -15, -19, -22, -26, 2, 6, 10, 14, 18, 22, 26,
	30, -2, -6, -10, -14, -18, -22, -26, -30, 2, 6, 10,
	14, 19, 23, 27, 31, -2, -6, -10, -14, -19, -23, -27,
	-31, 2, 7, 11, 16, 21, 26, 30, 35, -2, -7, -11, -16,
	-21, -26, -30, -35, 2, 7, 13, 18, 23, 28, 34, 39, -2,
	-7, -13, -18, -23, -28, -34, -39, 2, 8, 14, 20, 25,
	31, 37, 43, -2, -8, -14, -20, -25, -31, -37, -43, 3,
	9, 15, 21, 28, 34, 40, 46, -3, -9, -15, -21, -28, -34,
	-40, -46, 3, 10, 17, 24, 31, 38, 45, 52, -3, -10, -17,
	-24, -31, -38, -45, -52, 3, 11, 19, 27, 34, 42, 50,
	58, -3, -11, -19, -27, -34, -42, -50, -58, 4, 12, 21,
	29, 38, 46, 55, 63, -4, -12, -21, -29, -38, -46, -55,
	-63, 4, 13, 23, 32, 41, 50, 60, 69, -4, -13, -23, -32,
	-41, -50, -60, -69, 5, 15, 25, 35, 46, 56, 66, 76, -5,
	-15, -25, -35, -46, -56, -66, -76, 5, 16, 28, 39, 50,
	61, 73, 84, -5, -16, -28, -39, -50, -61, -73, -84, 6,
	18, 31, 43, 56, 68, 81, 93, -6, -18, -31, -43, -56,
	-68, -81, -93, 6, 20, 34, 48, 61, 75, 89, 103, -6, -20,
	-34, -48, -61, -75, -89, -103, 7, 22, 37, 52, 67, 82,
	97, 112, -7, -22, -37, -52, -67, -82, -97, -112, 8,
	24, 41, 57, 74, 90, 107, 123, -8, -24, -41, -57, -74,
	-90, -107, -123, 9, 27, 45, 63, 82, 100, 118, 136, -9,
	-27, -45, -63, -82, -100, -118, -136, 10, 30, 50, 70,
	90, 110, 130, 150, -10, -30, -50, -70, -90, -110, -130,
	-150, 11, 33, 55, 77, 99, 121, 143, 165, -11, -33, -55,
	-77, -99, -121, -143, -165, 12, 36, 60, 84, 109, 133,
	157, 181, -12, -36, -60, -84, -109, -133, -157, -181,
	13, 40, 66, 93, 120, 147, 173, 200, -13, -40, -66, -93,
	-120, -147, -173, -200, 14, 44, 73, 103, 132, 162, 191,
	221, -14, -44, -73, -103, -132, -162, -191, -221, 16,
	48, 81, 113, 146, 178, 211, 243, -16, -48, -81, -113,
	-146, -178, -211, -243, 17, 53, 89, 125, 160, 196, 232,
	268, -17, -53, -89, -125, -160, -196, -232, -268, 19,
	58, 98, 137, 176, 215, 255, 294, -19, -58, -98, -137,
	-176, -215, -255, -294, 21, 64, 108, 151, 194, 237,
	281, 324, -21, -64, -108, -151, -194, -237, -281, -324,
	23, 71, 118, 166, 213, 261, 308, 356, -23, -71, -118,
	-166, -213, -261, -308, -356, 26, 78, 130, 182, 235,
	287, 339, 391, -26, -78, -130, -182, -235, -287, -339,
	-391, 28, 86, 143, 201, 258, 316, 373, 431, -28, -86,
	-143, -201, -258, -316, -373, -431, 31, 94, 158, 221,
	284, 347, 411, 474, -31, -94, -158, -221, -284, -347,
	-411, -474, 34, 104, 174, 244, 313, 383, 453, 523, -34,
	-104, -174, -244, -313, -383, -453, -523, 38, 115, 191,
	268, 345, 422, 498, 575, -38, -115, -191, -268, -345,
	-422, -498, -575, 42, 126, 210, 294, 379, 463, 547,
	631, -42, -126, -210, -294, -379, -463, -547, -631,
	46, 139, 231, 324, 417, 510, 602, 695, -46, -139, -231,
	-324, -417, -510, -602, -695, 51, 153, 255, 357, 459,
	561, 663, 765, -51, -153, -255, -357, -459, -561, -663,
	-765, 56, 168, 280, 392, 505, 617, 729, 841, -56, -168,
	-280, -392, -505, -617, -729, -841, 61, 185, 308, 432,
	555, 679, 802, 926, -61, -185, -308, -432, -555, -679,
	-802, -926, 68, 204, 340, 476, 612, 748, 884, 1020,
	-68, -204, -340, -476, -612, -748, -884, -1020, 74,
	224, 373, 523, 672, 822, 971, 1121, -74, -224, -373,
	-523, -672, -822, -971, -1121, 82, 246, 411, 575, 740,
	904, 1069, 1233, -82, -246, -411, -575, -740, -904,
	-1069, -1233, 90, 271, 452, 633, 814, 995, 1176, 1357,
	-90, -271, -452, -633, -814, -995, -1176, -1357, 99,
	298, 497, 696, 895, 1094, 1293, 1492, -99, -298, -497,
	-696, -895, -1094, -1293, -1492, 109, 328, 547, 766,
	985, 1204, 1423, 1642, -109, -328, -547, -766, -985,
	-1204, -1423, -1642, 120, 361, 601, 842, 1083, 1324,
	1564, 1805, -120, -361, -601, -842, -1083, -1324, -1564,
	-1805, 132, 397, 662, 927, 1192, 1457, 1722, 1987, -132,
	-397, -662, -927, -1192, -1457, -1722, -1987, 145, 437,
	728, 1020, 1311, 1603, 1894, 2186, -145, -437, -728,
	-1020, -1311, -1603, -1894, -2186, 160, 480, 801, 1121,
	1442, 1762, 2083, 2403, -160, -480, -801, -1121, -1442,
	-1762, -2083, -2403, 176, 529, 881, 1234, 1587, 1940,
	2292, 2645, -176, -529, -881, -1234, -1587, -1940, -2292,
	-2645, 194, 582, 970, 1358, 1746, 2134, 2522, 2910,
	-194, -582, -970, -1358, -1746, -2134, -2522, -2910,
	213, 640, 1066, 1493, 1920, 2347, 2773, 3200, -213,
	-640, -1066, -1493, -1920, -2347, -2773, -3200, 234,
	704, 1173, 1643, 2112, 2582, 3051, 3521, -234, -704,
	-1173, -1643, -2112, -2582, -3051, -3521, 258, 774,
	1291, 1807, 2324, 2840, 3357, 3873, -258, -774, -1291,
	-1807, -2324, -2840, -3357, -3873, 284, 852, 1420, 1988,
	2556, 3124, 3692, 4260, -284, -852, -1420, -1988, -2556,
	-3124, -3692, -4260, 312, 937, 1561, 2186, 2811, 3436,
	4060, 4685, -312, -937, -1561, -2186, -2811, -3436,
	-4060, -4685, 343, 1030, 1718, 2405, 3092, 3779, 4467,
	5154, -343, -1030, -1718, -2405, -3092, -3779, -4467,
	-5154, 378, 1134, 1890, 2646, 3402, 4158, 4914, 5670,
	-378, -1134, -1890, -2646, -3402, -4158, -4914, -5670,
	415, 1247, 2079, 2911, 3742, 4574, 5406, 6238, -415,
	-1247, -2079, -2911, -3742, -4574, -5406, -6238, 457,
	1372, 2287, 3202, 4117, 5032, 5947, 6862, -457, -1372,
	-2287, -3202, -4117, -5032, -5947, -6862, 503, 1509,
	2516, 3522, 4529, 5535, 6542, 7548, -503, -1509, -2516,
	-3522, -4529, -5535, -6542, -7548, 553, 1660, 2767,
	3874, 4981, 6088, 7195, 8302, -553, -1660, -2767, -3874,
	-4981, -6088, -7195, -8302, 608, 1826, 3044, 4262, 5479,
	6697, 7915, 9133, -608, -1826, -3044, -4262, -5479,
	-6697, -7915, -9133, 669, 2009, 3348, 4688, 6027, 7367,
	8706, 10046, -669, -2009, -3348, -4688, -6027, -7367,
	-8706, -10046, 736, 2210, 3683, 5157, 6630, 8104, 9577,
	11051, -736, -2210, -3683, -5157, -6630, -8104, -9577,
	-11051, 810, 2431, 4052, 5673, 7294, 8915, 10536, 12157,
	-810, -2431, -4052, -5673, -7294, -8915, -10536, -12157,
	891, 2674, 4457, 6240, 8023, 9806, 11589, 13372, -891,
	-2674, -4457, -6240, -8023, -9806, -11589, -13372, 980,
	2941, 4903, 6864, 8825, 10786, 12748, 14709, -980, -2941,
	-4903, -6864, -8825, -10786, -12748, -14709, 1078, 3236,
	5393, 7551, 9708, 11866, 14023, 16181, -1078, -3236,
	-5393, -7551, -9708, -11866, -14023, -16181, 1186, 3559,
	5933, 8306, 10679, 13052, 15426, 17799, -1186, -3559,
	-5933, -8306, -10679, -13052, -15426, -17799, 1305,
	3915, 6526, 9136, 11747, 14357, 16968, 19578, -1305,
	-3915, -6526, -9136, -11747, -14357, -16968, -19578,
	1435, 4307, 7179, 10051, 12922, 15794, 18666, 21538,
	-1435, -4307, -7179, -10051, -12922, -15794, -18666,
	-21538, 1579, 4738, 7896, 11055, 14214, 17373, 20531,
	23690, -1579, -4738, -7896, -11055, -14214, -17373,
	-20531, -23690, 1737, 5212, 8686, 12161, 15636, 19111,
	22585, 26060, -1737, -5212, -8686, -12161, -15636, -19111,
	-22585, -26060, 1911, 5733, 9555, 13377, 17200, 21022,
	24844, 28666, -1911, -5733, -9555, -13377, -17200, -21022,
	-24844, -28666, 2102, 6306, 10511, 14715, 18920, 23124,
	27329, 31533, -2102, -6306, -10511, -14715, -18920,
	-23124, -27329, -31533, 2312, 6937, 11562, 16187, 20812,
	25437, 30062, 32767, -2312, -6937, -11562, -16187, -20812,
	-25437, -30062, -32767, 2543, 7631, 12718, 17806, 22893,
	27981, 32767, 32767, -2543, -7631, -12718, -17806, -22893,
	-27981, -32767, -32767, 2798, 8394, 13990, 19586, 25183,
	30779, 32767, 32767, -2798, -8394, -13990, -19586, -25183,
	-30779, -32767, -32767, 3077, 9233, 15389, 21545, 27700,
	32767, 32767, 32767, -3077, -9233, -15389, -21545, -27700,
	-32767, -32767, -32767, 3385, 10157, 16928, 23700, 30471,
	32767, 32767, 32767, -3385, -10157, -16928, -23700,
	-30471, -32767, -32767, -32767, 3724, 11172, 18621,
	26069, 32767, 32767, 32767, 32767, -3724, -11172, -18621,
	-26069, -32767, -32767, -32767, -32767, 4095, 12287,
	20479, 28671, 32767, 32767, 32767, 32767, -4095, -12287,
	-20479, -28671, -32767, -32767, -32767, -32767
};

static const int p1s[17] = { 0, 4, 3, 4, 2, 4, 3, 4, 1, 4, 3, 4,  2, 4,  3, 4,  0 };
static const int p2s[17] = { 0, 1, 1, 3, 1, 5, 3, 7, 1, 9, 5, 11, 3, 13, 7, 15, 1 };

#pragma endregion

// Last Express ADPCM is similar to MS IMA mono, but inverts its nibbles
// and does not have the 4 byte per channel requirement

class LastExpress_ADPCMStream : public Audio::ADPCMStream {
public:
	LastExpress_ADPCMStream(Common::SeekableReadStream *stream, DisposeAfterUse::Flag disposeAfterUse, uint32 blockSize, int32 filterId) :
			Audio::ADPCMStream(stream, disposeAfterUse, 44100, 1, blockSize) {
		_currentFilterId = -1;
		_nextFilterId = filterId;
		_stepAdjust1 = 0;
		_stepAdjust2 = 0;
	}

	int readBuffer(int16 *buffer, const int numSamples) {
		int samples = 0;
		// Temporary data
		int step = 0;
		int sample = 0;
		byte idx = 0;

		assert(numSamples % 2 == 0);

		while (samples < numSamples && !_stream->eos() && _stream->pos() < _stream->size()) {
			if (_blockPos[0] == _blockAlign) {
				// read block header
				_status.ima_ch[0].last = _stream->readSint16LE();
				_status.ima_ch[0].stepIndex = _stream->readSint16LE() << 6;
				_blockPos[0] = 4;

				// Get current filter
				_currentFilterId = _nextFilterId;
				//_nextFilterId = -1; // FIXME: the filter id should be recomputed based on the sound entry status for each block

				// No filter: skip decoding
				if (_currentFilterId == -1)
					break;

				// Compute step adjustment
				_stepAdjust1 = p1s[_currentFilterId];
				_stepAdjust2 = p2s[_currentFilterId];
			}

			for (; samples < numSamples && _blockPos[0] < _blockAlign && !_stream->eos() && _stream->pos() < _stream->size(); samples += 2) {
				byte data = _stream->readByte();
				_blockPos[0]++;

				// First nibble
				idx = data >> 4;
				step = stepTable[idx + _status.ima_ch[0].stepIndex / 4];
				sample = CLIP<int>(imaTable[idx + _status.ima_ch[0].stepIndex / 4] + _status.ima_ch[0].last, -32767, 32767);
				buffer[samples] = (_stepAdjust2 * sample) >> _stepAdjust1;

				// Second nibble
				idx = data & 0xF;
				_status.ima_ch[0].stepIndex = stepTable[idx + step / 4];
				_status.ima_ch[0].last = CLIP(imaTable[idx + step / 4] + sample, -32767, 32767);
				buffer[samples + 1] = (_stepAdjust2 * _status.ima_ch[0].last) >> _stepAdjust1;
			}
		}

		return samples;
	}

	void setFilterId(int32 filterId) { _nextFilterId = filterId; }

private:
	int32 _currentFilterId;
	int32 _nextFilterId;    // the sound filter id, -1 for none
	int32 _stepAdjust1;
	int32 _stepAdjust2;
};

//////////////////////////////////////////////////////////////////////////
// Sound
//////////////////////////////////////////////////////////////////////////
SimpleSound::SimpleSound() : _size(0), _blocks(0), _blockSize(0) {}

SimpleSound::~SimpleSound() {
	stop();
}

// Stop the sound
void SimpleSound::stop() const {
	g_system->getMixer()->stopHandle(_handle);
}

void SimpleSound::loadHeader(Common::SeekableReadStream *in) {
	_size = in->readUint32LE();
	_blocks = in->readUint16LE();
	debugC(5, kLastExpressDebugSound, "    sound header data: size=\"%d\", %d blocks", _size, _blocks);

	assert (_size % _blocks == 0);
	_blockSize = _size / _blocks;
}

LastExpress_ADPCMStream *SimpleSound::makeDecoder(Common::SeekableReadStream *in, int32 filterId) const {
	return new LastExpress_ADPCMStream(in, DisposeAfterUse::YES, _blockSize, filterId);
}

void SimpleSound::play(Audio::AudioStream *as) {
	g_system->getMixer()->playStream(Audio::Mixer::kPlainSoundType, &_handle, as);
}

//////////////////////////////////////////////////////////////////////////
// StreamedSound
//////////////////////////////////////////////////////////////////////////
StreamedSound::StreamedSound() : _as(NULL), _loaded(false) {}

StreamedSound::~StreamedSound() {
	_as = NULL;
}

bool StreamedSound::load(Common::SeekableReadStream *stream, int32 filterId) {
	if (!stream)
		return false;

	g_system->getMixer()->stopHandle(_handle);

	loadHeader(stream);

	// Start decoding the input stream
	stream = new Common::SeekableSubReadStream(stream, stream->pos(), stream->pos() + _size, DisposeAfterUse::YES);
	_as = makeDecoder(stream, filterId);

	// Start playing the decoded audio stream
	play(_as);

	_loaded = true;

	return true;
}

bool StreamedSound::isFinished() {
	if (!_loaded)
		return false;

	return !g_system->getMixer()->isSoundHandleActive(_handle);
}

void StreamedSound::setFilterId(int32 filterId) {
	if (!_as)
		return;

	_as->setFilterId(filterId);
}

//////////////////////////////////////////////////////////////////////////
// StreamedSound
//////////////////////////////////////////////////////////////////////////
AppendableSound::AppendableSound() : SimpleSound() {
	// Create an audio stream where the decoded chunks will be appended
	_as = Audio::makeQueuingAudioStream(44100, 1);
	_finished = false;

	// Start playing the decoded audio stream
	play(_as);

	// Initialize the block size
	// TODO: get it as an argument?
	_blockSize = 739;
}

AppendableSound::~AppendableSound() {
	finish();

	_as = NULL;
}

void AppendableSound::queueBuffer(const byte *data, uint32 size) {
	Common::MemoryReadStream *buffer = new Common::MemoryReadStream(data, size);
	queueBuffer(buffer);
}

void AppendableSound::queueBuffer(Common::SeekableReadStream *bufferIn) {
	if (!_as)
		error("[AppendableSound::queueBuffer] Audio stream is invalid");

	// Setup the ADPCM decoder
	LastExpress_ADPCMStream *adpcm = makeDecoder(bufferIn);
	adpcm->setFilterId(16);

	// Queue the stream
	_as->queueAudioStream(adpcm);
}

void AppendableSound::finish() {
	if (!_as)
		error("[AppendableSound::finish] Audio stream is invalid");

	if (!_finished)
		_as->finish();

	_finished = true;
}

bool AppendableSound::isFinished() {
	return _as->endOfStream();
}

} // End of namespace LastExpress
