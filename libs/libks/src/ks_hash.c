/*
 * Copyright (c) 2002, Christopher Clark
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ks.h"
#include "ks_hash.h"

struct entry
{
    void *k, *v;
    unsigned int h;
	ks_hashtable_flag_t flags;
	ks_hashtable_destructor_t destructor;
    struct entry *next;
};

struct ks_hashtable_iterator {
	unsigned int pos;
	struct entry *e;
	struct ks_hashtable *h;
};

struct ks_hashtable {
    unsigned int tablelength;
    struct entry **table;
    unsigned int entrycount;
    unsigned int loadlimit;
    unsigned int primeindex;
    unsigned int (*hashfn) (void *k);
    int (*eqfn) (void *k1, void *k2);
};

/*****************************************************************************/

/*****************************************************************************/
static inline unsigned int
hash(ks_hashtable_t *h, void *k)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    unsigned int i = h->hashfn(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}


/*****************************************************************************/
/* indexFor */
static __inline__ unsigned int
indexFor(unsigned int tablelength, unsigned int hashvalue) {
    return (hashvalue % tablelength);
}

/* Only works if tablelength == 2^N */
/*static inline unsigned int
  indexFor(unsigned int tablelength, unsigned int hashvalue)
  {
  return (hashvalue & (tablelength - 1u));
  }
*/

/*****************************************************************************/
#define freekey(X) free(X)

/*
  Credit for primes table: Aaron Krowne
  http://br.endernet.org/~akrowne/
  http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
*/
static const unsigned int primes[] = {
	53, 97, 193, 389,
	769, 1543, 3079, 6151,
	12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869,
	3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189,
	805306457, 1610612741
};
const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor = 0.65f;

/*****************************************************************************/
KS_DECLARE(ks_status_t)
ks_create_hashtable(ks_hashtable_t **hp, unsigned int minsize,
						unsigned int (*hashf) (void*),
						int (*eqf) (void*,void*))
{
    ks_hashtable_t *h;
    unsigned int pindex, size = primes[0];

    /* Check requested hashtable isn't too large */
    if (minsize > (1u << 30)) {*hp = NULL; return KS_STATUS_FAIL;}
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { 
			size = primes[pindex]; 
			break; 
		}
    }
    h = (ks_hashtable_t *) malloc(sizeof(ks_hashtable_t));

    if (NULL == h) abort(); /*oom*/

    h->table = (struct entry **)malloc(sizeof(struct entry*) * size);

    if (NULL == h->table) abort(); /*oom*/

    memset(h->table, 0, size * sizeof(struct entry *));
    h->tablelength  = size;
    h->primeindex   = pindex;
    h->entrycount   = 0;
    h->hashfn       = hashf;
    h->eqfn         = eqf;
    h->loadlimit    = (unsigned int) ceil(size * max_load_factor);

	*hp = h;
	return KS_STATUS_SUCCESS;
}

/*****************************************************************************/
static int
hashtable_expand(ks_hashtable_t *h)
{
    /* Double the size of the table to accomodate more entries */
    struct entry **newtable;
    struct entry *e;
    struct entry **pE;
    unsigned int newsize, i, index;
    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (struct entry **)malloc(sizeof(struct entry*) * newsize);
    if (NULL != newtable)
		{
			memset(newtable, 0, newsize * sizeof(struct entry *));
			/* This algorithm is not 'stable'. ie. it reverses the list
			 * when it transfers entries between the tables */
			for (i = 0; i < h->tablelength; i++) {
				while (NULL != (e = h->table[i])) {
					h->table[i] = e->next;
					index = indexFor(newsize,e->h);
					e->next = newtable[index];
					newtable[index] = e;
				}
			}
			ks_safe_free(h->table);
			h->table = newtable;
		}
    /* Plan B: realloc instead */
    else 
		{
			newtable = (struct entry **)
				realloc(h->table, newsize * sizeof(struct entry *));
			if (NULL == newtable) { (h->primeindex)--; return 0; }
			h->table = newtable;
			memset(newtable[h->tablelength], 0, newsize - h->tablelength);
			for (i = 0; i < h->tablelength; i++) {
				for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
					index = indexFor(newsize,e->h);

					if (index == i) {
						pE = &(e->next);
					} else {
						*pE = e->next;
						e->next = newtable[index];
						newtable[index] = e;
					}
				}
			}
		}
    h->tablelength = newsize;
    h->loadlimit   = (unsigned int) ceil(newsize * max_load_factor);
    return -1;
}

/*****************************************************************************/
KS_DECLARE(unsigned int)
ks_hashtable_count(ks_hashtable_t *h)
{
    return h->entrycount;
}

static void * _ks_hashtable_remove(ks_hashtable_t *h, void *k, unsigned int hashvalue, unsigned int index) {
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */

    struct entry *e;
    struct entry **pE;
    void *v;


    pE = &(h->table[index]);
    e = *pE;
    while (NULL != e) {
		/* Check hash value to short circuit heavier comparison */
		if ((hashvalue == e->h) && (h->eqfn(k, e->k))) {
			*pE = e->next;
			h->entrycount--;
			v = e->v;
			if (e->flags & HASHTABLE_FLAG_FREE_KEY) {
				freekey(e->k);
			}
			if (e->flags & HASHTABLE_FLAG_FREE_VALUE) {
				ks_safe_free(e->v); 
				v = NULL;
			} else if (e->destructor) {
				e->destructor(e->v);
				v = e->v = NULL;
			}
			ks_safe_free(e);
			return v;
		}
		pE = &(e->next);
		e = e->next;
	}
    return NULL;
}

/*****************************************************************************/
KS_DECLARE(int)
ks_hashtable_insert_destructor(ks_hashtable_t *h, void *k, void *v, ks_hashtable_flag_t flags, ks_hashtable_destructor_t destructor)
{
    struct entry *e;
	unsigned int hashvalue = hash(h, k);
    unsigned index = indexFor(h->tablelength, hashvalue);

	if (flags & HASHTABLE_DUP_CHECK) {
		_ks_hashtable_remove(h, k, hashvalue, index);
	}

    if (++(h->entrycount) > h->loadlimit)
		{
			/* Ignore the return value. If expand fails, we should
			 * still try cramming just this value into the existing table
			 * -- we may not have memory for a larger table, but one more
			 * element may be ok. Next time we insert, we'll try expanding again.*/
			hashtable_expand(h);
			index = indexFor(h->tablelength, hashvalue);
		}
    e = (struct entry *)malloc(sizeof(struct entry));
    if (NULL == e) { --(h->entrycount); return 0; } /*oom*/
    e->h = hashvalue;
    e->k = k;
    e->v = v;
	e->flags = flags;
	e->destructor = destructor;
    e->next = h->table[index];
    h->table[index] = e;
    return -1;
}

/*****************************************************************************/
KS_DECLARE(void *) /* returns value associated with key */
ks_hashtable_search(ks_hashtable_t *h, void *k)
{
    struct entry *e;
    unsigned int hashvalue, index;
    hashvalue = hash(h,k);
    index = indexFor(h->tablelength,hashvalue);
    e = h->table[index];
    while (NULL != e) {
		/* Check hash value to short circuit heavier comparison */
		if ((hashvalue == e->h) && (h->eqfn(k, e->k))) return e->v;
		e = e->next;
	}
    return NULL;
}

/*****************************************************************************/
KS_DECLARE(void *) /* returns value associated with key */
ks_hashtable_remove(ks_hashtable_t *h, void *k)
{
	unsigned int hashvalue = hash(h,k);
	return _ks_hashtable_remove(h, k, hashvalue, indexFor(h->tablelength,hashvalue));
}

/*****************************************************************************/
/* destroy */
KS_DECLARE(void)
ks_hashtable_destroy(ks_hashtable_t **h)
{
    unsigned int i;
    struct entry *e, *f;
    struct entry **table = (*h)->table;

	for (i = 0; i < (*h)->tablelength; i++) {
		e = table[i];
		while (NULL != e) {
			f = e; e = e->next; 

			if (f->flags & HASHTABLE_FLAG_FREE_KEY) {
				freekey(f->k); 
			}
			
			if (f->flags & HASHTABLE_FLAG_FREE_VALUE) {
				ks_safe_free(f->v); 
			} else if (f->destructor) {
				f->destructor(f->v);
				f->v = NULL;
			}
			ks_safe_free(f); 
		}
	}
    
    ks_safe_free((*h)->table);
	free(*h);
	*h = NULL;
}

KS_DECLARE(ks_hashtable_iterator_t *) ks_hashtable_next(ks_hashtable_iterator_t **iP)
{

	ks_hashtable_iterator_t *i = *iP;
	
	if (i->e) {
		if ((i->e = i->e->next) != 0) { 
			return i;
		} else {
			i->pos++;
		}
	}

	while(i->pos < i->h->tablelength && !i->h->table[i->pos]) {
		i->pos++;
	}
	
	if (i->pos >= i->h->tablelength) {
		goto end;
	}
	
	if ((i->e = i->h->table[i->pos]) != 0) { 
		return i;
	}

 end:

	free(i);
	*iP = NULL;

	return NULL;
}

KS_DECLARE(ks_hashtable_iterator_t *) ks_hashtable_first_iter(ks_hashtable_t *h, ks_hashtable_iterator_t *it)
{
	ks_hashtable_iterator_t *iterator;

	if (it) {
		iterator = it;
	} else {
		ks_zmalloc(iterator, sizeof(*iterator));
	}

	ks_assert(iterator);

	iterator->pos = 0;
	iterator->e = NULL;
	iterator->h = h;

	return ks_hashtable_next(&iterator);
}

KS_DECLARE(void) ks_hashtable_this_val(ks_hashtable_iterator_t *i, void *val)
{
	if (i->e) {
		i->e->v = val;
	}
}

KS_DECLARE(void) ks_hashtable_this(ks_hashtable_iterator_t *i, const void **key, ssize_t *klen, void **val)
{
	if (i->e) {
		if (key) {
			*key = i->e->k;
		}
		if (klen) {
			*klen = (int)strlen(i->e->k);
		}
		if (val) {
			*val = i->e->v;
		}
	} else {
		if (key) {
			*key = NULL;
		}
		if (klen) {
			*klen = 0;
		}
		if (val) {
			*val = NULL;
		}
	}
}


/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */

