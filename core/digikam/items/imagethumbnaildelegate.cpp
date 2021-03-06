/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-02-06
 * Description : thumbnail bar for images - the delegate
 *
 * Copyright (C) 2002-2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2010 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 * Copyright (C) 2002-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009-2010 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "imagethumbnaildelegate.moc"
#include "imagedelegatepriv.h"

// Qt includes

// KDE includes

#include <kdebug.h>

// Local includes

#include "albumsettings.h"
#include "imagecategorizedview.h"
#include "imagedelegateoverlay.h"
#include "imagemodel.h"
#include "imagefiltermodel.h"
#include "thumbnailloadthread.h"

namespace Digikam
{

class ImageThumbnailDelegate::ImageThumbnailDelegatePrivate : public ImageDelegate::ImageDelegatePrivate
{
public:

    ImageThumbnailDelegatePrivate()
    {
        flow                = QListView::LeftToRight;

        // switch off drawing of frames
        drawMouseOverFrame  = false;
        drawFocusFrame      = false;

        // switch off composing rating over background
        ratingOverThumbnail = true;
    }

    QListView::Flow flow;
    QRect           viewSize;

};

ImageThumbnailDelegate::ImageThumbnailDelegate(ImageCategorizedView* parent)
    : ImageDelegate(*new ImageThumbnailDelegatePrivate, parent)
{
}

ImageThumbnailDelegate::~ImageThumbnailDelegate()
{
}

void ImageThumbnailDelegate::setFlow(QListView::Flow flow)
{
    Q_D(ImageThumbnailDelegate);
    d->flow = flow;
}

void ImageThumbnailDelegate::setDefaultViewOptions(const QStyleOptionViewItem& option)
{
    Q_D(ImageThumbnailDelegate);
    // store before calling parent class
    d->viewSize = option.rect;
    ImageDelegate::setDefaultViewOptions(option);
}

int ImageThumbnailDelegate::maximumSize() const
{
    Q_D(const ImageThumbnailDelegate);
    return ThumbnailLoadThread::maximumThumbnailPixmapSize(true) + 2*d->radius + 2*d->margin;
}

int ImageThumbnailDelegate::minimumSize() const
{
    Q_D(const ImageThumbnailDelegate);
    return ThumbnailSize::Small + 2*d->radius + 2*d->margin;
}

bool ImageThumbnailDelegate::acceptsActivation(const QPoint& pos, const QRect& visualRect,
        const QModelIndex& index, QRect* activationRect) const
{
    // reuse implementation from grandparent
    return ItemViewImageDelegate::acceptsActivation(pos, visualRect, index, activationRect);
}

void ImageThumbnailDelegate::updateContentWidth()
{
    Q_D(ImageThumbnailDelegate);
    int maxSize;

    if (d->flow == QListView::LeftToRight)
    {
        maxSize = d->viewSize.height();
    }
    else
    {
        maxSize = d->viewSize.width();
    }

    d->thumbSize = ThumbnailLoadThread::thumbnailPixmapSize(true, maxSize - 2*d->radius - 2*d->margin);

    ImageDelegate::updateContentWidth();
}

void ImageThumbnailDelegate::updateRects()
{
    Q_D(ImageThumbnailDelegate);

    d->pixmapRect = QRect(d->margin, d->margin, d->contentWidth, d->contentWidth);
    d->rect = QRect(0, 0, d->contentWidth + 2*d->margin, d->contentWidth + 2*d->margin);

    if (AlbumSettings::instance()->getIconShowRating())
    {
        int top    = d->rect.bottom() - d->margin - d->starPolygonSize.height() - 2;
        d->ratingRect = QRect(d->margin, top, d->contentWidth, d->starPolygonSize.height());
    }

    if (d->flow == QListView::LeftToRight)
    {
        d->gridSize  = QSize(d->rect.width() + d->spacing, d->rect.height());
    }
    else
    {
        d->gridSize  = QSize(d->rect.width(), d->rect.height() + d->spacing);
    }
}

} // namespace Digikam
