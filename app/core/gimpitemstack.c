/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpitemstack.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>

#include "core-types.h"

#include "gimpitem.h"
#include "gimpitemstack.h"


/*  local function prototypes  */

static GObject * gimp_item_stack_constructor (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);

static void      gimp_item_stack_add         (GimpContainer         *container,
                                              GimpObject            *object);
static void      gimp_item_stack_remove      (GimpContainer         *container,
                                              GimpObject            *object);


G_DEFINE_TYPE (GimpItemStack, gimp_item_stack, GIMP_TYPE_LIST)

#define parent_class gimp_item_stack_parent_class


static void
gimp_item_stack_class_init (GimpItemStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerClass *container_class = GIMP_CONTAINER_CLASS (klass);

  object_class->constructor = gimp_item_stack_constructor;

  container_class->add      = gimp_item_stack_add;
  container_class->remove   = gimp_item_stack_remove;
}

static void
gimp_item_stack_init (GimpItemStack *stack)
{
}

static GObject *
gimp_item_stack_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpContainer *container;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  container = GIMP_CONTAINER (object);

  g_assert (g_type_is_a (gimp_container_get_children_type (container),
                         GIMP_TYPE_ITEM));

  return object;
}

static void
gimp_item_stack_add (GimpContainer *container,
                     GimpObject    *object)
{
  g_object_ref_sink (object);

  GIMP_CONTAINER_CLASS (parent_class)->add (container, object);

  g_object_unref (object);
}

static void
gimp_item_stack_remove (GimpContainer *container,
                        GimpObject    *object)
{
  GIMP_CONTAINER_CLASS (parent_class)->remove (container, object);
}


/*  public functions  */

GimpContainer *
gimp_item_stack_new (GType item_type)
{
  g_return_val_if_fail (g_type_is_a (item_type, GIMP_TYPE_ITEM), NULL);

  return g_object_new (GIMP_TYPE_ITEM_STACK,
                       "name",          g_type_name (item_type),
                       "children-type", item_type,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       "unique-names",  TRUE,
                       NULL);
}

gint
gimp_item_stack_get_n_items (GimpItemStack *stack)
{
  GList *list;
  gint   n_items = 0;

  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), 0);

  for (list = GIMP_LIST (stack)->list; list; list = g_list_next (list))
    {
      GimpItem      *item = list->data;
      GimpContainer *children;

      n_items++;

      children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

      if (children)
        n_items += gimp_item_stack_get_n_items (GIMP_ITEM_STACK (children));
    }

  return n_items;
}

gboolean
gimp_item_stack_is_flat (GimpItemStack *stack)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), TRUE);

  for (list = GIMP_LIST (stack)->list; list; list = g_list_next (list))
    {
      GimpViewable *viewable = list->data;

      if (gimp_viewable_get_children (viewable))
        return FALSE;
    }

  return TRUE;
}

GList *
gimp_item_stack_get_item_iter (GimpItemStack *stack)
{
  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), NULL);

  return GIMP_LIST (stack)->list;
}

GList *
gimp_item_stack_get_item_list (GimpItemStack *stack)
{
  GList *list;
  GList *result = NULL;

  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), NULL);

  for (list = GIMP_LIST (stack)->list;
       list;
       list = g_list_next (list))
    {
      GimpViewable  *viewable = list->data;
      GimpContainer *children;

      result = g_list_prepend (result, viewable);

      children = gimp_viewable_get_children (viewable);

      if (children)
        {
          GList *child_list;

          child_list = gimp_item_stack_get_item_list (GIMP_ITEM_STACK (children));

          while (child_list)
            {
              result = g_list_prepend (result, child_list->data);

              child_list = g_list_remove (child_list, child_list->data);
            }
        }
    }

  return g_list_reverse (result);
}

GimpItem *
gimp_item_stack_get_item_by_tattoo (GimpItemStack *stack,
                                    GimpTattoo     tattoo)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), NULL);

  for (list = GIMP_LIST (stack)->list; list; list = g_list_next (list))
    {
      GimpItem      *item = list->data;
      GimpContainer *children;

      if (gimp_item_get_tattoo (item) == tattoo)
        return item;

      children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

      if (children)
        {
          item = gimp_item_stack_get_item_by_tattoo (GIMP_ITEM_STACK (children),
                                                     tattoo);

          if (item)
            return item;
        }
    }

  return NULL;
}

GimpItem *
gimp_item_stack_get_item_by_name (GimpItemStack *stack,
                                  const gchar   *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), NULL);

  for (list = GIMP_LIST (stack)->list; list; list = g_list_next (list))
    {
      GimpItem      *item = list->data;
      GimpContainer *children;

      if (! strcmp (gimp_object_get_name (item), name))
        return item;

      children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

      if (children)
        {
          item = gimp_item_stack_get_item_by_name (GIMP_ITEM_STACK (children),
                                                   name);

          if (item)
            return item;
        }
    }

  return NULL;
}

GimpItem *
gimp_item_stack_get_parent_by_path (GimpItemStack *stack,
                                    GList         *path,
                                    gint          *index)
{
  GimpItem *parent = NULL;
  guint32   i;

  g_return_val_if_fail (GIMP_IS_ITEM_STACK (stack), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  i = GPOINTER_TO_UINT (path->data);

  if (index)
    *index = i;

  while (path->next)
    {
      GimpObject    *child;
      GimpContainer *children;

      child = gimp_container_get_child_by_index (GIMP_CONTAINER (stack), i);

      g_return_val_if_fail (GIMP_IS_ITEM (child), parent);

      children = gimp_viewable_get_children (GIMP_VIEWABLE (child));

      g_return_val_if_fail (GIMP_IS_ITEM_STACK (children), parent);

      parent = GIMP_ITEM (child);
      stack  = GIMP_ITEM_STACK (children);

      path = path->next;

      i = GPOINTER_TO_UINT (path->data);

      if (index)
        *index = i;
    }

  return parent;
}

static void
gimp_item_stack_invalidate_preview (GimpViewable *viewable)
{
  GimpContainer *children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_item_stack_invalidate_previews (GIMP_ITEM_STACK (children));

  gimp_viewable_invalidate_preview (viewable);
}

void
gimp_item_stack_invalidate_previews (GimpItemStack *stack)
{
  g_return_if_fail (GIMP_IS_ITEM_STACK (stack));

  gimp_container_foreach (GIMP_CONTAINER (stack),
                          (GFunc) gimp_item_stack_invalidate_preview,
                          NULL);
}