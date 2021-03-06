/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-11-21
 * Description : Qt Model for Tags - drag and drop handling
 *
 * Copyright (C) 2009 by Johannes Wienke <languitar at semipol dot de>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef TAGDRAGDROP_H
#define TAGDRAGDROP_H

// Local includes

#include "albummodeldragdrophandler.h"
#include "albummodel.h"

namespace Digikam
{

class TagDragDropHandler : public AlbumModelDragDropHandler
{
    Q_OBJECT

public:

    TagDragDropHandler(TagModel* model);

    TagModel* model() const
    {
        return static_cast<TagModel*>(m_model);
    }

    virtual bool dropEvent(QAbstractItemView* view, const QDropEvent* e, const QModelIndex& droppedOn);
    virtual Qt::DropAction accepts(const QDropEvent* e, const QModelIndex& dropIndex);
    virtual QStringList mimeTypes() const;
    virtual QMimeData* createMimeData(const QList<Album*>&);

Q_SIGNALS:

    void assignTags(const QList<qlonglong>& imageIDs, const QList<int>& tagIDs);
};

} // namespace Digikam

#endif /* TAGDRAGDROP_H */
