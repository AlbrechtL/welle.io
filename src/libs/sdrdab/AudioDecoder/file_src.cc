/*
 * @class FileSrc
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 13, 2015
 *
 * @version 1.0 beta
 * @copyright Copyright (c) 2015 Kacper Patro
 * @par License
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "file_src.h"

FileSrc::FileSrc(const char *path):
    data_(new Data),
    path_(path) {
        data_->abstract_src = this;
        data_->player_data = NULL;
    }

FileSrc::~FileSrc() {
    delete data_;
}

void FileSrc::SetSrc(void *player_data) {
    data_->player_data = reinterpret_cast<Player::Data *>(player_data);

    data_->src = gst_element_factory_make("filesrc", "src");
    g_assert(data_->src);

    g_object_set(data_->src, "location", path_, NULL);

    gst_bin_add_many(GST_BIN(data_->player_data->pipeline),
            data_->src,
            NULL
            );
}

const char *FileSrc::name() const {
    return "file_src";
}

void FileSrc::LinkSrc() {
    g_assert(gst_element_link_many(
                data_->src,
                data_->player_data->iddemux,
                NULL)
            );
}

void FileSrc::ResetSrc() {
    return;
}
