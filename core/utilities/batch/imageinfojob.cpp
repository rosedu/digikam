/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-22-01
 * Description : digikamalbum KIO slave interface to get image
 *               info from database.
 *
 * Copyright (C) 2007-2011 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "imageinfojob.moc"

// Qt includes

#include <QString>
#include <QDataStream>

// KDE includes

#include <kio/job.h>
#include <kurl.h>
#include <kdebug.h>

// Local includes

#include "album.h"
#include "albummanager.h"
#include "albumsettings.h"
#include "imagelister.h"

namespace Digikam
{

class ImageInfoJob::ImageInfoJobPriv
{
public:

    ImageInfoJobPriv() :
        job(0)
    {
    }

    KIO::TransferJob* job;
};

ImageInfoJob::ImageInfoJob()
    : d(new ImageInfoJobPriv)
{
}

ImageInfoJob::~ImageInfoJob()
{
    if (d->job)
    {
        d->job->kill();
    }

    delete d;
}

void ImageInfoJob::allItemsFromAlbum(Album* album)
{
    if (d->job)
    {
        d->job->kill();
        d->job = 0;
    }

    if (!album)
    {
        return;
    }

    ImageLister lister;
    d->job = lister.startListJob(album->databaseUrl());

    connect(d->job, SIGNAL(finished(KJob*)),
            this, SLOT(slotResult(KJob*)));

    connect(d->job, SIGNAL(data(KIO::Job*, const QByteArray&)),
            this, SLOT(slotData(KIO::Job*, const QByteArray&)));
}

void ImageInfoJob::stop()
{
    if (d->job)
    {
        d->job->kill();
        d->job = 0;
    }
}

bool ImageInfoJob::isRunning()
{
    return d->job;
}

void ImageInfoJob::slotResult(KJob* job)
{
    d->job = 0;

    if (job->error())
    {
        kWarning() << "Failed to list url: " << job->errorString();
        return;
    }

    emit signalCompleted();
}

void ImageInfoJob::slotData(KIO::Job*, const QByteArray& data)
{
    if (data.isEmpty())
    {
        return;
    }

    ImageInfoList itemsList;
    QDataStream   ds(data);

    while (!ds.atEnd())
    {
        ImageListerRecord record;
        ds >> record;

        ImageInfo info(record);

        itemsList.append(info);
    }

    emit signalItemsInfo(itemsList);
}

}  // namespace Digikam
