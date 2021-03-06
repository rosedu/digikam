/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-12-05
 * Description : collection of basic models used for views in digikam
 *
 * Copyright (C) 2009-2010 by Johannes Wienke <languitar at semipol dot de>
 * Copyright (C) 2010 by Andi Clemens <andi dot clemens at gmx dot net>
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

#include "digikammodelcollection.moc"

// KDE includes

#include <kiconloader.h>

// Local settings

#include "albumsettings.h"

namespace Digikam
{

class DigikamModelCollection::DigikamModelCollectionPriv
{

public:

    DigikamModelCollectionPriv()
    {
        albumModel        = 0;
        tagModel          = 0;
        tagFilterModel    = 0;
        searchModel       = 0;
        dateAlbumModel    = 0;
        imageVersionModel = 0;
    }

    AlbumModel*         albumModel;
    TagModel*           tagModel;
    TagModel*           tagFilterModel;
    SearchModel*        searchModel;
    DateAlbumModel*     dateAlbumModel;
    ImageVersionsModel* imageVersionModel;
};

DigikamModelCollection::DigikamModelCollection() :
    d(new DigikamModelCollectionPriv)
{
    d->albumModel     = new AlbumModel(AlbumModel::IncludeRootAlbum);
    d->tagModel       = new TagModel(AbstractAlbumModel::IncludeRootAlbum);
    d->tagFilterModel = new TagModel(AbstractAlbumModel::IgnoreRootAlbum);
    d->tagFilterModel->setCheckable(true);
    d->tagFilterModel->setTristate(true);

    d->searchModel       = new SearchModel();
    d->dateAlbumModel    = new DateAlbumModel();
    d->imageVersionModel = new ImageVersionsModel();

    // set icons initially
    slotAlbumSettingsChanged();

    connect(AlbumSettings::instance(), SIGNAL(setupChanged()),
            this, SLOT(slotAlbumSettingsChanged()));
}

DigikamModelCollection::~DigikamModelCollection()
{
    delete d->tagModel;
    delete d->tagFilterModel;
    delete d->albumModel;
    delete d->searchModel;
    delete d->dateAlbumModel;
    delete d->imageVersionModel;
    delete d;
}

AlbumModel* DigikamModelCollection::getAlbumModel() const
{
    return d->albumModel;
}

TagModel* DigikamModelCollection::getTagModel() const
{
    return d->tagModel;
}

TagModel* DigikamModelCollection::getTagFilterModel() const
{
    return d->tagFilterModel;
}

SearchModel* DigikamModelCollection::getSearchModel() const
{
    return d->searchModel;
}

DateAlbumModel* DigikamModelCollection::getDateAlbumModel() const
{
    return d->dateAlbumModel;
}

ImageVersionsModel* DigikamModelCollection::getImageVersionsModel() const
{
    return d->imageVersionModel;
}

void DigikamModelCollection::slotAlbumSettingsChanged()
{
    d->dateAlbumModel->setPixmaps(
        SmallIcon(
            "view-calendar-list",
            AlbumSettings::instance()->getTreeViewIconSize()),
        SmallIcon(
            "view-calendar-month",
            AlbumSettings::instance()->getTreeViewIconSize()));
}

} // namespace Digikam
