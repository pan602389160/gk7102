#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "linklist_interface.h"

void list_init(List *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->len = 0;
}

bool is_empty(List *list)
{
	return (list->head == NULL);
}

static struct node *make_node(void *data)    //把用户传递过来的数据打包为一个链表节点
{
	struct node *n;

	n = malloc(sizeof(struct node));
	assert(n != NULL);

	n->next = NULL;
	n->data = data;

	return n;
}

static void delete_node(List *list, struct node *key)
{
	struct node *n;

	n = list->head;

	if(n == key){
		list->head = n->next;
		list->len--;
		return;
	}

	while(n->next){
		if(n->next == key){
			if(key == list->tail)
				list->tail = n;
			n->next = n->next->next;
			list->len--;
			return;
		}
		n = n->next;
	}
}

static void insert_node(List *list, struct node *key)
{
	if(list->head == NULL){
		list->head = key;
		list->tail = key;
	}else{
		list->tail->next = key;
		list->tail = key;
	}

	list->len++;
}

static struct node *get_node(List *list, int idx)
{
	int i = 0;
	struct node *n;
	n = list->head;

	while(n && i < idx){
		i++;
		n = n->next;
	}

	if(n)
		return n;

	return NULL;
}

void list_insert_at_head(List *list, void *data)    //头插法
{
	struct node *n;
	n = make_node(data);

	if(list->head == NULL){ //如果是空链表
		list->head = n;
		list->tail = n;
	}
	else{                   //如果不是非空链表
		n->next = list->head;
		list->head = n;
	}
	list->len++;
}

void list_insert_at_index(List *list, void *data, long index)
{
	long i = 1; //从1开始算
	struct node *p, *n;

	p = list->head;

	while(p && i < index){
		p = p->next;
		i++;
	}

	if(p){ //如果链表遍历完了，计数i还没到index，说明第index个节点不存在。
		n = make_node(data);
		n->next = p->next;
		p->next = n;
		list->len++;
	}
}

void list_insert_at_tail(List *list, void *data)    //尾插法
{
	struct node *n;
	n = make_node(data);

	if(is_empty(list)){    //如果是空链表
		list->head = n;
		list->tail = n;
	}
	else{                      //如果不是非空链表
		list->tail->next = n;
		list->tail = n;
	}
	list->len++;
}

void list_insert(List *list, void *data)    //默认采用尾插法
{
#if 1
	list_insert_at_tail(list, data);
#else
	struct node *n;

	n = make_node(data);

	if(list->head == NULL){
		list->head = n;
		list->tail = n;
	} else {
		list->tail->next = n;
		list->tail = n;
	}
	list->len++;
#endif
}

void * list_delete(List *list, void *key, int (*compare)(const void *, const void *))
{
	void *data;
	struct node *n, *t;

	if (list_get_length(list) == 0)
		return NULL;

	n = list->head;

	if(!compare(n->data, key)){    //如果要删除的节点为首节点
		t = n;
		data = n->data;
		list->head = n->next;
		free(t);
		list->len--;
		return data;
	}

	while(n->next != NULL){        //遍历查找符合条件的节点，删除之
		if(compare(n->next->data, key) == 0){    //只删除第一个符合条件的节点。
			t = n->next;
			if(n->next == list->tail)
				list->tail = n;
			n->next = n->next->next;
			data = t->data;
			free(t);
			list->len--;
			return data;    //把删除的数据返回给用户，供用户后续的处理使用。
		}
		n = n->next;
	}
	return NULL;    //没找到匹配的节点，返回NULL
}

void *list_delete_at_index(List *list, long idx)
{
	long i = 1; //从1开始算
	void *data = NULL;
	void *ret = NULL;
	struct node *n = NULL;

	n = get_node(list, idx);

	if (n) {
		data = n->data;
		delete_node(list, n);
		free(n);
	}

	return data;
}

void *list_search(List *list, void *key, int (*compare)(const void *, const void *))
{
	struct node *n;
	n = list->head;

	while(n){
		if(!compare(n->data, key))    //找到了，返回找到的数据
			return n->data;
		n = n->next;
	}

	return NULL;    //找不到，返回NULL
}

static struct node *find_min_node(List *list,
				  int (*compare)(const void *, const void *))
{
	struct node *min, *n;

	n = list->head;
	min = list->head;

	while(n){
		if(compare(min->data, n->data) > 0)
			min = n;
		n = n->next;
	}

	return min;
}

void list_sort(List *list,
	       int (*compare)(const void *, const void *))
{
	List tmp;
	struct node *n;

	list_init(&tmp);

	while(! is_empty(list)){
		n = find_min_node(list, compare);
		delete_node(list, n);
		insert_node(&tmp, n);
	}
	list->head = tmp.head;
	list->tail = tmp.tail;
	list->len = tmp.len;
}

void list_traverse(List *list, void (*handle)(void *, void *), void *priv)
{
	struct node *p;

	p = list->head;
	while(p){
		handle(p->data, priv);
		p = p->next;
	}
}

int list_traverse_cond(List *list, int (*handle)(List *list, void *, void *), void *priv)
{
	struct node *p;
	struct node *n;
	int ret;

	p = list->head;
	while(p){
		n = p->next;
		ret = handle(list, p->data, priv);
		if (ret <= 0)
			return ret;

		p = n;
	}

	return 0;
}

void list_reverse(List *list)
{
	struct node *pre = NULL, *next, *p = list->head;

	list->tail = list->head;    //tail指向head；
	while(p){
		next = p->next;
		if(!next)  //当p->next为最后一个节点时，让head指向p->next
			list->head = p;
		//记录当前节点为pre，作为下一个节点的next.第一个节点为NULL，初始化时已定义。
		p->next = pre;
		pre = p;
		p = next;
	}
}

long list_get_length(List *list)
{
	return (list->len);
}

void *list_get_element(List *list, int idx)
{
	printf("======list_get_element======\n");
	int i = 0;
	struct node *n;
	n = list->head;

	while(n && i < idx){
		i++;
		n = n->next;
	}

	if(n)
		return n->data;

	return NULL;
}

void list_destroy(List *list, void (*destroy)(void *))
{
	struct node *n, *t;
	n = list->head;

	while(n){
		t = n->next;    //t只起一个记录n->next的功能，否则后面把n free掉之后，就找不到n->next了。
		if(destroy)  //传递用户自定义的数据处理函数，为0时不执行
			destroy(n->data);    //使用用户提供的destroy函数来释放用户传递过来的数据。
		free(n);
		n = t;  //把n free掉之后，再把t给n，相当于把n->next给n,如此循环遍历链表，挨个删除，
	}

	list->head = NULL;
	list->tail = NULL;
	list->len = 0;
}

