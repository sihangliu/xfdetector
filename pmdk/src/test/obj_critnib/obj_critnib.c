/*
 * Copyright 2015-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * obj_critnib.c -- unit test for critnib hash table
 */

#include <errno.h>

#include "alloc.h"
#include "critnib.h"
#include "unittest.h"
#include "util.h"

#define TEST_INSERTS 100
#define TEST_VAL(x) ((void *)((uintptr_t)(x)))

static int Rcounter_malloc;

static void *
__wrap_malloc(size_t size)
{
	switch (util_fetch_and_add32(&Rcounter_malloc, 1)) {
		case 1: /* internal out_err malloc */
		default:
			return malloc(size);
		case 2: /* tab malloc */
		case 0: /* critnib malloc */
			return NULL;
	}
}

static void
test_critnib_new_delete(void)
{
	struct critnib *c = NULL;

	/* critnib malloc fail */
	c = critnib_new();
	UT_ASSERTeq(c, NULL);

	/* first insert malloc fail */
	c = critnib_new();
	UT_ASSERTeq(critnib_insert(c, 0, NULL), ENOMEM);
	critnib_delete(c);

	/* all ok */
	c = critnib_new();
	UT_ASSERTne(c, NULL);

	critnib_delete(c);
}

static void
test_insert_get_remove(void)
{
	struct critnib *c = critnib_new();
	UT_ASSERTne(c, NULL);

	for (unsigned i = 0; i < TEST_INSERTS; ++i)
		UT_ASSERTeq(critnib_insert(c, i, TEST_VAL(i)), 0);

	for (unsigned i = 0; i < TEST_INSERTS; ++i)
		UT_ASSERTeq(critnib_get(c, i), TEST_VAL(i));

	for (unsigned i = 0; i < TEST_INSERTS; ++i)
		UT_ASSERTeq(critnib_remove(c, i), TEST_VAL(i));

	for (unsigned i = 0; i < TEST_INSERTS; ++i)
		UT_ASSERTeq(critnib_remove(c, i), NULL);

	for (unsigned i = 0; i < TEST_INSERTS; ++i)
		UT_ASSERTeq(critnib_get(c, i), NULL);

	critnib_delete(c);
}

static uint64_t
rnd15()
{
	return rand() & 0x7fff; /* Windows provides only 15 bits */
}

static uint64_t
rnd64()
{
	return rnd15()
		| rnd15() << 15
		| rnd15() << 30
		| rnd15() << 45
		| rnd15() << 60;
}

static void
test_smoke()
{
	struct critnib *c = critnib_new();

	critnib_insert(c, 123, (void *)456);
	UT_ASSERTeq(critnib_get(c, 123), (void *)456);
	UT_ASSERTeq(critnib_get(c, 124), 0);

	critnib_delete(c);
}

static void
test_key0()
{
	struct critnib *c = critnib_new();

	critnib_insert(c, 1, (void *)1);
	critnib_insert(c, 0, (void *)2);
	critnib_insert(c, 65536, (void *)3);
	UT_ASSERTeq(critnib_get(c, 1), (void *)1);
	UT_ASSERTeq(critnib_remove(c, 1), (void *)1);
	UT_ASSERTeq(critnib_get(c, 0), (void *)2);
	UT_ASSERTeq(critnib_remove(c, 0), (void *)2);
	UT_ASSERTeq(critnib_get(c, 65536), (void *)3);
	UT_ASSERTeq(critnib_remove(c, 65536), (void *)3);

	critnib_delete(c);
}

static void
test_1to1000()
{
	struct critnib *c = critnib_new();

	for (uint64_t i = 0; i < 1000; i++)
		critnib_insert(c, i, (void *)i);

	for (uint64_t i = 0; i < 1000; i++)
		UT_ASSERTeq(critnib_get(c, i), (void *)i);

	critnib_delete(c);
}

static void
test_insert_delete()
{
	struct critnib *c = critnib_new();

	for (uint64_t i = 0; i < 10000; i++) {
		UT_ASSERTeq(critnib_get(c, i), (void *)0);
		critnib_insert(c, i, (void *)i);
		UT_ASSERTeq(critnib_get(c, i), (void *)i);
		UT_ASSERTeq(critnib_remove(c, i), (void *)i);
		UT_ASSERTeq(critnib_get(c, i), (void *)0);
	}

	critnib_delete(c);
}

static void
test_insert_bulk_delete()
{
	struct critnib *c = critnib_new();

	for (uint64_t i = 0; i < 10000; i++) {
		UT_ASSERTeq(critnib_get(c, i), (void *)0);
		critnib_insert(c, i, (void *)i);
		UT_ASSERTeq(critnib_get(c, i), (void *)i);
	}

	for (uint64_t i = 0; i < 10000; i++) {
		UT_ASSERTeq(critnib_get(c, i), (void *)i);
		UT_ASSERTeq(critnib_remove(c, i), (void *)i);
		UT_ASSERTeq(critnib_get(c, i), (void *)0);
	}

	critnib_delete(c);
}

static void
test_ffffffff_and_friends()
{
	static uint64_t vals[] = {
		0,
		0x7fffffff,
		0x80000000,
		0xffffffff,
		0x7fffffffFFFFFFFF,
		0x8000000000000000,
		0xFfffffffFFFFFFFF,
	};

	struct critnib *c = critnib_new();

	for (int i = 0; i < ARRAY_SIZE(vals); i++)
		critnib_insert(c, vals[i], (void *)~vals[i]);

	for (int i = 0; i < ARRAY_SIZE(vals); i++)
		UT_ASSERTeq(critnib_get(c, vals[i]), (void *)~vals[i]);

	for (int i = 0; i < ARRAY_SIZE(vals); i++)
		UT_ASSERTeq(critnib_remove(c, vals[i]), (void *)~vals[i]);

	critnib_delete(c);
}

static void
test_insert_delete_random()
{
	struct critnib *c = critnib_new();

	for (uint64_t i = 0; i < 10000; i++) {
		uint64_t v = rnd64();
		critnib_insert(c, v, (void *)v);
		UT_ASSERTeq(critnib_get(c, v), (void *)v);
		UT_ASSERTeq(critnib_remove(c, v), (void *)v);
		UT_ASSERTeq(critnib_get(c, v), 0);
	}

	critnib_delete(c);
}

static void
test_le_basic()
{
	struct critnib *c = critnib_new();
#define INS(x) critnib_insert(c, (x), (void *)(x))
	INS(1);
	INS(2);
	INS(3);
	INS(0);
	INS(4);
	INS(0xf);
	INS(0xe);
	INS(0x11);
	INS(0x12);
	INS(0x20);
#define GET_SAME(x) UT_ASSERTeq(critnib_get(c, (x)), (void *)(x))
#define GET_NULL(x) UT_ASSERTeq(critnib_get(c, (x)), NULL)
	GET_NULL(122);
	GET_SAME(1);
	GET_SAME(2);
	GET_SAME(3);
	GET_SAME(4);
	GET_NULL(5);
	GET_SAME(0x11);
	GET_SAME(0x12);
#define LE(x, y) UT_ASSERTeq(critnib_find_le(c, (x)), (void *)(y))
	LE(1, 1);
	LE(2, 2);
	LE(5, 4);
	LE(6, 4);
	LE(0x11, 0x11);
	LE(0x15, 0x12);
	LE(0xfffffff, 0x20);
#undef INS
#undef GET_SAME
#undef GET_NULL
#undef LE
	critnib_delete(c);
}

/*
 * Spread the bits somehow -- more than a few (4 here) children per node
 * is unlikely to bring interested cases.  This function leaves two bits
 * per nib, producing taller trees.
 */
static uint64_t
expand_bits(int y)
{
	uint64_t x = (uint64_t)y;

	return (x & 0xc000) << 14 | (x & 0x3000) << 12 | (x & 0x0c00) << 10 |
		(x & 0x0300) << 8 | (x & 0x00c0) << 6 | (x & 0x0030) << 4 |
		(x & 0x000c) << 2 | (x & 0x0003);
}

static void
test_le_brute()
{
	struct critnib *c = critnib_new();
	char ws[65536] = {
		0,
	};

	for (uint32_t cnt = 0; cnt < 1024; cnt++) {
		int w = rand() & 0xffff;
		if (ws[w]) {
			critnib_remove(c, expand_bits(w));
			ws[w] = 0;
		} else {
			critnib_insert(c, expand_bits(w),
				(void *)expand_bits(w));
			ws[w] = 1;
		}

		for (uint32_t cnt2 = 0; cnt2 < 1024; cnt2++) {
			w = rand() & 0xffff;
			int v;
			for (v = w; v >= 0 && !ws[v]; v--)
				;
			uint64_t res =
				(uint64_t)critnib_find_le(c, expand_bits(w));
			uint64_t exp = (v >= 0) ? expand_bits(v) : 0;
			UT_ASSERTeq(res, exp);
		}
	}

	critnib_delete(c);
}

static void
test_same_only()
{
	struct critnib *c = critnib_new();

	critnib_insert(c, 123, (void *)456);
	critnib_insert(c, 123, (void *)457);
	UT_ASSERTeq(critnib_get(c, 123), (void *)456);
	UT_ASSERTeq(critnib_get(c, 124), 0);

	critnib_delete(c);
}

static void
test_same_two()
{
	struct critnib *c = critnib_new();

	critnib_insert(c, 122, (void *)111);
	critnib_insert(c, 123, (void *)456);
	critnib_insert(c, 123, (void *)457);
	UT_ASSERTeq(critnib_get(c, 122), (void *)111);
	UT_ASSERTeq(critnib_get(c, 123), (void *)456);
	UT_ASSERTeq(critnib_get(c, 124), 0);

	critnib_delete(c);
}

static void
test_remove_nonexist()
{
	struct critnib *c = critnib_new();

	/* root */
	UT_ASSERTeq(critnib_remove(c, 1), NULL);

	/* in a leaf node */
	critnib_insert(c, 2, (void *)2);
	UT_ASSERTeq(critnib_remove(c, 1), NULL);

	/* in a non-leaf node */
	critnib_insert(c, 3, (void *)3);
	UT_ASSERTeq(critnib_remove(c, 1), NULL);

	critnib_delete(c);
}

int
main(int argc, char *argv[])
{
	START(argc, argv, "obj_critnib");

	set_func_malloc(__wrap_malloc);

	test_critnib_new_delete();
	test_insert_get_remove();

	test_smoke();
	test_key0();
	test_1to1000();
	test_insert_delete();
	test_insert_bulk_delete();
	test_ffffffff_and_friends();
	test_insert_delete_random();
	test_le_basic();
	test_le_brute();
	test_same_only();
	test_same_two();
	test_remove_nonexist();

	DONE(NULL);
}
