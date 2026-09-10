

#include <stdlib.h>
#include <string.h>
#include "buffer_ref.h"
#include "os_atom.h"


bufferRefHndl_S *bufferRef_create( void *data, int size,\
							bufferRefFreeInterface freeInterface,\
                              void *user)
{
	bufferRefHndl_S *ref = NULL;
	bufferInner_S   *buf = NULL;

	buf = malloc(sizeof(*buf));
	if (!buf)
	{
		return NULL;
	}
	memset(buf,0,sizeof(*buf));

	buf->data     = data;
	buf->size     = size;
	buf->freeInterface = freeInterface;
	buf->user     = user;
	buf->refcount = 1;

	ref = malloc(sizeof(*ref));
	if (!ref)
	{
		free(buf);buf = NULL;
		return NULL;
	}
	memset(ref,0,sizeof(*ref));

	ref->buffer = buf;
	return ref;
}

bufferRefHndl_S *bufferRef_ref(bufferRefHndl_S *buf)
{
	if(buf == NULL)
	{
		printf("this argument is null!!!\n");
		return NULL;
	}

	bufferRefHndl_S *ref = malloc(sizeof(*ref));
	if (!ref)
	{
		return NULL;
	}
	memset(ref,0,sizeof(*ref));
	*ref = *buf;
	os_atomic_add_fetch(&buf->buffer->refcount, 1);
	return ref;
}

static void bufferRef_replace(bufferRefHndl_S **dst, bufferRefHndl_S **src)
{
	bufferInner_S *b = NULL;

    b = (*dst)->buffer;
    if (src)
    {
        **dst = **src;
        free(*src);*src = NULL;
    } else
    {
        free(*dst);*dst = NULL;
    }

    if (os_atomic_fetch_sub(&b->refcount, 1) == 1)
    {
    	if(b->freeInterface)
    	{
    		b->freeInterface(b->user,b->data);
    	}
        free(b);b = NULL;
    }
}


void bufferRef_unref(bufferRefHndl_S **buf)
{
    if (!buf || !*buf)
        return;

    bufferRef_replace(buf, NULL);
}










