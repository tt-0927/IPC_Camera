


#ifndef MINHEAP_INTERNAL_H_INCLUDED_
#define MINHEAP_INTERNAL_H_INCLUDED_
 

#include "minheap_event.h"
 
typedef struct min_heap
{
    struct event** p;
    unsigned int n, a;
}min_heap_t;

static inline void	     min_heap_ctor_(min_heap_t* s);//初始化堆，调用前需要malloc
static inline void	     min_heap_dtor_(min_heap_t* s);//删除堆，free
static inline void	     min_heap_elem_init_(struct event* e);//初始化超时事件，min_heap_idx = -1
static inline int	     min_heap_elt_is_top_(const struct event *e);//超时事件是否为顶
static inline int	     min_heap_empty_(min_heap_t* s);//堆是否为空
static inline unsigned	     min_heap_size_(min_heap_t* s);//获取当前事件个数，min_heap_t中的n
static inline struct event*  min_heap_top_(min_heap_t* s);//获取堆顶事件
static inline int	     min_heap_reserve_(min_heap_t* s, unsigned n);//调整分配内存
static inline int	     min_heap_push_(min_heap_t* s, struct event* e);//压入新的事件
static inline struct event*  min_heap_pop_(min_heap_t* s);//弹出堆顶事件
static inline int	     min_heap_adjust_(min_heap_t *s, struct event* e);//调整e在最小堆的位置
static inline int	     min_heap_erase_(min_heap_t* s, struct event* e);//在最小堆中销毁e，并用堆的最后一个堆元素进行替换和调整
static inline void	     min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e);//将某个叶子结点的堆元素上浮到合适的位置，用于堆元素的插入
static inline void	     min_heap_shift_up_unconditional_(min_heap_t* s, unsigned hole_index, struct event* e);//和min_heap_shift_up_类似，只是一次无条件上浮
static inline void	     min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e);//将某个叶子结点的堆元素下浮到合适的位置，用于堆元素的取出

#define min_heap_elem_greater(a, b) \
    (evutil_timercmp(&(a)->ev_timeout, &(b)->ev_timeout, >))
 
void min_heap_ctor_(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor_(min_heap_t* s) { if (s->p) mm_free(s->p); }
void min_heap_elem_init_(struct event* e) { e->ev_timeout_pos.min_heap_idx = -1; }
int min_heap_empty_(min_heap_t* s) { return 0u == s->n; }
unsigned min_heap_size_(min_heap_t* s) { return s->n; }
struct event* min_heap_top_(min_heap_t* s) { return s->n ? *s->p : 0; }

int min_heap_push_(min_heap_t* s, struct event* e)
{
    if (min_heap_reserve_(s, s->n + 1))
        return -1;
    min_heap_shift_up_(s, s->n++, e);
    return 0;
}
 
struct event* min_heap_pop_(min_heap_t* s)
{
    if (s->n)
    {
        struct event* e = *s->p;
        min_heap_shift_down_(s, 0u, s->p[--s->n]);
        e->ev_timeout_pos.min_heap_idx = -1;
        return e;
    }
    return 0;
}
 
int min_heap_elt_is_top_(const struct event *e)
{
    return e->ev_timeout_pos.min_heap_idx == 0;
}
 
int min_heap_erase_(min_heap_t* s, struct event* e)
{
    if (-1 != e->ev_timeout_pos.min_heap_idx)
    {
        struct event *last = s->p[--s->n];
        unsigned parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;
        /* we replace e with the last element in the heap.  We might need to
           shift it upward if it is less than its parent, or downward if it is
           greater than one or both its children. Since the children are known
           to be less than the parent, it can't need to shift both up and
           down. */
        if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
            min_heap_shift_up_unconditional_(s, e->ev_timeout_pos.min_heap_idx, last);
        else
            min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, last);
        e->ev_timeout_pos.min_heap_idx = -1;
        return 0;
    }
    return -1;
}
 
int min_heap_adjust_(min_heap_t *s, struct event *e)
{
    if (-1 == e->ev_timeout_pos.min_heap_idx) {
        return min_heap_push_(s, e);
    } else {
        unsigned parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;
        /* The position of e has changed; we shift it up or down
         * as needed.  We can't need to do both. */
        if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], e))
            min_heap_shift_up_unconditional_(s, e->ev_timeout_pos.min_heap_idx, e);
        else
            min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, e);
        return 0;
    }
}
 
int min_heap_reserve_(min_heap_t* s, unsigned n)
{
    if (s->a < n)
    {
        struct event** p;
        unsigned a = s->a ? s->a * 2 : 8;
        if (a < n)
            a = n;
        if (!(p = (struct event**)mm_realloc(s->p, a * sizeof *p)))
            return -1;
        s->p = p;
        s->a = a;
    }
    return 0;
}
 
void min_heap_shift_up_unconditional_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned parent = (hole_index - 1) / 2;
    do
    {
        (s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    } while (hole_index && min_heap_elem_greater(s->p[parent], e));
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}
 
void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned parent = (hole_index - 1) / 2;
    while (hole_index && min_heap_elem_greater(s->p[parent], e))
    {
        (s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}
 
void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned min_child = 2 * (hole_index + 1);
    while (min_child <= s->n)
    {
        min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
        if (!(min_heap_elem_greater(e, s->p[min_child])))
            break;
        (s->p[hole_index] = s->p[min_child])->ev_timeout_pos.min_heap_idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}
 
#endif /* MINHEAP_INTERNAL_H_INCLUDED_ */

