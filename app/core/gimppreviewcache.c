/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Andy Thomas alt@gimp.org
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

#include <gegl.h>

#include "core-types.h"

#include "gimppreviewcache.h"
#include "gimptempbuf.h"


#define MAX_CACHE_PREVIEWS 5


typedef struct
{
  GimpTempBuf *buf;
  gint         width;
  gint         height;
} PreviewNearest;


static gint
preview_cache_compare (gconstpointer  a,
                       gconstpointer  b)
{
  const GimpTempBuf *buf1 = a;
  const GimpTempBuf *buf2 = b;

  if (gimp_temp_buf_get_width  (buf1) > gimp_temp_buf_get_width  (buf2) &&
      gimp_temp_buf_get_height (buf1) > gimp_temp_buf_get_height (buf2))
    return -1;

  return 1;
}

static void
preview_cache_find_exact (gpointer data,
                          gpointer udata)
{
  GimpTempBuf    *buf     = data;
  PreviewNearest *nearest = udata;

  if (nearest->buf)
    return;

  if (gimp_temp_buf_get_width  (buf) == nearest->width &&
      gimp_temp_buf_get_height (buf) == nearest->height)
    {
      nearest->buf = buf;
      return;
    }
}

static void
preview_cache_find_biggest (gpointer data,
                            gpointer udata)
{
  GimpTempBuf    *buf     = data;
  PreviewNearest *nearest = udata;

  if (gimp_temp_buf_get_width  (buf) >= nearest->width &&
      gimp_temp_buf_get_height (buf) >= nearest->height)
    {
      /* Ok we could make the preview out of this one...
       * If we already have it are these bigger dimensions?
       */
      if (nearest->buf)
        {
          if (gimp_temp_buf_get_width  (nearest->buf) > gimp_temp_buf_get_width  (buf) &&
              gimp_temp_buf_get_height (nearest->buf) > gimp_temp_buf_get_height (buf))
            return;
        }

      nearest->buf = buf;
    }
}

static void
preview_cache_remove_smallest (GSList **plist)
{
  GSList      *list;
  GimpTempBuf *smallest = NULL;

#ifdef PREVIEW_CACHE_DEBUG
  g_print ("preview_cache_remove_smallest\n");
#endif

  for (list = *plist; list; list = list->next)
    {
      if (!smallest)
        {
          smallest = list->data;
        }
      else
        {
          GimpTempBuf *this = list->data;

          if (gimp_temp_buf_get_width (smallest) * gimp_temp_buf_get_height (smallest) >
              gimp_temp_buf_get_width (this)     * gimp_temp_buf_get_height (this))
            {
              smallest = this;
            }
        }
    }

  if (smallest)
    {
      *plist = g_slist_remove (*plist, smallest);

#ifdef PREVIEW_CACHE_DEBUG
      g_print ("preview_cache_remove_smallest: removed %d x %d\n",
               smallest->width, smallest->height);
#endif

      gimp_temp_buf_unref (smallest);
    }
}

#ifdef PREVIEW_CACHE_DEBUG
static void
preview_cache_print (GSList *plist)
{
  GSList  *list;

  g_print ("preview cache dump:\n");

  for (list = plist; list; list = list->next)
    {
      GimpTempBuf *buf = list->data;

      g_print ("\tvalue w,h [%d,%d]\n", buf->width, buf->height);
    }
}
#endif  /* PREVIEW_CACHE_DEBUG */

void
gimp_preview_cache_invalidate (GSList **plist)
{
#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_invalidate\n");
  preview_cache_print (*plist);
#endif

  g_slist_free_full (*plist, (GDestroyNotify) gimp_temp_buf_unref);
  *plist = NULL;
}

void
gimp_preview_cache_add (GSList      **plist,
                        GimpTempBuf  *buf)
{
#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_add: %d x %d\n", buf->width, buf->height);
  preview_cache_print (*plist);
#endif

  if (g_slist_length (*plist) >= MAX_CACHE_PREVIEWS)
    {
      preview_cache_remove_smallest (plist);
    }

  *plist = g_slist_insert_sorted (*plist, buf, preview_cache_compare);
}

GimpTempBuf *
gimp_preview_cache_get (GSList **plist,
                        gint     width,
                        gint     height)
{
  PreviewNearest pn;

#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_get: %d x %d\n", width, height);
  preview_cache_print (*plist);
#endif

  pn.buf    = NULL;
  pn.width  = width;
  pn.height = height;

  g_slist_foreach (*plist, preview_cache_find_exact, &pn);

  if (pn.buf)
    {
#ifdef PREVIEW_CACHE_DEBUG
      g_print ("gimp_preview_cache_get: found exact match %d x %d\n",
               pn.buf->width, pn.buf->height);
#endif

      return pn.buf;
    }

  g_slist_foreach (*plist, preview_cache_find_biggest, &pn);

  if (pn.buf)
    {
      GimpTempBuf *preview;
      gint         pwidth;
      gint         pheight;
      gdouble      x_ratio;
      gdouble      y_ratio;
      guchar      *src_data;
      guchar      *dest_data;
      gint         bytes;
      gint         loop1;
      gint         loop2;

#ifdef PREVIEW_CACHE_DEBUG
      g_print ("gimp_preview_cache_get: nearest value: %d x %d\n",
               pn.buf->width, pn.buf->height);
#endif

      /* Make up new preview from the large one... */
      pwidth  = gimp_temp_buf_get_width  (pn.buf);
      pheight = gimp_temp_buf_get_height (pn.buf);

      /* Now get the real one and add to cache */
      preview = gimp_temp_buf_new (width, height,
                                   gimp_temp_buf_get_format (pn.buf));

      /* preview from nearest bigger one */
      if (width)
        x_ratio = (gdouble) pwidth / (gdouble) width;
      else
        x_ratio = 0.0;

      if (height)
        y_ratio = (gdouble) pheight / (gdouble) height;
      else
        y_ratio = 0.0;

      src_data  = gimp_temp_buf_get_data (pn.buf);
      dest_data = gimp_temp_buf_get_data (preview);

      bytes = babl_format_get_bytes_per_pixel (gimp_temp_buf_get_format (preview));

      for (loop1 = 0 ; loop1 < height ; loop1++)
        for (loop2 = 0 ; loop2 < width ; loop2++)
          {
            gint    i;
            guchar *src_pixel;
            guchar *dest_pixel;

            src_pixel = src_data +
              ((gint) (loop2 * x_ratio)) * bytes +
              ((gint) (loop1 * y_ratio)) * pwidth * bytes;

            dest_pixel = dest_data +
              (loop2 + loop1 * width) * bytes;

            for (i = 0; i < bytes; i++)
              *dest_pixel++ = *src_pixel++;
          }

      gimp_preview_cache_add (plist, preview);

      return preview;
    }

#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_get returning NULL\n");
#endif

  return NULL;
}

gsize
gimp_preview_cache_get_memsize (GSList *cache)
{
  GSList *list;
  gsize   memsize = 0;

  if (! cache)
    return 0;

  for (list = cache; list; list = list->next)
    memsize += sizeof (GSList) + gimp_temp_buf_get_memsize (list->data);

  return memsize;
}
