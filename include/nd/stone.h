/* stone.h — nd-stone's cross-module type: the map tile stones/minerals appear on. */

#ifndef ND_STONE_H
#define ND_STONE_H

#include <stdint.h>
#include <string.h>

typedef struct {
	unsigned id[3];
	unsigned char n, max;
} stone_tile_t;

#ifndef ND_XXH32_U32_DEFINED
#define ND_XXH32_U32_DEFINED 1
static inline uint32_t
nd_xxh32_u32(uint32_t val, uint32_t seed)
{
	uint32_t h32 = seed + 0x165667B1u + 4u;
	h32 += val * 0xC2B2AE3Du;
	h32 = ((h32 << 17) | (h32 >> 15)) * 0x27D4EB2Fu;
	h32 ^= h32 >> 15;
	h32 *= 0x85EBCA77u;
	h32 ^= h32 >> 13;
	h32 *= 0xC2B2AE3Du;
	h32 ^= h32 >> 16;
	return h32;
}
#endif

/* Size of one stone from 3 hash bits: 5/8 small stone (1), 2/8 rock (2),
 * 1/8 boulder (3). Mostly pebbles, some rocks, the odd boulder. */
static inline unsigned char
stone_size_roll(unsigned bits)
{
	unsigned r = bits & 7;

	return (unsigned char)(r < 5 ? 1 : r < 7 ? 2 : 3);
}

static inline void
nd_stone_noise_eval(stone_tile_t *sd, uint32_t he, uint32_t w, uint8_t bio_idx,
                    uint32_t tm, const unsigned skel_ids[3])
{
	memset(sd, 0, sizeof(*sd));
	if (he <= w || bio_idx == 0)
		return;

	uint32_t sv = nd_xxh32_u32(he, 0x5709Eu ^ tm);
	unsigned char n = (unsigned char)(sv & 0x3u);
	if (!n)
		return;

	/* Biome-aware geological stone & boulder frequency:
	 *   - Volcanic (18), Cold Desert (6), Tundra (2), Shrubland (7), Mediterranean (4):
	 *     rocky outcrops and frost-shattered/arid talus (~11-18% of rooms).
	 *   - Deep soils in Temperate Grassland (10), Wetland (5), Desert sand ergs (14),
	 *     and Tropical Rainforest humus (17): sparse surface stones (~1.2% of rooms).
	 *   - Forests, Alpine Meadows & Woodlands: classic 4.7% frequency. */
	int pass_gate = 0;
	int rocky_bias = 0;
	switch (bio_idx) {
	case 18: /* volcanic basalt & obsidian */
		pass_gate = ((sv & 0x18u) != 0);
		rocky_bias = 2;
		break;
	case 2:  /* tundra frost-shattered felsenmeer */
	case 6:  /* cold desert gravel & boulders */
	case 7:  /* rocky chaparral / shrubland */
		pass_gate = ((sv & 0x38u) >= 0x18u);
		rocky_bias = 1;
		break;
	case 3:  /* alpine meadow erratics */
	case 4:  /* mediterranean limestone garrigue */
		pass_gate = ((sv & 0x18u) != 0 && (sv & 0x60u) != 0);
		rocky_bias = 1;
		break;
	case 5:  /* wetland peat */
	case 10: /* deep prairie grassland loam */
	case 14: /* eolian sand erg */
	case 17: /* deep rainforest humus */
		pass_gate = ((sv & 0xF8u) == 0x38u);
		n = 1;
		break;
	default:
		pass_gate = ((sv & 0x18u) != 0 && (sv & 0x20u) != 0);
		break;
	}
	if (!pass_gate)
		return;

	unsigned char placed = 0;
	for (int i = 0; i < (int)n && i < 3; i++) {
		unsigned side = (unsigned)((sv >> (8 + i * 3)) % 3u);
		if ((placed >> side) & 1u)
			side = (side + 1u) % 3u;
		placed |= (unsigned char)(1u << side);

		unsigned roll_bits = (sv >> (16 + i * 3)) & 7u;
		if (rocky_bias > 0 && roll_bits >= 3u && roll_bits < 7u)
			roll_bits += 1u;
		unsigned char sz = stone_size_roll(roll_bits);
		sd->id[side] = skel_ids ? skel_ids[(sz >= 1 && sz <= 3) ? (sz - 1) : 0] : (unsigned)sz;
		sd->n |= (unsigned char)(sz << (side * 2));
	}
	sd->max = n;
}

static inline int
nd_stone_slot_glyph(const stone_tile_t *sd, unsigned side, uint8_t *out_glyph, uint8_t *out_fg)
{
	if (!sd || side >= 3)
		return 0;
	unsigned char sz = (unsigned char)((sd->n >> (side * 2)) & 3u);
	if (sz == 1) {
		if (out_glyph) *out_glyph = '*';
		if (out_fg)    *out_fg    = 7;
		return 1;
	}
	if (sz == 2) {
		if (out_glyph) *out_glyph = 'o';
		if (out_fg)    *out_fg    = 3;
		return 1;
	}
	if (sz == 3) {
		if (out_glyph) *out_glyph = 'O';
		if (out_fg)    *out_fg    = 15;
		return 1;
	}
	return 0;
}

#endif /* !ND_STONE_H */
