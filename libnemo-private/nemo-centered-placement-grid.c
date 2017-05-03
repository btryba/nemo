/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* Copyright (C) 1999, 2000 Free Software Foundation
   Copyright (C) 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.
*/


#include "math.h"

#include <eel/eel-art-extensions.h>
#include "nemo-icon-private.h"
// #include "nemo-icon-info.h"
#include "nemo-file.h"

NemoCenteredPlacementGrid *
nemo_centered_placement_grid_new (NemoIconContainer *container, gboolean horizontal)
{
    NemoCenteredPlacementGrid *grid;
    gint width, height;
    gint num_columns;
    gint num_rows;
    gint i;
    gint snap_x, snap_y;
    gint icon_size;

    GtkAllocation allocation;

    /* Get container dimensions */
    gtk_widget_get_allocation (GTK_WIDGET (container), &allocation);

    icon_size = nemo_get_desktop_icon_size_for_zoom_level (container->details->zoom_level);

    width  = nemo_icon_container_get_canvas_width (container, allocation);
    height = nemo_icon_container_get_canvas_height (container, allocation);

    snap_x = GET_VIEW_CONSTANT (container, snap_size_x);
    snap_y = GET_VIEW_CONSTANT (container, snap_size_y);

    num_columns = width / snap_x;
    num_rows = height / snap_y;

    if (num_columns == 0 || num_rows == 0) {
        return NULL;
    }

    grid = g_new0 (NemoCenteredPlacementGrid, 1);
    grid->container = container;
    grid->horizontal = horizontal;
    grid->icon_size = icon_size;
    grid->real_snap_x = snap_x;
    grid->real_snap_y = snap_y;
    grid->num_columns = num_columns;
    grid->num_rows = num_rows;

    grid->borders = gtk_border_new ();

    grid->grid_memory = g_new0 (int, (num_rows * num_columns));
    grid->icon_grid = g_new0 (int *, num_columns);
    
    for (i = 0; i < num_columns; i++) {
        grid->icon_grid[i] = grid->grid_memory + (i * num_rows);
    }

    grid->borders->left = (width - (num_columns * snap_x)) / 2;
    grid->borders->right = (width - (num_columns * snap_x)) / 2;
    grid->borders->top = (height - (num_rows * snap_y)) / 2;
    grid->borders->bottom = (height - (num_rows * snap_y)) / 2;

    return grid;
}

void
nemo_centered_placement_grid_free (NemoCenteredPlacementGrid *grid)
{
    g_free (grid->icon_grid);
    g_free (grid->grid_memory);
    gtk_border_free (grid->borders);
    g_free (grid);
}

static gboolean
nemo_centered_placement_grid_position_is_free (NemoCenteredPlacementGrid *grid,
                                               gint                       grid_x,
                                               gint                       grid_y)
{
    g_assert (grid_x >= 0 && grid_x < grid->num_columns);
    g_assert (grid_y >= 0 && grid_y < grid->num_rows);

    return grid->icon_grid[grid_x][grid_y] == 0;
}

static void
nemo_centered_placement_grid_mark (NemoCenteredPlacementGrid *grid,
                                   gint                       grid_x,
                                   gint                       grid_y)
{
    g_assert (grid_x >= 0 && grid_x < grid->num_columns);
    g_assert (grid_y >= 0 && grid_y < grid->num_rows);

    grid->icon_grid[grid_x][grid_y] = 1;
}

static void
nemo_centered_placement_grid_unmark (NemoCenteredPlacementGrid *grid,
                                     gint                       grid_x,
                                     gint                       grid_y)
{
    g_assert (grid_x >= 0 && grid_x < grid->num_columns);
    g_assert (grid_y >= 0 && grid_y < grid->num_rows);

    grid->icon_grid[grid_x][grid_y] = 0;
}

void
nemo_centered_placement_grid_nominal_to_icon_position (NemoCenteredPlacementGrid *grid,
                                                       gint                       x_nominal,
                                                       gint                       y_nominal,
                                                       gint                      *x_adjusted,
                                                       gint                      *y_adjusted)
{
    *x_adjusted =   x_nominal
                  + (grid->real_snap_x / 2)
                  - (grid->icon_size   / 2);

    *y_adjusted =   y_nominal
                  + (grid->real_snap_y / 2)
                  - GET_VIEW_CONSTANT (grid->container, icon_vertical_adjust);
}

void
nemo_centered_placement_grid_icon_position_to_nominal (NemoCenteredPlacementGrid *grid,
                                                       gint                       x_adjusted,
                                                       gint                       y_adjusted,
                                                       gint                      *x_nominal,
                                                       gint                      *y_nominal)
{
    *x_nominal =   x_adjusted
                 + (grid->icon_size   / 2)
                 - (grid->real_snap_x / 2);

    *y_nominal =   y_adjusted
                 - (grid->real_snap_y / 2)
                 + GET_VIEW_CONSTANT (grid->container, icon_vertical_adjust);
}

void
nemo_centered_placement_grid_mark_icon (NemoCenteredPlacementGrid *grid, NemoIcon *icon)
{
    gint nom_x, nom_y, grid_x, grid_y;

    if (icon->x <= 0.0 && icon->y <= 0.0) {
        return;
    }

    nemo_centered_placement_grid_icon_position_to_nominal (grid,
                                                           icon->x,
                                                           icon->y,
                                                           &nom_x, &nom_y);

    grid_x = (nom_x - grid->borders->left) / grid->real_snap_x;
    grid_y = (nom_y - grid->borders->top) / grid->real_snap_y;

    if ((grid_x >= 0 && grid_x < grid->num_columns) &&
       (grid_y >= 0 && grid_y < grid->num_rows)) {
        nemo_centered_placement_grid_mark (grid, grid_x, grid_y);
    }
}

void
nemo_centered_placement_grid_unmark_icon (NemoCenteredPlacementGrid *grid,
                                          NemoIcon                  *icon)
{
    gint nom_x, nom_y, grid_x, grid_y;

    if (icon->x <= 0.0 && icon->y <= 0.0) {
        return;
    }

    nemo_centered_placement_grid_icon_position_to_nominal (grid,
                                                           icon->x,
                                                           icon->y,
                                                           &nom_x, &nom_y);

    grid_x = (nom_x - grid->borders->left) / grid->real_snap_x;
    grid_y = (nom_y - grid->borders->top) / grid->real_snap_y;

    if ((grid_x >= 0 && grid_x < grid->num_columns) &&
       (grid_y >= 0 && grid_y < grid->num_rows)) {
        nemo_centered_placement_grid_unmark (grid, grid_x, grid_y);
    }
}

void
nemo_centered_placement_grid_get_next_free_position (NemoCenteredPlacementGrid *grid,
                                                     gint                      *x_out,
                                                     gint                      *y_out)
{
    gint x, y, x_ret, y_ret;

    x_ret = -1;
    y_ret = -1;

    if (grid->horizontal) {
        for (y = 0; y < grid->num_rows; y++) {
            for (x = 0; x < grid->num_columns; x++) {
                if (nemo_centered_placement_grid_position_is_free (grid, x, y)) {
                    nemo_centered_placement_grid_mark (grid, x, y);
                    x_ret = grid->borders->left + (x * grid->real_snap_x);
                    y_ret = grid->borders->top + (y * grid->real_snap_y);
                    goto out;
                }
            }
        }
    } else {
        for (x = 0; x < grid->num_columns; x++) {
            for (y = 0; y < grid->num_rows; y++) {
                if (nemo_centered_placement_grid_position_is_free (grid, x, y)) {
                    nemo_centered_placement_grid_mark (grid, x, y);
                    x_ret = grid->borders->left + (x * grid->real_snap_x);
                    y_ret = grid->borders->top + (y * grid->real_snap_y);
                    goto out;
                }
            }
        }
    }

out:
    *x_out = x_ret;
    *y_out = y_ret;
}

void
nemo_centered_placement_grid_pre_populate (NemoCenteredPlacementGrid *grid,
                                           GList                     *icons)
{
    GList *p;
    NemoIcon *icon;

    for (p = icons; p != NULL; p = p->next) {
        icon = p->data;
        if (nemo_icon_container_icon_is_positioned (icon) && !icon->has_lazy_position) {
            nemo_centered_placement_grid_mark_icon (grid, icon);
        }
    }
}

void
nemo_centered_placement_grid_get_current_position_rect (NemoCenteredPlacementGrid *grid,
                                                        gint                       x,
                                                        gint                       y,
                                                        GdkRectangle              *rect,
                                                        gboolean                  *is_free)
{
    gint index_x, index_y;

    x -= grid->borders->left;
    y -= grid->borders->top;

    index_x = x / grid->real_snap_x;
    index_y = y / grid->real_snap_y;

    index_x = CLAMP (index_x, 0, grid->num_columns - 1);
    index_y = CLAMP (index_y, 0, grid->num_rows - 1);

    rect->x = index_x * grid->real_snap_x;
    rect->y = index_y * grid->real_snap_y;
    rect->width = grid->real_snap_x;
    rect->height = grid->real_snap_y;

    rect->x += grid->borders->left;
    rect->y += grid->borders->top;

    if (is_free) {
        *is_free = nemo_centered_placement_grid_position_is_free (grid, index_x, index_y);
    }
}

static NemoIcon *
get_icon_at_grid_position (NemoCenteredPlacementGrid *grid,
                           gint                       grid_x,
                           gint                       grid_y)
{
    NemoIcon *icon;
    GList *l;
    gint target_x, target_y, icon_x, icon_y;

    target_x = grid_x * grid->real_snap_x;
    target_y = grid_y * grid->real_snap_y;
    for (l = grid->container->details->icons; l != NULL; l = l->next) {
        icon = l->data;

        nemo_centered_placement_grid_icon_position_to_nominal (grid, icon->x, icon->y, &icon_x, &icon_y);

        icon_x -= grid->borders->left;
        icon_y -= grid->borders->top;

        if (target_x == icon_x && target_y == icon_y) {
            return icon;
        }
    }

    return NULL;
}

static void
get_next_grid_position (NemoCenteredPlacementGrid *grid,
                        gint                       grid_x_in,
                        gint                       grid_y_in,
                        gint                      *grid_x_out,
                        gint                      *grid_y_out)
{
    gint cur_grid_x, cur_grid_y;
    gboolean found_in;

    g_assert (grid_x_in >= 0 && grid_x_in < grid->num_columns);
    g_assert (grid_y_in >= 0 && grid_y_in < grid->num_rows);

    found_in = FALSE;

    if (grid->horizontal) {
        for (cur_grid_y = 0; cur_grid_y < grid->num_rows; cur_grid_y++) {
            for (cur_grid_x = 0; cur_grid_x < grid->num_columns; cur_grid_x++) {
                if (!found_in) {
                    if (grid_x_in == cur_grid_x && grid_y_in == cur_grid_y) {
                        found_in = TRUE;
                        continue;
                    }
                } else {
                    *grid_x_out = cur_grid_x;
                    *grid_y_out = cur_grid_y;
                    return;
                }
            }
        }
    } else {
        for (cur_grid_x = 0; cur_grid_x < grid->num_columns; cur_grid_x++) {
            for (cur_grid_y = 0; cur_grid_y < grid->num_rows; cur_grid_y++) {
                if (!found_in) {
                    if (grid_x_in == cur_grid_x && grid_y_in == cur_grid_y) {
                        found_in = TRUE;
                        continue;
                    }
                } else {
                    *grid_x_out = cur_grid_x;
                    *grid_y_out = cur_grid_y;
                    return;
                }
            }
        }
    }

    /* if we're at the end, return the last position */

    *grid_x_out = grid->num_columns - 1;
    *grid_y_out = grid->num_rows - 1;
}

GList *
nemo_centered_placement_grid_clear_grid_for_selection (NemoCenteredPlacementGrid *grid,
                                                       gint                       start_x,
                                                       gint                       start_y,
                                                       GList                     *drag_sel_list)
{
    NemoDragSelectionItem *item;
    NemoIcon *icon;
    GList *ret_list, *icon_list, *p;
    GdkRectangle grid_rect;
    gboolean is_free;
    gboolean insert_before, first_line;
    gint grid_x, grid_y, new_grid_x, new_grid_y, iter_x, iter_y, icon_new_x, icon_new_y;
    gint space_needed;

    /* Clear the grid of any items in the selection list */

    for (p = drag_sel_list; p != NULL; p = p->next) {
        item = p->data;
        icon = nemo_icon_container_get_icon_by_uri (grid->container, item->uri);

        if (icon == NULL) {
            continue;
        }

        nemo_centered_placement_grid_unmark_icon (grid, icon);
    }

    /* We need to have space_needed number of contiguous positions in the grid */
    space_needed = g_list_length (drag_sel_list);

    /* Find where we're starting from, whether inserting before or after the current
     * item */
    nemo_centered_placement_grid_get_current_position_rect (grid,
                                                            start_x,
                                                            start_y,
                                                            &grid_rect,
                                                            &is_free);

    grid_x = grid_rect.x / grid->real_snap_x;
    grid_y = grid_rect.y / grid->real_snap_y;

    insert_before = TRUE;

    if (!nemo_centered_placement_grid_position_is_free (grid, grid_x, grid_y)) {
        if (grid->horizontal) {
            insert_before = (start_x >= grid_rect.x) && (start_x < grid_rect.x + (grid_rect.width / 2));
        } else {
            insert_before = (start_y >= grid_rect.y) && (start_y < grid_rect.y + (grid_rect.height / 2));
        }
    }

    if (!insert_before) {
        get_next_grid_position (grid, grid_x, grid_y, &new_grid_x, &new_grid_y);
    } else {
        new_grid_x = grid_x;
        new_grid_y = grid_y;
    }

    /* Iterate thru the grid, starting at our insertion position, pick up existing icons that are in
     * the way until we have enough free space to lay down the original selection plus these icons. */

    first_line = TRUE;
    icon_list = NULL;

    if (grid->horizontal) {
        for (iter_y = new_grid_y; iter_y < grid->num_rows; iter_y++) {
            for (iter_x = first_line ? new_grid_x : 0; iter_x < grid->num_columns; iter_x++) {
                first_line = FALSE;

                if (nemo_centered_placement_grid_position_is_free (grid, iter_x, iter_y)) {
                    space_needed--;
                } else {
                    icon = get_icon_at_grid_position (grid, iter_x, iter_y);

                    icon_list = g_list_prepend (icon_list, icon);
                    nemo_centered_placement_grid_unmark (grid, iter_x, iter_y);
                }

                if (space_needed == 0) {
                    goto done_collecting;
                } else {
                    continue;
                }
            }
        }
    } else {
        for (iter_x = new_grid_x; iter_x < grid->num_columns; iter_x++) {
            for (iter_y = first_line ? new_grid_y : 0; iter_y < grid->num_rows; iter_y++) {
                first_line = FALSE;

                if (nemo_centered_placement_grid_position_is_free (grid, iter_x, iter_y)) {
                    space_needed--;
                } else {
                    icon = get_icon_at_grid_position (grid, iter_x, iter_y);

                    icon_list = g_list_prepend (icon_list, icon);
                    nemo_centered_placement_grid_unmark (grid, iter_x, iter_y);
                }

                if (space_needed == 0) {
                    goto done_collecting;
                } else {
                    continue;
                }
            }
        }
    }

    done_collecting:

    /* Now go thru the original selection list, and modify their target positions to our grid positions */

    icon_list = g_list_reverse (icon_list);

    iter_x = new_grid_x;
    iter_y = new_grid_y;

    for (p = drag_sel_list; p != NULL; p = p->next) {
        item = p->data;

        nemo_centered_placement_grid_nominal_to_icon_position (grid,
                                                               iter_x * grid->real_snap_x,
                                                               iter_y * grid->real_snap_y,
                                                               &icon_new_x,
                                                               &icon_new_y);

        item->got_icon_position = TRUE;
        item->icon_x = icon_new_x + grid->borders->left;
        item->icon_y = icon_new_y + grid->borders->top;

        get_next_grid_position (grid, iter_x, iter_y, &iter_x, &iter_y);
    }

    /* Build a NemoDragSelectionItem list from the NemoIcon list we picked up, setting the appropriate positions
     * following the original selection */

    ret_list = NULL;

    for (p = icon_list; p != NULL; p = p->next) {
        icon = p->data;

        nemo_centered_placement_grid_nominal_to_icon_position (grid,
                                                               iter_x * grid->real_snap_x,
                                                               iter_y * grid->real_snap_y,
                                                               &icon_new_x,
                                                               &icon_new_y);

        item = nemo_drag_selection_item_new ();

        item->uri = nemo_file_get_uri (NEMO_FILE (icon->data));
        item->got_icon_position = TRUE;

        item->icon_x = icon_new_x + grid->borders->left;
        item->icon_y = icon_new_y + grid->borders->top;

        item->icon_width = grid->icon_size;
        item->icon_height = grid->icon_size;

        ret_list = g_list_prepend (ret_list, item);

        get_next_grid_position (grid, iter_x, iter_y, &iter_x, &iter_y);
    }

    ret_list = g_list_reverse (ret_list);

    /* Return the 'push list' - extra icons to be moved, but behind the scenes, that won't be
     * selected once the drop is complete */

    return ret_list;
}

