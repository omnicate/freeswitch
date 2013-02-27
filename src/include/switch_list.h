/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2012, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Eliot Gable <egable@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Eliot Gable <egable@gmail.com>
 *
 * switch_list.h -- Basic Double Linked-List
 */

/*! \file switch_list.h
    \brief Basic Double Linked-List

	This is a basic double linked-list. If you wish to share a single instance of this list across multiple
	threads, you will need to protect it with a mutex or rwlock. 

	Inserts, deletes, appends, pushes, pops, shifts, unshifts, slices, and splices all have an amortized
	O(1) cost. The size of the list is maintained on all operations and is O(1) as long as you do not
	slice, splice, or concatinate elements from/to the list. The first call to list_size() after such an
	operation is O(n). Thus, if you use list_size() along with slice, splice, and concatination, you will
	incur serious performance bottlenecks.
*/

/*!
  \defgroup list Double Linked List
  \ingroup core1
  \{
*/

#ifndef SWITCH_LIST_H
#define SWITCH_LIST_H

#include <switch.h>

SWITCH_BEGIN_EXTERN_C

/* List API */

/*
 * \fn switch_status_t switch_list_create(switch_list_t **list)
 * \brief Create an empty, double linked-list.
 * \param list A pointer indicating where you want to store the created list.
 * \return SWITCH_SATUS_SUCCESS if the list was created, or SWITCH_STATUS_FALSE if creation failed.
 */

/*
 * 
 */


/*
 * \brief Defines the types and prototypes needed for a specialized list.
 * \param typename The name of the type to prefix to the list type names and list function prototype names.
 * \param type The data type to use in the list_node structure.
 */
#define SWITCH_DEFINE_LIST_PROTOTYPES(typename, type)					\
	struct typename##_list;												\
	typedef struct typename##_list switch_list_t;						\
	struct typename##_list_node;										\
	typedef struct typename##_list_node typename##_list_node_t;			\
																		\
	typedef switch_bool_t (*typename##_list_equal_func_t)(const type *left, const type *right);	\
	typedef switch_bool_t (*typename##_list_less_than_func_t)(const type *left, const type *right);	\
	typedef switch_bool_t (*typename##_list_greater_than_func_t)(const type *left, const type *right); \
																		\
	/*! \brief Representation of a typename##_list_node */				\
	struct typename##_list_node {										\
		/*! The data object for this node */							\
	    type *data;														\
		/*! The next item in the list */								\
		typename##_list_node_t *next;									\
		/*! The previous item in the list */							\
		typename##_list_node_t *prev;									\
	};																	\
																		\
	/*! \brief Representation of a typename##_list */					\
	struct typename##_list {											\
		/*! The head node of the list */								\
		typename##_list_node_t *head;									\
		/*! The last node in the list */								\
		typename##_list_node_t *tail;									\
		/*! Equality comparator function */								\
		typename##_list_equal_func_t *equals;							\
		/*! Less than comparator function */							\
		typename##_list_less_than_func_t *less_than;					\
		/*! Greater than comparator function */							\
		typename##_list_greater_than_func_t *greater_than;				\
		/*! This contains the number of items in the list */			\
		switch_size_t size;												\
		/*! Whether the size needs to be recalculated */				\
		switch_bool_t size_dirty;										\
	};																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_node_destroy(typename##_list_node_t **node); \
	SWITCH_DECLARE(switch_status_t) typename##_list_create(typename##_list_t **list); \
	SWITCH_DECLARE(switch_status_t) typename##_list_destroy(typename##_list_t **list); \
	SWITCH_DECLARE(switch_status_t) typename##_list_dup(typename##_list_t **list, const typename##_list_t *to_dup); \
	SWITCH_DECLARE(switch_status_t) typename##_list_insert_after(typename##_list_t *list, typename##_list_node_t *position, type* data); \
	SWITCH_DECLARE(switch_status_t) typename##_list_insert_before(typename##_list_t *list, typename##_list_node_t *position, type *data); \
	SWITCH_DECLARE(switch_status_t) typename##_list_append(typename##_list_t *list, type *data);	\
	SWITCH_DECLARE(switch_status_t) typename##_list_push_right(typename##_list_t *list, type *data);	\
	SWITCH_DECLARE(type *) typename##_list_pop(typename##_list_t *list);	\
	SWITCH_DECLARE(type *) typename##_list_pop_right(typename##_list_t *list);	\
	SWITCH_DECLARE(switch_status_t) typename##_list_unshift(typename##_list_t *list, type *data); \
	SWITCH_DECLARE(switch_status_t) typename##_list_push_left(typename##_list_t *list, type *data); \
	SWITCH_DECLARE(type *) typename##_list_shift(typename##_list_t *list); \
	SWITCH_DECLARE(type *) typename##_list_pop_left(typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_t *) typename##_list_slice(typename##_list_t *src_list, typename##_list_node_t *first, typename##_list_node_t *last); \
	SWITCH_DECLARE(typename##_list_t *) typename##_list_slicen(typename##_list_t *src_list, typename##_list_node_t *first, const switch_size_t count); \
	SWITCH_DECLARE(switch_status_t) typename##_list_splice_range(typename##_list_t *list, typename##_list_node_t *replace_start, \
																 typename##_list_node_t *replace_end, typename##_list_t **to_insert); \
	SWITCH_DECLARE(switch_status_t) typename##_list_splice_range_dup(typename##_list_t *list, typename##_list_node_t *replace_start, \
																	 typename##_list_node_t *replace_end, const typename##_list_t *to_insert); \
	SWITCH_DECLARE(switch_status_t) typename##_list_splicen(typename##_list_t *list, typename##_list_node_t *replace_start, \
															const switch_size_t count, typename##_list_t **to_insert); \
	SWITCH_DECLARE(switch_status_t) typename##_list_splicen_dup(typename##_list_t *list, typename##_list_node_t *replace_start, \
																const switch_size_t count, const typename##_list_t *to_insert); \
	SWITCH_DECLARE(switch_status_t) typename##_list_concat(typename##_list_t *list1, typename##_list_t **list2); \
	SWITCH_DECLARE(switch_status_t) typename##_list_concat_dup(typename##_list_t *list1, const typename##_list_t *list2); \
																		\
	SWITCH_DECLARE(switch_size_t) typename##_list_size(typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_head(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_tail(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_top(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_bottom(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_left(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_right(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_first(const typename##_list_t *list); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_last(const typename##_list_t *list);  \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_next(const typename##_list_node_t *node); \
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_prev(const typename##_list_node_t *node);


SWITCH_DEFINE_LIST_PROTOTYPES(switch, void)


#define SWITCH_DEFINE_LIST_FUNCTIONS(typename, type)					\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_head(const typename##_list_t *list) \
	{																	\
		return list->head;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_tail(const typename##_list_t *list) \
	{																	\
		return list->tail;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_top(const typename##_list_t *list) \
	{																	\
		return list->head;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_bottom(const typename##_list_t *list) \
	{																	\
		return list->tail;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_left(const typename##_list_t *list) \
	{																	\
		return list->head;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_right(const typename##_list_t *list) \
	{																	\
		return list->tail;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_first(const typename##_list_t *list) \
	{																	\
		return list->head;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_last(const typename##_list_t *list) \
	{																	\
		return list->tail;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_next(const typename##_list_node_t *node) \
	{																	\
		return node->next;												\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_node_t *) typename##_list_prev(const typename##_list_node_t *node) \
	{																	\
		return node->prev;												\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_create(typename##_list_t **list_in_out) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		typename##_list_t *list = NULL;									\
																		\
		switch_zmalloc(list, sizeof(*list));							\
		if (!list) {													\
			list = NULL;												\
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT,		\
							  "Out of Memory: Failed to allocate memory for list during initialization in %s!\n", __FUNCTION__); \
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_destroy(typename##_list_t **list_in_out) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		typename##_list_t *list = NULL;									\
																		\
		if (!list_in_out) {												\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		list = *list_in_out;											\
																		\
		if (list->head) {												\
			while (list->head) {										\
				typename##_list_shift(list);							\
			}															\
		}																\
		switch_safe_free(list);											\
		*list_in_out = NULL;											\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_dup(typename##_list_t **list_out, const typename##_list_t *to_dup)	\
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		typename##_list_t *list = NULL;									\
		typename##_list_node_t *node = NULL, *cur_node = NULL, *prev = NULL; \
																		\
		if (!to_dup) {													\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		if (typename##_list_create(&list) != SWITCH_STATUS_SUCCESS) {	\
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failed to create list during duplication in %s!\n", __FUNCTION__); \
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		/* Duplicated an empty list */									\
		if (!to_dup->head) {											\
			switch_goto_status(SWITCH_STATUS_SUCCESS, done);			\
		}																\
																		\
		switch_zmalloc(node, sizeof(*node));							\
		if (!node) {													\
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failed to create list node during duplication in %s!\n", __FUNCTION__); \
			typename##_list_destroy(&list);								\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		list->head = node;												\
		node->data = to_dup->head->data;								\
		prev = list->head;												\
		cur_node = to_dup->head;										\
		while (cur_node) {												\
			switch_zmalloc(node, sizeof(*node));						\
			if (!node) {												\
				/* Either fully dup the list, or dup none of it */		\
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failed to create list node during duplication in %s!\n", __FUNCTION__); \
				typename##_list_destroy(&list);							\
				switch_goto_status(SWITCH_STATUS_FALSE, done);			\
			}															\
			prev->next = node;											\
			node->prev = prev;											\
			node->data = cur_node->data;								\
			prev = node;												\
																		\
			cur_node = cur_node->next;									\
		}																\
		list->tail = node;												\
																		\
		*list_out = list;												\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_insert_after(typename##_list_t *list, typename##_list_node_t *position, type* data) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		typename##_list_node_t *tmp = NULL;								\
																		\
		if (!list || !position || !data) {								\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		switch_zmalloc(tmp, sizeof(*tmp));								\
		if (!tmp) {														\
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failed to create list node during insertion in %s!\n", __FUNCTION__); \
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
		tmp->next = position->next;										\
		tmp->prev = position;											\
																		\
		/* Case where there is another node after position */			\
		if (tmp->next) {												\
			tmp->next->prev = tmp;										\
		}																\
																		\
		position->next = tmp;											\
		tmp->data = data;												\
																		\
		/* Case where position is the last node in the list */			\
		if (list->tail == position) {									\
			list->tail = tmp;											\
		}																\
																		\
		if (!list->size_dirty) {										\
			list->size++;												\
		}																\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_insert_before(typename##_list_t *list, typename##_list_node_t *position, type *data) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
																		\
		typename##_list_node_t *tmp = NULL;								\
																		\
		if (!list || !position || !data) {								\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		switch_zmalloc(tmp, sizeof(*tmp));								\
		if (!tmp) {														\
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failed to create list node during insertion in %s!\n", __FUNCTION__); \
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
		tmp->next = position;											\
		tmp->prev = position->prev;										\
																		\
		/* Case where there is another node before position */			\
		if (tmp->prev) {												\
			tmp->prev->next = tmp;										\
		}																\
																		\
		position->prev = tmp;											\
		tmp->data = data;												\
																		\
		/* Case where position is the first node in the list */			\
		if (list->head == position) {									\
			list->head = tmp;											\
		}																\
																		\
		if (!list->size_dirty) {										\
			list->size++;												\
		}																\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_append(typename##_list_t *list, type *data) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		typename##_list_node_t *tmp = NULL;								\
																		\
		if (!list || !data) {											\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		switch_zmalloc(tmp, sizeof(*tmp));								\
		if (!tmp) {														\
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Failed to create list node during append in %s!\n", __FUNCTION__); \
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		/* Case where it is the first node in the list */				\
		if (!list->tail) {												\
			list->tail = tmp;											\
			list->head = tmp;											\
		} else {														\
			list->tail->next = tmp;										\
			tmp->prev = list->tail;										\
			list->tail = tmp;											\
		}																\
																		\
		if (!list->size_dirty) {										\
			list->size++;												\
		}																\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(type *) typename##_list_pop(typename##_list_t *list) \
	{																	\
		type *data = NULL;												\
		typename##_list_node_t *node = NULL;							\
																		\
		if (!list || !list->tail) {										\
			goto done;													\
		}																\
																		\
		node = list->tail;												\
		list->tail = node->prev;										\
		list->tail->next = NULL;										\
																		\
		data = node->data;												\
		switch_safe_free(node);											\
																		\
		if (!list->size_dirty) {										\
			list->size--;												\
		}																\
																		\
	done:																\
		return data;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_unshift(typename##_list_t *list, type *data) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		typename##_list_node_t *node = NULL;							\
																		\
		if (!list) {													\
			switch_goto_status(SWITCH_STATUS_FALSE, done);				\
		}																\
																		\
		switch_zmalloc(node, sizeof(*node));							\
		node->next = list->head;										\
		if (list->head) {												\
			list->head->prev = node;									\
		}																\
		list->head = node;												\
		node->data = data;												\
																		\
		if (!list->size_dirty) {										\
			list->size++;												\
		}																\
																		\
	done:																\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(type *) typename##_list_shift(typename##_list_t *list) \
	{																	\
		type *data = NULL;												\
		typename##_list_node_t *node = NULL;							\
																		\
		if (!list || !list->head) {										\
			goto done;													\
		}																\
																		\
		node = list->head;												\
		list->head = node->next;										\
		list->head->prev = NULL;										\
																		\
		data = node->data;												\
		switch_safe_free(node);											\
																		\
		if (!list->size_dirty) {										\
			list->size--;												\
		}																\
																		\
	done:																\
		return data;													\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_t *) typename##_list_slice(typename##_list_t *src_list, typename##_list_node_t *first, typename##_list_node_t *last) \
	{																	\
		typename##_list_t *list = NULL;									\
		typename##_list_node_t *end1 = NULL, *end2 = NULL;				\
																		\
		if (!first && !src_list->head) goto done;						\
		if (!last && !src_list->tail) goto done;						\
																		\
		if (!first)	{													\
			first = src_list->head;										\
		}																\
		if (!last) {													\
			last = src_list->tail;										\
		}																\
																		\
		end1 = (first->prev ? first->prev : NULL);						\
		end2 = (last->next ? last->next : NULL);						\
																		\
		typename##_list_create(&list);									\
																		\
		list->head = first;												\
		list->tail = last;												\
		first->prev = NULL;												\
		last->next = NULL;												\
																		\
		if (first == src_list->head) {									\
			src_list->head = end2;										\
			end2->prev = NULL;											\
		}																\
		if (last == src_list->tail) {									\
			src_list->tail = end1;										\
			end1->next = NULL;											\
		}																\
		if (end1 && end1->next) {										\
			end1->next = end2;											\
		}																\
		if (end2 && end2->prev) {										\
			end2->prev = end1;											\
		}																\
		src_list->size_dirty = SWITCH_TRUE;								\
		list->size_dirty = SWITCH_TRUE;									\
																		\
	done:																\
		return list;													\
	}																	\
																		\
	SWITCH_DECLARE(typename##_list_t *) typename##_list_slicen(typename##_list_t *src_list, typename##_list_node_t *first, const switch_size_t count) \
	{																	\
		typename##_list_t *list = NULL;									\
		return list;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_splice_range(typename##_list_t *list, typename##_list_node_t *replace_start, \
																 typename##_list_node_t *replace_end, typename##_list_t **to_insert) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_splice_range_dup(typename##_list_t *list, typename##_list_node_t *replace_start, \
																	 typename##_list_node_t *replace_end, const typename##_list_t *to_insert) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_splicen(typename##_list_t *list, typename##_list_node_t *replace_start, \
															const switch_size_t count, typename##_list_t **to_insert) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_splicen_dup(typename##_list_t *list, typename##_list_node_t *replace_start, \
																const switch_size_t count, const typename##_list_t *to_insert) \
	{																	\
		switch_status_t status = SWITCH_STATUS_SUCCESS;					\
		return status;													\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_concat(typename##_list_t *list1, typename##_list_t **list2) \
	{																	\
		return typename##_list_splicen(list1, list1->tail, 0, list2);	\
	}																	\
																		\
	SWITCH_DECLARE(switch_status_t) typename##_list_concat_dup(typename##_list_t *list1, const typename##_list_t *list2) \
	{																	\
		return typename##_list_splicen_dup(list1, list1->tail, 0, list2); \
	}																	\
																		\
	SWITCH_DECLARE(switch_size_t) typename##_list_size(typename##_list_t *list) \
	{																	\
		typename##_list_node_t *cur_node = NULL;						\
		if (!list) {													\
			return 0;													\
		}																\
		if (!list->size_dirty) return list->size;						\
																		\
		list->size = 0;													\
		cur_node = list->head;											\
		while (cur_node) {												\
			list->size++;												\
			cur_node = cur_node->next;									\
		}																\
		list->size_dirty = SWITCH_FALSE;								\
		return list->size;												\
	}																	\

///\}

SWITCH_END_EXTERN_C

#endif /* SWITCH_LIST_H */

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */ 
