/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-08-02
 * Description : class to get/set image collection
 *               information/properties using digiKam album database.
 *
 * Copyright (C) 2004-2005 by Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Copyright (C) 2004-2005 by Ralf Holzer <ralf at well.com>
 * Copyright (C) 2004-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef KIPIIMAGECOLLECTION_H
#define KIPIIMAGECOLLECTION_H

// Qt includes

#include <QString>
#include <QDate>

// KDE includes

#include <kurl.h>

// LibKIPI includes

#include <libkipi/imagecollection.h>
#include <libkipi/imagecollectionshared.h>

// Local includes

#include "albummanager.h"

namespace Digikam
{

class KipiImageCollection : public KIPI::ImageCollectionShared
{

public:

    enum Type
    {
        AllItems,
        SelectedItems
    };

public:

    KipiImageCollection(Type type, Album* const album, const QString& filter);
    ~KipiImageCollection();

    virtual QString name();
    virtual QString comment();
    virtual QString category();
    virtual QDate date();
    virtual KUrl::List images();
    virtual KUrl path();
    virtual KUrl uploadPath();
    virtual KUrl uploadRoot();
    virtual QString uploadRootName();
    virtual bool isDirectory();
    virtual bool operator==(ImageCollectionShared&);

private:

    KUrl::List imagesFromPAlbum(PAlbum* const album) const;
    KUrl::List imagesFromTAlbum(TAlbum* const album) const;

private:

    class KipiImageCollectionPriv;
    KipiImageCollectionPriv* const d;
};

}  // namespace Digikam

#endif  // KIPIIMAGECOLLECTION_H