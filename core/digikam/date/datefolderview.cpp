/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-04-27
 * Description : a folder view for date albums.
 *
 * Copyright (C) 2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2006-2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2009-2010 by Johannes Wienke <languitar at semipol dot de>
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

#include "datefolderview.moc"

// Qt includes

#include <QDateTime>
#include <QFont>
#include <QPainter>
#include <QStyle>
#include <QFileInfo>

// KDE includes


#include <kapplication.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <kcalendarsystem.h>
#include <kconfiggroup.h>
#include <kdebug.h>

// Local includes

#include "album.h"
#include "albumdb.h"
#include "albumsettings.h"
#include "albumtreeview.h"
#include "monthwidget.h"

namespace Digikam
{

class DateFolderView::DateFolderViewPriv
{
public:

    DateFolderViewPriv() :
        active(false),
        dateTreeView(0),
        monthview(0)
    {
    }

    bool               active;

    QString            selected;

    DateAlbumTreeView* dateTreeView;
    MonthWidget*       monthview;
};

DateFolderView::DateFolderView(QWidget* parent, DateAlbumModel* dateAlbumModel)
    : KVBox(parent), StateSavingObject(this),
      d(new DateFolderViewPriv)
{
    setObjectName("DateFolderView");

    d->dateTreeView = new DateAlbumTreeView(this);
    d->dateTreeView->setAlbumModel(dateAlbumModel);
    d->dateTreeView->setSelectAlbumOnClick(true);
    d->dateTreeView->setAlbumManagerCurrentAlbum(true);
    d->monthview    = new MonthWidget(this);

    connect(d->dateTreeView, SIGNAL(currentAlbumChanged(Album*)),
            this, SLOT(slotSelectionChanged(Album*)));

    // Loading of DAlbums may take longer that setting up the gui. Therefore
    // the first call to setActive may not set the current album in the album
    // manager as it is not yet loaded. To achieve this, we wait for loading
    // DAlbums and set the active album in the album manager if this tab is
    // active
    connect(AlbumManager::instance(), SIGNAL(signalAllDAlbumsLoaded()),
            this, SLOT(slotAllAlbumsLoaded()));
}

DateFolderView::~DateFolderView()
{
    saveState();
    delete d;
}

void DateFolderView::setImageModel(ImageFilterModel* model)
{
    d->monthview->setImageModel(model);
}

void DateFolderView::setActive(bool val)
{
    if (d->active == val)
    {
        return;
    }

    d->active = val;

    if (d->active)
    {
        AlbumManager::instance()->setCurrentAlbum(
            d->dateTreeView->currentAlbum());
        slotSelectionChanged(d->dateTreeView->currentAlbum());
    }
    else
    {
        d->monthview->setActive(false);
    }
}

void DateFolderView::slotSelectionChanged(Album* selectedAlbum)
{
    if (!d->active)
    {
        kDebug() << "Not active, returning without action";
        return;
    }

    d->monthview->setActive(false);

    DAlbum* dalbum = dynamic_cast<DAlbum*> (selectedAlbum);

    if (!dalbum)
    {
        return;
    }

    if (dalbum->range() == DAlbum::Month)
    {

        QDate date = dalbum->date();
        d->monthview->setActive(true);
        d->monthview->setYearMonth(date.year(), date.month());
    }
}

void DateFolderView::slotAllAlbumsLoaded()
{
    if (d->active)
    {
        AlbumManager::instance()->setCurrentAlbum(
            d->dateTreeView->currentAlbum());
        slotSelectionChanged(d->dateTreeView->currentAlbum());
    }
}

void DateFolderView::setConfigGroup(KConfigGroup group)
{
    StateSavingObject::setConfigGroup(group);
    d->dateTreeView->setConfigGroup(group);
}

void DateFolderView::doLoadState()
{
    d->dateTreeView->loadState();
}

void DateFolderView::doSaveState()
{
    d->dateTreeView->saveState();
}

void DateFolderView::gotoDate(const QDate& dt)
{

    kDebug() << "Going to date " << dt;

    QModelIndex dateIndex = d->dateTreeView->albumModel()->monthIndexForDate(dt);

    if (!dateIndex.isValid())
    {
        kDebug() << "Cannot find an album for date " << dt;
        return;
    }

    DAlbum* dateAlbum = d->dateTreeView->albumModel()->albumForIndex(dateIndex);

    if (!dateAlbum)
    {
        kWarning() << "Could not retrieve an album for index " << dateIndex;
        return;
    }

    kDebug() << "Got date album " << dateAlbum;

    d->dateTreeView->setCurrentAlbum(dateAlbum);

}

void DateFolderView::changeAlbumFromHistory(DAlbum* album)
{
    d->dateTreeView->setCurrentAlbum(album);
}

AlbumPointer<DAlbum> DateFolderView::currentAlbum() const
{
    return AlbumPointer<DAlbum> (d->dateTreeView->currentAlbum());
}

}  // namespace Digikam
