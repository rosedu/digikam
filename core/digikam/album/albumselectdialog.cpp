/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-06-16
 * Description : a dialog to select a target album to download
 *               pictures from camera
 *
 * Copyright (C) 2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2006-2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "albumselectdialog.moc"

// Qt includes

#include <QLabel>
#include <QFrame>
#include <QLayout>
#include <QCursor>
#include <QGridLayout>
#include <QPixmap>
#include <QPointer>

// KDE includes


#include <klocale.h>
#include <kapplication.h>
#include <kstandarddirs.h>

// Local includes

#include "album.h"
#include "albummanager.h"
#include "albumselectwidget.h"
#include "albumthumbnailloader.h"
#include "collectionmanager.h"

namespace Digikam
{

class AlbumSelectDialog::AlbumSelectDialogPrivate
{

public:

    AlbumSelectDialogPrivate()
    {
        albumSel  = 0;
        searchBar = 0;
    }

    QString            newAlbumString;

    AlbumSelectWidget* albumSel;

    SearchTextBar*     searchBar;
};

AlbumSelectDialog::AlbumSelectDialog(QWidget* parent, PAlbum* albumToSelect,
                                     const QString& header,
                                     const QString& newAlbumString)
    : KDialog(parent), d(new AlbumSelectDialogPrivate)
{
    d->newAlbumString = newAlbumString;

    setCaption(i18n("Select Album"));
    setButtons(Help|Ok|Cancel);
    setDefaultButton(Ok);
    setHelp("targetalbumdialog.anchor", "digikam");
    enableButtonOk(false);

    // -------------------------------------------------------------

    QWidget* page     = new QWidget(this);
    setMainWidget(page);

    QGridLayout* grid = new QGridLayout(page);
    QLabel* logo      = new QLabel(page);
    logo->setPixmap(QPixmap(KStandardDirs::locate("data", "digikam/data/logo-digikam.png"))
                    .scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QLabel* message   = new QLabel(page);
    message->setWordWrap(true);

    if (!header.isEmpty())
    {
        message->setText(header);
    }

    d->albumSel = new AlbumSelectWidget(page, albumToSelect);

    grid->addWidget(logo,        0, 0, 1, 1);
    grid->addWidget(message,     1, 0, 1, 1);
    grid->addWidget(d->albumSel, 0, 1, 3, 1);
    grid->setColumnStretch(1, 10);
    grid->setRowStretch(2, 10);
    grid->setMargin(0);
    grid->setSpacing(KDialog::spacingHint());

    // -------------------------------------------------------------

    connect(d->albumSel, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotSelectionChanged()));

    // -------------------------------------------------------------

    resize(500, 500);
    slotSelectionChanged();
}

AlbumSelectDialog::~AlbumSelectDialog()
{
    delete d;
}

void AlbumSelectDialog::slotSelectionChanged()
{
    PAlbum* currentAlbum = d->albumSel->currentAlbum();

    if (!currentAlbum || (currentAlbum->isRoot()))
    {
        enableButtonOk(false);
        return;
    }

    enableButtonOk(true);
}

PAlbum* AlbumSelectDialog::selectAlbum(QWidget* parent,
                                       PAlbum* albumToSelect,
                                       const QString& header,
                                       const QString& newAlbumString)
{
    QPointer<AlbumSelectDialog> dlg = new AlbumSelectDialog(parent, albumToSelect, header, newAlbumString);

    if (dlg->exec() != KDialog::Accepted)
    {
        delete dlg;
        return 0;
    }

    PAlbum* selectedAlbum = dlg->d->albumSel->currentAlbum();

    if (!selectedAlbum || (selectedAlbum->isRoot()))
    {
        delete dlg;
        return 0;
    }

    delete dlg;
    return selectedAlbum;
}

}  // namespace Digikam
