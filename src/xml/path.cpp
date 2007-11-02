/**************************************************************************\
 *
 *  This file is part of the SIM Scenery library.
 *  Copyright (C) 2000-2007 by Systems in Motion.  All rights reserved.
 *
 *  This software is proprietary to and embodies the confidential 
 *  technology of Systems in Motion.  Possession, use, or copying of this
 *  software and media is authorized only pursuant to a valid written
 *  license from Systems in Motion or an authorized sublicensor. 
 *
 *  For more information, contact SIM <http://www.sim.no/> by means of:
 *  Mail: Systems in Motion AS, Bygd�y all� 5, N-0257 Oslo, Norway;
 *  Email: <sales@sim.no>; Voice: +47 23 27 25 10; Fax: +47 23 27 25 11.
 *
\**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Inventor/C/base/string.h>
#include <Inventor/C/XML/path.h>
#include <Inventor/C/XML/utils.h>
#include <Inventor/C/XML/element.h>

/* ********************************************************************** */

struct cc_xml_path {
  struct path_node * head;
};

/* ********************************************************************** */

struct path_node {
  char * element;
  int idx;
  struct path_node * next;
  // make list doubly-linked for match_p use?
};

static
struct path_node *
path_node_new(const char * element, int idx)
{
  struct path_node * node;
  node = (struct path_node *) malloc(sizeof(struct path_node));
  node->element = cc_xml_strdup(element);
  node->idx = idx;
  node->next = NULL;
  return node;
}

static
struct path_node *
path_node_clone(struct path_node * node)
{
  return path_node_new(node->element, node->idx);
}

static
struct path_node *
path_node_delete(struct path_node * node)
{
  struct path_node * next;
  next = node->next;
  free(node->element);
  free(node);
  return next;
}

static
void
path_node_delete_chain(struct path_node * head)
{
  while ( head != NULL ) {
    struct path_node * prev = head;
    head = head->next;
    free(prev->element);
    free(prev);
  }
}

static
int
path_node_match_p(struct path_node * node, const cc_xml_elt * elt)
{
  if ( strcmp(node->element, cc_xml_elt_get_type(elt)) != 0 ) return FALSE;
  if ( node->idx != -1 ) {
    cc_xml_elt * parent = cc_xml_elt_get_parent(elt);
    if ( parent == NULL ) return (node->idx == 0) ? TRUE : FALSE;
    int i = cc_xml_elt_get_child_type_index(parent, elt);
    return (i == node->idx) ? TRUE : FALSE;
  }
  return TRUE;
}

/* ********************************************************************** */

cc_xml_path *
cc_xml_path_new(void)
{
  cc_xml_path * path = (cc_xml_path *) malloc(sizeof(struct cc_xml_path));
  path->head = NULL;
  assert(path);
  return path;
} // cc_xml_path_new()

void
cc_xml_path_delete_x(cc_xml_path * path)
{
  assert(path);
  path_node_delete_chain(path->head);
  free(path);
} // cc_xml_path_delete_x()

void
cc_xml_path_clear_x(cc_xml_path * path)
{
  assert(path);
  path_node_delete_chain(path->head);
  path->head = NULL;
} // cc_xml_path_clear_x()

static
void
cc_xml_path_set_x_va(cc_xml_path * path, va_list args)
{
  path_node_delete_chain(path->head);
  path->head = NULL;
  char * arg;
  while ( (arg = va_arg(args, char *)) != NULL ) {
    char * ptr;
    if ( (ptr = strchr(arg, '[')) == NULL ) {
      cc_xml_path_prepend_x(path, arg, -1);
    } else {
      int idx = atoi(ptr + 1);
      assert(idx >= -1);
      char * string = cc_xml_strndup(arg, static_cast<int>(ptr - arg));
      cc_xml_path_prepend_x(path, string, idx);
      free(string);
    }
  }
  cc_xml_path_reverse_x(path);
} // cc_xml_path_set_x_va()

void
cc_xml_path_copy_x(cc_xml_path * path, cc_xml_path * path2)
{
  assert(path && path2);
  cc_xml_path_clear_x(path);
  path_node * p1node;
  path_node * p1prev = NULL;
  path_node * p2node = path2->head;
  while ( p2node != NULL ) {
    p1node = path_node_clone(p2node);
    if ( p1prev == NULL ) path->head = p1node;
    else p1prev->next = p1node;
    p1prev = p1node;
  }
} // cc_xml_path_copy_x()

void
cc_xml_path_set_x(cc_xml_path * path, ...)
{
  assert(path);
  va_list args;
  va_start(args, path);
  cc_xml_path_set_x_va(path, args);
  va_end(args);
} // cc_xml_path_set_x()

void
cc_xml_path_reverse_x(cc_xml_path * path)
{
  assert(path);
  struct path_node * prev = NULL;
  struct path_node * node = path->head;
  while ( node != NULL ) {
    struct path_node * next = node->next;
    node->next = prev;
    prev = node;
    node = next;
  }
  path->head = prev;
} // cc_xml_path_reverse_x()

int
cc_xml_path_get_length(const cc_xml_path * path)
{
  assert(path);
  int len = 0;
  struct path_node * node = path->head;
  while ( node != NULL ) {
    assert(len < 100);
    len++;
    node = node->next;
  }
  return len;
} // cc_xml_path_get_length()

const char *
cc_xml_path_get_type(const cc_xml_path * path, int idx)
{
  assert(path && path->head && idx >= 0);
  struct path_node * node = path->head;
  int i;
  for ( i = 0; i < idx; i++ ) {
    assert(node->next != NULL);
    node = node->next;
  }
  return node->element;
} // cc_xml_path_get_type()

int
cc_xml_path_get_index(const cc_xml_path * path, int idx)
{
  assert(path && path->head && idx >= 0);
  struct path_node * node = path->head;
  int i;
  for ( i = 0; i < idx; i++ ) {
    assert(node->next != NULL);
    node = node->next;
  }
  return node->idx;
} // cc_xml_path_get_index()

int
cc_xml_path_match_p(const cc_xml_path * path, const cc_xml_elt * elt)
{
  // consider using a doubly-linked list instead of continous head->tail traversal
  int length = cc_xml_path_get_length(path);
  struct path_node * head = path->head;
  int i, j;
  for ( i = length - 1; i >= 0; i-- ) {
    struct path_node * node = head;
    for ( j = 0; j < i; j++ ) {
      node = node->next;
      assert(node);
    }
    if ( !path_node_match_p(node, elt) ) return FALSE;
    elt = cc_xml_elt_get_parent(elt);
    assert(elt);
  }
  return TRUE;
} // cc_xml_path_match_p()

void
cc_xml_path_append_x(cc_xml_path * path, const char * elt, int idx)
{
  assert(path);
  struct path_node * node = path->head;
  if ( node == NULL ) {
    path->head = path_node_new(elt, idx);
  } else {
    while ( node->next != NULL )
      node = node->next;
    node->next = path_node_new(elt, idx);
  }
} // cc_xml_path_append_x()

void
cc_xml_path_append_path_x(cc_xml_path * path, cc_xml_path * path2)
{
  assert(path && path2);
  struct path_node * p1node = path->head;
  struct path_node * p1prev = NULL;
  if ( p1node != NULL ) {
    while ( p1node->next != NULL ) {
      p1prev = p1node;
      p1node = p1node->next;
    }
  }
  struct path_node * p2node = path2->head;
  while ( p2node != NULL ) {
    p1node = path_node_clone(p2node);
    if ( p1prev == NULL ) path->head = p1node;
    else p1prev->next = p1node;
    p1prev = p1node;
  }
} // cc_xml_path_append_path_x()

void
cc_xml_path_prepend_x(cc_xml_path * path, const char * elt, int idx)
{
  assert(path);
  struct path_node * node = path->head;
  path->head = path_node_new(elt, idx);
  path->head->next = node;
} // cc_xml_path_prepend_x()

void
cc_xml_path_prepend_path_x(cc_xml_path * path, cc_xml_path * path2)
{
  assert(path && path2);
  struct path_node * p1node = NULL;
  struct path_node * p1head = NULL;
  struct path_node * p1tail = NULL;
  struct path_node * p2node = path2->head;
  while ( p2node != NULL ) {
    p1tail = path_node_clone(p2node);
    if ( p1head == NULL ) p1head = p1tail;
    else p1node->next = p1tail;
    p1node = p1tail;
  }
  if ( p1tail != NULL ) {
    p1tail->next = path->head;
    path->head = p1head;
  }
} // cc_xml_path_prepend_path_x()

void
cc_xml_path_truncate_x(cc_xml_path * path, int length)
{
  assert(path);
  int len = 0;
  struct path_node * node = path->head;
  while ( (node != NULL) && (len < length) ) {
    len++;
    node = node->next;
  }
  if ( node ) {
    path_node_delete_chain(node->next);
    node->next = NULL;
  }
} // cc_xml_path_truncate_x()

/* ********************************************************************** */

void
cc_xml_path_dump(cc_xml_path * path)
{
  assert(path);
  struct path_node * node = path->head;
  while ( node != NULL ) {
    if ( node != path->head )
      fprintf(stderr, ".");
    fprintf(stderr, "%s", node->element);
    if ( node->idx != -1 ) {
      fprintf(stderr, "[%d]", node->idx);
    }
    node = node->next;
  }
  fprintf(stderr, "\n");
}

/* ********************************************************************** */
