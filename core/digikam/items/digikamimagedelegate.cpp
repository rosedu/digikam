/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-04-19
 * Description : Qt item view for images - the delegate
 *
 * Copyright (C) 2002-2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2002-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009-2010 by Andi Clemens <andi dot clemens at gmx dot net>
 * Copyright (C) 2006-2010 by Marcel Wiesweg <marcel.wiesweg@gmx.de>
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

#include "digikamimagedelegate.moc"
#include "imagedelegatepriv.h"

// Qt includes

// KDE includes

#include <kdebug.h>
#include <KIconLoader>

// Local includes

#include "albumsettings.h"
#include "imagecategorydrawer.h"
#include "imagecategorizedview.h"
#include "imagedelegateoverlay.h"
#include "imagemodel.h"
#include "imagefiltermodel.h"
#include "digikamimagedelegatepriv.h"

namespace Digikam
{

DigikamImageDelegate::DigikamImageDelegate(ImageCategorizedView* parent)
    : ImageDelegate(*new DigikamImageDelegatePrivate, parent)
{
    Q_D(DigikamImageDelegate);
    d->init(this, parent);
}

DigikamImageDelegate::DigikamImageDelegate(DigikamImageDelegatePrivate& dd, ImageCategorizedView* parent)
    : ImageDelegate(dd, parent)
{
    Q_D(DigikamImageDelegate);
    d->init(this, parent);
}

void DigikamImageDelegatePrivate::init(DigikamImageDelegate* q, ImageCategorizedView* parent)
{
    categoryDrawer = new ImageCategoryDrawer(parent);

    QObject::connect(AlbumSettings::instance(), SIGNAL(setupChanged()),
                     q, SLOT(slotSetupChanged()));
}

DigikamImageDelegate::~DigikamImageDelegate()
{
}

void DigikamImageDelegate::updateRects()
{
    Q_D(DigikamImageDelegate);

    int y                              = d->margin;
    d->pixmapRect                      = QRect(d->margin, y, d->contentWidth, d->contentWidth);
    y                                  = d->pixmapRect.bottom();
    d->imageInformationRect            = QRect(d->margin, y, d->contentWidth, 0);
    const AlbumSettings* albumSettings = AlbumSettings::instance();

    const int iconSize = KIconLoader::SizeSmallMedium;
    d->pickLabelRect = QRect(d->margin, y, iconSize, iconSize);
    d->groupRect     = QRect(d->contentWidth - iconSize, y, iconSize, iconSize);

    if (albumSettings->getIconShowRating())
    {
        d->ratingRect = QRect(d->margin, y, d->contentWidth, d->starPolygonSize.height());
        y             = d->ratingRect.bottom();
    }

    if (albumSettings->getIconShowName())
    {
        d->nameRect = QRect(d->margin, y, d->contentWidth-d->margin, d->oneRowRegRect.height());
        y           = d->nameRect.bottom();
    }

    if (albumSettings->getIconShowComments())
    {
        d->commentsRect = QRect(d->margin, y, d->contentWidth, d->oneRowComRect.height());
        y               = d->commentsRect.bottom();
    }

    if (albumSettings->getIconShowDate())
    {
        d->dateRect = QRect(d->margin, y, d->contentWidth, d->oneRowXtraRect.height());
        y           = d->dateRect.bottom();
    }

    if (albumSettings->getIconShowModDate())
    {
        d->modDateRect = QRect(d->margin, y, d->contentWidth, d->oneRowXtraRect.height());
        y              = d->modDateRect.bottom();
    }

    if (albumSettings->getIconShowResolution())
    {
        d->resolutionRect = QRect(d->margin, y, d->contentWidth, d->oneRowXtraRect.height());
        y                 = d->resolutionRect.bottom() ;
    }

    if (albumSettings->getIconShowSize())
    {
        d->sizeRect = QRect(d->margin, y, d->contentWidth, d->oneRowXtraRect.height());
        y           = d->sizeRect.bottom();
    }

    if (albumSettings->getIconShowTags())
    {
        d->tagRect = QRect(d->margin, y, d->contentWidth, d->oneRowComRect.height());
        y          = d->tagRect.bottom();
    }

    d->imageInformationRect.setBottom(y);

    d->rect     = QRect(0, 0, d->contentWidth + 2*d->margin, y+d->margin+d->radius);
    d->gridSize = QSize(d->rect.width() + d->spacing, d->rect.height() + d->spacing);
}

} // namespace Digikam
