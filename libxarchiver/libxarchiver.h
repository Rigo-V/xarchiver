/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __LIBXARCHIVER_H__
#define __LIBXARCHIVER_H__


G_BEGIN_DECLS

void xarchiver_init();

int xarchiver_destroy();

XAArchive *xarchiver_archive_new(gchar *path, XAArchiveType type);
XASupport *xarchiver_find_archive_support(XAArchive *archive);
XASupport *xarchiver_find_type_support(XAArchiveType type);
void       xarchiver_all_support_connect(gchar *signal, GCallback fp);
void       xarchiver_support_connect(XASupport *support, gchar *signal, GCallback fp);
GSList *   xarchiver_get_supported_mimetypes(void);

G_END_DECLS

#endif /* __LIBXARCHIVER_H__ */
