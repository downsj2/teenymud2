/* attributes.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995 by Jason Downs.
 *                           All rights reserved.
 * 
 * TeenyMUD II is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * TeenyMUD II is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file 'COPYING'); if not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif			/* HAVE_STRING_H */
#include <ctype.h>

#include "conf.h"
#include "teeny.h"
#include "teenydb.h"
#include "externs.h"

/* user def'd attributes */

/* hash routine for attribute names */
int attr_hash(str)
    char *str;
{
  register unsigned int i = 0;

  if((str == (char *)NULL) || (str[0] == '\0'))
    return(0);

  while(str[0] != '\0') {
    i = (i * 10) + (to_lower(str[0]) - '0');
    str++;
  }
  return((i*1103515245 + 12345) % ATTR_WIDTH);
}

int attr_total(obj)
    int obj;
{
  struct obj_data *dat;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *) NULL)
    return (-1);

  return (dat->attr_total);
}

int attr_set(obj, name, flags)
    int obj;
    char *name;
    int flags;
{
  struct attr *attrs;
  struct obj_data *dat;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *) NULL)
    return (-1);

  for (attrs = dat->attributes[attr_hash(name)]; attrs != (struct attr *)NULL;
       attrs = attrs->next) {
    if (!strcasecmp(attrs->name, name)) {
      attrs->flags = flags;
      DSC_FLAG2(main_index[dat->objnum]) |= DIRTY;
      return (0);
    }
  }
  return (-2);
}

int attr_addlock(obj, name, data, flags)
    int obj;
    char *name;
    struct boolexp *data;
    int flags;
{
  register struct attr *curr, *new;
  struct obj_data *dat;
  int hashval;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *) NULL)
    return (-1);

  hashval = attr_hash(name);
  for (curr = dat->attributes[hashval]; curr != (struct attr *)NULL;
       curr = curr->next) {
    if (!strcasecmp(curr->name, name)) {
      if(curr->type != ATTR_LOCK) {
        ty_free((VOID *)(curr->dat).str);
	curr->type = ATTR_LOCK;
      } else
        boolexp_free((curr->dat).lock);
      (curr->dat).lock = data;
      strcpy(curr->name, name);
      if(islower(name[0]))
        (curr->name)[0] = to_upper((curr->name)[0]);
      curr->flags = flags;
      DSC_FLAG2(main_index[dat->objnum]) |= DIRTY;
      return (0);
    }
  }

  new = (struct attr *)ty_malloc(sizeof(struct attr), "attr_add.new");
  new->name = (char *) ty_strdup(name, "attr_add.name");
  if(islower(name[0]))
    (new->name)[0] = to_upper((new->name)[0]);
  (new->dat).lock = data;
  new->flags = flags;
  new->type = ATTR_LOCK;

  /* link it in, new style */
  new->next = dat->attributes[hashval];
  if(dat->attributes[hashval] == (struct attr *) NULL)
    new->prev = new;
  else {
    new->prev = (dat->attributes[hashval])->prev;
    (dat->attributes[hashval])->prev = new;
  }
  dat->attributes[hashval] = new;
  dat->attr_total++;

  DSC_FLAG2(main_index[dat->objnum]) |= DIRTY;
  DSC_SIZE(main_index[dat->objnum]) += strlen(name) + 1;
  mudstat.cache_usage += strlen(name) + 1;
  return (0);
}

int attr_add(obj, name, data, flags)
    int obj;
    char *name, *data;
    int flags;
{
  register struct attr *curr, *new;
  struct obj_data *dat;
  int oldsize, hashval;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *) NULL)
    return (-1);

  hashval = attr_hash(name);
  for (curr = dat->attributes[hashval]; curr != (struct attr *)NULL;
       curr = curr->next) {
    if (!strcasecmp(curr->name, name)) {
      if(curr->type != ATTR_STRING) {
        oldsize = 0;
	boolexp_free((curr->dat).lock);
	curr->type = ATTR_STRING;
      } else {
        oldsize = strlen((curr->dat).str) + 1;
        ty_free((VOID *) (curr->dat).str);
      }
      (curr->dat).str = (char *) ty_strdup(data, "attr_add.data");
      strcpy(curr->name, name);
      if(islower(name[0]))
        (curr->name)[0] = to_upper((curr->name)[0]);
      curr->flags = flags;
      DSC_FLAG2(main_index[dat->objnum]) |= DIRTY;
      DSC_SIZE(main_index[dat->objnum]) =
	  (DSC_SIZE(main_index[dat->objnum]) - oldsize) + strlen(data) + 1;
      mudstat.cache_usage = (mudstat.cache_usage - oldsize) + strlen(data) + 1;
      return (0);
    }
  }
  new = (struct attr *)ty_malloc(sizeof(struct attr), "attr_add.new");
  new->name = (char *) ty_strdup(name, "attr_add.name");
  if(islower(name[0]))
    (new->name)[0] = to_upper((new->name)[0]);
  (new->dat).str = (char *) ty_strdup(data, "attr_add.data");
  new->flags = flags;
  new->type = ATTR_STRING;

  /* link it in, new style */
  new->next = dat->attributes[hashval];
  if(dat->attributes[hashval] == (struct attr *) NULL)
    new->prev = new;
  else {
    new->prev = (dat->attributes[hashval])->prev;
    (dat->attributes[hashval])->prev = new;
  }
  dat->attributes[hashval] = new;
  dat->attr_total++;

  DSC_FLAG2(main_index[dat->objnum]) |= DIRTY;
  DSC_SIZE(main_index[dat->objnum]) += strlen(name) + strlen(data) + 2;
  mudstat.cache_usage += strlen(name) + strlen(data) + 2;
  return (0);
}

int attr_delete(obj, name)
    int obj;
    char *name;
{
  register struct attr *curr;
  struct obj_data *dat;
  int oldsize, hashval;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *) NULL)
    return (-1);

  hashval = attr_hash(name);
  for (curr = dat->attributes[hashval]; curr != (struct attr *)NULL;
       curr = curr->next) {
    if (!strcasecmp(curr->name, name)) {
      oldsize = strlen(curr->name) + 1;
      if (curr == dat->attributes[hashval]) {
	dat->attributes[hashval] = curr->next;
	if (dat->attributes[hashval] != (struct attr *)NULL)
	  (dat->attributes[hashval])->prev = dat->attributes[hashval];
      } else {
	(curr->prev)->next = curr->next;
      }
      if (curr->next != (struct attr *)NULL) {
	(curr->next)->prev = curr->prev;
      } else if (dat->attributes[hashval] != (struct attr *)NULL) {
        /* last thing in the list */
	(dat->attributes[hashval])->prev = curr->prev;
      }
      ty_free((VOID *) curr->name);
      switch(curr->type) {
      case ATTR_STRING:
        oldsize += strlen((curr->dat).str) + 1;
        ty_free((VOID *) (curr->dat).str);
	break;
      case ATTR_LOCK:
        boolexp_free((curr->dat).lock);
	break;
      }
      dat->attr_total--;
      ty_free((VOID *)curr);
      DSC_FLAG2(main_index[dat->objnum]) |= DIRTY;
      DSC_SIZE(main_index[dat->objnum]) -= oldsize;
      mudstat.cache_usage -= oldsize;
      break;
    }
  }
  return (0);
}

int attr_source(obj, name)
    int obj;
    char *name;
{
  register struct attr *attrs;
  struct obj_data *dat;
  int parent, depth, hashval;

  depth = 0;
  parent = obj;
  hashval = attr_hash(name);
  do {
    if (!_exists_object(parent))
      return (-1);
    if ((dat = lookup_obj(parent)) == (struct obj_data *) NULL)
      return (-1);
    for (attrs = dat->attributes[hashval]; attrs != (struct attr *)NULL;
         attrs = attrs->next) {
      if ((attrs->flags & A_PRIVATE) && (parent != obj))
	continue;
      if ((attrs->flags & A_PICKY)
          && (DSC_TYPE(main_index[parent]) != DSC_TYPE(main_index[obj])))
	continue;
      if ((DSC_FLAG2(main_index[parent]) & PICKY)
          && (DSC_TYPE(main_index[parent]) != DSC_TYPE(main_index[obj])))
	continue;
      if (!strcasecmp(attrs->name, name))
	return (parent);
    }
    parent = DSC_PARENT(main_index[dat->objnum]);
    depth++;
  } while ((parent != -1) && (depth <= mudconf.parent_depth));
  return(-1);
}

int attr_getlock_parent(obj, name, ret, flags, source)
    int obj;
    char *name;
    struct boolexp **ret;
    int *flags, *source;
{
  register struct attr *attrs;
  struct obj_data *dat;
  int parent, depth, hashval;

  depth = 0;
  parent = obj;
  hashval = attr_hash(name);
  do {
    if (!_exists_object(parent))
      return (-1);
   if ((dat = lookup_obj(parent)) == (struct obj_data *)NULL)
     return (-1);
   for (attrs = dat->attributes[hashval]; attrs != (struct attr *)NULL;
   	attrs = attrs->next) {
      if ((attrs->flags & A_PRIVATE) && (parent != obj))
        continue;
      if ((attrs->flags & A_PICKY)
          && (DSC_TYPE(main_index[parent]) != DSC_TYPE(main_index[obj])))
        continue;
      if ((DSC_FLAG2(main_index[parent]) & PICKY)
          && (DSC_TYPE(main_index[parent]) != DSC_TYPE(main_index[obj])))
        continue;
      if (!strcasecmp(attrs->name, name)) {
        if(attrs->type != ATTR_LOCK) {
	  *ret = (struct boolexp *)NULL;
	  *flags = -1;
	  *source = -1;
	} else {
	  *ret = (attrs->dat).lock;
	  *flags = attrs->flags;
	  *source = parent;
	}
	return (0);
      }
    }
    parent = DSC_PARENT(main_index[dat->objnum]);
    depth++;
  } while ((parent != -1) && (depth <= mudconf.parent_depth));

  *ret = (struct boolexp *) NULL;
  *flags = -1;
  *source = -1;
  return (0);
}

int attr_get_parent(obj, name, ret, flags, source)
    int obj;
    char *name, **ret;
    int *flags, *source;
{
  register struct attr *attrs;
  struct obj_data *dat;
  int parent, depth, hashval;

  depth = 0;
  parent = obj;
  hashval = attr_hash(name);
  do {
    if (!_exists_object(parent))
      return (-1);
    if ((dat = lookup_obj(parent)) == (struct obj_data *) NULL)
      return (-1);
    for (attrs = dat->attributes[hashval]; attrs != (struct attr *)NULL;
         attrs = attrs->next) {
      if ((attrs->flags & A_PRIVATE) && (parent != obj))
	continue;
      if ((attrs->flags & A_PICKY)
          && (DSC_TYPE(main_index[parent]) != DSC_TYPE(main_index[obj])))
        continue;
      if ((DSC_FLAG2(main_index[parent]) & PICKY)
          && (DSC_TYPE(main_index[parent]) != DSC_TYPE(main_index[obj])))
        continue;
      if (!strcasecmp(attrs->name, name)) {
        if(attrs->type != ATTR_STRING) {
	  *ret = (char *)NULL;
	  *flags = -1;
	  *source = -1;
	} else {
	  *ret = (attrs->dat).str;
	  *flags = attrs->flags;
	  *source = parent;
	}
	return (0);
      }
    }
    parent = DSC_PARENT(main_index[dat->objnum]);
    depth++;
  } while ((parent != -1) && (depth <= mudconf.parent_depth));

  *ret = (char *) NULL;
  *flags = -1;
  *source = -1;
  return (0);
}

int attr_getlock(obj, name, ret, flags)
    int obj;
    char *name;
    struct boolexp **ret;
    int *flags;
{
  register struct attr *attrs;
  struct obj_data *dat;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *)NULL)
    return (-1);

  for (attrs = dat->attributes[attr_hash(name)]; attrs != (struct attr *)NULL;
      attrs = attrs->next) {
    if(!strcasecmp(attrs->name, name)) {
      if(attrs->type != ATTR_LOCK) {
        *ret = (struct boolexp *)NULL;
	*flags = -1;
      } else {
        *ret = (attrs->dat).lock;
        *flags = attrs->flags;
      }
      return (0);
    }
  }

  *ret = (struct boolexp *)NULL;
  *flags = -1;
  return(0);
}

int attr_get(obj, name, ret, flags)
    int obj;
    char *name, **ret;
    int *flags;
{
  register struct attr *attrs;
  struct obj_data *dat;

  if (!_exists_object(obj))
    return (-1);
  if ((dat = lookup_obj(obj)) == (struct obj_data *) NULL)
    return (-1);

  for (attrs = dat->attributes[attr_hash(name)]; attrs != (struct attr *)NULL;
       attrs = attrs->next) {
    if (!strcasecmp(attrs->name, name)) {
      if(attrs->type != ATTR_STRING) {
        *ret = (char *)NULL;
	*flags = -1;
      } else {
        *ret = (attrs->dat).str;
        *flags = attrs->flags;
      }
      return (0);
    }
  }

  *ret = (char *) NULL;
  *flags = -1;
  return (0);
}

/* attribute search routines */
struct attr *attr_first(obj, search)
    int obj;
    struct asearch *search;
{
  struct obj_data *dat;
  register int idx;

  if(!_exists_object(obj))
    return((struct attr *)NULL);
  if((dat = lookup_obj(obj)) == (struct obj_data *)NULL)
    return((struct attr *)NULL);

  /* loop through the table, looking for the first attribute */
  for(idx = 0; idx < ATTR_WIDTH; idx++) {
    if(dat->attributes[idx] != (struct attr *)NULL) {
      search->elem = idx;
      search->curr = dat->attributes[idx];
      return(dat->attributes[idx]);
    }
  }
  return((struct attr *)NULL);
}

struct attr *attr_next(obj, search)
    int obj;
    struct asearch *search;
{
  struct obj_data *dat;
  
  if(!_exists_object(obj))
    return((struct attr *)NULL);
  if((dat = lookup_obj(obj)) == (struct obj_data *)NULL)
    return((struct attr *)NULL);

  /* find the next attribute */
  if(search->curr != (struct attr *)NULL) {
    if((search->curr)->next != (struct attr *)NULL) {
      /* elem is still correct */
      search->curr = (search->curr)->next;
      return(search->curr);
    }
    /* the current bucket is empty. find the next one. */
    while(search->elem < (ATTR_WIDTH-1)) {
      (search->elem)++;
      if(dat->attributes[search->elem] != (struct attr *)NULL) {
        search->curr = dat->attributes[search->elem];
	return(search->curr);
      }
    }
  }
  return((struct attr *)NULL);
}

/* attributes and list allocation */
void free_attributes(attrs)
    struct attr *attrs[];
{
  register struct attr *curr, *next;
  register int idx;

  for(idx = 0; idx < ATTR_WIDTH; idx++) {
    for(curr = attrs[idx]; curr != (struct attr *)NULL; curr = next) {
      next = curr->next;
      ty_free((VOID *)curr->name);
      switch(curr->type) {
      case ATTR_STRING:
        ty_free((VOID *)(curr->dat).str);
	break;
      case ATTR_LOCK:
        boolexp_free((curr->dat).lock);
	break;
      }
      ty_free((VOID *)curr);
    }
  }
}
